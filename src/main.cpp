#include <memory>
#include <string>
#include <iostream>

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus-zmq/xserver_zmq.hpp"
#include "xeus-zmq/xzmq_context.hpp"

#include "xeus-sas/xinterpreter.hpp"
#include "xeus-sas/xeus_sas_config.hpp"

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
    auto interpreter = std::make_unique<xeus_sas::interpreter>();

    // Print startup message
    std::cout << "Starting xeus-sas kernel version "
              << xeus_sas::version << std::endl;
    std::cout << "SAS Jupyter Kernel" << std::endl;

    // Create kernel
    xeus::xkernel kernel(
        config,
        xeus::get_user_name(),
        std::move(context),
        std::move(interpreter),
        xeus::make_xserver_default
    );

    // Start kernel
    kernel.start();

    return 0;
}
