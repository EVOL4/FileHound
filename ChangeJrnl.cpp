#include <algorithm>
#include <locale>
#include "ChangeJrnl.h"
#include "Pinyin.h"

// Exported function to create the index of a drive
CDriveIndex* _stdcall CreateIndex(WCHAR cDrive)
{
	CDriveIndex *pDI = new CDriveIndex();
	pDI->Init(cDrive);
	pDI->InitialzeForMonitoring();
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
	m_DbName = NULL;
}

CDriveIndex::~CDriveIndex()
{
	//������
	if (m_db->open(m_DbName, TreeDB::OWRITER))
	{
		CUR cur;
		cur.UsnJournalID = m_rujd.UsnJournalID;
		cur.CurUsn = m_rujd.StartUsn;
		m_db->set(m_dlRootIndex + 1, (char*)&cur, sizeof(cur));
		m_db->close();
	}
	
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
	if (m_db != NULL)
	{
		m_db->close();
		delete m_db;
	}
	if (m_DbName != NULL)
	{
		free(m_DbName);
	}

	m_MainWindow = NULL;
	m_hVolume = NULL;
	m_hAsyncVolume = NULL;
	m_buffer = NULL;
	m_OverLapped.hEvent = NULL;
	m_hMonitorThread = NULL;
	m_db = NULL;
	m_DbName = NULL;
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
	BOOL bReBuild = TRUE;

	do
	{
		//���뻺�������ڴ�
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer) break;

		//���volume�ľ��,ע���⽫Ҫ��ǰ�����нϸߵ�Ȩ��
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		int aa = GetLastError();
		if (INVALID_HANDLE_VALUE == m_hVolume) break;

		//��������첽IO���̷����,�����ں���(�첽��)�����µ�USN_RECORD
		m_hAsyncVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, TRUE);
		if (INVALID_HANDLE_VALUE == m_hAsyncVolume) break;

		//����ͬ���͵��¼�,�����첽IO���ʱ��������
		m_OverLapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//�����غ��߳�,��m_OverLapped.hEvent����,���첽IO���,�����µ�RECORDʱ,�߳̽�֪ͨ����
		m_hMonitorThread = CreateThread(NULL,
			0,
			MonitorThread,
			this,
			0,
			NULL);

		if (NULL == m_hMonitorThread) break;

		//��ʼ�����ݿ�����
		WCHAR wDbName[_MAX_PATH] = { 0 };
		wsprintf(wDbName, TEXT("%c.db"), m_tDriveLetter);

		size_t clen = WideCharToMultiByte(CP_ACP, 0, wDbName, wcslen(wDbName), NULL, 0, NULL, FALSE);
		clen++;
		char* DbName = (char*)malloc(sizeof(char)*clen);

		if (DbName == NULL)
		{
			break;
		}
		ZeroMemory(DbName, clen);
		WideCharToMultiByte(CP_ACP, 0, wDbName, -1, DbName, clen, NULL, FALSE);

		m_DbName = DbName;

		m_db = new TreeDB();
		if (NULL == m_db) break;

		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//��ø�Ŀ¼��FRN,����Ŀ¼�������ݿ�
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);
		m_dlRootIndex = Name2FRN(szRoot);

		//����Ƿ��Ѵ������ݿ�,���Ƚ����ݿ�id�Ƿ���ȷ
	
		if (m_db->open(m_DbName, TreeDB::OREADER | TreeDB::OWRITER))
		{
			CUR* pCur = NULL;
			size_t sz = 0;
			pCur = (CUR*)m_db->get(m_dlRootIndex+1, &sz);

			if (pCur != NULL)
			{
				DWORDLONG uid = pCur->UsnJournalID;
				if (uid == ujd.UsnJournalID)  //��־id����,�������¹���
				{
					SeekToUsn(pCur->CurUsn, 0xFFFFFFFF, FALSE, uid);
					bReBuild = FALSE;
				}
			}
			m_db->close();
		}
		////////////////////////////////////////////////////////////����Ĳ���Ų��ȥ
		if (bReBuild)
		{
			InitialzeForMonitoring();
		}
		else
		{

			NotifyMoreData(500);
		}
		bRet = TRUE;
	} while (FALSE);

	return bRet;
}

