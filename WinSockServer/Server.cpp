#include "HashServer.h"

client* hashArray[SIZE];

bool isWorking = true;

CRITICAL_SECTION hashmap_lock;

DWORD WINAPI exit_function(LPVOID lpParam) {
    printf("Unesite bilo sta da ugasite server: ");
    getchar();
    isWorking = false;
    return 0;
}

void GracefullyShutdown(HANDLE a) {
   
    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL) {
            free(hashArray[i]);
            hashArray[i] = NULL;
        }
    }

    CloseHandle(a);

    printf("Cleaning up..");

}

DWORD WINAPI clientHandler(LPVOID lpParam)
{
    // Ovde se obrađuju zahtevi od klijenta

    int flag = 0;
    int send_bytes;

    struct client klijent;//struktura koja se cuva u mapi
    struct message zahtev; //zahtev koji server prima
    ThreadArgs* argumenti = (ThreadArgs*)lpParam; //argumenti prosledjeni threadu

    struct ConnectMessage firstRecieve;//poruka koju server dobija prvo, vezana za registraciju

    char p[10];
    char currIme[NAME_SIZE];

    SOCKET clientSocket = argumenti->socket;
    char* ip = argumenti->ip;

    //promenljive koje se vracaju klijentu u zavisnosti da li je ime koje je poslao pri registraciji vec zauzeto
    char good[5] = "GOOD";
    char bad[5] = "BAD";

    while (flag == 0 && isWorking) {

        if (NonBlockingSocket(clientSocket, 1, 0) == 0) {
            continue;
        }

        int iResult = recv(clientSocket, (char*)&firstRecieve, sizeof(struct ConnectMessage), 0);

        if (iResult > 0)
        {
            EnterCriticalSection(&hashmap_lock);
            client* proveri = search(firstRecieve.clientName, hashArray);
            LeaveCriticalSection(&hashmap_lock);

            if (proveri == NULL && firstRecieve.clientName != "server") {
                iResult = send(clientSocket, good, (int)strlen(good) + 1, 0);
                printf("Registrovan client: %s.\n", firstRecieve.clientName);

            }
            else {
                iResult = send(clientSocket, bad, (int)strlen(bad) + 1, 0);
                continue;
            }
        }
        else if (iResult == 0)
        {
            printf("Connection with client closed.\n");

            EnterCriticalSection(&hashmap_lock);
            clientDelete(hashArray,clientSocket);
            LeaveCriticalSection(&hashmap_lock);

            printf("Client removed.\n");

            closesocket(clientSocket);

            return 1;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());

            EnterCriticalSection(&hashmap_lock);
            clientDelete(hashArray, clientSocket);
            LeaveCriticalSection(&hashmap_lock);

            printf("Client removed.\n");

            closesocket(clientSocket);

            return 1;
        }

        flag++;

        strcpy(klijent.ime, firstRecieve.clientName);
        strcpy(currIme, firstRecieve.clientName);
        klijent.socket = clientSocket;
        memcpy(klijent.ip, ip, INET_ADDRSTRLEN);
        klijent.port = firstRecieve.listenPort;

        //ovde upisujemo
        client* item = (client*)malloc(sizeof(client));

        if (item == NULL) {
            printf("Failed to allocate memory for item.\n");
            return 0;
        }

        strcpy(item->ime, klijent.ime);
        item->socket = clientSocket;
        item->port = klijent.port;
        strcpy(item->ip, klijent.ip);

        int hashIndex = hashCode(klijent.ime);

        EnterCriticalSection(&hashmap_lock);
        while (hashArray[hashIndex] != NULL && hashArray[hashIndex]->ime != "") {
            hashIndex++;
            hashIndex %= SIZE;
        }
        hashArray[hashIndex] = item;
        LeaveCriticalSection(&hashmap_lock);

        printf("Ovde je upisan klijent %s %s %d\n", klijent.ime, klijent.ip, klijent.port);

    }
    while (isWorking) {

        if (NonBlockingSocket(clientSocket, 1, 0) == 0) {
            continue;
        }

        int iResult = recv(clientSocket, (char*)&zahtev, sizeof(struct message), 0);

        char odgovor[50];
        memset(odgovor, 0, sizeof(odgovor));


        if (iResult > 0)
        {
            printf("\nOvo je server primio: %d %s %s\n", zahtev.direktna, zahtev.ime, zahtev.tekst);
        }
        else if (iResult == 0)
        {
            // connection was closed gracefully
            printf("Connection with client closed.\n");

            EnterCriticalSection(&hashmap_lock);
            clientDelete(hashArray, clientSocket);
            LeaveCriticalSection(&hashmap_lock);

            printf("Client removed.\n");

            closesocket(clientSocket);

            return 1;
        }
        else
        {
            // there was an error during recv
            printf("recv failed with error: %d\n", WSAGetLastError());

            EnterCriticalSection(&hashmap_lock);
            clientDelete(hashArray, clientSocket);
            LeaveCriticalSection(&hashmap_lock);

            printf("Client removed.\n");

            closesocket(clientSocket);
            return 1;
        }

        if (zahtev.direktna) {
            //kod koji salje soket
            EnterCriticalSection(&hashmap_lock);
            client* requestedClient = search(zahtev.ime, hashArray);
            LeaveCriticalSection(&hashmap_lock);

            if (requestedClient != NULL)
            {
                itoa(requestedClient->port, p, 10);
                strcat(odgovor, "D ");
                strcat(odgovor, requestedClient->ip);
                strcat(odgovor, " ");
                strcat(odgovor, p);
                strcat(odgovor, " ");
                strcat(odgovor, requestedClient->ime);

                printf("Ovaj odgovor server salje: %s", odgovor);

                if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                    perror("send");
                    return 1;
                }

                memset(odgovor, 0, sizeof(odgovor));
            }
            else 
            {
                
                strcpy(odgovor, "E Ne postoji korisnik sa takvim imenom");

                if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                    perror("send");
                    return 1;
                }
                
                memset(odgovor, 0, sizeof(odgovor));
            }


        }
        else {
            //kod koji prosledjuje poruku
            EnterCriticalSection(&hashmap_lock);
            client* requestedClient = search(zahtev.ime, hashArray);
            LeaveCriticalSection(&hashmap_lock);


            if (requestedClient != NULL)
            {
                strcat(odgovor, "P Klijent ");
                strcat(odgovor, currIme);//registrovano ime
                strcat(odgovor, " salje poruku: ");
                strcat(odgovor, zahtev.tekst);


                printf("Ovaj odgovor server salje: %s", odgovor);

                if ((send_bytes = send(requestedClient->socket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                    perror("send");
                    return 1;
                }
                memset(odgovor, 0, sizeof(odgovor));


            }
            else
            {
                strcpy(odgovor, "Ne postoji taj klijent");

                if ((send_bytes = send(clientSocket, odgovor, (int)strlen(odgovor) + 1, 0)) == -1) {
                    perror("send");
                    return 1;
                }
                memset(odgovor, 0, sizeof(odgovor));


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
    //char recvbuf[DEFAULT_BUFLEN];

    DWORD threadId;
    DWORD threadId2;

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
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
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
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

    HANDLE exit = CreateThread(NULL, 0, exit_function, 0, 0, &threadId2);

    std::vector<HANDLE> threads;
    InitializeCriticalSection(&hashmap_lock);


    while (isWorking)
    {
        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);

        if (NonBlockingSocket(listenSocket, 1, 0) == 0) {
            continue;
        }

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

        threads.push_back(clientThread);

    }

    for (auto& handle : threads) {
        CloseHandle(handle);
    }

    threads.clear();

    printf("Wait..");
    Sleep(2001);

    GracefullyShutdown(exit);

    DeleteCriticalSection(&hashmap_lock);

    Sleep(2001);

    closesocket(listenSocket);
    closesocket(clientSocket);
    WSACleanup();


    return 0;
}

