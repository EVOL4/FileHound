#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include "ChangeJrnlDB.h"

util*  util::_instance = NULL;

int main(int argc, char *argv[])
{


    QApplication a(argc, argv);
    int iRet = 0;
    QWidget* empty_parent = new QWidget;
    MainWindow* w = new MainWindow(empty_parent); //������������Ϊempty_parent,�������������Ͳ������ͼ��

    //�����һ�����ڹرյ�ʱ�򲻹ر�Ӧ�ó���,���Ե��� a.quit()����ȷ�˳�
    a.setQuitOnLastWindowClosed(false);
    //ȥ�������ͱ߿�
    w->setWindowFlag(Qt::FramelessWindowHint,true);
    w->setWindowFlag(Qt::NoDropShadowWindowHint,true);
    w->setAttribute(Qt::WA_TranslucentBackground);
    w->show();
    w->move ((QApplication::desktop()->width() - w->width())/2,(QApplication::desktop()->height() - w->height())/1.4);

    iRet = a.exec();

    delete empty_parent;  //������������ʱ,�Ӵ���Ҳ�ᱻ��ȷ����
    return iRet;
   // qAppΪָ��a��ȫ��ָ��
}


/*
���Ի��򡱣��Ի���һ���ǲ���Ϊ�����������ֵģ����������������û�жԻ����λ�õģ�ָ���Ի����parent���ԣ��������Ͳ������ͼ�ꡣ
����㲻ָ��������Ի���ͳ�Ϊ���������ˣ������������ͼ�ꡣ
����������ԣ��Ϳ���ʵ�����ǵĴ����Ƿ���Ҫ���������ϳ��֡�
�������Ҫ�ľ��ǣ�parent����ָ���˸����ڣ���������deleteʱ��Qt���Ա�֤�����Ӵ��ڶ��ᱻ��ȷ��delete������֤���ǵ��ڴ氲ȫ��
ԭ�ģ�https://blog.csdn.net/ly305750665/article/details/53646134
*/
