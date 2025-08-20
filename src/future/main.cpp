#include <iostream>
#include <future>
#include <thread>
#include <string>

// handle Ctrl + C
#include <csignal>
static bool running = true;
static void sigint_handler(int signum) { running = false; }

void readInput(std::promise<std::string>& p) {
    while(running){
        std::string input;
        std::cout << "Enter a command: ";
        std::getline(std::cin, input);
        p.set_value(input);
        p = std::promise<std::string>();
    }
}

void displayInput(std::future<std::string>& f) {
    while(running){
        std::string input = f.get();
        std::cout << "Received input: " << input << std::endl;
    }
}

int main() {
    std::promise<std::string> p;
    std::future<std::string> f = p.get_future();

    // for normal end, when using ctrl + c to kill program
    signal(SIGINT, sigint_handler);

    std::thread t1(readInput, std::ref(p));
    std::thread t2(displayInput, std::ref(f));

    // Wait for both threads to finish
    t1.join();
    t2.join();

    return 0;
}