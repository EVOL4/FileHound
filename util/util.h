#ifndef UTIL_H
#define UTIL_H

#include<qt_windows.h>
#include<QWidget>
#include<QCoreApplication>
#include<QTime>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QtWinExtras/QtWin>
#include <QMetaType>
#include <QDebug>
#include <QApplication>
#include <unordered_map>

#include <QTimer>

#include <assert.h>
#include "MyConfig.h"
#include "CustomMenu.h"

using namespace std;

typedef void(WINAPI *pfnSWITCHTOTHISWINDOW)(HWND,BOOL);

typedef enum FileType
{
	TYPE_FILE,
	TYPE_DIRECTORY,
	TYPE_HEADER
}FILE_TYPE;

typedef struct _SearchResult
{
	QImage icon;
	QString fileName;
	QString filePath;
	enum FileType type;
	//type file/directory/分隔计数
}SEARCH_RESULT_ITEM_DATA;



Q_DECLARE_METATYPE(SEARCH_RESULT_ITEM_DATA)



//工具类,单例实现,用于提供一些全局使用的function
class util:public QWidget
{
	Q_OBJECT

private:
	static util* _instance;
	static unordered_map<wstring, HICON>* _mapIcon; //存放图标的map
	QSystemTrayIcon*  _GlobalSysTrayIcon;
	QCustomMenu *  m_pCm;

public:

    pfnSWITCHTOTHISWINDOW _SwitchToThisWindow;
    void sleep(int ElapsedTime);
	void SetUpTrayIcon();
	void HideTrayIcon();
	static util* getInstance();
	static SEARCH_RESULT_ITEM_DATA GetFileData(const QString path);
	static void setStyle(const QString& qssFile,QWidget* targetWidget);
	static HICON getIcon(const WCHAR* szPath, bool isDir);

private:
    util();
    ~util();

private slots:
	void on_activatedSysTrayIcon(QSystemTrayIcon::ActivationReason reason);  
	void onMenuItemClicked(int ItemID);
};


#endif // UTIL_H


#define DELETE_IF_NOT_NULL(o) if (o!=NULL) { delete o; o=NULL;}

