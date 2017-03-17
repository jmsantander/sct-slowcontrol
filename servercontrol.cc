// servercontrol.cc
// Implementation of the class for mid-level server control and logging

#include <string>
#include <vector>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "servercontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

bool ServerControl::synchronize_network()
{
    if (!update_network(netinfo)) {
        return false;
    }
    // Store received messages
    std::vector<Connection>::iterator iter;
    for (iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        if ((iter->device == GUI) && (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!run_settings.ParseFromString(iter->message)) {
                return false;
            }
        } else if ((iter->device == PI) && (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!backplane_variables.ParseFromString(iter->message)) {
                return false;
            }
        } else if ((iter->device == TM) && (iter->recv_status == MSG_DONE)) {
            iter->recv_status = MSG_STANDBY;
            if (!target_variables.ParseFromString(iter->message)) {
                return false;
            }
        }
    }
    return true;
}

void ServerControl::prepare_highlevel_command()
{
    return;
}

void ServerControl::update_next_command()
{
    return;
}

// Return string (for logging) corresponding to the code for a command
std::string command_string(int command_code)
{
    switch(command_code) {
        case BP_NONE:
            return "none";
        case BP_FEE_VOLTAGES:
            return "read_voltages";
        case BP_FEE_CURRENTS:
            return "read_currents";
        case BP_FEE_PRESENT:
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

// Log backplane variables from the pi
void ServerControl::log_backplane_variables()
{
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
        pstmt->setString(1,
                command_string(backplane_variables.command_code()));
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
                backplane_variables.n_spi_messages(); spi_message_index++) {
            for (int spi_index = 0; spi_index < N_SPI; spi_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, spi_index);
                pstmt->setInt(3, spi_message_index);
                pstmt->setInt(4,
                        backplane_variables.spi_command(spi_message_index *
                            N_SPI + spi_index));
                pstmt->setInt(5, 
                        backplane_variables.spi_data(spi_message_index *
                            N_SPI + spi_index));
                pstmt->executeUpdate();
            }
        }
        delete pstmt;
        
        // Log additional data if required by command
        if (backplane_variables.command_code() == BP_FEE_VOLTAGES) {
            // Log FEE voltages
            pstmt = con->prepareStatement("INSERT INTO fee_voltage(id,\
                    fee_index, voltage) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, backplane_variables.voltage(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (backplane_variables.command_code() == BP_FEE_CURRENTS) {
            // Log FEE currents
            pstmt = con->prepareStatement("INSERT INTO fee_current(id,\
                  fee_index, current) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, backplane_variables.current(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (backplane_variables.command_code() == BP_FEE_PRESENT) {
            // Log modules present
            pstmt = con->prepareStatement("INSERT INTO fee_present(id,\
                    fee_index, present) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, backplane_variables.present(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (backplane_variables.command_code() == BP_SET_TRIGGER_MASK) {
            // Log trigger mask
            pstmt = con->prepareStatement("INSERT INTO trigger_mask(id,\
                    fee_index, mask) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < N_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, backplane_variables.trigger_mask(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        }
        delete con;
        std::cout << command_string(backplane_variables.command_code())
            << " logged." << std::endl;
    } catch (sql::SQLException &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
    }
}

// Log target variables from the tm controller
void ServerControl::log_target_variables()
{
    return;
}

// Log high level commands from the interface
void ServerControl::log_interface_command()
{
    return;
}

void ServerControl::log_messages_received()
{
    for (std::vector<Connection>::iterator iter =
            netinfo.connections.begin(); iter != netinfo.connections.end();
            ++iter) {
        // Log backplane variables from the pi, if received
        if ((iter->device == PI) && (iter->recv_status == MSG_DONE)) {
            log_backplane_variables();
        }
        // Log target variables from the tm controller, if received
        if ((iter->device == TM) && (iter->recv_status == MSG_DONE)) {
            log_target_variables();
        }
        // Log high level commands from the interface, if received
        if ((iter->device == GUI) && (iter->recv_status == MSG_DONE)) {
            log_interface_command();
        }
    }
}
