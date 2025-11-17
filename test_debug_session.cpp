#include <cstdio>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    std::cout << "Testing SAS -stdio communication..." << std::endl;

    int stdin_pipe[2];
    int stdout_pipe[2];

    pipe(stdin_pipe);
    pipe(stdout_pipe);

    pid_t pid = fork();

    if (pid == 0) {
        // Child - run SAS
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);

        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        execlp("/data/sas/SASFoundation/9.4/bin/sas_u8", "/data/sas/SASFoundation/9.4/bin/sas_u8",
               "-nodms", "-stdio", "-nonews", "-nosource", nullptr);
        exit(1);
    }

    // Parent
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    FILE* sas_stdin = fdopen(stdin_pipe[1], "w");
    FILE* sas_stdout = fdopen(stdout_pipe[0], "r");

    setvbuf(sas_stdin, nullptr, _IOLBF, 0);

    std::cout << "SAS started (PID: " << pid << ")" << std::endl;
    std::cout << "Sending command..." << std::endl;

    // Send a simple command
    fprintf(sas_stdin, "%%put hello world;\n");
    fprintf(sas_stdin, "%%put MARKER_END;\n");
    fflush(sas_stdin);

    std::cout << "Reading output..." << std::endl;

    // Read output with timeout
    char buffer[4096];
    int line_count = 0;
    bool found_marker = false;

    fd_set read_fds;
    struct timeval tv;

    while (!found_marker && line_count < 100) {
        FD_ZERO(&read_fds);
        FD_SET(fileno(sas_stdout), &read_fds);

        tv.tv_sec = 2;   // 2 second timeout
        tv.tv_usec = 0;

        int retval = select(fileno(sas_stdout) + 1, &read_fds, NULL, NULL, &tv);

        if (retval == -1) {
            std::cerr << "select() error" << std::endl;
            break;
        } else if (retval == 0) {
            std::cout << "Timeout waiting for output (read " << line_count << " lines so far)" << std::endl;
            break;
        } else {
            if (fgets(buffer, sizeof(buffer), sas_stdout)) {
                line_count++;
                std::string line(buffer);
                std::cout << "[" << line_count << "] " << line;

                if (line.find("MARKER_END") != std::string::npos) {
                    found_marker = true;
                    std::cout << "Found marker!" << std::endl;
                }
            } else {
                std::cout << "fgets returned null" << std::endl;
                break;
            }
        }
    }

    std::cout << "\nTotal lines read: " << line_count << std::endl;

    // Cleanup
    fprintf(sas_stdin, "endsas;\n");
    fflush(sas_stdin);
    fclose(sas_stdin);
    fclose(sas_stdout);

    int status;
    waitpid(pid, &status, 0);

    return 0;
}
