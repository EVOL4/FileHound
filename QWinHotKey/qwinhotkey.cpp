#include "QWinHotkey.h"

QWinHotkey::QWinHotkey()
{
    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
}


/*
  解析传入的QKeySequence,然后调用RegisterHotKey()注册热键,若成功,将记录加入链表,并返回其id
 */
quint32 QWinHotkey::setShortcut(const QKeySequence &shortcut)
{
    quint32 shortcutId = 0;



    Qt::KeyboardModifiers allMods = Qt::ShiftModifier| Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    //比如传入"Ctrl+b" 那么shortcut[0] == Qt::ControlModifier|Qt::Key_B,这里获得Key_B
    Qt::Key key = shortcut.isEmpty()? Qt::Key(0) : Qt::Key((shortcut[0]^allMods)&shortcut[0]);
    //这里获得Modifier,可能不止有一个
    Qt::KeyboardModifiers modifiers = shortcut.isEmpty() ? Qt::KeyboardModifiers(0) : Qt::KeyboardModifiers(shortcut[0] & allMods);

    //将qt格式的key和modifier转换为windows能识别的格式
    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(modifiers);

    bool bRet = registerShortcut(nativeKey, nativeMods); //注册热键
    if(bRet){
        this->shortcuts.append(qMakePair(nativeKey,nativeMods));
        shortcutId = nativeKey^nativeMods;
    }
    else
    {
        MessageBoxA(NULL,"注册失败","oops",MB_OK);
    }
    return shortcutId;
}

bool QWinHotkey::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    bool bRet = false;

    MSG* msg = static_cast<MSG*>(message);
    if(msg->message==WM_HOTKEY)
    {
        quint32 keycode = HIWORD(msg->lParam);
        quint32 modifiers = LOWORD(msg->lParam);
        activateShortcut(keycode,modifiers);
        bRet= true;
    }

    return bRet;
}

quint32 QWinHotkey::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    quint32 native = 0;
    if(modifiers&Qt::ShiftModifier)
        native|=MOD_SHIFT;
    if(modifiers&Qt::ControlModifier)
        native|=MOD_CONTROL;
    if(modifiers&Qt::AltModifier)
        native|=MOD_ALT;
    if(modifiers&Qt::MetaModifier)
        native|=MOD_WIN;

    return native;
}

quint32 QWinHotkey::nativeKeycode(Qt::Key key)
{
  switch (key)
        {
        case Qt::Key_Escape:
            return VK_ESCAPE;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            return VK_TAB;
        case Qt::Key_Backspace:
            return VK_BACK;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return VK_RETURN;
        case Qt::Key_Insert:
            return VK_INSERT;
        case Qt::Key_Delete:
            return VK_DELETE;
        case Qt::Key_Pause:
            return VK_PAUSE;
        case Qt::Key_Print:
            return VK_PRINT;
        case Qt::Key_Clear:
            return VK_CLEAR;
        case Qt::Key_Home:
            return VK_HOME;
        case Qt::Key_End:
            return VK_END;
        case Qt::Key_Left:
            return VK_LEFT;
        case Qt::Key_Up:
            return VK_UP;
        case Qt::Key_Right:
            return VK_RIGHT;
        case Qt::Key_Down:
            return VK_DOWN;
        case Qt::Key_PageUp:
            return VK_PRIOR;
        case Qt::Key_PageDown:
            return VK_NEXT;
        case Qt::Key_F1:
            return VK_F1;
        case Qt::Key_F2:
            return VK_F2;
        case Qt::Key_F3:
            return VK_F3;
        case Qt::Key_F4:
            return VK_F4;
        case Qt::Key_F5:
            return VK_F5;
        case Qt::Key_F6:
            return VK_F6;
        case Qt::Key_F7:
            return VK_F7;
        case Qt::Key_F8:
            return VK_F8;
        case Qt::Key_F9:
            return VK_F9;
        case Qt::Key_F10:
            return VK_F10;
        case Qt::Key_F11:
            return VK_F11;
        case Qt::Key_F12:
            return VK_F12;
        case Qt::Key_F13:
            return VK_F13;
        case Qt::Key_F14:
            return VK_F14;
        case Qt::Key_F15:
            return VK_F15;
        case Qt::Key_F16:
            return VK_F16;
        case Qt::Key_F17:
            return VK_F17;
        case Qt::Key_F18:
            return VK_F18;
        case Qt::Key_F19:
            return VK_F19;
        case Qt::Key_F20:
            return VK_F20;
        case Qt::Key_F21:
            return VK_F21;
        case Qt::Key_F22:
            return VK_F22;
        case Qt::Key_F23:
            return VK_F23;
        case Qt::Key_F24:
            return VK_F24;
        case Qt::Key_Space:
            return VK_SPACE;
        case Qt::Key_Asterisk:
            return VK_MULTIPLY;
        case Qt::Key_Plus:
            return VK_ADD;
        case Qt::Key_Comma:
            return VK_SEPARATOR;
        case Qt::Key_Minus:
            return VK_SUBTRACT;
        case Qt::Key_Slash:
            return VK_DIVIDE;
        case Qt::Key_MediaNext:
            return VK_MEDIA_NEXT_TRACK;
        case Qt::Key_MediaPrevious:
            return VK_MEDIA_PREV_TRACK;
        case Qt::Key_MediaPlay:
            return VK_MEDIA_PLAY_PAUSE;
        case Qt::Key_MediaStop:
            return VK_MEDIA_STOP;
            // couldn't find those in VK_*
            //case Qt::Key_MediaLast:
            //case Qt::Key_MediaRecord:
        case Qt::Key_VolumeDown:
            return VK_VOLUME_DOWN;
        case Qt::Key_VolumeUp:
            return VK_VOLUME_UP;
        case Qt::Key_VolumeMute:
            return VK_VOLUME_MUTE;

            // numbers
        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
            return static_cast<quint32>(key);

            // letters
        case Qt::Key_A:
        case Qt::Key_B:
        case Qt::Key_C:
        case Qt::Key_D:
        case Qt::Key_E:
        case Qt::Key_F:
        case Qt::Key_G:
        case Qt::Key_H:
        case Qt::Key_I:
        case Qt::Key_J:
        case Qt::Key_K:
        case Qt::Key_L:
        case Qt::Key_M:
        case Qt::Key_N:
        case Qt::Key_O:
        case Qt::Key_P:
        case Qt::Key_Q:
        case Qt::Key_R:
        case Qt::Key_S:
        case Qt::Key_T:
        case Qt::Key_U:
        case Qt::Key_V:
        case Qt::Key_W:
        case Qt::Key_X:
        case Qt::Key_Y:
        case Qt::Key_Z:
            return static_cast<quint32>(key);

        default:
            return 0;
        }

}

void QWinHotkey::activateShortcut(quint32 nativeKey, quint32 nativeMods)
{

    for(QPair<quint32,quint32> i:shortcuts)
    {
        if(i==qMakePair(nativeKey,nativeMods))
        {
            //todo:这里应该检测目标热键是否被激活
            emit this->activated();  //怎样识别是哪个热键被激活了呢?若能将id借由信号发出就好了
            break;
        }
    }

}

bool QWinHotkey::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    return RegisterHotKey(NULL,nativeKey^nativeMods,nativeMods,nativeKey);
}
