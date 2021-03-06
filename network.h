// Functions for implementing slow control network communication

#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

// Device codes for automatically setting up networking
#define PI 0
#define SERVER 1
#define GUI 2
#define TM 3

// Status codes for sending and receiving messages
#define MSG_STANDBY 0
#define MSG_READY 1
#define MSG_DONE 2
#define MSG_ERROR 3
#define MSG_CLOSED 4

#define INVALID_SOCKET -1

// Holds info for a single connection, including the incoming message
struct Connection {
    int socket;
    int device;
    std::string message;
    int recv_status;
    int send_status;
    Connection(int init_socket, int init_device) {
        socket = init_socket;
        device = init_device;
        recv_status = MSG_STANDBY;
        send_status = MSG_STANDBY;
    }
};

// Holds networking information 
// Specify device as PI, SERVER, TM, or GUI
struct Network_info {
    std::vector<Connection> connections;
    int device;
    std::string host_name; // host of server
    // listeners for server only to receive connections
    int pi_listener, tm_listener, gui_listener;
    Network_info() {};
    Network_info(int init_device, std::string init_host_name="") {
        device = init_device;
        host_name = init_host_name;
        pi_listener = INVALID_SOCKET;
        tm_listener = INVALID_SOCKET;
        gui_listener = INVALID_SOCKET;
    }
};

/* Set up network and update netinfo, as needed for specified device 
 * Specify device as PI, SERVER, TM, or GUI
 * Return true if setup successful, false if error */ 
bool setup_network(Network_info &netinfo, int device);

/* Receive and optionally send messages to and from network, updating netinfo
 * with any received messages and by removing any closed connections
 *
 * If outgoing message is empty string, will not send anything
 * message_device specifies destination for outgoing message
 * Specify timeout as time to wait for remote response in ms, default 0.5s
 * If timeout is set to be negative, will wait forever
 *
 * Return true if incoming message read [and outgoing message sent]
 * Return true and set incoming_message to empty string if timed out
 * Return false if error or connection closed */
bool update_network(Network_info &netinfo, std::string outgoing_message="", int message_device=SERVER, int timeout=500);

/* Close all connections in network, updating netinfo to reflect changes
 * Return true on success, false if error */
bool shutdown_network(Network_info &netinfo);

#endif
