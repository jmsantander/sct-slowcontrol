// slow_control_interface.cc
// Receive data from and transmit settings to the server

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <limits>

#include <ctime>
#include <algorithm>

#include "interfacecontrol.h"

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value);

// Sleep for a given number of milliseconds
void sleep_msec(int msec);

// Print the data corresponding to a given command code
void print_data(int data_type);

int main(int argc, char *argv[])
{
    // Parse command line arguments
    if (argc != 2) {
        std::cerr << "usage: slow_control_interface hostname" << std::endl;
        return 1;
    }
    std::string hostname = argv[1];
    
    // Make an InterfaceControl object
    InterfaceControl interface_control(hostname);
    
    // Communicate with the server: on each loop receive updated data and 
    // send updated settings
    std::cout << "communicating with the server...\n";
    std::string command, value;
    while (true) {
        // Send and receive messages
        interface_control.synchronize_network();
        // Read in a command from the user
        read_command(command, value);
        // Execute the command
        if (command.compare("p") == 0) {
            // Read if FEEs present
            std::cout << "Read modules present." << std::endl;
            interface_control.update_highlevel_command("read_modules_present");
        } else if (command.compare("v") == 0) {
            // Read FEE housekeeping voltages
            std::cout << "Read FEE voltages (V)." << std::endl;
            interface_control.update_highlevel_command("read_module_voltages");
        } else if (command.compare("i") == 0) {
            // Read FEE currents
            std::cout << "Read FEE currents (A)." << std::endl;
            interface_control.update_highlevel_command("read_module_currents");
        } else if (command.compare("c") == 0) {
            // Monitor trigger rate
            std::cout << "Monitor trigger rate." << std::endl;
            interface_control.update_highlevel_command("read_trigger_rate");
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
            interface_control.update_highlevel_command("set_trigger");
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
            if (!interface_control.exit())
                return 1;
            break;
        } else if (command.compare("") == 0) {
            continue; 
        } else {
            std::cout << "Command not recognized." << std::endl;
            continue;
        }
        // Update and send settings
        interface_control.update_highlevel_command(command, value);
        interface_control.synchronize_network();
        sleep_msec(200);
        interface_control.synchronize_network();
        // Display updated values
        if (interface_control.backplane_variables_received()) {
            print_data();
        } else {
            std::cout << "Still awaiting response..." << std::endl;
        }
    }
    
    return 0;
}

// Sleep for a given number of milliseconds
void sleep_msec(int msec)
{
    msec = std::min(msec, 999);
    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = 1000000 * msec; // convert millisec to nanosec

    nanosleep(&tim, NULL);
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

// Display SPI data
void display_spi_data()
{
    std::cout << " SOM  CMD DW 1 DW 2 DW 3 DW 4 DW 5 DW 6 DW 7 DW 8  EOM" 
        << std::endl;
    std::cout << std::setfill('0');
    std::cout << "\033[1;34m" << std::hex << std::setw(4)
        << backplane_variables_message().spi_data(0) << "\033[0m ";
    std::cout << "\033[1;33m" << std::hex << std::setw(4)
        << backplane_variables_message().spi_data(1) << "\033[0m ";
    for (int i = 2; i < 10; i++) {
        std::cout << std::hex << std::setw(4)
            << backplane_variables_message().spi_data(i) << " ";
    }
    std::cout << "\033[1;34m" << std::hex << std::setw(4)
        << backplane_variables_message().spi_data(10) << "\033[0m "
        << std::endl << std::endl;
    std::cout << std::setfill(' '); // clear fill
}

void print_data(int data_type)
{
    switch(data_type) {
        case BP_NONE:
        case BP_RESET_TRIGGER_AND_NSTIMER:
        case BP_SYNC:
        case BP_SET_HOLDOFF_TIME:
        case BP_SET_TACK_TYPE_AND_MODE:
        case BP_POWER_CONTROL_MODULES:
            return;
        case FEE_PRESENT:
        {
            for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "   " << std::setw(2)
                        << backplane_variables_message().present(i) << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::setw(2)
                        << backplane_variables_message().present(i) << "   ";
                } else {
                    std::cout << std::setw(2)
                        << backplane_variables_message().present(i) << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            break;
        }
        case BP_SET_TRIGGER_MASK:
        {
            std::cout << std::setfill('0');
            for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "     " << std::hex << std::setw(4)
                        << backplane_variables_message().trigger_mask(i) << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::hex << std::setw(4) 
                        << backplane_variables_message().trigger_mask(i)
                        << "     ";
                } else {
                    std::cout << std::hex << std::setw(4)
                        << backplane_variables_message().trigger_mask(i) << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            std::cout << std::setfill(' '); // clear fill
            break;
        }
        case FEE_VOLTAGES:
        {
            std::cout << std::endl << "FEE voltages:" << std::endl;
            std::cout << std::fixed << std::setprecision(2);
            for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "      " << std::setw(5)
                        << backplane_variables_message().voltage(i) << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::setw(5)
                        << backplane_variables_message().voltage(i) << "      ";
                } else {
                    std::cout << std::setw(5)
                        << backplane_variables_message().voltage(i) << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            break;
        }
        case FEE_CURRENTS:
        {
            std::cout << std::endl << "FEE currents:" << std::endl;
            std::cout << std::fixed << std::setprecision(2);
            for (int i = 0; i < N_FEES; i++) {
                if (i == 0 || i == 28) {
                    std::cout << "      " << std::setw(5)
                        << backplane_variables_message().current(i) << " ";
                } else if (i == 3 || i == 31) {
                    std::cout << std::setw(5)
                        << backplane_variables_message().current(i) << "      ";
                } else {
                    std::cout << std::setw(5)
                        << backplane_variables_message().current(i) << " ";
                }
                if (i == 3 || i == 9 || i == 15 || i == 21 || i == 27 || 
                        i == 31) {
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
            break;
        }
        case BP_SET_TRIGGER:
        case BP_ENABLE_DISABLE_TRIGGER:
        {
            display_spi_data();
            break;
        }
        case BP_READ_NSTIMER_TRIGGER_RATE:
        {
            unsigned long long nstimer;
            unsigned long tack_count;
            unsigned long trigger_count;
            float tack_rate;
            float trigger_rate;

            nstimer = (((unsigned long long)
                        backplane_variables_message().spi_data(2) << 48) |
                    ((unsigned long long)
                     backplane_variables_message().spi_data(3) << 32) |
                    ((unsigned long long)
                     backplane_variables_message().spi_data(4) << 16) |
                    ((unsigned long long)
                     backplane_variables_message().spi_data(5)));
            //TFPGA adds one extra on reset
            tack_count = ((backplane_variables_message().spi_data(6) << 16) |
                    backplane_variables_message().spi_data(7)) - 1;
            tack_rate = (float) nstimer / 1000000000;
            tack_rate = tack_count / tack_rate;
            trigger_count = ((backplane_variables_message().spi_data(8) << 16) |
                    backplane_variables_message().spi_data(9)) - 1;
            trigger_rate = (float) nstimer / 1000000000;
            trigger_rate = trigger_count / trigger_rate;

            display_spi_data();
            printf("nsTimer %llu ns\n", nstimer);
            printf("TACK Count %lu\n", tack_count);
            printf("TACK Rate: %6.2f Hz\n", tack_rate);
            // 4 phases or external trigger can HW trigger
        	printf("Hardware Trigger Count %lu\n", trigger_count);
        	printf("HW Trigger Rate: %6.2f Hz\n", trigger_rate);
            break;
        }
    }
}
