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
	//清理工作
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
		//让线程知道我们正在退出
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
// Method:    Init	 用于初始化数据,注意不要多次调初始化
// Returns:   BOOL	 若初始化失败,将返回FALSE,此时应终止所有操作
// Parameter: TCHAR _tDriveLetter  盘符
// Parameter: DWORD dwBufLength   用于接收数据的缓冲区大小,大块内存将有助于增快我们处理数据的速度
//************************************
BOOL CDriveIndex::Init(_TCHAR cDriveLetter, DWORD dwBufLength)
{
	dwBufLength += sizeof(DWORDLONG); //一个USN的大小
	m_tDriveLetter = cDriveLetter;
	BOOL bRet = FALSE;
	BOOL bReBuild = TRUE;

	do
	{
		//申请缓冲区堆内存
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer) break;

		//获得volume的句柄,注意这将要求当前进程有较高的权限
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		int aa = GetLastError();
		if (INVALID_HANDLE_VALUE == m_hVolume) break;

		//获得用于异步IO的盘符句柄,将用于后面(异步地)处理新的USN_RECORD
		m_hAsyncVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, TRUE);
		if (INVALID_HANDLE_VALUE == m_hAsyncVolume) break;

		//创建同步型的事件,用于异步IO完成时提醒我们
		m_OverLapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		//创建守候线程,当m_OverLapped.hEvent授信,即异步IO完成,即有新的RECORD时,线程将通知我们
		m_hMonitorThread = CreateThread(NULL,
			0,
			MonitorThread,
			this,
			0,
			NULL);

		if (NULL == m_hMonitorThread) break;

		//初始化数据库名称
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

		//获得根目录的FRN,将根目录插入数据库
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);
		m_dlRootIndex = Name2FRN(szRoot);

		//检测是否已存在数据库,并比较数据库id是否正确
	
		if (m_db->open(m_DbName, TreeDB::OREADER | TreeDB::OWRITER))
		{
			CUR* pCur = NULL;
			size_t sz = 0;
			pCur = (CUR*)m_db->get(m_dlRootIndex+1, &sz);

			if (pCur != NULL)
			{
				DWORDLONG uid = pCur->UsnJournalID;
				if (uid == ujd.UsnJournalID)  //日志id对上,不用重新构建
				{
					SeekToUsn(pCur->CurUsn, 0xFFFFFFFF, FALSE, uid);
					bReBuild = FALSE;
				}
			}
			m_db->close();
		}
		////////////////////////////////////////////////////////////上面的步骤挪出去
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

//初始化数据库
VOID CDriveIndex::InitialzeForMonitoring()
{
	//todo:删除旧的数据库
	USN_JOURNAL_DATA ujd = { 0 };
	BOOL IsOK = TRUE;
	while (IsOK && !this->Query(&ujd))
	{
		switch (GetLastError()) {
		case ERROR_JOURNAL_DELETE_IN_PROGRESS:
			//系统正在删除USN日志,得等到删除完毕后再次进行查询
			this->Delete(0, USN_DELETE_FLAG_NOTIFY);
			break;

		case ERROR_JOURNAL_NOT_ACTIVE:
			//当前盘上日志未启用
			Create(0x800000, 0x100000);
			break;

		default:
			IsOK = FALSE;
			break;
		}
	}

	if (!IsOK)
	{
		//查询失败,没什么可做的,返回
		return;
	}
	//填充数据库
	PopulateIndex2DB();

	//填充数据库后,需要更新数据库,因为填充的数据库中存在着无效的记录
	this->SeekToUsn(ujd.FirstUsn, 0xFFFFFFFF, FALSE, ujd.UsnJournalID);

	ProcessAvailableRecords();
}

