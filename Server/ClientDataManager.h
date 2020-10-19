#pragma once
#include "stdheader.h"
#include "Manager.h"
#include <vector>

class ByteBuffer;
class Ship;

struct Client
{
	// Client Register 이후엔 모든 thread가 읽기만 하는 데이터
	SOCKET clientSock;
	std::string ID;
	HANDLE hShaerMemoryReadEvent;
	bool team;	// true : Right Team, false : Left Team
	CRITICAL_SECTION sendCS;
	STATE state{ STATE::READY };

	// Client Register 이후에 갱신 될 수 있는 데이터
	char pick{ 'N' };

	Ship* myShip{ nullptr };

	Client()
	{
		hShaerMemoryReadEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		InitializeCriticalSection(&sendCS);
	}

	~Client()
	{
		DeleteCriticalSection(&sendCS);
		CloseHandle(hShaerMemoryReadEvent);
		closesocket(clientSock);
	}
};


class ClientDataManager
{
private:
	CRITICAL_SECTION clientListCS;
	std::vector<Client*> clients;

	ClientDataManager();

	HANDLE hClientReadEvents[MAXCLIENTNUM];
public:
	static ClientDataManager* Instance();

	~ClientDataManager();

	void RegisterClient(Client* clientData);
	void CloseClient(Client* clinetData);

	void Pick(Client* clientData, char pick);

	int GetClientsNum();
	bool IsOKID(const std::string& str);
	bool GetTeamTurn();

	HANDLE* GetClientReadEvents(int& size);

	int SendClientsData(Client* client);
	void SendDataToAllClients(ByteBuffer* byteBuf);
	void SendPlayMessage();
	void SendEndMessage(char winTeam);

	bool IsPlayStateSatisfied();

	void CreateShipObject(b2World* world, std::vector<Ship*>& shipList);

	void ReleasePick();
};

