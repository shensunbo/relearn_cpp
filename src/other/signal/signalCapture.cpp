#include <csignal>
#include <iostream>

void signalHandler(int signum) {
    std::cout << "Hello from signal (" << signum << ") !\n";
    // cleanup and close up stuff here
    // terminate program
    exit(signum);
}

int main() {
    // register signal SIGINT and signal handler
    // SIGINT is the signal sent when you press Ctrl+C
    signal(SIGINT, signalHandler); // √ Hello from signal (2) !
    // SIGTERM is the signal sent when you terminate the program using the kill command
    signal(SIGTERM, signalHandler); // √ Hello from signal (15) !

    // capture SIGSEGV signal, which is sent when a segmentation fault occurs
    signal(SIGSEGV, signalHandler); // √ Hello from signal (11) !

    // divide by zero to trigger SIGFPE signal, which is sent when a floating point exception occurs
    signal(SIGFPE, signalHandler); // √ Hello from signal (8) !

    // kill -9 <pid> to trigger SIGKILL signal, which is sent when a process is killed
    signal(SIGKILL, signalHandler); // SIGKILL cannot be caught or ignored, X Killed

    while (1) {
        std::cout << "Going to sleep...." << std::endl;
        sleep(1);
        // SIGSEGV
        // int* p = nullptr;
        // *p = 42;

        // SIGFPE
        // int x = 1 / 0;

    }

    return 0;
}