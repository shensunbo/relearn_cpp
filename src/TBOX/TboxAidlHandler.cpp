#include "TboxAidlHandler.h"
#include "MessageQueue.h"
#include "mylog.h"
#include <atomic>
#include <thread>
#include <chrono>

TboxAidlHandler::TboxAidlHandler(std::shared_ptr<MessageQueue> toFdbus,
                                  std::shared_ptr<MessageQueue> fromFdbus)
    : mMsgToFdbus(toFdbus), mMsgFromFdbus(fromFdbus) {}

TboxAidlHandler::~TboxAidlHandler() {
    mylog(MyLogLevel::I, "[AidlHandler] Destructor called, stopping worker thread");
    running = false;
    if (mMsgFromFdbus) mMsgFromFdbus->stop();
    if (workerThread.joinable()) workerThread.join();
    mylog(MyLogLevel::I, "[AidlHandler] Worker thread joined");
}

void TboxAidlHandler::startWorker() {
    if (running) {
        mylog(MyLogLevel::W, "[AidlHandler] startWorker called but already running");
        return;
    }
    running = true;
    workerThread = std::thread([this]() {
        mylog(MyLogLevel::I, "[AidlHandler] Worker thread started");
        while (running) {
            auto msgOpt = mMsgFromFdbus->tryReceiveFor(std::chrono::milliseconds(200));
            if (!msgOpt) continue;

            const Message& msg = *msgOpt;
            mylog(MyLogLevel::I, "[AidlHandler] Worker received message id=%d method=%d",
                  (int)msg.id, (int)msg.method);

            switch (msg.id) {
                case MessageId::ServiceProvider: {
                    std::string sp = std::get<std::string>(msg.payload);
                    mylog(MyLogLevel::I, "[AidlHandler] Updating ServiceProvider: %s", sp.c_str());
                    mServiceProvider.set(sp);
                    mylog(MyLogLevel::I, "[AidlHandler] ServiceProvider updated and callbacks fired");
                    break;
                }
                case MessageId::WANConnInfo: {
                    WANConnInfoType info = std::get<WANConnInfoType>(msg.payload);
                    mylog(MyLogLevel::I, "[AidlHandler] Updating WANConnInfo: %d", (int)info);
                    mWANConnInfo.set(info);
                    mylog(MyLogLevel::I, "[AidlHandler] WANConnInfo updated and callbacks fired");
                    break;
                }
                case MessageId::CallInfo: {
                    CallInfoType call = std::get<CallInfoType>(msg.payload);
                    mylog(MyLogLevel::I, "[AidlHandler] Updating CallInfo: 0x%02x", (int)call);
                    mCallInfo.set(call);
                    mylog(MyLogLevel::I, "[AidlHandler] CallInfo updated and callbacks fired");
                    break;
                }
                case MessageId::PrivateswitchState: {
                    bool state = std::get<bool>(msg.payload);
                    mylog(MyLogLevel::I, "[AidlHandler] Updating PrivateSwitchState: %d", state);
                    mPrivateSwitchState.set(state);
                    mylog(MyLogLevel::I,
                          "[AidlHandler] PrivateSwitchState updated and callbacks fired");
                    break;
                }
                default:
                    mylog(MyLogLevel::W, "[AidlHandler] Unknown message id: %d, ignoring",
                          (int)msg.id);
                    break;
            }
        }
        mylog(MyLogLevel::I, "[AidlHandler] Worker thread exiting");
    });
}