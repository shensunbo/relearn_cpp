/*
How to add a new message type:
1. Define the payload struct/class for your message in PayloadDefine.h (or another suitable header).
2. Add a new entry to the MessageId enum (e.g., MyNewMessage).
3. Add a MessagePayloadType specialization for your new MessageId, setting 'type' to your payload type.
4. Add your new PayloadType to the AllPayloads std::variant.
5. (Optional) Add test cases for your new message type in msgQueueTest.cpp.

Example:
// In PayloadDefine.h:
struct MyPayload { int foo; std::string bar; };

// In MsgDefine.h:
enum class MessageId { ..., MyNewMessage };
template<> struct MessagePayloadType<MessageId::MyNewMessage> { using type = MyPayload; };
// Add PayloadType<MessageId::MyNewMessage> to AllPayloads.
*/

#pragma once
#include "PayloadDefine.h"
#include <string>
#include <variant>

enum class MessageId {
    Temperature,
    Position,
    AppConfig,
    LogMessage,
    UserInfo,
    DeviceStatus,
    DataPacket,
    Nested,
    Settings,
};

enum class MessageMethod {
    GET,
    SET,
    GET_ACK,
    SET_ACK,
};

// Forward declaration
template<MessageId ID>
struct MessagePayloadType;

// Convenient alias
template<MessageId ID>
using PayloadType = typename MessagePayloadType<ID>::type;

// Specializations
template<> struct MessagePayloadType<MessageId::Temperature> { using type = int; };
template<> struct MessagePayloadType<MessageId::Position>    { using type = Point; };
template<> struct MessagePayloadType<MessageId::AppConfig>   { using type = Config; };
template<> struct MessagePayloadType<MessageId::LogMessage>  { using type = std::string; };
template<> struct MessagePayloadType<MessageId::UserInfo>    { using type = User; };
template<> struct MessagePayloadType<MessageId::DeviceStatus> { using type = DeviceStatus; };
template<> struct MessagePayloadType<MessageId::DataPacket>  { using type = DataPacket; };
template<> struct MessagePayloadType<MessageId::Nested>      { using type = Nested; };
template<> struct MessagePayloadType<MessageId::Settings>    { using type = Settings; };

// Collect all possible payload types
using AllPayloads = std::variant<
    PayloadType<MessageId::Temperature>,
    PayloadType<MessageId::Position>,
    PayloadType<MessageId::AppConfig>,
    PayloadType<MessageId::LogMessage>,
    PayloadType<MessageId::UserInfo>,
    PayloadType<MessageId::DeviceStatus>,
    PayloadType<MessageId::DataPacket>,
    PayloadType<MessageId::Nested>,
    PayloadType<MessageId::Settings>
>;