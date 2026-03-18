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
#include <string>
#include <variant>
//
// msg id and method definitions
//
enum class MessageId {
    ServiceProvider,
    WANConnInfo,
    CallInfo,
    PrivateswitchState,
};

enum class MessageMethod {
    GET,
    SET,
    GET_ACK,
    SET_ACK,
};

// 
// payload type definitions, support basic types, structs, and nested complex types
//

enum class WANConnInfoType {
    NoNetwork = 0,
    Connecting,
    Net2G,
    Net3G,
    Net4G,
    Net5G,
};

enum class CallInfoType {
    ICallIncoming = 0x01,
    ICallDialing = 0x03,
    ICallOutgoing = 0x04,
    Idel = 0x05,
    ECallIncoming = 0x06,
    ECallDialing = 0x07,
    ECallOutgoing = 0x08,
};


//
//  MessageId <-> Payload type mapping
//

// Forward declaration
template<MessageId ID>
struct MessagePayloadType;

// Convenient alias
template<MessageId ID>
using PayloadType = typename MessagePayloadType<ID>::type;

// Specializations
template<> struct MessagePayloadType<MessageId::ServiceProvider> { using type = std::string; };
template<> struct MessagePayloadType<MessageId::WANConnInfo>    { using type = WANConnInfoType; };
template<> struct MessagePayloadType<MessageId::CallInfo>   { using type = CallInfoType; };
template<> struct MessagePayloadType<MessageId::PrivateswitchState>  { using type = bool; };


// Collect all possible payload types
using AllPayloads = std::variant<
    PayloadType<MessageId::ServiceProvider>,
    PayloadType<MessageId::WANConnInfo>,
    PayloadType<MessageId::CallInfo>,
    PayloadType<MessageId::PrivateswitchState>
>;