/**
 * AidlSim.cpp — Interactive AIDL client simulator (IPC version)
 *
 * Connects to the TboxService process via Unix socket.
 * Run tboxService first, then this binary.
 *
 * Capabilities:
 *   - Query any signal attribute from TboxService
 *   - Subscribe to change notifications
 *   - Set PrivateSwitchState  (the only AIDL-writable signal)
 *
 * Build target: aidlClient
 */
#include "TboxSocketClient.h"
#include "mylog.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>

// ---------------------------------------------------------------------------
// Pretty-print helpers (no dependency on MsgDefine.h)
// ---------------------------------------------------------------------------
static const char* wanStr(const std::string& v) {
    if (v == "0") return "NoNetwork(0)";
    if (v == "1") return "Connecting(1)";
    if (v == "2") return "2G(2)";
    if (v == "3") return "3G(3)";
    if (v == "4") return "4G(4)";
    if (v == "5") return "5G(5)";
    return v.c_str();
}

static const char* callStr(const std::string& v) {
    if (v == "1") return "ICall-Incoming(1)";
    if (v == "3") return "ICall-Dialing(3)";
    if (v == "4") return "ICall-Outgoing(4)";
    if (v == "5") return "Idle(5)";
    if (v == "6") return "ECall-Incoming(6)";
    if (v == "7") return "ECall-Dialing(7)";
    if (v == "8") return "ECall-Outgoing(8)";
    return v.c_str();
}

static void printMenu() {
    std::cout
        << "\n"
        << "=================================================\n"
        << "    AIDL Client Simulator  [IPC via socket]\n"
        << "=================================================\n"
        << "  --- Query ---\n"
        << "  q1    Query ServiceProvider\n"
        << "  q2    Query WANConnInfo\n"
        << "  q3    Query CallInfo\n"
        << "  q4    Query PrivateSwitchState\n"
        << "  qall  Query all signals\n"
        << "\n"
        << "  --- Subscribe (registers callback) ---\n"
        << "  s1    Subscribe ServiceProvider\n"
        << "  s2    Subscribe WANConnInfo\n"
        << "  s3    Subscribe CallInfo\n"
        << "  s4    Subscribe PrivateSwitchState\n"
        << "  sall  Subscribe all signals\n"
        << "\n"
        << "  --- Set (AIDL-writable only) ---\n"
        << "  set4 <0|1>   Set PrivateSwitchState\n"
        << "\n"
        << "  help  Show this menu\n"
        << "  exit  Quit\n"
        << "=================================================\n"
        << "> " << std::flush;
}

