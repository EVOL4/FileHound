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
	 BOOL initJournal();
	 VOID Find(wstring &strQuery,vector<SearchResultFile> &rgsrfResults, int maxResults);

	 static DWORD WINAPI WorkerThread(PVOID parameter);
	 static DWORD WINAPI Seeker(PVOID parameter);
private:

	std::vector<_TCHAR> m_vDriveLetters; //所有的盘符
	std::vector<CDriveIndex*> m_vChangeJrnls; //每个盘符对应一个ChangeJournal
	
};

struct Item
{
	CDriveIndex* pDI;
	wstring* pSzQuery;
	vector<SearchResultFile>* pResult;
	DWORD nMax;
	Item()
	{
		pDI = NULL;
		pSzQuery = NULL;
		pResult = NULL;
		nMax = -1;
	}
};


