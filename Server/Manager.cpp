#include "Manager.h"
#include "ClientDataManager.h"

ShareMemoryManager::ShareMemoryManager()
{
	hMainThreadWrite = CreateEvent(NULL, TRUE, TRUE, NULL);
}

ShareMemoryManager::~ShareMemoryManager()
{
	CloseHandle(hMainThreadWrite);
}

ShareMemoryManager * ShareMemoryManager::Instance()
{
	static ShareMemoryManager instance;

	return &instance;
}

// Client수가 MAXCLIENT가 되었을 때 실행
// ->listen socket을 닫아서 새로 클라이언트가 들어오진 않는다
//   BUT!!! 중간에 나가버릴 수가 있다.
//   Event가 중간에 닫힐 수 있다.
//   -> 이에 대한 처리하자
//				DeadLock 상상하면서 프로그램 만들자
void ShareMemoryManager::ChangeToPlayState()
{
	ResetEvent(hMainThreadWrite);
	
	while (true) {
		// ClientShareMemoryReadEvent 리스트, 숫자를 가져온다.
		ClientDataManager* clientMgr = ClientDataManager::Instance();
		int nClients, retval;
		HANDLE* hClientReadEvents = clientMgr->GetClientReadEvents(nClients);
		retval = WaitForMultipleObjects(nClients, hClientReadEvents, TRUE, INFINITE);
		// 이때 중간에 Client가 나갈 수 있기 때문에
		if (retval == WAIT_OBJECT_0) break;
	}
	state = STATE::PLAY;
	SetEvent(hMainThreadWrite);
}

void ShareMemoryManager::GetState(HANDLE hClientReadEvent, STATE& s)
{
	WaitForSingleObject(hMainThreadWrite, INFINITE);
	ResetEvent(hClientReadEvent);
	s = state;
	SetEvent(hClientReadEvent);
}

void ShareMemoryManager::ChangeState(STATE val)
{
	ResetEvent(hMainThreadWrite);
	while (true) {
		// ClientShareMemoryReadEvent 리스트, 숫자를 가져온다.
		ClientDataManager* clientMgr = ClientDataManager::Instance();
		int nClients, retval;
		HANDLE* hClientReadEvents = clientMgr->GetClientReadEvents(nClients);
		retval = WaitForMultipleObjects(nClients, hClientReadEvents, TRUE, INFINITE);
		// 이때 중간에 Client가 나갈 수 있기 때문에
		if (retval == WAIT_OBJECT_0) break;
	}
	state = val;
	SetEvent(hMainThreadWrite);
}

