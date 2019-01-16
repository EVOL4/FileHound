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
	TCHAR m_tDriveLetter;		//��ǰ�̷�
	HANDLE m_hVolume;			//��ǰ�̷��ľ��
	HANDLE m_hAsyncVolume;		//�����첽IO���̷����,������FILE_FLAG_OVERLAPPED��־��õľ�����������첽IO
	HANDLE m_hMonitorThread;	//���ڽ��������ݵ��غ��̵߳ľ��

	PBYTE m_buffer;
	OVERLAPPED m_OverLapped;    //�첽IO ol�ṹ
	static HANDLE OpenVolume(TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO);
	static DWORD  MonitorThread(PVOID lpParameter);

};

