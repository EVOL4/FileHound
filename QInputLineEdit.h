#pragma once
#include <qlineedit.h>

class QInputLineEdit :public QLineEdit
{
	Q_OBJECT
public:
	explicit QInputLineEdit(QWidget *parent = 0);
	~QInputLineEdit();

protected:
	virtual void QInputLineEdit::keyPressEvent(QKeyEvent *event)  override;

signals:
	void arrowKeyPressed(QKeyEvent *event);
};

