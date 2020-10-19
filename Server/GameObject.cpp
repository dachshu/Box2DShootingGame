#include "GameObject.h"
#include "State.h"
#include "ClientDataManager.h"
#include "ByteBuffer.h"

uint16 planetFilter = 0x0001;
uint16 shipFilter = 0x0002;
uint16 bulletFilter = 0x0004;
uint16 RrespawnFilter = 0x0010;
uint16 LrespawnFilter = 0x0020;
uint16 shieldFilter = 0x1000;

uint16 planetMask = 0x0fff;
uint16 RshipMask = 0x002f;
uint16 LshipMask = 0x001f;
uint16 RbulletMask = 0x1023;
uint16 LbulletMask = 0x1013;
uint16 respawnMask = 0x0fff;
uint16 shildMask = 0x0004;

GameObject::GameObject()
{
}


GameObject::~GameObject()
{
}

Planet::Planet(b2World * world, b2Vec2 pos, float rad)
{
	type = ObjectType::Planet;

	b2BodyDef planetBodyDef;
	planetBodyDef.position.Set(pos.x, pos.y);
	//planetBodyDef.userData = this;
	body = world->CreateBody(&planetBodyDef);

	b2CircleShape circle;
	circle.m_radius = rad;
	b2FixtureDef fixtureDef;
	fixtureDef.density = 0.0f;
	fixtureDef.shape = &circle;
	fixtureDef.filter.categoryBits = planetFilter;
	fixtureDef.filter.maskBits = planetMask;

	body->CreateFixture(&fixtureDef);
}

Planet::Planet(b2World* world, b2Vec2 pos, float halfWidth, float halfHeight)
{
	type = ObjectType::Planet;

	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(pos.x, pos.y);
	groundBodyDef.userData = this;
	body = world->CreateBody(&groundBodyDef);

	b2PolygonShape groundBox;
	groundBox.SetAsBox(halfWidth, halfHeight);
	b2FixtureDef fixtureDef;
	fixtureDef.density = 0.0f;
	fixtureDef.shape = &groundBox;
	fixtureDef.filter.categoryBits = planetFilter;
	fixtureDef.filter.maskBits = planetMask;

	body->CreateFixture(&fixtureDef);
}

RespawnPlace::RespawnPlace(b2World * world, bool team)
	: team{ team }
{
	type = ObjectType::RespawnPlace;
	
	// right team
	if (team) {
		b2BodyDef bodyDef;
		bodyDef.position.Set(0.0f, -28.0f);
		bodyDef.userData = this;
		body = world->CreateBody(&bodyDef);

		b2PolygonShape box;
		box.SetAsBox(4.0f, 2.0f);
		b2FixtureDef fixtureDef;
		fixtureDef.density = 0.0f;
		fixtureDef.shape = &box;
		fixtureDef.filter.categoryBits = RrespawnFilter;
		fixtureDef.filter.maskBits = respawnMask;

		body->CreateFixture(&fixtureDef);
	}
	// left team
	else
	{
		b2BodyDef bodyDef;
		bodyDef.position.Set(0.0f, 28.0f);
		bodyDef.userData = this;
		body = world->CreateBody(&bodyDef);

		b2PolygonShape box;
		box.SetAsBox(4.0f, 2.0f);
		b2FixtureDef fixtureDef;
		fixtureDef.density = 0.0f;
		fixtureDef.shape = &box;
		fixtureDef.filter.categoryBits = LrespawnFilter;
		fixtureDef.filter.maskBits = respawnMask;

		body->CreateFixture(&fixtureDef);
	}
}

