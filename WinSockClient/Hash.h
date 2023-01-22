#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>



#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define SIZE 2000


typedef struct {
    SOCKET soket;
    char ime[20];
}klijent;

int hashCode(char* key);
klijent* search(char* key,klijent* hashArray[]);
void insert(klijent* item, klijent* hashArray[]);
void clientDelete(klijent* hashArray[], SOCKET s, char clientName[]);
void display(klijent* hashArray[]);