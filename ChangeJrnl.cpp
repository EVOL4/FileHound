#include "ChangeJrnl.h"

ChangeJrnl::ChangeJrnl(TCHAR _tDriveLetter)
{
	m_hVolume = INVALID_HANDLE_VALUE;
	m_hAsyncVolume = INVALID_HANDLE_VALUE;
	m_hMonitorThread = NULL;
	m_OverLapped = { 0 };
	m_buffer = NULL;
}

ChangeJrnl::~ChangeJrnl()
{
	//清理工作
	if (m_hVolume != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hVolume);
	}
	if (m_hAsyncVolume != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hAsyncVolume);
	}
	if (m_buffer != NULL)
	{
		HeapFree(GetProcessHeap(),0, m_buffer);
	}
	if (m_OverLapped.hEvent != NULL)
	{
		//让线程知道我们正在退出
		SetEvent(m_OverLapped.hEvent);
		CloseHandle(m_OverLapped.hEvent);
	}
	if (m_hMonitorThread != NULL)
	{
		WaitForSingleObject(m_hMonitorThread, INFINITE);
		CloseHandle(m_hMonitorThread);
	}
}

//************************************
// Method:    Init	 用于初始化数据,注意不要多次调初始化
// Returns:   BOOL	 若初始化失败,将返回FALSE,此时应终止所有操作
// Parameter: TCHAR _tDriveLetter  盘符
// Parameter: DWORD dwBufLength   用于接收数据的缓冲区大小,越大数据处理得越快
//************************************
BOOL ChangeJrnl::Init(TCHAR cDriveLetter, DWORD dwBufLength)
{
	m_tDriveLetter = cDriveLetter;
	BOOL bRet = FALSE;

	__try {
		//申请缓冲区堆内存
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer)  __leave;

		//获得volume的句柄,注意这将要求当前进程有较高的权限
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		if (INVALID_HANDLE_VALUE == m_hVolume)  __leave;

		//获得用于异步IO的盘符句柄,将用于后面(异步地)处理新的USN_RECORD
		m_hAsyncVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, TRUE);
		if (INVALID_HANDLE_VALUE == m_hAsyncVolume)  __leave;

		//创建同步型的事件,用于异步IO完成时提醒我们
		m_OverLapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//创建守候线程,当m_OverLapped.hEvent授信,即异步IO完成,即有新的RECORD时,线程将通知我们
		m_hMonitorThread = CreateThread(NULL,
			0,
			MonitorThread,
			this,
			0,
			NULL);

		if (NULL == m_hMonitorThread)  __leave;

		BOOL bRet = TRUE;
	}
	__finally {
		if (!bRet) DebugBreak();
	}

	return bRet;
}

//************************************
// Method:    Create 对目标卷创建usn日志,若已存在,该函数将更新日志的MaximumSize和AllocationDelta两个成员
// Returns:   BOOL 若创建失败,则不该继续任何操作
// Parameter: DWORDLONG MaximumSize	日志的最大大小
// Parameter: DWORDLONG AllocationDelta	需要扩大时,扩大的幅度(按字节算),0为默认
//************************************
BOOL ChangeJrnl::Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta)
{
	CREATE_USN_JOURNAL_DATA cujd;
	DWORD dwLength = 0;
	cujd.AllocationDelta = AllocationDelta;
	cujd.MaximumSize = MaximumSize;

	BOOL bRet = DeviceIoControl(m_hVolume,
		FSCTL_CREATE_USN_JOURNAL,
		&cujd,
		sizeof(cujd),
		NULL,
		0,
		&dwLength,
		NULL);

#ifdef DEBUG
	if (!bRet)
	{
		DWORD le = GetLastError();
	}
#endif // DEBUG

	return bRet;
}

//************************************
// Method:    Delete 删除该盘上给定id的日志,没事别调用该函数,重新创建日志并创建数据库很耗时的
// Returns:   BOOL 若删除失败,请参考LastError的值
// Parameter: DWORDLONG UsnJournalID 日志的id
// Parameter: DWORD DeleteFlags 可以是USN_DELETE_FLAG_DELETE或USN_DELETE_FLAG_NOTIFY,也可以是两者的组合
//USN_DELETE_FLAG_DELETE将让系统发出删除操作,但由系统来觉定何时删除,DeviceIoControl将立即返回
//而USN_DELETE_FLAG_NOTIFY会让DeviceIoControl在日志被删除后才返回,但该标志本身不会让系统删除日志
//若两者都被set,DeviceIoControl将会发出删除日志的通知,并且在日志被删除后才返回
//************************************
BOOL ChangeJrnl::Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags)
{
	DELETE_USN_JOURNAL_DATA dujd;
	DWORD dwLength = 0;

	dujd.UsnJournalID = UsnJournalID;
	dujd.DeleteFlags = DeleteFlags;

	BOOL bRet = DeviceIoControl(m_hVolume,
		FSCTL_DELETE_USN_JOURNAL,
		&dujd,
		sizeof(dujd),
		NULL,
		0,
		&dwLength,
		NULL);

#ifdef DEBUG
	if (!bRet)
	{
		DWORD le = GetLastError();
	}
#endif // DEBUG

	return bRet;
}

//************************************
// Method:    Query 查询当前日志的信息
// Returns:   BOOL
// Parameter: PUSN_JOURNAL_DATA pUsnJournalData 接受数据的指针,注意传入指针的合法性,请自行申请内存
//************************************
BOOL ChangeJrnl::Query(PUSN_JOURNAL_DATA pUsnJournalData)
{
	DWORD dwLength = 0;
	BOOL bRet = FALSE;

	__try {
		bRet = DeviceIoControl(m_hVolume,
			FSCTL_QUERY_USN_JOURNAL,
			pUsnJournalData,
			sizeof(*pUsnJournalData),
			NULL,
			0,
			&dwLength,
			NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		pUsnJournalData = NULL;
	}
	return bRet;
}

//************************************
// Method:    OpenVolume				打开目标盘
// Returns:   HANDLE					若CreateFile调用失败,将返回INVALID_HANDLE_VALUE
// Parameter: TCHAR cDriveLetter		盘符
// Parameter: DWORD dwAccess			所需的权限
// Parameter: BOOL bAsyncIO				是否用于异步IO
//************************************
HANDLE ChangeJrnl::OpenVolume(TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO)
{
	TCHAR szVolumePath[16];
	wsprintf(szVolumePath, TEXT("\\\\.\\%c"), cDriveLetter);
	HANDLE hRet = CreateFile(szVolumePath,
		dwAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		(bAsyncIO ? FILE_FLAG_OVERLAPPED : 0),
		NULL);

#ifdef DEBUG
	if (hRet == INVALID_HANDLE_VALUE)
	{
		DWORD reason = GetLastError();
	}
#endif // DEBUG

	return hRet;
}

DWORD ChangeJrnl::MonitorThread(PVOID lpParameter)
{
	if (NULL == lpParameter) return 0;
	ChangeJrnl* cj = (ChangeJrnl*)lpParameter;

	while (TRUE)
	{
		if (!WaitForSingleObject(cj->m_OverLapped.hEvent, INFINITE))  //免得返回WAIT_FAILED 0xFFFFFFFF
		{
			//qt主窗口中(HWND)this->WinId
			//PostMessage()
		}

		//if 当前类未与窗口相连,则无法发送消息,线程退出
	}

	return 0;
}