MotherShip::MotherShip(b2World * world, bool team)
	: team(team)
{
	type = ObjectType::MotherShip;

	// right team
	if (team) {
		b2BodyDef bodyDef;
		bodyDef.position.Set(42.0f, -27.0f);
		//bodyDef.userData = this;
		body = world->CreateBody(&bodyDef);

		b2CircleShape circle;
		circle.m_radius = 2.5f;
		b2FixtureDef fixtureDef;
		fixtureDef.density = 0.0f;
		fixtureDef.shape = &circle;
		fixtureDef.filter.categoryBits = shipFilter;
		fixtureDef.filter.maskBits = RshipMask;

		body->CreateFixture(&fixtureDef);
	}
	else {
		b2BodyDef bodyDef;
		bodyDef.position.Set(-42.0f, 27.0f);
		//bodyDef.userData = this;
		body = world->CreateBody(&bodyDef);

		b2CircleShape circle;
		circle.m_radius = 2.5f;
		b2FixtureDef fixtureDef;
		fixtureDef.density = 0.0f;
		fixtureDef.shape = &circle;
		fixtureDef.filter.categoryBits = shipFilter;
		fixtureDef.filter.maskBits = LshipMask;
		

		body->CreateFixture(&fixtureDef);
	}
}

Ship::Ship(Client * ownerClient, bool team)
	: owner{ownerClient}, team{team}
{
	ownerName = owner->ID;
	InitializeCriticalSection(&ownerCS);
	for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
	InitializeCriticalSection(&inputCS);
}

void Ship::SetInput(bool* in, float rad)
{
	EnterCriticalSection(&inputCS);
	for (int i = 0; i < INPUTNUM; ++i)
		input[i] = in[i];
	radian = rad;
	LeaveCriticalSection(&inputCS);
}

Client * Ship::GetOwner()
{
	Client* ret;
	EnterCriticalSection(&ownerCS);
	ret = owner;
	LeaveCriticalSection(&ownerCS);
	return ret;
}

void Ship::SetOwner(Client* c)
{
	EnterCriticalSection(&ownerCS);
	owner = c;
	LeaveCriticalSection(&ownerCS);
}

void Ship::AppendMyData(ByteBuffer * byteBuf)
{
	byteBuf->Append((byte*)&alive, sizeof(bool));
	int len = ownerName.size();
	byteBuf->Append((byte*)&len, sizeof(int));
	byteBuf->Append((byte*)ownerName.data(), len);
	b2Vec2 pos = body->GetPosition();
	float x = pos.x;
	float y = pos.y;
	byteBuf->Append((byte*)&x, sizeof(float));
	byteBuf->Append((byte*)&y, sizeof(float));
	if (!alive) return;
	float rad = body->GetAngle();
	byteBuf->Append((byte*)&rad, sizeof(float));
	byteBuf->Append((byte*)&onShot, sizeof(bool));
	byteBuf->Append((byte*)&onSkill, sizeof(bool));
	onShot = false;
}

Dealer::Dealer(b2World* world, Client * ownerClient, bool team)
	:Ship(ownerClient, team)
{
	type = ObjectType::Dealer;
	HP = DEALERMAXHP;

	b2BodyDef bodyDef;
	if (team) bodyDef.position.Set(0.0f, -28.0f);
	else bodyDef.position.Set(0.0f, 28.0f);
	bodyDef.type = b2_dynamicBody;
	bodyDef.angle = b2_pi / 2.0f;
	bodyDef.linearDamping = 0.5f;
	bodyDef.allowSleep = true;
	bodyDef.awake = true;
	bodyDef.fixedRotation = true;
	bodyDef.bullet = true;
	bodyDef.active = true;
	//bodyDef.userData = this;
	body = world->CreateBody(&bodyDef);

	b2Vec2 vertices[3];
	vertices[0].Set(0.0f, 0.5f);
	vertices[1].Set(-0.4f, -0.5f);
	vertices[2].Set(0.4f, -0.5f);
	int32 count = 3;

	b2PolygonShape dealerShip;
	dealerShip.Set(vertices, count);

	b2FixtureDef fixtureDef;
	fixtureDef.density = 1.0f;
	fixtureDef.filter.categoryBits = shipFilter;
	if (team)fixtureDef.filter.maskBits = RshipMask;
	else fixtureDef.filter.maskBits = LshipMask;
	fixtureDef.friction = 0.3f;
	fixtureDef.shape = &dealerShip;
	body->CreateFixture(&fixtureDef);
}

