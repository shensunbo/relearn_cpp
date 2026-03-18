#include "TboxService.h"
#include "mylog.h"

TboxService::TboxService()
    : mMsgToFdbus(std::make_shared<MessageQueue>()),
      mMsgFromFdbus(std::make_shared<MessageQueue>()),
      mAidlHandler(mMsgToFdbus, mMsgFromFdbus),
      mFdbusHandler(mMsgToFdbus, mMsgFromFdbus)
{
    mylog(MyLogLevel::I, "[TboxService] Constructed (workers not yet started)");
}

TboxService::~TboxService() {
    mylog(MyLogLevel::I, "[TboxService] Destructor called");
}

void TboxService::start() {
    mylog(MyLogLevel::I, "[TboxService] Starting worker threads");
    mAidlHandler.startWorker();
    mFdbusHandler.startWorker();
    mylog(MyLogLevel::I, "[TboxService] Worker threads started");
}