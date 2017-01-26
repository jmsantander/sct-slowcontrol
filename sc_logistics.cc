// Implementation of slow control logistics functions

#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>

#include "sc_logistics.h"

// Split a string into component words using the specified delimiter
void split(const std::string &s, char delim, std::vector<std::string> &words)
{
    std::stringstream ss(s);
    std::string word;
    while (getline(ss, word, delim)) {
        words.push_back(word);
    }
}

// Read in and store a command from the user from stdin
// Return true if the command is valid, false otherwise
bool read_command(std::string &command, std::string &value)
{
    std::string s;
    std::vector<std::string> words;
    
    std::cout << "Enter command: ";

    std::getline(std::cin, s);
    split(s, ' ', words);

    // Require either a single word command, or a word followed by a value
    if ((words.size() < 1) || (words.size() > 2)) {
        return false;
    }
    command = words[0];
    if (words.size() == 2) {
        value = words[1];
    } else {
        value = "";
    }

    return true;
}

void sleep_msec(int msec)
{
    struct timespec tim;
    const long NSEC = 1000000 * msec; // convert millisec to nanosec
    tim.tv_sec = 0;
    tim.tv_nsec = NSEC;

    nanosleep(&tim, NULL);
}
