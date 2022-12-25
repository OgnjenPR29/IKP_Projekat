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
HANDLE mutex;

struct message {
    
    bool direktna;
    char ime[20];
    char tekst[250];

};

DWORD WINAPI input_thread_function(LPVOID lpParam) {

    int n;
    int recv_bytes;
    bool direktno;
    char pomocna;
    char str1[20];
    char str2[250];
    struct message poruka;


    SOCKET connectSocket = *(SOCKET*)lpParam;
    while (1) {

        while (true) {

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
        scanf(" %s%n", str1,&n);
        str1[n] = '\0';


        if (direktno == 1) {
            strcpy(str2, "Posalji mi soket");
            printf("Ovo se salje serveru: %s %s", str1, str2);
        }

        else {

            printf("\nUnesite tekst poruke: ");
            scanf(" %[^\n]%n", str2, &n);
            str2[n] = '\0';
            printf("Ovo se salje serveru: %s %s", str1, str2);

        }

        poruka.direktna = direktno;
        strcpy(poruka.ime, str1);
        strcpy(poruka.tekst, str2);

//        WaitForSingleObject(mutex, INFINITE);
        int iResult = send(connectSocket, (const char*)&poruka, sizeof(poruka), 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);
       // ReleaseMutex(mutex);
    }

    return 0;

}

DWORD WINAPI recv_thread_function(LPVOID lpParam) {

    SOCKET connectSocket = *(SOCKET*)lpParam;

    while (true) {

        int recv_bytes;
        char buff[1024];

        //WaitForSingleObject(mutex, INFINITE);
        if ((recv_bytes = recv(connectSocket, buff, 512, 0)) == -1) {
            perror("recv");
            return 1;
        }

        buff[recv_bytes] = '\0';

        printf("\nReceived: %s\n", buff);
        //ReleaseMutex(mutex);

    }
    return 0;
}

int __cdecl main() 
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    DWORD threadId1;
    DWORD threadId2;
    int n;

    printf("Unesite ime za registraciju: ");
    char messageToSend[20];
    scanf(" %s%n", messageToSend, &n);
    messageToSend[n] = '\0';

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

    mutex = CreateMutex(NULL, FALSE, NULL);

    HANDLE consoleThread = CreateThread(NULL, 0, input_thread_function, &connectSocket, 0, &threadId1);
    if (consoleThread == NULL)
    {
        printf("Failed to create client thread.\n");
        closesocket(connectSocket);
        return 0;
    }

    HANDLE recvThread = CreateThread(NULL, 0, recv_thread_function, &connectSocket, 0, &threadId2);
    if (recvThread == NULL)
    {
        printf("Failed to create client thread.\n");
        closesocket(connectSocket);
        return 0;
    }

    WaitForSingleObject(consoleThread, INFINITE);
    WaitForSingleObject(recvThread, INFINITE);
    

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
