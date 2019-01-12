#include <QKeyEvent>
#include "mainwindow.h"
#include "QInputLineEdit.h"


QInputLineEdit::QInputLineEdit(QWidget *parent):QLineEdit(parent)
{

}


QInputLineEdit::~QInputLineEdit()
{

}

void QInputLineEdit::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Left:  //��ϣ��lineedit�������Ҽ�ͷ����
	{
		emit arrowKeyPressed(event);
		return;
	}
	case Qt::Key_Right:
	{
		emit arrowKeyPressed(event);
		return;
	}
	default:
		return QLineEdit::keyPressEvent(event);
	}
}
