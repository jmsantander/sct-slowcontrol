// run_control.h
// Header file containing the class for mid-level server control and logging

#ifndef RUN_CONTROL_H
#define RUN_CONTROL_H

#include <string>
#include <vector>
#include <queue>

#include "network.h"
#include "slow_control.pb.h"

struct CommandDefinition {
    std::string command_name;
    int device; // code for device to send to (PI or TM)
    int n_ints; // number of integer arguments
    int n_floats; // number of float arguments
    int n_strings; // number of string arguments
    bool operator==(const CommandDefinition& rhs) const {
        return ((command_name == rhs.command_name)
                && (device == rhs.device)
                && (n_ints == rhs.n_ints)
                && (n_floats == rhs.n_floats)
                && (n_strings == rhs.n_strings));
    }
};

struct LowLevelCommand {
    CommandDefinition def;
    int priority = 0; // code for command priority; default is lowest priority
    std::vector<unsigned int> int_args;
    std::vector<float> float_args;
    std::vector<std::string> string_args;
    bool operator==(const LowLevelCommand& rhs) const {
        return ((def == rhs.def)
                && (priority == rhs.priority)
                && (int_args == rhs.int_args)
                && (float_args == rhs.float_args) 
                && (string_args == rhs.string_args));
    }
};

struct HighLevelCommand {
    std::string command_name;
    std::vector<LowLevelCommand> commands;
};

class RunControl {
protected:
    Network_info netinfo;
    slow_control::RunSettings run_settings;
    slow_control::TargetVariables target_variables;
    slow_control::LowLevelCommand target_command;
    slow_control::BackplaneVariables backplane_variables;
    slow_control::LowLevelCommand backplane_command;

    std::vector<CommandDefinition> command_definitions;
    std::vector<HighLevelCommand> high_level_commands;
    std::vector<LowLevelCommand> active_commands;
    std::queue<LowLevelCommand> command_queue;

    std::string outgoing_message;
    int outgoing_message_device;
    std::vector<int> received_messages; // codes of devices that got messages

    std::string db_host;
    std::string db_username;
    std::string db_password;
public:
    RunControl(std::string host, std::string username,
            std::string password) : netinfo(SERVER) {
        // Verify that the version of the Protocol Buffer library we linked
        // against is compatible with the version of the headers we compiled
        // against.
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        db_host = host;
        db_username = username;
        db_password = password;
        outgoing_message = "";
        outgoing_message_device = -1;
    }
    // Parse the high level command configuration file, storing a vector of 
    // high level command objects, each containing the corresponding vector
    // of low level commands
    bool parse_command_config(std::string command_config_file);

    bool synchronize_network();

    // If a low level command is awaiting in the queue, send it to the 
    // appropriate device to be performed on next synchronization
    void send_next_command();
    
    // Log backplane variables from the pi
    void log_backplane_variables();
    
    // Log target variables from the tm controller
    void log_target_variables();
    
    // Log high level commands from the interface
    void log_interface_command();
    
    // If a new high level command, updated backplane variables,
    // or updated target variables were received, log them
    void process_received_messages();
};

#endif
