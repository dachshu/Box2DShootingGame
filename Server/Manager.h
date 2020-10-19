#pragma once
#include "stdheader.h"

enum class STATE {
	READY, PLAY, START, END
};

// Main Thread만 write하고 Client thread들은 읽기만 하는 상황을 위한 이벤트 메니저 클래스
// 공유 메모리에 대한 Main Thread의 Write Event를 가지고 있다.
// Client Thread들이 공유 메모리에 대한 작업을 하고 있는지 알기 위한 Event들은
// Client Manager한테 받아온다.
class ShareMemoryManager
{
private:
	HANDLE hMainThreadWrite;

	// Share Memory
	// Ready State 일때 share Memory
	STATE state{ STATE::READY };

	ShareMemoryManager();
public:
	~ShareMemoryManager();
	static ShareMemoryManager* Instance();

	// state 공유 메모리에 대한 함수
	//		main thread가 호출하는 함수
	void ChangeToPlayState();
	//		client thread가 호출하는 함수
	void GetState(HANDLE hClientReadEvent, STATE& s);
	void ChangeState(STATE val);
};
