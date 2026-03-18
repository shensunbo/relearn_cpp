#pragma once
#include "MsgDefine.h"
#include <variant>

struct Message {
    MessageId id;
    MessageMethod method;
    AllPayloads payload;

    // Constructor template (type safe)
    template<MessageId ID>
    static Message make(MessageMethod m, PayloadType<ID> value) {
        return Message{ID, m, AllPayloads(std::move(value))};
    }

    // Safely get payload
    template<MessageId ID>
    const PayloadType<ID>& getPayload() const {
        return std::get<PayloadType<ID>>(payload);
    }
};