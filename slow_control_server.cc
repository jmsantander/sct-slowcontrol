// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI
// Log all data packets received from the Raspberry Pi

#include <iostream>
#include <string>
#include <vector>

#include "servercontrol.h"

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc < 4) {
        std::cout << "usage: slow_control_server db_host db_username " <<
            "db_password" << std::endl;
        return 1;
    }
    std::string db_host = argv[1];
    std::string db_username = argv[2];
    std::string db_password = argv[3];
   
    // Set up server control
    ServerControl server_control(db_host, db_username, db_password);

    // Load available commands from config file
    std::cout << "Loading commands..." << std::endl;
    if (!server_control.parse_command_config("commands.config")) {
        std::cout << "Could not load commands. Exiting..." << std::endl;
        return 1;
    }
    std::cout << "Commands loaded." << std::endl;

    // Update network
    while (true) {
        // Send and receive commands and variables from clients
        server_control.synchronize_network();
        // If a high level command was received, populate a queue of the
        // corresponding low level commands for the pi and target controller
        // If a new high level command, updated backplane variables,
        // or updated target variables were received, log them
        server_control.process_received_messages();
        // If there are low level commands in the queue and the appropriate
        // controller for the first one isn't occupied, send that command
        server_control.send_next_command();
    }
    
    return 0;
}
