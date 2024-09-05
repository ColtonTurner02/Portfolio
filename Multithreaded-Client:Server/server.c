//
// Created by Colton Turner on 8/30/24.
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdbool.h>


#define MAX_BUFFER 1024

/*Global address & port*/
char *ADDRESS = "127.0.0.1";
long PORT = 9999
;
/*Struct Definitions*/

//Queue
typedef struct {
    char *buffer[MAX_BUFFER];
    int head, tail;
    int full, empty;
    pthread_mutex_t *mtx;
    pthread_cond_t *notFull, *notEmpty;
}queue;

//Data variables
typedef struct{
    fd_set serverReadFd;
    int socketFd;
    queue *q;
    int clientSockets[MAX_BUFFER];
    int numClients;
    pthread_mutex_t *clientMtx;
}dataVars;

typedef struct{
    dataVars *data;
    int clientSocketFd;
}clientHandlerVars;

/*Chat function declarations*/
void start(int socketFd);
void bindSocket(int socketFd, struct sockaddr_in *server_addr, long port);
void removeClient(dataVars *clientData, int clientSockFd);

/*Thread Handlers Declarations*/
_Noreturn void *newClientHandler(void *data);
void *connClientHandler(void *data);
_Noreturn void *messageHandler(void *data);

/*Queue function declarations*/
queue* initQueue(void);
void destroyQueue(queue *q);
void enqueue(queue *q, char *msg);
char *dequeue(queue *q);

int main(){
    struct sockaddr_in serverAddr;
    int serverSock = socket(AF_INET,SOCK_STREAM,0);
    if(serverSock < 0 ){
        perror("[-] Error creating server socket\n");
        exit(1);
    }
    printf("[+] TCP Socket Created\n");
    bindSocket(serverSock,&serverAddr,PORT);
    if(listen(serverSock,1) < 0){
        perror("[-] Listen Error in start()\n");
        exit(1);
    }
    printf("Listening...\n");
    start(serverSock);
    close(serverSock);
}
/*Begin client handler thread & message handler thread */
void start(int socketFd){
    dataVars data;
    data.numClients = 0;
    data.socketFd = socketFd;
    data.q = initQueue();
    data.clientMtx = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(data.clientMtx,NULL);

    //New Connection Handler Thread Creation
    pthread_t connectionThread;
    if (pthread_create(&connectionThread, NULL, newClientHandler, &data) != 0) {
        perror("[-] New Client Thread Create Error\n");
    }
    printf("[+] Connection thread handler created\n");
    FD_ZERO(&(data.serverReadFd));
    FD_SET(socketFd, &(data.serverReadFd));

    //Message Handler Thread Creation
    pthread_t messageThread;
    if (pthread_create(&messageThread, NULL, messageHandler, &data) != 0) {
        perror("[-] Message Handler Thread Create Error");
    }
    //Cleanup
    pthread_join(connectionThread, NULL);
    pthread_join(messageThread, NULL);


    destroyQueue(data.q);
    pthread_mutex_destroy(data.clientMtx);
    free(data.clientMtx);
}


