#pragma once
#include "Header.h"
#define SIZE 2000

int hashCode(char* key);
client* search(char* key, client* array[]);
void insert(client* item);
client* clientDelete(char* key);
void display();