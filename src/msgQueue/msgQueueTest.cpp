#include "MsgQueue.h"
#include "MsgDefine.h"
#include "MsgUtils.h"
#include <iostream>
#include "mylog.h"


int main() {
    MessageQueue mq;

    // ✅ 合法：Temperature 注册为 int
    sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, 42);

    // ✅ 合法：Position 注册为 Point
    sendMessage<MessageId::Position>(mq, MessageMethod::SET, Point{10, 20});

    // ❌ 编译错误：如果未注册或类型不匹配
    // sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, "hot"); // error!

    // 接收并处理
    Message msg = mq.receive();

    // 安全提取 payload（基于 msg.id）
    if (msg.id == MessageId::Temperature) {
        int temp = std::get<int>(msg.payload);
        mylog(MyLogLevel::I, "Received Temperature message with payload: %d", temp);
    }
    // ... 其他类型

    return 0;
}