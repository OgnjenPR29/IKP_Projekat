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

typedef struct {
    SOCKET socket;
    int port;
    char ip[INET_ADDRSTRLEN];
} ThreadArgs;

struct client clients[MAX_CLIENTS];
int num_clients = 0;

struct message {

    bool direktna;
    char ime[20];
    char tekst[250];

};

DWORD WINAPI clientHandler(LPVOID lpParam)
{
    // Ovde se obrađuju zahtevi od klijenta

    int flag = 0;

    struct client klijent;
    struct message poruka;
    char recvbuf[20];
    
    ThreadArgs* argumenti = (ThreadArgs*)lpParam;

    SOCKET clientSocket = argumenti->socket;
    char* ip = argumenti->ip;
    int port = argumenti->port;
    char p[5];
    itoa(port, p, 10);

    if (flag == 0) {

        int iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);

        if (iResult > 0)
        {
            printf("Message received from client: %s.\n", recvbuf);
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
        }

        flag++;
        memcpy(klijent.ime, recvbuf, 20);
        klijent.socket = clientSocket;
        memcpy(klijent.ip, ip, INET_ADDRSTRLEN);
        klijent.port = port;
        
        clients[num_clients++] = klijent;

    }
    else {
        while (1) {
            int iResult = recv(clientSocket, (char*)&poruka, sizeof(struct message), 0);
            char odgovor[25];

            if (iResult > 0)
            {
                printf("Message received from client: %s.\n", recvbuf);
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
            }

            if (poruka.direktna) {
                //kod koji salje soket
                size_t arrayLength = sizeof(clients) / sizeof(clients[0]);
                    for(int i = 0; i < arrayLength; i++) {
                        if (clients[i].ime == poruka.ime) {
                            strcpy(odgovor, clients[i].ip);
                            strcpy(odgovor, "   ");
                            strcpy(odgovor, p);
                        
                        }
                    }
            }
            else {
                //kod koji prosledjuje poruku
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

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
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

    // Set listenSocket in listening mode
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
        // Prihvatanje konekcije od klijenta
        //addrLen = sizeof(clientAddr);

        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);

        struct sockaddr_in localAddress;
        addressLength = sizeof(localAddress);
        getsockname(clientSocket, (struct sockaddr*)&localAddress, &addressLength);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddress.sin_addr, ip, INET_ADDRSTRLEN);
        int port = ntohs(localAddress.sin_port);

        clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &addressLength);

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

   int iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(listenSocket);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
	// Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
	return true;
}
