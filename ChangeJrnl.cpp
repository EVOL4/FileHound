#include <algorithm>
#include <locale>
#include "ChangeJrnl.h"
#include "Pinyin.h"

// Exported function to create the index of a drive
CDriveIndex* _stdcall CreateIndex(WCHAR cDrive)
{
	CDriveIndex *pDI = new CDriveIndex();
	pDI->Init(cDrive);
	pDI->PopulateIndex();
	return pDI;
}

CDriveIndex::CDriveIndex()
{

	m_tDriveLetter = 'A';
	m_hVolume = INVALID_HANDLE_VALUE;

	m_rujd = { 0 };
	m_buffer = NULL;
	m_dwBufferLength = 0;
	m_usnCurrent = NULL;

	m_hAsyncVolume = INVALID_HANDLE_VALUE;
	m_OverLapped = { 0 };
	m_dwDelay = 0;
	m_MainWindow = NULL;
	m_hMonitorThread = NULL;
}

CDriveIndex::~CDriveIndex()
{
	//������
	m_MainWindow = NULL;

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
		HeapFree(GetProcessHeap(), 0, m_buffer);
	}
	if (m_OverLapped.hEvent != NULL)
	{
		//���߳�֪�����������˳�
		AbandonMonitor();
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
// Parameter: DWORD dwBufLength   ���ڽ������ݵĻ�������С,����ڴ潫�������������Ǵ������ݵ��ٶ�
//************************************
BOOL CDriveIndex::Init(_TCHAR cDriveLetter, DWORD dwBufLength)
{
	dwBufLength += sizeof(DWORDLONG); //һ��USN�Ĵ�С
	m_tDriveLetter = cDriveLetter;
	BOOL bRet = FALSE;

	__try {
		//���뻺�������ڴ�
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer)  __leave;

		//���volume�ľ��,ע���⽫Ҫ��ǰ�����нϸߵ�Ȩ��
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		int aa = GetLastError();
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

		bRet = TRUE;
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
BOOL CDriveIndex::Create(DWORDLONG MaximumSize, DWORDLONG AllocationDelta)
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
BOOL CDriveIndex::Delete(DWORDLONG UsnJournalID, DWORD DeleteFlags)
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


	return bRet;
}

//************************************
// Method:    Query ��ѯ��ǰ��־����Ϣ
// Returns:   BOOL
// Parameter: PUSN_JOURNAL_DATA pUsnJournalData �������ݵ�ָ��,ע�⴫��ָ��ĺϷ���,�����������ڴ�
//************************************
BOOL CDriveIndex::Query(PUSN_JOURNAL_DATA pUsnJournalData)
{
	DWORD dwLength = 0;
	BOOL bRet = FALSE;

	bRet = DeviceIoControl(m_hVolume,
		FSCTL_QUERY_USN_JOURNAL,
		NULL,
		0,
		pUsnJournalData,
		sizeof(*pUsnJournalData),
		&dwLength,
		NULL);

	return bRet;
}

//************************************
// Method:    DropLastResult  m_LastResult�����û����������ڼ���Ч
//************************************
VOID CDriveIndex::DropLastResult()
{
	m_LastResult = SearchResult();
}

void CDriveIndex::FindInPreviousResults(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, unsigned int iOffset, int maxResults, int &nResults)
{
}

void CDriveIndex::GetPath(IndexEntry& i , wstring & szFullPath)
{
	if (szFullPath.capacity() < _MAX_PATH)
	{
		szFullPath.reserve(_MAX_PATH);
	}

	szFullPath = wstring(i.szName);
	DWORDLONG Index = i.ParentFRN;

	do {
		DWORDLONG offset;
		if (mapIndexToOffset.find(Index) == mapIndexToOffset.end())
			break;
		offset = mapIndexToOffset[Index];
		szFullPath = wstring(rgIndexes[offset].szName) + ((szFullPath.length() != 0) ? TEXT("\\") : TEXT("")) + szFullPath;
		Index = rgIndexes[offset].ParentFRN;
	} while (Index != 0);
}


void CDriveIndex::FindInJournal(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, int maxResults, int &nResults)
{

	for (auto j : rgIndexes)
	{
		SearchResultFile result;
		DWORDLONG Length = (j.Filter & 0xE000000000000000ui64) >> 61ui64;
		DWORDLONG Filter = j.Filter & 0x1FFFFFFFFFFFFFFFui64; //ȥ������λ

		if (!((Filter & QueryFilter) == QueryFilter && QueryLength <= Length))
			continue;

		score_t MatchQuality;
		wstring szFileName(j.szName);
		/*
		if strQuery��������
			szFileName��תӢ��
		else
		*/
		transform(szFileName.begin(), szFileName.end(), szFileName.begin(), pinyinFirstLetter);  

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), result.MatchPos);   //ģ��ƥ��ռ��ʱ����� 

		if (MatchQuality > QUALITY_OK)
		{
			nResults++;

			if (maxResults != -1 && nResults > maxResults) //����������
			{
				nResults = -1;
				break;
			}
			result.Filename = j.szName;
			GetPath(j, result.Path);
			BOOL bFound = TRUE;

			if (strQueryPath != NULL)
			{
				wstring strPathLower(result.Path);
				transform(strPathLower.begin(), strPathLower.end(), strPathLower.begin(), tolower);
				bFound = strPathLower.find(*strQueryPath) != -1;
			}

			if (bFound)
			{
				//split path
				WCHAR szDrive[_MAX_DRIVE];
				WCHAR szPath[_MAX_PATH];
				WCHAR szName[_MAX_FNAME];
				WCHAR szExt[_MAX_EXT];
				_wsplitpath(result.Path.c_str(), szDrive, szPath, szName, szExt);
				result.Path = wstring(szDrive) + wstring(szPath);
				result.Filter = j.Filter;
				result.MatchQuality = MatchQuality;
				rgsrfResults.insert(rgsrfResults.end(), result);
			}
		}
	}
	
}

