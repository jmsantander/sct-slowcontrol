// run_control.cc
// Implementation of the class for mid-level server control and logging

#include <iterator>

#include <iostream>
#include <fstream>
#include <sstream>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "run_control.h"

// TODO: load constants from config file
const int NUM_FEES = 32; // number of modules allowed for in underlying code
const int SPI_MESSAGE_LENGTH = 11;

bool RunControl::synchronize_network()
{
    if (!update_network(netinfo, outgoing_message, outgoing_message_device)) {
        return false;
    }
    outgoing_message = "";
    // Store received messages
    for (auto it = netinfo.connections.begin();
            it != netinfo.connections.end(); ++it) {
        if ((it->device == GUI) && (it->recv_status == MSG_DONE)) {
            it->recv_status = MSG_STANDBY;
            if (!run_settings.ParseFromString(it->message)) {
                return false;
            }
            received_messages.push_back(GUI);
        } else if ((it->device == PI) && (it->recv_status == MSG_DONE)) {
            it->recv_status = MSG_STANDBY;
            if (!backplane_variables.ParseFromString(it->message)) {
                return false;
            }
            received_messages.push_back(PI);
        } else if ((it->device == TM) && (it->recv_status == MSG_DONE)) {
            it->recv_status = MSG_STANDBY;
            if (!target_variables.ParseFromString(it->message)) {
                return false;
            }
            received_messages.push_back(TM);
        }
    }
    return true;
}

// Split a string into component words using the specified delimiter
void split(std::string s, std::vector<std::string> &words, char delim)
{
    words.clear();
    std::stringstream ss(s);
    std::string word;
    while (getline(ss, word, delim)) {
        words.push_back(word);
    }
}

// Get the device code of the matching string
// Return true if match found, false otherwise 
bool get_device_code(std::string code_string, int &device_code)
{
    if (code_string == "PI") {
        device_code = PI;
    } else if (code_string == "TM") {
        device_code = TM;
    } else if (code_string == "RC") {
        device_code = SERVER;
    } else {
        return false;
    }
    return true;
}

