#include "Header.h"

char regName[NAME_SIZE];
klijent* hashArray[SIZE];
bool isWorking = true;

int count1 = 0;
int count2 = 0;

CRITICAL_SECTION hashmap_lock;
CRITICAL_SECTION count1_lock;
CRITICAL_SECTION count2_lock;

void GracefullyShutdown(HANDLE a, HANDLE b, HANDLE c) {

    //ocistimo hash
    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL) {
            free(hashArray[i]);
            hashArray[i] = NULL;
        }
    }

    //zatvorimo handlove
    CloseHandle(a);
    CloseHandle(b);
    CloseHandle(c);
    printf("Cleaning up..");



}

//kada se direktno konektujes ovde primas poruke od tih klijenata
DWORD WINAPI client_IConnect_recv_function(LPVOID lpParam) {

    char buff[1024];
    klijent* client = (klijent*)lpParam;
    SOCKET clientSocket = client->soket;

    while (isWorking) {
        
        if (NonBlockingSocket(clientSocket, 1, 0) == 0) {
            continue;
        }
        
        int iResult = recv(clientSocket, buff, DEFAULT_BUFLEN, 0);
        buff[iResult] = '\0';
        
        if (iResult > 0) {
            printf("\nRecieved: %s.\n", buff);
        }
        else if (iResult == 0) {
            printf("\nConnection closed.\n");
            break;
        }
        else {
            printf("\nError receiving message: %d\n", WSAGetLastError());
            break;
        }

    }

    char clientName[DEFAULT_BUFLEN];

    EnterCriticalSection(&hashmap_lock);
    clientDelete(hashArray, clientSocket, clientName);
    LeaveCriticalSection(&hashmap_lock);

    printf("\nRemoved client %s from hashmap.\n", clientName);

    //cleanup
    closesocket(clientSocket);

    EnterCriticalSection(&count1_lock);
    count1 = count1 - 1;
    LeaveCriticalSection(&count1_lock);

    return 0;

}

