#pragma once
#include <QListView>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QStandardItemModel>
#include <QThread>
#include "util/util.h"
#include "QSearchResultDelegate.h"
#include "QActionListView.h"

#include <shldisp.h>
#include <ExDisp.h>
#include <ShObjIdl.h>
#include <ShlGuid.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <QMutex>
#include <vector>
#include <stack>
#include "MyDB.h"

extern MyDB* _db;
using namespace std;


typedef struct _ListViewItem
{
	bool isDir;
	wstring fullPath;
	QPersistentModelIndex index;
	//type file/directory/分隔计数
}ListViewItem;


class Loader :public QThread
{
	Q_OBJECT
public:

	Loader()
	{
		m_taskList = NULL;
		shouldBreak = false;
	}

	bool SubmitTaskList(vector<ListViewItem>* taskList)
	{
		if (taskList&&(taskList->size()>0))
		{
			m_taskList = taskList;
			return true;
		}

		return false;
	}

	void Break()
	{
		m_mutex.lock();
		shouldBreak = true;
		m_mutex.unlock();
	}



protected:
	void run() override {

		m_mutex.lock();
		shouldBreak = false;
		m_mutex.unlock();


		for (auto i : (*m_taskList))
		{
			HICON hIcon = util::getIcon(i.fullPath.data(), i.isDir);
			assert(hIcon != NULL);
			if (shouldBreak)
			{
				break;
			}
			emit imageReady(i.index, hIcon);  //todo:线程被终止时,应删除未被处理的HICON
		}
		if (m_taskList)
		{
			delete m_taskList;
			m_taskList = NULL;
		}
	}



private:
	vector<ListViewItem>* m_taskList;
	volatile bool shouldBreak;
	QMutex m_mutex;
signals:
	void imageReady(QPersistentModelIndex index, HICON hIcon);
};


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
	vector<SearchResultFile> m_results;
	QMutex m_mutex;

	Loader*  m_loader;

	void SwitchMode(enum FH::ViewMode mode);
	void Expand();
	void Collapse();

	void ClearActionString();

	bool QueryContextMenu(QModelIndex& index);
	void parseMenu(std::vector<FH::MenuItemInfo>&actionList,HMENU hTargetMenu);
	void ShowMenu(HMENU hTargetMenu);

public slots:
	void showResults(const QString & input);
	void keyEventForward(QKeyEvent *event);
	void clearResults();
	void on_imageReady(QPersistentModelIndex index, HICON hIcon);
private slots:
	void indexMouseOver(const QModelIndex &index);
	void indexMousePressed(const QModelIndex &index);

protected :
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
	
	
};




#define OFFSET_FIRST_COMMAND 0x0
#define OFFSET_LAST_COMMAND 0x7FF