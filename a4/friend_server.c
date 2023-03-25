#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "friends.h"

#ifndef PORT
  #define PORT 3000
#endif

#define MAX_CONNECTIONS 12
#define MAX_BACKLOG 5
#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM "\r\n"

struct sockname {
    int sock_fd;
    char *username;
};

int no_connections = 0;

/*
 * Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, struct sockname *users) {
    int user_index = 0;
    while (user_index < MAX_CONNECTIONS && users[user_index].sock_fd != -1) {
        user_index++;
    }

    if (user_index == MAX_CONNECTIONS) {
        fprintf(stderr, "server: max concurrent connections\n");
        return -1;
    }

    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    users[user_index].sock_fd = client_fd;
    users[user_index].username = NULL;
    return client_fd;
}

int read_from(int client_index, struct sockname *users) {
    int fd = users[client_index].sock_fd;
    char buf[MAX_NAME];

    int num_read = read(fd, &buf, MAX_NAME - 1); //31 character read at most
    buf[num_read] = '\0';
    if (users[client_index].username == NULL && num_read != 0){
        //need to read in a username:
        users[client_index].username = malloc(num_read + 1);
        strncpy(users[client_index].username, buf, num_read+1); //copies in the null terminator too so alls good
    }
}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found. The return value is the index into buf
 * where the current line ends.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
    char * end_ptr = strstr(buf, "\r\n");
    if(end_ptr){
        if ((end_ptr - buf) < n){
            return (end_ptr - buf) + 2;
        }
    }
    return -1;
}


int main(int argc, char* argv[]) {

    struct sockname *users = NULL;//init user array

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Set information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    //ability to reuse port right away:
    int on = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                            (const char *) &on, sizeof(on));
    if (status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    //setting padding
    memset(&server.sin_zero, 0, 8);

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    // The client accept - message accept loop. First, we prepare to listen to multiple
    // file descriptors by initializing a set of file descriptors.
    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

     while(1){

        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, users);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection\n");
        }

        // ... otherwise, it must be a client socket. Read from it.
        for (int index = 0; index < no_connections; index++){
            if (users[index].sock_fd > -1 && FD_ISSET(users[index].sock_fd, &listen_fds)) {
                //valid user read request
                // Note: never reduces max_fd
                int client_closed = read_from(index, users);
                if (client_closed > 0) {
                    FD_CLR(client_closed, &all_fds);
                } //remove clsoe fd from listening set
            }
        }
     }

    // Should never get here.
    return 1;
    
}