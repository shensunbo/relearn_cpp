
#include "MsgQueue.h"
#include "MsgDefine.h"
#include "MsgUtils.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "mylog.h"


// Forward declarations
void test_basic();
void test_type_safety();
void test_multithreaded();
void test_stress();
void test_interactive();


int main() {
    mylog(MyLogLevel::I, "==== MessageQueue Test Begin ====");
    test_basic();
    test_type_safety();
    test_multithreaded();
    test_stress();
    test_interactive();
    mylog(MyLogLevel::I, "==== MessageQueue Test End ====");
    return 0;
}
// Interactive test: user modifies message value via command line, receiver prints both operation and received message
void test_interactive() {
    mylog(MyLogLevel::I, "[Test] Interactive user input test...");
    MessageQueue mq;
    std::atomic<bool> running{true};
    std::thread receiver([&]() {
        while (running) {
            Message msg = mq.receive();
            switch (msg.id) {
                case MessageId::Temperature: {
                    int temp = std::get<int>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received Temperature: %d", temp);
                    break;
                }
                case MessageId::LogMessage: {
                    std::string s = std::get<std::string>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received LogMessage: %s", s.c_str());
                    break;
                }
                case MessageId::Position: {
                    Point pt = std::get<Point>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received Position: (%d, %d)", pt.x, pt.y);
                    break;
                }
                case MessageId::UserInfo: {
                    User u = std::get<User>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received UserInfo: id=%d, name=%s, friends=%zu", u.id, u.name.c_str(), u.friends.size());
                    break;
                }
                case MessageId::DeviceStatus: {
                    DeviceStatus d = std::get<DeviceStatus>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received DeviceStatus: name=%s, online=%d, battery=%.1f", d.deviceName.c_str(), d.online, d.battery);
                    break;
                }
                case MessageId::AppConfig: {
                    Config cfg = std::get<Config>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received AppConfig: setting1=%d, setting2=%d", cfg.setting1, cfg.setting2);
                    break;
                }
                case MessageId::DataPacket: {
                    DataPacket p = std::get<DataPacket>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received DataPacket: seq=%d, size=%zu", p.seq, p.data.size());
                    break;
                }
                case MessageId::Nested: {
                    Nested n = std::get<Nested>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received Nested: user=%s, device=%s, packets=%zu", n.user.name.c_str(), n.status.deviceName.c_str(), n.packets.size());
                    break;
                }
                case MessageId::Settings: {
                    Settings s = std::get<Settings>(msg.payload);
                    mylog(MyLogLevel::I, "[Receiver] Received Settings: kv size=%zu", s.kv.size());
                    break;
                }
                default:
                    mylog(MyLogLevel::I, "[Receiver] Received unknown message id: %d", (int)msg.id);
            }
        }
    });

    while (true) {
        std::cout << "\n==== Message Send Menu ====" << std::endl;
        std::cout << "1. Send Temperature (int)" << std::endl;
        std::cout << "2. Send LogMessage (string)" << std::endl;
        std::cout << "3. Send Position (x y)" << std::endl;
        std::cout << "4. Send UserInfo (id name)" << std::endl;
        std::cout << "5. Send DeviceStatus (name online(0/1) battery)" << std::endl;
        std::cout << "6. Quit" << std::endl;
        std::cout << ">> Choice: ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "6" || line == "q") {
            running = false;
            sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, 0);
            break;
        }
        if (line == "1") {
            std::cout << "Enter int value: ";
            int val = 0;
            std::cin >> val; std::cin.ignore();
            mylog(MyLogLevel::I, "[User] Sending Temperature: %d", val);
            sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, val);
        } else if (line == "2") {
            std::cout << "Enter log message: ";
            std::string msg;
            std::getline(std::cin, msg);
            mylog(MyLogLevel::I, "[User] Sending LogMessage: %s", msg.c_str());
            sendMessage<MessageId::LogMessage>(mq, MessageMethod::SET, msg);
        } else if (line == "3") {
            std::cout << "Enter x y: ";
            int x, y;
            std::cin >> x >> y; std::cin.ignore();
            mylog(MyLogLevel::I, "[User] Sending Position: (%d, %d)", x, y);
            sendMessage<MessageId::Position>(mq, MessageMethod::SET, Point{x, y});
        } else if (line == "4") {
            std::cout << "Enter id and name: ";
            int id; std::string name;
            std::cin >> id >> name; std::cin.ignore();
            mylog(MyLogLevel::I, "[User] Sending UserInfo: id=%d, name=%s", id, name.c_str());
            sendMessage<MessageId::UserInfo>(mq, MessageMethod::SET, User{id, name, {}});
        } else if (line == "5") {
            std::cout << "Enter deviceName online(0/1) battery: ";
            std::string dev; int online; double bat;
            std::cin >> dev >> online >> bat; std::cin.ignore();
            mylog(MyLogLevel::I, "[User] Sending DeviceStatus: %s %d %.1f", dev.c_str(), online, bat);
            sendMessage<MessageId::DeviceStatus>(mq, MessageMethod::SET, DeviceStatus{dev, online != 0, bat});
        } else {
            mylog(MyLogLevel::E, "Unknown menu choice: %s", line.c_str());
        }
    }
    receiver.join();
    mylog(MyLogLevel::I, "[Test] Interactive user input test finished.");
}

