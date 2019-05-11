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
	//清理工作
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
		//让线程知道我们正在退出
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

	__try {
		//申请缓冲区堆内存
		m_buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, dwBufLength);
		if (NULL == m_buffer)  __leave;

		//获得volume的句柄,注意这将要求当前进程有较高的权限
		m_hVolume = OpenVolume(cDriveLetter, GENERIC_READ | GENERIC_WRITE, FALSE);
		int aa = GetLastError();
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

		bRet = TRUE;
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

		MatchQuality = match_positions(strQuery.c_str(), szFileName.c_str(), result.MatchPos);   //模糊匹配占的时间很少 

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

int CDriveIndex::Find(wstring *strQuery, wstring *strQueryPath, vector<SearchResultFile> *rgsrfResults, BOOL bSort, int maxResults)
{
	//默认搜索文件
	unsigned int SearchWhere = IN_FILES;
	//目标在vector中的位置
	unsigned int iOffset = 0;
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
	if (!(strQueryPath != NULL && (m_LastResult.maxResults == -1 || m_LastResult.iOffset == 0) && (m_LastResult.SearchPath.length() == 0 || strQueryPathLower.find(m_LastResult.SearchPath) == 0)))
		m_LastResult = SearchResult();

	//根据输入计算出相应的filter
	DWORDLONG QueryFilter = MakeFilter(&strQueryLower);
	DWORDLONG QueryLength = (QueryFilter & 0xE000000000000000ui64) >> 61ui64;  //计算长度
	QueryFilter = QueryFilter & 0x1FFFFFFFFFFFFFFFui64;  //长度位就可以不要了

	//与上次的查询相同,且上次的查询有结果,则使用上次的结果
	if (strQueryLower.compare(m_LastResult.Query) == 0 && m_LastResult.Results.size() > 0 && (m_LastResult.SearchEndedWhere == NO_WHERE && iOffset != 1))
	{
		//保持上次查询的位置
		SearchWhere = m_LastResult.SearchEndedWhere;
		iOffset = m_LastResult.iOffset;
		bSkipSearch = TRUE;
		for (int i = 0; i != m_LastResult.Results.size(); i++)  //todo:若上次的查询结果中,某些文件被修改/删除/创建了,那么lastresult就无效了,所以需要在搜索框消失时,清空上次的搜索结果
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
				if (maxResults != -1 && nResults > maxResults) //若到达了指定的结果数量,则停止
				{
					nResults = -1;  //到了这里,说明我们上次因到达了结果上限而没能搜出所有符合条件的文件,所以需要继续上次的位置搜索
					SearchWhere = NO_WHERE;  //改变一些变量,控制流程继续搜索
					iOffset = 1;
					break;
				}
				rgsrfResults->insert(rgsrfResults->end(), m_LastResult.Results[i]);
			}
		}
		//上次因到达了结果上限而没能搜出所有符合条件的文件,所以需要继续上次的位置搜索
		if (m_LastResult.maxResults != -1 && m_LastResult.SearchEndedWhere != NO_WHERE && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}
	//这次的查询比上次的更精确,那么使用上次的结果来进一步搜索
	else if (strQueryLower.find(m_LastResult.Query) != -1 && m_LastResult.Results.size() > 0)  //todo:如果上次的搜索结果不完全,那这次的也肯定不完全
	{
		//保持上次查询的位置
		SearchWhere = m_LastResult.SearchEndedWhere;
		iOffset = m_LastResult.iOffset;
		bSkipSearch = TRUE;

		FindInPreviousResults(*strQuery, szQueryLower, QueryFilter, QueryLength, pstrQueryPathLower, *rgsrfResults, 0, maxResults, nResults);

		//上次因到达了结果上限而没能搜出所有符合条件的文件,所以需要继续上次的位置搜索
		if (m_LastResult.maxResults != -1 && m_LastResult.SearchEndedWhere != NO_WHERE && (maxResults == -1 || nResults < maxResults))
			bSkipSearch = FALSE;
	}

	//开始搜索



	if (SearchWhere == IN_FILES && !bSkipSearch) 
	{
		//在数据库中搜索
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
	do
	{
		USN_JOURNAL_DATA ujd;
		if (!Query(&ujd)) break;

		//获得跟目录的FRN,将根目录插入数据库
		WCHAR szRoot[_MAX_DRIVE];
		wsprintf(szRoot, TEXT("%c:\\"), m_tDriveLetter);

		wstring szTemp(szRoot);
		DWORDLONG IndexRoot = Name2FRN(szTemp);

		wsprintf(szRoot, TEXT("%c:"), m_tDriveLetter);
		Add(IndexRoot, &wstring(szRoot), 0);
		m_dlRootIndex = IndexRoot;

		//第一次遍历,统计所有文件/文件夹,让容器预留出空间
		MFT_ENUM_DATA_V0 med; //v1是不行的
		med.StartFileReferenceNumber = 0; //第一次需要设置为0以枚举所有文件,之后可以设置为需要的起始位置
		med.LowUsn = 0;
		med.HighUsn = ujd.NextUsn; //枚举截至到这里的文件,之后新增的等初始化完毕后再处理

		BYTE BufferData[sizeof(DWORDLONG) + 0x10000]; //每次处理64k的数据

		
		DWORD cb;
		UINT numFiles = 1; //至少有个根目录c:
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
				if (pRecord->FileAttributes&FILE_ATTRIBUTE_NORMAL || pRecord->FileAttributes == FILE_ATTRIBUTE_ARCHIVE || pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					numFiles++;
				}
				pRecord = (PUSN_RECORD)((PBYTE)pRecord + pRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)BufferData;
		}
		//计数完毕,让容器预留出合适的空间,避免后面每次插入时都要重新申请内存
		rgIndexes.reserve(numFiles); //25m
		mapIndexToOffset.reserve(numFiles); //10m
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
				DWORD dwLength = pRecord->FileNameLength / sizeof(WCHAR);
				wstring szName(pName, dwLength); //据官方文档所说,不能依赖pRecord->FileName成员来获得文件名
				
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
DWORDLONG CDriveIndex::MakeFilter(wstring* szName)
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

	//todo:中文路径/标点符号需要详细的测试
	if (!(szName->length() > 0))
	{
		return 0;
	}
	DWORDLONG Filter = 0;
	int counts[26] = { 0 }; //检查是否有重复出现的字符

	wstring szLower(*szName);
	transform(szLower.begin(), szLower.end(), szLower.begin(), pinyinFirstLetter);  //拼音转换,只需要首字母,同时还转成小写的了

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
			if (__this->m_MainWindow == NULL) break;//当前类未与窗口相连,则无法发送消息,线程退出
			if (__this->m_dwDelay) Sleep(__this->m_dwDelay); //等待一段时间再发送,避免占用资源

			PostMessage(__this->m_MainWindow, WM_JOURNALCHANGED, __this->GetLetter(), NULL);
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
DWORDLONG CDriveIndex::Name2FRN(wstring& strPath)
{
	DWORDLONG frn = 0;

	HANDLE hFile = CreateFile(strPath.c_str(),
		0, // 摘自MSDN: if NULL, the application can query certain metadata even if GENERIC_READ access would have been denied,这里要获得盘符的FRN,只能为0
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





