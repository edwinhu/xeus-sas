#include <memory>
#include <string>
#include <iostream>
#include <csignal>
#include <atomic>
#include <unistd.h>

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus-zmq/xserver_zmq.hpp"
#include "xeus-zmq/xzmq_context.hpp"

#include "xeus-sas/xinterpreter.hpp"
#include "xeus-sas/xeus_sas_config.hpp"

// Global flag to indicate interrupt was requested
std::atomic<bool> g_interrupt_requested{false};

// Global pointer to interpreter for signal handler access
xeus_sas::interpreter* g_interpreter = nullptr;

/**
 * @brief Custom SIGINT handler to gracefully recover from interrupts
 *
 * When SIGINT is received (Ctrl-C or kernel interrupt), this handler:
 * 1. Prevents the signal from propagating to the child SAS process
 * 2. Restarts the SAS session to recover gracefully
 * 3. Notifies the user that session state was lost
 *
 * This is necessary because SAS running in batch mode (-stdio) does not
 * support graceful interruption. Sending SIGINT to SAS would kill the
 * process, breaking the kernel connection. Instead, we intercept the
 * signal and restart the session.
 *
 * IMPORTANT: This handler must be async-signal-safe. We only set a flag
 * and defer actual work to avoid calling non-async-signal-safe functions.
 */
void sigint_handler(int /* signal */)
{
    // Only set the interrupt flag - actual handling happens in main loop
    g_interrupt_requested.store(true, std::memory_order_release);

    // Write a simple message to stderr (async-signal-safe)
    const char msg[] = "\n[xeus-sas] Interrupt received, will restart SAS session...\n";
    // write() is async-signal-safe, unlike std::cerr
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

int main(int argc, char* argv[])
{
    // Parse command line arguments
    std::string connection_file = argc > 2 ? argv[2] : "";

    if (connection_file.empty())
    {
        std::cerr << "Usage: xsas -f connection_file" << std::endl;
        return 1;
    }

    // Load configuration
    xeus::xconfiguration config = xeus::load_configuration(connection_file);

    // Create context
    auto context = xeus::make_zmq_context();

    // Create interpreter instance
    auto interpreter_ptr = std::make_unique<xeus_sas::interpreter>();

    // Store raw pointer for signal handler access (before moving ownership)
    g_interpreter = interpreter_ptr.get();

    // Install custom SIGINT handler
    // This prevents SIGINT from killing the child SAS process
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls

    if (sigaction(SIGINT, &sa, nullptr) != 0)
    {
        std::cerr << "Warning: Failed to install SIGINT handler" << std::endl;
    }
    else
    {
        std::cerr << "[xeus-sas] Custom SIGINT handler installed for graceful interrupt recovery" << std::endl;
    }

    // Print startup message
    std::cout << "Starting xeus-sas kernel version "
              << xeus_sas::version << std::endl;
    std::cout << "SAS Jupyter Kernel" << std::endl;
    std::cout << "NOTE: Interrupting the kernel will restart the SAS session (session state will be lost)" << std::endl;

    // Create kernel
    xeus::xkernel kernel(
        config,
        xeus::get_user_name(),
        std::move(context),
        std::move(interpreter_ptr),
        xeus::make_xserver_default
    );

    // Start kernel
    kernel.start();

    return 0;
}