// Parse the high level command configuration file, storing a vector of 
// high level command objects, each containing the corresponding vector
// of low level commands
bool RunControl::parse_command_config(std::string command_config_file)
{
    // Set the initial mode
    enum Mode {
        READ_FILE,
        COMMAND,
        HIGH_LEVEL_COMMAND_NAME,
        HIGH_LEVEL_COMMAND
    };
    Mode mode = READ_FILE;

    // Load the command config file
    std::ifstream ccfile(command_config_file);

    // Parse line by line
    std::string line;
    std::string orig_line;
    int line_counter = 0;
    while (std::getline(ccfile, line)) {
        // Get info for error messages
        line_counter++;
        orig_line = line;

        // Remove comments
        auto pos = line.find('#');
        if (pos != std::string::npos) {
            line.erase(pos);
        }
        // Skip lines that are empty or only contained comments
        if (line.empty()) {
            continue;
        }
        // Skip lines containing only whitespace
        const std::string whitespace = " \n\r\t";
        pos = line.find_first_not_of(whitespace);
        if (pos == std::string::npos) {
            continue;
        }
        // Remove leading and trailing whitespace 
        line = line.substr(pos, line.find_last_not_of(whitespace) + 1);
        
        // Break up line into words
        std::vector<std::string> words;
        split(line, words, ' ');

        // Parse the line depending on current mode
        bool error_on_line = false;
        switch(mode)
        {
            case READ_FILE:
            {
                if (line == "BEGIN DEFINITIONS") {
                    mode = COMMAND;
                } else if (line == "BEGIN SEQUENCE") {
                    mode = HIGH_LEVEL_COMMAND_NAME;
                } else {
                    // Not a valid line
                    error_on_line = true;
                }
                break;
            }
            case COMMAND:
            {
                if (line == "END DEFINITIONS") {
                    mode = READ_FILE;
                } else if (words.size() == 5) {
                    // Define a new command
                    CommandDefinition new_command;
                    // Parse command name
                    new_command.command_name = words[1];
                    // Parse device code
                    if (!get_device_code(words[0], new_command.device)) {
                        std::cerr << "unknown device code " << words[0]
                            << std::endl;
                        error_on_line = true;
                        break;
                    }
                    // Parse number of unsigned integer arguments
                    try {
                        new_command.n_ints = std::stoi(words[2]);
                    } catch (...) {
                        std::cerr << "could not convert " << words[2]
                            << " to int" << std::endl;
                        error_on_line = true;
                        break;
                    }
                    // Parse number of float arguments
                    try {
                        new_command.n_floats = std::stoi(words[3]);
                    } catch (...) {
                        std::cerr << "could not convert " << words[3]
                            << " to int" << std::endl;
                        error_on_line = true;
                        break;
                    }
                    // Parse number of string arguments
                    try {
                        new_command.n_strings = std::stoi(words[4]);
                    } catch (...) {
                        std::cerr << "could not convert " << words[4]
                            << " to int" << std::endl;
                        error_on_line = true;
                        break;
                    }
                    // Record the new command definition
                    command_definitions.push_back(new_command);
                } else {
                    // Not a valid line
                    error_on_line = true;
                }
                break;
            }
            case HIGH_LEVEL_COMMAND_NAME:
            {
                if (words.size() == 1) {
                    // Check that high level command name isn't already used
                    std::string high_level_command_name = words[0];
                    for (auto it = high_level_commands.begin();
                            it != high_level_commands.end(); ++it) {
                        if (it->command_name == high_level_command_name) {
                            std::cerr << "high level command name already used"
                                << std::endl;
                            error_on_line = true;
                            break;
                        }
                    }
                    if (error_on_line) {
                        break;
                    }
                    // Create a new high level command
                    HighLevelCommand new_high_level_command;
                    new_high_level_command.command_name =
                        high_level_command_name;
                    high_level_commands.push_back(new_high_level_command);
                    mode = HIGH_LEVEL_COMMAND;
                } else {
                    // Invalid high level command name
                    error_on_line = true;
                }
                break;
            }
            case HIGH_LEVEL_COMMAND:
            {
                if (line == "END SEQUENCE") {
                    mode = READ_FILE;
                } else if ((words.size() >= 2) && !(words.size() % 2)) {
                    // Parse device code
                    int device_code;
                    if (!get_device_code(words[0], device_code)) {
                        std::cerr << "unknown device code " << words[0]
                            << std::endl;
                        error_on_line = true;
                        break;
                    }
                    // Parse command name
                    std::string command_name = words[1];
                    // Require device, command name, and even number of
                    // arguments (label-value pairs)
                    LowLevelCommand new_low_level_command;
                    int len_words = words.size();
                    for (int i = 2; i < len_words; i+=2) {
                        if ((i == 2) && (words[i] == "CHK")) {
                            // Parse optional command priority
                            try {
                                new_low_level_command.priority =
                                    std::stoi(words[3]);
                            } catch (...) {
                                std::cerr << "could not convert "
                                    << words[3] << " to int" << std::endl;
                                error_on_line = true;
                                break;
                            }
                        } else if (words[i] == "INT") {
                            // Parse optional unsigned integer argument
                            try {
                                new_low_level_command.int_args.push_back(
                                        std::stoi(words[i + 1]));
                            } catch (...) {
                                std::cerr << "could not convert "
                                    << words[i + 1] << " to int" << std::endl;
                                error_on_line = true;
                                break;
                            }
                        } else if (words[i] == "FLT") {
                            // Parse optional float argument
                            try {
                                new_low_level_command.float_args.push_back(
                                        std::stof(words[i + 1]));
                            } catch (...) {
                                std::cerr << "Error: could not convert "
                                    << words[i + 1] << " to float" << std::endl;
                                error_on_line = true;
                                break;
                            }
                        } else if (words[i] == "STR") {
                            // Parse optional string argument
                            new_low_level_command.string_args.push_back(
                                        words[i + 1]);
                        } else {
                            // Not a valid argument
                            error_on_line = true;
                            break;
                        }
                    }
                    // Match command to a known definition
                    bool match_found = false;
                    for (std::vector<CommandDefinition>::iterator iter =
                            command_definitions.begin();
                            iter != command_definitions.end(); ++iter) {
                        if ((command_name == iter->command_name)
                                && (device_code == iter->device)
                                && (new_low_level_command.int_args.size() ==
                                    (std::size_t) iter->n_ints)
                                && (new_low_level_command.float_args.size() ==
                                    (std::size_t) iter->n_floats)
                                && (new_low_level_command.string_args.size() ==
                                    (std::size_t) iter->n_strings)) {
                            match_found = true;
                            new_low_level_command.def = *iter;
                            // Add to the current high level command
                            high_level_commands.back().commands.push_back(
                                    new_low_level_command);
                            break;
                        }
                    }
                    if (!match_found) {
                        std::cerr << "no match found for command "
                             << command_name << " on specified device with "
                             << "number of parameters given" << std::endl;
                        error_on_line = true;
                    }
                } else {
                    // Invalid line
                    error_on_line = true;
                }
                break;
            }
        }
        if (error_on_line) {
            std::cerr << "Error: could not parse line " << line_counter << ':'
                << std::endl << orig_line << std::endl;
            return false;
        }
    }
    if (mode != READ_FILE) {
        std::cerr << "Error: end of config file reached with mode " << mode
            << std::endl;
        return false;
    }
    return true;
}

