#include "State.h"
#include "ClientDataManager.h"
#include "globalFunc.h"
#include "Manager.h"
#include "Manager.h"
#include "ByteBuffer.h"
#include <chrono>

State::State()
{
}


State::~State()
{
}


// Ready State에서 main thread는 listen과 client를 accept -> register 하는 작업을 한다.
bool Ready::MainThreadProcess()
{
	std::cout << "Main Thread Change State To Ready State" << std::endl;
	int retval;

	// listen socket 생성
	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	SOCKET clientSock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;

	ClientDataManager* clientMgr = ClientDataManager::Instance();

	while (!clientMgr->IsPlayStateSatisfied()) {
		if (clientMgr->GetClientsNum() < MAXCLIENTNUM) {
		addrlen = sizeof(clientaddr);
		clientSock = accept(listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (clientSock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		std::cout << "[TCP Server] 클라이언트 접속: IP Address=" << inet_ntoa(clientaddr.sin_addr)
			<< ", Port Number=" << ntohs(clientaddr.sin_port) << std::endl;

			// 로그인
			// 1.아이디 받기 길이 -> 이름(string)
			char buf[BUFSIZE];
			int len;
			retval = recvn(clientSock, (char*)&len, sizeof(int), 0);
			if ((retval == SOCKET_ERROR) || (retval == 0)) {
				err_display("recv()");
				closesocket(clientSock);
				continue;
			}
			retval = recvn(clientSock, buf, len, 0);
			if ((retval == SOCKET_ERROR) || (retval == 0)) {
				err_display("recv()");
				closesocket(clientSock);
				continue;
			}
			buf[retval] = '\0';
			std::string str(buf);
			std::cout << str << std::endl;

			// 2. 중복 체크
			bool isOK = clientMgr->IsOKID(str);

			// 3. 클라이언트에게 OK/NO 보내기
			retval = send(clientSock, (char*)&isOK, sizeof(bool), 0);
			if ((retval == SOCKET_ERROR) || (retval == 0)) {
				err_display("recv()");
				closesocket(clientSock);
				continue;
			}

			if (!isOK) {
				std::cout << "[TCP Server] ID 중복 클라이언트 거절: IP Address=" << inet_ntoa(clientaddr.sin_addr)
					<< ", Port Number=" << ntohs(clientaddr.sin_port) << std::endl;
				closesocket(clientSock);
				continue;
			}

			Client* clientdata = new Client();
			clientdata->clientSock = clientSock;
			clientdata->ID = str;
			clientdata->team = clientMgr->GetTeamTurn();

			hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)clientdata, 0, NULL);
			if (hThread == NULL) {
				delete clientdata;
				closesocket(clientSock);
				continue;
			}

			clientMgr->RegisterClient(clientdata);
		}
		//else {
		//	std::cout << "[TCP Server] 인원수 초과 클라이언트 거절: IP Address=" << inet_ntoa(clientaddr.sin_addr)
		//		<< ", Port Number=" << ntohs(clientaddr.sin_port) << std::endl;
		//	closesocket(clientSock);
		//}
	}

	closesocket(listenSock);


	// 모든 클라이언트가 픽 완료 했을 때 STATE 바꾸기!!!
	//ShareMemoryManager* memoryMgr = ShareMemoryManager::Instance();
	//memoryMgr->ChangeToPlayState();
	std::cout << "Change State To Play" << std::endl;
	return true;
}

// 클라이언트 중간에 나가면 false 리턴하기
bool Ready::ClientThreadProcess(Client* client)
{
	std::cout << "클라이언트 전용 thread Change State To ReadyState" << std::endl;
	int retval;

	ClientDataManager* clientMgr = ClientDataManager::Instance();
	retval = clientMgr->SendClientsData(client);
	if ((retval == SOCKET_ERROR) || (retval == 0)) return false;
	
	while(true)
	{
		// 클라이언트가 보내는 메시지 읽고 메시지 명령어에 따라 해석(분기)
		char Msg;
		retval = recvn(client->clientSock, &Msg, sizeof(char), 0);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			clientMgr->CloseClient(client);
			return false;
		}
		if (Msg == 'X') {
			client->state = STATE::PLAY;
			break;
		}
		if (Msg == 'I') {
			char tmp[10];
			retval = recvn(client->clientSock, tmp, 10, 0);
			if ((retval == SOCKET_ERROR) || (retval == 0)) {
				err_display("recv()");
				clientMgr->CloseClient(client);
				return false;
			}
			continue;
		}
		retval = ProcessClientMessage(Msg, client);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			clientMgr->CloseClient(client);
			return false;
		}
	}
	return true;
}

