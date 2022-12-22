#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
bool InitializeWindowsSockets();

struct message {
    
    bool direktna;
    char* ime;
    char* tekst;

};

int __cdecl main() 
{
    // socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // message to send
    printf("Unesite ime za registraciju: ");

    char messageToSend[20];
    fgets(messageToSend, 20, stdin);
    
    // Validate the parameters

    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);

    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        getch();
        closesocket(connectSocket);
        WSACleanup();
    }
 
    // Send an prepared message with null terminator included
    iResult = send( connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0 );

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
    //getch();

    while (true) {

        bool direktno;
        char pomocna;
        char str1[20];
        char str2[250];

        while (1) {

            printf("Unesite koji tip komunikacije zelite D-direktno P-preko servera: ");
            scanf(" %c", &pomocna);

            if (pomocna == 'D' || pomocna == 'd') {
                direktno = 1;
                break;
            }
            else if (pomocna == 'P' || pomocna == 'p') {
                direktno = 0;
                break;
            }
            else {
                printf("Niste uneli dobar flag!\n");
            }
        }

        printf("Unesite ime klijenta sa kim zelite da komunicirate: ");
        fgets(str1, 20, stdin);

        if (direktno == 1) {
            strcpy(str2,"Posalji mi soket");
        }
        else {
            printf("Unesite tekst poruke: ");
            fgets(str2, 250, stdin);
        }

        struct message poruka;
        poruka.direktna = direktno;
        poruka.ime = str1;
        poruka.tekst = str2;

        iResult = send(connectSocket, (const char*)&poruka, sizeof(poruka), 0);
        
        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);
    }

    // cleanup
    closesocket(connectSocket);
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
