#pragma once
#include <qt_windows.h>
#include <stdio.h>
#include <tchar.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../KC/kchashdb.h"
#include "match.h"
using namespace std;
using namespace kyotocabinet;
#define WM_JOURNALCHANGED (WM_USER + 0x110)
#define NO_WHERE 0
#define IN_FILES 1  //�����ļ�
#define IN_DIRECTORIES 2 //����Ŀ¼


#define QUALITY_OK (score_t)0.6



struct IndexEntry
{
	DWORDLONG ParentFRN;
	DWORDLONG Filter;
	wstring szName;  //��������!! 
};  

struct DBEntry
{
	DWORDLONG ParentFRN;
	DWORDLONG Filter;
	CHAR szName[1];
};

struct CUR
{
	DWORDLONG UsnJournalID;
	USN CurUsn;
};

struct SearchResultFile
{
	wstring Filename; //�ļ���
	wstring Path; //·��
	DWORDLONG Filter;
	DWORDLONG Index;
	score_t MatchQuality; //����
	SearchResultFile()
	{
		Filename = wstring();
		Path = wstring();
		Filter = 0;
		Index = 0;
		MatchQuality = 0.0f;
	}
	SearchResultFile(const SearchResultFile &srf)  //�������캯��
	{
		Filename = srf.Filename;
		Path = srf.Path;
		Filter = srf.Filter;
		Index = srf.Index;
		MatchQuality = srf.MatchQuality;
	}
	SearchResultFile (wstring aFilename, wstring aPath, DWORDLONG aFilter, DWORDLONG aIndex, score_t aMatchQuality = 1)
	{
		Filename = aFilename;
		Path = aPath;
		Filter = aFilter;
		Index = aIndex;
		MatchQuality = aMatchQuality;
	}
	bool operator<(const SearchResultFile& i)
	{
		return MatchQuality == i.MatchQuality ? Path + Filename < i.Path + i.Filename : MatchQuality > i.MatchQuality; //���÷���ͬ,����������
	}
};


struct SearchResult
{
	wstring Query;
	wstring SearchPath;
	vector<SearchResultFile> Results;
	unsigned int SearchEndedWhere;
	int maxResults;
	DWORDLONG iOffSet;
	SearchResult()
	{
		Query = wstring();
		SearchPath = wstring();
		iOffSet = 0;
		Results = vector<SearchResultFile>();
		SearchEndedWhere = NO_WHERE;
		maxResults = -1;
	}
};

class CDriveIndex
{
public /*Functions*/:
	CDriveIndex();
	~CDriveIndex();
	BOOL Init(_TCHAR cDriveLetter, DWORD dwBufLength = 0x10000);
	VOID InitialzeForMonitoring();
	void ProcessAvailableRecords();

	//USN
	BOOL Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta);
	BOOL Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags);
	BOOL Query(PUSN_JOURNAL_DATA pUsnJournalData);
	VOID SeekToUsn(USN usn, DWORD ReasonMask,DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID);
	static HANDLE OpenVolume(_TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO);

	//DataBase
	/*abandoned*/VOID PopulateIndex();

	BOOL Add_DB(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64);
	BOOL Change_DB(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64);
	BOOL Delete_DB(DWORDLONG Index);
	VOID PopulateIndex2DB();



	//�������
	VOID DropLastResult();
	int Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort,  int maxResults);
	void FindInPreviousResults(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, int maxResults, int &nResults);
	void FindInJournal(wstring & strQuery, const WCHAR *& szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile>& rgsrfResults, int maxResults, int & nResults);
	void GetPath_DB(DWORDLONG index, wstring & szPath);
	/*abandoned*/void GetPath(IndexEntry& i, wstring & szFullPath);
	void FindInDB(wstring &strQuery, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, int maxResults, int &nResults);
	PUSN_RECORD EnumNext();
	DWORDLONG MakeFilter(LPCWSTR szName, size_t szLength);

	_TCHAR GetLetter() { return m_tDriveLetter; }

protected /*Functions*/:
	//������
	/*abandoned*/BOOL Add(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64, BOOL IsDir = FALSE);

	DWORDLONG Name2FRN(LPCWSTR strPath);
	VOID NotifyMoreData(DWORD dwDelay);
	VOID AbandonMonitor();
	static DWORD  MonitorThread(PVOID lpParameter);



private:
	_TCHAR m_tDriveLetter;		//��ǰ�̷�

	HANDLE m_hVolume;			//��ǰ�̷��ľ��
	
	
	

	//���ڱ���Journal�����ݳ�Ա
	READ_USN_JOURNAL_DATA_V0 m_rujd; //ע��汾..win10��win7�� v1���е�
	PBYTE m_buffer;					// �������ݵ�Block,����DeviceIoControl��ϵͳ���������  ��һ��USN�� + USN_RECORD*n ����������
	DWORD m_dwBufferLength;			//Block������ݳ���
	PUSN_RECORD m_usnCurrent;       //��ǰ��USN_RECORD

	//�����첽��ȡ���ݲ��򴰿�֪ͨ�ĳ�Ա
	HANDLE m_hAsyncVolume;		//�����첽IO���̷����,������FILE_FLAG_OVERLAPPED��־��õľ�����������첽IO
	OVERLAPPED m_OverLapped;    //�첽IO OverLapped�ṹ
	DWORD m_dwDelay;			//ÿ��֪ͨ���ݸ���֮ǰ����һ��
	HWND m_MainWindow;			//������Ϣ�Ĵ���,�����µ�����ʱ,���ڽ�����WM_JOURNALCHANGED��Ϣ
	HANDLE m_hMonitorThread;	//���ڽ��������ݵ��غ��̵߳ľ��
	USN m_AsyncUsn;				//
	//���ݿ��Ա
	DWORDLONG m_dlRootIndex;	//��Ŀ¼��FRN
	TreeDB* m_db;
	SearchResult m_LastResult; //��һ�������Ľ��,���ڻ�������������𽥱���ϸ�����,���Կ�������һ�ε���������м�������


 	unordered_map<DWORDLONG , DWORDLONG> mapIndexToOffset;
	vector<IndexEntry> rgIndexes;

	LPSTR m_DbName;

};

CDriveIndex* _stdcall CreateIndex(WCHAR cDrive);