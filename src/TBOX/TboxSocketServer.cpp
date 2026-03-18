#include "TboxSocketServer.h"
#include "FdbusServerMock.h"
#include "mylog.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <any>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static std::string readLineFromFd(int fd) {
    std::string line;
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return ""; // connection closed or error
        if (c == '\n') return line;
        if (c != '\r') line += c;
    }
}

// ---------------------------------------------------------------------------
// ClientConn::writeLine
// ---------------------------------------------------------------------------
bool TboxSocketServer::ClientConn::writeLine(const std::string& line) {
    std::lock_guard<std::mutex> lk(writeMutex);
    if (closed) return false;
    std::string msg = line + "\n";
    ssize_t n = send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (n != static_cast<ssize_t>(msg.size())) {
        closed = true;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
TboxSocketServer::TboxSocketServer(TboxService& svc, const std::string& socketPath)
    : mSvc(svc), mSocketPath(socketPath)
{
    mylog(MyLogLevel::I, "[TboxSocketServer] Initialized, socket path: %s",
          mSocketPath.c_str());
}

TboxSocketServer::~TboxSocketServer() {
    stop();
    std::lock_guard<std::mutex> lk(mClientThreadsMutex);
    for (auto& t : mClientThreads) {
        if (t.joinable()) t.join();
    }
    mylog(MyLogLevel::I, "[TboxSocketServer] Destroyed");
}

// ---------------------------------------------------------------------------
// setupAttributeCallbacks — register one callback per Attribute at startup
// ---------------------------------------------------------------------------
void TboxSocketServer::setupAttributeCallbacks() {
    // ServiceProvider
    mSvc.getAidlHandler().mServiceProvider.addHandler(
        [this](const std::any& /*old*/, const std::any& newVal) {
            try {
                const auto& sp = std::any_cast<const std::string&>(newVal);
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] Attribute ServiceProvider changed -> \"%s\"",
                      sp.c_str());
                notifySubscribers("ServiceProvider", sp);
            } catch (const std::exception& e) {
                mylog(MyLogLevel::E,
                      "[TboxSocketServer] ServiceProvider callback error: %s", e.what());
            }
        });

    // WANConnInfo
    mSvc.getAidlHandler().mWANConnInfo.addHandler(
        [this](const std::any& /*old*/, const std::any& newVal) {
            try {
                auto info = std::any_cast<WANConnInfoType>(newVal);
                std::string val = std::to_string(static_cast<int>(info));
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] Attribute WANConnInfo changed -> %s", val.c_str());
                notifySubscribers("WANConnInfo", val);
            } catch (const std::exception& e) {
                mylog(MyLogLevel::E,
                      "[TboxSocketServer] WANConnInfo callback error: %s", e.what());
            }
        });

    // CallInfo
    mSvc.getAidlHandler().mCallInfo.addHandler(
        [this](const std::any& /*old*/, const std::any& newVal) {
            try {
                auto call = std::any_cast<CallInfoType>(newVal);
                std::string val = std::to_string(static_cast<int>(call));
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] Attribute CallInfo changed -> %s", val.c_str());
                notifySubscribers("CallInfo", val);
            } catch (const std::exception& e) {
                mylog(MyLogLevel::E,
                      "[TboxSocketServer] CallInfo callback error: %s", e.what());
            }
        });

    // PrivateSwitchState
    mSvc.getAidlHandler().mPrivateSwitchState.addHandler(
        [this](const std::any& /*old*/, const std::any& newVal) {
            try {
                bool state = std::any_cast<bool>(newVal);
                std::string val = state ? "1" : "0";
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] Attribute PrivateSwitchState changed -> %s",
                      val.c_str());
                notifySubscribers("PrivateSwitchState", val);
            } catch (const std::exception& e) {
                mylog(MyLogLevel::E,
                      "[TboxSocketServer] PrivateSwitchState callback error: %s", e.what());
            }
        });

    mylog(MyLogLevel::I, "[TboxSocketServer] All 4 attribute callbacks registered");
}

