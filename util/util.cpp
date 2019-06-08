#include "util.h"
#include <qfile.h>

#define MENU_ITEM_END 0x001

void util::sleep(int ElapsedTime)
{
	if (ElapsedTime < 0)
	{
		ElapsedTime = 0;
	}
	QTime t;
	t.start();
	while (t.elapsed() < ElapsedTime)
		QCoreApplication::processEvents();
}

util *util::getInstance()
{
	if (nullptr == _instance)
	{
		_instance = new util();
	}
	return _instance;
}

void util::SetUpTrayIcon()
{
	//��������ͼ��,�����Դ�󻹵�qmakeһ��,�鷳
	QIcon icon = QIcon(":/main.png");
	m_pCm = new QCustomMenu;

	m_pCm->addMenuItem(QIcon(":/end.png"), QString::fromWCharArray(L"����"), MENU_ITEM_END);

	connect(m_pCm,
		SIGNAL(menuItemClicked(int)),
		this,
		SLOT(onMenuItemClicked(int))
	);

	//todo:�ǵó������ʱ�����ڴ�,�Լ�ʵ�ֲ˵��Ķ���
	_GlobalSysTrayIcon = new QSystemTrayIcon(this);
	_GlobalSysTrayIcon->setIcon(icon);
	_GlobalSysTrayIcon->setToolTip("FileHound");
	//�������̵���¼�
	connect(_GlobalSysTrayIcon,
		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this,
		SLOT(on_activatedSysTrayIcon(QSystemTrayIcon::ActivationReason)));

	_GlobalSysTrayIcon->show();
}

void util::HideTrayIcon()
{
	_GlobalSysTrayIcon->hide();
}

SEARCH_RESULT_ITEM_DATA util::GetFileData(const QString path)
{
	SEARCH_RESULT_ITEM_DATA ret;
	QFileInfo fileInfo(path);
	if (!fileInfo.exists())
	{
		ret.fileName = "";
		ret.filePath = "";
		return ret;
	}
	if (fileInfo.isDir()) //��Ϊ
	{
		SHFILEINFO psfi;//todo:�����ļ���������ȡͼ��
		SHGetFileInfo(path.toStdWString().c_str(), FILE_ATTRIBUTE_DIRECTORY, &psfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_DISPLAYNAME);
		ret.filePath = path;
		ret.icon = QtWin::fromHICON(psfi.hIcon).toImage();  //todo:����ͼ��Ӧ���ɺ�̨�߳̽��,�������������,
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName); //����SHFILEINFO.szDisplayName����һ�����������ָ��,���Բ�������ȥ�ͷ��ڴ�
		ret.type = TYPE_DIRECTORY;
		DestroyIcon(psfi.hIcon);  //��Ҫ�ֶ��ͷ�ͼ���ڴ�
	}
	else if (fileInfo.isFile())
	{
		SHFILEINFO psfi;
		SHGetFileInfo(path.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &psfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_DISPLAYNAME);
		ret.filePath = path;
		ret.icon = QtWin::fromHICON(psfi.hIcon).toImage();  //todo:����ͼ��Ӧ���ɺ�̨�߳̽��,�������������
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName);
		ret.type = TYPE_FILE;
		DestroyIcon(psfi.hIcon);
	}

	return ret;
}

void util::setStyle(const QString& qssFile, QWidget* targetWidget)
{
	if ((!QFile::exists(qssFile)) || targetWidget == nullptr)
	{
		return;
	}
	QFile qss(qssFile);
	qss.open(QFile::ReadOnly);
	targetWidget->setStyleSheet(qss.readAll());
	qss.close();
}

HICON util::getIcon(const WCHAR* szPath, bool isDir)
{
	if (_mapIcon == NULL)
	{
		_mapIcon = new unordered_map<wstring, HICON>;
	}

	HICON hIcon = NULL;
	WCHAR szExt[_MAX_EXT];
	_wsplitpath(szPath, NULL, NULL, NULL, szExt); //��ȡ�ļ���չ��
	std::wstring extension(szExt);

	SHFILEINFO psfi;

	if (isDir)
	{
		extension = wstring(L"folder");
	}
	else if (wcslen(szExt) == 0)//û����չ��
	{
		SHGetFileInfo(szPath, 0, &psfi, sizeof(SHFILEINFO), SHGFI_ICON);
		hIcon = psfi.hIcon;
		(*_mapIcon)[wstring(szPath)] = hIcon;
		return hIcon;
	}

	if (extension.compare(L".exe") == 0 || extension.compare(L".lnk") == 0)
	{
		SHGetFileInfo(szPath, 0, &psfi, sizeof(SHFILEINFO), SHGFI_ICON);
		hIcon = psfi.hIcon;
		(*_mapIcon)[wstring(szPath)] = hIcon;
	}
	else
	{
		if (NULL != _mapIcon)
		{
			auto i = _mapIcon->find(extension);
			if (i != _mapIcon->end())
			{
				hIcon = i->second;
			}
			else
			{
				if (isDir)
				{
					SHGetFileInfo(extension.data(), FILE_ATTRIBUTE_DIRECTORY, &psfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);
				}
				else
				{
					SHGetFileInfo(extension.data(), FILE_ATTRIBUTE_NORMAL, &psfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);
					
				}
				hIcon = psfi.hIcon;
				if (NULL != hIcon)
				{
					(*_mapIcon)[extension] = hIcon;
				}
			}
		}
	}
	
	assert(hIcon != NULL);
	
	return hIcon;
}

util::util()
	:m_pCm(NULL)
{
	HMODULE hUser32 = LoadLibraryA("User32.dll");
	if (hUser32 == NULL)
	{
		//todo: ��־����
	}
	_SwitchToThisWindow = (pfnSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
	if (_SwitchToThisWindow == NULL)
	{
		//todo: ��־����
	}
}

util::~util()
{
	_GlobalSysTrayIcon->hide();
	delete _GlobalSysTrayIcon;
	if (_mapIcon)
	{
		for (auto i:*_mapIcon)
		{
			HICON hIcon = i.second;
			if (hIcon)
			{
				DestroyIcon(hIcon);
			}
		}
		delete _mapIcon;
		_mapIcon = NULL;
	}
	//todo: FreeLibrary
}

void util::on_activatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	QPoint pos = QCursor::pos();
	QRect rect(pos.x() - m_pCm->width() + 10, pos.y() - m_pCm->height() + 10, m_pCm->width(), m_pCm->height());

	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
		//��������ͼ��
		break;
	case QSystemTrayIcon::DoubleClick:
		//˫������ͼ��
		break;
	case QSystemTrayIcon::Context:
		//�Ҽ������
		m_pCm->setGeometry(rect);
		m_pCm->show();
		break;

	default:
		break;
	}
}

void util::onMenuItemClicked(int ItemID)
{
	switch (ItemID)
	{
	case MENU_ITEM_END:
	{
		_GlobalSysTrayIcon->hide();
		m_pCm->close();
		qApp->exit(0);
		break;
	}
	default:
	{
		break;
	}
	}
}