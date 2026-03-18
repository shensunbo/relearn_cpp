#pragma once
#include "mylog.h"
#include <thread>
#include <atomic>
#include "MessageQueue.h"
#include "FdbusServerMock.h"
#include "MsgDefine.h"

class TboxFdbusHandler {
public:
    TboxFdbusHandler(std::shared_ptr<MessageQueue> toFdbus, std::shared_ptr<MessageQueue> fromFdbus);
    ~TboxFdbusHandler();

    void startWorker();

    // Send a message toward TboxAidlHandler (puts on mMsgFromFdbus)
    template<MessageId ID>
    bool sendMessage(MessageMethod method, PayloadType<ID> payload) {
        if (!mMsgFromFdbus){
            mylog(MyLogLevel::E, "[FdbusHandler] Cannot send message, mMsgFromFdbus is null");
            return false;
        }
        Message msg{ID, method, std::move(payload)};
        mMsgFromFdbus->send(std::move(msg));
        mylog(MyLogLevel::D, "[FdbusHandler] Sent message id: %d", (int)ID);
        return true;
    }

    // Expose the mock so CLI / tests can update signals
    FdbusServerMock& getFdbusServer() { return mFdbusServer; }

private:
    std::shared_ptr<MessageQueue> mMsgToFdbus;
    std::shared_ptr<MessageQueue> mMsgFromFdbus;
    std::thread workerThread;
    std::atomic<bool> running{false};
    FdbusServerMock mFdbusServer;
};