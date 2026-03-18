#pragma once
/**
 * TboxSocketClient — Unix domain socket IPC client.
 *
 * Connects to TboxSocketServer (socket path /tmp/tbox_service.sock by default).
 *
 * Usage:
 *   TboxSocketClient c;
 *   c.connect();
 *   auto val  = c.get("ServiceProvider");
 *   c.set("PrivateSwitchState", "1");     // AIDL path
 *   c.update("WANConnInfo", "5");          // FdBus-server path
 *   c.subscribe("ServiceProvider", [](auto& sig, auto& val) { ... });
 *
 * Incoming NOTIFY messages are dispatched to callbacks from the background
 * reader thread (keep callbacks non-blocking).
 */
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "mylog.h"

class TboxSocketClient {
public:
    static constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/tbox_service.sock";

    explicit TboxSocketClient(const std::string& socketPath = DEFAULT_SOCKET_PATH);
    ~TboxSocketClient();

    // Connect to server; starts background reader thread on success
    bool connect();
    void disconnect();
    bool isConnected() const { return mFd >= 0 && mRunning; }

    // -----------------------------------------------------------------------
    // Synchronous request-response (blocks for up to 3 s)
    // -----------------------------------------------------------------------
    // Query current value; returns value string, or empty string on error
    std::string get(const std::string& signal);

    // AIDL-writable (PrivateSwitchState only): returns true on OK
    bool set(const std::string& signal, const std::string& value);

    // FdBus-server update (any signal): returns true on OK
    bool update(const std::string& signal, const std::string& value);

    // Register for async NOTIFY; registers callback then sends SUBSCRIBE command
    // Callback signature: (signalName, newValue)
    bool subscribe(const std::string& signal,
                   std::function<void(const std::string& signal,
                                      const std::string& value)> cb);

private:
    std::string mSocketPath;
    int mFd{-1};
    std::atomic<bool> mRunning{false};

    // Background reader thread
    std::thread mReaderThread;

    // Response synchronisation (one outstanding command at a time)
    std::mutex mCmdMutex;   // serialises sendCommand() calls
    std::mutex mRespMutex;
    std::condition_variable mRespCv;
    std::string mPendingResp;
    bool mHasResp{false};

    // Subscription callbacks: signal -> [callbacks]
    std::mutex mCbMutex;
    std::map<std::string,
             std::vector<std::function<void(const std::string&,
                                            const std::string&)>>> mCallbacks;

    void readerLoop();
    bool sendLine(const std::string& line);

    // Sends cmd, waits for next VALUE/OK/ERROR response; returns response line
    std::string sendCommand(const std::string& cmd);
};