// Translate a low level command protocol buffer into the corresponding struct
void write_command_buffer_to_struct(
        slow_control::LowLevelCommand command_buffer,
        LowLevelCommand &command_struct)
{
    command_struct.def.command_name = command_buffer.command_name();
    command_struct.def.device = command_buffer.device();
    command_struct.def.n_ints = command_buffer.int_args_size();
    command_struct.def.n_floats = command_buffer.float_args_size();
    command_struct.def.n_strings = command_buffer.string_args_size();
    command_struct.priority = command_buffer.priority();
    for (int i = 0; i < command_struct.def.n_ints; i++) {
        command_struct.int_args.push_back(command_buffer.int_args(i));
    }
    for (int i = 0; i < command_struct.def.n_floats; i++) {
        command_struct.float_args.push_back(command_buffer.float_args(i));
    }
    for (int i = 0; i < command_struct.def.n_strings; i++) {
        command_struct.string_args.push_back(command_buffer.string_args(i));
    }
}

void write_command_struct_to_buffer(LowLevelCommand command_struct,
        slow_control::LowLevelCommand &command_buffer)
{
    command_buffer.set_command_name(command_struct.def.command_name);
    command_buffer.set_device(command_struct.def.device);
    command_buffer.set_priority(command_struct.priority);
    command_buffer.clear_int_args();
    command_buffer.clear_float_args();
    command_buffer.clear_string_args();
    for (int i = 0; i < command_struct.def.n_ints; i++) {
        command_buffer.add_int_args(command_struct.int_args[i]);
    }
    for (int i = 0; i < command_struct.def.n_floats; i++) {
        command_buffer.add_float_args(command_struct.float_args[i]);
    }
    for (int i = 0; i < command_struct.def.n_strings; i++) {
        command_buffer.add_string_args(command_struct.string_args[i]);
    }
}

void RunControl::send_next_command()
{
    // If no commands to perform, nothing to do
    if (command_queue.empty()) {
        return;
    }

    // If any active commands have a priority overriding the next one, don't 
    // send anything
    for (auto it = active_commands.begin(); it != active_commands.end();
            ++it) {
        // priority level 1: block new commands from same device
        // priority level 2: block new commands from any device
        if ((it->priority == 2)
                || ((it->def.device == command_queue.front().def.device)
                    && (it->priority == 1))) {
            return;
        }
    }

    // OK to send command. Set as outgoing message for next synchronization
    if (command_queue.front().def.device == PI) {
        write_command_struct_to_buffer(command_queue.front(),
                backplane_command);
        backplane_command.SerializeToString(&outgoing_message);
    } else if (command_queue.front().def.device == TM) {
        write_command_struct_to_buffer(command_queue.front(),
                target_command);
        target_command.SerializeToString(&outgoing_message);
    }
    outgoing_message_device = command_queue.front().def.device;

    // Move the command to active commands vector
    active_commands.push_back(command_queue.front());
    command_queue.pop();
}

