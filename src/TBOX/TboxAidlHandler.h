#pragma once

#include "MessageQueue.h"
#include "Attribute.h"
#include <thread>
#include <atomic>

class TboxAidlHandler {
public:
    TboxAidlHandler(std::shared_ptr<MessageQueue> toFdbus, std::shared_ptr<MessageQueue> fromFdbus);
    ~TboxAidlHandler();

    // receive msg from Fdbus handler
    void startWorker();
    
    template<MessageId ID>
    bool sendMessage(MessageMethod method, PayloadType<ID> payload) {
        if (!mMsgToFdbus){
            mylog(MyLogLevel::E, "[TboxAidlHandler] Cannot send message, mMsgToFdbus is null");
            return false;
        }
        Message msg{ID, method, std::move(payload)};
        mMsgToFdbus->send(std::move(msg));
        mylog(MyLogLevel::D, "[TboxAidlHandler] Sent message id: %d", (int)ID);
        return true;
    }


    // attribute definition
    Attribute mServiceProvider{std::string("00000")}; 
    Attribute mWANConnInfo{WANConnInfoType::NoNetwork};
    Attribute mCallInfo{CallInfoType::Idel};
    Attribute mPrivateSwitchState{false};

private:
    std::shared_ptr<MessageQueue> mMsgToFdbus;
    std::shared_ptr<MessageQueue> mMsgFromFdbus;
    std::thread workerThread;
    std::atomic<bool> running{false};
};