// Test function implementations
void test_basic() {
    mylog(MyLogLevel::I, "[Test] Basic send/receive...");
    MessageQueue mq;
    sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, 42);
    sendMessage<MessageId::Position>(mq, MessageMethod::SET, Point{10, 20});
    sendMessage<MessageId::AppConfig>(mq, MessageMethod::SET, Config{1, true});
    sendMessage<MessageId::LogMessage>(mq, MessageMethod::SET, std::string("log test"));
    sendMessage<MessageId::UserInfo>(mq, MessageMethod::SET, User{1001, "Alice", {2,3,4}});
    sendMessage<MessageId::DeviceStatus>(mq, MessageMethod::SET, DeviceStatus{"dev42", true, 88.5});
    sendMessage<MessageId::DataPacket>(mq, MessageMethod::SET, DataPacket{{1,2,3,4,5}, 7});
    sendMessage<MessageId::Nested>(mq, MessageMethod::SET, Nested{User{1002, "Bob", {1,3}}, DeviceStatus{"dev99", false, 0.0}, {DataPacket{{9,8,7}, 1}}});
    Settings s; s.kv["lang"] = "C++"; s.kv["os"] = "Linux";
    sendMessage<MessageId::Settings>(mq, MessageMethod::SET, s);

    for (int i = 0; i < 9; ++i) {
        Message msg = mq.receive();
        switch (msg.id) {
            case MessageId::Temperature: {
                int temp = std::get<int>(msg.payload);
                mylog(MyLogLevel::I, "Received Temperature: %d", temp);
                assert(temp == 42);
                break;
            }
            case MessageId::Position: {
                Point pt = std::get<Point>(msg.payload);
                mylog(MyLogLevel::I, "Received Position: (%d, %d)", pt.x, pt.y);
                assert(pt.x == 10 && pt.y == 20);
                break;
            }
            case MessageId::AppConfig: {
                Config cfg = std::get<Config>(msg.payload);
                mylog(MyLogLevel::I, "Received AppConfig: setting1=%d, setting2=%d", cfg.setting1, cfg.setting2);
                assert(cfg.setting1 == 1 && cfg.setting2 == true);
                break;
            }
            case MessageId::LogMessage: {
                std::string s = std::get<std::string>(msg.payload);
                mylog(MyLogLevel::I, "Received LogMessage: %s", s.c_str());
                assert(s == "log test");
                break;
            }
            case MessageId::UserInfo: {
                User u = std::get<User>(msg.payload);
                mylog(MyLogLevel::I, "Received UserInfo: id=%d, name=%s, friends=%zu", u.id, u.name.c_str(), u.friends.size());
                assert(u.id == 1001 && u.name == "Alice" && u.friends.size() == 3);
                break;
            }
            case MessageId::DeviceStatus: {
                DeviceStatus d = std::get<DeviceStatus>(msg.payload);
                mylog(MyLogLevel::I, "Received DeviceStatus: name=%s, online=%d, battery=%.1f", d.deviceName.c_str(), d.online, d.battery);
                assert(d.deviceName == "dev42" && d.online == true && d.battery == 88.5);
                break;
            }
            case MessageId::DataPacket: {
                DataPacket p = std::get<DataPacket>(msg.payload);
                mylog(MyLogLevel::I, "Received DataPacket: seq=%d, size=%zu", p.seq, p.data.size());
                assert(p.seq == 7 && p.data.size() == 5);
                break;
            }
            case MessageId::Nested: {
                Nested n = std::get<Nested>(msg.payload);
                mylog(MyLogLevel::I, "Received Nested: user=%s, device=%s, packets=%zu", n.user.name.c_str(), n.status.deviceName.c_str(), n.packets.size());
                assert(n.user.id == 1002 && n.user.name == "Bob" && n.status.deviceName == "dev99");
                break;
            }
            case MessageId::Settings: {
                Settings s = std::get<Settings>(msg.payload);
                mylog(MyLogLevel::I, "Received Settings: kv size=%zu", s.kv.size());
                assert(s.kv.at("lang") == "C++" && s.kv.at("os") == "Linux");
                break;
            }
            default:
                mylog(MyLogLevel::E, "Unknown message id: %d", (int)msg.id);
                assert(false && "Unknown message id");
        }
    }
    mylog(MyLogLevel::I, "[Test] Basic send/receive OK");
}

