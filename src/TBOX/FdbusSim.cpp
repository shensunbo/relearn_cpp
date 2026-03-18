/**
 * FdbusSim.cpp — Interactive FdBus server simulator (IPC version)
 *
 * Connects to the TboxService process via Unix socket.
 * Run tboxService first, then this binary.
 *
 * Capabilities:
 *   - UPDATE any signal (simulates FdBus network → TboxService → AIDL pipeline)
 *   - SET PrivateSwitchState via AIDL path (simulates AIDL client writing)
 *   - GET any signal from TboxService
 *   - SUBSCRIBE to observe changes
 *
 * Build target: fdbusClient
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
        << "   FdBus Server Simulator  [IPC via socket]\n"
        << "=================================================\n"
        << "  --- FdBus server signal update (-> AIDL) ---\n"
        << "  up1 <str>  Update ServiceProvider\n"
        << "  up2 <0-5>  Update WANConnInfo\n"
        << "             0=NoNet 1=Conn 2=2G 3=3G 4=4G 5=5G\n"
        << "  up3 <n>    Update CallInfo\n"
        << "             1=IIn 3=IDial 4=IOut 5=Idle\n"
        << "             6=EIn 7=EDial 8=EOut\n"
        << "  up4 <0|1>  Update PrivateSwitchState (FdBus side)\n"
        << "\n"
        << "  --- Simulate AIDL client writing PrivateSwitch ---\n"
        << "  setps <0|1>  AIDL path: flows AIDL -> mMsgToFdbus\n"
        << "               -> FdbusHandler -> FdbusServerMock\n"
        << "               -> echo back to AIDL Attribute\n"
        << "\n"
        << "  --- Query / Subscribe ---\n"
        << "  get <signal>       Query a signal\n"
        << "  sub <signal>       Subscribe to a signal\n"
        << "  getall             Query all signals\n"
        << "\n"
        << "  help   Show this menu\n"
        << "  exit   Quit\n"
        << "=================================================\n"
        << "> " << std::flush;
}

int main() {
    mylog(MyLogLevel::I, "[FdbusSim] Starting FdBus server simulator (IPC mode)");

    TboxSocketClient client;

    mylog(MyLogLevel::I, "[FdbusSim] Connecting to TboxService...");
    if (!client.connect()) {
        fprintf(stderr,
                "[FdbusSim] ERROR: Cannot connect to TboxService at %s\n"
                "           Make sure tboxService is running first.\n",
                TboxSocketClient::DEFAULT_SOCKET_PATH);
        return 1;
    }
    mylog(MyLogLevel::I, "[FdbusSim] Connected to TboxService");

    // Subscribe to all signals to observe AIDL-side attribute updates
    mylog(MyLogLevel::I, "[FdbusSim] Subscribing to all signals for observation...");
    auto observeCb = [](const std::string& sig, const std::string& val) {
        std::string display = val;
        if (sig == "WANConnInfo")             display = wanStr(val);
        else if (sig == "CallInfo")           display = callStr(val);
        else if (sig == "PrivateSwitchState") display = (val == "1" ? "ON" : "OFF");
        fprintf(stdout, "\n[AIDL-OBSERVE] %s -> %s\n> ",
                sig.c_str(), display.c_str());
        fflush(stdout);
    };
    client.subscribe("ServiceProvider",    observeCb);
    client.subscribe("WANConnInfo",        observeCb);
    client.subscribe("CallInfo",           observeCb);
    client.subscribe("PrivateSwitchState", observeCb);
    mylog(MyLogLevel::I, "[FdbusSim] Observation subscriptions registered");

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

        mylog(MyLogLevel::D, "[FdbusSim] Command: \"%s\"", cmd.c_str());

        if (cmd == "exit" || cmd == "quit") {
            mylog(MyLogLevel::I, "[FdbusSim] Exit requested");
            break;

        } else if (cmd == "help") {
            printMenu();
            continue;

        } else if (cmd == "up1") {
            std::string sp;
            if (!(iss >> sp)) {
                std::cout << "  Usage: up1 <string>\n";
            } else {
                mylog(MyLogLevel::I, "[FdbusSim] UPDATE ServiceProvider -> \"%s\"", sp.c_str());
                bool ok = client.update("ServiceProvider", sp);
                std::cout << "  UPDATE ServiceProvider -> \"" << sp << "\" "
                          << (ok ? "OK" : "FAILED") << "\n"
                          << "  (watch for AIDL-OBSERVE notification)\n";
            }

        } else if (cmd == "up2") {
            int v = -1;
            if (!(iss >> v) || v < 0 || v > 5) {
                std::cout << "  Usage: up2 <0-5>  (0=No,1=Conn,2=2G,3=3G,4=4G,5=5G)\n";
            } else {
                mylog(MyLogLevel::I, "[FdbusSim] UPDATE WANConnInfo -> %d", v);
                bool ok = client.update("WANConnInfo", std::to_string(v));
                std::cout << "  UPDATE WANConnInfo -> " << wanStr(std::to_string(v))
                          << " " << (ok ? "OK" : "FAILED") << "\n"
                          << "  (watch for AIDL-OBSERVE notification)\n";
            }

        } else if (cmd == "up3") {
            int v = -1;
            if (!(iss >> v)) {
                std::cout << "  Usage: up3 <n>  "
                             "(1=IIn,3=IDial,4=IOut,5=Idle,6=EIn,7=EDial,8=EOut)\n";
            } else {
                mylog(MyLogLevel::I, "[FdbusSim] UPDATE CallInfo -> %d", v);
                bool ok = client.update("CallInfo", std::to_string(v));
                std::cout << "  UPDATE CallInfo -> " << callStr(std::to_string(v))
                          << " " << (ok ? "OK" : "FAILED") << "\n"
                          << "  (watch for AIDL-OBSERVE notification)\n";
            }

        } else if (cmd == "up4") {
            int v = -1;
            if (!(iss >> v) || (v != 0 && v != 1)) {
                std::cout << "  Usage: up4 <0|1>\n";
            } else {
                mylog(MyLogLevel::I, "[FdbusSim] UPDATE PrivateSwitchState (FdBus side) -> %d", v);
                bool ok = client.update("PrivateSwitchState", std::to_string(v));
                std::cout << "  UPDATE PrivateSwitchState -> " << (v ? "ON" : "OFF")
                          << " " << (ok ? "OK" : "FAILED") << "\n"
                          << "  (watch for AIDL-OBSERVE notification)\n";
            }

        } else if (cmd == "setps") {
            int v = -1;
            if (!(iss >> v) || (v != 0 && v != 1)) {
                std::cout << "  Usage: setps <0|1>\n";
            } else {
                mylog(MyLogLevel::I,
                      "[FdbusSim] SET PrivateSwitchState via AIDL path -> %d", v);
                bool ok = client.set("PrivateSwitchState", std::to_string(v));
                std::cout << "  SET PrivateSwitchState (AIDL path) -> " << (v ? "ON" : "OFF")
                          << " " << (ok ? "OK" : "FAILED") << "\n"
                          << "  Flow: AIDL -> mMsgToFdbus -> FdbusHandler ->\n"
                          << "        FdbusServerMock -> echo back to AidlHandler\n"
                          << "  (watch for AIDL-OBSERVE notification)\n";
            }

        } else if (cmd == "get") {
            std::string sig;
            if (!(iss >> sig)) {
                std::cout << "  Usage: get <ServiceProvider|WANConnInfo|"
                             "CallInfo|PrivateSwitchState>\n";
            } else {
                auto val = client.get(sig);
                std::string display = val;
                if (sig == "WANConnInfo")             display = wanStr(val);
                else if (sig == "CallInfo")           display = callStr(val);
                else if (sig == "PrivateSwitchState") display = (val == "1" ? "ON" : "OFF");
                std::cout << "  " << sig << " = " << display << "\n";
            }

        } else if (cmd == "sub") {
            std::string sig;
            if (!(iss >> sig)) {
                std::cout << "  Usage: sub <signal>\n";
            } else {
                client.subscribe(sig, [](const std::string& s, const std::string& v) {
                    fprintf(stdout, "\n[SUB] %s -> %s\n> ", s.c_str(), v.c_str());
                    fflush(stdout);
                });
                std::cout << "  Subscribed to " << sig << "\n";
            }

        } else if (cmd == "getall") {
            auto sp  = client.get("ServiceProvider");
            auto wan = client.get("WANConnInfo");
            auto ci  = client.get("CallInfo");
            auto ps  = client.get("PrivateSwitchState");
            std::cout << "  ServiceProvider    = \"" << sp << "\"\n"
                      << "  WANConnInfo        = " << wanStr(wan) << "\n"
                      << "  CallInfo           = " << callStr(ci) << "\n"
                      << "  PrivateSwitchState = " << (ps == "1" ? "ON" : "OFF") << "\n";

        } else {
            std::cout << "  Unknown command \"" << cmd
                      << "\". Type 'help' for menu.\n";
        }

        // Give the pipeline time to propagate before next prompt
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "> " << std::flush;
    }

    mylog(MyLogLevel::I, "[FdbusSim] Shutting down");
    client.disconnect();
    return 0;
}
