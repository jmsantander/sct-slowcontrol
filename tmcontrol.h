// tmcontrol.h
// Header file containing the class for mid-level target control

#ifndef SC_TMCONTROL
#define SC_TMCONTROL

#include <string>

#include "runcontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

class TMControl: public RunControl {
protected:
    Network_info netinfo;
    slow_control::TargetVariables target_variables;
    slow_control::TargetCommand target_command;
    bool updates_to_send;
public:
    TMControl(std::string hostname) : RunControl(), netinfo(TM, hostname) {
        updates_to_send = false;
    }
    bool synchronize_network();
    bool command_received();
    void save_updated_variables();
    const std::string command() { return target_command.command(); }
    const std::string parameter() { return target_command.parameter(); }
};

#endif
