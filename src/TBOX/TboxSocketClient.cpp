#include "TboxSocketClient.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <chrono>

// ---------------------------------------------------------------------------
TboxSocketClient::TboxSocketClient(const std::string& socketPath)
    : mSocketPath(socketPath)
{
    mylog(MyLogLevel::I, "[TboxSocketClient] Created, socket path: %s",
          mSocketPath.c_str());
}

TboxSocketClient::~TboxSocketClient() {
    disconnect();
}

// ---------------------------------------------------------------------------
bool TboxSocketClient::connect() {
    mFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mFd < 0) {
        mylog(MyLogLevel::E, "[TboxSocketClient] socket() failed: %s", strerror(errno));
        return false;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(mFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        mylog(MyLogLevel::E, "[TboxSocketClient] connect() to %s failed: %s",
              mSocketPath.c_str(), strerror(errno));
        close(mFd);
        mFd = -1;
        return false;
    }

    mRunning = true;
    mHasResp = false;
    mReaderThread = std::thread([this]() { readerLoop(); });

    mylog(MyLogLevel::I, "[TboxSocketClient] Connected to %s (fd=%d)",
          mSocketPath.c_str(), mFd);
    return true;
}

// ---------------------------------------------------------------------------
void TboxSocketClient::disconnect() {
    if (!mRunning.exchange(false)) return;

    mylog(MyLogLevel::I, "[TboxSocketClient] Disconnecting...");

    if (mFd >= 0) {
        shutdown(mFd, SHUT_RDWR);
        close(mFd);
        mFd = -1;
    }

    // Unblock any sendCommand() waiting for a response
    {
        std::lock_guard<std::mutex> lk(mRespMutex);
        mPendingResp = "ERROR disconnected";
        mHasResp = true;
        mRespCv.notify_all();
    }

    if (mReaderThread.joinable()) mReaderThread.join();
    mylog(MyLogLevel::I, "[TboxSocketClient] Disconnected");
}

