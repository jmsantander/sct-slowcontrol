// interfacecontrol.h
// Header file containing the class for the high-level user interface

#ifndef SC_INTERFACECONTROL
#define SC_INTERFACECONTROL

#include <string>
#include <vector>

#include "sc_network.h"
#include "sc_protobuf.pb.h"

class InterfaceControl {
protected:
    Network_info netinfo;
    slow_control::RunSettings run_settings;
    slow_control::MessageWrapper message_wrap;
    slow_control::TargetVariables target_variables;
    slow_control::BackplaneVariables backplane_variables;
    bool updates_to_send;
    int message_received;
public:
    InterfaceControl(std::string hostname) : netinfo(GUI,
            hostname) {
        // Verify that the version of the Protocol Buffer library we linked
        // against is compatible with the version of the headers we compiled
        // against.
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        updates_to_send = false;
        message_received = slow_control::MessageWrapper::NONE;
    }
    bool synchronize_network();
    
    void update_highlevel_command(std::string highlevel_command,
            std::string highlevel_parameter="");
    
    bool exit() {
        return (shutdown_network(netinfo));
    }
    
    bool backplane_variables_received() {
        return (message_received == slow_control::MessageWrapper::BP_VARS);
    }
    
    bool target_variables_received() {
        return (message_received == slow_control::MessageWrapper::TM_VARS);
    }
    
    slow_control::TargetVariables target_variables_message() {
        return target_variables;
    }
    
    slow_control::BackplaneVariables backplane_variables_message() {
        return backplane_variables;
    }
};

#endif
