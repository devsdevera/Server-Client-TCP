/**
 * Skeleton file for server.c
 *
 * You are free to modify this file to implement the server specifications
 * as detailed in Assignment 3 handout.
 *
 * As a matter of good programming habit, you should break up your imple-
 * mentation into functions. All these functions should contained in this
 * file as you are only allowed to submit this file.
 */

// Include necessary header files
#include <stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include <stdbool.h> // for bool type

/**
 * The main function should be able to accept a command-line argument
 * argv[0]: program name
 * argv[1]: port number
 *
 * Read the assignment handout for more details about the server program
 * design specifications.
 */

// FUNCTION TO THROW ERROR
void error(const char * msg){
    printf("Error: %s\n", msg);
    exit(1);
}

// FUNCTION TO TRANSMIT ANY MESSAGE TO CLIENT
void transmit(int cfd, char * buffer, char * line){
    memset(buffer, 0, 100);                             // all values to be initialized to 0
    strncpy(buffer, line, strlen(line));                // writes to client immediately

    int s = send(cfd, buffer, 100, 0);                  // int s = write(client, buffer, 100);
    if(s < 0) error("Error: writing to client socket");
}

// FUNCTION TO CHECK FOR ERROR
void check(int fd, const char * msg){
    if (fd < 0) {
        error(msg);
    }
}

// FUNCTION TO CHECK IF A STRING CONTAINS A SPACE
bool contains_space(char *str) {
  while (*str != '\0') { // iterate until the end of the string
    if (*str == ' ') { // check if current character is space
      return true; // return true if space is found
    }
    str++; // move to the next character
  }
  return false; // space not found, return false
}

// FUNCTION TO CHECK IF A STRING IS JUST ""
bool is_empty(char *str) {
  return (*str == '\0');
}

// FUNCTION FOR READING AND WRITING WITH CLIENT
void loop(int cfd, char * command, char * bye, int terminate, char * get, char * put);

int main(int argc, char *argv[])
{
    if(argc != 2){
        return -1;  // argument not specified or not one argument.
    }
    if (atoi(argv[1]) < 1024){  // if the port number is less than 1024
        return -1;              // atoi is a function turning argument to integer
    }

    // CREATING TCP SOCKET

    int fd = socket(AF_INET, SOCK_STREAM, 0);    // family, type of socket, usually 0
    check(fd, "Creating Socket");

    // BINDING THE SOCKET

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;              // htons automatically converts big endian and small endian
    addr.sin_port = htons(atoi(argv[1]));   // Any port greater than 1024
    addr.sin_addr.s_addr = INADDR_ANY;      // double structure call. cascading. any ports.

    int bd = bind(fd, (struct sockaddr *) &addr, sizeof(addr));     // type cast the address, generic address type.
    check(bd, "Binding Socket");

    // START LISTENING

    int ln = listen(fd, SOMAXCONN);         // a listen system call taking two arguments
    check(ln, "Listening Connections");     // give the maximum, socket max connection supported by OS.

    char bye[] = "BYE";
    char get[] = "GET";    // the command must have a space after if it. Argument begins after space.
    char put[] = "PUT";    // this way it prevents getysample1.txt which is invalid.
    char command[5];        // 5 to include /0

    printf("Starting server...\n");     // In accordance to testing video. Server successfully made.
    int terminate = 1;
    while(terminate == 1){

        struct sockaddr_in client;                                                      // this time it holds a client address.
        int addrlen = sizeof(client);
        int cfd = accept(fd, (struct sockaddr *) &client, (socklen_t *) &addrlen);      // client file descriptor

        if(cfd < 0) {
            printf("Error: Accepting Client");      // would not exit, as other clients may work
        }
        loop(cfd, command, bye, terminate, get, put);
    }
    return 0;
}


