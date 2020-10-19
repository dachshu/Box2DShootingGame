#pragma once
#include "stdheader.h"
#define INPUTNUM 6
#define MotherShipMaxHP 1000
#define HEALERMAXHP 250
#define DEALERMAXHP 250
#define TANKERMAXHP 400
#define DEALERMAXSPEED 20
#define HEALERMAXSPEED 30
#define TANKERMAXSPEED 15

#define BULLETRADIUS 0.1
#define HBULLETMAXSPEED 50
#define TBULLETMAXSPEED 50

struct Client;
class Play;
class ByteBuffer;

enum class ObjectType {
	Planet, MotherShip, RespawnPlace, 
	Dealer, Healer, Tanker, 
	HealerBullet, TankerBullet
};

enum INPUTENUM {
	UKey, DKey, LKey, RKey,
	LButton, RButton
};

//// Filter, Mask 정의-------
//uint16 planetFilter = 0x0001;
//uint16 shipFilter = 0x0002;
//uint16 bulletFilter = 0x0004;
//uint16 RrespawnFilter = 0x0010;
//uint16 LrespawnFilter = 0x0020;
//uint16 shieldFilter = 0x1000;
//
//uint16 planetMask = 0x0fff;
//uint16 RshipMask = 0x002f;
//uint16 LshipMask = 0x001f;
//uint16 RbulletMask = 0x1023;
//uint16 LbulletMask = 0x1013;
//uint16 respawnMask = 0x0fff;
//uint16 shildMask = 0x0004;

//----------------------	
class GameObject;

struct UserData {
	GameObject* data;
	ObjectType type;
};

class GameObject
{
protected:
	b2Body* body{ nullptr };
	ObjectType type;
	UserData userData;

public:
	GameObject();
	~GameObject();

	ObjectType GetType() { return type; }
	b2Body* GetBody() { return body; }
	void DestroyBody(b2World* world) { if(body)world->DestroyBody(body); body = nullptr; }
	void SetBodyUserData(GameObject* data) { 
		userData.data = data;
		userData.type = type;
		body->SetUserData(&userData);
	}
};

class Planet : public GameObject {
public:
	Planet(){}
	Planet(b2World* world, b2Vec2 pos, float rad);
	Planet(b2World* world, b2Vec2 pos, float halfWidth, float halfHeight);
};

class RespawnPlace : public GameObject {
	bool team;
public:
	RespawnPlace() {};
	RespawnPlace(b2World* world, bool team);
};

class MotherShip : public GameObject {
	bool team;
	int HP{ MotherShipMaxHP };
public:
	MotherShip() {}
	// MotherShip 크기 미리 결정, team에 따라 위치 결정
	MotherShip(b2World* world, bool team);

	bool GetTeam() { return team; }
	void Heal(int val) {
		HP += val;
		if (HP > MotherShipMaxHP) HP = MotherShipMaxHP;
	}
	void Attack(int val) {
		HP -= val;
	}
	int GetHP() { return HP; }
};

class DynamicObject : public GameObject {
public:
	virtual void PreUpdate(Play* playState) = 0;
	virtual void PostUpate(Play* playState) = 0;
};

class Ship : public DynamicObject {
protected:
	Client* owner;
	std::string ownerName;
	CRITICAL_SECTION ownerCS;

	bool team;

	bool input[INPUTNUM];
	float radian{ 0.0f };
	CRITICAL_SECTION inputCS;

	int HP;
	bool alive{ true };
	std::chrono::system_clock::time_point deadTime;

	bool onSkill{ false };
	std::chrono::system_clock::time_point skillStartTime;
	bool onShot{ false };

public:
	// 생성된 위치는 type에 따른 respawn 위치에서
	Ship(Client* ownerClient, bool team);
	virtual void ProcessInput(Play* playState) = 0;
	virtual void ProcessSpecialSkill(Play* playState) = 0;
	virtual void ProcessRespawn(Play* playState) = 0;

	void SetInput(bool* in, float rad);
	bool GetTeam() { return team; }
	virtual void Heal(int val) = 0;
	virtual void Attack(int val) {	HP -= val;  }
	std::string GetOwnerName() { return ownerName; }
	Client* GetOwner();
	void SetOwner(Client* c);
	void OffShot() { onShot = false; }
	
	void AppendMyData(ByteBuffer* byteBuf);
};

class Dealer : public Ship {
	bool rayReserved{ false };

	b2Vec2 blackHolePos;
public:
	Dealer(b2World* world, Client* ownerClient, bool team);
	virtual void PreUpdate(Play* playState);
	virtual void ProcessInput(Play* playState);
	virtual void ProcessSpecialSkill(Play* playState);
	virtual void ProcessRespawn(Play* playState);

	virtual void PostUpate(Play* playState);

	virtual void Heal(int val){
		HP += val;
		if (HP > DEALERMAXHP) HP = DEALERMAXHP;
	}
};

class Healer : public Ship {
	b2Vec2 dir;
public:
	Healer(b2World* world, Client* ownerClient, bool team);
	virtual void PreUpdate(Play* playState);
	virtual void ProcessInput(Play* playState);
	virtual void ProcessSpecialSkill(Play* playState);
	virtual void ProcessRespawn(Play* playState);

	virtual void PostUpate(Play* playState);

	virtual void Heal(int val) {
		HP += val;
		if (HP > HEALERMAXHP) HP = HEALERMAXHP;
	}
};

class Tanker : public Ship {
	b2Fixture* shieldFixture{ nullptr };
public:
	Tanker(b2World* world, Client* ownerClient, bool team);
	virtual void PreUpdate(Play* playState);
	virtual void ProcessInput(Play* playState);
	virtual void ProcessSpecialSkill(Play* playState);
	virtual void ProcessRespawn(Play* playState);

	virtual void PostUpate(Play* playState);

	virtual void Attack(int val) { if (!shieldFixture) HP -= val; }
	virtual void Heal(int val) {
		HP += val;
		if (HP > TANKERMAXHP) HP = TANKERMAXHP;
	}
};

class Bullet : public DynamicObject {
	bool team;
	b2Vec2 vDir;
	float maxSpeed;
	
	bool crash{ false };
	char target{ 'N' };
	bool targetTeam;
	std::string who;

public:
	Bullet(b2World* world, bool team, ObjectType type, b2Vec2 pos, b2Vec2 dir);
	virtual void PreUpdate(Play* playState);
	virtual void PostUpate(Play* playState);

	bool GetCrash() { return crash; }
	void SetCrash(bool val) { crash = val; }
	void InputCrashData(char tg, bool tgTeam = false, std::string tgName = "") {
		target = tg; targetTeam = tgTeam; who = tgName;
	}
	b2Body* GetBody() { return body; }
	bool GetTeam() { return team; }

	void AppendMyData(ByteBuffer* byteBuf);
};