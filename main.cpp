#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QtSql/QSqlDatabase>
#include <QDebug>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <Shldisp.h>

#include "match.h"
#include "MyDB.h"
#include "ChangeJrnl.h"
#include "Pinyin.h"
util*  util::_instance = NULL;


int main(int argc, char *argv[])
{
// 	WCHAR sub[] = { L"w" };
// 	WCHAR sub2[] = { L"wcm" };
// 	WCHAR sub3[] = { L"Wincmm" };
// 	WCHAR m[] = { L"WinContextMenu-master" };
// 
// 	WCHAR* a = sub;
// 	WCHAR* b = m;
//  	BOOL bRet = has_match(a, b);
// 	size_t len = wcslen(m);
// 	size_t * positions = (size_t *)malloc(len * sizeof(size_t));
// 	ZeroMemory(positions, len * sizeof(size_t));
// 	score_t score = match_positions(a, b, positions);
// 	score = match_positions(sub2, b, positions);
// 	score = match_positions(sub3, b, positions);
// 
// 
// 	for (size_t i=0;i<len;i++)
// 	{
// 		size_t x = positions[i];
// 		size_t y = x;
// 	}

	
// 	WCHAR xx[] = { L"腾讯QQ.lnk" };
// 	WCHAR xxx[] = { L"txqq" };
// 	size_t xxxx[_MAX_PATH] = { 0 };
// 
// 	DWORD fiat = GetFileAttributes(L"C:\\");
// 	int dd = GetLastError();
// 	score_t sc = match_positions(xxx, xx, xxxx);
//  	CDriveIndex* c = CreateIndex('C');
// 
// 	wstring query(L"txqq");
// 
// 	vector<SearchResultFile> results;
// 
// 	clock_t startTime, endTime;
//     startTime = clock();//计时开始
// 	c->Find(&query, NULL, &results, TRUE, -1);
// 	endTime = clock();//计时结束
// 	WCHAR time[MAX_PATH] = { 0 };
// 	int dTime = (int)(endTime - startTime) / CLOCKS_PER_SEC;
// 	wsprintf(time, L"Time is %d",dTime);
// 	MessageBox(NULL, time, time, MB_OK);
// 
// 	
// 
//  	delete c;
// 
// 	int x = 0;

    QApplication a(argc, argv);
    int iRet = 0;
    QWidget* empty_parent = new QWidget;
    MainWindow* w = new MainWindow(empty_parent); //将父窗口设置为empty_parent,这样在任务栏就不会出现图标

    //在最后一个窗口关闭的时候不关闭应用程序,可以调用 a.quit()来正确退出
    a.setQuitOnLastWindowClosed(false);
    //去掉背景和边框
    w->setWindowFlag(Qt::FramelessWindowHint,true);
    w->setWindowFlag(Qt::NoDropShadowWindowHint,true);
    w->setAttribute(Qt::WA_TranslucentBackground);
    w->show();
    w->move ((QApplication::desktop()->width() - w->width())/2,(QApplication::desktop()->height() - w->height())/1.4);

    iRet = a.exec();

    delete empty_parent;  //当父窗口销毁时,子窗口也会被正确销毁
    return iRet;
   // qApp为指向a的全局指针
}


/*
“对话框”，对话框一般是不作为顶层容器出现的，因此在任务栏上是没有对话框的位置的，指定对话框的parent属性，任务栏就不会出现图标。
如果你不指定，这个对话框就成为顶层容器了，任务栏会出现图标。
利用这个特性，就可以实现我们的窗体是否需要在任务栏上出现。
另外很重要的就是，parent参数指明了父窗口，当父窗口delete时，Qt可以保证所有子窗口都会被正确的delete掉，保证我们的内存安全。
原文：https://blog.csdn.net/ly305750665/article/details/53646134
*/
