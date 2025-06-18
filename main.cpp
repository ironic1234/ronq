#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <thread>

void run_command(const char* cmd) {
    int pipefd[2];
    pipe(pipefd);
    pid_t grandchild = fork();
    if (grandchild == 0) {
        // Grandchild: set up error prefixing for stderr
        close(pipefd[0]);
        int err_pipe[2];
        pipe(err_pipe);
        pid_t err_fork = fork();
        if (err_fork == 0) {
            // Error prefixing process
            close(err_pipe[1]);
            FILE* err_stream = fdopen(err_pipe[0], "r");
            char err_buf[4096];
            while (fgets(err_buf, sizeof(err_buf), err_stream)) {
                // Prefix stderr lines
                dprintf(pipefd[1], "STDERR:%s", err_buf);
            }
            fclose(err_stream);
            _exit(0);
        } else {
            // Grandchild: redirect stderr to err_pipe
            close(err_pipe[0]);
            dup2(err_pipe[1], STDERR_FILENO);
            close(err_pipe[1]);
            // Redirect stdout directly to pipe
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
            perror("exec failed");
            _exit(1);
        }
    } else {
        // Parent (child of main): read from pipe, color output
        close(pipefd[1]);
        FILE* stream = fdopen(pipefd[0], "r");
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), stream)) {
            std::string line(buffer);
            if (line.rfind("STDERR:", 0) == 0) {
                // Red for errors
                std::cout << "\033[31m[" << cmd << "]\033[0m " << line.substr(7);
            } else {
                // Green for normal output
                std::cout << "\033[32m[" << cmd << "]\033[0m " << line;
            }
        }
        fclose(stream);
        int status;
        waitpid(grandchild, &status, 0);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command1> <command2> ..." << std::endl;
        return 1;
    }
    std::vector<std::thread> threads;
    for (int i = 1; i < argc; i++) {
        threads.emplace_back(run_command, argv[i]);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
