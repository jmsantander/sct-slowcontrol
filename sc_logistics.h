// Functions for slow control logistics, including logging

#include <string>

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value);

// Sleep for a given number of milliseconds
void sleep_msec(int msec);
