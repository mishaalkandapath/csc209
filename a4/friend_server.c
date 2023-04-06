#include "friends.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef PORT
  #define PORT 3000
#endif

#define _GNU_SOURCE
#define MAX_CONNECTIONS 12
#define MAX_BACKLOG 5
#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \r\n"


//Defining the struct for the linked list of users that are connected
struct sockname {
    int sock_fd;
    char *username;
    struct sockname *next;
    struct sockname *previous;
    char *last_command;
    int inbuf;
    char *after;
};

//function headers for everything
int error(char *msg, int fd);

int tokenize(char *cmd, char **cmd_argv, int fd);

int find_network_newline(const char *buf, int n);

void close_connection(struct sockname * user);

void syscall_errors(char *msg, int return_val);

void send_post(char *author, char *target, char *msg, struct sockname *users);

int process_args(int cmd_argc, char **cmd_argv, struct sockname * user, User **user_list_ptr, struct sockname *users);

int accept_connection(int fd, struct sockname *users);

int read_from(struct sockname *user, User **user_list_ptr, struct sockname *users);

int read_username(struct sockname *user, User **user_list_ptr) ;

int handle_partial_reads(int nbytes, struct sockname *user, User **user_list_ptr, struct sockname *users);

int read_from_client(struct sockname *user, User **user_list_ptr, struct sockname *users);

/* 
 * Read and process commands
 * Return:  -1 for quit command, or the client possibly disconnected
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, struct sockname * user, User **user_list_ptr, struct sockname *users) {
    char * username = user -> username; //username of user currently connected and sending command
    User *user_list = *user_list_ptr; //duplicate of linked list of connected users
    int client_fd = user -> sock_fd; //socket file descriptor of user currently connected and sending command
    if (cmd_argc <= 0) { //no command entered
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) { //quit, handled outside on return
        return -1;
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
        char *buf = list_users(user_list); //never empty as there is atleast one user (the one who entered the command)
        int r = write(client_fd, buf, strlen(buf)); //write the output to the client
        syscall_errors("write", r); //check if write failed system-wise
        if (r != strlen(buf)){
            return -1; //client probably disconnected, due to inadequate 
        }
        free(buf); //buf was malloced in list_users with snprintf, so free it.
    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
        char msg[28+MAX_NAME]; // string to hold custom message on successful command
        //27 is the length of the string "You are now friends with \0" + 2 for \r\n
        switch (make_friends(cmd_argv[1], username, user_list)) {
            case 0: //success on making friends
                strncpy(msg, "You are now friends with \0", 28+MAX_NAME); //copy in the null terminator too 
                strncat(msg, cmd_argv[1], MAX_NAME);//null terminated
                strncat(msg, "\r\n", 27+MAX_NAME - strlen(msg));
                return error(msg, client_fd); //write the appropriate message to the client
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
        syscall_errors("malloc", contents == NULL ? -1 : 0);

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(username, user_list);
        User *target = find_user(cmd_argv[1], user_list);
        switch (make_post(author, target, contents)) {
            
            case 0: //success on making post, send the output to the appropriate user
                send_post(author -> name, target -> name, contents, users);
                return 0;
            case 1:
                return error("You can only post to your friends\r\n", client_fd);

            case 2:
                return error("The user you entered does not exist\r\n", client_fd);

        }

    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {

        User *user = find_user(cmd_argv[1], user_list);
        char * buf = print_user(user);
        int r = write(client_fd, buf, strlen(buf));
        syscall_errors("write", r); //check if write failed system-wise
        if (r != strlen(buf)){
            return -1; //client probably disconnected
        }
        printf("%s", buf);
        free(buf);//buf was malloced in print_user with snprintf, so free it.
    } else { //incorrect command usage
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

    int client_fd = accept(fd, NULL, NULL); //get the client file descriptor for communication

    if (client_fd < 0) { //allocating a client fd failed, kill the server
        perror("server: accept");
        close(fd);
        exit(1);
    }

    //ask for username 
    char *enter_string = "What is your user name?\r\n";
    int num_written = write(client_fd, enter_string, strlen(enter_string));
    syscall_errors("write", num_written); //check if write failed system-wise
    if (num_written != strlen("What is your user name?\r\n")) {
        return -1;
    }

    if (users -> sock_fd == -1){ //empty first spot, easily filled in 
        users -> sock_fd = client_fd; //initialize the client fd
        users -> username = NULL; //set the username to NULL for now
        users -> inbuf = 0; //nothing in the buffer
        users -> last_command = malloc(INPUT_BUFFER_SIZE); // setup space for commans
        strncpy(users -> last_command, "\0", INPUT_BUFFER_SIZE);  //set the last command to null
        users -> after = (users -> last_command);//set the after pointer to the last command
        //next and previous pointer is null default by initialization (since this is the head of the linked_list)
    }else{
        while (users -> next){ //keep going until the end fo the list, i,e user -> next == NULL
            users = users -> next;
        }
        users -> next = malloc(sizeof(struct sockname)); //make space for the next user
        users -> next -> previous = users; //set the previous pointer to the current user
        //same procedure as above
        users -> next -> sock_fd = client_fd;
        users -> next -> username = NULL;
        users -> next -> next = NULL;
        users -> next -> inbuf = 0;
        users -> next -> last_command = malloc(INPUT_BUFFER_SIZE);
        strncpy(users -> next -> last_command, "\0", INPUT_BUFFER_SIZE); 
        users -> next -> after = (users -> next -> last_command);
    }
    
    return client_fd;
}

/*
Read and process non-username related commands from the user
return -1 if they quit or possibly disconnected, else return 0
*/

