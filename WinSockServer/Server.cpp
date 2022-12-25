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

struct client {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
    char ime[20];

};

struct client clients[MAX_CLIENTS];
int num_clients = 0;

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

DWORD WINAPI clientHandler(LPVOID lpParam)
{
    // Ovde se obrađuju zahtevi od klijenta

    int flag = 0;
    int send_bytes;

    struct client klijent;
    struct message poruka;
    ThreadArgs* argumenti = (ThreadArgs*)lpParam;

    char rb[1024];
    char p[10];
    char currIme[20];

    SOCKET clientSocket = argumenti->socket;
    char* ip = argumenti->ip;
    int port = argumenti->port;

    while (flag == 0) {

        int iResult = recv(clientSocket, rb, DEFAULT_BUFLEN, 0);

        if (iResult > 0)
        {
            printf("Registrovan client: %s.\n", rb);
        }
        else if (iResult == 0)
        {
            printf("Connection with client closed.\n");
            closesocket(clientSocket);
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
        }

        flag++;

        strcpy(klijent.ime, rb);
        strcpy(currIme, rb);
        klijent.socket = clientSocket;
        memcpy(klijent.ip, ip, INET_ADDRSTRLEN);
        klijent.port = port;

        printf("Ovde je upisan klijent %s %s %d\n", klijent.ime, klijent.ip, klijent.port);

        clients[num_clients++] = klijent;

    }
        while (1) {

            int iResult = recv(clientSocket, (char*)&poruka, sizeof(struct message), 0);
            char odgovor[50];
            memset(odgovor, 0, sizeof(odgovor));

            printf("\nOvo je server primio: %d %s %s\n",poruka.direktna, poruka.ime, poruka.tekst);

            if (iResult > 0)
            {
                printf("Message received from client: %s.\n", rb);
            }
            else if (iResult == 0)
            {
                // connection was closed gracefully
                printf("Connection with client closed.\n");
                closesocket(clientSocket);
            }
            else
            {
                // there was an error during recv
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                return 1;
            }

            if (poruka.direktna) {
                //kod koji salje soket
                size_t arrayLength = sizeof(clients) / sizeof(clients[0]);

                for(int i = 0; i < arrayLength; i++) {

                    if (strcmp(clients[i].ime,poruka.ime) == 0) {

                        itoa(clients[i].port, p, 10);

                        strcat(odgovor, "D ");
                        strcat(odgovor, clients[i].ip);
                        strcat(odgovor, "   ");
                        strcat(odgovor, p);
                           
                        printf("Ovaj odgovor server salje: %s", odgovor);

                        if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                            perror("send");
                            return 1;
                        }
                        memset(odgovor, 0, sizeof(odgovor));

                        break;
                    }
                    if(i == (int)(arrayLength) - 1) {
                        strcpy(odgovor, "Ne postoji korisnik sa takvim imenom");
                        if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                            perror("send");
                            return 1;
                        }
                        memset(odgovor, 0, sizeof(odgovor));

                        break;
                    }
                }
            }
            else {
                //kod koji prosledjuje poruku
                size_t arrayLength = sizeof(clients) / sizeof(clients[0]);

                for (int i = 0; i < arrayLength; i++) {
                 
                    if (strcmp(clients[i].ime, poruka.ime) == 0) {

                        strcat(odgovor, "P Klijent ");
                        strcat(odgovor, currIme);//registrovano ime
                        strcat(odgovor, " salje poruku: ");
                        strcat(odgovor, poruka.tekst);

                        printf("\nOvaj odgovor server salje: %s", odgovor);

                        if ((send_bytes = send(clients[i].socket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                            perror("send");
                            return 1;
                        }
                        memset(odgovor, 0, sizeof(odgovor));

                        break;

                    }
                    if(i == (int)(arrayLength) - 1)
                    {
                        strcpy(odgovor, "Ne postoji taj klijent");
                        if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                            perror("send");
                            return 1;
                        }
                        memset(odgovor, 0, sizeof(odgovor));

                        break;
                    }
                }

            }
        }

    


    return 0;
}


int  main(void) 
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET clientSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];
    
    DWORD threadId;

    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }
    
    // Prepare address information structures
    addrinfo *resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if ( iResult != 0 )
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
                          SOCK_STREAM,  // stream socket
                          IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    iResult = bind( listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    iResult = listen(listenSocket, SOMAXCONN);

    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

	printf("Server initialized, waiting for clients.\n");


    while (1)
    {
        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);

        clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &addressLength);

        //printf("Ovo je soket od klijenta: %u\n",clientSocket);

        struct sockaddr_in localAddress;
        addressLength = sizeof(localAddress);
        getpeername(clientSocket, (struct sockaddr*)&localAddress, &addressLength);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddress.sin_addr, ip, INET_ADDRSTRLEN);
        int port = ntohs(localAddress.sin_port);

        printf("%s %d \n", ip, port);

        ThreadArgs argumenti;
        memcpy(argumenti.ip, ip, INET_ADDRSTRLEN);
        argumenti.port = port;
        argumenti.socket = clientSocket;

        if (clientSocket == INVALID_SOCKET)
        {
            printf("Accept failed.\n");
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        // Kreiranje novog thread-a za obradu zahteva od klijenta
        HANDLE clientThread = CreateThread(NULL, 0, clientHandler, &argumenti, 0, &threadId);
       
        if (clientThread == NULL)
        {
            printf("Failed to create client thread.\n");
            closesocket(clientSocket);
            continue;
        }
    }


   /* do
    {
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.


        acceptedSocket = accept(listenSocket, NULL, NULL);//soket od klijenta koji se trenutno konektovao



        if (acceptedSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

       do
        {
            // Receive data until the client shuts down the connection
            iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
            {
                printf("Message received from client: %s.\n", recvbuf);
            }
            else if (iResult == 0)
            {
                // connection was closed gracefully
                printf("Connection with client closed.\n");
                closesocket(acceptedSocket);
            }
            else
            {
                // there was an error during recv
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocket);
            }
        } while (iResult > 0);

        // here is where server shutdown loguc could be placed

    } while (1);*/

    // shutdown the connection since we're done

    iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocket);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
	return true;
}
