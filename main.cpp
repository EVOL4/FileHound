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

	
	WCHAR xx[] = { L"z��hh de" };
	WCHAR xxx[] = { L"��d" };
	size_t xxxx[_MAX_PATH] = { 0 };
	//[0x00000000] = { Filename = L"$TxfLog.blf" Path = L"\\$RmMetadata\\$TxfLog\\" Filter = 0xe00000d000884862 ... }

	DWORD fiat = GetFileAttributes(L"C:\\");
	int dd = GetLastError();
	score_t sc = match_positions(xxx, xx, xxxx);
 	CDriveIndex* c = CreateIndex('H');

	wstring query(L"t");

	vector<SearchResultFile> results;

	clock_t startTime, endTime;
    startTime = clock();//��ʱ��ʼ
	c->Find(&query, NULL, &results, TRUE, -1);
	endTime = clock();//��ʱ����
	WCHAR time[MAX_PATH] = { 0 };
	int dTime = (int)(endTime - startTime) / CLOCKS_PER_SEC;
	wsprintf(time, L"Time is %d",dTime);
	MessageBox(NULL, time, time, MB_OK);

	

 	delete c;

	int x = 0;

//     QApplication a(argc, argv);
//     int iRet = 0;
//     QWidget* empty_parent = new QWidget;
//     MainWindow* w = new MainWindow(empty_parent); //������������Ϊempty_parent,�������������Ͳ������ͼ��
// 
//     //�����һ�����ڹرյ�ʱ�򲻹ر�Ӧ�ó���,���Ե��� a.quit()����ȷ�˳�
//     a.setQuitOnLastWindowClosed(false);
//     //ȥ�������ͱ߿�
//     w->setWindowFlag(Qt::FramelessWindowHint,true);
//     w->setWindowFlag(Qt::NoDropShadowWindowHint,true);
//     w->setAttribute(Qt::WA_TranslucentBackground);
//     w->show();
//     w->move ((QApplication::desktop()->width() - w->width())/2,(QApplication::desktop()->height() - w->height())/1.4);
// 
//     iRet = a.exec();
// 
//     delete empty_parent;  //������������ʱ,�Ӵ���Ҳ�ᱻ��ȷ����
//     return iRet;
   // qAppΪָ��a��ȫ��ָ��
}


/*
���Ի��򡱣��Ի���һ���ǲ���Ϊ�����������ֵģ����������������û�жԻ����λ�õģ�ָ���Ի����parent���ԣ��������Ͳ������ͼ�ꡣ
����㲻ָ��������Ի���ͳ�Ϊ���������ˣ������������ͼ�ꡣ
����������ԣ��Ϳ���ʵ�����ǵĴ����Ƿ���Ҫ���������ϳ��֡�
�������Ҫ�ľ��ǣ�parent����ָ���˸����ڣ���������deleteʱ��Qt���Ա�֤�����Ӵ��ڶ��ᱻ��ȷ��delete������֤���ǵ��ڴ氲ȫ��
ԭ�ģ�https://blog.csdn.net/ly305750665/article/details/53646134
*/
