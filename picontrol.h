// picontrol.h
// Header file containing the class for mid-level pi control

#ifndef SC_PICONTROL
#define SC_PICONTROL

#define N_COMMANDS 4

#include <string>

#include "runcontrol.h"
#include "sc_network.h"
#include "sc_protobuf.pb.h"
#include "backplane_lowlevel.h"

class PiControl: public RunControl {
protected:
    Network_info netinfo;
    slow_control::BackplaneVariables backplane_variables;
    slow_control::BackplaneCommand backplane_command;
    bool updates_to_send;
public:
    PiControl(std::string hostname) : RunControl(), netinfo(PI, hostname)
    {
        updates_to_send = false;
        initialize_lowlevel();
    }
    bool synchronize_network();
    void update_backplane_variables();
};

#endif
