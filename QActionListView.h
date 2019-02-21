#pragma once
#include <QListView>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QStandardItemModel>
#include "util/util.h"
#include "QSearchResultDelegate.h"

#include <shldisp.h>
#include <ExDisp.h>
#include <ShObjIdl.h>
#include <ShlGuid.h>
#include <Shlobj.h>

class OleInitializer
{
public:
	OleInitializer() {
		 OleInitialize(NULL);
	}
		
	~OleInitializer() {
		 OleUninitialize();
	}
};

class QActionListView:public QListView
{
	Q_OBJECT

public:
	explicit QActionListView(QWidget *parent = 0);
	~QActionListView();
	bool GetActions(QModelIndex& index);
	void DisplayActions();


public slots:
	void showResults(const QString & input);
	void clearResult();

private slots:
	void indexMouseOver(const QModelIndex &index);
	void indexMousePressed(const QModelIndex &index);

private:


	QStandardItemModel* m_model;
	QSearchResultDelegate* m_Delegate;
	OleInitializer *m_ole;
	QModelIndex m_prevIndex;
	HMENU m_hMenu;
	IContextMenu * m_cm;
	IContextMenu2* m_cm2;
	IContextMenu3* m_cm3;


};

#define OFFSET_FIRST_COMMAND 0x0
#define OFFSET_LAST_COMMAND 0x7FF