// ---------------------------------------------------------------------------
// notifySubscribers
// ---------------------------------------------------------------------------
void TboxSocketServer::notifySubscribers(const std::string& signal,
                                         const std::string& value) {
    std::string msg = "NOTIFY " + signal + " " + value;
    std::lock_guard<std::mutex> lk(mConnsMutex);
    int count = 0;
    for (auto& conn : mConns) {
        if (!conn->closed && conn->subscriptions.count(signal)) {
            mylog(MyLogLevel::D,
                  "[TboxSocketServer] Sending NOTIFY to fd=%d: %s", conn->fd, msg.c_str());
            if (conn->writeLine(msg)) ++count;
        }
    }
    mylog(MyLogLevel::I,
          "[TboxSocketServer] Notified %d subscriber(s): signal=%s value=%s",
          count, signal.c_str(), value.c_str());
}

// ---------------------------------------------------------------------------
// run — blocking accept loop
// ---------------------------------------------------------------------------
void TboxSocketServer::run() {
    mServerFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mServerFd < 0) {
        mylog(MyLogLevel::E, "[TboxSocketServer] socket() failed: %s", strerror(errno));
        return;
    }

    // Remove stale socket file
    unlink(mSocketPath.c_str());

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(mServerFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        mylog(MyLogLevel::E, "[TboxSocketServer] bind() failed: %s", strerror(errno));
        close(mServerFd);  mServerFd = -1;
        return;
    }

    if (listen(mServerFd, 10) < 0) {
        mylog(MyLogLevel::E, "[TboxSocketServer] listen() failed: %s", strerror(errno));
        close(mServerFd);  mServerFd = -1;
        return;
    }

    // Register attribute callbacks after socket is ready
    setupAttributeCallbacks();

    mRunning = true;
    mylog(MyLogLevel::I, "[TboxSocketServer] Listening on %s (fd=%d)",
          mSocketPath.c_str(), mServerFd);

    while (mRunning) {
        struct sockaddr_un clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(mServerFd,
                              reinterpret_cast<struct sockaddr*>(&clientAddr),
                              &clientLen);
        if (clientFd < 0) {
            if (mRunning) {
                mylog(MyLogLevel::E, "[TboxSocketServer] accept() failed: %s",
                      strerror(errno));
            }
            break;
        }

        mylog(MyLogLevel::I, "[TboxSocketServer] New client connected fd=%d", clientFd);

        auto conn = std::make_shared<ClientConn>();
        conn->fd = clientFd;

        {
            std::lock_guard<std::mutex> lk(mConnsMutex);
            mConns.push_back(conn);
        }

        std::lock_guard<std::mutex> tlk(mClientThreadsMutex);
        mClientThreads.emplace_back([this, conn]() { handleClient(conn); });
    }

    mylog(MyLogLevel::I, "[TboxSocketServer] Accept loop exited");
}

// ---------------------------------------------------------------------------
// stop
// ---------------------------------------------------------------------------
void TboxSocketServer::stop() {
    if (!mRunning.exchange(false)) return;
    mylog(MyLogLevel::I, "[TboxSocketServer] Stopping...");

    // Close all client connections
    {
        std::lock_guard<std::mutex> lk(mConnsMutex);
        for (auto& conn : mConns) {
            conn->closed = true;
            if (conn->fd >= 0) {
                shutdown(conn->fd, SHUT_RDWR);
                close(conn->fd);
                conn->fd = -1;
            }
        }
        mConns.clear();
    }

    // Unblock accept()
    if (mServerFd >= 0) {
        shutdown(mServerFd, SHUT_RDWR);
        close(mServerFd);
        mServerFd = -1;
    }
    unlink(mSocketPath.c_str());
    mylog(MyLogLevel::I, "[TboxSocketServer] Stopped");
}

