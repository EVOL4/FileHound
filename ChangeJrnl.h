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
#define IN_FILES 1  //搜索文件
#define IN_DIRECTORIES 2 //搜索目录


#define QUALITY_OK 0.6



struct IndexEntry
{
	DWORDLONG ParentFRN;
	DWORDLONG Filter;
	wstring szName;  //不能用类!! 
};  



struct SearchResultFile
{
	wstring Filename; //文件名
	wstring Path; //路径
	DWORDLONG Filter;
	score_t MatchQuality; //分数
	size_t MatchPos[_MAX_FNAME]; //匹配的位置
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
		return MatchQuality == i.MatchQuality ? Path + Filename < i.Path + i.Filename : MatchQuality > i.MatchQuality; //若得分相同,则按名字排序
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
	//搜索相关
	VOID DropLastResult();
	
	int Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort,  int maxResults);
	
protected /*Functions*/:
	DWORDLONG MakeFilter(wstring* szName);
	BOOL Add(DWORDLONG Index, wstring *szName, DWORDLONG ParentIndex, DWORDLONG Filter = 0ui64);
	DWORDLONG Name2FRN(wstring& strPath);



	void FindInPreviousResults(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, unsigned int  iOffset, int maxResults, int &nResults);
	

	void FindInJournal(wstring & strQuery, const WCHAR *& szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile>& rgsrfResults,int maxResults, int & nResults);



	VOID AbandonMonitor();
	static HANDLE OpenVolume(_TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO);
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

	//数据库成员
	DWORDLONG m_dlRootIndex;	//根目录的FRN

	SearchResult m_LastResult; //上一次搜索的结果,由于会出现输入数据逐渐变详细的清况,所以可以在上一次的搜索结果中继续搜索


 	unordered_map<DWORDLONG , DWORDLONG> mapIndexToOffset;
	vector<IndexEntry> rgIndexes;

	void GetPath(IndexEntry& i, wstring & szFullPath);

};

CDriveIndex* _stdcall CreateIndex(WCHAR cDrive);