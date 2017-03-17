// slow_control_tm.cc
/* Listen for and execute remote user commands for the backplane. Continually
 * send out a status report back to the user for display by the GUI. */

#include <iostream>
#include <string>

#include "tmcontrol.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_tm hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];

    TMControl target_control(hostname);

    // Communicate with the server: on each loop send updated data and 
    // receive updated settings
    std::string command;
    std::string parameter;
    std::cout << "communicating with the server...\n";
    while (true) {
        // Send and receive messages
        target_control.synchronize_network();
        if (target_control.command_received()) {
            command = target_control.command();
            parameter = target_control.parameter();
            // perform_updates(command, parameter) (implement in python)
            target_control.save_updated_variables();
        }
    }
    return 0;
}
