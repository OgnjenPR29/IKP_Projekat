#include "Header.h"

char regName[20];
HANDLE mutex;
klijent* hashArray[SIZE];

CRITICAL_SECTION hashmap_lock;
CRITICAL_SECTION server_socket_lock;
CRITICAL_SECTION client_socket_lock;

klijent* search(char* key) {
    int hashIndex = hashCode(key);

    while (hashArray[hashIndex] != NULL) {

        if (strcmp(hashArray[hashIndex]->ime, key) == 0)
            return hashArray[hashIndex];

        ++hashIndex;
        hashIndex %= SIZE;
    }

    return NULL;
}

//TO DO:
// 
//zameniti niz u serveru sa hashmapom -usljebrka
//proslediti hashmapu kao parametar u niti u client.cpp -na kraju
//lockovi ili mutexi ili semafori da se skonta gde treba i da se implementira -sad
//treba pokusati da se razdvoji sve u .h i .cpp fajlove sto je vise moguce -ne znam da li ce moci
//treba napraviti proveru da li je vec neko registrovan sa tim imenom i javiti klijentu da postoji to ime *
//i treba da se brise iz hash mape ukoliko se prekine konekcija sa kljentom - to cemo na kraju 
//treba proveriti da li se klijent konektovao sa nekim klijentom **
//(ili taj klijent sa njim, tj da li postoji u hash mapi na klijentskoj strani) i onemoguciti mu da to opet uradi **




//kada se direktno konektujes ovde primas poruke od tih klijenata
DWORD WINAPI client_IConnect_recv_function(LPVOID lpParam) {

    char buff[1024];
    klijent* client = (klijent*)lpParam;
    SOCKET clientSocket = client->soket;

    while (true) {

        int iResult = recv(clientSocket, buff, DEFAULT_BUFLEN, 0);
        buff[iResult] = '\0';
        if (iResult > 0) {
            printf("\nReceived message from client %s: %s.\n", client->ime, buff);
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
    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL && hashArray[i]->soket == clientSocket) {
            strcpy(clientName, hashArray[i]->ime);
            hashArray[i] = NULL;
            break;
        }
    }
    LeaveCriticalSection(&hashmap_lock);

    printf("\nRemoved client %s from hashmap.\n", clientName);

    //cleanup
    closesocket(clientSocket);
    return 0;

}

//ovde se ide kad se primi od servera port i ip za direktnu komunikaciju -ovde ide hash mapa
DWORD WINAPI connect_to_client_function(LPVOID lpParam) {

    int iResult;
    DWORD threadId;
    
    zaKonekciju* poruka = (zaKonekciju*)lpParam;
    
    char ip[INET_ADDRSTRLEN];
    int port = poruka->port;

    strcpy(ip, poruka->ip);
    
    printf("\nUsao je u konekciju: ip: %s i port %d\n", ip, port);

    //pravimo soket
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

    char name[20];
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

    return 0;
}