int main() {
    mylog(MyLogLevel::I, "[AidlSim] Starting AIDL client (IPC mode)");

    TboxSocketClient client;

    mylog(MyLogLevel::I, "[AidlSim] Connecting to TboxService...");
    if (!client.connect()) {
        fprintf(stderr,
                "[AidlSim] ERROR: Cannot connect to TboxService at %s\n"
                "          Make sure tboxService is running first.\n",
                TboxSocketClient::DEFAULT_SOCKET_PATH);
        return 1;
    }
    mylog(MyLogLevel::I, "[AidlSim] Connected to TboxService");

    // Auto-subscribe to all signals so user sees changes immediately
    mylog(MyLogLevel::I, "[AidlSim] Auto-subscribing to all signals...");
    auto notifyCb = [](const std::string& sig, const std::string& val) {
        // Pretty-print based on signal name
        std::string display = val;
        if (sig == "WANConnInfo")        display = wanStr(val);
        else if (sig == "CallInfo")      display = callStr(val);
        else if (sig == "PrivateSwitchState") display = (val == "1" ? "ON" : "OFF");

        fprintf(stdout, "\n[SUBSCRIPTION] %s changed -> %s\n> ",
                sig.c_str(), display.c_str());
        fflush(stdout);
    };

    client.subscribe("ServiceProvider",    notifyCb);
    client.subscribe("WANConnInfo",        notifyCb);
    client.subscribe("CallInfo",           notifyCb);
    client.subscribe("PrivateSwitchState", notifyCb);
    mylog(MyLogLevel::I, "[AidlSim] All subscriptions registered");

    printMenu();

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << "> " << std::flush;
            continue;
        }

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        mylog(MyLogLevel::D, "[AidlSim] Command: \"%s\"", cmd.c_str());

        if (cmd == "exit" || cmd == "quit") {
            mylog(MyLogLevel::I, "[AidlSim] Exit requested");
            break;

        } else if (cmd == "help") {
            printMenu();
            continue;

        } else if (cmd == "q1") {
            auto v = client.get("ServiceProvider");
            std::cout << "  ServiceProvider = \"" << v << "\"\n";

        } else if (cmd == "q2") {
            auto v = client.get("WANConnInfo");
            std::cout << "  WANConnInfo = " << wanStr(v) << "\n";

        } else if (cmd == "q3") {
            auto v = client.get("CallInfo");
            std::cout << "  CallInfo = " << callStr(v) << "\n";

        } else if (cmd == "q4") {
            auto v = client.get("PrivateSwitchState");
            std::cout << "  PrivateSwitchState = " << (v == "1" ? "ON" : "OFF") << "\n";

        } else if (cmd == "qall") {
            auto sp  = client.get("ServiceProvider");
            auto wan = client.get("WANConnInfo");
            auto ci  = client.get("CallInfo");
            auto ps  = client.get("PrivateSwitchState");
            std::cout << "  ServiceProvider    = \"" << sp << "\"\n"
                      << "  WANConnInfo        = " << wanStr(wan) << "\n"
                      << "  CallInfo           = " << callStr(ci) << "\n"
                      << "  PrivateSwitchState = " << (ps == "1" ? "ON" : "OFF") << "\n";

        } else if (cmd == "s1") {
            client.subscribe("ServiceProvider", [](const std::string& s, const std::string& v) {
                fprintf(stdout, "\n[SUB-EXTRA] ServiceProvider -> \"%s\"\n> ", v.c_str());
                fflush(stdout);
            });
            std::cout << "  Extra ServiceProvider subscription added\n";

        } else if (cmd == "s2") {
            client.subscribe("WANConnInfo", [](const std::string& s, const std::string& v) {
                fprintf(stdout, "\n[SUB-EXTRA] WANConnInfo -> %s\n> ", wanStr(v));
                fflush(stdout);
            });
            std::cout << "  Extra WANConnInfo subscription added\n";

        } else if (cmd == "s3") {
            client.subscribe("CallInfo", [](const std::string& s, const std::string& v) {
                fprintf(stdout, "\n[SUB-EXTRA] CallInfo -> %s\n> ", callStr(v));
                fflush(stdout);
            });
            std::cout << "  Extra CallInfo subscription added\n";

        } else if (cmd == "s4") {
            client.subscribe("PrivateSwitchState", [](const std::string& s, const std::string& v) {
                fprintf(stdout, "\n[SUB-EXTRA] PrivateSwitchState -> %s\n> ",
                        v == "1" ? "ON" : "OFF");
                fflush(stdout);
            });
            std::cout << "  Extra PrivateSwitchState subscription added\n";

        } else if (cmd == "sall") {
            auto cb = [](const std::string& sig, const std::string& val) {
                std::string display = val;
                if (sig == "WANConnInfo")        display = wanStr(val);
                else if (sig == "CallInfo")      display = callStr(val);
                else if (sig == "PrivateSwitchState") display = (val == "1" ? "ON" : "OFF");
                fprintf(stdout, "\n[SUB-EXTRA] %s -> %s\n> ",
                        sig.c_str(), display.c_str());
                fflush(stdout);
            };
            client.subscribe("ServiceProvider",    cb);
            client.subscribe("WANConnInfo",        cb);
            client.subscribe("CallInfo",           cb);
            client.subscribe("PrivateSwitchState", cb);
            std::cout << "  Extra subscriptions added for all signals\n";

        } else if (cmd == "set4") {
            int v = -1;
            if (!(iss >> v) || (v != 0 && v != 1)) {
                std::cout << "  Usage: set4 <0|1>\n";
            } else {
                mylog(MyLogLevel::I, "[AidlSim] SET PrivateSwitchState -> %d", v);
                bool ok = client.set("PrivateSwitchState", std::to_string(v));
                std::cout << "  SET PrivateSwitchState(" << (v ? "ON" : "OFF") << ") "
                          << (ok ? "OK" : "FAILED") << "\n";
                if (ok) {
                    // Give the echo-back pipeline time to propagate
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    auto cur = client.get("PrivateSwitchState");
                    std::cout << "  Confirmed PrivateSwitchState = "
                              << (cur == "1" ? "ON" : "OFF") << "\n";
                }
            }

        } else {
            std::cout << "  Unknown command \"" << cmd
                      << "\". Type 'help' for menu.\n";
        }

        // Short pause so subscription callbacks can print before next prompt
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "> " << std::flush;
    }

    mylog(MyLogLevel::I, "[AidlSim] Shutting down");
    client.disconnect();
    return 0;
}
