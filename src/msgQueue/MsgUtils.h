#pragma once
#include "MsgQueue.h"

template<MessageId ID>
void sendMessage(MessageQueue& q, MessageMethod method, PayloadType<ID> payload) {
    Message msg{ID, method, std::move(payload)};
    q.send(std::move(msg));
}