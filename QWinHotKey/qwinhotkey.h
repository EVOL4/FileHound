#ifndef QWINHOTKEY_H
#define QWINHOTKEY_H
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <QPair>
#include <QList>
#include "util/util.h"

class QWinHotKey:public QObject,public QAbstractNativeEventFilter  //��ʹ��connect()�������ź�,����̳�QObject,����Ҫ������ǰ��
{
    Q_OBJECT
public:
    QWinHotKey();

    quint32 setShortcut(const QKeySequence &shortcut); // Ϊ�˱�����QAction->setShortcut()���һ��,������ֵΪһ���ȼ���id
    //bool  unsetShortcut(const quint32 shortcutId); //����Ŀ���ȼ�id,����ע��Ŀ���ȼ�
     bool nativeEventFilter(const QByteArray & eventType,void * message, long * result); // ���ڴ���windows���͸����ǵ���Ϣ

private:

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);  // ���ڽ�alt/ctrl/shift/win����qt��ʽ��ת��Ϊwindows��ʽ��
    quint32 nativeKeycode(Qt::Key key); // ���ڽ���ϼ�����һ������qt��ʽvkת��Ϊwindows��ʽ��vk
    void activateShortcut(quint32 nativeKey, quint32 nativeMods); //��windows�����Ƿ���WM_HOTKEYʱ,˵���ȼ�������,��ʱӦ���øú�����emit �ź� activated()
    bool registerShortcut(quint32 nativeKey, quint32 nativeMods); // ������windowsע���ȼ�
    //bool unregisterShortcut(quint32 nativeKey, quint32 nativeMods); // ������windowsע���ȼ�
   // void destroyShortcuts(); //���౻����ʱ����,ע�������ȼ�


private:
    QList< QPair<quint32, quint32> > shortcuts; //���ǵ���ݼ�����ܶ�,ʹ������洢,ÿ���ڵ���һ��pair����,pair������<�����key,���Ƽ�key>���


signals:
    void activated(); //���ȼ�������ʱ,���źŷ���,�û���ѡ����մ��ź������н�һ���Ĳ���


};

#endif // QWINHOTKEY_H