//��ʼ�����ݿ�
VOID CDriveIndex::InitialzeForMonitoring()
{
	//todo:ɾ���ɵ����ݿ�
	USN_JOURNAL_DATA ujd = { 0 };
	BOOL IsOK = TRUE;
	while (IsOK && !this->Query(&ujd))
	{
		switch (GetLastError()) {
		case ERROR_JOURNAL_DELETE_IN_PROGRESS:
			//ϵͳ����ɾ��USN��־,�õȵ�ɾ����Ϻ��ٴν��в�ѯ
			this->Delete(0, USN_DELETE_FLAG_NOTIFY);
			break;

		case ERROR_JOURNAL_NOT_ACTIVE:
			//��ǰ������־δ����
			Create(0x800000, 0x100000);
			break;

		default:
			IsOK = FALSE;
			break;
		}
	}

	if (!IsOK)
	{
		//��ѯʧ��,ûʲô������,����
		return;
	}
	//������ݿ�
	PopulateIndex2DB();

	//������ݿ��,��Ҫ�������ݿ�,��Ϊ�������ݿ��д�������Ч�ļ�¼
	this->SeekToUsn(ujd.FirstUsn, 0xFFFFFFFF, FALSE, ujd.UsnJournalID);

	ProcessAvailableRecords();
}

//�������еļ�¼,���øú���ǰ����Ӧ����һ��SeekToUsn��Ϊ��������λ
void CDriveIndex::ProcessAvailableRecords()
{
	if (!m_db->open(m_DbName, TreeDB::OWRITER))
	{
		return;
	}
	PUSN_RECORD pRecord = NULL;
	while (pRecord = this->EnumNext())
	{
		if (pRecord->Reason&USN_REASON_CLOSE)
		{
			LPCWSTR pName = (LPCWSTR)((PBYTE)pRecord + pRecord->FileNameOffset);
			size_t dwLength = pRecord->FileNameLength / sizeof(WCHAR);
			//������¼
			if ( pRecord->Reason&USN_REASON_FILE_CREATE)
			{
				Add_DB(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
			}
			//��¼���
			if ( pRecord->Reason&USN_REASON_RENAME_NEW_NAME)
			{
				Change_DB(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
			}
			//��¼ɾ��
			if (pRecord->Reason&USN_REASON_FILE_DELETE)
			{
				Delete_DB(pRecord->FileReferenceNumber);
			}
		}
	}
	m_db->close();
	if (GetLastError() == S_OK) //�������¼�¼�����������,EnumNext()����LastError����ΪNOERROR
	{
		//todo:��ʼ��̨�����߳�
		NotifyMoreData(500);
	}
	else
	{
		//�д�����,��־���ܱ�ɾ����֮���,��֮��Ҫ���´�������
		InitialzeForMonitoring();
	}
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

void CDriveIndex::FindInPreviousResults(wstring &strQuery, const WCHAR* &szQueryLower, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, int maxResults, int &nResults)
{
	m_db->tune_options(TreeDB::TLINEAR);
	if (!m_db->tune_page_cache(1 * 1024 * 1024)) {
		cerr << "get error: " << m_db->error().name() << endl;
	}
	if (!m_db->tune_map(10 * 1024 * 1024)) {
		cerr << "get error: " << m_db->error().name() << endl;
	}

	if (!m_db->open("test.db", TreeDB::OREADER)) {
		cerr << "get error: " << m_db->error().name() << endl;
		return;
	}

	for (auto i : m_LastResult.Results)
	{
		SearchResultFile result;
		DWORDLONG Length = (i.Filter & 0xE000000000000000ui64) >> 61ui64;
		DWORDLONG Filter = i.Filter & 0x1FFFFFFFFFFFFFFFui64;
		if (!((Filter & QueryFilter) == QueryFilter && QueryLength <= Length))
			continue;

		score_t MatchQuality;

		MatchQuality = match_positions(strQuery.c_str(), i.Filename.c_str(), NULL);   //ģ��ƥ��ռ��ʱ�����

		if (MatchQuality > QUALITY_OK)
		{
			nResults++;

			if (maxResults != -1 && nResults > maxResults) //����������
			{
				nResults = -1;
				break;
			}

			BOOL bFound = TRUE;

			if (bFound)
			{
				rgsrfResults.insert(rgsrfResults.end(), i);
			}
		}
	}
	m_db->close();
}

void CDriveIndex::GetPath(IndexEntry& i, wstring & szFullPath)
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

//abandoned!!
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

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), NULL);   //ģ��ƥ��ռ��ʱ�����

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
void  CDriveIndex::GetPath_DB(DWORDLONG index, wstring & szPath)
{
	DBEntry* pNext = NULL;
	size_t sz = 0;
	string path;
	do {
		pNext = (DBEntry*)m_db->get(index, &sz);

		if (pNext == NULL)
		{
			break;
		}
		path = string(pNext->szName) + ((path.length() != 0) ? "\\" : "") + path;
		index = pNext->ParentFRN;
		delete[]  pNext;
		pNext = NULL;
	} while (index != 0);

	int cslen = MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, NULL, 0);
	WCHAR* csName = new WCHAR[cslen];
	ZeroMemory(csName, sizeof(WCHAR)*cslen);
	MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, csName, cslen);

	szPath = csName;
	delete[] csName;
}

