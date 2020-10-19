#include "ClientDataManager.h"
#include "ByteBuffer.h"
#include "globalFunc.h"
#include "GameObject.h"

ClientDataManager::ClientDataManager()
{
	InitializeCriticalSection(&clientListCS);
}

ClientDataManager* ClientDataManager::Instance()
{
	static ClientDataManager instance;

	return &instance;
}


ClientDataManager::~ClientDataManager()
{
	DeleteCriticalSection(&clientListCS);
}

void ClientDataManager::RegisterClient(Client * clientData)
{
	EnterCriticalSection(&clientListCS);
	// 기존의 클라이언트들에게 IN Msg 보내기
	ByteBuffer inMsg;
	char Msg = 'I';
	int len = clientData->ID.size();
	inMsg.Resize(sizeof(char) + sizeof(int) + len);
	inMsg.Append((byte*)&Msg, sizeof(char));
	inMsg.Append((byte*)&len, sizeof(int));
	inMsg.Append((byte*)clientData->ID.data(), len);

	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&clients[i]->sendCS);
		send(clients[i]->clientSock, (char*)inMsg.Buffer(), inMsg.GetSize(), 0);
		bool myTeam = false;
		if (clientData->team == clients[i]->team) myTeam = true;
		send(clients[i]->clientSock, (char*)&myTeam, sizeof(bool), 0);
		LeaveCriticalSection(&clients[i]->sendCS);
	}

	clients.push_back(clientData);
	LeaveCriticalSection(&clientListCS);


}

void ClientDataManager::CloseClient(Client * clientData)
{
	EnterCriticalSection(&clientListCS);
	std::string id;
	for (auto it = clients.begin(); it != clients.end();) {
		if ((*it) == clientData) {
			id = (*it)->ID;
			if ((*it)->myShip) (*it)->myShip->SetOwner(nullptr);
			delete (*it);
			(*it) = nullptr;
			it = clients.erase(it);
		}
		else ++it;
	}
	// 다른 클라이언트 들에게 Out Message Send
	ByteBuffer outMsg;
	char Msg = 'O';
	int len = id.size();
	outMsg.Resize(sizeof(char) + sizeof(int) + len);
	outMsg.Append((byte*)&Msg, sizeof(char));
	outMsg.Append((byte*)&len, sizeof(int));
	outMsg.Append((byte*)id.data(), len);

	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&clients[i]->sendCS);
		send(clients[i]->clientSock, (char*)outMsg.Buffer(), outMsg.GetSize(), 0);
		LeaveCriticalSection(&clients[i]->sendCS);
	}
	LeaveCriticalSection(&clientListCS);
}

void ClientDataManager::Pick(Client * clientData, char pick)
{
	// 같은 팀중에 pick이 없으면 모든 클라이언트에게(자기 자신 포함) Pick Msg 보내기
	EnterCriticalSection(&clientListCS);
	if (pick != 'N') {
		for (int i = 0; i < clients.size(); ++i) {
			if (clients[i] == clientData) continue;
			else if (clients[i]->team == clientData->team && clients[i]->pick == pick) {
				LeaveCriticalSection(&clientListCS);
				return;
			}
		}
	}
	clientData->pick = pick;

	// 다른 클라이언트 들에게 Pick Message Send
	ByteBuffer pickMsg;
	char Msg = 'P';
	int len = clientData->ID.size();
	pickMsg.Resize(sizeof(char) + sizeof(int) + len + sizeof(char));
	pickMsg.Append((byte*)&Msg, sizeof(char));
	pickMsg.Append((byte*)&len, sizeof(int));
	pickMsg.Append((byte*)clientData->ID.data(), len);
	pickMsg.Append((byte*)&pick, sizeof(char));

	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&clients[i]->sendCS);
		send(clients[i]->clientSock, (char*)pickMsg.Buffer(), pickMsg.GetSize(), 0);
		LeaveCriticalSection(&clients[i]->sendCS);
	}

	std::cout << clientData->ID << "Pick" << pick << std::endl;

	LeaveCriticalSection(&clientListCS);
}

int ClientDataManager::GetClientsNum()
{
	EnterCriticalSection(&clientListCS);
	int ret = clients.size();
	LeaveCriticalSection(&clientListCS);
	return ret;
}

bool ClientDataManager::IsOKID(const std::string & str)
{
	bool ret = true;
	EnterCriticalSection(&clientListCS);
	for (unsigned int i = 0; i < clients.size(); ++i) {
		if (str == clients[i]->ID) {
			ret = false;
			break;
		}
	}
	LeaveCriticalSection(&clientListCS);
	return ret;
}

bool ClientDataManager::GetTeamTurn()
{
	EnterCriticalSection(&clientListCS);
	int nRightTeam = 0;
	int nLeftTeam = 0;
	for (int i = 0; i < clients.size(); ++i) {
		if (clients[i]->team) ++nRightTeam;
		else ++nLeftTeam;
	}
	LeaveCriticalSection(&clientListCS);
	if (nRightTeam == nLeftTeam) return true;
	else if (nRightTeam < nLeftTeam) return true;
	else return false;


}

