#pragma once
#include <qt_windows.h>


#define WM_JOURNALCHANGED (WM_USER + 0x110)

class ChangeJrnl
{
public:
	ChangeJrnl(TCHAR);
	~ChangeJrnl();

	BOOL Init(TCHAR cDriveLetter, DWORD dwBufLength);
	BOOL Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta);
	BOOL Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags);
	BOOL Query(PUSN_JOURNAL_DATA pUsnJournalData);

	


private:
	TCHAR m_tDriveLetter;		//当前盘符
	HANDLE m_hVolume;			//当前盘符的句柄
	HANDLE m_hAsyncVolume;		//用于异步IO的盘符句柄,必须以FILE_FLAG_OVERLAPPED标志获得的句柄才能用于异步IO
	HANDLE m_hMonitorThread;	//用于接收新数据的守候线程的句柄

	PBYTE m_buffer;
	OVERLAPPED m_OverLapped;    //异步IO ol结构
	static HANDLE OpenVolume(TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO);
	static DWORD  MonitorThread(PVOID lpParameter);

};

