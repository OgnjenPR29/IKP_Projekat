#include "Hash.h"

klijent* item;

int hashCode(char* key) {
    int hash = 0;
    for (int i = 0; i < strlen(key); i++) {
        hash += key[i];
    }
    return hash % SIZE;
}

klijent* search(char* key, klijent* hashArray[]) {
    int hashIndex = hashCode(key);

    while (hashArray[hashIndex] != NULL) {

        if (strcmp(hashArray[hashIndex]->ime, key) == 0)
            return hashArray[hashIndex];

        ++hashIndex;
        hashIndex %= SIZE;
    }

    return NULL;
}

void insert(klijent* item, klijent* hashArray[]) {
    int hashIndex = hashCode(item->ime);

    while (hashArray[hashIndex] != NULL && strcmp(hashArray[hashIndex]->ime, item->ime) != 0) {
        ++hashIndex;
        hashIndex %= SIZE;
    }
    hashArray[hashIndex] = item;
}

void clientDelete(klijent* hashArray[], SOCKET clientSocket, char clientName[]) {
    for (int i = 0; i < SIZE; i++) {
        if (hashArray[i] != NULL && hashArray[i]->soket == clientSocket) {
            strcpy(clientName, hashArray[i]->ime);
            hashArray[i] = NULL;
            break;
        }
    }
}

void display(klijent* hashArray[]) {
    int i = 0;

    for (i = 0; i < SIZE; i++) {

        if (hashArray[i] != NULL)
            printf(" (%s,%d)", hashArray[i]->ime);
        else
            printf(" ~~ ");
    }

    printf("\n");
}