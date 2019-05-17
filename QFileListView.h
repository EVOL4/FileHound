#pragma once
#include <QListView>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QStandardItemModel>
#include "util/util.h"
#include "QSearchResultDelegate.h"
#include "QActionListView.h"

#include <shldisp.h>
#include <ExDisp.h>
#include <ShObjIdl.h>
#include <ShlGuid.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <vector>
#include <stack>

class QFileListView :public QListView
{
	Q_OBJECT


public:
	explicit QFileListView(QWidget *parent=0);

	static QFileListView* currentList;


	~QFileListView();



private:
	QStandardItemModel* m_filesModel;
	QStandardItemModel* m_actionsModel;
	QStandardItemModel* m_currentModel;
	QSearchResultDelegate* m_Delegate;
	uint m_viewMode;
	uint m_depth;

	OleInitializer *m_ole;
	QModelIndex m_prevIndex;
	HMENU m_hMenu;
	IContextMenu * m_cm;
	IContextMenu2* m_cm2;
	IContextMenu3* m_cm3;

	std::stack<HMENU>m_CollapseStack;


	void SwitchMode(enum FH::ViewMode mode);
	void Expand();
	void Collapse();

	void ClearActionString();

	bool QueryContextMenu(QModelIndex& index);
	void ParseMenu(std::vector<FH::MenuItemInfo>&actionList,HMENU hTargetMenu);
	void ShowMenu(HMENU hTargetMenu);

public slots:
	void showResults(const QString & input);
	void keyEventForward(QKeyEvent *event);
	void clearResults();

private slots:
	void indexMouseOver(const QModelIndex &index);
	void indexMousePressed(const QModelIndex &index);

protected :
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
	
	
};

#define OFFSET_FIRST_COMMAND 0x0
#define OFFSET_LAST_COMMAND 0x7FF