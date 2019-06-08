#pragma once
#include <qt_windows.h>
#include <stdio.h>
#include <tchar.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include "../KC/kchashdb.h"
#include "match.h"
using namespace std;
using namespace kyotocabinet;
#define WM_JOURNALCHANGED (WM_USER + 0x110)
#define NO_WHERE 0
#define IN_FILES 1  //搜索文件
#define IN_DIRECTORIES 2 //搜索目录


#define QUALITY_OK (score_t)0.6



struct IndexEntry
{
	DWORDLONG ParentFRN;
	DWORDLONG Filter;
	wstring szName;  //不能用类!! 
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
	wstring Filename; //文件名
	wstring Path; //路径
	DWORDLONG Filter;
	DWORDLONG Index;
	score_t MatchQuality; //分数
	SearchResultFile()
	{
		Filename = wstring();
		Path = wstring();
		Filter = 0;
		Index = 0;
		MatchQuality = 0.0f;
	}
	SearchResultFile(const SearchResultFile &srf)  //拷贝构造函数
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
		return MatchQuality == i.MatchQuality ? Path + Filename < i.Path + i.Filename : MatchQuality > i.MatchQuality; //若得分相同,则按名字排序
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



	//搜索相关
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
	//监控相关
	/*abandoned*/BOOL Add(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64, BOOL IsDir = FALSE);

	DWORDLONG Name2FRN(LPCWSTR strPath);
	VOID NotifyMoreData(DWORD dwDelay);
	VOID AbandonMonitor();
	static DWORD  MonitorThread(PVOID lpParameter);



private:
	_TCHAR m_tDriveLetter;		//当前盘符

	HANDLE m_hVolume;			//当前盘符的句柄
	
	
	

	//用于遍历Journal的数据成员
	READ_USN_JOURNAL_DATA_V0 m_rujd; //注意版本..win10和win7下 v1不行的
	PBYTE m_buffer;					// 接收数据的Block,调用DeviceIoControl后系统将向里填充  下一个USN号 + USN_RECORD*n 这样的数据
	DWORD m_dwBufferLength;			//Block里的数据长度
	PUSN_RECORD m_usnCurrent;       //当前的USN_RECORD

	//用于异步读取数据并向窗口通知的成员
	HANDLE m_hAsyncVolume;		//用于异步IO的盘符句柄,必须以FILE_FLAG_OVERLAPPED标志获得的句柄才能用于异步IO
	OVERLAPPED m_OverLapped;    //异步IO OverLapped结构
	DWORD m_dwDelay;			//每次通知数据更新之前休眠一会
	HWND m_MainWindow;			//接收消息的窗口,当有新的数据时,窗口将接收WM_JOURNALCHANGED消息
	HANDLE m_hMonitorThread;	//用于接收新数据的守候线程的句柄
	USN m_AsyncUsn;				//
	//数据库成员
	DWORDLONG m_dlRootIndex;	//根目录的FRN
	TreeDB* m_db;
	SearchResult m_LastResult; //上一次搜索的结果,由于会出现输入数据逐渐变详细的清况,所以可以在上一次的搜索结果中继续搜索


 	unordered_map<DWORDLONG , DWORDLONG> mapIndexToOffset;
	vector<IndexEntry> rgIndexes;

	LPSTR m_DbName;
	CRITICAL_SECTION m_cs;

};

CDriveIndex* _stdcall CreateIndex(WCHAR cDrive);