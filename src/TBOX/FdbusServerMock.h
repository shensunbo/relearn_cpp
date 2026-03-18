#pragma once
#include <string>
#include <cstdint>
#include <functional>
#include <mutex>
#include "mylog.h"
#include "MsgDefine.h"

/**
 * FdbusServerMock — simulates the remote FdBus server.
 *
 * Responsibilities:
 *  - Holds the authoritative state for all signals.
 *  - updateXxx()  : called from CLI to simulate FdBus network changes;
 *                   fires SignalUpdateCallback so TboxFdbusHandler can
 *                   forward the new value to TboxAidlHandler via mMsgFromFdbus.
 *  - setPrivateSwitchState() : called by TboxFdbusHandler when AIDL has
 *                   written the PrivateSwitch (flow: AIDL → mMsgToFdbus →
 *                   TboxFdbusHandler → here).  Does NOT fire callback
 *                   (AIDL is the source; no echo back needed).
 *  - queryXxx()   : return current cached state.
 */
class FdbusServerMock {
public:
    // Callback signature: (MessageId, AllPayloads)
    using SignalUpdateCallback = std::function<void(MessageId, AllPayloads)>;

    FdbusServerMock() = default;

    // Register the callback that TboxFdbusHandler uses to forward updates
    void setSignalUpdateCallback(SignalUpdateCallback cb) {
        std::lock_guard<std::mutex> lk(mtx_);
        signalUpdateCallback_ = std::move(cb);
        mylog(MyLogLevel::I, "[FdbusServerMock] SignalUpdateCallback registered");
    }

    // -----------------------------------------------------------------------
    // Called by CLI to simulate FdBus-side signal changes
    // -----------------------------------------------------------------------
    void updateServiceProvider(const std::string& sp) {
        std::unique_lock<std::mutex> lk(mtx_);
        mylog(MyLogLevel::I, "[FdbusServerMock] updateServiceProvider: %s -> %s",
              serviceProvider_.c_str(), sp.c_str());
        serviceProvider_ = sp;
        auto cb = signalUpdateCallback_;
        lk.unlock();
        if (cb) {
            cb(MessageId::ServiceProvider, AllPayloads(serviceProvider_));
        }
    }

    void updateWANConnInfo(WANConnInfoType info) {
        std::unique_lock<std::mutex> lk(mtx_);
        mylog(MyLogLevel::I, "[FdbusServerMock] updateWANConnInfo: %d -> %d",
              (int)wanConnInfo_, (int)info);
        wanConnInfo_ = info;
        auto cb = signalUpdateCallback_;
        lk.unlock();
        if (cb) {
            cb(MessageId::WANConnInfo, AllPayloads(wanConnInfo_));
        }
    }

    void updateCallInfo(CallInfoType call) {
        std::unique_lock<std::mutex> lk(mtx_);
        mylog(MyLogLevel::I, "[FdbusServerMock] updateCallInfo: 0x%02x -> 0x%02x",
              (int)callInfo_, (int)call);
        callInfo_ = call;
        auto cb = signalUpdateCallback_;
        lk.unlock();
        if (cb) {
            cb(MessageId::CallInfo, AllPayloads(callInfo_));
        }
    }

    // -----------------------------------------------------------------------
    // Called by TboxFdbusHandler when AIDL sets PrivateSwitch.
    // Echoes the value back via signalUpdateCallback so TboxAidlHandler's
    // Attribute gets updated and subscribers are notified.
    // -----------------------------------------------------------------------
    void setPrivateSwitchState(bool state) {
        std::unique_lock<std::mutex> lk(mtx_);
        mylog(MyLogLevel::I, "[FdbusServerMock] setPrivateSwitchState (from AIDL): %d -> %d",
              (int)privateSwitchState_, (int)state);
        privateSwitchState_ = state;
        auto cb = signalUpdateCallback_;
        lk.unlock();
        // Echo back so TboxAidlHandler Attribute is updated and clients notified
        if (cb) {
            mylog(MyLogLevel::I,
                  "[FdbusServerMock] Echoing PrivateSwitchState back to AidlHandler: %d",
                  (int)state);
            cb(MessageId::PrivateswitchState, AllPayloads(state));
        }
    }

    // Called from CLI to simulate FdBus-side PrivateSwitch update
    void updatePrivateSwitchState(bool state) {
        std::unique_lock<std::mutex> lk(mtx_);
        mylog(MyLogLevel::I,
              "[FdbusServerMock] updatePrivateSwitchState (from FdBus CLI): %d -> %d",
              (int)privateSwitchState_, (int)state);
        privateSwitchState_ = state;
        auto cb = signalUpdateCallback_;
        lk.unlock();
        if (cb) {
            cb(MessageId::PrivateswitchState, AllPayloads(state));
        }
    }

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    std::string queryServiceProvider() const {
        std::lock_guard<std::mutex> lk(mtx_);
        mylog(MyLogLevel::D, "[FdbusServerMock] queryServiceProvider: %s",
              serviceProvider_.c_str());
        return serviceProvider_;
    }

    WANConnInfoType queryWANConnInfo() const {
        std::lock_guard<std::mutex> lk(mtx_);
        mylog(MyLogLevel::D, "[FdbusServerMock] queryWANConnInfo: %d", (int)wanConnInfo_);
        return wanConnInfo_;
    }

    CallInfoType queryCallInfo() const {
        std::lock_guard<std::mutex> lk(mtx_);
        mylog(MyLogLevel::D, "[FdbusServerMock] queryCallInfo: 0x%02x", (int)callInfo_);
        return callInfo_;
    }

    bool queryPrivateSwitchState() const {
        std::lock_guard<std::mutex> lk(mtx_);
        mylog(MyLogLevel::D, "[FdbusServerMock] queryPrivateSwitchState: %d",
              (int)privateSwitchState_);
        return privateSwitchState_;
    }

private:
    mutable std::mutex mtx_;
    std::string      serviceProvider_{"00000"};
    WANConnInfoType  wanConnInfo_{WANConnInfoType::NoNetwork};
    CallInfoType     callInfo_{CallInfoType::Idel};
    bool             privateSwitchState_{false};
    SignalUpdateCallback signalUpdateCallback_;
};