/*Queue Initialization*/
queue *initQueue(void){
    queue *q = malloc(sizeof(queue));
    if(q==NULL){
        perror("[-] No Space for Queue Init Allocation\n");
        exit(1);
    }

    q->empty = 1;
    q->full = q->head = q->tail = 0;

    //Queue mtx init
    q->mtx = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    if(q->mtx == NULL){
        perror("[-] No Space for Queue mutex allocation\n");
        exit(1);
    }
    pthread_mutex_init(q->mtx,NULL);

    //Queue cond vars init
    q->notFull = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    if(q->notFull == NULL){
        perror("[-] No space for Queue Conditional Variable Full allocation\n");
        exit(1);
    }
    pthread_cond_init(q->notFull,NULL);
    q->notEmpty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    if(q->notEmpty == NULL){
        perror("[-] No space for Queue Conditional Variable Emtpy allocation\n");
        exit(1);
    }
    pthread_cond_init(q->notEmpty,NULL);

    return q;

}
/*Queue Functions*/
void destroyQueue(queue *q){
    pthread_mutex_destroy(q->mtx);
    pthread_cond_destroy(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->mtx);
    free(q->notFull);
    free(q->notEmpty);
    free(q);
}
void enqueue(queue *q, char *msg){
    q->buffer[q->tail] = msg;
    q->tail++;
    if(q->tail == MAX_BUFFER){
        q->tail =0;
    }
    //Queue full
    if(q->tail == q->head){
        q->full = 1;
    }
    q->empty = 0;
}
char *dequeue(queue *q){
    char *msg = q->buffer[q->head];
    q->head++;
    if(q->head == MAX_BUFFER){
        q->head = 0;
    }
    //Queue empty
    if(q->head == q->tail){
        q->empty = 1;
    }
    q->full = 0;

    return msg;
}
/*Server Operation functions*/
void bindSocket(int socketFd, struct sockaddr_in *server_addr,long port){
    //struct sockaddr_in addr = *server_addr;
    //zero out struct
    memset(server_addr,0, sizeof(*server_addr));
    /*Fill out serveraddr_in structure*/
    server_addr->sin_family= AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(ADDRESS);
    server_addr->sin_port = htons(port);

    /*Bind Address and Port to serverSock*/
    if((bind(socketFd, (struct sockaddr*)server_addr, sizeof(struct sockaddr_in))) < 0){
        perror("[-] Bind Error\n");
        exit(1);
    }
    printf("[+] Bind Done\n");
}
void removeClient(dataVars *clientData, int clientSockFd){
    //lock
    pthread_mutex_lock(clientData->clientMtx);
    //remove clientSocketFd from list in dataVars
    for(int i =0; i<MAX_BUFFER;i++)
    {
        if(clientData->clientSockets[i] == clientSockFd)
        {
            clientData->clientSockets[i] = 0;
            close(clientSockFd);
            clientData->numClients--;
            i = MAX_BUFFER; //No need to iterate further once Fd is found ... why not break;?
        }
    }
    //unlock
    pthread_mutex_unlock(clientData->clientMtx);
}
/*Thread Handlers */
_Noreturn void *newClientHandler(void *data) {
    //Cast Void data to dataVars *
    dataVars *newData = (dataVars *) data;
    //Loop for connections
    while(1)
    {
        int clientSocketFd = accept(newData->socketFd, NULL,NULL);
        if (clientSocketFd > 0)
        {
            fprintf(stderr, "New Client Connected : Client%d\n", clientSocketFd);
            //Add new client to clientData->clientlist
            //lock
            pthread_mutex_lock(newData->clientMtx);
            if (newData->numClients < MAX_BUFFER)
            {
                newData->clientSockets[newData->numClients] = clientSocketFd;
                FD_ISSET(newData->clientSockets[newData->numClients],&newData->serverReadFd);
//                for (int i = 0; i < MAX_BUFFER; i++)
//                {
//                    //Check if FD is set
//                    if (!FD_ISSET(clientSocketFd, &newData->serverReadFd))
//                    {
//                        newData->clientSockets[i] = clientSocketFd;
//                        i = MAX_BUFFER; //Stop for loop
//                    }
//                }

                FD_SET(clientSocketFd, &(newData->serverReadFd));

                clientHandlerVars clientData;
                clientData.clientSocketFd = clientSocketFd;
                clientData.data = newData;
                /*Authentication Function!!!
                 * Needs clientData
                 * While(1)
                 * send -> "Enter User: \n Pass:"
                 * recv -> str(user) str(pass)
                 * SQL -> check if user,pass in db
                 * if(true) -> move on to communicate
                 * if(false) -> try again?
                 * */



                //Create Client Thread
                pthread_t clientThread;
                if ((pthread_create(&clientThread, NULL, connClientHandler, &clientData)) < 0) {
                    perror("[-] New Connected Client -- Client Thread Create Error\n");
                    close(clientSocketFd);
                } else {
                    fprintf(stderr,"Client has joined. Socket: %d\n",clientSocketFd);
                    newData->numClients++;
                }
            }
            //Unlock
            pthread_mutex_unlock(newData->clientMtx);
        }
    }

}
void *connClientHandler(void *data){
    clientHandlerVars *clientVars = (clientHandlerVars *) data;
    dataVars *chatData = (dataVars *) clientVars->data;

    queue *q = chatData->q;
    int clientSocketFd = clientVars->clientSocketFd;

    char msg[MAX_BUFFER];
    while(1){
        int rcvBytes = read(clientSocketFd,msg,MAX_BUFFER -1);
        msg[rcvBytes] = '\0';
        if(strcmp(msg,"/exit\n") ==0){
            fprintf(stderr,"Client has disconnected %d\n",clientSocketFd);
            removeClient(chatData,clientSocketFd);
            return NULL;
        }
        else{
            //Wait if queue is full
            while(q->full){
                pthread_cond_wait(q->notFull,q->mtx);
            }
            //Get Lock and push msg to queue, set cond variables
            pthread_mutex_lock(q->mtx);
            fprintf(stderr, "Pushing message to queue: %s\n", msg);
            enqueue(q,msg);
            pthread_mutex_unlock(q->mtx);
            //Let threads know queue is not empty | signals messageHandler thread in while(q->empty)
            pthread_cond_signal(q->notEmpty);

        }
    }
}

_Noreturn void *messageHandler(void *data) {
    dataVars *chatData = (dataVars *) data;
    queue *q = chatData->q;
    int *clientSockets = chatData->clientSockets;
    //Loop
    while (1) {
        //Lock for queue operations
        pthread_mutex_lock(q->mtx);
        //Wait while queue is empty
        while (q->empty) {
            pthread_cond_wait(q->notEmpty, q->mtx);
        }
        char *msg = dequeue(q);
        //Unlock
        pthread_mutex_unlock(q->mtx);

        //Let threads know that queue is not full | | signals connClientHandler thread in while(q->notEmpty)
        pthread_cond_signal(q->notFull);

        //Broadcast message out stdout
        fprintf(stderr, "Broadcasted Message: %s\n", msg);
        //Loop through all connected clients to broadcast
        for (int i = 0; i < chatData->numClients; i++) {
            int clientSocket = clientSockets[i];
            if (clientSocket != 0 && write(clientSocket, msg, MAX_BUFFER - 1) < 0) {
                perror("[-] MessageHandler -- Socket Send Error\n");
            }
        }
    }

}

bool authenticate(clientHandlerVars *clientData){


}
