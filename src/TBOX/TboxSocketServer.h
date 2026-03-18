#pragma once
/**
 * TboxSocketServer — Unix domain socket IPC server embedded in TboxService process.
 *
 * Protocol (newline-terminated text):
 *
 *   Client → Server:
 *     GET <signal>                Query current attribute value
 *     SET <signal> <value>        AIDL-writable signals only (PrivateSwitchState)
 *     UPDATE <signal> <value>     FdBus-server-side signal update
 *     SUBSCRIBE <signal>          Register for async change notifications
 *
 *   Server → Client:
 *     VALUE <signal> <value>      Response to GET
 *     OK                          Success response to SET/UPDATE/SUBSCRIBE
 *     ERROR <reason>              Failure response
 *     NOTIFY <signal> <value>     Async notification (for SUBSCRIBE)
 *
 *   Signals:      ServiceProvider  WANConnInfo  CallInfo  PrivateSwitchState
 *   Encodings:    string           int(0-5)     int       0|1
 */
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <set>
#include <atomic>
#include <thread>
#include "TboxService.h"
#include "mylog.h"

class TboxSocketServer {
public:
    static constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/tbox_service.sock";

    explicit TboxSocketServer(TboxService& svc,
                              const std::string& socketPath = DEFAULT_SOCKET_PATH);
    ~TboxSocketServer();

    // Blocking — runs the accept loop until stop() is called
    void run();
    void stop();

private:
    // State for one connected client
    struct ClientConn {
        int fd{-1};
        std::mutex writeMutex;
        std::set<std::string> subscriptions;
        std::atomic<bool> closed{false};

        bool writeLine(const std::string& line);
    };

    TboxService&  mSvc;
    std::string   mSocketPath;
    int           mServerFd{-1};
    std::atomic<bool> mRunning{false};

    std::vector<std::shared_ptr<ClientConn>> mConns;
    std::mutex mConnsMutex;

    std::vector<std::thread> mClientThreads;
    std::mutex mClientThreadsMutex;

    // Register Attribute change handlers that fan out NOTIFY to subscribers
    void setupAttributeCallbacks();

    // Run in a dedicated thread for each accepted client
    void handleClient(std::shared_ptr<ClientConn> conn);

    // Dispatch one command line; return the response line to send back
    std::string processCommand(const std::string& cmd,
                               std::shared_ptr<ClientConn> conn);

    // Push NOTIFY <signal> <value> to every client subscribed to signal
    void notifySubscribers(const std::string& signal, const std::string& value);
};
