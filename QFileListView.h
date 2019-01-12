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
	void showResults(const QString & input);
	void keyEventForward(QKeyEvent *event);
	void clearResults();

private slots:
	void indexMouseOver(const QModelIndex &index);
	void indexMousePressed(const QModelIndex &index);

protected slots:
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
};