// ---------------------------------------------------------------------------
// handleClient — runs in per-client thread
// ---------------------------------------------------------------------------
void TboxSocketServer::handleClient(std::shared_ptr<ClientConn> conn) {
    mylog(MyLogLevel::I, "[TboxSocketServer] Client thread started fd=%d", conn->fd);

    while (!conn->closed && mRunning) {
        std::string line = readLineFromFd(conn->fd);
        if (line.empty()) {
            mylog(MyLogLevel::I, "[TboxSocketServer] Client disconnected fd=%d", conn->fd);
            conn->closed = true;
            break;
        }

        mylog(MyLogLevel::I, "[TboxSocketServer] fd=%d << \"%s\"", conn->fd, line.c_str());

        std::string response = processCommand(line, conn);

        if (!response.empty()) {
            mylog(MyLogLevel::I, "[TboxSocketServer] fd=%d >> \"%s\"",
                  conn->fd, response.c_str());
            if (!conn->writeLine(response)) {
                mylog(MyLogLevel::W, "[TboxSocketServer] Write failed fd=%d", conn->fd);
                conn->closed = true;
                break;
            }
        }
    }

    // Cleanup
    if (conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
    }
    conn->closed = true;

    {
        std::lock_guard<std::mutex> lk(mConnsMutex);
        mConns.erase(std::remove_if(mConns.begin(), mConns.end(),
                                    [&conn](const auto& c) { return c == conn; }),
                     mConns.end());
    }

    mylog(MyLogLevel::I, "[TboxSocketServer] Client thread exited fd=%d", conn->fd);
}