int CDriveIndex::Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort, int maxResults)
{
	//Ĭ�������ļ�
	unsigned int SearchWhere = IN_FILES;
	//Ŀ����vector�е�λ��
	unsigned int iOffset = 0;
	//���������ϴ���ͬ,��ֱ�����ϴεĽ��,������������
	BOOL bSkipSearch = FALSE;

	//�������Ľ������,-1��ʾ�ѵ��ĳ���������
	int nResults = 0;

	//����Ϊ��,����
	if (strQuery->length() == 0)
	{
		m_LastResult.Query = wstring(TEXT(""));
		m_LastResult.Results = vector<SearchResultFile>();
		return nResults;
	}

	if (strQueryPath)
	{
		//�������·���Ƿ����ڵ�ǰ��
		WCHAR szDrive[_MAX_PATH] = { 0 };
		_wsplitpath(strQueryPath->c_str(), szDrive, NULL, NULL, NULL);
		for (unsigned int j = 0; j != _MAX_DRIVE; j++)
			szDrive[j] = toupper(szDrive[j]);
		if (0 != wstring(szDrive).compare(wstring(1, toupper(m_tDriveLetter))))
			return 0;
	}

	wstring strQueryLower(*strQuery);
	transform(strQueryLower.begin(), strQueryLower.end(), strQueryLower.begin(), tolower);
	const WCHAR* szQueryLower = strQueryLower.c_str();

	wstring strQueryPathLower(strQueryPath != NULL ? *strQueryPath : TEXT(""));
	transform(strQueryPathLower.begin(), strQueryPathLower.end(), strQueryPathLower.begin(), tolower);
	wstring* pstrQueryPathLower = (strQueryPath != NULL && strQueryPathLower.length() > 0) ? &strQueryPathLower : NULL;

	//������·�����ϴεĲ�ͬ,���ϴεĽ���͸÷���
	if (!(strQueryPath != NULL && (m_LastResult.maxResults == -1 || m_LastResult.iOffset == 0) && (m_LastResult.SearchPath.length() == 0 || strQueryPathLower.find(m_LastResult.SearchPath) == 0)))
		m_LastResult = SearchResult();

	//��������������Ӧ��filter
	DWORDLONG QueryFilter = MakeFilter(&strQueryLower);
	DWORDLONG QueryLength = (QueryFilter & 0xE000000000000000ui64) >> 61ui64;  //���㳤��
	QueryFilter = QueryFilter & 0x1FFFFFFFFFFFFFFFui64;  //����λ�Ϳ��Բ�Ҫ��

	//���ϴεĲ�ѯ��ͬ,���ϴεĲ�ѯ�н��,��ʹ���ϴεĽ��
	if (strQueryLower.compare(m_LastResult.Query) == 0 && m_LastResult.Results.size() > 0 && (m_LastResult.SearchEndedWhere == NO_WHERE && iOffset != 1))
	{
		//�����ϴβ�ѯ��λ��
		SearchWhere = m_LastResult.SearchEndedWhere;
		iOffset = m_LastResult.iOffset;
		bSkipSearch = TRUE;
		for (int i = 0; i != m_LastResult.Results.size(); i++)  //todo:���ϴεĲ�ѯ�����,ĳЩ�ļ����޸�/ɾ��/������,��ôlastresult����Ч��,������Ҫ����������ʧʱ,����ϴε��������
		{
			BOOL bFound = TRUE;
			if (pstrQueryPathLower)
			{
				wstring strPathLower(m_LastResult.Results[i].Path);
				transform(strPathLower.begin(), strPathLower.end(), strPathLower.begin(), tolower);
				bFound = (strPathLower.find(strQueryPathLower) != wstring::npos);
			}
			if (bFound)
			{
				nResults++;
				if (maxResults != -1 && nResults > maxResults) //��������ָ���Ľ������,��ֹͣ
				{
					nResults = -1;  //��������,˵�������ϴ��򵽴��˽�����޶�û���ѳ����з����������ļ�,������Ҫ�����ϴε�λ������
					SearchWhere = NO_WHERE;  //�ı�һЩ����,�������̼�������
					iOffset = 1;
					break;
				}
				rgsrfResults->insert(rgsrfResults->end(), m_LastResult.Results[i]);
			}
		}
		//�ϴ��򵽴��˽�����޶�û���ѳ����з����������ļ�,������Ҫ�����ϴε�λ������
		if (m_LastResult.maxResults != -1 && m_LastResult.SearchEndedWhere != NO_WHERE && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}
	//��εĲ�ѯ���ϴεĸ���ȷ,��ôʹ���ϴεĽ������һ������
	else if (strQueryLower.find(m_LastResult.Query) != -1 && m_LastResult.Results.size() > 0)  //todo:����ϴε������������ȫ,����ε�Ҳ�϶�����ȫ
	{
		//�����ϴβ�ѯ��λ��
		SearchWhere = m_LastResult.SearchEndedWhere;
		iOffset = m_LastResult.iOffset;
		bSkipSearch = TRUE;

		FindInPreviousResults(*strQuery, szQueryLower, QueryFilter, QueryLength, pstrQueryPathLower, *rgsrfResults, 0, maxResults, nResults);

		//�ϴ��򵽴��˽�����޶�û���ѳ����з����������ļ�,������Ҫ�����ϴε�λ������
		if (m_LastResult.maxResults != -1 && m_LastResult.SearchEndedWhere != NO_WHERE && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}

	//��ʼ����



	if (SearchWhere == IN_FILES && !bSkipSearch) 
	{
		//�����ݿ�������
		FindInJournal(*strQuery, szQueryLower, QueryFilter, QueryLength, (strQueryPath != NULL ? &strQueryPathLower : NULL),  *rgsrfResults, maxResults, nResults);
		//If we found the maximum number of results in the file index we stop here
		if (maxResults != -1 && nResults == -1)
			iOffset++; //Start with next entry on the next incremental search
		else //Search isn't limited or not all results found yet, continue in directory index
		{
			SearchWhere = IN_DIRECTORIES;
			iOffset = 0;
		}
	}
}

//************************************
// Method:    SeekToUsn  �ú���Ӧ�ڶ�ȡ����ǰ����, ����m_rujd�ṹ,���Զ�λ��ȡ����ʼλ��,���˶�ȡ�Ľ��,������ȡ�Ľ����ʱ����
// Parameter: USN usn	��ʼ��ȡ���ݵĵ�usn��,ÿ��usn��Ӧ��һ��USN_RECORD�ṹ,������0,
// Parameter: DWORD ReasonMask	��Ϊ0xFFFFFFFF,�����е�RECORD�������ظ�����
// Parameter: DWORD ReturnOnlyOnClose ֻ����reason����USN_REASON_CLOSE�Ľ��,����ζ������ֻ�����ļ������������ս��
// Parameter: DWORDLONG UsnJournalID ��־id,�������Ŷ
//************************************
VOID CDriveIndex::SeekToUsn(USN usn, DWORD ReasonMask, DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID)
{
	m_rujd.StartUsn = usn;
	m_rujd.ReasonMask = (ReturnOnlyOnClose != 0) ? (ReasonMask | USN_REASON_CLOSE) : ReasonMask; //��ReturnOnlyOnClose=true,��ReasonMask��Ҫָ��USN_REASON_CLOSE��־
	m_rujd.ReturnOnlyOnClose = ReturnOnlyOnClose;
	m_rujd.Timeout = 0;
	m_rujd.BytesToWaitFor = 0;
	m_rujd.UsnJournalID = UsnJournalID;

	m_dwBufferLength = 0;
	m_usnCurrent = NULL;
}

//************************************
// Method:    PopulateIndex  ��ʼ�����ݿ�,����һ������,��ԭ�ȵ����ݿⲻ����Чʱ����
//************************************
VOID CDriveIndex::PopulateIndex()
{
	do
	{
		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//��ø�Ŀ¼��FRN,����Ŀ¼�������ݿ�
		WCHAR szRoot[_MAX_DRIVE];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);

		wstring szTemp(szRoot);
		DWORDLONG IndexRoot = Name2FRN(szTemp);

		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add(IndexRoot, &wstring(szRoot), 0);
		m_dlRootIndex = IndexRoot;

		//��һ�α���,ͳ�������ļ�/�ļ���,������Ԥ�����ռ�
		MFT_ENUM_DATA_V0 med; //v1�ǲ��е�
		med.StartFileReferenceNumber = 0; //��һ����Ҫ����Ϊ0��ö�������ļ�,֮���������Ϊ��Ҫ����ʼλ��
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //ö�ٽ�����������ļ�,֮�������ĵȳ�ʼ����Ϻ��ٴ���

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //ÿ�δ���64k������

		
		DWORD cb;
		UINT numFiles = 1; //�����и���Ŀ¼c:
		while (FALSE != DeviceIoControl(m_hVolume, //����������Ҫ�����Զ�/дȨ�޴��̷�����
			FSCTL_ENUM_USN_DATA,
			&med,
			sizeof(med),
			BufferData,
			sizeof(BufferData),
			&cb,
			NULL))
		{
			PUSN_RECORD pRecord = (PUSN_RECORD)&BufferData[sizeof(USN)];

			while ((PBYTE)pRecord < (BufferData + cb))
			{
				if (pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE || pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					numFiles++;
				}
				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		//�������,������Ԥ�������ʵĿռ�,�������ÿ�β���ʱ��Ҫ���������ڴ�
		rgIndexes.reserve(numFiles); //25m
		mapIndexToOffset.reserve(numFiles); //10m
		//�ڶ��α���,��ʼ���ݵĲ���
	

		med.StartFileReferenceNumber = 0; 

		while (FALSE != DeviceIoControl(m_hVolume,
			FSCTL_ENUM_USN_DATA,
			&med, sizeof(med),
			BufferData,
			sizeof(BufferData),
			&cb,
			NULL))
		{
			PUSN_RECORD pRecord = (PUSN_RECORD)&BufferData[sizeof(USN)];

			while ((PBYTE)pRecord < (BufferData + cb))
			{
				LPCWSTR pName = (LPCWSTR)((PBYTE)pRecord + pRecord->FileNameOffset);
				DWORD dwLength = pRecord->FileNameLength / sizeof(WCHAR);
				wstring szName(pName, dwLength); //�ݹٷ��ĵ���˵,��������pRecord->FileName��Ա������ļ���
				
				if (pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE|| pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					Add(pRecord->FileReferenceNumber, &szName, pRecord->ParentFileReferenceNumber);
				}
				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		rgIndexes.shrink_to_fit(); //100m
		DebugBreak();
	} while (FALSE);
}

//************************************
// Method:    EnumNext  ��Block�л����һ��USN_RECORD,����ǰBlock���USN_RECORD���Ѿ�����,���ȡ��һ�����ݵ�Block��
// Returns:   PUSN_RECORD
//************************************
PUSN_RECORD CDriveIndex::EnumNext()
{
	if (NULL == m_buffer)
	{
		DebugBreak();
	}
	//��ʼ��ʱ��Block�Ѿ�����ʱ
	if ((NULL == m_usnCurrent)
		|| ((PBYTE)m_usnCurrent + m_usnCurrent->RecordLength) >= (m_buffer + m_dwBufferLength))
	{
		m_usnCurrent = NULL;

		if (DeviceIoControl(m_hVolume,
			FSCTL_READ_USN_JOURNAL,
			&m_rujd,
			sizeof(m_rujd),
			m_buffer,
			HeapSize(GetProcessHeap(), 0, m_buffer),
			&m_dwBufferLength,
			NULL))
		{
			//�п��ܵ�����Record������������,���ǽ�����NULL�������ĵ�����,��ʱ������Ҫ����LastError������û�д�����
			SetLastError(NO_ERROR);

			//���ݵĿ�ͷ��usn�͵��ڸÿ����ݵ����һ��USN_RECORD����һ��
			//�����usn�洢����,��һ�ζ�����ʱ���Ӹô�������
			m_rujd.StartUsn = *(USN *)m_buffer;

			//��������һ��USN_RECORD,��ô������Ҫ�������ǵ�ָ��ΪBLOCK�еĵ�һ��
			if (m_dwBufferLength > sizeof(USN))
			{
				m_usnCurrent = (PUSN_RECORD)(m_buffer + sizeof(USN));
			}
		}
	}
	else
	{
		//Block�ﻹ��ʣ���¼,ֱ�Ӷ�����������
		m_usnCurrent = (PUSN_RECORD)((PBYTE)m_usnCurrent + m_usnCurrent->RecordLength);
	}
	return m_usnCurrent;
}

//************************************
// Method:    MakeFilter �����ļ�������һ��64λ��ֵ,�����ڹ��˳������Ĳ������������ļ�
// Returns:   DWORDLONG �������64λֵ
// Parameter: wstring * szName Ŀ���ļ���
//************************************
DWORDLONG CDriveIndex::MakeFilter(wstring* szName)
{
	/*
	64λֵ��ÿһλ�Ľ���:
	0-25 a-z
	26-35 0-9
	36 .
	37 �ո�
	38 !#$&'()+,-~_
	39 �Ƿ����2����ͬ���ַ�
	40 �Ƿ����3�������ϵ���ͬ�ַ�
	�����λ��Ϊ����Ϊ2�ĳ������ַ��Ƿ����. ����wiki�ϵ�Ƶ�ʱ�: http://en.wikipedia.org/wiki/Letter_frequency
	41 TH
	42 HE
	43 AN
	44 RE
	45 ER
	46 IN
	47 ON
	48 AT
	49 ND
	50 ST
	51 ES
	52 EN
	53 OF
	54 TE
	55 ED
	56 OR
	57 TI
	58 HI
	59 AS
	60 TO
	61-63 �ַ�������3λ�������8���ַ�,����8�ַ�ǿ��Ϊ8,һ���û������붼���8С
	*/

	//todo:����·��/��������Ҫ��ϸ�Ĳ���
	if (!(szName->length() > 0))
	{
		return 0;
	}
	DWORDLONG Filter = 0;
	int counts[26] = { 0 }; //����Ƿ����ظ����ֵ��ַ�

	wstring szLower(*szName);
	transform(szLower.begin(), szLower.end(), szLower.begin(), pinyinFirstLetter);  //ƴ��ת��,ֻ��Ҫ����ĸ,ͬʱ��ת��Сд����

	size_t size = szLower.length();
	WCHAR c;
	locale loc;
	for (size_t i = 0; i < size; i++)
	{
		c = szLower[i];
		if (c > 96 && c < 123)  //a-z
		{
			//������a-z�ĵ�����ĸ
			Filter |= 1ui64 << (DWORDLONG)((DWORDLONG)c - 97ui64); //��Ӧ0~25λ
			counts[c - 97]++; //����
			//Ȼ�������ڵ��ַ��뵱ǰ�ַ��Ƿ񹹳�2�ֽڵĶ��ַ�
			if (i < size - 1)
			{
				if (c == L't' && szLower[i + 1] == L'h') //th
					Filter |= 1ui64 << 41;
				else if (c == L'h' && szLower[i + 1] == L'e') //he
					Filter |= 1ui64 << 41;
				else if (c == L'a' && szLower[i + 1] == L'n') //an
					Filter |= 1ui64 << 41;
				else if (c == L'r' && szLower[i + 1] == L'e') //re
					Filter |= 1ui64 << 41;
				else if (c == L'e' && szLower[i + 1] == L'r') //er
					Filter |= 1ui64 << 41;
				else if (c == L'i' && szLower[i + 1] == L'n') //in
					Filter |= 1ui64 << 41;
				else if (c == L'o' && szLower[i + 1] == L'n') //on
					Filter |= 1ui64 << 41;
				else if (c == L'a' && szLower[i + 1] == L't') //at
					Filter |= 1ui64 << 41;
				else if (c == L'n' && szLower[i + 1] == L'd') //nd
					Filter |= 1ui64 << 41;
				else if (c == L's' && szLower[i + 1] == L't') //st
					Filter |= 1ui64 << 41;
				else if (c == L'e' && szLower[i + 1] == L's') //es
					Filter |= 1ui64 << 41;
				else if (c == L'e' && szLower[i + 1] == L'n') //en
					Filter |= 1ui64 << 41;
				else if (c == L'o' && szLower[i + 1] == L'f') //of
					Filter |= 1ui64 << 41;
				else if (c == L't' && szLower[i + 1] == L'e') //te
					Filter |= 1ui64 << 41;
				else if (c == L'e' && szLower[i + 1] == L'd') //ed
					Filter |= 1ui64 << 41;
				else if (c == L'o' && szLower[i + 1] == L'r') //or
					Filter |= 1ui64 << 41;
				else if (c == L't' && szLower[i + 1] == L'i') //ti
					Filter |= 1ui64 << 41;
				else if (c == L'h' && szLower[i + 1] == L'i') //hi
					Filter |= 1ui64 << 41;
				else if (c == L'a' && szLower[i + 1] == L's') //as
					Filter |= 1ui64 << 41;
				else if (c == L't' && szLower[i + 1] == L'o') //to
					Filter |= 1ui64 << 41;
			}
		}
		else if (c >= L'0' && c <= '9') //0-9
			Filter |= 1ui64 << (c - L'0' + 26ui64);
		else if (c == L'.') //.
			Filter |= 1ui64 << 36;
		else if (c == L' ') // space
			Filter |= 1ui64 << 37;
		//else if (c == L'!' || c == L'#' || c == L'$' || c == L'&' || c == L'\'' || c == L'(' || c == L')' || c == L'+' || c == L',' || c == L'-' || c == L'~' || c == L'_')
		else if (ispunct(c, loc))
			Filter |= 1ui64 << 38; // ����ĸ��ֱ�����
	}
	for (unsigned int i = 0; i != 26; i++)
	{
		if (counts[i] == 2)
			Filter |= 1ui64 << 39;
		else if (counts[i] > 2)
			Filter |= 1ui64 << 40;
	}
	DWORDLONG length = (szLower.length() > 7 ? 7ui64 : (DWORDLONG)szLower.length()) & 0x00000007ui64; //3 bits for length -> 8 max
	Filter |= length << 61ui64;

	return Filter;
}

//************************************
// Method:    Add �����ݿ������һ���ļ�
// Returns:   BOOL
// Parameter: DWORDLONG Index �ļ��е�FRN
// Parameter: wstring * szName �ļ�����
// Parameter: DWORDLONG ParentIndex ��Ŀ¼��FRN
// Parameter: DWORDLONG Filter ������,���ڲ�ѯ
//************************************
BOOL CDriveIndex::Add(DWORDLONG Index, wstring *szName, DWORDLONG ParentIndex, DWORDLONG Filter)
{
	if (mapIndexToOffset.find(Index) != mapIndexToOffset.end())
		return(FALSE); // Index already in database;

	if (!Filter)
		Filter = MakeFilter(szName);

	IndexEntry ie;
	ie.Filter = Filter;
	ie.ParentFRN = ParentIndex;
	ie.szName = wstring(*szName);
	rgIndexes.insert(rgIndexes.end(), ie);

	mapIndexToOffset[Index] = rgIndexes.size() - 1;
	return(TRUE);
}



//************************************
// Method:    AbandonMonitor  //�����ֶ��˳��߳�
//************************************
VOID CDriveIndex::AbandonMonitor()
{
	if (NULL != m_hMonitorThread)
	{
		DWORD dwRet = QueueUserAPC(
			(PAPCFUNC)[](ULONG_PTR) {OutputDebugString(TEXT("QueueUserAPC")); },
			m_hMonitorThread,
			NULL);

		if (0 == dwRet)
		{
			DebugBreak();
			dwRet = GetLastError();
		}
	}
}

//************************************
// Method:    OpenVolume				��Ŀ����
// Returns:   HANDLE					��CreateFile����ʧ��,������INVALID_HANDLE_VALUE
// Parameter: TCHAR cDriveLetter		�̷�
// Parameter: DWORD dwAccess			�����Ȩ��
// Parameter: BOOL bAsyncIO				�Ƿ������첽IO
//************************************
HANDLE CDriveIndex::OpenVolume(_TCHAR cDriveLetter, DWORD dwAccess, BOOL bAsyncIO)
{
	TCHAR szVolumePath[16];
	wsprintf(szVolumePath, TEXT("\\\\.\\%c:"), cDriveLetter);
	HANDLE hRet = CreateFile(szVolumePath,
		dwAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		(bAsyncIO ? FILE_FLAG_OVERLAPPED : 0),
		NULL);


	return hRet;
}

DWORD CDriveIndex::MonitorThread(PVOID lpParameter)
{
	if (NULL == lpParameter) return 0;
	CDriveIndex* __this = (CDriveIndex*)lpParameter;

	while (TRUE)
	{
		//�ȴ��¼�������,���¼������첽IO,�������������첽��ʽ����DeviceIoControl֮ǰ(����ʼ����Record����֮ǰ)���¼������ᴥ��
		DWORD dwRet = WaitForSingleObjectEx(__this->m_OverLapped.hEvent, INFINITE, TRUE);

		if (dwRet == WAIT_OBJECT_0)
		{
			if (__this->m_MainWindow == NULL) break;//��ǰ��δ�봰������,���޷�������Ϣ,�߳��˳�
			if (__this->m_dwDelay) Sleep(__this->m_dwDelay); //�ȴ�һ��ʱ���ٷ���,����ռ����Դ

			PostMessage(__this->m_MainWindow, WM_JOURNALCHANGED, __this->GetLetter(), NULL);
		}
		else if (dwRet == WAIT_IO_COMPLETION)
		{
			//�߳���Ҫ�˳�
			OutputDebugString(TEXT("MonitorThread Abandoned"));
			break;
		}
	}

	return 0;
}





//************************************
// Method:    FRN ���ָ���ļ�/�ļ��е�FRN
// Returns:   DWORDLONG ָ���ļ�/�ļ��е�FRN
// Parameter: wstring * strPath �ļ�·��
//************************************
DWORDLONG CDriveIndex::Name2FRN(wstring& strPath)
{
	DWORDLONG frn = 0;

	HANDLE hFile = CreateFile(strPath.c_str(),
		0, // ժ��MSDN: if NULL, the application can query certain metadata even if GENERIC_READ access would have been denied,����Ҫ����̷���FRN,ֻ��Ϊ0
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,   //You must set this flag to obtain a handle to a directory
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
		return frn;

	BY_HANDLE_FILE_INFORMATION fi;
	if (GetFileInformationByHandle(hFile, &fi))
	{
		frn = (((DWORDLONG)fi.nFileIndexHigh) << 32) | fi.nFileIndexLow;
	}

	CloseHandle(hFile);
	return frn;
}





