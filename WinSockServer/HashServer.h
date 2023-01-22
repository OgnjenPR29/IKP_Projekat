#pragma once
#include "Header.h"
#define SIZE 200

int hashCode(char* key);
client* search(char* key, client* array[]);
void insert(client* item, client* array[]);
void clientDelete(client* array[], SOCKET s);
void display(client* array[]);