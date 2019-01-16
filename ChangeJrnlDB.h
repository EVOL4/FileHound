#pragma once
#include<qt_windows.h>
#include <vector>
#include "ChangeJrnl.h"
//由于每个卷都有着自己的数据库,所以有必要按卷来管理数据,本类则负责管理全卷


class ChangeJrnlDB
{
	
public:
	ChangeJrnlDB();
	~ChangeJrnlDB();

	 DWORD initVolumes();

private:
	std::vector<_TCHAR> m_vDriveLetters; //所有的盘符
	std::vector<ChangeJrnl*> m_vChangeJrnls; //每个盘符对应一个ChangeJournal
	
};

