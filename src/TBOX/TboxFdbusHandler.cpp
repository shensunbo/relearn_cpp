#include "TboxFdbusHandler.h"
#include "MessageQueue.h"
#include "mylog.h"
#include <atomic>
#include <thread>
#include <chrono>

TboxFdbusHandler::TboxFdbusHandler(std::shared_ptr<MessageQueue> toFdbus,
                                   std::shared_ptr<MessageQueue> fromFdbus)
    : mMsgToFdbus(toFdbus), mMsgFromFdbus(fromFdbus)
{
    // Register the FdBus-to-AIDL signal callback.
    // When FdbusServerMock::updateXxx() is called (e.g. from CLI),
    // this callback puts the new value onto mMsgFromFdbus so
    // TboxAidlHandler can update its Attributes and fire subscriptions.
    mFdbusServer.setSignalUpdateCallback(
        [this](MessageId id, AllPayloads payload) {
            mylog(MyLogLevel::I,
                  "[FdbusHandler] FdBus signal update, forwarding to AidlHandler, id=%d",
                  (int)id);
            if (!mMsgFromFdbus) {
                mylog(MyLogLevel::E,
                      "[FdbusHandler] mMsgFromFdbus is null, cannot forward signal id=%d",
                      (int)id);
                return;
            }
            Message msg{id, MessageMethod::SET, std::move(payload)};
            mMsgFromFdbus->send(std::move(msg));
            mylog(MyLogLevel::D, "[FdbusHandler] Signal id=%d forwarded to mMsgFromFdbus",
                  (int)id);
        });
}

TboxFdbusHandler::~TboxFdbusHandler() {
    mylog(MyLogLevel::I, "[FdbusHandler] Destructor called, stopping worker thread");
    running = false;
    if (mMsgToFdbus) mMsgToFdbus->stop();
    if (workerThread.joinable()) workerThread.join();
    mylog(MyLogLevel::I, "[FdbusHandler] Worker thread joined");
}

// Worker thread: receives AIDL→FdBus messages from mMsgToFdbus
void TboxFdbusHandler::startWorker() {
    if (running) {
        mylog(MyLogLevel::W, "[FdbusHandler] startWorker called but already running");
        return;
    }
    running = true;
    workerThread = std::thread([this]() {
        mylog(MyLogLevel::I, "[FdbusHandler] Worker thread started");
        while (running) {
            auto msgOpt = mMsgToFdbus->tryReceiveFor(std::chrono::milliseconds(200));
            if (!msgOpt) continue;

            const Message& msg = *msgOpt;
            mylog(MyLogLevel::I, "[FdbusHandler] Worker received message id=%d method=%d",
                  (int)msg.id, (int)msg.method);

            switch (msg.id) {
                case MessageId::PrivateswitchState: {
                    bool state = std::get<bool>(msg.payload);
                    mylog(MyLogLevel::I,
                          "[FdbusHandler] Received PrivateSwitchState from AIDL: %d", state);
                    mFdbusServer.setPrivateSwitchState(state);
                    mylog(MyLogLevel::I,
                          "[FdbusHandler] FdbusServerMock PrivateSwitchState updated to: %d",
                          state);
                    break;
                }
                default:
                    mylog(MyLogLevel::W, "[FdbusHandler] Unexpected message id: %d, ignoring",
                          (int)msg.id);
                    break;
            }
        }
        mylog(MyLogLevel::I, "[FdbusHandler] Worker thread exiting");
    });
}