//处理所有的记录,调用该函数前至少应调用一次SeekToUsn来为本函数定位
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
			//新增记录
			if ( pRecord->Reason&USN_REASON_FILE_CREATE)
			{
				Add_DB(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
			}
			//记录变更
			if ( pRecord->Reason&USN_REASON_RENAME_NEW_NAME)
			{
				Change_DB(pRecord->FileReferenceNumber, pName, dwLength, pRecord->ParentFileReferenceNumber);
			}
			//记录删除
			if (pRecord->Reason&USN_REASON_FILE_DELETE)
			{
				Delete_DB(pRecord->FileReferenceNumber);
			}
		}
	}
	m_db->close();
	if (GetLastError() == S_OK) //若所有新记录都被处理完毕,EnumNext()将把LastError设置为NOERROR
	{
		//todo:开始后台更新线程
		NotifyMoreData(500);
	}
	else
	{
		//有错误发生,日志可能被删除了之类的,总之需要重新处理数据
		InitialzeForMonitoring();
	}
}

//************************************
// Method:    Create 对目标卷创建usn日志,若已存在,该函数将更新日志的MaximumSize和AllocationDelta两个成员
// Returns:   BOOL 若创建失败,则不该继续任何操作
// Parameter: DWORDLONG MaximumSize	日志的最大大小
// Parameter: DWORDLONG AllocationDelta	需要扩大时,扩大的幅度(按字节算),0为默认
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
// Method:    Delete 删除该盘上给定id的日志,没事别调用该函数,重新创建日志并创建数据库很耗时的
// Returns:   BOOL 若删除失败,请参考LastError的值
// Parameter: DWORDLONG UsnJournalID 日志的id
// Parameter: DWORD DeleteFlags 可以是USN_DELETE_FLAG_DELETE或USN_DELETE_FLAG_NOTIFY,也可以是两者的组合
//USN_DELETE_FLAG_DELETE将让系统发出删除操作,但由系统来觉定何时删除,DeviceIoControl将立即返回
//而USN_DELETE_FLAG_NOTIFY会让DeviceIoControl在日志被删除后才返回,但该标志本身不会让系统删除日志
//若两者都被set,DeviceIoControl将会发出删除日志的通知,并且在日志被删除后才返回
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
// Method:    Query 查询当前日志的信息
// Returns:   BOOL
// Parameter: PUSN_JOURNAL_DATA pUsnJournalData 接受数据的指针,注意传入指针的合法性,请自行申请内存
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
// Method:    DropLastResult  m_LastResult仅在用户操作界面期间有效
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

		MatchQuality = match_positions(strQuery.c_str(), i.Filename.c_str(), NULL);   //模糊匹配占的时间很少

		if (MatchQuality > QUALITY_OK)
		{
			nResults++;

			if (maxResults != -1 && nResults > maxResults) //超出上限了
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
		DWORDLONG Filter = j.Filter & 0x1FFFFFFFFFFFFFFFui64; //去除长度位

		if (!((Filter & QueryFilter) == QueryFilter && QueryLength <= Length))
			continue;

		score_t MatchQuality;
		wstring szFileName(j.szName);
		/*
		if strQuery里有中文
			szFileName不转英文
		else
		*/
		transform(szFileName.begin(), szFileName.end(), szFileName.begin(), pinyinFirstLetter);

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), NULL);   //模糊匹配占的时间很少

		if (MatchQuality > QUALITY_OK)
		{
			nResults++;

			if (maxResults != -1 && nResults > maxResults) //超出上限了
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
		//需要从上次的结果开始搜索
		if (!cur->jump((char*)&m_LastResult.iOffSet, sizeof(DWORDLONG)))
		{
			//上次的位置发生了改变,从头开始
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
		DWORDLONG Filter = entry->Filter & 0x1FFFFFFFFFFFFFFFui64; //去除长度位

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
		// 		if strQuery里有中文
		// 			then szFileName不转英文
		// 		else

		std::transform(szFileName.begin(), szFileName.end(), szFileName.begin(), pinyinFirstLetter);

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), nullptr);   //模糊匹配占的时间很少

		if (MatchQuality > QUALITY_OK)
		{
			if (maxResults != -1 && nResults > maxResults) //超出上限了
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
	//默认搜索文件
	unsigned int SearchWhere = IN_FILES;
	//若输入与上次相同,则直接用上次的结果,跳过搜索过程
	BOOL bSkipSearch = FALSE;

	//搜索到的结果数量,-1表示搜到的超过了上限
	int nResults = 0;

	//输入为空,返回
	if (strQuery->length() == 0)
	{
		m_LastResult.Query = wstring(TEXT(""));
		m_LastResult.Results = vector<SearchResultFile>();
		return nResults;
	}

	if (strQueryPath)
	{
		//检测搜索路径是否属于当前盘
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

	//若搜索路径与上次的不同,那上次的结果就该废弃
	if (strQueryPath != NULL && (m_LastResult.maxResults == -1))
	{
		if (!(m_LastResult.SearchPath.length() == 0 || strQueryPathLower.find(m_LastResult.SearchPath) == 0))
		{
			m_LastResult = SearchResult();
		}
	}

	//根据输入计算出相应的filter
	DWORDLONG QueryFilter = MakeFilter((WCHAR*)strQueryLower.c_str(), strQueryLower.length());
	DWORDLONG QueryLength = (QueryFilter & 0xE000000000000000ui64) >> 61ui64;  //计算长度
	QueryFilter = QueryFilter & 0x1FFFFFFFFFFFFFFFui64;  //长度位就可以不要了

	//这次的查询比上次的更精确,那么使用上次的结果来进一步搜索
	/*else */
	if (strQueryLower.find(m_LastResult.Query) != -1 && m_LastResult.Results.size() > 0)  //todo:如果上次的搜索结果不完全,那这次的也肯定不完全
	{
		bSkipSearch = TRUE;
		FindInPreviousResults(*strQuery, szQueryLower, QueryFilter, QueryLength, pstrQueryPathLower, *rgsrfResults, maxResults, nResults);
		//上次因到达了结果上限而没能搜出所有符合条件的文件,所以需要继续上次的位置搜索
		if (m_LastResult.maxResults != -1 && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}

	//开始搜索

	if (!bSkipSearch)
	{
		//在数据库中搜索
		FindInDB(*strQuery, QueryFilter, QueryLength, (strQueryPath != NULL ? &strQueryPathLower : NULL), *rgsrfResults, maxResults, nResults);

		//由于用户限制,只给出了部分结果,所以iOffSet不会清空
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
// Method:    SeekToUsn  该函数应在读取操作前调用, 更新m_rujd结构,可以定位读取的起始位置,过滤读取的结果,决定读取的结果何时返回
// Parameter: USN usn	开始读取数据的的usn号,每个usn对应着一个USN_RECORD结构,若传入0,
// Parameter: DWORD ReasonMask	若为0xFFFFFFFF,则所有的RECORD都将返回给我们
// Parameter: DWORD ReturnOnlyOnClose 只返回reason里有USN_REASON_CLOSE的结果,这意味着我们只关心文件被操作的最终结果
// Parameter: DWORDLONG UsnJournalID 日志id,不能填错哦
//************************************
VOID CDriveIndex::SeekToUsn(USN usn, DWORD ReasonMask, DWORD ReturnOnlyOnClose, DWORDLONG UsnJournalID)
{
	m_rujd.StartUsn = usn;
	m_rujd.ReasonMask = (ReturnOnlyOnClose != 0) ? (ReasonMask | USN_REASON_CLOSE) : ReasonMask; //若ReturnOnlyOnClose=true,则ReasonMask需要指明USN_REASON_CLOSE标志
	m_rujd.ReturnOnlyOnClose = ReturnOnlyOnClose;
	m_rujd.Timeout = 0;
	m_rujd.BytesToWaitFor = 0;
	m_rujd.UsnJournalID = UsnJournalID;

	m_dwBufferLength = 0;
	m_usnCurrent = NULL;
}

//************************************
// Method:    PopulateIndex  初始化数据库,当第一次运行,或原先的数据库不再有效时调用
//************************************
VOID CDriveIndex::PopulateIndex()
{
	//Empty()
	do
	{
		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//获得跟目录的FRN,将根目录插入数据库
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);
		DWORDLONG IndexRoot = Name2FRN(szRoot);
		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add(IndexRoot, szRoot, wcslen(szRoot), 0, 0, TRUE);

		m_dlRootIndex = IndexRoot;

		//第一次遍历,统计所有文件/文件夹,让容器预留出空间
		MFT_ENUM_DATA_V0 med; //v1是不行的
		med.StartFileReferenceNumber = 0; //第一次需要设置为0以枚举所有文件,之后可以设置为需要的起始位置
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //枚举截至到这里的文件,之后新增的等初始化完毕后再处理

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //每次处理64k的数据

		DWORD cb;
		UINT numDirs = 1, numFiles = 0; //至少有个根目录c:
		while (FALSE != DeviceIoControl(m_hVolume, //这个句柄就需要我们以读/写权限打开盘符才行
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
		//计数完毕,让容器预留出合适的空间,避免后面每次插入时都要重新申请内存
		rgIndexes.reserve(numFiles + numDirs);
		mapIndexToOffset.reserve(numDirs); //该成员用作获得完整路径,所以只用记录文件夹

		//第二次遍历,开始数据的插入

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

				//wstring szName(pName, dwLength); //据官方文档所说,不能依赖pRecord->FileName成员来获得文件名

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
	if (-1 == m_db->check(Index))// 数据库中不存在
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
	if (-1 == m_db->check(Index))// 数据库中不存在
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

		//获得根目录的FRN,将根目录插入数据库
		WCHAR szRoot[_MAX_PATH];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);

		//构建数据库
		if (!m_db->open(m_DbName, TreeDB::OWRITER | TreeDB::OCREATE)) break;

		m_db->clear();
		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add_DB(m_dlRootIndex, szRoot, wcslen(szRoot), 0);

		MFT_ENUM_DATA_V0 med; //v1是不行的
		med.StartFileReferenceNumber = 0; //第一次需要设置为0以枚举所有文件,之后可以设置为需要的起始位置
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //枚举截至到这里的文件,之后新增的等初始化完毕后再处理

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //每次处理64k的数据

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
				//wstring szName(pName, dwLength); //据官方文档所说,不能依赖pRecord->FileName成员来获得文件名

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
// Method:    EnumNext  从Block中获得下一个USN_RECORD,若当前Block里的USN_RECORD都已经遍历,则读取下一块数据到Block里
// Returns:   PUSN_RECORD
//************************************
PUSN_RECORD CDriveIndex::EnumNext()
{
	if (NULL == m_buffer)
	{
		DebugBreak();
	}
	//初始化时或Block已经用完时
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
			//有可能当所有Record都被读出来了,我们将返回NULL给函数的调用者,此时我们需要设置LastError表明并没有错误发生
			SetLastError(NO_ERROR);

			//数据的开头的usn就等于该块数据的最后一个USN_RECORD的下一个
			//将这个usn存储起来,下一次读数据时将从该处继续读
			m_rujd.StartUsn = *(USN *)m_buffer;

			//若有至少一个USN_RECORD,那么我们需要更新我们的指针为BLOCK中的第一块
			if (m_dwBufferLength > sizeof(USN))
			{
				m_usnCurrent = (PUSN_RECORD)(m_buffer + sizeof(USN));
			}
		}
	}
	else
	{
		//Block里还有剩余记录,直接读出来就是了
		m_usnCurrent = (PUSN_RECORD)((PBYTE)m_usnCurrent + m_usnCurrent->RecordLength);
	}
	return m_usnCurrent;
}

//************************************
// Method:    MakeFilter 根据文件名计算一个64位的值,可用于过滤出大量的不符合条件的文件
// Returns:   DWORDLONG 计算出的64位值
// Parameter: wstring * szName 目标文件名
//************************************
DWORDLONG CDriveIndex::MakeFilter(LPCWSTR szName, size_t szLength)
{
	/*
	64位值中每一位的解释:
	0-25 a-z
	26-35 0-9
	36 .
	37 空格
	38 !#$&'()+,-~_
	39 是否具有2个相同的字符
	40 是否具有3个及以上的相同字符
	下面的位数为长度为2的常见的字符是否出现. 基于wiki上的频率表: http://en.wikipedia.org/wiki/Letter_frequency
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
	61-63 字符串长度3位代表最大8个字符,大于8字符强制为8,一般用户的输入都会比8小
	*/
	wstring  name(szName, szLength);
	//todo:中文路径/标点符号需要详细的测试
	if (!(name.length() > 0))
	{
		return 0;
	}
	DWORDLONG Filter = 0;
	int counts[26] = { 0 }; //检查是否有重复出现的字符

	wstring szLower(name);
	std::transform(szLower.begin(), szLower.end(), szLower.begin(), pinyinFirstLetter);  //拼音转换,只需要首字母,同时还转成小写的了

	size_t size = szLower.length();
	WCHAR c;
	locale loc;
	for (size_t i = 0; i < size; i++)
	{
		c = szLower[i];
		if (c > 96 && c < 123)  //a-z
		{
			//首先是a-z的单个字母
			Filter |= 1ui64 << (DWORDLONG)((DWORDLONG)c - 97ui64); //对应0~25位
			counts[c - 97]++; //计数
			//然后检测相邻的字符与当前字符是否构成2字节的短字符
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
			Filter |= 1ui64 << 38; // 其余的各种标点符号
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
// Method:    Add 向数据库里插入一个文件
// Returns:   BOOL
// Parameter: DWORDLONG Index 文件夹的FRN
// Parameter: wstring * szName 文件夹名
// Parameter: DWORDLONG ParentIndex 父目录的FRN
// Parameter: DWORDLONG Filter 过滤器,用于查询
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
// Method:    AbandonMonitor  //用于手动退出线程
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
// Method:    OpenVolume				打开目标盘
// Returns:   HANDLE					若CreateFile调用失败,将返回INVALID_HANDLE_VALUE
// Parameter: TCHAR cDriveLetter		盘符
// Parameter: DWORD dwAccess			所需的权限
// Parameter: BOOL bAsyncIO				是否用于异步IO
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
		//等待事件被触发,该事件用于异步IO,所以在我们以异步方式调用DeviceIoControl之前(即开始接收Record更新之前)该事件都不会触发
		DWORD dwRet = WaitForSingleObjectEx(__this->m_OverLapped.hEvent, INFINITE, TRUE);

		if (dwRet == WAIT_OBJECT_0)
		{
			//if (__this->m_MainWindow == NULL) break;//当前类未与窗口相连,则无法发送消息,线程退出
			if (__this->m_dwDelay) Sleep(__this->m_dwDelay); //等待一段时间再发送,避免占用资源
			OutputDebugString(TEXT("new record!"));

			__this->ProcessAvailableRecords();
			__this->NotifyMoreData(__this->m_dwDelay);

			//PostMessage(__this->m_MainWindow, WM_JOURNALCHANGED, __this->GetLetter(), NULL);
		}
		else if (dwRet == WAIT_IO_COMPLETION)
		{
			//线程需要退出
			OutputDebugString(TEXT("MonitorThread Abandoned"));
			break;
		}
	}

	return 0;
}

//************************************
// Method:    FRN 获得指定文件/文件夹的FRN
// Returns:   DWORDLONG 指定文件/文件夹的FRN
// Parameter: wstring * strPath 文件路径
//************************************
DWORDLONG CDriveIndex::Name2FRN(LPCWSTR strPath)
{
	DWORDLONG frn = 0;

	HANDLE hFile = CreateFile(strPath,
		0, // 摘自MSDN: if NULL, the application can query certain metadata even if GENERIC_READ access would have been denied,这里要获得盘符的FRN,只能为0
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,   //摘自MSDN:You must set this flag to obtain a handle to a directory
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

//开启数据更新
VOID CDriveIndex::NotifyMoreData(DWORD dwDelay)
{
	m_dwDelay = dwDelay;

	READ_USN_JOURNAL_DATA_V0 rujd;
	rujd = m_rujd;
	rujd.BytesToWaitFor = 1;
	DWORD dwRet = 0;
	// 至少读一个字节,当读出一个字节后,说明有新的记录,m_OverLapped中的事件将被授信
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