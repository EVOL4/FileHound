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
	//������
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
		//���߳�֪�����������˳�
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
// Method:    Init	 ���ڳ�ʼ������,ע�ⲻҪ��ε���ʼ��
// Returns:   BOOL	 ����ʼ��ʧ��,������FALSE,��ʱӦ��ֹ���в���
// Parameter: TCHAR _tDriveLetter  �̷�
// Parameter: DWORD dwBufLength   ���ڽ������ݵĻ�������С,Խ�����ݴ����Խ��
//************************************
BOOL ChangeJrnl::Init(TCHAR cDriveLetter, DWORD dwBufLength)
{
	m_tDriveLetter = cDriveLetter;
	BOOL bRet = FALSE;

	__try {
		//���뻺�������ڴ�
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer)  __leave;

		//���volume�ľ��,ע���⽫Ҫ��ǰ�����нϸߵ�Ȩ��
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		if (INVALID_HANDLE_VALUE == m_hVolume)  __leave;

		//��������첽IO���̷����,�����ں���(�첽��)�����µ�USN_RECORD
		m_hAsyncVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, TRUE);
		if (INVALID_HANDLE_VALUE == m_hAsyncVolume)  __leave;

		//����ͬ���͵��¼�,�����첽IO���ʱ��������
		m_OverLapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//�����غ��߳�,��m_OverLapped.hEvent����,���첽IO���,�����µ�RECORDʱ,�߳̽�֪ͨ����
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
// Method:    Create ��Ŀ�����usn��־,���Ѵ���,�ú�����������־��MaximumSize��AllocationDelta������Ա
// Returns:   BOOL ������ʧ��,�򲻸ü����κβ���
// Parameter: DWORDLONG MaximumSize	��־������С
// Parameter: DWORDLONG AllocationDelta	��Ҫ����ʱ,����ķ���(���ֽ���),0ΪĬ��
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
// Method:    Delete ɾ�������ϸ���id����־,û�±���øú���,���´�����־���������ݿ�ܺ�ʱ��
// Returns:   BOOL ��ɾ��ʧ��,��ο�LastError��ֵ
// Parameter: DWORDLONG UsnJournalID ��־��id
// Parameter: DWORD DeleteFlags ������USN_DELETE_FLAG_DELETE��USN_DELETE_FLAG_NOTIFY,Ҳ���������ߵ����
//USN_DELETE_FLAG_DELETE����ϵͳ����ɾ������,����ϵͳ��������ʱɾ��,DeviceIoControl����������
//��USN_DELETE_FLAG_NOTIFY����DeviceIoControl����־��ɾ����ŷ���,���ñ�־��������ϵͳɾ����־
//�����߶���set,DeviceIoControl���ᷢ��ɾ����־��֪ͨ,��������־��ɾ����ŷ���
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
// Method:    Query ��ѯ��ǰ��־����Ϣ
// Returns:   BOOL
// Parameter: PUSN_JOURNAL_DATA pUsnJournalData �������ݵ�ָ��,ע�⴫��ָ��ĺϷ���,�����������ڴ�
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
// Method:    OpenVolume				��Ŀ����
// Returns:   HANDLE					��CreateFile����ʧ��,������INVALID_HANDLE_VALUE
// Parameter: TCHAR cDriveLetter		�̷�
// Parameter: DWORD dwAccess			�����Ȩ��
// Parameter: BOOL bAsyncIO				�Ƿ������첽IO
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
		if (!WaitForSingleObject(cj->m_OverLapped.hEvent, INFINITE))  //��÷���WAIT_FAILED 0xFFFFFFFF
		{
			//qt��������(HWND)this->WinId
			//PostMessage()
		}

		//if ��ǰ��δ�봰������,���޷�������Ϣ,�߳��˳�
	}

	return 0;
}