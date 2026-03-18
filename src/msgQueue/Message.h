#pragma once
#include "MsgDefine.h"
#include <variant>

struct Message {
    MessageId id;
    MessageMethod method;
    AllPayloads payload;

    // 构造函数模板（类型安全）
    template<MessageId ID>
    static Message make(MessageMethod m, PayloadType<ID> value) {
        return Message{ID, m, AllPayloads(std::move(value))};
    }

    // 安全获取 payload
    template<MessageId ID>
    const PayloadType<ID>& getPayload() const {
        return std::get<PayloadType<ID>>(payload);
    }
};