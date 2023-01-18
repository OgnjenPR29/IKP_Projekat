#pragma once
#include "Header.h"
#define SIZE 2000

/*
typedef struct {
    struct client client;
    char ime[20];
}ClientHash;
*/
int hashCode(char* key);
client* search(char* key, client* array[]);
void insert(client* item);
client* clientDelete(char* key);
void display();