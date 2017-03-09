// Implementation of slow control logistics functions

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/statement.h>

#include "sc_logistics.h"
#include "sc_protobuf.pb.h"

// Database password for user root
std::string db_password = "";

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

// Sleep for a given number of milliseconds
void sleep_msec(int msec)
{
    msec = std::min(msec, 999);
    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = 1000000 * msec; // convert millisec to nanosec

    nanosleep(&tim, NULL);
}

// Simulate reading FEE data, for testing without a real Pi
void simulate_fee_data(float fee_buffer[], const int n_fees)
{
    sleep_msec(40);
    for (int i = 0; i < n_fees; i++) {
		fee_buffer[i] = i;
    }
}

// Simulate reading FEEs present, for testing without a real Pi
void simulate_fees_present(unsigned short fees_present[], const int n_fees)
{
    for (int i = 0; i < n_fees; i++) {
        fees_present[i] = i % 2;
    }
}

// Simulate setting the trigger mask, for testing without a real Pi
void simulate_trigger_mask(unsigned short trigger_mask[], const int n_fees)
{
    for (int i = 0; i < n_fees; i++) {
        trigger_mask[i] = i;
    }
}

// Display SPI data
void display_spi_data(unsigned short spi_data[])
{
    std::cout << " SOM  CMD DW 1 DW 2 DW 3 DW 4 DW 5 DW 6 DW 7 DW 8  EOM" 
        << std::endl;
    std::cout << std::setfill('0');
    std::cout << "\033[1;34m" << std::hex << std::setw(4) << spi_data[0]
        << "\033[0m ";
    std::cout << "\033[1;33m" << std::hex << std::setw(4) << spi_data[1]
        << "\033[0m ";
    for (int i = 2; i < 10; i++) {
        std::cout << std::hex << std::setw(4) << spi_data[i] << " ";
    }
    std::cout << "\033[1;34m" << std::hex << std::setw(4) << spi_data[10]
        << "\033[0m " << std::endl << std::endl;
    std::cout << std::setfill(' '); // clear fill
}

// Get database password from user
void get_database_credentials()
{
    sql::Driver *driver;
    sql::Connection *con;
    
    driver = get_driver_instance();
    
    while (true) {
        std::cout << "Enter database password for user root: ";
        std::cin >> db_password;
        try {
            con = driver->connect("localhost", "root", db_password);
            break;
        } catch (sql::SQLException e) {
            std::cout << "Access denied. Try again." << std::endl;
        }
    }

    std::cout << "Database credentials confirmed." << std::endl;

    delete con;
}
                                    
// Log a data message
void log_data_message(std::string message)
{
    slow_control::Backplane_data data;
    if (!data.ParseFromString(message)) {
        std::cerr << "Warning: could not parse data message" << std::endl;
        return;
    }
    switch(data.command_code()) {
        case BP_NONE:
            break;
        case FEE_VOLTAGES:
        {
            // Log command
            std::cout << "LOG " << "fee_voltages" << std::endl;
            // Log FEE voltages
            break;
        }
        case FEE_CURRENTS:
        {
            // Log command
            std::cout << "LOG " << "fee_currents" << std::endl;
            // Log FEE currents
            break;
        }
        case FEE_PRESENT:
        {
            // Log command
            std::cout << "LOG " << "fee_present" << std::endl;
            // Log FEEs present
            break;
        }
        case BP_SET_TRIGGER_MASK:
        {
            // Log command
            std::cout << "LOG " << "set_trigger_mask" << std::endl;
            // Log trigger mask
            break;
        }
        case BP_RESET_TRIGGER_AND_NSTIMER:
        {
            // Log command
            std::cout << "LOG " << "reset_trigger_and_nstimer" << std::endl;
            break;
        }
        case BP_SYNC:
        {
            // Log command
            std::cout << "LOG " << "sync" << std::endl;
            break;
        }
        case BP_SET_HOLDOFF_TIME:
        {
            // Log command
            std::cout << "LOG " << "set_holdoff_time" << std::endl;
            break;
        }
        case BP_SET_TACK_TYPE_AND_MODE:
        {
            // Log command
            std::cout << "LOG " << "set_tack_type_and_mode" << std::endl;
            break;
        }
        case BP_POWER_CONTROL_MODULES:
        {
            // Log command
            std::cout << "LOG " << "power_control_modules" << std::endl;
            break;
        }
        case BP_SET_TRIGGER:
        {
            // Log command
            std::cout << "LOG " << "set_trigger" << std::endl;
            // Log SPI
            break;
        }
        case BP_READ_NSTIMER_TRIGGER_RATE:
        {
            // Log command
            std::cout << "LOG " << "read_nstimer_trigger_rate" << std::endl;
            // Log SPI
            break;
        }
        case BP_ENABLE_DISABLE_TRIGGER:
        {
            // Log command
            std::cout << "LOG " << "enable_disable_trigger" << std::endl;
            // Log SPI
            break;
        }
    }
}
