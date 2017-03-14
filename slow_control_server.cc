// slow_control_server.cc
// Receive data from the Raspberry Pi and transmit to the GUI
// Log all data packets received from the Raspberry Pi

#include <iostream>
#include <string>
#include <vector>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "sc_network.h"
#include "sc_logistics.h"
#include "sc_protobuf.pb.h"

// Log a data message
void log_data_message(std::string message, std::string db_host,
        std::string db_username, std::string db_password);

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
    
    // Set up networking info
    Network_info netinfo(SERVER);

    // Update network
    while (true) {
        update_network(netinfo);
        // If the server received a (data) message from the Pi, log it
        for (std::vector<Connection>::iterator iter =
                netinfo.connections.begin(); iter != netinfo.connections.end();
                ++iter) {
            if ((iter->device == PI) && (iter->recv_status == MSG_DONE)) {
                log_data_message(iter->message, db_host, db_username,
                        db_password);
            }
        }
    }
    
    // Shut down network
    if (!shutdown_network(netinfo)) {
        return 1;
    }

    return 0;
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

// Log a data message
void log_data_message(std::string message, std::string db_host,
        std::string db_username, std::string db_password)
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
        con = driver->connect(db_host, db_username, db_password);
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
