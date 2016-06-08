// Functions for implementing network communication

#ifndef SC_NETWORK
#define SC_NETWORK

int open_connection(int *new_fd, char *hostname=NULL);
void close_connection(int sockfd);
int send_message(int sockfd, char *message, int message_length);
int receive_message(int sockfd, char *buffer, int max_length);

#endif
