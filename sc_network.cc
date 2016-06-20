// sc_network.cc
// Functions for implementing network communication

// Source for internet sockets code:
// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "sc_network.h"

#define PI_PORT "3141" // the port Pi will connect to server on
#define GUI_PORT "5926" // the port GUI will connect to server on

#define BACKLOG 10 // how many pending connections queue will hold
#define HEADER_LENGTH 2 // length of network short
#define MAX_MESSAGE_LENGTH 512 // maximum packet size not including header

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Open a new connection on the socket with file descriptor new_connection.
 * If hostname is not set, default is to open a server and wait for incoming
 * connections.
 * If hostname is set, will connect to that host.
 * Return true on success, false on failure. */
bool open_connection(int &new_connection, const char *port,
        const char *hostname=NULL)
{
    int connection; // listen on connection
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    int is_server = 1; // default: server if remote host not specified
    if (hostname != NULL)
        is_server = 0; // client

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (is_server)
        hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return false;
    }
    
    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((connection = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (is_server) {
            if (setsockopt(connection, SOL_SOCKET, SO_REUSEADDR, &yes,
                        sizeof(int)) == -1) {
                perror("setsockopt");
                return false;
            }

            if (bind(connection, p->ai_addr, p->ai_addrlen) == -1) {
                close(connection);
                perror("bind");
                continue;
            }
        } else { // is client
            if (connect(connection, p->ai_addr, p->ai_addrlen) == -1) {
                close(connection);
                perror("connect");
                continue;
            }
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    
    if (p == NULL) {
        if (is_server)
            fprintf(stderr, "failed to bind\n");
        else
            fprintf(stderr, "failed to connect\n");
        return false;
    }

    // If server, listen for an incoming connection and accept a new socket
    // from it (then close the first)
    // If client, we're good - just use the socket we have
    if (is_server) {
        if (listen(connection, BACKLOG) == -1) {
            perror("listen");
            return false;
        }

        sin_size = sizeof their_addr;
        while(1) {
            if ((new_connection = accept(connection,
                            (struct sockaddr *)&their_addr,
                            &sin_size)) == -1) {
                perror("accept");
                continue;
            } else {
                break;
            }
        }
        close(connection);
    } else { // is client
        new_connection = connection;
    }
    
    return true;
}

/* Close the connection on socket with file descriptor connection.
 * Return true on success, false on failure. */
bool close_connection(int connection)
{
    if (close(connection) == -1)
        return false;

    return true;
}

/* Send a message to socket with file descriptor connection.
 * Return 0 on success, -1 on error. */
int send_message(int connection, std::string message)
{
    // Prepend total message length as a header for use by receiver
    short header = htons(message.length()); // use portable format
    const int total_length = HEADER_LENGTH + message.length();
    char buffer[total_length];
    memcpy(buffer, &header, HEADER_LENGTH);
    // Intentionally do not copy null terminator from message.c_str()
    memcpy(buffer+HEADER_LENGTH, message.c_str(), message.length());
   
    // Send repeatedly until entire message sent
    int bytes_sent = 0;
    int n;
    while (bytes_sent < total_length) {
        n = send(connection, buffer+bytes_sent,
                std::min((total_length - bytes_sent), MAX_MESSAGE_LENGTH), 0);
        if (n == -1) {
            perror("send");
            return -1;
        }
        bytes_sent += n;
    }
    return 0;
}

/* Receive a message from socket with file descriptor connection.
 * Return 0 on success, -1 on error, -2 on connection closed. */
int receive_message(int connection, std::string &message)
{
    int remaining_length, numbytes;
    char header_buffer[HEADER_LENGTH];
    char message_buffer[MAX_MESSAGE_LENGTH];

    // Read in length of message from header
    if ((numbytes = recv(connection, header_buffer, HEADER_LENGTH, 0)) <= 0) {
        if (numbytes < 0) {
            perror("recv");
            return -1; // error
        }
        return -2; // socket was closed
    }
    remaining_length = ntohs(*(short*)header_buffer);

    // Read in message
    message.clear();
    while (remaining_length > 0) {
        if ((numbytes = recv(connection, message_buffer,
                        std::min(remaining_length, MAX_MESSAGE_LENGTH),
                        0)) <= 0) {
            if (numbytes < 0) {
                perror("recv");
                return -1; // error
            }
            return -2; // socket was closed
        }
        message.append(message_buffer, numbytes);
        remaining_length -= numbytes;
    }

    return 0;
}

/* Set up network and update netinfo, as needed for specified platform 
 * Specify platform as PI, SERVER, or GUI
 * Return true if setup successful, false if error */ 
bool setup_network(Network_info &netinfo, int platform) {
    switch (platform) {
        // Pi: connect to an open port on the server
        case PI:
        {
            std::cout << "connecting to the server..." << std::endl;
            int server = -1;
            if (!open_connection(server, PI_PORT,
                        netinfo.host_name.c_str())) {
                std::cout << "failed to connect to server." << std::endl;
                return false;
            }
            std::cout << "connected to the server!" << std::endl;
            netinfo.connections.push_back(server);
            break;
        }
        // Server: wait for incoming connections from Pi and GUIs
        case SERVER:
        {
            std::cout << "connecting to the Pi..." << std::endl;
            int pi = -1; 
            if (!open_connection(pi, PI_PORT)) {
                std::cout << "failed to connect to Pi." << std::endl;
                return false;
            }
            std::cout << "connected to the Pi!" << std::endl;
            netinfo.connections.push_back(pi);
            std::cout << "connecting to the GUI..." << std::endl;
            int gui = -1; 
            if (!open_connection(gui, GUI_PORT)) {
                std::cout << "failed to connect to GUI." << std::endl;
                return false;
            }
            std::cout << "connected to the GUI!" << std::endl;
            netinfo.connections.push_back(gui);
            break;
        }
        // GUI: connect to an open port on the server
        case GUI:
        {
            std::cout << "connecting to the server..." << std::endl;
            int server = -1;
            if (!open_connection(server, GUI_PORT,
                        netinfo.host_name.c_str())) {
                std::cout << "failed to connect to server." << std::endl;
                return false;
            }
            std::cout << "connected to the server!" << std::endl;
            netinfo.connections.push_back(server);
            break;
        }
        default: // invalid platform specification
            std::cout << "warning: invalid platform specified!" << std::endl;
            return false;
    }
    return true;
}

/* Receive and optionally send messages to and from network, updating netinfo
 * as needed
 * Specify timeout as time to wait for remote response in ms, default 0.1s
 * Return true if incoming message read [and outgoing message sent]
 * Return true and set incoming_message to empty string if timed out
 * Return false if error or connection closed */
bool update_network(Network_info &netinfo, std::string &incoming_message,
        std::string outgoing_message, int timeout) {
    return true;
}
bool update_network(Network_info &netinfo, std::string &incoming_message,
        int timeout) {
    return true;
}

/* Close all connections in network, updating netinfo to reflect changes
 * Return true on success, false if error */
bool shutdown_network(Network_info &netinfo) {
    bool no_errors = true;
    for (std::vector<int>::iterator iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ) {
        if (close_connection(*iter)) {
            iter = netinfo.connections.erase(iter);
        } else {
            no_errors = false;
            ++iter;
        }
    }
    return no_errors;
}
