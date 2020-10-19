#pragma once
#include "stdheader.h"

void err_quit(char* msg);
void err_display(char* msg);
int recvn(SOCKET s, char* buf, int len, int flags);

DWORD WINAPI ProcessClient(LPVOID arg);