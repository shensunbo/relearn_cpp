#pragma once
#include "PayloadDefine.h"
#include <string>
#include <variant>

enum class MessageId {
    Temperature,
    Position,
    AppConfig,
    LogMessage,
};

enum class MessageMethod {
    GET,
    SET,
    GET_ACK,
    SET_ACK,
};

// 前向声明
template<MessageId ID>
struct MessagePayloadType;

// 便捷别名
template<MessageId ID>
using PayloadType = typename MessagePayloadType<ID>::type;

// 特化
template<> struct MessagePayloadType<MessageId::Temperature> { using type = int; };
template<> struct MessagePayloadType<MessageId::Position>    { using type = Point; };
template<> struct MessagePayloadType<MessageId::AppConfig>   { using type = Config; };
template<> struct MessagePayloadType<MessageId::LogMessage>  { using type = std::string; };

// 收集所有可能的 payload 类型
using AllPayloads = std::variant<
    PayloadType<MessageId::Temperature>,
    PayloadType<MessageId::Position>,
    PayloadType<MessageId::AppConfig>,
    PayloadType<MessageId::LogMessage>
>;