// NEEDED TWICE FOR FORK CHILD AND FORK UNSUCCESSFUL
void loop(int cfd, char * command, char * bye, int terminate, char * get, char * put){
    char buffer[100];
    transmit(cfd, buffer, "HELLO\n");
    int end = 1;
    while(end == 1){

        // START READING, CONTINUE UNTIL BYE

        memset(buffer, 0, 100);
        int r = read(cfd, buffer, 100);

        if(r < 0){
            printf("Error: Reading Client");    // don't exit
            //transmit(cfd, buffer, "SERVER 500 Get Error\n");
        }

        printf("Received message: %s", buffer);     // server will record what the client is sending through
        sscanf(buffer, "%s", command);         // make command be the first 4 characters of buffer (luckily all commands are 3 letters)


        //if(strcmp(buffer, "BYE\n") == 0){
        if(strcasecmp(command, bye) == 0){          // bye will not allow for any spaces just bye.
            char * argument = buffer + 3;
            argument[strcspn(argument, "\n")] = '\0';   //new_str[strlen(new_str)-1] = '\0';

            if(is_empty(argument)){                     // bye should remain by itself, no arguments
                close(cfd);
                end = 0;                                // end inner while loop, close client
                printf("Closing connection with client\n");
            }else{
                transmit(cfd, buffer, "SERVER 502 Command Error\n");    // if it finds an argument, do not accept
            }
        }
        else if(strcasecmp(command, get) == 0){     //if(strncmp(buffer, get, strlen(get)) == 0){

            char * argument = buffer + 4;
            argument[strcspn(argument, "\n")] = '\0';   //new_str[strlen(new_str)-1] = '\0';

            if(contains_space(argument)){
                transmit(cfd, buffer, "SERVER 500 Get Error\n");        // if the argument contains a space, deny
            }else if(is_empty(argument)){
                transmit(cfd, buffer, "SERVER 500 Get Error\n");        // if the argument is empty, deny
            }else{
                FILE * in = fopen(argument, "r");
                if (in == NULL) {
                    //printf("Error: Could not open %s file.\n", argument);
                    transmit(cfd, buffer, "SERVER 404 Not Found\n");

                } else {
                    transmit(cfd, buffer, "SERVER 200 OK\n");   // transmit server 200 ok message
                    transmit(cfd, buffer, "\n");
                    char line[256];
                    while (fgets(line, sizeof(line), in) != NULL) {
                        transmit(cfd, buffer, line);            // for each line, transmit the line.
                    }
                    transmit(cfd, buffer, "\n");
                    transmit(cfd, buffer, "\n");                // transmit numerous \n as per specs
                    transmit(cfd, buffer, "\n");
                    fclose(in);
                }
            }
        }
        else if(strcasecmp(command, put) == 0){

            char * argument = buffer + 4;               // remove first four characters, and remove \n
            argument[strcspn(argument, "\n")] = '\0';   // new_str[strlen(new_str)-1] = '\0';

            if(contains_space(argument)){
                transmit(cfd, buffer, "SERVER 501 Put Error\n");        // if the argument contains a space, deny
            }else if(is_empty(argument)){
                transmit(cfd, buffer, "SERVER 501 Put Error\n");        // if the argument is empty, deny
            }else{
                FILE * out = fopen(argument, "w");
                if (out == NULL) {
                    printf("Error: Could not open %s file.\n", argument);
                    transmit(cfd, buffer, "SERVER 501 Put Error\n");

                } else {
                    int lock = 0;                           // lock solution, only after two \n end while loop
                    while (lock < 2) {
                        memset(buffer, 0, 100);
                        int r = read(cfd, buffer, 100);     // put read line into buffer
                        if(r < 0){
                            error("Error: reading from client socket");
                            transmit(cfd, buffer, "SERVER 501 Put Error\n");
                        }
                        fprintf(out, "%s", buffer);         // write to the outfile what is in buffer
                        if(strcmp(buffer, "\n") == 0){
                            lock++;
                        }else{                              // important, if not \n always reset back to 0
                            lock = 0;
                        }
                    }
                    fclose(out);                            // close the out file and transmit success message
                    transmit(cfd, buffer, "SERVER 201 CREATED\n");
                }
            }

        }else{
            transmit(cfd, buffer, "SERVER 502 Command Error\n");    // no command was found, transmit message
        }
    }
}


