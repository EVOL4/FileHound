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

protected :
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
	//bool nativeEvent(const QByteArray &eventType, void *message, long *result) override; https://blog.csdn.net/xiezhongyuan07/article/details/83585100 
	//在listview中接收WM_JOURNALCHANGED还是在主窗口中接收?想了想让主窗口接收比较好
};