//ovde se ide kad se primi od servera port i ip za direktnu komunikaciju
DWORD WINAPI connect_to_client_function(LPVOID lpParam) {

    int iResult;
    DWORD threadId;
    
    zaKonekciju* poruka = (zaKonekciju*)lpParam; 
    
    char ip[INET_ADDRSTRLEN];
    int port = poruka->port;

    strcpy(ip, poruka->ip);
    

    SOCKET connectSocket = INVALID_SOCKET;
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //definisemo port i ip na koji zelimo da se povezemo
    sockaddr_in otherClientAddress;
    otherClientAddress.sin_family = AF_INET;
    otherClientAddress.sin_addr.s_addr = inet_addr(ip);
    otherClientAddress.sin_port = htons(port);

    //konektujemo se
    if (connect(connectSocket, (SOCKADDR*)&otherClientAddress, sizeof(otherClientAddress)) == SOCKET_ERROR)
    {
        printf("\nUnable to connect to other client.\n");
        getch();
        closesocket(connectSocket);
        WSACleanup();
    }

    printf("\nSuccessfully connected to %s!\n", poruka->ime);

    //saljemo nase ime da bi klijent znao sa kim komunicira i da bi mogao da ispise ko mu salje poruke
    iResult = send(connectSocket, regName, (int)strlen(regName) + 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("\nsend failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("\nBytes Sent: %ld\n", iResult);

    //upis u niz ili hash set

    char name[NAME_SIZE];
    klijent* item = (klijent*)malloc(sizeof(klijent));
    if (item == NULL) {
        printf("\nFailed to allocate memory for item.\n");
        return 0;
    }
    strcpy(name, poruka->ime);

    strcpy(item->ime, poruka->ime);
    item->soket = connectSocket;
    int hashIndex = hashCode(name);

    EnterCriticalSection(&hashmap_lock);
    while (hashArray[hashIndex] != NULL && hashArray[hashIndex]->ime != "") {
        hashIndex++;
        hashIndex %= SIZE;
    }
    hashArray[hashIndex] = item;
    LeaveCriticalSection(&hashmap_lock);

    HANDLE recvThread = CreateThread(NULL, 0, client_IConnect_recv_function, item, 0, &threadId);
    if (recvThread != NULL)
    {
        EnterCriticalSection(&count1_lock);
        count1++;
        LeaveCriticalSection(&count1_lock);
    }

    CloseHandle(recvThread);

    return 0;
}

//za unos i slanje
DWORD WINAPI input_thread_function(LPVOID lpParam) {

    int n;
    int recv_bytes;
    bool direktno;
    char pomocna[2];//pomocna samo kupi flag direktna ili ne
    char provera[NAME_SIZE];
    char imeKlijenta[NAME_SIZE];
    char tekstPoruke[TEXT_SIZE];
    struct message poruka;

    while (isWorking) {

        printf("\nUnesite sa kim zelite da komunicirate ('server' ukoliko zelite sa serverom, 'STOP' za gasenje app): ");
        scanf(" %20s", &provera);

        if (strcmp(provera, "STOP") == 0) {
            isWorking = false;
            break;
        }

        if (strcmp(provera, "server") == 0) {

            SOCKET connectSocket = *(SOCKET*)lpParam;
            while (1) {

                while (true) {

                    printf("Unesite koji tip komunikacije zelite D-direktno P-preko servera: ");
                    scanf(" %2s", &pomocna);

                    if (strcmp(pomocna,"D") == 0 || strcmp(pomocna,"d") == 0) {
                        direktno = 1;
                        break;
                    }
                    else if (strcmp(pomocna, "P") == 0 || strcmp(pomocna, "p") == 0) {
                        direktno = 0;
                        break;
                    }
                    else {
                        printf("\nNiste uneli dobar flag!\n");
                    }
                }

               printf("Unesite ime klijenta sa kim zelite da komunicirate: ");
                scanf(" %20s%n", imeKlijenta, &n);
                imeKlijenta[n] = '\0';

                if (direktno == 1) {

                    //treba proveriti da li to sto je u imeKlijenta postoji u nasoj hashmapi i ako postoji - greska

                    EnterCriticalSection(&hashmap_lock);
                    klijent* kl = search(imeKlijenta, hashArray);
                    LeaveCriticalSection(&hashmap_lock);

                    if (kl != NULL) {
                        printf("\nVec ste se konektovali sa tim klijentom.\n");
                        break;
                    }

                    strcpy(tekstPoruke, "Posalji mi soket");
                    //printf("\nOvo se salje serveru: %s %s", imeKlijenta, tekstPoruke);
                }

                else {

                    printf("\nUnesite tekst poruke(do 250 karaktera): ");
                    scanf(" %250[^\n]%n", tekstPoruke, &n);
                    tekstPoruke[n] = '\0';
                    //printf("%s", tekstPoruke);
                   // printf("\nOvo se salje serveru: %s %s", imeKlijenta, tekstPoruke);

                }

                poruka.direktna = direktno;
                strcpy(poruka.ime, imeKlijenta);
                strcpy(poruka.tekst, tekstPoruke);


                int iResult = send(connectSocket, (const char*)&poruka, sizeof(poruka), 0);
                if (iResult == SOCKET_ERROR)
                {
                    printf("\nsend failed with error: %d\n", WSAGetLastError());
                    closesocket(connectSocket);
                    WSACleanup();
                    return 1;
                }

                printf("\nBytes Sent: %ld\n", iResult);
                break;
            }
        }
        else {

            char mess[300];
            char temp[TEXT_SIZE];
            int v;

            strcpy(mess, "Klijent ");
            strcat(mess, regName);
            strcat(mess, " salje: ");

            //proci kroz hash mapu i izvuci klijenta sa kojim zelimo da komuniciramo ukoliko ne postoji baciti continue

            EnterCriticalSection(&hashmap_lock);
            klijent* k = search(provera,hashArray);
            LeaveCriticalSection(&hashmap_lock);

            if (k == NULL) {
                printf("\nne postoji taj klijent\n");
                continue;
            }

            printf("\nUnesite poruku(do 250 karaktera): ");
            scanf(" %250[^\n]%n", temp, &v);
            temp[v] = '\0';

            strcat(mess, temp);

            int iResult = send(k->soket, mess, sizeof(mess), 0);

            if (iResult == SOCKET_ERROR)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(k->soket);
                WSACleanup();
                return 1;
            }

            printf("Bytes Sent: %ld\n", iResult);

        }
    }
    return 0;

}

//za primanje poruka od servera
DWORD WINAPI recv_thread_function(LPVOID lpParam) {

    SOCKET servSocket = *(SOCKET*)lpParam;

    DWORD threadId;

    std::vector<HANDLE> connectThreads;

    int recv_bytes;
    char buff[1024];

    while (isWorking) {


        if (NonBlockingSocket(servSocket, 1, 0) == 0) {
            continue;
        }

        recv_bytes = recv(servSocket, buff, 512, 0);

        if (recv_bytes == -1) {
            perror("recv");
            return 1;
        }

        buff[recv_bytes] = '\0';


        if(strncmp(buff, "D", 1) == 0) {

            char* token;
            char type;
            char ip[INET_ADDRSTRLEN];
            int port;
            char name[NAME_SIZE];

            token = strtok(buff, " ");
            type = token[0];

            token = strtok(NULL, " ");
            strcpy(ip, token);

            token = strtok(NULL, " ");
            port = atoi(token);

            token = strtok(NULL, " ");
            strcpy(name, token);

            printf("\ntype: %c ", type);
            printf("ip: %s ", ip);
            printf("port: %d ", port);
            printf("name: %s\n", name);

            zaKonekciju poruka;

            strcpy(poruka.ip,ip);
            poruka.port = port;
            strcpy(poruka.ime, name);

            HANDLE connectThread = CreateThread(NULL, 0, connect_to_client_function, &poruka, 0, &threadId);
            if (connectThread == NULL)
            {
                printf("Failed to create connect_to_client_function thread.\n");
                return 0;
            }
            connectThreads.push_back(connectThread);
        }
        else{
            printf("\nReceived: %s\n", buff);
        }

    }

    for (auto& handle : connectThreads)
    {
        CloseHandle(handle);
    }

    connectThreads.clear();

    return 0;
}

//ovde posle listena se primaju poruke od klijenata
DWORD WINAPI recv_function_for_client(LPVOID lpParam) {

    SOCKET clientSocket = *(SOCKET*)lpParam;
    
    char message[DEFAULT_BUFLEN];

    while (isWorking) {

        if (NonBlockingSocket(clientSocket, 1, 0) == 0) {
            continue;
        }

        int iResult = recv(clientSocket, message, DEFAULT_BUFLEN, 0);
        
        if (iResult > 0) {
            printf("\n%s.\n", message);
        }
        else if (iResult == 0) {
            printf("Connection closed.\n");
            break;
        }
        else {
            printf("Error receiving message: %d\n", WSAGetLastError());
            break;
        }
    }

    //remove client from hashmap
    char clientName[DEFAULT_BUFLEN];

    EnterCriticalSection(&hashmap_lock);
    clientDelete(hashArray, clientSocket, clientName);
    LeaveCriticalSection(&hashmap_lock);

    printf("\nRemoved client %s from hashmap.\n", clientName);

    //cleanup
    closesocket(clientSocket);
    
    EnterCriticalSection(&count2_lock);
    count2 = count2 - 1;
    LeaveCriticalSection(&count2_lock);

    return 0;

}

//tred za listen  
DWORD WINAPI connection_thread_function(LPVOID lpParam) {

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        return 1;
    }

    int listenPort = *(int*)lpParam;
    
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(listenPort);

    if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        return 1;
    }

    DWORD communicationThreadId;
    std::vector<HANDLE> communication_thread_handles;

    while (isWorking) {

        if(NonBlockingSocket(listenSocket,2,0)==0){
            continue;
        }

        SOCKET connectSocket = accept(listenSocket, NULL, NULL);
        
        if (connectSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            return 1;
        }
       
        char name[NAME_SIZE];

        if (NonBlockingSocket(connectSocket, 2, 0) == 0) {
            continue;
        }
        int iResult = recv(connectSocket, name, 20, 0);

        if (iResult > 0) {
            printf("\nConnection with client established: %s.\n", name);
        
            klijent* item = (klijent*)malloc(sizeof(klijent));

            if (item == NULL) {
                printf("Failed to allocate memory for item.\n");
                return 0;
            }

            strcpy(item->ime, name);
            item->soket = connectSocket;
            int hashIndex = hashCode(name);

            EnterCriticalSection(&hashmap_lock);
            while (hashArray[hashIndex] != NULL && hashArray[hashIndex]->ime != "") {
                hashIndex++;
                hashIndex %= SIZE;
            }
            hashArray[hashIndex] = item;
            LeaveCriticalSection(&hashmap_lock);

        }

        HANDLE communication_thread_handle = CreateThread(NULL, 0, recv_function_for_client, &connectSocket, 0, &communicationThreadId);
        
        if (communication_thread_handle == NULL) {
            printf("CreateThread failed with error: %d\n", GetLastError());
            closesocket(connectSocket);
            return 1;
        }
        EnterCriticalSection(&count2_lock);
        count2++;
        LeaveCriticalSection(&count2_lock);

        communication_thread_handles.push_back(communication_thread_handle);

    }

    // Close all thread handles
    for (auto& handle : communication_thread_handles) {
        CloseHandle(handle);
    }

    communication_thread_handles.clear();

    return 0;
}



