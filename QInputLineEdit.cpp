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
	case Qt::Key_Right:
	case Qt::Key_Up:
	case Qt::Key_Down:
	{
		emit arrowKeyPressed(event);
		return;
	}


	default:
		return QLineEdit::keyPressEvent(event); 
	}
}
