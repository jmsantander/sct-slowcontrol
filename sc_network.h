// Functions for implementing slow control network communication

#ifndef SC_NETWORK
#define SC_NETWORK

#include <string>
#include <vector>

// Codes for automatically setting up networking on appropriate device
#define PI 0
#define SERVER 1
#define GUI 2

// Holds info for a single connection, including the incoming message
struct Connection {
    int socket;
    std::string message;
    bool unread_message;
    Connection(int init_socket) {
        socket = init_socket;
        unread_message = false;
    }
};

// Holds networking information 
struct Network_info {
    std::vector<Connection> connections;
    int device;
    std::string host_name; // host of server
    Network_info() {};
    Network_info(std::string init_host_name) {
        host_name = init_host_name;
    }
};

/* Set up network and update netinfo, as needed for specified device 
 * Specify device as PI, SERVER, or GUI
 * Return true if setup successful, false if error */ 
bool setup_network(Network_info &netinfo, int device);

/* Receive and optionally send messages to and from network, updating netinfo
 * with any received messages and by removing any closed connections
 *
 * If outgoing message is empty string, will not send anything
 * Specify timeout as time to wait for remote response in ms, default 0.1s
 * If timeout is set to be negative, will wait forever
 *
 * Return true if incoming message read [and outgoing message sent]
 * Return true and set incoming_message to empty string if timed out
 * Return false if error or connection closed */
bool update_network(Network_info &netinfo, std::string outgoing_message="",
        int timeout=100);

/* Close all connections in network, updating netinfo to reflect changes
 * Return true on success, false if error */
bool shutdown_network(Network_info &netinfo);

#endif