// ---------------------------------------------------------------------------
// processCommand
// ---------------------------------------------------------------------------
std::string TboxSocketServer::processCommand(const std::string& line,
                                              std::shared_ptr<ClientConn> conn) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    // -----------------------------------------------------------------------
    if (cmd == "GET") {
        std::string signal;
        iss >> signal;
        mylog(MyLogLevel::I, "[TboxSocketServer] GET %s (fd=%d)", signal.c_str(), conn->fd);
        try {
            if (signal == "ServiceProvider") {
                auto v = mSvc.getAidlHandler().mServiceProvider.get<std::string>();
                mylog(MyLogLevel::I, "[TboxSocketServer] GET ServiceProvider = \"%s\"",
                      v.c_str());
                return "VALUE ServiceProvider " + v;

            } else if (signal == "WANConnInfo") {
                auto v = mSvc.getAidlHandler().mWANConnInfo.get<WANConnInfoType>();
                mylog(MyLogLevel::I, "[TboxSocketServer] GET WANConnInfo = %d", (int)v);
                return "VALUE WANConnInfo " + std::to_string(static_cast<int>(v));

            } else if (signal == "CallInfo") {
                auto v = mSvc.getAidlHandler().mCallInfo.get<CallInfoType>();
                mylog(MyLogLevel::I, "[TboxSocketServer] GET CallInfo = 0x%02x", (int)v);
                return "VALUE CallInfo " + std::to_string(static_cast<int>(v));

            } else if (signal == "PrivateSwitchState") {
                bool v = mSvc.getAidlHandler().mPrivateSwitchState.get<bool>();
                mylog(MyLogLevel::I, "[TboxSocketServer] GET PrivateSwitchState = %d",
                      (int)v);
                return std::string("VALUE PrivateSwitchState ") + (v ? "1" : "0");

            } else {
                mylog(MyLogLevel::W, "[TboxSocketServer] GET unknown signal: %s",
                      signal.c_str());
                return "ERROR unknown signal: " + signal;
            }
        } catch (const std::exception& e) {
            mylog(MyLogLevel::E, "[TboxSocketServer] GET %s error: %s",
                  signal.c_str(), e.what());
            return std::string("ERROR ") + e.what();
        }

    // -----------------------------------------------------------------------
    } else if (cmd == "SET") {
        // Only PrivateSwitchState is AIDL-writable
        std::string signal, value;
        iss >> signal >> value;
        mylog(MyLogLevel::I, "[TboxSocketServer] SET %s=%s (fd=%d)",
              signal.c_str(), value.c_str(), conn->fd);

        if (signal != "PrivateSwitchState") {
            mylog(MyLogLevel::W,
                  "[TboxSocketServer] SET rejected: %s is not AIDL-writable", signal.c_str());
            return "ERROR signal not writable by AIDL: " + signal;
        }

        bool state = (value == "1" || value == "true");
        mylog(MyLogLevel::I, "[TboxSocketServer] SET PrivateSwitchState -> %d", (int)state);
        bool ok = mSvc.getAidlHandler()
                      .sendMessage<MessageId::PrivateswitchState>(MessageMethod::SET, state);
        mylog(MyLogLevel::I, "[TboxSocketServer] SET PrivateSwitchState sent=%d", (int)ok);
        return ok ? "OK" : "ERROR send failed";

    // -----------------------------------------------------------------------
    } else if (cmd == "UPDATE") {
        // FdBus server side — any signal can be updated
        std::string signal, value;
        iss >> signal >> value;
        mylog(MyLogLevel::I, "[TboxSocketServer] UPDATE %s=%s (fd=%d)",
              signal.c_str(), value.c_str(), conn->fd);

        FdbusServerMock& srv = mSvc.getFdbusHandler().getFdbusServer();

        try {
            if (signal == "ServiceProvider") {
                srv.updateServiceProvider(value);
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] UPDATE ServiceProvider = \"%s\"", value.c_str());
                return "OK";

            } else if (signal == "WANConnInfo") {
                int v = std::stoi(value);
                srv.updateWANConnInfo(static_cast<WANConnInfoType>(v));
                mylog(MyLogLevel::I, "[TboxSocketServer] UPDATE WANConnInfo = %d", v);
                return "OK";

            } else if (signal == "CallInfo") {
                int v = std::stoi(value);
                srv.updateCallInfo(static_cast<CallInfoType>(v));
                mylog(MyLogLevel::I, "[TboxSocketServer] UPDATE CallInfo = 0x%02x", v);
                return "OK";

            } else if (signal == "PrivateSwitchState") {
                bool state = (value == "1" || value == "true");
                srv.updatePrivateSwitchState(state);
                mylog(MyLogLevel::I,
                      "[TboxSocketServer] UPDATE PrivateSwitchState (FdBus-side) = %d",
                      (int)state);
                return "OK";

            } else {
                mylog(MyLogLevel::W, "[TboxSocketServer] UPDATE unknown signal: %s",
                      signal.c_str());
                return "ERROR unknown signal: " + signal;
            }
        } catch (const std::exception& e) {
            mylog(MyLogLevel::E, "[TboxSocketServer] UPDATE %s error: %s",
                  signal.c_str(), e.what());
            return std::string("ERROR ") + e.what();
        }

    // -----------------------------------------------------------------------
    } else if (cmd == "SUBSCRIBE") {
        std::string signal;
        iss >> signal;
        mylog(MyLogLevel::I, "[TboxSocketServer] SUBSCRIBE %s (fd=%d)",
              signal.c_str(), conn->fd);

        static const std::set<std::string> valid{
            "ServiceProvider", "WANConnInfo", "CallInfo", "PrivateSwitchState"
        };
        if (!valid.count(signal)) {
            mylog(MyLogLevel::W, "[TboxSocketServer] SUBSCRIBE unknown signal: %s",
                  signal.c_str());
            return "ERROR unknown signal: " + signal;
        }

        conn->subscriptions.insert(signal);
        mylog(MyLogLevel::I, "[TboxSocketServer] fd=%d now subscribed to %s",
              conn->fd, signal.c_str());
        return "OK";

    // -----------------------------------------------------------------------
    } else {
        mylog(MyLogLevel::W, "[TboxSocketServer] Unknown command: \"%s\" (fd=%d)",
              cmd.c_str(), conn->fd);
        return "ERROR unknown command: " + cmd;
    }
}