void test_type_safety() {
    mylog(MyLogLevel::I, "[Test] Type safety...");
    MessageQueue mq;
    sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, 123);
    Message msg = mq.receive();
    try {
        // Wrong type extraction
        Point pt = std::get<Point>(msg.payload);
        mylog(MyLogLevel::E, "Type safety test failed: extracted Point from int");
        assert(false && "Type safety test failed: extracted Point from int");
    } catch (const std::bad_variant_access& e) {
        mylog(MyLogLevel::I, "Type safety OK: exception caught: %s", e.what());
    }
}

void test_multithreaded() {
    mylog(MyLogLevel::I, "[Test] Multi-threaded producer/consumer...");
    MessageQueue mq;
    constexpr int N = 4;
    constexpr int MSGS = 1000;
    std::atomic<int> sent{0}, received{0};
    std::vector<std::thread> producers, consumers;

    // Producer threads
    for (int i = 0; i < N; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < MSGS; ++j) {
                int val = i * MSGS + j;
                sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, val);
                sent.fetch_add(1, std::memory_order_relaxed);
                if (j % 200 == 0) mylog(MyLogLevel::I, "Producer %d sent %d", i, val);
            }
        });
    }
    // Consumer threads
    for (int i = 0; i < N; ++i) {
        consumers.emplace_back([&, i]() {
            for (int j = 0; j < MSGS; ++j) {
                Message msg = mq.receive();
                if (msg.id == MessageId::Temperature) {
                    int temp = std::get<int>(msg.payload);
                    if (j % 200 == 0) mylog(MyLogLevel::I, "Consumer %d received %d", i, temp);
                }
                received.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    mylog(MyLogLevel::I, "[Test] Multi-threaded done. Sent=%d, Received=%d", sent.load(), received.load());
}

void test_stress() {
    mylog(MyLogLevel::I, "[Test] Stress test...");
    MessageQueue mq;
    constexpr int N = 8;
    constexpr int MSGS = 5000;
    std::atomic<int> sent{0}, received{0};
    std::vector<std::thread> producers, consumers;

    for (int i = 0; i < N; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < MSGS; ++j) {
                int type = (i + j) % 9;
                switch (type) {
                    case 0:
                        sendMessage<MessageId::Temperature>(mq, MessageMethod::SET, i * MSGS + j);
                        break;
                    case 1:
                        sendMessage<MessageId::Position>(mq, MessageMethod::SET, Point{i, j});
                        break;
                    case 2:
                        sendMessage<MessageId::AppConfig>(mq, MessageMethod::SET, Config{j, (j % 2) == 0});
                        break;
                    case 3:
                        sendMessage<MessageId::LogMessage>(mq, MessageMethod::SET, std::string("stress log"));
                        break;
                    case 4:
                        sendMessage<MessageId::UserInfo>(mq, MessageMethod::SET, User{j, "U", {i}});
                        break;
                    case 5:
                        sendMessage<MessageId::DeviceStatus>(mq, MessageMethod::SET, DeviceStatus{"dev", (j%2)==0, 100.0-j%100});
                        break;
                    case 6:
                        sendMessage<MessageId::DataPacket>(mq, MessageMethod::SET, DataPacket{{(uint8_t)(j%256)}, j});
                        break;
                    case 7:
                        sendMessage<MessageId::Nested>(mq, MessageMethod::SET, Nested{User{j, "N", {i}}, DeviceStatus{"d", true, 1.0}, {}});
                        break;
                    case 8: {
                        Settings s; s.kv["k"] = "v"; sendMessage<MessageId::Settings>(mq, MessageMethod::SET, s);
                        break;
                    }
                }
                sent.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (int i = 0; i < N; ++i) {
        consumers.emplace_back([&, i]() {
            for (int j = 0; j < MSGS; ++j) {
                Message msg = mq.receive();
                switch (msg.id) {
                    case MessageId::Temperature:
                        std::get<int>(msg.payload);
                        break;
                    case MessageId::Position:
                        std::get<Point>(msg.payload);
                        break;
                    case MessageId::AppConfig:
                        std::get<Config>(msg.payload);
                        break;
                    case MessageId::LogMessage:
                        std::get<std::string>(msg.payload);
                        break;
                    case MessageId::UserInfo:
                        std::get<User>(msg.payload);
                        break;
                    case MessageId::DeviceStatus:
                        std::get<DeviceStatus>(msg.payload);
                        break;
                    case MessageId::DataPacket:
                        std::get<DataPacket>(msg.payload);
                        break;
                    case MessageId::Nested:
                        std::get<Nested>(msg.payload);
                        break;
                    case MessageId::Settings:
                        std::get<Settings>(msg.payload);
                        break;
                    default:
                        mylog(MyLogLevel::E, "Unknown id in stress test: %d", (int)msg.id);
                }
                received.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    mylog(MyLogLevel::I, "[Test] Stress test done. Sent=%d, Received=%d", sent.load(), received.load());
}