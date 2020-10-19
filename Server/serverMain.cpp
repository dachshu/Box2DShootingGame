#include "stdheader.h"
#include "State.h"
#include "ClientDataManager.h"

Ready readyState;
Play playState;

int main(int argc, char* argb[])
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
	
	while (true) {
		bool retval;
		retval = readyState.MainThreadProcess();
		if (!retval) break;
		retval = playState.MainThreadProcess();
		if (!retval) break;
	}

	WSACleanup();
}


DWORD WINAPI ProcessClient(LPVOID arg)
{
	Client* client = (Client*)arg;

	while (true) {
		bool ret;
		ret = readyState.ClientThreadProcess(client);
		if (!ret) return 0;
		ret = playState.ClientThreadProcess(client);
		if (!ret) return 0;
	}
}