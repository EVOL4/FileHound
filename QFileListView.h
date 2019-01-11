#pragma once
#include <QListView>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QStandardItemModel>
#include "util/util.h"
#include "QSearchResultDelegate.h"

class QFileListView :public QListView
{
	Q_OBJECT


public:
	explicit QFileListView(QWidget *parent=0);
	~QFileListView();

private:
	QStandardItemModel* m_model;
	QSearchResultDelegate* m_Delegate;

public slots:
	void showResults();
	void keyEventForward(QKeyEvent *event);
private slots:
	void indexMouseOver(const QModelIndex &index);
	void indexMousePressed(const QModelIndex &index);
};

