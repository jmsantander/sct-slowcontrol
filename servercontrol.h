// servercontrol.h
// Header file containing the class for mid-level server control and logging

#ifndef SC_SERVERCONTROL
#define SC_SERVERCONTROL

#include <string>
#include <vector>

#include "runcontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

struct Command {
    int command_code;
    int device;
};

class ServerControl: public RunControl {
protected:
    Network_info netinfo;
    slow_control::TargetVariables target_variables;
    slow_control::TargetCommand target_command;
    slow_control::BackplaneVariables backplane_variables;
    slow_control::BackplaneCommand backplane_command;
    std::vector<Command> commands_to_send;
    bool pi_occupied;
    bool tm_occupied;
    std::string db_host;
    std::string db_username;
    std::string db_password;
public:
    ServerControl(std::string host, std::string username,
            std::string password) : RunControl(), netinfo(SERVER) {
        db_host = host;
        db_username = username;
        db_password = password;
        pi_occupied = false;
        tm_occupied = false;
    }
    bool synchronize_network();
    void prepare_highlevel_command();
    void update_next_command();
    
    // Log backplane variables from the pi
    void log_backplane_variables();
    
    // Log target variables from the tm controller
    void log_target_variables();
    
    // Log high level commands from the interface
    void log_interface_command();
    
    // If a new high level command, updated backplane variables,
    // or updated target variables were received, log them
    void log_messages_received();
};

#endif