void Dealer::PreUpdate(Play* playState)
{
	if (alive) {
		Dealer::ProcessInput(playState);
		Dealer::ProcessSpecialSkill(playState);

	}
	else { Dealer::ProcessRespawn(playState); }
}

void Dealer::ProcessInput(Play* playState)
{
	EnterCriticalSection(&inputCS);
	b2Vec2 force(0.0f, 0.0f);

	if (input[INPUTENUM::UKey]) force += b2Vec2(0.0f, 1.0f);
	if (input[INPUTENUM::DKey]) force += b2Vec2(0.0f, -1.0f);
	if (input[INPUTENUM::LKey]) force += b2Vec2(-1.0f, 0.0f);
	if (input[INPUTENUM::RKey]) force += b2Vec2(1.0f, 0.0f);
	force.Normalize();
	force *= 3.0f;
	body->ApplyForceToCenter(force, true);

	if (input[INPUTENUM::LButton]) {
		rayReserved = true; onShot = true;
	}
	if (input[INPUTENUM::RButton]) {
		onSkill = true;
		skillStartTime = std::chrono::system_clock::now();
		blackHolePos = body->GetPosition();
	}
	b2Vec2 pos = body->GetPosition();
	body->SetTransform(pos, radian);
	for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
	LeaveCriticalSection(&inputCS);
}

void Dealer::ProcessSpecialSkill(Play * playState)
{
	if (onSkill) {
		auto d = std::chrono::system_clock::now() - skillStartTime;
		if (std::chrono::duration_cast<std::chrono::seconds>(d).count() < 4) {
			playState->GiveGravity(!team, blackHolePos);
		}
		else {
			onSkill = false;
		}
	}
}

void Dealer::ProcessRespawn(Play * playState)
{
	auto d = std::chrono::system_clock::now() - deadTime;
	if (std::chrono::duration_cast<std::chrono::seconds>(d).count() > 5) {
		body->SetActive(true);
		float y = (team) ? -28.0f : 28.0f;
		body->SetTransform(b2Vec2(0.0f, y), b2_pi / 2);
		alive = true;
	}
}

void Dealer::PostUpate(Play* playState)
{
	// RayCasting 贸府
	if (rayReserved) {
		playState->RayCating(team, body->GetPosition(), body->GetAngle());
		rayReserved = false;
	}
	// velocity maximum 贸府
	b2Vec2 linearVelocity = body->GetLinearVelocity();
	if (linearVelocity.Length() >=  DEALERMAXSPEED) {
		linearVelocity.Normalize();
		linearVelocity *= DEALERMAXSPEED;
		body->SetLinearVelocity(linearVelocity);
	}
	// HP 犬牢 alive贸府
	if (HP <= 0) {
		deadTime = std::chrono::system_clock::now();
		alive = false;
		body->SetActive(false);
		rayReserved = false;
		for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
		onSkill = false;
		HP = DEALERMAXHP;
	}
}

Healer::Healer(b2World * world, Client * ownerClient, bool team)
	:Ship(ownerClient, team)
{
	type = ObjectType::Healer;
	HP = HEALERMAXHP;

	b2BodyDef bodyDef;
	if (team) bodyDef.position.Set(-3.0f, -28.0f);
	else bodyDef.position.Set(-3.0f, 28.0f);
	bodyDef.type = b2_dynamicBody;
	bodyDef.angle = b2_pi / 2.0f;
	bodyDef.linearDamping = 0.5f;
	bodyDef.allowSleep = true;
	bodyDef.awake = true;
	bodyDef.fixedRotation = true;
	bodyDef.bullet = true;
	bodyDef.active = true;
	//bodyDef.userData = this;
	body = world->CreateBody(&bodyDef);

	b2Vec2 vertices[6];
	vertices[0].Set(-0.15f, 0.75f);
	vertices[1].Set(-0.5f, -0.25f);
	vertices[2].Set(-0.15f, -0.75f);
	vertices[3].Set(0.15f, -0.75f);
	vertices[4].Set(0.5f, -0.25f);
	vertices[5].Set(0.15f, 0.75f);
	int32 count = 6;

	b2PolygonShape healerShip;
	healerShip.Set(vertices, count);

	b2FixtureDef fixtureDef;
	fixtureDef.density = 0.7f;
	fixtureDef.filter.categoryBits = shipFilter;
	if (team)fixtureDef.filter.maskBits = RshipMask;
	else fixtureDef.filter.maskBits = LshipMask;
	fixtureDef.friction = 0.3f;
	fixtureDef.shape = &healerShip;
	body->CreateFixture(&fixtureDef);
}

