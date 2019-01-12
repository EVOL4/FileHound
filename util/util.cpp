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
	//��������ͼ��,�����Դ�󻹵�qmakeһ��,�鷳
	QIcon icon = QIcon(":/icon/main_icon/main.png");
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
		ret.fileName = QString::fromWCharArray(psfi.szDisplayName);
		ret.type = TYPE_DIRECTORY;
		DestroyIcon(psfi.hIcon);
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
        //todo: ��־����
    }
    _SwitchToThisWindow = (pfnSWITCHTOTHISWINDOW)GetProcAddress(hUser32,"SwitchToThisWindow");
    if(_SwitchToThisWindow==NULL)
    {
         //todo: ��־����
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
		//��������ͼ��
		break;
	case QSystemTrayIcon::DoubleClick:

		//˫������ͼ��

		break;
	case QSystemTrayIcon::Context:
		//�Ҽ������
		break;

	default:
		break;
	}
}

