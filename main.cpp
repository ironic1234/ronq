#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <string>

void run_command(const std::string& command) {
    std::cout << "Running command: " << command << std::endl;
   
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << command << " failed with exit code: " << result << std::endl;
    } else {
        std::cout << command <<  " executed successfully." << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command1> <command2> ..." << std::endl;
        return 1;
    }
    std::vector<std::thread> threads;
    for (int i = 1; i < argc; ++i) {
        threads.emplace_back(run_command, argv[i]);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
