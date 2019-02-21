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
	case Qt::Key_Left:  //不希望lineedit接收左右箭头按键
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
