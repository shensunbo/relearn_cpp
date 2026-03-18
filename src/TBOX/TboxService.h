#pragma once

#include "TboxAidlHandler.h"
#include "TboxFdbusHandler.h"
#include <memory>

class TboxService {
public:
    TboxService();
    ~TboxService();

    // Start worker threads in both handlers
    void start();

    // Accessors for testing / simulation
    TboxAidlHandler& getAidlHandler()   { return mAidlHandler; }
    TboxFdbusHandler& getFdbusHandler() { return mFdbusHandler; }

private:
    std::shared_ptr<MessageQueue> mMsgToFdbus;
    std::shared_ptr<MessageQueue> mMsgFromFdbus;
    TboxAidlHandler mAidlHandler;
    TboxFdbusHandler mFdbusHandler;
};