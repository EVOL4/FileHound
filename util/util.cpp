#include "util.h"
#include <qfile.h>

void util::sleep(int ElapsedTime)
{
    if(ElapsedTime<0)
    {
        ElapsedTime = 0;
    }
    QTime t;
    t.start();
    while(t.elapsed()<ElapsedTime)
        QCoreApplication::processEvents();
}

util *util::getInstance()
{
    if(nullptr==_instance)
    {
        _instance=new util();
    }
    return _instance;
}

void util::SetUpTrayIcon()
{
	//创建托盘图标,添加资源后还得qmake一下,麻烦
	QIcon icon = QIcon(":/icon/main_icon/main.png");
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
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName);
		DestroyIcon(psfi.hIcon);
	}
	else if (fileInfo.isFile())
	{
		SHFILEINFO psfi;
		SHGetFileInfo(path.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &psfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_DISPLAYNAME);
		ret.filePath = path;
		ret.icon = QtWin::fromHICON(psfi.hIcon).toImage();  //todo:加载图标应交由后台线程解决,这里仅作测试用
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName);
		DestroyIcon(psfi.hIcon);
	}
	
	return ret;
}



 void util::setStyle(const QString& qssFile, QWidget* targetWidget)
 {
	 if ((!QFile::exists(qssFile)) || targetWidget==nullptr)
	 {
		 return;
	 }
	 QFile qss(qssFile);
	 qss.open(QFile::ReadOnly);
	 targetWidget->setStyleSheet(qss.readAll());
	 qss.close();
 }

util::util()
{
    HMODULE hUser32 = LoadLibraryA("User32.dll");
    if(hUser32==NULL)
    {
        //todo: 日志跟踪
    }
    _SwitchToThisWindow = (pfnSWITCHTOTHISWINDOW)GetProcAddress(hUser32,"SwitchToThisWindow");
    if(_SwitchToThisWindow==NULL)
    {
         //todo: 日志跟踪
    }
}

util::~util()
{
	_GlobalSysTrayIcon->hide();
	delete _GlobalSysTrayIcon;
    //todo: FreeLibrary
}

void util::on_activatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
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
		break;

	default:
		break;
	}
}

