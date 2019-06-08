#include <tchar.h>
#include "MyDB.h"


CRITICAL_SECTION _cs;
CRITICAL_SECTION _cs2;


MyDB::MyDB()
{
	InitializeCriticalSection(&_cs);
	InitializeCriticalSection(&_cs2);
}


MyDB::~MyDB()
{
	EnterCriticalSection(&_cs2);
	for (auto i:m_vChangeJrnls)
	{
		if (i)
		{
			delete i;
			i = nullptr;
		}
		
	}
	LeaveCriticalSection(&_cs2);

	DeleteCriticalSection(&_cs);
	DeleteCriticalSection(&_cs2);

}

DWORD MyDB::initVolumes()
{
	DWORD dwRet = 0;
	DWORD bitMap = GetLogicalDrives();
	//msdn上说靠lable和盘符来识别硬盘都有一定不准确性,系统靠的是GUID path来识别某个盘
	//如 C:的guid path可以是"\\?\Volume{26a21bda-a627-11d7-9931-806e6f6e6963}\"
	//同时CreateFile也可以传入硬盘的GUID path来访问该盘
	//而FindFirstVolume
	// 	FindNextVolume
	// 	FindVolumeClose
	//这三个函数可以枚举出所有盘的GUID path
	//详情可见https://docs.microsoft.com/zh-cn/windows/desktop/FileIO/displaying-volume-paths
	//但是很麻烦而且没必要,我就这样了

	for (unsigned int i = 0; i < 26; i++)
	{
		if (bitMap >> i & (BYTE)1)
		{
			_TCHAR drivePath[] = TEXT( "C:\\" );
			_TCHAR lpFileSystemNameBuffer[10];
			_TCHAR driveLetter = 'A' + i;
			drivePath[0] = driveLetter;
			if (GetVolumeInformation(
				drivePath,
				NULL,
				MAX_PATH,
				NULL,
				NULL,
				NULL,
				lpFileSystemNameBuffer,
				MAX_PATH)
				)
			{
				if (!_tcscmp(lpFileSystemNameBuffer, TEXT("NTFS")))//是NTFS吗
				{
					dwRet++;
					m_vDriveLetters.push_back(driveLetter);
				}
			}
			else
			{
				WCHAR error[MAX_PATH] = { 0 };
				wsprintf(error, TEXT("GetVolumeInformation error: %x"), GetLastError());
				OutputDebugString(error);
				DebugBreak();
			}
		}
	}
	return  GetLastError();
	
}

BOOL MyDB::initJournal()
{
	BOOL bRet = FALSE;
	for (auto i:m_vDriveLetters)
	{
		CDriveIndex* pDI = new CDriveIndex();
		bRet = pDI->Init(i);
		m_vChangeJrnls.push_back(pDI);
	}
	return bRet;
}

VOID MyDB::Find(wstring &strQuery, vector<SearchResultFile> &rgsrfResults, int maxResults)
{
	vector<HANDLE> hThreads;
	
	if (!TryEnterCriticalSection(&_cs2))
		return;
	
	
	for (auto i:m_vChangeJrnls)
	{
		Item* pItem = new Item();
		pItem->pDI = i;
		pItem->pResult = &rgsrfResults;
		pItem->pSzQuery = &strQuery;
		pItem->nMax = maxResults;
		HANDLE hSeeker = CreateThread(NULL,
			0,
			Seeker,
			pItem,
			0,
			NULL);
		if (hSeeker!=NULL)
		{
			hThreads.push_back(hSeeker);
		}
	}

	DWORD dwRet = WaitForMultipleObjects(hThreads.size(),
		hThreads.data(),
		TRUE, INFINITE);

	//assert(WAIT_OBJECT_0 == dwRet);


	for (auto handle: hThreads)
	{
		CloseHandle(handle);
	}

	LeaveCriticalSection(&_cs2);

}

DWORD WINAPI MyDB::WorkerThread(PVOID parameter)
{

	return 0;
}

DWORD WINAPI MyDB::Seeker(PVOID parameter)
{
	
	if (!parameter) return 0;
	
	Item* pItem = (Item*)parameter;

	CDriveIndex* pDrive = pItem->pDI;
	vector<SearchResultFile>* pResults = pItem->pResult;
	wstring* pQuery = pItem->pSzQuery;


	vector<SearchResultFile> myResult;


	pDrive->Find(pQuery, NULL, &myResult, FALSE, pItem->nMax);


	EnterCriticalSection(&_cs);
	for (auto i: myResult)
	{
		pResults->push_back(i);
	}
	LeaveCriticalSection(&_cs);
	OutputDebugString(L"Seeker Exit\r\n");
	delete parameter;

}
