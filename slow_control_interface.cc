// slow_control_interface.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <limits>

#include "sc_network.h"
#include "sc_backplane.h"
#include "sc_logistics.h"

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value);

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_interface hostname" << std::endl;
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
    unsigned long fee_power = 0; // for 'n' command to power control FEEs
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
        } else if (command.compare("v") == 0) {
            // Read FEE housekeeping voltages
            std::cout << "Read FEE voltages (V)." << std::endl;
            new_settings = FEE_VOLTAGES;
        } else if (command.compare("i") == 0) {
            // Read FEE currents
            std::cout << "Read FEE currents (A)." << std::endl;
            new_settings = FEE_CURRENTS;
        } else if (command.compare("c") == 0) {
            // Monitor trigger rate
            std::cout << "Monitor trigger rate." << std::endl;
            new_settings = BP_READ_NSTIMER_TRIGGER_RATE;
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
        } else if (command.compare("j") == 0) {
            // Set trigger mask
            std::cout << "Set trigger mask." << std::endl;
            new_settings = BP_SET_TRIGGER_MASK;
        } else if (command.compare("l") == 0) {
            // Reset trigger counter and timer
            std::cout << "Reset trigger counter and timer." << std::endl;
            new_settings = BP_RESET_TRIGGER_AND_NSTIMER;
        } else if (command.compare("n") == 0) {
            // Power modules on and off
            std::cout << "Power on/off modules." << std::endl;
            std::cout << "Enter FEEs to Power ON/OFF (32 bits 0=off 1=on) "
                << "0-0xFFFFFFFF: ";
            std::cin >> std::hex >> fee_power;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			settings_commands[0] = fee_power >> 16;
            settings_commands[1] = fee_power & 0x0000ffff;
            new_settings = BP_POWER_CONTROL_MODULES;
        } else if (command.compare("o") == 0) {
            // Set holdoff time
            std::cout << "Set holdoff time." << std::endl;
            std::cout << "Enter Hold Off in hex: ";
            std::cin >> std::hex >> settings_commands[0];
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            new_settings = BP_SET_HOLDOFF_TIME;
        } else if (command.compare("s") == 0) {
            // Send sync command
            std::cout << "Send sync command." << std::endl;
            new_settings = BP_SYNC;
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
        } else if (command.compare("m") == 0) {
            // Display menu
            std::cout << "MENU" << std::endl;
            std::cout << "  c    [ Monitor trigger rate ]" << std::endl;
            std::cout << "  d    [ Set trigger at time ]" << std::endl;
            std::cout << "  g    [ Enable or disable trigger/TACK ]" 
                << std::endl;
            std::cout << "  i    [ Read FEE currents ]" << std::endl;
            std::cout << "  j    [ Set trigger mask ]" << std::endl;
            std::cout << "  l    [ Reset trigger counter and timer ]"
                << std::endl;
            std::cout << "  m    [ Display menu ]" << std::endl;
            std::cout << "  n    [ Power on/off modules ]" << std::endl;
            std::cout << "  o    [ Set holdoff time ]" << std::endl;
            std::cout << "  p    [ Read FEEs present ]" << std::endl;
            std::cout << "  s    [ Send sync command ]" << std::endl;
            std::cout << "  v    [ Read FEE voltages ]" << std::endl;
            std::cout << "  x    [ Exit user interface ]" << std::endl;
            std::cout << "  z    [ Set TACK type and mode ]" << std::endl;
            continue;
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
        // Update and send settings
        backplane.update_settings(new_settings, settings_commands);
        backplane.synchronize_network(netinfo);
        // Get results of command
        sleep_msec(200);
        backplane.synchronize_network(netinfo);
        // Display updated values
        backplane.print_data(new_settings);
    }

    return 0;
}

// Split a string into component words using the specified delimiter
void split(const std::string &s, char delim, std::vector<std::string> &words)
{
    std::stringstream ss(s);
    std::string word;
    while (getline(ss, word, delim)) {
        words.push_back(word);
    }
}

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value)
{
    std::string s;
    std::vector<std::string> words;

    command = "";
    value = "";

    std::cout << "Enter command: ";

    std::getline(std::cin, s);
    split(s, ' ', words);

    // Require either a single word command, or a word followed by a value
    if ((words.size() < 1) || (words.size() > 2)) {
        return false;
    }
    command = words[0];
    if (words.size() == 2) {
        value = words[1];
    }

    return true;
}