void CDriveIndex::FindInDB(wstring &strQuery, DWORDLONG QueryFilter, DWORDLONG QueryLength, wstring * strQueryPath, vector<SearchResultFile> &rgsrfResults, int maxResults, int &nResults)
{
	m_db->tune_options(TreeDB::TLINEAR);
	if (!m_db->tune_page_cache(1 * 1024 * 1024)) {
		cerr << "get error: " << m_db->error().name() << endl;
	}
	if (!m_db->tune_map(10 * 1024 * 1024)) {
		cerr << "get error: " << m_db->error().name() << endl;
	}

	if (!m_db->open(m_DbName, TreeDB::OREADER|TreeDB::OWRITER)) {
		const char* a = m_db->error().name();
		cerr << "get error: " << a << endl;
		return;
	}

	DB::Cursor* cur = m_db->cursor();

	if (m_LastResult.iOffSet != 0)
	{
		//��Ҫ���ϴεĽ����ʼ����
		if (!cur->jump((char*)&m_LastResult.iOffSet, sizeof(DWORDLONG)))
		{
			//�ϴε�λ�÷����˸ı�,��ͷ��ʼ
			cur->jump();
		}
	}
	else
	{
		cur->jump();
	}

	DBEntry* entry = NULL;
	DWORDLONG* pIndex = NULL;
	size_t cbIndex = 0, cbValue = 0;

	while (pIndex = (DWORDLONG*)cur->get(&cbIndex, (const char**)&entry, &cbValue, true), pIndex)
	{
		if (*pIndex > m_dlRootIndex)
		{
			delete[] pIndex;
			continue;
		}
		SearchResultFile result;
		DWORDLONG Length = (entry->Filter & 0xE000000000000000ui64) >> 61ui64;
		DWORDLONG Filter = entry->Filter & 0x1FFFFFFFFFFFFFFFui64; //ȥ������λ

		if (!((Filter & QueryFilter) == QueryFilter && QueryLength <= Length))
		{
			delete[] pIndex;
			pIndex = NULL;
			entry = NULL;
			continue;
		}

		int cslen = MultiByteToWideChar(CP_ACP, 0, entry->szName, -1, NULL, 0);
		WCHAR* csName = new WCHAR[cslen];
		ZeroMemory(csName, sizeof(WCHAR)*cslen);
		MultiByteToWideChar(CP_ACP, 0, entry->szName, -1, csName, cslen);

		score_t MatchQuality;
		wstring szFileName(csName);
		//
		// 		if strQuery��������
		// 			then szFileName��תӢ��
		// 		else

		std::transform(szFileName.begin(), szFileName.end(), szFileName.begin(), pinyinFirstLetter);

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), nullptr);   //ģ��ƥ��ռ��ʱ�����

		if (MatchQuality > QUALITY_OK)
		{
			if (maxResults != -1 && nResults > maxResults) //����������
			{
				nResults = -1;
				m_LastResult.iOffSet = *pIndex;
				delete[] pIndex;
				delete[] csName;
				break;
			}
			nResults++;

			result.Filename = csName;
			GetPath_DB(*pIndex, result.Path);

			BOOL bFound = TRUE;

			if (strQueryPath != NULL)
			{
				wstring strPathLower(result.Path);
				std::transform(strPathLower.begin(), strPathLower.end(), strPathLower.begin(), tolower);
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
				result.Filter = entry->Filter;
				result.Index = *pIndex;
				result.MatchQuality = MatchQuality;
				rgsrfResults.insert(rgsrfResults.end(), result);
			}
		}

		delete[] pIndex;
		delete[] csName;
		pIndex = NULL;
		entry = NULL;
	}

	delete cur;	m_db->close();
}

