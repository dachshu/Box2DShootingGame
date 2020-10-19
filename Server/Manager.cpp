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

// Client���� MAXCLIENT�� �Ǿ��� �� ����
// ->listen socket�� �ݾƼ� ���� Ŭ���̾�Ʈ�� ������ �ʴ´�
//   BUT!!! �߰��� �������� ���� �ִ�.
//   Event�� �߰��� ���� �� �ִ�.
//   -> �̿� ���� ó������
//				DeadLock ����ϸ鼭 ���α׷� ������
void ShareMemoryManager::ChangeToPlayState()
{
	ResetEvent(hMainThreadWrite);
	
	while (true) {
		// ClientShareMemoryReadEvent ����Ʈ, ���ڸ� �����´�.
		ClientDataManager* clientMgr = ClientDataManager::Instance();
		int nClients, retval;
		HANDLE* hClientReadEvents = clientMgr->GetClientReadEvents(nClients);
		retval = WaitForMultipleObjects(nClients, hClientReadEvents, TRUE, INFINITE);
		// �̶� �߰��� Client�� ���� �� �ֱ� ������
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
		// ClientShareMemoryReadEvent ����Ʈ, ���ڸ� �����´�.
		ClientDataManager* clientMgr = ClientDataManager::Instance();
		int nClients, retval;
		HANDLE* hClientReadEvents = clientMgr->GetClientReadEvents(nClients);
		retval = WaitForMultipleObjects(nClients, hClientReadEvents, TRUE, INFINITE);
		// �̶� �߰��� Client�� ���� �� �ֱ� ������
		if (retval == WAIT_OBJECT_0) break;
	}
	state = val;
	SetEvent(hMainThreadWrite);
}

