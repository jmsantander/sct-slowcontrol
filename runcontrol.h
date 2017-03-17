// runcontrol.h
// Header file containing the class for high-level run control
// Lower level programs inherit from this, adding their specific functionality

#ifndef SC_RUNCONTROL
#define SC_RUNCONTROL

// Codes for updating data and settings on backplane
#define BP_NONE 0
#define BP_FEE_VOLTAGES 1
#define BP_FEE_CURRENTS 2
#define BP_FEE_PRESENT 3
#define BP_RESET_TRIGGER_AND_NSTIMER 4
#define BP_SET_TRIGGER 5
#define BP_READ_NSTIMER_TRIGGER_RATE 6
#define BP_ENABLE_DISABLE_TRIGGER 7
#define BP_SET_HOLDOFF_TIME 8
#define BP_SET_TACK_TYPE_AND_MODE 9
#define BP_POWER_CONTROL_MODULES 10
#define BP_SYNC 11
#define BP_SET_TRIGGER_MASK 12

#define N_FEES 32 // number of FEEs
#define N_SPI 11 // length of an SPI data array
#define N_SPI_MESSAGES 4 // max number of SPI messages from a single command

#include <string>

#include "sc_protobuf.pb.h"

class RunControl
{
protected:
    slow_control::RunSettings run_settings;
public:
    RunControl();
    const std::string highlevel_command() {
        return run_settings.highlevel_command();
    }
    const std::string highlevel_parameter() {
        return run_settings.highlevel_parameter();
    }
};

#endif
