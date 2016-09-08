// Functions for implementing low level communication to the backplane

#ifndef SC_LOWLEVEL
#define SC_LOWLEVEL

// Initialize low level SPI communication
// Return true if successful, false otherwise
bool initialize_lowlevel();

// Read in and store FEE housekeeping voltages
void read_voltages(float *voltages, int n_fees);

#endif
