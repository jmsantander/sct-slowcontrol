// interfacecontrol.h
// Header file containing the class for the high-level user interface

#ifndef SC_INTERFACECONTROL
#define SC_INTERFACECONTROL

#include <string>
#include <vector>

#include "runcontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"

class InterfaceControl: public RunControl {
protected:
    Network_info netinfo;
    slow_control::MessageWrapper message_wrap;
    slow_control::TargetVariables target_variables;
    slow_control::BackplaneVariables backplane_variables;
    bool updates_to_send;
public:
    InterfaceControl(std::string hostname) : RunControl(), netinfo(GUI,
            hostname) {
        updates_to_send = false;
    }
    bool synchronize_network();
    void update_highlevel_command(std::string highlevel_command,
            std::string highlevel_parameter);
    slow_control::TargetVariables target_variables_message() {
        return target_variables;
    }
    slow_control::BackplaneVariables backplane_variables_message() {
        return backplane_variables;
    }
};

#endif