int read_from(struct sockname *user, User **user_list_ptr, struct sockname *users){

    int fd = user -> sock_fd; //get the file descriptor for the user sending command
    char *buf = malloc(INPUT_BUFFER_SIZE); //leave enough space for copying in everything in the user's command buffer
    strncpy(buf, user -> last_command, strlen(user -> last_command)); //copy the users command buffer in 
    buf[strlen(user -> last_command)] = '\0'; //null terminate it just in case
    // process the input
    char *cmd_argv[INPUT_ARG_MAX_NUM];
    int cmd_argc = tokenize(buf, cmd_argv, fd);
    int r = process_args(cmd_argc, cmd_argv, user, user_list_ptr, users);//run adequate commands to make requested changes
    free(buf); //free the intermediate buffer that holds users command buffers

    if (cmd_argc > 0 && r  == -1) { // quit was called by the user
        return -1;
    }

    return r; //return the result of the command

}

/*
    * Read a username from the client. Return 0 on success, -1 on error.
*/
int read_username(struct sockname *user, User **user_list_ptr) {
    int fd = user -> sock_fd; //get the file descriptor for the user sending command

    char *buf = malloc(INPUT_BUFFER_SIZE); //leave enough space for copying in everything in the user's command buffer
    if (strlen(user -> last_command) > 31){ //if the username is too long, truncate it
        error("Username too long, truncated to 31 chars.\r\n", fd); //tell the user that their username was truncated
        strncpy(buf, user -> last_command, 31); //copy in the first 31 chars
        buf[31] = '\0';
    }else{//otherwise just copy in the whole thing
        strncpy(buf, user -> last_command, strlen(user -> last_command));
        buf[strlen(user -> last_command)] = '\0';
    }

    //need to read in a username:
    user -> username = malloc(strlen(buf) + 1); //allocate space for the usernameß
    strncpy(user -> username, buf, strlen(buf) + 1); //copies in the null terminator too so alls good
    (user -> username)[strlen(buf)] = '\0'; //but just in case, null terminate it
    free(buf); //free the intermediate buffer that holds users command buffers

    switch(create_user(user -> username, user_list_ptr)){ //create the user using function from friends.c
        case 1:
            return error("Welcome back.\r\nGo ahead and enter user commands>\r\n", fd);
        default:
            return error("Welcome.\r\nGo ahead and enter user commands>\r\n", fd);
    }

    return 0; // standard return
}

/*
    * Read from a client and process the input. Return -1 if they quit or possibly disconnected, else return 0.
*/

int read_from_client(struct sockname *user, User **user_list_ptr, struct sockname *users){

    //append the next chunk of user commands into the users command buffer
    int nbytes = read(user -> sock_fd, user -> after, INPUT_BUFFER_SIZE - user -> inbuf);
    if (nbytes > 0){ //something was read
        user -> inbuf += nbytes; //update the number of bytes in the buffer
        return handle_partial_reads(nbytes, user, user_list_ptr, users); //handle the command processing, if there is a full command in command buffer
    }else if (nbytes == 0){ //nothing was read, client disconnected
        return -1;
    }else{ //some system read error, disconnect the client
        perror("server: read");
        return -1;
    }
    return -1; //shouldn't get here

}


