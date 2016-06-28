// sc_network.cc
// Functions for implementing network communication

// Source for internet sockets code:
// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
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

/* Close the socket with file descriptor sockfd.
 * Return true on success, false on failure. */
bool close_socket(int &sockfd)
{
    if (close(sockfd) == -1) {
        return false;
    }
    sockfd = INVALID_SOCKET;

    return true;
}

/* Open a socket with file descriptor sockfd usable for listening for
 * incoming connections on the specified port.
 * Return true on success, false on failure */
bool listen_socket(int &sockfd, std::string port)
{
    int rv;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1; // for setsockopt()
    
    // Get address info using the settings specified in hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return false;
    }
    
    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == INVALID_SOCKET) {
            perror("socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            sockfd = INVALID_SOCKET;
            return false;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    
    if (p == NULL) {
        std::cerr << "failed to bind" << std::endl;
        sockfd = INVALID_SOCKET;
        return false;
    }
   
    // Listen on the socket
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        close_socket(sockfd);
        return false;
    }

    return true;
}

/* Open a socket with file descriptor sockfd connected to the specified remote
 * host on the specified port.
 * Return true on success, false on failure */
bool connect_socket(int &sockfd, std::string port, std::string host_name)
{   
    int rv;
    struct addrinfo hints, *servinfo, *p;

    // Get address info using the settings specified in hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host_name.c_str(), port.c_str(), &hints,
                    &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return false;
    }

    // Loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == INVALID_SOCKET) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo); // all done with this structure
    
    if (p == NULL) {
        std::cerr << "failed to connect" << std::endl;
        sockfd = INVALID_SOCKET;
        return false;
    }

    return true;
}

/* Open a socket with file descriptor sockfd connected to the first incoming
 * connection on the specified listener within the specified timeout in ms.
 * Return true on success, false on error or timeout */
