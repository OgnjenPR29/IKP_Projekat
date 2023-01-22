#pragma once
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <vector>
#include <thread>


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1000
#define NAME_SIZE 21
#define TEXT_SIZE 251

bool InitializeWindowsSockets();
int NonBlockingSocket(SOCKET socket, long seconds, long milliseconds);


struct client {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
    char ime[NAME_SIZE];

};

struct ConnectMessage {
    int listenPort;
    char clientName[NAME_SIZE];
};

typedef struct {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
} ThreadArgs;


struct message {

    bool direktna;
    char ime[NAME_SIZE];
    char tekst[TEXT_SIZE];

};