int Ready::ProcessClientMessage(char msg, Client * client)
{
	switch (msg)
	{
	case 'P':
		return ProcessPickMessage(client);
	default:
		return 1;
	}
}

int Ready::ProcessPickMessage(Client * client)
{
	int retval;
	char pick;
	retval = recvn(client->clientSock, &pick, sizeof(char), 0);
	if (retval <= 0) return retval;
	// Client Manager한테 Pick 등록
	// 만일 같은 팀에 같은 pick이 거의 동시에 들어온 상태면 먼저 온것 허용
	// 클라이언트는 Pick + 자기id Msg 받으면 pick 성공으로 인식하기
	ClientDataManager* clientMgr = ClientDataManager::Instance();
	clientMgr->Pick(client, pick);

	return retval;
}


bool Play::MainThreadProcess()
{
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	CreateWorld();
	while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp).count() < 3) {}
	
	ClientDataManager* ClientMgr = ClientDataManager::Instance();
	ClientMgr->SendPlayMessage();

	// Start State로 바꾸기
	ShareMemoryManager* memoryMgr = ShareMemoryManager::Instance();
	memoryMgr->ChangeState(STATE::START);
	std::cout << "Main Thread Change State To Play State" << std::endl;

	char winTeam = 'N';
	char exitCode = 'N';
	while (true) {
		tp = std::chrono::system_clock::now();
		// 두 모선중 하나의 체력이 < 0 이면 break
		for (int i = 0; i < 2; ++i) {
			int hp = motherShipList[i].GetHP();
			if (hp <= 0) {
				if (winTeam == 'N') winTeam = (motherShipList[i].GetTeam()) ? 'L' : 'R';
				else winTeam = 'D';
			}
		}
		if (winTeam != 'N') {
			exitCode = 'E'; 
			break;
		}
		if (shipList.size() == 0) {
			exitCode = 'O';  
			break;
		}

		PreUpdate();
		
		// Simulate

		float32 timeStep = 1.0f / 70.0f;
		int32 velocityIterations = 3;
		int32 positionIterations = 1;
		world->Step(timeStep, velocityIterations, positionIterations);
		world->ClearForces();
	
		PostUpdate();
		
		SendData();
		while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - tp).count()
			< 1000 / 50) {
		}
	}
	// 여기서 5초 끌기
	if (exitCode == 'E') {
		tp = std::chrono::system_clock::now();
		while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp).count() < 5) {}
		ByteBuffer byteBuffer;
		byteBuffer.Resize(sizeof(char));
		char msg = 'X';
		byteBuffer.Append((byte*)&msg, sizeof(char));
		ClientMgr->SendDataToAllClients(&byteBuffer);
		ClientMgr->ReleasePick();
	}
	// State 바꾸기
	DestoryWorld();
	return true;
}

bool Play::ClientThreadProcess(Client * client)
{
	std::cout << "클라이언트 전용 thread Change State To PlayState" << std::endl;
	ShareMemoryManager* memMgr = ShareMemoryManager::Instance();
	ClientDataManager* clientMgr = ClientDataManager::Instance();

	int retval;
	while (true)
	{
		// 클라이언트가 보내는 메시지 읽고 클라이언트 ship에 데이터 넣기
		// 이때 고정길이 데이터 받는다 bool 6개 + float 1개
		char Msg;
		retval = recvn(client->clientSock, &Msg, sizeof(char), 0);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			clientMgr->CloseClient(client);
			return false;
		}
		if (Msg == 'P') {
			char pick;
			retval = recvn(client->clientSock, &pick, sizeof(char), 0);
			if ((retval == SOCKET_ERROR) || (retval == 0)) {
				err_display("recv()");
				clientMgr->CloseClient(client);
				return false;
			}
			continue;
		}
		if (Msg == 'X') {
			client->state = STATE::READY;
			break;
		}
		bool input[INPUTNUM];
		retval = recvn(client->clientSock, (char*)input, sizeof(bool)*INPUTNUM, 0);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			clientMgr->CloseClient(client);
			return false;
		}
		float radian;
		retval = recvn(client->clientSock, (char*)&radian, sizeof(float), 0);
		if ((retval == SOCKET_ERROR) || (retval == 0)) {
			err_display("recv()");
			clientMgr->CloseClient(client);
			return false;
		}
		client->myShip->SetInput(input, radian);
	}

	std::cout << "Play State 종료" << std::endl;
	return true;
}