//za unos i slanje
DWORD WINAPI input_thread_function(LPVOID lpParam) {

    int n;
    int recv_bytes;
    bool direktno;
    char pomocna[2];
    char provera[20];
    char str1[20];
    char str2[250];
    struct message poruka;

    while (1) {
        printf("\nUnesite sa kim zelite da komunicirate ('server' ukoliko zelite sa serverom): ");
        scanf(" %s", &provera);

        if (strcmp(provera, "server") == 0) {

            SOCKET connectSocket = *(SOCKET*)lpParam;
            while (1) {

                while (true) {

                    printf("Unesite koji tip komunikacije zelite D-direktno P-preko servera: ");
                    scanf(" %s", &pomocna);

                    if (strcmp(pomocna,"D") == 0 || strcmp(pomocna,"d") == 0) {
                        direktno = 1;
                        break;
                    }
                    else if (strcmp(pomocna, "P") == 0 || strcmp(pomocna, "p") == 0) {
                        direktno = 0;
                        break;
                    }
                    else {
                        printf("Niste uneli dobar flag!\n");
                    }
                }

               printf("Unesite ime klijenta sa kim zelite da komunicirate: ");
                scanf(" %s%n", str1, &n);
                str1[n] = '\0';

                if (direktno == 1) {

                    //treba proveriti da li to sto je u str1 postoji u nasoj hashmapi i ako postoji - greska

                    EnterCriticalSection(&hashmap_lock);
                    klijent* kl = search(str1);
                    LeaveCriticalSection(&hashmap_lock);

                    if (kl != NULL) {
                        printf("\nVec ste se konektovali sa tim klijentom.\n");
                        break;
                    }

                    strcpy(str2, "Posalji mi soket");
                    printf("\nOvo se salje serveru: %s %s", str1, str2);
                }

                else {

                    printf("\nUnesite tekst poruke(do 250 karaktera): ");
                    scanf(" %249[^\n]%n", str2, &n);
                    str2[n] = '\0';
                    printf("\nOvo se salje serveru: %s %s", str1, str2);

                }

                poruka.direktna = direktno;
                strcpy(poruka.ime, str1);
                strcpy(poruka.tekst, str2);


                EnterCriticalSection(&server_socket_lock);
                int iResult = send(connectSocket, (const char*)&poruka, sizeof(poruka), 0);
                LeaveCriticalSection(&server_socket_lock);
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
            char temp[250];
            int v;

            strcpy(mess, "Klijent ");
            strcat(mess, regName);
            strcat(mess, " salje: ");

            //proci kroz hash mapu i izvuci klijenta sa kojim zelimo da komuniciramo ukoliko ne postoji baciti continue

            EnterCriticalSection(&hashmap_lock);
            klijent* k = search(provera);
            LeaveCriticalSection(&hashmap_lock);

            if (k == NULL) {
                printf("\nne postoji taj klijent\n");
                continue;
            }

            printf("\nUnesite poruku(do 250 karaktera): ");
            scanf(" %249[^\n]%n", temp, &v);
            temp[v] = '\0';

            strcat(mess, temp);

            EnterCriticalSection(&server_socket_lock);
            int iResult = send(k->soket, mess, sizeof(mess), 0);
            LeaveCriticalSection(&server_socket_lock);

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

    while (true) {

        int recv_bytes;
        char buff[1024];

        EnterCriticalSection(&server_socket_lock);
        recv_bytes = recv(servSocket, buff, 512, 0);
        LeaveCriticalSection(&server_socket_lock);

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
            char name[20];

            token = strtok(buff, " ");
            type = token[0];

            token = strtok(NULL, " ");
            strcpy(ip, token);

            token = strtok(NULL, " ");
            port = atoi(token);

            token = strtok(NULL, " ");
            strcpy(name, token);

            printf("\ntype: %c\n", type);
            printf("ip: %s\n", ip);
            printf("port: %d\n", port);
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
        }
        else{
            printf("\nReceived: %s\n", buff);
        }

    }
    return 0;
}

//ovde posle listena se primaju poruke od klijenata
DWORD WINAPI recv_function_for_client(LPVOID lpParam) {

    /*DWORD wait_result = WaitForSingleObject(mutex, INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        printf("WaitForSingleObject failed: %d\n", GetLastError());
        return 1;
    }*/

    SOCKET clientSocket = *(SOCKET*)lpParam;
    
    char message[DEFAULT_BUFLEN];

    while (1) {
        int iResult = recv(clientSocket, message, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            printf("\nReceived message from client: %s.\n", message);
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
    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL && hashArray[i]->soket == clientSocket) {
            strcpy(clientName, hashArray[i]->ime);
            hashArray[i] = NULL;
            break;
        }
    }
    LeaveCriticalSection(&hashmap_lock);

    printf("\nRemoved client %s from hashmap.\n", clientName);

    //cleanup
    closesocket(clientSocket);
    return 0;

}

//tred za listen -ovde ide hash mapa
DWORD WINAPI connection_thread_function(LPVOID lpParam) {

    /*mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        printf("CreateMutex error: %d\n", GetLastError());
        return 1;
    }*/

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        return 1;
    }

    int listenPort = *(int*)lpParam;
    
    // Bind the socket to an IP address and port
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(listenPort);

    if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        return 1;
    }

    // Start listening for incoming connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        return 1;
    }

    DWORD communicationThreadId;
    // Accept incoming connections and create threads to handle communication

    while (true) {

        SOCKET connectSocket = accept(listenSocket, NULL, NULL);
        
        if (connectSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            return 1;
        }
        //primi ime i sacuvaj  ga


        char name[20];
        int iResult = recv(connectSocket, name, 20, 0);

        if (iResult > 0) {
            printf("\nReceived client name: %s.\n", name);
        
            //save client name and socket
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
    }

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
    char messageToSend[20];
    ConnectMessage message;

    printf("Unesite ime za registraciju(MAX 19 karaktera): ");
    scanf(" %19s%n", messageToSend, &n);
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
 
    //ovde salje ime za rregistraciju i port
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

    //ovde da se uradi prvi recv koji ce da proveri da li smo uspesno registrovani
    //a na serverskoj strani moramo uraditi jedan send odmah nakon sto primimo ime za registraciju
    //i taj send ce da vrati OK ili NE OK i u zavisnosti od toga mi smemo da nastavimo dalje
    //ako je NE OK onda ispisati da postoji to ime i ponovo prikazati formu za registraciju

    printf("Bytes Sent: %ld\n", iResult);

    //mutex = CreateMutex(NULL, FALSE, NULL);

    InitializeCriticalSection(&hashmap_lock);
    InitializeCriticalSection(&server_socket_lock);
    InitializeCriticalSection(&client_socket_lock);

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
    
    DeleteCriticalSection(&hashmap_lock);

    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL) {
            free(hashArray[i]);
            hashArray[i] = NULL;
        }
    }

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

