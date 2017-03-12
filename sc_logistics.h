// Functions for slow control logistics, including logging
// Includes defines used by all files

#ifndef SC_LOGISTICS
#define SC_LOGISTICS

// Codes for updating data and settings on backplane
#define BP_NONE 0
#define FEE_VOLTAGES 1
#define FEE_CURRENTS 2
#define FEE_PRESENT 3
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
#define N_COMMANDS 4 // number of commands for sending settings
#define N_SPI 11 // length of an SPI data array
#define N_MESSAGES 4 // maximum number of SPI messages from a single command

// Sleep for a given number of milliseconds
void sleep_msec(int msec);

#endif