void Play::CreateWorld()
{
	b2Vec2 gravity(0.0f, 0.0f);
	world = new b2World(gravity);

	// Planet 생성
	// 1. world ground box 생성
	//planetList.emplace_back(world, b2Vec2(0.0f, 40.0f), 50.0f, 5.0f);
	//planetList.back().SetBodyUserData(&planetList.back());
	//planetList.emplace_back(world, b2Vec2(0.0f, -40.0f), 50.0f, 5.0f);
	//planetList.back().SetBodyUserData(&planetList.back());
	//planetList.emplace_back(world, b2Vec2(-55.0f, 0.0f), 5.0f, 45.0f);
	//planetList.back().SetBodyUserData(&planetList.back());
	//planetList.emplace_back(world, b2Vec2(55.0f, 0.0f), 5.0f, 45.0f);
	//planetList.back().SetBodyUserData(&planetList.back());
	planetList[0] = Planet(world, b2Vec2(0.0f, 40.0f), 50.0f, 5.0f);
	planetList[1] = Planet(world, b2Vec2(0.0f, -40.0f), 50.0f, 5.0f);
	planetList[2] = Planet(world, b2Vec2(-55.0f, 0.0f), 5.0f, 45.0f);
	planetList[3] = Planet(world, b2Vec2(55.0f, 0.0f), 5.0f, 45.0f);
	planetList[4] = Planet(world, b2Vec2(-15.0f, -1.0f), 1.7f);
	planetList[5] = Planet(world, b2Vec2(-3.0f, -15.0f), 1.7f);
	planetList[6] = Planet(world, b2Vec2(42.0f, -5.0f), 1.7f);
	planetList[7] = Planet(world, b2Vec2(-37.5f, 10.0f), 3.3f);
	planetList[8] = Planet(world, b2Vec2(9.0f, 5.0f), 3.3f);
	planetList[9] = Planet(world, b2Vec2(-30.0f, -20.0f), 5.2f);
	planetList[10] = Planet(world, b2Vec2(29.0f, 20.0f), 5.2f);
	planetList[11] = Planet(world, b2Vec2(18.0f, -15.0f), 6.0f);
	planetList[12] = Planet(world, b2Vec2(-17.0f, 13.0f), 6.6f);
	for (int i = 0; i < PLANETNUM; ++i) {
		planetList[i].SetBodyUserData(&planetList[i]);
	}

	// 2. planet 생성

	// Respawn Place 생성
	respawnList[0] = RespawnPlace(world, false);
	respawnList[0].SetBodyUserData(&respawnList[0]);
	respawnList[1] = RespawnPlace(world, true);
	respawnList[1].SetBodyUserData(&respawnList[1]);
	
	// MotherShip 생성
	motherShipList[0] = MotherShip(world, false);
	motherShipList[0].SetBodyUserData(&motherShipList[0]);
	motherShipList[1] = MotherShip(world, true);
	motherShipList[1].SetBodyUserData(&motherShipList[1]);

	// Ship 생성
	ClientDataManager* clientMgr = ClientDataManager::Instance();
	clientMgr->CreateShipObject(world, shipList);
}

void Play::DestoryWorld()
{
	delete world;
	//planetList.clear();
	for (int i = 0; i < shipList.size(); ++i) delete shipList[i];
	shipList.clear();
	for (int i = 0; i < bulletList.size(); ++i) delete bulletList[i];
	bulletList.clear();
	for (int i = 0; i < 2; ++i)rayInfo[i].tested = false;
}

