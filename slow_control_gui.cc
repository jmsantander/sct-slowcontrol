// slow_control_gui.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <string>
#include <unistd.h>
#include <limits>

#include "sc_network.h"
#include "sc_backplane.h"
#include "sc_logistics.h"

// Convenience function to update backplane with new requested settings and
// synchronizing them over the network
void update_and_send_settings(Backplane &backplane, Network_info &netinfo, 
        int new_settings, unsigned short settings_commands[])
{
    backplane.update_settings(new_settings, settings_commands);
    backplane.synchronize_network(netinfo);
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_gui hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];
    
    // Make a Backplane object
    Backplane backplane;
    
    // Set up networking info
    Network_info netinfo(GUI, hostname);

    // Communicate with the server: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the server...\n";
    // Send and receive messages
    backplane.synchronize_network(netinfo);
    std::string command, value;
    int new_settings = BP_NONE;
    unsigned short settings_commands[N_COMMANDS] = {};
    while (true) {
        // Reinitialize settings values to defaults
        new_settings = BP_NONE;
        for (int i = 0; i < N_COMMANDS; i++) {
            settings_commands[i] = 0;
        }
        // Read in a command from the user
        read_command(command, value);
        // Execute the command
        if (command.compare("p") == 0) {
            // Read if FEEs present
            std::cout << "Read FEEs present." << std::endl;
            new_settings = FEE_PRESENT;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("v") == 0) {
            // Read FEE housekeeping voltages
            std::cout << "Read FEE voltages (V)." << std::endl;
            new_settings = FEE_VOLTAGES;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("i") == 0) {
            // Read FEE currents
            std::cout << "Read FEE currents (A)." << std::endl;
            new_settings = FEE_CURRENTS;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("c") == 0) {
            // Monitor trigger rate
            std::cout << "Monitor trigger rate." << std::endl;
            new_settings = BP_READ_NSTIMER_TRIGGER_RATE;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("d") == 0) {
            // Set trigger at time
            std::cout << "Set trigger at time." << std::endl;
            // Get commands from user
            std::cout << "Enter Trig at Time value 63-48 bits in hex: ";
            std::cin >> std::hex >> settings_commands[0];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter Trig at Time value 47-32 bits in hex: ";
            std::cin >> std::hex >> settings_commands[1];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter Trig at Time value 31-16 bits in hex: ";
            std::cin >> std::hex >> settings_commands[2];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter Trig at Time value 15-0  bits in hex: ";
            std::cin >> std::hex >> settings_commands[3];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << std::endl;
            new_settings = BP_SET_TRIGGER;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("g") == 0) {
            // Enable or disable trigger/TACK
            std::cout << "Enable or disable trigger/TACK." << std::endl;
            // Get commands from user
            std::cout << "Enter En/Disable Triggers and TACKs in Hex\n"
                << "(Bit 0 is Phase A logic, 1 Phase B, 2 Phase C, 3 Phase D\n"
                << "Bit 4 is External Trigger\n" << "Bit 5 is TACK messages "
                << "to TMs 0-15, 6 TMs 16-31): ";
            std::cin >> std::hex >> settings_commands[0];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            new_settings = BP_ENABLE_DISABLE_TRIGGER;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("l") == 0) {
            // Reset trigger counter and timer
            std::cout << "Reset trigger counter and timer." << std::endl;
            new_settings = BP_RESET_TRIGGER_AND_NSTIMER;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("o") == 0) {
            // Set holdoff time
            std::cout << "Set holdoff time." << std::endl;
            std::cout << "Enter Hold Off in hex: ";
            std::cin >> std::hex >> settings_commands[0];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            new_settings = BP_SET_HOLDOFF_TIME;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("z") == 0) {
            // Set TACK type and mode
            std::cout << "Set TACK type and mode." << std::endl;
            std::cout << "Enter Tack Type (0-3): ";
            while (std::cin >> std::hex >> settings_commands[0]
                    && settings_commands[0] > 3) { 
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                        '\n');
                std::cout << "Not a valid entry." << std::endl;
                std::cout << "Enter Tack Type (0-3): ";
			}
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter Tack Mode (0-3): ";
            while (std::cin >> std::hex >> settings_commands[1]
                    && settings_commands[1] > 3) { 
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                        '\n');
                std::cout << "Not a valid entry." << std::endl;
                std::cout << "Enter Tack Mode (0-3): ";
			}
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            settings_commands[0] = ((settings_commands[0] << 2) |
                    settings_commands[1]);
            settings_commands[1] = 0;

            new_settings = BP_SET_TACK_TYPE_AND_MODE;
            update_and_send_settings(backplane, netinfo, new_settings,
                    settings_commands);
        } else if (command.compare("x") == 0) {
            // Exit the GUI
            std::cout << "Exit." << std::endl;
            // Shut down network first
            if (!shutdown_network(netinfo))
                return 1;
            break;
        } else if (command.compare("") == 0) {
            continue; 
        } else {
            std::cout << "Command not recognized." << std::endl;
            continue;
        }
        // Get results of command
        sleep_msec(100);
        backplane.synchronize_network(netinfo);
        // Display updated values
        backplane.print_data(new_settings);
    }

    return 0;
}