int CDriveIndex::Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort, int maxResults)
{
	//Ĭ�������ļ�
	unsigned int SearchWhere = IN_FILES;
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
	if (strQueryPath != NULL && (m_LastResult.maxResults == -1))
	{
		if (!(m_LastResult.SearchPath.length() == 0 || strQueryPathLower.find(m_LastResult.SearchPath) == 0))
		{
			m_LastResult = SearchResult();
		}
	}

	//��������������Ӧ��filter
	DWORDLONG QueryFilter = MakeFilter((WCHAR*)strQueryLower.c_str(), strQueryLower.length());
	DWORDLONG QueryLength = (QueryFilter & 0xE000000000000000ui64) >> 61ui64;  //���㳤��
	QueryFilter = QueryFilter & 0x1FFFFFFFFFFFFFFFui64;  //����λ�Ϳ��Բ�Ҫ��

	//��εĲ�ѯ���ϴεĸ���ȷ,��ôʹ���ϴεĽ������һ������
	/*else */
	if (strQueryLower.find(m_LastResult.Query) != -1 && m_LastResult.Results.size() > 0)  //todo:����ϴε������������ȫ,����ε�Ҳ�϶�����ȫ
	{
		bSkipSearch = TRUE;
		FindInPreviousResults(*strQuery, szQueryLower, QueryFilter, QueryLength, pstrQueryPathLower, *rgsrfResults, maxResults, nResults);
		//�ϴ��򵽴��˽�����޶�û���ѳ����з����������ļ�,������Ҫ�����ϴε�λ������
		if (m_LastResult.maxResults != -1 && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}

	//��ʼ����

	if (!bSkipSearch)
	{
		//�����ݿ�������
		FindInDB(*strQuery, QueryFilter, QueryLength, (strQueryPath != NULL ? &strQueryPathLower : NULL), *rgsrfResults, maxResults, nResults);

		//�����û�����,ֻ�����˲��ֽ��,����iOffSet�������
		if (!(maxResults != -1 && nResults == -1))
		{
			m_LastResult.iOffSet = 0;
		}
	}

	m_LastResult.Query = wstring(strQueryLower);
	m_LastResult.SearchPath = strQueryPathLower;
	m_LastResult.Results = vector<SearchResultFile>();
	m_LastResult.maxResults = maxResults;
	m_LastResult.SearchEndedWhere = SearchWhere;

	//Update last results
	for (unsigned int i = 0; i != rgsrfResults->size(); i++)
		m_LastResult.Results.insert(m_LastResult.Results.end(), (*rgsrfResults)[i]);
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
	//Empty()
	do
	{
		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//��ø�Ŀ¼��FRN,����Ŀ¼�������ݿ�
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);
		DWORDLONG IndexRoot = Name2FRN(szRoot);
		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add(IndexRoot, szRoot, wcslen(szRoot), 0, 0, TRUE);

		m_dlRootIndex = IndexRoot;

		//��һ�α���,ͳ�������ļ�/�ļ���,������Ԥ�����ռ�
		MFT_ENUM_DATA_V0 med; //v1�ǲ��е�
		med.StartFileReferenceNumber = 0; //��һ����Ҫ����Ϊ0��ö�������ļ�,֮���������Ϊ��Ҫ����ʼλ��
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //ö�ٽ�����������ļ�,֮�������ĵȳ�ʼ����Ϻ��ٴ���

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //ÿ�δ���64k������

		DWORD cb;
		UINT numDirs = 1, numFiles = 0; //�����и���Ŀ¼c:
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
				if (pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					numDirs++;
				}
				else  if (pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE)
				{
					numFiles++;
				}
				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		//�������,������Ԥ�������ʵĿռ�,�������ÿ�β���ʱ��Ҫ���������ڴ�
		rgIndexes.reserve(numFiles + numDirs);
		mapIndexToOffset.reserve(numDirs); //�ó�Ա�����������·��,����ֻ�ü�¼�ļ���

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
				size_t dwLength = pRecord->FileNameLength / sizeof(WCHAR);

				//wstring szName(pName, dwLength); //�ݹٷ��ĵ���˵,��������pRecord->FileName��Ա������ļ���

				if (pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					Add(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber, 0, TRUE);
				}
				else  if (pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE)
				{
					Add(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
				}
				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		rgIndexes.shrink_to_fit();  //78mb
	} while (FALSE);
}

BOOL CDriveIndex::Add_DB(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter)
{
	if (-1 != m_db->check(Index))
	{
		return(FALSE); // Index already in database;
	}

	if (NULL == szName)
	{
		return(FALSE);
	}

	size_t clen = WideCharToMultiByte(CP_ACP, 0, szName, szLength, NULL, 0, NULL, FALSE);

	DWORD len = sizeof(DBEntry) + clen * sizeof(char);
	DBEntry *entry = (DBEntry *)malloc(len);
	if (entry == NULL)
	{
		return(FALSE);
	}
	ZeroMemory(entry, len);
	WideCharToMultiByte(CP_ACP, 0, szName, -1, entry->szName, clen, NULL, FALSE);

	if (!Filter)
		Filter = MakeFilter(szName, szLength);
	entry->Filter = Filter;
	entry->ParentFRN = ParentIndex;

	if (!m_db->add(Index, (char*)entry, len))
	{
		cerr << "add error: " << m_db->error().name() << endl;
	}

	free(entry);

	return(TRUE);
}

BOOL CDriveIndex::Change_DB(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter /*= 0ui64*/)
{
	if (-1 == m_db->check(Index))// ���ݿ��в�����
	{
		return(FALSE);
	}

	if (NULL == szName)
	{
		return(FALSE);
	}

	size_t clen = WideCharToMultiByte(CP_ACP, 0, szName, szLength, NULL, 0, NULL, FALSE);

	DWORD len = sizeof(DBEntry) + clen * sizeof(char);
	DBEntry *entry = (DBEntry *)malloc(len);
	if (entry == NULL)
	{
		return(FALSE);
	}
	ZeroMemory(entry, len);
	WideCharToMultiByte(CP_ACP, 0, szName, -1, entry->szName, clen, NULL, FALSE);

	if (!Filter)
		Filter = MakeFilter(szName, szLength);
	entry->Filter = Filter;
	entry->ParentFRN = ParentIndex;

	if (!m_db->set(Index, (char*)entry, len))
	{
		cerr << "set error: " << m_db->error().name() << endl;
	}

	free(entry);

	return(TRUE);
}

BOOL CDriveIndex::Delete_DB(DWORDLONG Index)
{
	if (-1 == m_db->check(Index))// ���ݿ��в�����
	{
		return(FALSE);
	}
	if (!m_db->remove(Index))
	{
		cerr << "delete error: " << m_db->error().name() << endl;
	}
}

VOID CDriveIndex::PopulateIndex2DB()
{
	//Empty()
	do
	{
		
		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//��ø�Ŀ¼��FRN,����Ŀ¼�������ݿ�
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);

		//�������ݿ�
		if (!m_db->open(m_DbName, TreeDB::OWRITER | TreeDB::OCREATE)) break;

		m_db->clear();
		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add_DB(m_dlRootIndex, szRoot, wcslen(szRoot), 0);

		MFT_ENUM_DATA_V0 med; //v1�ǲ��е�
		med.StartFileReferenceNumber = 0; //��һ����Ҫ����Ϊ0��ö�������ļ�,֮���������Ϊ��Ҫ����ʼλ��
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //ö�ٽ�����������ļ�,֮�������ĵȳ�ʼ����Ϻ��ٴ���

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //ÿ�δ���64k������

		DWORD cb;

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
				size_t dwLength = pRecord->FileNameLength / sizeof(WCHAR);
				//wstring szName(pName, dwLength); //�ݹٷ��ĵ���˵,��������pRecord->FileName��Ա������ļ���

				if (pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY || pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE)
				{
					Add_DB(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
				}

				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		m_db->close();


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
DWORDLONG CDriveIndex::MakeFilter(LPCWSTR szName, size_t szLength)
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
	wstring  name(szName, szLength);
	//todo:����·��/��������Ҫ��ϸ�Ĳ���
	if (!(name.length() > 0))
	{
		return 0;
	}
	DWORDLONG Filter = 0;
	int counts[26] = { 0 }; //����Ƿ����ظ����ֵ��ַ�

	wstring szLower(name);
	std::transform(szLower.begin(), szLower.end(), szLower.begin(), pinyinFirstLetter);  //ƴ��ת��,ֻ��Ҫ����ĸ,ͬʱ��ת��Сд����

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
BOOL CDriveIndex::Add(DWORDLONG Index, LPCWSTR szName, size_t szLength, DWORDLONG ParentIndex, DWORDLONG Filter, BOOL IsDir)
{
	if (mapIndexToOffset.find(Index) != mapIndexToOffset.end())
		return(FALSE); // Index already in database;

	if (NULL == szName)
	{
		return(FALSE);
	}
	size_t BuffeSize = sizeof(WCHAR)*(szLength + 1);
	size_t sourceSize = sizeof(WCHAR)*szLength;
	WCHAR* filename = (WCHAR*)malloc(BuffeSize);
	ZeroMemory(filename, BuffeSize);
	memcpy_s(filename, BuffeSize, szName, sourceSize);

	if (!Filter)
		Filter = MakeFilter(filename, BuffeSize);

	IndexEntry ie;
	ie.Filter = Filter;
	ie.ParentFRN = ParentIndex;
	ie.szName = filename;
	rgIndexes.push_back(ie);

	if (IsDir)
	{
		mapIndexToOffset[Index] = rgIndexes.size() - 1;
	}

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
			//if (__this->m_MainWindow == NULL) break;//��ǰ��δ�봰������,���޷�������Ϣ,�߳��˳�
			if (__this->m_dwDelay) Sleep(__this->m_dwDelay); //�ȴ�һ��ʱ���ٷ���,����ռ����Դ
			OutputDebugString(TEXT("new record!"));

			__this->ProcessAvailableRecords();
			__this->NotifyMoreData(__this->m_dwDelay);

			//PostMessage(__this->m_MainWindow, WM_JOURNALCHANGED, __this->GetLetter(), NULL);
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
DWORDLONG CDriveIndex::Name2FRN(LPCWSTR strPath)
{
	DWORDLONG frn = 0;

	HANDLE hFile = CreateFile(strPath,
		0, // ժ��MSDN: if NULL, the application can query certain metadata even if GENERIC_READ access would have been denied,����Ҫ����̷���FRN,ֻ��Ϊ0
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,   //ժ��MSDN:You must set this flag to obtain a handle to a directory
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

//�������ݸ���
VOID CDriveIndex::NotifyMoreData(DWORD dwDelay)
{
	m_dwDelay = dwDelay;

	READ_USN_JOURNAL_DATA_V0 rujd;
	rujd = m_rujd;
	rujd.BytesToWaitFor = 1;
	DWORD dwRet = 0;
	// ���ٶ�һ���ֽ�,������һ���ֽں�,˵�����µļ�¼,m_OverLapped�е��¼���������
	BOOL fOk = DeviceIoControl(m_hAsyncVolume,
		FSCTL_READ_USN_JOURNAL,
		&rujd,
		sizeof(rujd),
		&m_AsyncUsn,
		sizeof(m_AsyncUsn),
		&dwRet,
		&m_OverLapped);

	DWORD lr = GetLastError();

	if (!fOk && (GetLastError() != ERROR_IO_PENDING)) {
		cerr << "get error: NotifyMoreData" << endl;
	}
}