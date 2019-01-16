#include <tchar.h>
#include "ChangeJrnlDB.h"




ChangeJrnlDB::ChangeJrnlDB()
{
}


ChangeJrnlDB::~ChangeJrnlDB()
{
	for (auto i:m_vChangeJrnls)
	{
		delete i;
		i = nullptr;
	}
}

DWORD ChangeJrnlDB::initVolumes()
{
	DWORD dwRet = 0;
	DWORD bitMap = GetLogicalDrives();
	//msdn上说靠lable和盘符来识别硬盘都有一定不准确性,系统靠的是GUID path来识别某个盘
	//如 C:的guid path可以是"\\?\Volume{26a21bda-a627-11d7-9931-806e6f6e6963}\"
	//同时CreateFile也可以传入硬盘的GUID path来访问该盘
	//而FindFirstVolume
	// 	FindNextVolume
	// 	FindVolumeClose
	//这三个函数可以枚举出所有盘的GUID path
	//详情可见https://docs.microsoft.com/zh-cn/windows/desktop/FileIO/displaying-volume-paths
	//但是很麻烦而且没必要,我就这样了

	for (unsigned int i = 0; i < 26; i++)
	{
		if (bitMap >> i & (BYTE)1)
		{
			_TCHAR drivePath[] = TEXT( "C:\\" );
			_TCHAR lpFileSystemNameBuffer[10];
			_TCHAR driveLetter = 'A' + i;
			drivePath[0] = driveLetter;
			if (GetVolumeInformation(
				drivePath,
				NULL,
				MAX_PATH,
				NULL,
				NULL,
				NULL,
				lpFileSystemNameBuffer,
				MAX_PATH)
				)
			{
				if (!_tcscmp(lpFileSystemNameBuffer, TEXT("NTFS")))//是NTFS吗
				{
					dwRet++;
					m_vDriveLetters.push_back(driveLetter);
				}
			
			}
		}
	}
	return 0;
	
}