int __cdecl main() 
{
    SOCKET connectSocket = INVALID_SOCKET;
    DWORD threadId1;
    DWORD threadId2;
    DWORD threadId3;


    int n;
    int iResult;
    char messageToSend[NAME_SIZE];
    ConnectMessage message; //za registraciju poruka

    printf("Unesite ime za registraciju(MAX 19 karaktera): ");
    scanf(" %20s%n", messageToSend, &n);
    messageToSend[n] = '\0';

    strcpy(message.clientName, messageToSend);

    printf("Unesite port za komunikaciju s drugim klijentima: ");
    scanf("%d", &message.listenPort);

    strcpy(regName, messageToSend);

    printf("\n%s %d\n", message.clientName, message.listenPort);

    if(InitializeWindowsSockets() == false)
    {
		return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
 
    //ovde salje da bi se konektovali na server
    iResult = send( connectSocket, (const char*)&message, sizeof(ConnectMessage), 0);

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    char uspesnaRegistracija[5];

    if ((iResult = recv(connectSocket, uspesnaRegistracija, 5, 0)) == -1) {
        perror("recv");
        return 1;
    }

    while(strcmp(uspesnaRegistracija, "GOOD") != 0) {

        printf("\nTo ime je zauzeto, pokusajte ponovo.\n");

        printf("Unesite ime za registraciju(MAX 19 karaktera): ");
        scanf(" %19s%n", messageToSend, &n);
        messageToSend[n] = '\0';

        strcpy(message.clientName, messageToSend);

        printf("Unesite port za komunikaciju s drugim klijentima: ");
        scanf("%d", &message.listenPort);

        strcpy(regName, messageToSend);

        printf("\n%s %d\n", message.clientName, message.listenPort);

        iResult = send(connectSocket, (const char*)&message, sizeof(ConnectMessage), 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
    
        if ((iResult = recv(connectSocket, uspesnaRegistracija, 5, 0)) == -1) {
            perror("recv");
            return 1;
        }
    }

    printf("Bytes Sent: %ld\n", iResult);

    InitializeCriticalSection(&hashmap_lock);
    InitializeCriticalSection(&count1_lock);
    InitializeCriticalSection(&count2_lock);

    //za slanje poruka
    HANDLE consoleThread = CreateThread(NULL, 0, input_thread_function, &connectSocket, 0, &threadId1);
    
    if (consoleThread == NULL)
    {
        printf("Failed to create console thread.\n");
        closesocket(connectSocket);
        return 0;
    }

    //recv za servera
    HANDLE recvServerThread = CreateThread(NULL, 0, recv_thread_function, &connectSocket, 0, &threadId2);
   
    if (recvServerThread == NULL)
    {
        printf("Failed to create recv server thread.\n");
        closesocket(connectSocket);
        return 0;
    }

    HANDLE listenThread = CreateThread(NULL, 0, connection_thread_function, &message.listenPort, 0, &threadId2);
    
    if (listenThread == NULL)
    {
        printf("Failed to create listen thread.\n");
        closesocket(connectSocket);
        return 0;
    }

    WaitForSingleObject(consoleThread, INFINITE);
    WaitForSingleObject(recvServerThread, INFINITE);
    WaitForSingleObject(listenThread, INFINITE);
    
    while (count1 != 0 && count2 != 0) {

    }

    printf("Wait..");
    Sleep(2000);

    GracefullyShutdown(consoleThread, recvServerThread, listenThread);

    DeleteCriticalSection(&hashmap_lock);
    DeleteCriticalSection(&count1_lock);
    DeleteCriticalSection(&count2_lock);

    Sleep(2001);

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