void Play::PreUpdate()
{
	for (int i = 0; i < shipList.size(); ++i) {
		shipList[i]->PreUpdate(this);
	}
	for (auto it = bulletList.begin(); it != bulletList.end();) {
		if ((*it)->GetCrash()) {
			// body 지우기
			(*it)->DestroyBody(world);
			delete (*it);
			// bullet List에서 지우기
			it = bulletList.erase(it);
		}
		else {
			(*it)->PreUpdate(this);
			++it;
		}
	}
}

void Play::PostUpdate()
{
	for (int i = 0; i < bulletList.size(); ++i) {
		b2Body* bulletBody = bulletList[i]->GetBody();
		bool crash{ false };
		char target{ 'N' };
		bool targetTeam{ false };
		std::string who;
		for (b2ContactEdge* ce = bulletBody->GetContactList(); ce; ce = ce->next) {
			b2Contact* c = ce->contact;
			const b2Body* bodyA = c->GetFixtureA()->GetBody();
			const b2Body* targetBody = c->GetFixtureB()->GetBody();
			if (targetBody == bulletBody) {
				targetBody = bodyA;
			}

			UserData* bUsrData = (UserData*)(bulletBody->GetUserData());
			ObjectType bt = bUsrData->type;
			UserData* tUsrData = (UserData*)(targetBody->GetUserData());
			ObjectType t = tUsrData->type;
			
			switch (t)
			{
			case ObjectType::Planet:
			case ObjectType::RespawnPlace:
				crash = true;
				target = 'O';
				break;
			case ObjectType::Dealer:
			case ObjectType::Healer:
			case ObjectType::Tanker:
			{
				targetTeam = ((Ship*)(tUsrData->data))->GetTeam();
				if ((bulletList[i]->GetType() == ObjectType::HealerBullet && t == ObjectType::Healer)
					&& (bulletList[i]->GetTeam() == targetTeam))break;
				if ((bulletList[i]->GetType() == ObjectType::TankerBullet && t == ObjectType::Tanker)
					&& (bulletList[i]->GetTeam() == targetTeam))break;
				crash = true;
				target = 'S';
				who = ((Ship*)(tUsrData->data))->GetOwnerName();
				if (bulletList[i]->GetType() == ObjectType::HealerBullet) {
					if (bulletList[i]->GetTeam() == targetTeam) {
						((Ship*)(tUsrData->data))->Heal(40);
					}
				}
				else if (bulletList[i]->GetType() == ObjectType::TankerBullet) {
					if (bulletList[i]->GetTeam() != targetTeam) {
						((Ship*)(tUsrData->data))->Attack(40);
					}
				}
				break;
			}
			case ObjectType::MotherShip:
			{
				crash = true;
				target = 'M';
				targetTeam = ((MotherShip*)(tUsrData->data))->GetTeam();
				if (bulletList[i]->GetType() == ObjectType::HealerBullet) {
					if (bulletList[i]->GetTeam() == targetTeam) {
						((MotherShip*)(tUsrData->data))->Heal(40);
					}
				}
				else if (bulletList[i]->GetType() == ObjectType::TankerBullet) {
					if (bulletList[i]->GetTeam() != targetTeam) {
						((MotherShip*)(tUsrData->data))->Attack(40);
					}
				}
				break;
			}
			}
			if (target == 'S' || target == 'M') break;
		}
		bulletList[i]->SetCrash(crash);
		if (crash) {
			switch (target)
			{
			case 'O':
				bulletList[i]->InputCrashData(target);
				break;
			case 'S':
				bulletList[i]->InputCrashData(target, targetTeam, who);
				break;
			case 'M':
				bulletList[i]->InputCrashData(target, targetTeam);
				break;
			}
		}
		bulletList[i]->PostUpate(this);
	}
	for (int i = 0; i < shipList.size(); ++i)
		shipList[i]->PostUpate(this);
}

