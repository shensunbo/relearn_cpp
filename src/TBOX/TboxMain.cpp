/**
 * TboxMain.cpp — TboxService server process entry point.
 *
 * Starts TboxAidlHandler and TboxFdbusHandler worker threads, then runs
 * the Unix domain socket server that both aidlClient and fdbusClient
 * connect to for IPC.
 *
 * Send SIGINT or SIGTERM to shut down gracefully.
 */
#include "TboxService.h"
#include "TboxSocketServer.h"
#include "mylog.h"

#include <csignal>
#include <atomic>
#include <cstdlib>

static std::atomic<bool> g_quit{false};
static TboxSocketServer* g_server = nullptr;

static void sigHandler(int sig) {
    mylog(MyLogLevel::I, "[TboxMain] Signal %d received — initiating graceful shutdown", sig);
    g_quit = true;
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    mylog(MyLogLevel::I, "[TboxMain] ====================================");
    mylog(MyLogLevel::I, "[TboxMain] TboxService process starting up");
    mylog(MyLogLevel::I, "[TboxMain] Socket: %s", TboxSocketServer::DEFAULT_SOCKET_PATH);
    mylog(MyLogLevel::I, "[TboxMain] ====================================");

    // Install signal handlers for graceful shutdown
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    // Construct TboxService (creates queues, AidlHandler, FdbusHandler)
    mylog(MyLogLevel::I, "[TboxMain] Constructing TboxService...");
    TboxService svc;

    // Start internal worker threads
    mylog(MyLogLevel::I, "[TboxMain] Starting TboxService worker threads...");
    svc.start();
    mylog(MyLogLevel::I, "[TboxMain] Worker threads running");

    // Create and run the IPC socket server (blocks until stop())
    mylog(MyLogLevel::I, "[TboxMain] Creating TboxSocketServer...");
    TboxSocketServer server(svc);
    g_server = &server;

    mylog(MyLogLevel::I, "[TboxMain] Entering server run loop (Ctrl+C to stop)");
    server.run();   // blocks

    mylog(MyLogLevel::I, "[TboxMain] Server run loop returned");
    g_server = nullptr;

    mylog(MyLogLevel::I, "[TboxMain] TboxService process shut down cleanly");
    return 0;
}