HANDLE* ClientDataManager::GetClientReadEvents(int& size)
{
	// Read 하는 중에 클라이언트 나갈 수 있다....
	// DeadLock 생각하면서 프로그램 짜자
	// size = GetClientsNum();
	EnterCriticalSection(&clientListCS);
	size = clients.size();
	for (int i = 0; i < size; ++i) {
		hClientReadEvents[i] = clients[i]->hShaerMemoryReadEvent;
	}

	LeaveCriticalSection(&clientListCS);

	return hClientReadEvents;
}

int ClientDataManager::SendClientsData(Client* client)
{
	EnterCriticalSection(&clientListCS);
	int nClient = clients.size();
	int IDLenSum = 0;
	for (int i = 0; i < nClient; ++i)
		IDLenSum += clients[i]->ID.size();
	ByteBuffer byteBuf;
	byteBuf.Resize(nClient*(sizeof(int) + sizeof(bool) + sizeof(char)) + IDLenSum);
	bool myTeam = client->team;
	for (int i = 0; i < nClient; ++i) {
		int lenID = clients[i]->ID.size();
		// ID 길이
		byteBuf.Append((byte*)&lenID, sizeof(int));
		// ID string
		byteBuf.Append((byte*)clients[i]->ID.data(), lenID);
		// Team
		bool isMyTeam = true;
		if (myTeam != clients[i]->team) isMyTeam = false;
		byteBuf.Append((byte*)&isMyTeam, sizeof(bool));
		// Pick
		byteBuf.Append((byte*)&clients[i]->pick, sizeof(char));
	}
	LeaveCriticalSection(&clientListCS);

	int retval;
	EnterCriticalSection(&client->sendCS);

	// client수 보내주기
	retval = send(client->clientSock, (char*)&nClient, sizeof(int), 0);
	if ((retval == SOCKET_ERROR) || (retval == 0)) {
		err_display("recv()");
		CloseClient(client);
		return retval;
	}
	// clients 데이터 보내기 이때 nClient > 0 일때
	if (nClient > 0) {
		retval = send(client->clientSock, (char*)byteBuf.Buffer(), byteBuf.GetSize(), 0);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			CloseClient(client);
			return retval;
		}
	}

	LeaveCriticalSection(&client->sendCS);

	return retval;
}

void ClientDataManager::SendDataToAllClients(ByteBuffer * byteBuf)
{
	EnterCriticalSection(&clientListCS);

	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&(clients[i]->sendCS));
		send(clients[i]->clientSock, (char*)byteBuf->Buffer(), byteBuf->GetSize(), 0);
		LeaveCriticalSection(&(clients[i]->sendCS));
	}
	LeaveCriticalSection(&clientListCS);
}

void ClientDataManager::SendPlayMessage()
{
	// Start Msg, Team('L', 'R') 보내기
	EnterCriticalSection(&clientListCS);
	char msg = 'X';
	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&clients[i]->sendCS);
		char team = (clients[i]->team) ? 'R' : 'L';
		char data[2] = { msg, team };
		send(clients[i]->clientSock, (char*)data, sizeof(char) * 2, 0);
		LeaveCriticalSection(&clients[i]->sendCS);
	}
	LeaveCriticalSection(&clientListCS);
}

void ClientDataManager::SendEndMessage(char winTeam)
{
	EnterCriticalSection(&clientListCS);
	char data[2] = { 'E', winTeam };

	for (int i = 0; i < clients.size(); ++i) {
		EnterCriticalSection(&clients[i]->sendCS);
		send(clients[i]->clientSock, (char*)data, sizeof(char) * 2, 0);
		LeaveCriticalSection(&clients[i]->sendCS);
	}
	LeaveCriticalSection(&clientListCS);
}

bool ClientDataManager::IsPlayStateSatisfied()
{
	EnterCriticalSection(&clientListCS);
	int nClient = clients.size();
	if (nClient == MAXCLIENTNUM) {
		for (int i = 0; i < nClient; ++i) {
			if (clients[i]->pick == 'N') {
				LeaveCriticalSection(&clientListCS);
				return false;
			}
		}
		LeaveCriticalSection(&clientListCS);
		return true;
	}
	else
	{
		LeaveCriticalSection(&clientListCS);
		return false;
	}
}

void ClientDataManager::CreateShipObject(b2World* world, std::vector<Ship*>& shipList)
{
	EnterCriticalSection(&clientListCS);
	shipList.reserve(clients.size());
	for (int i = 0; i < clients.size(); ++i) {
		switch (clients[i]->pick)
		{
		case 'D': {
			Ship* ship = new Dealer(world, clients[i], clients[i]->team);
			ship->SetBodyUserData(ship);
			shipList.push_back(ship);
			clients[i]->myShip = ship;
			break;
		}
		case 'H': {
			Ship* ship = new Healer(world, clients[i], clients[i]->team);
			ship->SetBodyUserData(ship);
			shipList.push_back(ship);
			clients[i]->myShip = ship;
			break;
		}
		case 'T': {
			Ship* ship = new Tanker(world, clients[i], clients[i]->team);
			ship->SetBodyUserData(ship);
			shipList.push_back(ship);
			clients[i]->myShip = ship;
			break; 
		}
		default:
			break;
		}
	}
	LeaveCriticalSection(&clientListCS);
}

void ClientDataManager::ReleasePick()
{
	EnterCriticalSection(&clientListCS);
	for (int i = 0; i < clients.size(); ++i) {
		clients[i]->pick = 'N';
	}
	LeaveCriticalSection(&clientListCS);
}
