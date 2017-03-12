// Implementation of slow control logistics functions

#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <algorithm>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "sc_logistics.h"
#include "sc_protobuf.pb.h"

// Database credentials
std::string db_username = "";
std::string db_password = "";

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

// Return string (for logging) corresponding to the code for a command
std::string command_string(int command_code)
{
    switch(command_code) {
        case BP_NONE:
            return "none";
        case FEE_VOLTAGES:
            return "read_voltages";
        case FEE_CURRENTS:
            return "read_currents";
        case FEE_PRESENT:
            return "read_modules_present";
        case BP_SET_TRIGGER_MASK:
            return "set_trigger_mask";
        case BP_RESET_TRIGGER_AND_NSTIMER:
            return "reset_trigger_and_nstimer";
        case BP_SYNC:
            return "sync";
        case BP_SET_HOLDOFF_TIME:
            return "set_holdoff_time";
        case BP_SET_TACK_TYPE_AND_MODE:
            return "set_tack_type_and_mode";
        case BP_POWER_CONTROL_MODULES:
            return "power_control_modules";
        case BP_SET_TRIGGER:
            return "set_trigger";
        case BP_READ_NSTIMER_TRIGGER_RATE:
            return "read_nstimer_trigger_rate";
        case BP_ENABLE_DISABLE_TRIGGER:
            return "enable_disable_trigger";
        default:
            return "";
    }
}

// Get database username and password from user
void get_database_credentials()
{
    sql::Driver *driver;
    sql::Connection *con;
    
    driver = get_driver_instance();

    while (true) {
        std::cout << "Enter database username: ";
        std::cin >> db_username;
        std::cout << "Enter database password for user " << db_username << ": ";
        std::cin >> db_password;
        try {
            con = driver->connect("localhost", db_username, db_password);
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
    try {    
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;
   
        // Connect to database
        driver = get_driver_instance();
        con = driver->connect("localhost", db_username, db_password);
        stmt = con->createStatement();
        stmt->execute("USE test");
  
        // Log command name
        pstmt = con->prepareStatement("INSERT INTO main(command) VALUES (?)");
        pstmt->setString(1, command_string(data.command_code()));
        pstmt->execute();
        delete pstmt;

        // Get id for synchronizing tables
        res = stmt->executeQuery("SELECT LAST_INSERT_ID() AS 'id'");
        res->next();
        int id = res->getInt("id");
        delete res;
        delete stmt;
        
        // Log SPI data
        pstmt = con->prepareStatement("INSERT INTO spi(id, spi_index,\
                spi_message_index, spi_command, spi_data)\
                VALUES (?, ?, ?, ?, ?)");
        for (int spi_message_index = 0; spi_message_index <
                data.n_spi_messages(); spi_message_index++) {
            for (int spi_index = 0; spi_index < N_SPI; spi_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, spi_index);
                pstmt->setInt(3, spi_message_index);
                pstmt->setInt(4, data.spi_command(spi_message_index * N_SPI +
                            spi_index));
                pstmt->setInt(5, data.spi_data(spi_message_index * N_SPI +
                            spi_index));
                pstmt->executeUpdate();
            }
        }
        delete pstmt;
        
        // Log additional data if required by command
        if (data.command_code() == FEE_VOLTAGES) {
            // Log FEE voltages
            pstmt = con->prepareStatement("INSERT INTO fee_voltage(id,\
                    fee_index, voltage) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, data.voltage(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (data.command_code() == FEE_CURRENTS) {
            // Log FEE currents
            pstmt = con->prepareStatement("INSERT INTO fee_current(id,\
                  fee_index, current) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, data.current(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (data.command_code() == FEE_PRESENT) {
            // Log modules present
            pstmt = con->prepareStatement("INSERT INTO fee_present(id,\
                    fee_index, present) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, data.present(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (data.command_code() == BP_SET_TRIGGER_MASK) {
            // Log trigger mask
            pstmt = con->prepareStatement("INSERT INTO trigger_mask(id,\
                    fee_index, mask) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, data.trigger_mask(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        }
        delete con;
        std::cout << command_string(data.command_code()) << " logged."
            << std::endl;
    } catch (sql::SQLException &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
    }
}
