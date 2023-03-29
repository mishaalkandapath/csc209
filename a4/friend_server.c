#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "friends.c"

#ifndef PORT
  #define PORT 3000
#endif

#define _GNU_SOURCE
#define MAX_CONNECTIONS 12
#define MAX_BACKLOG 5
#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \r\n"

struct sockname {
    int sock_fd;
    char *username;
    struct sockname *next;
    char *last_command;
    int inbuf;
    char *after;
};

//global variable to keep track of all the users connected
struct sockname *users;


/* 
 * Print a formatted error message to stderr.
 */
int error(char *msg, int fd) {
    if (write(fd, msg, strlen(msg)) != strlen(msg)){
        return -1; //client probably disconnected
    }
    return 0;
}

/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv, int fd) {
    int cmd_argc = 0;
    char *next_token = strtok(cmd, DELIM);    
    while (next_token != NULL) {
        if (cmd_argc >= INPUT_ARG_MAX_NUM - 1) {
            error("Too many arguments!\r\n", fd);
            cmd_argc = 0;
            break;
        }
        cmd_argv[cmd_argc] = next_token;
        cmd_argc++;
        next_token = strtok(NULL, DELIM);
    }

    return cmd_argc;
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

/**
 * Send the given message from author to target, if currently connected:
 * 
*/
int send_post(char *author, char *target, char *msg){

    struct sockname *current = users;
    while (current != NULL){
        if (strcmp(current -> username, target) == 0){
            char *buf = malloc(strlen("From ") + strlen(author) + strlen(": ") + strlen(msg) + 3);
            int ret = snprintf(buf, strlen("From ") + strlen(author) + strlen(": ") + strlen(msg) + 3, "From %s: %s\r\n", author, msg);
            if (ret == -1){
                perror("malloc");
                exit(1);
            }
            write(current -> sock_fd, buf, strlen(buf));//not sure how to check disconnection here, as we are in a different client, so will leave this 
            free(buf);
            return 0;
        }
    }
    return 0;
}

/* 
 * Read and process commands
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, char * username, int client_fd) {
    User *user_list = *user_list_ptr;

    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
        return -1;
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
        char *buf = list_users(user_list);
        if (buf == NULL) {
            return error("user not found\r\n", client_fd);
        }else{
            if (write(client_fd, buf, strlen(buf)) != strlen(buf)){
                return -1; //client probably disconnected
            }
            printf("%s", buf);
            free(buf);
        }

    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
        char msg[27+MAX_NAME];
        switch (make_friends(cmd_argv[1], username, user_list)) {
            case 0:
                strncpy(msg, "You are now friends with \0", 27+MAX_NAME); //copy in the null terminator too 
                strncat(msg, cmd_argv[1], MAX_NAME);//null terminated
                strncat(msg, "\r\n", 27+MAX_NAME - strlen(msg));
                return error(msg, client_fd);
            case 1:
                return error("You are already friends\r\n", client_fd);
            case 2:
                return error("At least one of you entered has the max number of friends\r\n", client_fd);
            case 3:
                return error("You can't friend yourself\r\n", client_fd);
            case 4:
                return error("The user you entered does not exist\r\n", client_fd);

        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 3) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 2; i < cmd_argc; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(username, user_list);
        User *target = find_user(cmd_argv[1], user_list);
        switch (make_post(author, target, contents)) {
            case 0: 
            //send the post to the target person. 
            return send_post(author -> name, target -> name, contents);

            case 1:
                return error("You can only post to your friends\r\n", client_fd);

            case 2:
                return error("The user you entered does not exist\r\n", client_fd);

        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {
        User *user = find_user(cmd_argv[1], user_list);
        char * buf = print_user(user);
        if (buf == NULL) {
            return error("User not found\r\n", client_fd);
        }else{
            if (write(client_fd, buf, strlen(buf)) != strlen(buf)){
                return -1; //client probably disconnected
            }
            printf("%s", buf);
            free(buf);
        }
    } else {
        return error("Incorrect syntax\r\n", client_fd);
    }
    return 0;
}

/*
 * Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, struct sockname *users) {

    // if (user_index == MAX_CONNECTIONS) {
    //     fprintf(stderr, "server: max concurrent connections\n");
    //     return -1;
    // }

    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    //ask for username 
    char *enter_string = "What is your user name?\r\n";
    int num_written = write(client_fd, enter_string, strlen(enter_string));
    if (num_written != strlen("What is your user name?\r\n")) {
        return -1;
    }

    if (users -> sock_fd == -1){
        users -> sock_fd = client_fd;
        users -> username = NULL;
        users -> next = NULL;
        users -> inbuf = 0;
        users -> last_command = malloc(INPUT_BUFFER_SIZE);
        strncpy(users -> last_command, "\0", INPUT_BUFFER_SIZE); 
        users -> after = (users -> last_command);
    }else{
        while (users -> next){
            users = users -> next;
        }
        users -> next = malloc(sizeof(struct sockname));
        users -> next -> sock_fd = client_fd;
        users -> next -> username = NULL;
        users -> next -> next = NULL;
        users -> next -> inbuf = 0;
        users -> next -> last_command = malloc(INPUT_BUFFER_SIZE);
        strncpy(users -> next -> last_command, "\0", INPUT_BUFFER_SIZE); 
        users -> next -> after = (users -> next -> last_command);;
    }
    
    return client_fd;
}

int read_from(struct sockname *user, User **user_list_ptr){
    int fd = user -> sock_fd;
    char *buf = malloc(INPUT_BUFFER_SIZE);
    strncpy(buf, user -> last_command, strlen(user -> last_command));
    buf[strlen(user -> last_command)] = '\0';
    // process the input
    char *cmd_argv[INPUT_ARG_MAX_NUM];
    int cmd_argc = tokenize(buf, cmd_argv, fd);
    int r = process_args(cmd_argc, cmd_argv, user_list_ptr, user -> username, fd);
    free(buf);
    if (cmd_argc > 0 && r  == -1) {
        close(fd); // can only reach if quit command was entered
        return -1;
    }
    return r;
}

int read_username(struct sockname *user, User **user_list_ptr) {
    int fd = user -> sock_fd;

    char *buf = malloc(INPUT_BUFFER_SIZE); 
    if (strlen(user -> last_command) > 31){
        error("Username too long, truncated to 31 chars.\r\n", fd);
        strncpy(buf, user -> last_command, 31);
        buf[31] = '\0';
    }else{
        strncpy(buf, user -> last_command, strlen(user -> last_command));
        buf[strlen(user -> last_command)] = '\0';
    }

    //need to read in a username:
    user -> username = malloc(strlen(buf) + 1);
    printf("buf number %ld", strlen(buf) + 1);
    strncpy(user -> username, buf, strlen(buf) + 1); //copies in the null terminator too so alls good
    (user -> username)[strlen(buf)] = '\0';
    free(buf);
    switch(create_user(user -> username, user_list_ptr)){
        case 1:
            return error("Welcome back.\r\nGo ahead and enter user commands>\r\n", fd);
            break;
        case 2:
            return error("username is too long\r\n", fd);
            break;
            default:
            return error("Welcome.\r\nGo ahead and enter user commands>\r\n", fd);
    }
    printf("server: client is now known as %s", user -> username);
    return 100;
}

/**
 * Handle partial reads from clients
*/
int read_from_client(struct sockname *user, User **user_list_ptr){
    int latest_ret = 10; //random vakue
    int nbytes = read(user -> sock_fd, user -> after, INPUT_BUFFER_SIZE - user -> inbuf);
    if (nbytes > 0){
        user -> inbuf += nbytes;
        int where;
        while ((where = find_network_newline(user -> last_command, user -> inbuf)) > 0) {
            (user -> last_command)[where - 2]= '\0';
            printf("in here\n");
            printf("Next message: %s\n", user -> last_command); //NEED TO RUN COMMAND HERE
            if (user -> username == NULL){
                //this is a username operation
                latest_ret = read_username(user, user_list_ptr);
            }else{
                latest_ret = latest_ret == -1 ? -1 : read_from(user, user_list_ptr);
            }
            (user -> inbuf) -= (where);
            memmove(user -> last_command, user -> last_command + where, user -> inbuf);
            for (int i = user -> inbuf; i < INPUT_BUFFER_SIZE; i++){
                (user -> last_command)[i] = '\0';
            }
        }
        printf("moving out of theloop here");
        user -> after = user -> last_command + user -> inbuf;
        return latest_ret;
    }else if (nbytes == 0){
        return -1;
    }else{
        perror("server: read");
        return -1;
    }
    return -1; //should never happen


}


int main(int argc, char* argv[]) {

    // Create the heads of the empty data structure
    User *user_list = NULL;
    users = malloc(sizeof(struct sockname));
    users -> username = NULL;
    users -> sock_fd = -1;
    users -> next = NULL;

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

    while (1) {
        struct sockname *temp = users;
        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, temp);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection %d\n", client_fd);
        }

        // ... otherwise, it must be a client socket. Read from it.
        temp = users;
        while (temp){
            if ((temp -> sock_fd) > -1 && FD_ISSET(temp -> sock_fd, &listen_fds)) {
                printf("Client %d %s is saying somrthiung \n", temp -> sock_fd ,temp -> username);
                //valid user read request
                // Note: never reduces max_fd
                int client_closed = read_from_client(temp, &user_list);
                printf("sdfsdf %d\n", client_closed);
                if (client_closed < 0) {
                    FD_CLR(temp -> sock_fd, &all_fds);
                } //remove clsoe fd from listening set
            }
            temp = temp -> next;
        }
        

    }

    //freeing memory on finish
    while (users){
        //free memory
        if (users -> username){
            free(users -> username);
        }
        free(users);
        users = users -> next;
    }
}