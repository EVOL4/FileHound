#pragma once
#include<qt_windows.h>
#include <vector>
#include "ChangeJrnl.h"
//����ÿ���������Լ������ݿ�,�����б�Ҫ��������������,�����������ȫ��


class ChangeJrnlDB
{
	
public:
	ChangeJrnlDB();
	~ChangeJrnlDB();

	 DWORD initVolumes();

private:
	std::vector<_TCHAR> m_vDriveLetters; //���е��̷�
	std::vector<ChangeJrnl*> m_vChangeJrnls; //ÿ���̷���Ӧһ��ChangeJournal
	
};