void Play::SendData()
{
	ClientDataManager* clientMgr = ClientDataManager::Instance();
	//1. ship data 보내기
	ByteBuffer shipData;
	int nameLenSum{ 0 };
	for (auto it = shipList.begin(); it != shipList.end();) {
		if (!(*it)->GetOwner()) {
			// body 지우기
			(*it)->DestroyBody(world);
			delete (*it);
			it = shipList.erase(it);
		}
		else {
			nameLenSum += (*it)->GetOwnerName().size();
			++it;
		}
	}
	int nShip = shipList.size();
	shipData.Resize(sizeof(char) + sizeof(int) + nShip*(sizeof(bool) + sizeof(int) + 3 * sizeof(float)
		+ 2 * sizeof(bool)) + nameLenSum);
	char msg = 'S';
	shipData.Append((byte*)&msg, sizeof(char));
	shipData.Append((byte*)&nShip, sizeof(int));
	for (int i = 0; i < nShip; ++i)
		shipList[i]->AppendMyData(&shipData);
	clientMgr->SendDataToAllClients(&shipData);
	
	// 2. Laser Data
	ByteBuffer laserData;
	laserData.Resize(sizeof(char) + sizeof(int) + 2 * (sizeof(bool) + 4 * sizeof(float) + sizeof(char) + sizeof(int)));
	AppendRayData(&laserData);
	clientMgr->SendDataToAllClients(&laserData);

	// 3. BulletData
	ByteBuffer bulletData;
	msg = 'B';
	int nBullet = bulletList.size();
	bulletData.Resize(sizeof(char) + sizeof(int) + nBullet*(sizeof(char) + sizeof(float) * 2 
		+ sizeof(char) + sizeof(bool) + sizeof(char) + sizeof(int)));
	bulletData.Append((byte*)&msg, sizeof(char));
	bulletData.Append((byte*)&nBullet, sizeof(int));
	//if (nBullet != 0)std::cout << "블릿 수" << nBullet << std::endl;
	for (int i = 0; i < nBullet; ++i) {
		bulletList[i]->AppendMyData(&bulletData);
	}
	clientMgr->SendDataToAllClients(&bulletData);
}

void Play::AppendRayData(ByteBuffer* byteBuf)
{
	char msg = 'L';
	byteBuf->Append((byte*)&msg, sizeof(char));
	int num{ 0 };
	for (int i = 0; i < 2; ++i) if (rayInfo[i].tested) ++num;
	byteBuf->Append((byte*)&num, sizeof(num));
	if (num) {
		for (int i = 0; i < 2; ++i) {
			if (rayInfo[i].tested) {
				rayInfo[i].tested = false;
				float sx{ rayInfo[i].sp.x }, sy{ rayInfo[i].sp.y };
				float ex{ rayInfo[i].ep.x }, ey{ rayInfo[i].ep.y };
				char ownerTeam = (rayInfo[i].ownerTeam) ? 'R' : 'L';
				byteBuf->Append((byte*)&ownerTeam, sizeof(char));
				byteBuf->Append((byte*)&sx, sizeof(float));
				byteBuf->Append((byte*)&sy, sizeof(float));
				byteBuf->Append((byte*)&ex, sizeof(float));
				byteBuf->Append((byte*)&ey, sizeof(float));
				byteBuf->Append((byte*)&rayInfo[i].target, sizeof(char));
				if (rayInfo[i].target == 'O') continue;
				if (rayInfo[i].target == 'M') {
					char targetTeam = (rayInfo[i].targetTeam) ? 'R' : 'L';
					byteBuf->Append((byte*)&targetTeam, sizeof(char));
				}
				else if (rayInfo[i].target == 'S') {
					int len = rayInfo[i].targetName.size();
					byteBuf->Append((byte*)&len, sizeof(int));
					byteBuf->Append((byte*)rayInfo[i].targetName.data(), len);
				}
			}
		}
	}
}

void Play::GiveGravity(bool targetTeam, b2Vec2 pos)
{
	for (int i = 0; i < shipList.size(); ++i) {
		if (shipList[i]->GetTeam() == targetTeam) {
			b2Vec2 targetPos = shipList[i]->GetBody()->GetPosition();
			b2Vec2 dir = pos - targetPos;
			dir.Normalize();
			dir *= 4;
			shipList[i]->GetBody()->ApplyForceToCenter(dir, true);
		}
	}
}

