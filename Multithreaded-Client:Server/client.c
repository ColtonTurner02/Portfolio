//
// Created by Colton Turner on 8/28/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_BUFFER 1024
static int socketFd;
static char *address = "127.0.0.1";
long port = 9999;

_Noreturn void chatloop(char *name);
void buildMessage(char *result, char *name, char *msg);
void setupAndConnect(struct sockaddr_in *serverAddr, int socketFd, long port);
void setNonBlock(int fd);
void interruptHandler(int sig);


int main(int argc, char *argv[])
{
    char *name;
    struct sockaddr_in serverAddr;

    if(argc != 2)
    {
        fprintf(stderr, "./client [username]\n");
        exit(1);
    }

    name = argv[1];
    if((socketFd = socket(AF_INET, SOCK_STREAM, 0))== -1)
    {
        fprintf(stderr, "Couldn't create socket\n");
        exit(1);
    }

    setupAndConnect(&serverAddr,socketFd, port);
    setNonBlock(socketFd);
    setNonBlock(0);

    //Set a handler for the interrupt signal
    signal(SIGINT, interruptHandler);

    chatloop(name);
}

//Main loop to take in chat input and display output
_Noreturn void chatloop(char *name)
{
    fd_set clientFds;
    char chatMsg[MAX_BUFFER];
    char chatBuffer[MAX_BUFFER], msgBuffer[MAX_BUFFER];

    while(1)
    {
        //Reset the fd set each time since select() modifies it
        FD_ZERO(&clientFds);
        FD_SET(socketFd, &clientFds);
        FD_SET(0, &clientFds);
        if(select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) //wait for an available fd
        {
            for(int fd = 0; fd < FD_SETSIZE; fd++)
            {
                if(FD_ISSET(fd, &clientFds))
                {
                    if(fd == socketFd) //receive data from server
                    {
                        int numBytesRead = read(socketFd, msgBuffer, MAX_BUFFER - 1);
                        msgBuffer[numBytesRead] = '\0';
                        printf("%s", msgBuffer);
                        memset(&msgBuffer, 0, sizeof(msgBuffer));
                    }
                    else if(fd == 0) //read from keyboard (stdin) and send to server
                    {
                        fgets(chatBuffer, MAX_BUFFER - 1, stdin);
                        if(strcmp(chatBuffer, "/exit\n") == 0)
                            interruptHandler(-1); //Reuse the interruptHandler function to disconnect the client
                        else
                        {
                            buildMessage(chatMsg, name, chatBuffer);
                            if(write(socketFd, chatMsg, MAX_BUFFER - 1) == -1) perror("write failed: ");
                            //printf("%s", chatMsg);
                            memset(&chatBuffer, 0, sizeof(chatBuffer));
                        }
                    }
                }
            }
        }
    }
}

//Concatenates the name with the message and puts it into result
void buildMessage(char *result, char *name, char *msg)
{
    memset(result, 0, MAX_BUFFER);
    strcpy(result, name);
    strcat(result, ": ");
    strcat(result, msg);
}

//Sets up the socket and connects
void setupAndConnect(struct sockaddr_in *serverAddr, int socketFd, long port)
{
    memset(serverAddr, 0, sizeof(*serverAddr));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = inet_addr(address);
    serverAddr->sin_port = htons(port);
    if(connect(socketFd, (struct sockaddr *) serverAddr, sizeof(struct sockaddr)) < 0)
    {
        perror("Couldn't connect to server");
        exit(1);
    }
}

//Sets the fd to nonblocking
void setNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        perror("fcntl failed");

    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

//Notify the server when the client exits by sending "/exit"
void interruptHandler(int sig_unused)
{
    if(write(socketFd, "/exit\n", MAX_BUFFER - 1) == -1)
        perror("write failed: ");

    close(socketFd);
    exit(1);
}