void Healer::PreUpdate(Play * playState)
{
	if (alive) {
		Healer::ProcessInput(playState);
		Healer::ProcessSpecialSkill(playState);

	}
	else { Healer::ProcessRespawn(playState); }
}

void Healer::ProcessInput(Play * playState)
{
	EnterCriticalSection(&inputCS);
	b2Vec2 force(0.0f, 0.0f);
	if (input[INPUTENUM::UKey]) force += b2Vec2(0.0f, 1.0f);
	if (input[INPUTENUM::DKey]) force += b2Vec2(0.0f, -1.0f);
	if (input[INPUTENUM::LKey]) force += b2Vec2(-1.0f, 0.0f);
	if (input[INPUTENUM::RKey]) force += b2Vec2(1.0f, 0.0f);
	force.Normalize();
	dir = force;
	dir *= 5;
	force *= 3.0f;
	body->ApplyForceToCenter(force, true);

	if (input[INPUTENUM::LButton]) {
		playState->AddBullet(team, ObjectType::HealerBullet, body->GetPosition(),
			b2Vec2(cosf(-(b2_pi/2.0f - radian) + b2_pi), sinf(-(b2_pi / 2.0f - radian) + b2_pi)));
		onShot = true;
	}
	if (input[INPUTENUM::RButton]) {
		onSkill = true;
		skillStartTime = std::chrono::system_clock::now();
	}
	b2Vec2 pos = body->GetPosition();
	body->SetTransform(pos, radian);
	for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
	LeaveCriticalSection(&inputCS);
}

void Healer::ProcessSpecialSkill(Play * playState)
{
	if (onSkill) {
		auto d = std::chrono::system_clock::now() - skillStartTime;
		if (std::chrono::duration_cast<std::chrono::seconds>(d).count() < 3) {
			body->ApplyForceToCenter(dir, true);
		}
		else {
			onSkill = false;
		}
	}
}

void Healer::ProcessRespawn(Play * playState)
{
	auto d = std::chrono::system_clock::now() - deadTime;
	if (std::chrono::duration_cast<std::chrono::seconds>(d).count() > 5) {
		body->SetActive(true);
		float y = (team) ? -28.0f : 28.0f;
		body->SetTransform(b2Vec2(-3.0f, y), b2_pi / 2);
		alive = true;
	}
}

void Healer::PostUpate(Play * playState)
{
	// velocity maximum 贸府
	b2Vec2 linearVelocity = body->GetLinearVelocity();
	if (linearVelocity.Length() >= HEALERMAXSPEED) {
		linearVelocity.Normalize();
		linearVelocity *= HEALERMAXSPEED;
		body->SetLinearVelocity(linearVelocity);
	}
	// HP 犬牢 alive贸府
	if (HP <= 0) {
		deadTime = std::chrono::system_clock::now();
		alive = false;
		body->SetActive(false);
		for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
		onSkill = false;
		HP = HEALERMAXHP;
	}
}

