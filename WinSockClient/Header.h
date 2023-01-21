#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Hash.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016


struct message {

    bool direktna;
    char ime[20];
    char tekst[250];

};

typedef struct {
    int listenPort;
    char clientName[20];
}ConnectMessage;

struct zaKonekciju {

    char ip[INET_ADDRSTRLEN];
    int port;
    char ime[20];

};

bool InitializeWindowsSockets();
int NonBlockingSocket(SOCKET socket, long seconds, long milliseconds);

