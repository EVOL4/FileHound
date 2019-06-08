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
	//创建托盘图标,添加资源后还得qmake一下,麻烦
	QIcon icon = QIcon(":/main.png");
	m_pCm = new QCustomMenu;

	m_pCm->addMenuItem(QIcon(":/end.png"), QString::fromWCharArray(L"结束"), MENU_ITEM_END);

	connect(m_pCm,
		SIGNAL(menuItemClicked(int)),
		this,
		SLOT(onMenuItemClicked(int))
	);

	//todo:记得程序结束时回收内存,以及实现菜单的定制
	_GlobalSysTrayIcon = new QSystemTrayIcon(this);
	_GlobalSysTrayIcon->setIcon(icon);
	_GlobalSysTrayIcon->setToolTip("FileHound");
	//处理托盘点击事件
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
	if (fileInfo.isDir()) //若为
	{
		SHFILEINFO psfi;//todo:根据文件类型来读取图标
		SHGetFileInfo(path.toStdWString().c_str(), FILE_ATTRIBUTE_DIRECTORY, &psfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_DISPLAYNAME);
		ret.filePath = path;
		ret.icon = QtWin::fromHICON(psfi.hIcon).toImage();  //todo:加载图标应交由后台线程解决,这里仅作测试用,
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName); //由于SHFILEINFO.szDisplayName里是一个数组而不是指针,所以不用我们去释放内存
		ret.type = TYPE_DIRECTORY;
		DestroyIcon(psfi.hIcon);  //需要手动释放图标内存
	}
	else if (fileInfo.isFile())
	{
		SHFILEINFO psfi;
		SHGetFileInfo(path.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &psfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_DISPLAYNAME);
		ret.filePath = path;
		ret.icon = QtWin::fromHICON(psfi.hIcon).toImage();  //todo:加载图标应交由后台线程解决,这里仅作测试用
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
	_wsplitpath(szPath, NULL, NULL, NULL, szExt); //获取文件扩展名
	std::wstring extension(szExt);

	SHFILEINFO psfi;

	if (isDir)
	{
		extension = wstring(L"folder");
	}
	else if (wcslen(szExt) == 0)//没有扩展名
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
		//todo: 日志跟踪
	}
	_SwitchToThisWindow = (pfnSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
	if (_SwitchToThisWindow == NULL)
	{
		//todo: 日志跟踪
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
		//单击托盘图标
		break;
	case QSystemTrayIcon::DoubleClick:
		//双击托盘图标
		break;
	case QSystemTrayIcon::Context:
		//右键被点击
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