Tanker::Tanker(b2World * world, Client * ownerClient, bool team)
	: Ship(ownerClient, team)
{
	type = ObjectType::Tanker;
	HP = TANKERMAXHP;

	b2BodyDef bodyDef;
	if (team) bodyDef.position.Set(3.0f, -28.0f);
	else bodyDef.position.Set(3.0f, 28.0f);
	bodyDef.type = b2_dynamicBody;
	bodyDef.angle = b2_pi / 2.0f;
	bodyDef.linearDamping = 0.5f;
	bodyDef.allowSleep = true;
	bodyDef.awake = true;
	bodyDef.fixedRotation = true;
	bodyDef.bullet = true;
	bodyDef.active = true;
	//bodyDef.userData = this;
	body = world->CreateBody(&bodyDef);

	b2Vec2 vertices[6];
	vertices[0].Set(-0.5f, 0.75f);
	vertices[1].Set(-0.75f, 0.5f);
	vertices[2].Set(-0.75f, -0.75f);
	vertices[3].Set(0.75f, -0.75f);
	vertices[4].Set(0.75f, 0.5f);
	vertices[5].Set(0.5f, 0.75f);
	int32 count = 6;

	b2PolygonShape tankerShip;
	tankerShip.Set(vertices, count);

	b2FixtureDef fixtureDef;
	fixtureDef.density = 0.5f;
	fixtureDef.filter.categoryBits = shipFilter;
	if (team)fixtureDef.filter.maskBits = RshipMask;
	else fixtureDef.filter.maskBits = LshipMask;
	fixtureDef.friction = 0.3f;
	fixtureDef.shape = &tankerShip;
	body->CreateFixture(&fixtureDef);
}

void Tanker::PreUpdate(Play * playState)
{
	if (alive) {
		Tanker::ProcessInput(playState);
		Tanker::ProcessSpecialSkill(playState);

	}
	else { Tanker::ProcessRespawn(playState); }
}

void Tanker::ProcessInput(Play * playState)
{
	EnterCriticalSection(&inputCS);
	b2Vec2 force(0.0f, 0.0f);
	if (input[INPUTENUM::UKey]) force += b2Vec2(0.0f, 1.0f);
	if (input[INPUTENUM::DKey]) force += b2Vec2(0.0f, -1.0f);
	if (input[INPUTENUM::LKey]) force += b2Vec2(-1.0f, 0.0f);
	if (input[INPUTENUM::RKey]) force += b2Vec2(1.0f, 0.0f);
	force.Normalize();
	force *= 3.0f;
	body->ApplyForceToCenter(force, true);

	if (input[INPUTENUM::LButton]) {
		playState->AddBullet(team, ObjectType::TankerBullet, body->GetPosition(),
			b2Vec2(cosf(-(b2_pi / 2.0f - radian) + b2_pi), sinf(-(b2_pi / 2.0f - radian) + b2_pi)));
		onShot = true;
	}
	if (!onSkill) {
		if (input[INPUTENUM::RButton]) {
			onSkill = true;
			skillStartTime = std::chrono::system_clock::now();

			b2CircleShape circle;
			circle.m_radius = 3.0f;
			b2FixtureDef fixtureDef;
			fixtureDef.density = 0.01f;
			fixtureDef.filter.categoryBits = shieldFilter;
			fixtureDef.filter.maskBits = shildMask;
			fixtureDef.friction = 0.1f;
			fixtureDef.shape = &circle;
			shieldFixture = body->CreateFixture(&fixtureDef);
		}
	}
	else {
		if (!input[INPUTENUM::RButton]) {
			body->DestroyFixture(shieldFixture);
			shieldFixture = nullptr;
			onSkill = false;
		}
	}
	b2Vec2 pos = body->GetPosition();
	body->SetTransform(pos, radian);
	for (int i = 0; i < INPUTNUM - 1; ++i) input[i] = false;
	LeaveCriticalSection(&inputCS);
}

void Tanker::ProcessSpecialSkill(Play * playState)
{
}

void Tanker::ProcessRespawn(Play * playState)
{
	auto d = std::chrono::system_clock::now() - deadTime;
	if (std::chrono::duration_cast<std::chrono::seconds>(d).count() > 5) {
		body->SetActive(true);
		float y = (team) ? -28.0f : 28.0f;
		body->SetTransform(b2Vec2(3.0f, y), b2_pi / 2);
		alive = true;
		if(shieldFixture)body->DestroyFixture(shieldFixture);
		shieldFixture = nullptr;
		onSkill = false;
	}
}