int main(int argc, char* argv[]) {
    //variable to keep track of all the users connected
    struct sockname *users;
    // Create the head of the empty data structure
    users = malloc(sizeof(struct sockname)); //space for the head of the list
    users -> username = NULL;
    users -> sock_fd = -1;
    users -> next = NULL;
    users -> previous = NULL;

    // yet another linked_list to keep track of all the users and necessary application information
    User *user_list = NULL;

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

        if (select(max_fd + 1, &listen_fds, NULL, NULL, NULL) == -1) { //something went wrong with select, kill the server
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, temp);
            if (client_fd > max_fd) { //update the max fd if necessary
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds); //add the new client to the set of fds to listen to
            printf("Accepted connection %d\n", client_fd);
        }

        // ... otherwise, it must be a client socket. Read from it.
        temp = users; //reset the temp pointer
        while (temp){ //loop through all the users
            if ((temp -> sock_fd) > -1 && FD_ISSET(temp -> sock_fd, &listen_fds)) { //if the user is valid and the fd is in the set
                printf("Client %d %s is saying somrthiung \n", temp -> sock_fd ,temp -> username);
                //valid user command request
                int client_closed = read_from_client(temp, &user_list, users); //process client input
                if (client_closed < 0) { //-1 if the client disconnected
                    FD_CLR(temp -> sock_fd, &all_fds);
                    //remove the user from the list of users
                    close_connection(temp);
                }
            }
            temp = temp -> next; //move to the next user
        }
        

    }
}

//HELPER FUNCTION TO CLEANLY CLOSE A CONNECTION
/*
    * Close the connection to the given user, and free all associated memory.
    * not setting the socket to -1 now, as it needs to be cleared in the main loop
*/
void close_connection(struct sockname *user){
    close((user) -> sock_fd); //close the client file descriptor
    if ((user) -> username){ //if the user has a username, free it
        free((user) -> username);
    }
    free((user) -> last_command); //free the user's command buffer
    (user) -> sock_fd = -1; //set the socket to -1 to signify empty
}

//HELPER FUNCTION TO HANDLE PARTIAL READS
/*
    * This function sends the requested command from client, once the entire command has been received    
    * Read from a client and process the input. Return -1 if they quit or possibly disconnected, else return 0.
*/

int handle_partial_reads(int nbytes, struct sockname *user, User **user_list_ptr, struct sockname *users){

    int latest_ret = 0; //store whether any of the commands have failed indicating possible disconnect
    int where;
    while ((where = find_network_newline(user -> last_command, user -> inbuf)) > 0) { //run until we run out of full commands to process
        (user -> last_command)[where - 2]= '\0'; //null terminate the command, to indicate end of next command to process
        printf("Next message: %s\n", user -> last_command); //NEED TO RUN COMMAND HERE
        if (user -> username == NULL){
            //this is a username operation as there is no username yet
            latest_ret = read_username(user, user_list_ptr);
        }else{ //some other non-username related command
            latest_ret = latest_ret == -1 ? -1 : read_from(user, user_list_ptr, users);
        }
        (user -> inbuf) -= (where); //update the number of bytes in the buffer
        memmove(user -> last_command, user -> last_command + where, user -> inbuf); //move the rest of the buffer to the front
        memset(user -> last_command + user -> inbuf, '\0', INPUT_BUFFER_SIZE - user -> inbuf); //null terminate the rest of the buffer
    }
    user -> after = user -> last_command + user -> inbuf; //update the pointer to the end of the filled buffer
    return latest_ret;
}

//HELPER FUCNTION TO SEND A POST:
/**
 * Send the given message from author to target, if currently connected: 
*/
void send_post(char *author, char *target, char *msg, struct sockname *users){
    //iterate through the list of users (socket-side list)
    struct sockname *current = users; //duplicate the pointer so that we don't change the original
    while (current){ //keep going until the very end = NULL
        //write to every user with a matching user and valid socket
        if (strcmp(current -> username, target) == 0 && current -> sock_fd != -1){
            //compute the length of the message to be sent
            int write_length = strlen("From ") + strlen(author) + strlen(": ") + strlen(msg) + 3;
            char *buf = malloc(write_length);
            int ret = snprintf(buf, write_length, "From %s: %s\r\n", author, msg);
            syscall_errors("malloc", ret); //check if we failed to allocate memory
            int r = write(current -> sock_fd, buf, strlen(buf));  //write to the user
            syscall_errors("write", r); //check if write failed system-wise
            free(buf); //free intermediate message buf
        }
        current = current -> next; //move to the next user
    }
}

// STRING PROCESSING HELPER FUNCTIONS
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

//ERROR HANDLING HELPERS:
/* 
 * Print a formatted error message to the user 
 Return -1 if write fails, 0 otherwise
 */
int error(char *msg, int fd) {
    if (write(fd, msg, strlen(msg)) != strlen(msg)){
        return -1; //client probably disconnected
    }
    return 0;
}

/*
    * Handle almost all system calls, and exit if they fail
*/
void syscall_errors(char *msg, int return_val){
    if (return_val == -1){
        perror(msg);
        exit(1);
    }
}