bool accept_socket(int &sockfd, int &listener, std::string port,
        int timeout=100)
{
    // Confirm that listener is valid
    if (listener == INVALID_SOCKET) {
        if (!listen_socket(listener, port)) {
            return false;
        }
    }

    // Check if any remote connections
    int rv;
    struct pollfd ufds[1];
    ufds[0].fd = listener;
    ufds[0].events = POLLIN;
    
    rv = poll(ufds, 1, timeout);

    // Accept incoming connection or handle error
    if (rv == -1) { // error with poll
        perror("poll");
        return false;
    } else if (rv == 0) { // timeout
        return false;
    } else if (ufds[0].revents & POLLIN) { // incoming connection ready!
        struct sockaddr_storage their_addr; // connector's address information
        socklen_t sin_size = sizeof their_addr;
        if ((sockfd = accept(listener, (struct sockaddr *)&their_addr,
                        &sin_size)) == INVALID_SOCKET) {
            perror("accept");
            return false;
        }
        return true;
    } else if ((ufds[0].revents & POLLERR) || (ufds[0].revents & POLLNVAL)) {
        // error with listener
        close_socket(listener);
        return false;
    } else { // unanticipated condition
        return false;
    }
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

/* Count how many connections of the specified device are present in netinfo
 * Do not validate whether or not the connections actually work
 * Return the number of connections found */
int count_connections(Network_info &netinfo, int device)
{
    int connections = 0;
    for (std::vector<Connection>::iterator iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        if (iter->device == device) {
            connections += 1;
        }
    }
    return connections;
}

/* Remove all connections containing any send or receive errors from netinfo
 * Return true on success, false on failure */
bool clean_connections(Network_info &netinfo)
{
    bool no_errors = true;
    for (std::vector<Connection>::iterator iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ) {
        if ((iter->recv_status == MSG_CLOSED) ||
                (iter->send_status == MSG_CLOSED)) {
            iter = netinfo.connections.erase(iter);
            continue;
        } else if ((iter->recv_status == MSG_ERROR) ||
                (iter->send_status == MSG_ERROR)) {
            if (!close_socket(iter->socket)) {
                no_errors = false;
            }
            iter = netinfo.connections.erase(iter);
            continue;
        } else {
            ++iter;
        }
    }
    return no_errors;
}

/* Use poll() to determine which connections are open for sending and receiving
 * within the specified timeout in ms
 * Return true on success, false on any error */
bool poll_connections(Network_info &netinfo, int timeout)
{
    int rv;
    const int n_connections = netinfo.connections.size();
    struct pollfd ufds[n_connections];

    // Set up pollfd structs
    for (int i = 0; i < n_connections; i++) {
        ufds[i].fd = netinfo.connections[i].socket;
        ufds[i].events = POLLIN | POLLOUT;
    }
    
    // Wait for events on the sockets, with the specified timeout
    rv = poll(ufds, n_connections, timeout);
    
    // Check for error
    if (rv == -1) {
        return false;
    }

    // Record the results of the poll in netinfo
    for (int i = 0; i < n_connections; i++) {
        if (ufds[i].revents & POLLIN) {
            netinfo.connections[i].recv_status = MSG_READY;
        }
        if (ufds[i].revents & POLLOUT) {
            netinfo.connections[i].send_status = MSG_READY;
        }
        if (ufds[i].revents & POLLERR) {
            netinfo.connections[i].recv_status = MSG_ERROR;
            netinfo.connections[i].send_status = MSG_ERROR;
        }
        if (ufds[i].revents & POLLNVAL) {
            netinfo.connections[i].recv_status = MSG_ERROR;
            netinfo.connections[i].send_status = MSG_ERROR;
        }
        if (ufds[i].revents & POLLHUP) {
            netinfo.connections[i].recv_status = MSG_CLOSED;
            netinfo.connections[i].send_status = MSG_CLOSED;
        }
    }

    return true;
}

/* Add a connection of the specified device to netinfo
 * Return true if connection added, false if not */
bool add_connection(Network_info &netinfo, int device)
{
    int sockfd = INVALID_SOCKET;
    std::string port;

    switch(netinfo.device) {
        case PI:
        {
            port = PI_PORT;
            if (!connect_socket(sockfd, port, netinfo.host_name)) {
                return false;
            }
            break;
        }
        case SERVER:
        {
            // Accept connection from appropriate device
            if (device == PI) {
                port = PI_PORT;
                if (!accept_socket(sockfd, netinfo.pi_listener, port)) {
                    return false;
                }
            } else if (device == GUI) {
                port = GUI_PORT;
                if (!accept_socket(sockfd, netinfo.gui_listener, port)) {
                    return false;
                }
            } else {
                std::cout << "Error: server must connect to Pi or GUI"
                    << std::endl;
                return false;
            }
            break;
        }
        case GUI:
        {
            port = GUI_PORT;
            if (!connect_socket(sockfd, port, netinfo.host_name)) {
                return false;
            }
            break;
        }
    }
        
    netinfo.connections.push_back(Connection(sockfd, device));

    return true;
}

/* Set up network and update netinfo, as needed for specified device 
 * Specify device as PI, SERVER, or GUI
 * Return true if setup successful, false if error */ 
bool setup_network(Network_info &netinfo, int device)
{
    switch (device) {
        // Pi and GUI: connect to an open port on the server
        case PI:
        case GUI:
        {
            // Connect to a server if not already connected
            if (count_connections(netinfo, SERVER) == 0) {
                if (!add_connection(netinfo, SERVER)) {
                    return false;
                }
                std::cout << "connected to the server!" << std::endl;
            }
            break;
        }
        // Server: wait for incoming connections from Pi and GUIs
        case SERVER:
        {
            bool could_not_connect = false; // must connect to both Pi and GUI
            // Connect to a Pi if not already connected
            if (count_connections(netinfo, PI) == 0) {
                if (!add_connection(netinfo, PI)) {
                    could_not_connect = true;
                } else {
                    std::cout << "connected to the Pi!" << std::endl;
                }
            }
            // Connect to a GUI if not already connected
            if (count_connections(netinfo, GUI) == 0) {
                if (!add_connection(netinfo, GUI)) {
                    could_not_connect = true;
                } else {
                    std::cout << "connected to the GUI!" << std::endl;
                }
            }
            if (could_not_connect) {
                return false;
            }
            break;
        }
        default: // invalid device specification
            std::cout << "warning: invalid device specified!" << std::endl;
            return false;
    }
    // Set device for future reference
    netinfo.device = device;
    return true;
}

/* Receive and optionally send messages to and from network, updating netinfo
 * with any received messages and by removing any closed connections
 *
 * If outgoing message is empty string, will not send anything
 * Specify timeout as time to wait for remote response in ms, default 0.1s
 * If timeout is set to be negative, will wait forever
 *
 * Return true if incoming message read [and outgoing message sent]
 * Return true and set incoming_message to empty string if timed out
 * Return false if error or connection closed
 * Return false if could not properly set up network */
bool update_network(Network_info &netinfo, std::string outgoing_message,
        int timeout)
{
    // Make sure network is properly set up
    if (!(setup_network(netinfo, netinfo.device))) {
        return false;
    }
    // Determine which connections are ready to read and write
    if (!poll_connections(netinfo, timeout)) {
        return false;
    }
    // Receive messages from all connections
    int rv;
    bool no_errors = true;
    for (std::vector<Connection>::iterator iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ++iter) {
        // Read from connection only if ready to receive, otherwise skip
        if (iter->recv_status != MSG_READY) {
            continue;
        }
        // Attempt to receive the message
        if ((rv = receive_message(iter->socket, iter->message)) < 0) {
            no_errors = false;
            if (rv == -2) {
                iter->recv_status = MSG_CLOSED;
            } else {
                iter->recv_status = MSG_ERROR;
            }
        } else {
            // Successfully received the message
            iter->recv_status = MSG_DONE;
        }
    }
    // Send messages to connections as specified by device
    switch (netinfo.device) {
        // Pi or GUI: send message (data or settings) to server
        case PI:
        case GUI:
        {
            // Skip sending if no outgoing message
            if (outgoing_message.empty()) {
                break;
            }
            // Send the message to the server
            std::vector<Connection>::iterator iter;
            for (iter = netinfo.connections.begin();
                    iter != netinfo.connections.end(); ++iter) {
                // Attempt to send to the server
                if (iter->device == SERVER) {
                    // Send only if server is ready
                    if (iter->send_status == MSG_READY) {
                        // Attempt to send the message
                        if ((rv = send_message(iter->socket,
                                        outgoing_message)) < 0) {
                            no_errors = false;
                            iter->send_status = MSG_ERROR;
                        } else {
                            // Successfully sent the message
                            iter->send_status = MSG_DONE;
                        }
                    }
                }
            }
            break;
        }
        // Server: send data to GUI and settings to Pi
        case SERVER:
        {
            std::vector<Connection>::iterator iter_pi, iter_gui;
            for (iter_pi = netinfo.connections.begin();
                    iter_pi != netinfo.connections.end(); ++iter_pi) {
                if (iter_pi->device == PI) {
                    for (iter_gui = netinfo.connections.begin();
                            iter_gui != netinfo.connections.end();
                            ++iter_gui) {
                        if (iter_gui->device == GUI) {
                            // Send settings from Pi to GUI only if both ready
                            if ((iter_pi->recv_status == MSG_DONE) &&
                                    (iter_gui->send_status == MSG_READY)) {
                                // Attempt to send the message
                                if ((rv = send_message(iter_gui->socket,
                                                iter_pi->message)) < 0) {
                                    no_errors = false;
                                    iter_gui->send_status = MSG_ERROR;
                                } else {
                                    // Successfully sent the message
                                    iter_gui->send_status = MSG_DONE;
                                }
                            }
                            // Send data from GUI to Pi if both ready
                            if ((iter_gui->recv_status == MSG_DONE) &&
                                    (iter_pi->send_status == MSG_READY)) {
                                // Attempt to send the message
                                if ((rv = send_message(iter_pi->socket,
                                                iter_gui->message)) < 0) {
                                    no_errors = false;
                                    iter_pi->send_status = MSG_ERROR;
                                } else {
                                    // Successfully sent the message
                                    iter_pi->send_status = MSG_DONE;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    // Remove any connections that produced errors
    if (!clean_connections(netinfo)) {
        return false;
    }

    return no_errors;
}

/* Close all connections and listeners in network, updating netinfo to reflect
 * changes
 * Return true on success, false if error */
bool shutdown_network(Network_info &netinfo)
{
    bool no_errors = true;
    // Close connections
    for (std::vector<Connection>::iterator iter = netinfo.connections.begin();
            iter != netinfo.connections.end(); ) {
        if (close_socket(iter->socket)) {
            iter = netinfo.connections.erase(iter);
        } else {
            no_errors = false;
            ++iter;
        }
    }
    // Close listeners if any
    if (netinfo.pi_listener != INVALID_SOCKET) {
        if (!close_socket(netinfo.pi_listener)) {
            no_errors = false;
        }
    }
    if (netinfo.gui_listener != INVALID_SOCKET) {
        if (!close_socket(netinfo.gui_listener)) {
            no_errors = false;
        }
    }
    return no_errors;
}
