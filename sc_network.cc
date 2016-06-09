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

#include <string>

#define PORT "3141" // the port users will be connecting on

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 1000 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int open_connection(int &new_connection, char *hostname)
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
    
    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
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

void close_connection(int connection)
{
    close(connection);
}

int send_message(int connection, std::string message)
{
    if (send(connection, message.c_str(), message.length(), 0) == -1) {
        perror("send");
        return false;
    }
    return true;
}

int receive_message(int connection, std::string &message)
{
    int numbytes;
    char buffer[MAXDATASIZE];

    if ((numbytes = recv(connection, buffer, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        return false;
    }

    if (numbytes == 0)
        return false;

    message.assign(buffer, numbytes);

    return true;
}
