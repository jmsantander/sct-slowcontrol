// Functions for implementing slow control network communication

#ifndef SC_NETWORK
#define SC_NETWORK

#include <string>

/* Open a new connection on the socket with file descriptor new_connection.
 * If hostname is not set, default is to open a server and wait for incoming
 * connections.
 * If hostname is set, will connect to that host.
 * Return true on success, false on failure. */
bool open_connection(int &new_connection, char *hostname=NULL);

// Close the connection on socket with file descriptor connection.
void close_connection(int connection);

/* Send a message to socket with file descriptor connection.
 * Return true on success, false on failure. */
bool send_message(int connection, std::string message);

/* Receive a message from socket with file descriptor connection.
 * Return true on success, false on failure. */
bool receive_message(int connection, std::string &message);

#endif
