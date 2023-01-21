#pragma once
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1000

bool InitializeWindowsSockets();
int NonBlockingSocket(SOCKET socket, long seconds, long milliseconds);


struct client {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
    char ime[20];

};


struct ConnectMessage {
    int listenPort;
    char clientName[20];
};

typedef struct {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
} ThreadArgs;


struct message {

    bool direktna;
    char ime[20];
    char tekst[250];

};
