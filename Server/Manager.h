#pragma once
#include "stdheader.h"

enum class STATE {
	READY, PLAY, START, END
};

// Main Thread�� write�ϰ� Client thread���� �б⸸ �ϴ� ��Ȳ�� ���� �̺�Ʈ �޴��� Ŭ����
// ���� �޸𸮿� ���� Main Thread�� Write Event�� ������ �ִ�.
// Client Thread���� ���� �޸𸮿� ���� �۾��� �ϰ� �ִ��� �˱� ���� Event����
// Client Manager���� �޾ƿ´�.
class ShareMemoryManager
{
private:
	HANDLE hMainThreadWrite;

	// Share Memory
	// Ready State �϶� share Memory
	STATE state{ STATE::READY };

	ShareMemoryManager();
public:
	~ShareMemoryManager();
	static ShareMemoryManager* Instance();

	// state ���� �޸𸮿� ���� �Լ�
	//		main thread�� ȣ���ϴ� �Լ�
	void ChangeToPlayState();
	//		client thread�� ȣ���ϴ� �Լ�
	void GetState(HANDLE hClientReadEvent, STATE& s);
	void ChangeState(STATE val);
};
