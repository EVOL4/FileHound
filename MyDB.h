#pragma once
#include<qt_windows.h>
#include <QObject>
#include <vector>
#include "ChangeJrnl.h"
#include "../KC/kchashdb.h"

using namespace std;
using namespace kyotocabinet;

class MyDB: public QObject
{
	Q_OBJECT

public:
	MyDB();
	~MyDB();

	 DWORD initVolumes();

private:
	std::vector<_TCHAR> m_vDriveLetters; //所有的盘符
	std::vector<CDriveIndex*> m_vChangeJrnls; //每个盘符对应一个ChangeJournal
	
};

