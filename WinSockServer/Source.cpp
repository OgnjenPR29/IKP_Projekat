#include "Header.h"

int NonBlockingSocket(SOCKET socket, long seconds, long milliseconds)
{
	FD_SET set;
	timeval timeVal;
	FD_ZERO(&set);
	FD_SET(socket, &set);
	timeVal.tv_sec = seconds;
	timeVal.tv_usec = milliseconds;
	return select(0, &set, NULL, NULL, &timeVal);
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