// ---------------------------------------------------------------------------
bool TboxSocketClient::sendLine(const std::string& line) {
    std::string msg = line + "\n";
    ssize_t n = send(mFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (n != static_cast<ssize_t>(msg.size())) {
        mylog(MyLogLevel::E, "[TboxSocketClient] sendLine failed: %s", strerror(errno));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// readerLoop — background thread
// ---------------------------------------------------------------------------
void TboxSocketClient::readerLoop() {
    mylog(MyLogLevel::I, "[TboxSocketClient] Reader thread started");

    std::string buf;
    while (mRunning) {
        char c;
        ssize_t n = recv(mFd, &c, 1, 0);
        if (n <= 0) {
            mylog(MyLogLevel::I, "[TboxSocketClient] Server closed connection (recv=%zd)", n);
            break;
        }
        if (c == '\r') continue;
        if (c != '\n') {
            buf += c;
            continue;
        }

        // Got a complete line
        std::string line = std::move(buf);
        buf.clear();

        mylog(MyLogLevel::D, "[TboxSocketClient] Received: \"%s\"", line.c_str());

        if (line.size() >= 7 && line.substr(0, 7) == "NOTIFY ") {
            // Format: "NOTIFY <signal> <value>"
            std::istringstream iss(line.substr(7));
            std::string signal, value;
            iss >> signal >> value;

            mylog(MyLogLevel::I,
                  "[TboxSocketClient] NOTIFY signal=\"%s\" value=\"%s\"",
                  signal.c_str(), value.c_str());

            std::lock_guard<std::mutex> lk(mCbMutex);
            auto it = mCallbacks.find(signal);
            if (it != mCallbacks.end()) {
                mylog(MyLogLevel::D,
                      "[TboxSocketClient] Dispatching to %zu callback(s) for %s",
                      it->second.size(), signal.c_str());
                for (auto& cb : it->second) {
                    try {
                        cb(signal, value);
                    } catch (const std::exception& e) {
                        mylog(MyLogLevel::E,
                              "[TboxSocketClient] Callback exception for %s: %s",
                              signal.c_str(), e.what());
                    }
                }
            } else {
                mylog(MyLogLevel::W,
                      "[TboxSocketClient] NOTIFY for unregistered signal: %s",
                      signal.c_str());
            }

        } else {
            // VALUE / OK / ERROR — response to the in-flight command
            mylog(MyLogLevel::D, "[TboxSocketClient] Response: \"%s\"", line.c_str());
            std::lock_guard<std::mutex> lk(mRespMutex);
            mPendingResp = std::move(line);
            mHasResp = true;
            mRespCv.notify_one();
        }
    }

    // Unblock any blocked sendCommand()
    {
        std::lock_guard<std::mutex> lk(mRespMutex);
        if (!mHasResp) {
            mPendingResp = "ERROR server closed";
            mHasResp = true;
            mRespCv.notify_all();
        }
    }

    mRunning = false;
    mylog(MyLogLevel::I, "[TboxSocketClient] Reader thread exited");
}

// ---------------------------------------------------------------------------
// sendCommand — serialised request-response
// ---------------------------------------------------------------------------
std::string TboxSocketClient::sendCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> cmdLk(mCmdMutex);  // one command at a time

    {
        std::lock_guard<std::mutex> lk(mRespMutex);
        mHasResp = false;
    }

    mylog(MyLogLevel::D, "[TboxSocketClient] Sending command: \"%s\"", cmd.c_str());

    if (!sendLine(cmd)) {
        return "ERROR send failed";
    }

    std::unique_lock<std::mutex> lk(mRespMutex);
    bool ok = mRespCv.wait_for(lk, std::chrono::seconds(3),
                                [this] { return mHasResp; });
    if (!ok) {
        mylog(MyLogLevel::E, "[TboxSocketClient] Timeout waiting for response to: %s",
              cmd.c_str());
        return "ERROR timeout";
    }

    std::string resp = std::move(mPendingResp);
    mHasResp = false;
    mylog(MyLogLevel::D, "[TboxSocketClient] Got response: \"%s\"", resp.c_str());
    return resp;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
std::string TboxSocketClient::get(const std::string& signal) {
    mylog(MyLogLevel::I, "[TboxSocketClient] GET %s", signal.c_str());
    std::string resp = sendCommand("GET " + signal);

    // Parse "VALUE <signal> <value>"
    if (resp.size() > 6 && resp.substr(0, 6) == "VALUE ") {
        std::istringstream iss(resp.substr(6));
        std::string sig, val;
        iss >> sig >> val;
        mylog(MyLogLevel::I, "[TboxSocketClient] GET %s = \"%s\"",
              signal.c_str(), val.c_str());
        return val;
    }
    mylog(MyLogLevel::E, "[TboxSocketClient] GET %s failed: %s",
          signal.c_str(), resp.c_str());
    return "";
}

bool TboxSocketClient::set(const std::string& signal, const std::string& value) {
    mylog(MyLogLevel::I, "[TboxSocketClient] SET %s = %s",
          signal.c_str(), value.c_str());
    std::string resp = sendCommand("SET " + signal + " " + value);
    bool ok = (resp == "OK");
    if (!ok) {
        mylog(MyLogLevel::E, "[TboxSocketClient] SET %s failed: %s",
              signal.c_str(), resp.c_str());
    }
    return ok;
}

bool TboxSocketClient::update(const std::string& signal, const std::string& value) {
    mylog(MyLogLevel::I, "[TboxSocketClient] UPDATE %s = %s",
          signal.c_str(), value.c_str());
    std::string resp = sendCommand("UPDATE " + signal + " " + value);
    bool ok = (resp == "OK");
    if (!ok) {
        mylog(MyLogLevel::E, "[TboxSocketClient] UPDATE %s failed: %s",
              signal.c_str(), resp.c_str());
    }
    return ok;
}

bool TboxSocketClient::subscribe(
    const std::string& signal,
    std::function<void(const std::string&, const std::string&)> cb)
{
    mylog(MyLogLevel::I, "[TboxSocketClient] SUBSCRIBE %s", signal.c_str());
    {
        std::lock_guard<std::mutex> lk(mCbMutex);
        mCallbacks[signal].push_back(std::move(cb));
        mylog(MyLogLevel::D,
              "[TboxSocketClient] Registered callback for %s (total=%zu)",
              signal.c_str(), mCallbacks[signal].size());
    }
    std::string resp = sendCommand("SUBSCRIBE " + signal);
    bool ok = (resp == "OK");
    if (!ok) {
        mylog(MyLogLevel::E, "[TboxSocketClient] SUBSCRIBE %s failed: %s",
              signal.c_str(), resp.c_str());
    } else {
        mylog(MyLogLevel::I, "[TboxSocketClient] SUBSCRIBE %s registered OK",
              signal.c_str());
    }
    return ok;
}
