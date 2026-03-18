#include "AidlClientMock.h"
#include "mylog.h"
#include <any>
#include <stdexcept>

// ---------------------------------------------------------------------------
AidlClientMock::AidlClientMock() {
    mylog(MyLogLevel::I, "[AidlClientMock] Initialising — starting TboxService workers");
    tboxService.start();
    mylog(MyLogLevel::I, "[AidlClientMock] Ready");
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------
std::string AidlClientMock::queryServiceProvider() {
    mylog(MyLogLevel::I, "[AidlClientMock] queryServiceProvider");
    auto val = tboxService.getAidlHandler().mServiceProvider.get<std::string>();
    mylog(MyLogLevel::I, "[AidlClientMock] ServiceProvider = \"%s\"", val.c_str());
    return val;
}

uint8_t AidlClientMock::queryWANConnInfo() {
    mylog(MyLogLevel::I, "[AidlClientMock] queryWANConnInfo");
    auto val = tboxService.getAidlHandler().mWANConnInfo.get<WANConnInfoType>();
    mylog(MyLogLevel::I, "[AidlClientMock] WANConnInfo = %d", (int)val);
    return static_cast<uint8_t>(val);
}

uint8_t AidlClientMock::queryCallInfo() {
    mylog(MyLogLevel::I, "[AidlClientMock] queryCallInfo");
    auto val = tboxService.getAidlHandler().mCallInfo.get<CallInfoType>();
    mylog(MyLogLevel::I, "[AidlClientMock] CallInfo = 0x%02x", (int)val);
    return static_cast<uint8_t>(val);
}

bool AidlClientMock::queryPrivateSwitchState() {
    mylog(MyLogLevel::I, "[AidlClientMock] queryPrivateSwitchState");
    auto val = tboxService.getAidlHandler().mPrivateSwitchState.get<bool>();
    mylog(MyLogLevel::I, "[AidlClientMock] PrivateSwitchState = %d", (int)val);
    return val;
}

// ---------------------------------------------------------------------------
// Subscribe helpers — wrap std::any callbacks to typed lambdas
// ---------------------------------------------------------------------------
void AidlClientMock::subscribeServiceProvider(
    const std::function<void(const std::string&)>& callback)
{
    mylog(MyLogLevel::I, "[AidlClientMock] subscribeServiceProvider — adding handler");
    tboxService.getAidlHandler().mServiceProvider.addHandler(
        [callback](const std::any& /*old*/, const std::any& newVal) {
            try {
                const auto& val = std::any_cast<const std::string&>(newVal);
                mylog(MyLogLevel::I,
                      "[AidlClientMock] ServiceProvider changed → \"%s\"", val.c_str());
                callback(val);
            } catch (const std::bad_any_cast& e) {
                mylog(MyLogLevel::E,
                      "[AidlClientMock] ServiceProvider bad_any_cast: %s", e.what());
            }
        });
}

void AidlClientMock::subscribeWANConnInfo(const std::function<void(uint8_t)>& callback) {
    mylog(MyLogLevel::I, "[AidlClientMock] subscribeWANConnInfo — adding handler");
    tboxService.getAidlHandler().mWANConnInfo.addHandler(
        [callback](const std::any& /*old*/, const std::any& newVal) {
            try {
                auto val = std::any_cast<WANConnInfoType>(newVal);
                mylog(MyLogLevel::I,
                      "[AidlClientMock] WANConnInfo changed → %d", (int)val);
                callback(static_cast<uint8_t>(val));
            } catch (const std::bad_any_cast& e) {
                mylog(MyLogLevel::E,
                      "[AidlClientMock] WANConnInfo bad_any_cast: %s", e.what());
            }
        });
}

void AidlClientMock::subscribeCallInfo(const std::function<void(uint8_t)>& callback) {
    mylog(MyLogLevel::I, "[AidlClientMock] subscribeCallInfo — adding handler");
    tboxService.getAidlHandler().mCallInfo.addHandler(
        [callback](const std::any& /*old*/, const std::any& newVal) {
            try {
                auto val = std::any_cast<CallInfoType>(newVal);
                mylog(MyLogLevel::I,
                      "[AidlClientMock] CallInfo changed → 0x%02x", (int)val);
                callback(static_cast<uint8_t>(val));
            } catch (const std::bad_any_cast& e) {
                mylog(MyLogLevel::E,
                      "[AidlClientMock] CallInfo bad_any_cast: %s", e.what());
            }
        });
}

void AidlClientMock::subscribePrivateSwitchState(
    const std::function<void(bool)>& callback)
{
    mylog(MyLogLevel::I, "[AidlClientMock] subscribePrivateSwitchState — adding handler");
    tboxService.getAidlHandler().mPrivateSwitchState.addHandler(
        [callback](const std::any& /*old*/, const std::any& newVal) {
            try {
                bool val = std::any_cast<bool>(newVal);
                mylog(MyLogLevel::I,
                      "[AidlClientMock] PrivateSwitchState changed → %d", (int)val);
                callback(val);
            } catch (const std::bad_any_cast& e) {
                mylog(MyLogLevel::E,
                      "[AidlClientMock] PrivateSwitchState bad_any_cast: %s", e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// Set — only PrivateSwitch is AIDL-writable
// ---------------------------------------------------------------------------
bool AidlClientMock::setPrivateSwitchState(bool state) {
    mylog(MyLogLevel::I, "[AidlClientMock] setPrivateSwitchState → %d", (int)state);
    bool ok = tboxService.getAidlHandler()
                         .sendMessage<MessageId::PrivateswitchState>(
                             MessageMethod::SET, state);
    mylog(MyLogLevel::I, "[AidlClientMock] setPrivateSwitchState sent=%d", (int)ok);
    return ok;
}

// ---------------------------------------------------------------------------
// Simulate FdBus server-side updates (testing helper)
// ---------------------------------------------------------------------------
void AidlClientMock::simulateServiceProviderUpdate(const std::string& sp) {
    mylog(MyLogLevel::I,
          "[AidlClientMock] simulateServiceProviderUpdate → \"%s\"", sp.c_str());
    tboxService.getFdbusHandler().getFdbusServer().updateServiceProvider(sp);
}

void AidlClientMock::simulateWANConnInfoUpdate(WANConnInfoType info) {
    mylog(MyLogLevel::I, "[AidlClientMock] simulateWANConnInfoUpdate → %d", (int)info);
    tboxService.getFdbusHandler().getFdbusServer().updateWANConnInfo(info);
}

void AidlClientMock::simulateCallInfoUpdate(CallInfoType call) {
    mylog(MyLogLevel::I, "[AidlClientMock] simulateCallInfoUpdate → 0x%02x", (int)call);
    tboxService.getFdbusHandler().getFdbusServer().updateCallInfo(call);
}
