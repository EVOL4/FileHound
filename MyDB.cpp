#include <tchar.h>
#include "MyDB.h"




MyDB::MyDB()
{
}


MyDB::~MyDB()
{
	for (auto i:m_vChangeJrnls)
	{
		delete i;
		i = nullptr;
	}
}

DWORD MyDB::initVolumes()
{
	DWORD dwRet = 0;
	DWORD bitMap = GetLogicalDrives();
	//msdn��˵��lable���̷���ʶ��Ӳ�̶���һ����׼ȷ��,ϵͳ������GUID path��ʶ��ĳ����
	//�� C:��guid path������"\\?\Volume{26a21bda-a627-11d7-9931-806e6f6e6963}\"
	//ͬʱCreateFileҲ���Դ���Ӳ�̵�GUID path�����ʸ���
	//��FindFirstVolume
	// 	FindNextVolume
	// 	FindVolumeClose
	//��������������ö�ٳ������̵�GUID path
	//����ɼ�https://docs.microsoft.com/zh-cn/windows/desktop/FileIO/displaying-volume-paths
	//���Ǻ��鷳����û��Ҫ,�Ҿ�������

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
				if (!_tcscmp(lpFileSystemNameBuffer, TEXT("NTFS")))//��NTFS��
				{
					dwRet++;
					m_vDriveLetters.push_back(driveLetter);
				}
			
			}
		}
	}
	return 0;
	
}
