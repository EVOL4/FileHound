#ifndef QWINHOTKEY_H
#define QWINHOTKEY_H
#include <QAbstractNativeEventFilter>
#include <QAbstractEventDispatcher>
#include <QWidget>
#include <QPair>
#include <QList>
#include "util/util.h"

class QWinHotkey:public QObject,public QAbstractNativeEventFilter  //欲使用connect()来处理信号,必须继承QObject,而且要放在最前面
{
    Q_OBJECT
public:
    QWinHotkey();

    quint32 setShortcut(const QKeySequence &shortcut); // 为了保持与QAction->setShortcut()风格一致,但返回值为一个热键的id
    //bool  unsetShortcut(const quint32 shortcutId); //接受目标热键id,用于注销目标热键
     bool nativeEventFilter(const QByteArray & eventType,void * message, long * result); // 用于处理windows发送给我们的消息

private:

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);  // 用于将alt/ctrl/shift/win键从qt格式码转换为windows格式码
    quint32 nativeKeycode(Qt::Key key); // 用于将组合键的另一个键的qt格式vk转换为windows格式的vk
    void activateShortcut(quint32 nativeKey, quint32 nativeMods); //当windows向我们发送WM_HOTKEY时,说明热键被激活,此时应调用该函数来emit 信号 activated()
    bool registerShortcut(quint32 nativeKey, quint32 nativeMods); // 用于向windows注册热键
    //bool unregisterShortcut(quint32 nativeKey, quint32 nativeMods); // 用于向windows注销热键
   // void destroyShortcuts(); //在类被销毁时调用,注销所有热键


private:
    QList< QPair<quint32, quint32> > shortcuts; //考虑到快捷键不会很多,使用链表存储,每个节点是一个pair对象,pair对象由<任意键key,控制键key>组成


signals:
    void activated(); //当热键被激活时,此信号发出,用户可选择接收此信号来进行进一步的操作


};

#endif // QWINHOTKEY_H
