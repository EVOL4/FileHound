#pragma once
#include<qt_windows.h>
#include <vector>
#include "ChangeJrnl.h"



class MyDB
{
	
public:
	MyDB();
	~MyDB();

	 DWORD initVolumes();

private:
	std::vector<_TCHAR> m_vDriveLetters; //所有的盘符
	std::vector<CDriveIndex*> m_vChangeJrnls; //每个盘符对应一个ChangeJournal
	
};

