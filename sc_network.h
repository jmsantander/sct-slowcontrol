// Functions for implementing slow control network communication

#ifndef SC_NETWORK
#define SC_NETWORK

#include <string>
#include <vector>

// Codes for automatically setting up networking on appropriate platform
#define PI 0
#define SERVER 1
#define GUI 2

// Holds networking information 
struct Network_info {
    std::vector<int> connections;
    std::string host_name; // server host
    Network_info() {};
    Network_info(std::string init_host_name) {
        host_name = init_host_name;
    }
};

/* Set up network and update netinfo, as needed for specified platform 
 * Specify platform as PI, SERVER, or GUI
 * Return true if setup successful, false if error */ 
bool setup_network(Network_info &netinfo, int platform);

/* Receive and optionally send messages to and from network, updating netinfo
 * as needed
 * Specify timeout as time to wait for remote response in ms, default 0.1s
 * Return true if incoming message read [and outgoing message sent]
 * Return true and set incoming_message to empty string if timed out
 * Return false if error or connection closed */
bool update_network(Network_info &netinfo, std::string &incoming_message,
        std::string outgoing_message, int timeout=100);
bool update_network(Network_info &netinfo, std::string &incoming_message,
        int timeout=100);

/* Close all connections in network, updating netinfo to reflect changes
 * Return true on success, false if error */
bool shutdown_network(Network_info &netinfo);

#endif
