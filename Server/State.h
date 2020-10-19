#pragma once
#include <vector>
#include "GameObject.h"
#define PLANETNUM 13

struct Client;

class State
{
public:
	State();
	~State();

	// main thread�� ȣ���ؼ� �Լ��ϴ� �Լ�
	virtual bool MainThreadProcess() = 0;
	virtual bool ClientThreadProcess(Client* ) = 0;
};


class Ready : public State
{
public:
	virtual bool MainThreadProcess();
	virtual bool ClientThreadProcess(Client*);
	
private:
	int ProcessClientMessage(char msg, Client* client);
	// Msg ó�� �Լ�
	int ProcessPickMessage(Client* client);
};

class Play : public State
{
private:
	struct RayCastInfo {
		bool tested{ false };
		bool ownerTeam;
		b2Vec2 sp;
		b2Vec2 ep;
		char target{ 'N' };
		bool targetTeam;
		std::string targetName;
	};

public:
	Play(){}
	virtual bool MainThreadProcess();
	virtual bool ClientThreadProcess(Client* client);


private:
	// Main Thread ȣ�� �Լ�
	void CreateWorld();
	// ��ü ���� �ʱ�ȭ
	void DestoryWorld();
	// Main Thread Routine �Լ�
	void PreUpdate();
	void PostUpdate();
	void SendData();

	void AppendRayData(ByteBuffer* byteBuf);

private:
	b2World* world{ nullptr };

	//std::vector<Planet> planetList;
	Planet planetList[PLANETNUM];

	// idx 0 : left, idx 1 : right
	RespawnPlace respawnList[2];
	MotherShip motherShipList[2];

	std::vector<Ship*> shipList;
	std::vector<Bullet*> bulletList;

	// idx 0 : left, idx 1: right
	RayCastInfo rayInfo[2];
public:

	void GiveGravity(bool targetTeam, b2Vec2 pos);
	void AddBullet(bool team, ObjectType type, b2Vec2 pos, b2Vec2 dir);
	void RayCating(bool team, b2Vec2 sp, float radian);
};