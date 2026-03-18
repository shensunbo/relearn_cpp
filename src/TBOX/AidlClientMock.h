#pragma once
#include <functional>
#include <string>
#include <cstdint>
#include "TboxService.h"
#include "MsgDefine.h"

/**
 * AidlClientMock — simulates the AIDL client side.
 *
 * It owns a TboxService (started in constructor) and provides:
 *  - queryXxx()           : read the current Attribute value
 *  - subscribeXxx()       : register a change callback on the Attribute
 *  - setPrivateSwitchState() : the only AIDL-writable signal; flows
 *                             AIDL → mMsgToFdbus → FdbusHandler → FdbusServerMock
 *  - simulateXxx()        : helper to drive FdBus-side updates so that
 *                           subscription callbacks can be exercised in tests
 */
class AidlClientMock {
public:
    AidlClientMock();
    ~AidlClientMock() = default;

    // -----------------------------------------------------------------------
    // Query (read current Attribute value)
    // -----------------------------------------------------------------------
    std::string queryServiceProvider();
    uint8_t     queryWANConnInfo();
    uint8_t     queryCallInfo();
    bool        queryPrivateSwitchState();

    // -----------------------------------------------------------------------
    // Subscribe (register change callback)
    // -----------------------------------------------------------------------
    void subscribeServiceProvider(const std::function<void(const std::string&)>& callback);
    void subscribeWANConnInfo(const std::function<void(uint8_t)>& callback);
    void subscribeCallInfo(const std::function<void(uint8_t)>& callback);
    void subscribePrivateSwitchState(const std::function<void(bool)>& callback);

    // -----------------------------------------------------------------------
    // Set  (AIDL is the master only for PrivateSwitch)
    // -----------------------------------------------------------------------
    bool setPrivateSwitchState(bool state);

    // -----------------------------------------------------------------------
    // Simulate FdBus server updates (for testing subscriptions)
    // -----------------------------------------------------------------------
    void simulateServiceProviderUpdate(const std::string& sp);
    void simulateWANConnInfoUpdate(WANConnInfoType info);
    void simulateCallInfoUpdate(CallInfoType call);

private:
    TboxService tboxService; // Owns the full service pipeline
};