#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <thread>

const char* COLORS[] = {
    "\033[32m", // Green
    "\033[36m", // Cyan
    "\033[33m", // Yellow
    "\033[35m", // Magenta
    "\033[34m", // Blue
    "\033[91m", // Bright Red
    "\033[92m", // Bright Green
    "\033[93m", // Bright Yellow
    "\033[94m", // Bright Blue
    "\033[95m", // Bright Magenta
};

/// Executes a shell command with stdout and stderr merged into one pipe.
///
/// This function redirects both stdout and stderr to the provided file descriptor
/// and then uses `execl` to execute the command via `/bin/sh -c`.
///
/// @param cmd The shell command to execute.
/// @param write_fd File descriptor to redirect output.
void exec_command(const char* cmd, int write_fd) {
    dup2(write_fd, STDOUT_FILENO);
    dup2(write_fd, STDERR_FILENO);
    close(write_fd);

    execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
    perror("exec failed");
    _exit(1);
}

/// Reads output from a file descriptor and prints it with color.
///
/// Since both stdout and stderr are merged, this function applies a consistent
/// prefix and color to all lines. (Green is used by default.)
///
/// @param cmd The command name to display in the output prefix.
/// @param read_fd File descriptor to read from.
/// @param color ANSI color code string to use for prefix.
void handle_output(const char* cmd, int read_fd, const char* color) {
    FILE* stream = fdopen(read_fd, "r");
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), stream)) {
        std::cout << color << "[" << cmd << "]\033[0m " << buffer;
    }
    fclose(stream);
}

/// Runs a shell command in a subprocess and displays output in color.
///
/// Spawns a child process to execute the command, capturing both stdout and stderr
/// through a pipe. Output is prefixed with the command name and printed in green.
///
/// @param cmd The shell command to execute.
void run_command(const char* cmd, const char* color) {
    int pipefd[2];
    pipe(pipefd);

    pid_t child = fork();
    if (child == 0) {
        // In child
        close(pipefd[0]);
        exec_command(cmd, pipefd[1]);
    } else {
        // In parent
        close(pipefd[1]);
        handle_output(cmd, pipefd[0], color);
        int status;
        waitpid(child, &status, 0);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command1> <command2> ..." << std::endl;
        return 1;
    }

    const int color_count = sizeof(COLORS) / sizeof(COLORS[0]);
    std::vector<std::thread> threads;

    for (int i = 1; i < argc; i++) {
        const char* color = COLORS[(i - 1) % color_count];
        threads.emplace_back(run_command, argv[i], color);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