void Tanker::PostUpate(Play * playState)
{
	// velocity maximum 贸府
	b2Vec2 linearVelocity = body->GetLinearVelocity();
	if (linearVelocity.Length() >= TANKERMAXSPEED) {
		linearVelocity.Normalize();
		linearVelocity *= TANKERMAXSPEED;
		body->SetLinearVelocity(linearVelocity);
	}
	// HP 犬牢 alive贸府
	if (HP <= 0) {
		deadTime = std::chrono::system_clock::now();
		alive = false;
		body->SetActive(false);
		for (int i = 0; i < INPUTNUM; ++i) input[i] = false;
		onSkill = false;
		HP = TANKERMAXHP;
	}
}

Bullet::Bullet(b2World* world, bool team, ObjectType t, b2Vec2 pos, b2Vec2 dir)
	: team(team)
{
	type = t;
	dir.Normalize();
	vDir = dir;
	if (t == ObjectType::HealerBullet) maxSpeed = HBULLETMAXSPEED;
	else if (t == ObjectType::TankerBullet) maxSpeed = TBULLETMAXSPEED;

	b2BodyDef bodyDef;
	b2Vec2 offSet = dir;
	offSet *= 0.7f;
	pos += offSet;
	bodyDef.position.Set(pos.x, pos.y);
	bodyDef.type = b2_dynamicBody;
	bodyDef.angle = 0.0f;
	bodyDef.linearDamping = 0.0f;
	bodyDef.allowSleep = false;
	bodyDef.awake = true;
	bodyDef.fixedRotation = true;
	bodyDef.bullet = true;
	bodyDef.active = true;
	//bodyDef.userData = this;
	dir *= 10;
	bodyDef.linearVelocity = dir;
	body = world->CreateBody(&bodyDef);

	b2CircleShape circle;
	circle.m_radius = BULLETRADIUS;
	b2FixtureDef fixtureDef;
	fixtureDef.density = 1.0f;
	fixtureDef.filter.categoryBits = bulletFilter;
	if (team) fixtureDef.filter.maskBits = RbulletMask;
	else fixtureDef.filter.maskBits = LbulletMask;
	fixtureDef.friction = 0.1f;
	fixtureDef.shape = &circle;
	body->CreateFixture(&fixtureDef);
}

void Bullet::PreUpdate(Play * playState)
{
	if (!crash) {
		//b2Vec2 force = vDir;
		//if (type == ObjectType::HealerBullet) force *= 5;
		//else if (type == ObjectType::TankerBullet) force *= 7;
		//body->ApplyForceToCenter(force, true);
	}
}

void Bullet::PostUpate(Play * playState)
{
	b2Vec2 linearVelocity = body->GetLinearVelocity();
	if (linearVelocity.Length() >= maxSpeed) {
		linearVelocity.Normalize();
		linearVelocity *= maxSpeed;
		body->SetLinearVelocity(linearVelocity);
	}
}

void Bullet::AppendMyData(ByteBuffer * byteBuf)
{
	char ownerTeam = (team) ? 'R' : 'L';
	b2Vec2 pos = body->GetPosition();
	float x = pos.x;
	float y = pos.y;
	char t;
	if (type == ObjectType::HealerBullet) t = 'H';
	else if (type == ObjectType::TankerBullet) t = 'T';

	byteBuf->Append((byte*)&ownerTeam, sizeof(char));
	byteBuf->Append((byte*)&x, sizeof(float));
	byteBuf->Append((byte*)&y, sizeof(float));
	byteBuf->Append((byte*)&t, sizeof(char));
	byteBuf->Append((byte*)&crash, sizeof(bool));
	if (!crash) return;
	byteBuf->Append((byte*)&target, sizeof(char));
	if (target == 'O') return;
	if (target == 'M') {
		char tgTeam = (targetTeam) ? 'R' : 'L';
		byteBuf->Append((byte*)&tgTeam, sizeof(char));
		return;
	}
	if (target == 'S') {
		int len = who.size();
		byteBuf->Append((byte*)&len, sizeof(int));
		byteBuf->Append((byte*)who.data(), len);
	}

}
