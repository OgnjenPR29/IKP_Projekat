#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

bool InitializeWindowsSockets();

struct message {
    
    bool direktna;
    char ime[20];
    char tekst[250];

};

int __cdecl main() 
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    printf("Unesite ime za registraciju: ");
    char messageToSend[20];
    scanf(" %s", messageToSend);

    if(InitializeWindowsSockets() == false)
    {
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

    /*int random_number = rand() % 100 + 1;

    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    //clientAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    clientAddress.sin_port = htons(27017 + random_number);

    bind(connectSocket, (SOCKADDR*)&clientAddress, sizeof(clientAddress));*/

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

    while (true) {

        int recv_bytes;
        bool direktno;
        char buff[1024];
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
        scanf(" %s", str1);
        printf("%s\n", str1);


        if (direktno == 1) {
            strcpy(str2,"Posalji mi soket");
            printf("%s\n", str2);
        }
        else {
            printf("Unesite tekst poruke: ");
            scanf(" %s", str2);
            printf("%s\n", str2);

        }

        struct message poruka;
        poruka.direktna = direktno;
        strcpy(poruka.ime,str1);
        strcpy(poruka.tekst,str2);

        iResult = send(connectSocket, (const char*)&poruka, sizeof(poruka), 0);
        printf("Prosao je send poruke\n");


        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);

        if ((recv_bytes = recv(connectSocket, buff, 512, 0)) == -1) {
            perror("recv");
            return 1;
        }
        buff[recv_bytes] = '\0';

        //recv_bytes = recv(connectSocket, buff, 512, 0);

        printf("Received: %s\n", buff);


    }

    closesocket(connectSocket);
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