// Log backplane variables from the pi
void RunControl::log_backplane_variables()
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
        pstmt->setString(1, backplane_variables.command().command_name());
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
            for (int spi_index = 0; spi_index < SPI_MESSAGE_LENGTH;
                    spi_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, spi_index);
                pstmt->setInt(3, spi_message_index);
                pstmt->setInt(4,
                        backplane_variables.spi_command(spi_message_index *
                            SPI_MESSAGE_LENGTH + spi_index));
                pstmt->setInt(5, 
                        backplane_variables.spi_data(spi_message_index *
                            SPI_MESSAGE_LENGTH + spi_index));
                pstmt->executeUpdate();
            }
        }
        delete pstmt;
        
        // Log additional data if required by command
        if (backplane_variables.command().command_name() ==
                "read_module_voltages") {
            // Log FEE voltages
            pstmt = con->prepareStatement("INSERT INTO fee_voltage(id,\
                    fee_index, voltage) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < NUM_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, backplane_variables.voltage(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (backplane_variables.command().command_name() ==
                "read_module_currents") {
            // Log FEE currents
            pstmt = con->prepareStatement("INSERT INTO fee_current(id,\
                  fee_index, current) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < NUM_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setDouble(3, backplane_variables.current(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if (backplane_variables.command().command_name() ==
                "read_modules_present") {
            // Log modules present
            pstmt = con->prepareStatement("INSERT INTO fee_present(id,\
                    fee_index, present) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < NUM_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, backplane_variables.present(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        } else if ((backplane_variables.command().command_name() ==
                    "set_trigger_mask_from_file")
                || (backplane_variables.command().command_name() ==
                    "close_trigger_mask")) {
            // Log trigger mask
            pstmt = con->prepareStatement("INSERT INTO trigger_mask(id,\
                    fee_index, mask) VALUES (?, ?, ?)");
            for (int fee_index = 0; fee_index < NUM_FEES; fee_index++) {
                pstmt->setInt(1, id);
                pstmt->setInt(2, fee_index);
                pstmt->setInt(3, backplane_variables.trigger_mask(fee_index));
                pstmt->executeUpdate();
            }
            delete pstmt;
        }
        delete con;
        std::cout << backplane_variables.command().command_name()
            << " logged." << std::endl;
    } catch (sql::SQLException &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
    }
}

// Log target variables from the tm controller
void RunControl::log_target_variables()
{
    return;
}

// Log high level commands from the interface
void RunControl::log_interface_command()
{
    return;
}

void RunControl::process_received_messages()
{
    for (auto it = received_messages.begin();
            it != received_messages.end(); ) {
        int device = *it;
        it = received_messages.erase(it);
        // If GUI, break down high level command into low level components
        if (device == GUI) {
            // Add the low level commands for the received high level
            // command to queue (if there's a matching entry)
            for (auto hl_cmd_it = high_level_commands.begin();
                    hl_cmd_it != high_level_commands.end(); ++hl_cmd_it) {
                if (run_settings.high_level_command() == 
                        hl_cmd_it->command_name) {
                    for (auto ll_cmd_it = hl_cmd_it->commands.begin();
                            ll_cmd_it != hl_cmd_it->commands.end();
                            ++ll_cmd_it) {
                        command_queue.push(*ll_cmd_it);
                    }
                    break;
                }
            }
            // Log high level commands from the interface
            log_interface_command();
            continue;
        }
        // Since not GUI, it's a client
        // Extract command for comparison to active commands
        LowLevelCommand received_command;
        if (device == PI) {
            write_command_buffer_to_struct(backplane_variables.command(),
                    received_command);
        } else if (device == TM) {
            write_command_buffer_to_struct(target_variables.command(),
                    received_command);
        } else { // not a valid client, shouldn't happen
            continue;
        }
        // Since message received, remove corresponding command from list
        // of active commands
        for (auto active_cmd_it = active_commands.begin();
                active_cmd_it != active_commands.end(); ++active_cmd_it) {
            if (*active_cmd_it == received_command) {
                active_commands.erase(active_cmd_it);
                break;
            }
        }
        if (device == PI) {
            // Log backplane variables from the pi
            log_backplane_variables();
        } else if (device == TM) {
            // Log target variables from the tm controller
            log_target_variables();
        }
    }
}
