#pragma once
#include <qt_windows.h>
#include <stdio.h>
#include <tchar.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "match.h"
using namespace std;

#define WM_JOURNALCHANGED (WM_USER + 0x110)
#define NO_WHERE 0
#define IN_FILES 1  //�����ļ�
#define IN_DIRECTORIES 2 //����Ŀ¼


#define QUALITY_OK 0.6

struct HashMapEntry
{
	DWORDLONG ParentFRN;
	unsigned int iOffset;
};

struct MAP_ENTRY
{
	DWORDLONG ParentFRN;
	DWORDLONG Filter;
};

struct USNEntry
{
	DWORDLONG ParentIndex;
	wstring Name;
	USNEntry(wstring aName, DWORDLONG aParentIndex)
	{
		Name = aName;
		ParentIndex = aParentIndex;
	}
	USNEntry()
	{
		ParentIndex = 0;
		Name = wstring();
	}
};

struct IndexedFile
{
	DWORDLONG Index;
	//DWORDLONG ParentIndex;
	DWORDLONG Filter;
	bool operator<(const IndexedFile& i)
	{
		return Index < i.Index;
	}
	IndexedFile()
	{
		Index = 0;
		Filter = 0;
	}
};

struct IndexedDirectory
{
	DWORDLONG Index;
	//DWORDLONG ParentIndex;
	DWORDLONG Filter;
	unsigned int nFiles; //��Ŀ¼�µ��ļ���
	bool operator<(const IndexedDirectory& i)
	{
		return Index < i.Index;
	}
	IndexedDirectory()
	{
		Index = 0;
		Filter = 0;
		nFiles = 0;
	}
};

struct SearchResultFile
{
	wstring Filename; //�ļ���
	wstring Path; //·��
	DWORDLONG Filter;
	score_t MatchQuality; //����
	size_t MatchPos[_MAX_FNAME]; //ƥ���λ��
	SearchResultFile()
	{
		Filename = wstring();
		Path = wstring();
		Filter = 0;
		MatchQuality = 0.0f;
	}
	SearchResultFile(const SearchResultFile &srf)
	{
		Filename = srf.Filename;
		Path = srf.Path;
		Filter = srf.Filter;
		MatchQuality = srf.MatchQuality;
	}
	SearchResultFile(wstring aPath, wstring aFilename, DWORDLONG aFilter, float aMatchQuality = 1)
	{
		Filename = aFilename;
		Path = aPath;
		Filter = aFilter;
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
	int iOffset; //0 when finished
	unsigned int SearchEndedWhere;
	int maxResults;
	SearchResult()
	{
		Query = wstring();
		SearchPath = wstring();
		Results = vector<SearchResultFile>();
		iOffset = 0;
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
	BOOL Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta);
	BOOL Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags);
	BOOL Query(PUSN_JOURNAL_DATA pUsnJournalData);
	VOID SeekToUsn(USN usn, DWORD ReasonMask,DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID);
	VOID PopulateIndex();
	PUSN_RECORD EnumNext();

	_TCHAR GetLetter() { return m_tDriveLetter; }
	//�������
	VOID DropLastResult();
	
	int Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort,  int maxResults);
	
protected /*Functions*/:
	DWORDLONG MakeFilter(wstring* szName);
	BOOL AddDir(DWORDLONG Index, wstring *szName, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64);
	BOOL Add(DWORDLONG Index, wstring *szName, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64);
	DWORDLONG Name2FRN(wstring * strPath);
	USNEntry FRN2Name(DWORDLONG frn);
	void FRN2Path(DWORDLONG frn, wstring* Path);
	INT64 DirOffsetByIndex(DWORDLONG FRNPath);

	void FindInPreviousResults(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, unsigned int  iOffset, int maxResults, int &nResults);
	void FindRecursively(wstring strQuery, const WCHAR* szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> rgsrfResults, int maxResults, int nResults);

	template<class T>
	void FindInJournal(wstring & strQuery, const WCHAR *& szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<T>& rgJournalIndex, vector<SearchResultFile>& rgsrfResults, unsigned int iOffset, int maxResults, int & nResults);



	VOID AbandonMonitor();
	static HANDLE OpenVolume(_TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO);
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

	//���ݿ��Ա
	DWORDLONG m_dlRootIndex;	//��Ŀ¼��FRN
	vector<IndexedFile> rgFiles; //�ļ����ݿ�
	vector<IndexedDirectory> rgDirectories; //�ļ������ݿ�
	SearchResult m_LastResult; //��һ�������Ľ��,���ڻ�������������𽥱���ϸ�����,���Կ�������һ�ε���������м�������


// 	unordered_map<DWORDLONG, MAP_ENTRY> mapIndexToOffset;
// 	vector<wstring> rgszNames

	

};


CDriveIndex* _stdcall CreateIndex(WCHAR cDrive);