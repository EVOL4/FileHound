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
	std::vector<_TCHAR> m_vDriveLetters; //���е��̷�
	std::vector<CDriveIndex*> m_vChangeJrnls; //ÿ���̷���Ӧһ��ChangeJournal
	
};

