#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Hash.h"
#include <vector>

#define NAME_SIZE 21
#define TEXT_SIZE 251
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016


struct message {

    bool direktna;
    char ime[NAME_SIZE];
    char tekst[TEXT_SIZE];

};

typedef struct {
    int listenPort;
    char clientName[NAME_SIZE];
}ConnectMessage;

struct zaKonekciju {

    char ip[INET_ADDRSTRLEN];
    int port;
    char ime[NAME_SIZE];

};

bool InitializeWindowsSockets();
int NonBlockingSocket(SOCKET socket, long seconds, long milliseconds);