void Play::AddBullet(bool team, ObjectType type, b2Vec2 pos, b2Vec2 dir)
{
	bulletList.emplace_back(new Bullet(world, team, type, pos, dir));
	bulletList.back()->SetBodyUserData(bulletList.back());
}

void Play::RayCating(bool team, b2Vec2 sp, float radian)
{
	int idx = (team) ? 1 : 0;
	int ememyIdx = (team) ? 0 : 1;
	rayInfo[idx].tested = true;
	rayInfo[idx].ownerTeam = team;
	// Planet List, 적 Respawn Space, MotherShip, Ship List들에 대하여
	// RayCasing
	float rayLength = 200;
	b2Vec2 p1 = sp;
	b2Vec2 p2 = p1 + rayLength * b2Vec2(cosf(-(b2_pi / 2.0f - radian) + b2_pi), sinf(-(b2_pi / 2.0f - radian) + b2_pi));
	
	b2RayCastInput input;
	input.p1 = p1;
	input.p2 = p2;
	input.maxFraction = 1;

	float closestFraction = 1; //start with end of line as p2
	GameObject* closestObj{ nullptr };
	//check every fixture of every body to find closest
	// 1. Planet List
	//for (int i = 0; i < planetList.size(); ++i) {
	for (int i = 0; i < PLANETNUM; ++i) {
		for (b2Fixture* f = planetList[i].GetBody()->GetFixtureList(); f; f = f->GetNext()) {
			b2RayCastOutput output;
			if (!f->RayCast(&output, input, 0)) continue;
			if (output.fraction < closestFraction) {
				closestFraction = output.fraction;
				rayInfo[idx].target = 'O';
			}
		}
	}
	// 2. 적 Respawn Space
	for (b2Fixture* f = respawnList[ememyIdx].GetBody()->GetFixtureList(); f; f = f->GetNext()) {
		b2RayCastOutput output;
		if (!f->RayCast(&output, input, 0)) continue;
		if (output.fraction < closestFraction) {
			closestFraction = output.fraction;
			rayInfo[idx].target = 'O';
		}
	}
	// 3. MotherShip
	for (int i = 0; i < 2; ++i) {
		for (b2Fixture* f = motherShipList[i].GetBody()->GetFixtureList(); f; f = f->GetNext()) {
			b2RayCastOutput output;
			if (!f->RayCast(&output, input, 0)) continue;
			if (output.fraction < closestFraction) {
				closestFraction = output.fraction;
				rayInfo[idx].target = 'M';
				rayInfo[idx].targetTeam = motherShipList[i].GetTeam();
				closestObj = &motherShipList[i];
			}
		}
	}
	// 4. Ship List
	for (int i = 0; i < shipList.size(); ++i) {
		for (b2Fixture* f = shipList[i]->GetBody()->GetFixtureList(); f; f = f->GetNext()) {
			b2RayCastOutput output;
			if (!f->RayCast(&output, input, 0)) continue;
			if (output.fraction < closestFraction) {
				closestFraction = output.fraction;
				rayInfo[idx].target = 'S';
				rayInfo[idx].targetTeam = shipList[i]->GetTeam();
				rayInfo[idx].targetName = shipList[i]->GetOwnerName();
				closestObj = shipList[i];
			}
		}
	}
	b2Vec2 intersectionPoint = p1 + closestFraction * (p2 - p1);
	rayInfo[idx].sp = p1;
	rayInfo[idx].ep = intersectionPoint;
	// HP 깍기
	if (rayInfo[idx].target == 'M') {
		if (team != ((MotherShip*)closestObj)->GetTeam()) {
			((MotherShip*)closestObj)->Attack(20);
		}
	}
	else if (rayInfo[idx].target == 'S') {
		if (team != ((Ship*)closestObj)->GetTeam()) {
			((Ship*)closestObj)->Attack(20);
		}
	}

	if (closestFraction == 1)
		rayInfo[idx].tested = false;
	if (closestFraction == 0)
		rayInfo[idx].tested = false;
}
