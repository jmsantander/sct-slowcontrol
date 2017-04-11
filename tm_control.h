// tm_control.h
// Header file containing the class for mid-level target control

#ifndef TM_CONTROL_H
#define TM_CONTROL_H

#include <string>

#include "network.h"
#include "slow_control.pb.h"

class TMControl {
protected:
    Network_info netinfo;
    slow_control::TargetVariables target_variables;
    slow_control::LowLevelCommand target_command;
    bool updates_to_send;
public:
    TMControl(std::string hostname) : netinfo(TM, hostname) {
        updates_to_send = false;
    }
    bool synchronize_network();
    bool command_received();
    void save_updated_variables();
};

#endif
