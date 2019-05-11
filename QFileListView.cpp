#include "QFileListView.h"

QFileListView* QFileListView::currentList = NULL;

QFileListView::QFileListView(QWidget *parent/*=0*/) :QListView(parent), m_hMenu(NULL), m_cm(NULL), m_cm2(NULL), m_cm3(NULL), m_depth(0)
{
	auto shadowEffectCopy = new QGraphicsDropShadowEffect();
	shadowEffectCopy->setOffset(0, 0);
	shadowEffectCopy->setColor(QColor(90, 78, 85));
	shadowEffectCopy->setBlurRadius(10);

	this->setGraphicsEffect(shadowEffectCopy);
	this->setMinimumHeight(498);
	this->setMaximumHeight(498);
	this->setMouseTracking(true);
	this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);//解决多出的空白,以及自动下滚时滚到尾部的问题
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setTabKeyNavigation(true);
	this->setFocusPolicy(Qt::NoFocus);
	util::setStyle(":/listview.qss", this);

	m_Delegate = new QSearchResultDelegate(this);
	m_filesModel = new QStandardItemModel();
	m_actionsModel = new QStandardItemModel();
	m_currentModel = m_filesModel;
	m_viewMode = FH::MODE_FILES;
	m_ole = new OleInitializer();

	this->setModel(m_currentModel);
	this->setItemDelegate(m_Delegate);

	connect(this, SIGNAL(entered(const QModelIndex &)), this, SLOT(indexMouseOver(const QModelIndex &)));//当鼠标从items上划过时,将所选项设置为鼠标指向的index,
	//这将触发委托类的paint()里的if判断,使得所选项的背景上色,让用户看见
	connect(this, SIGNAL(pressed(const QModelIndex &)), this, SLOT(indexMousePressed(const QModelIndex &)));

	QFileListView::currentList = this;
}

QFileListView::~QFileListView()
{
	DELETE_IF_NOT_NULL(m_Delegate);
	DELETE_IF_NOT_NULL(m_filesModel);
	DELETE_IF_NOT_NULL(m_actionsModel);
}

void QFileListView::SwitchMode(enum FH::ViewMode mode)
{
	m_Delegate->SwitchMode(mode);

	switch (mode)
	{
	case FH::MODE_FILES:
	{
		m_currentModel = m_filesModel;

		m_viewMode = mode;
		break;
	}
	case FH::MODE_ACTIONS:
	{
		m_currentModel = m_actionsModel;
		m_viewMode = mode;
		break;
	}
	default:
		break;
	}
	this->setModel(m_currentModel);
}

void QFileListView::showResults(const QString & input)
{
	m_currentModel->clear();

	for (int i = 0; i < 100; i++)
	{
		//QStandardItemModel::clear()将自动回收其拥有的所有item,所以我们只管申请,不管释放内存
		QStandardItem*  singleItem = new QStandardItem;//必须申请内存,局部内存将被销毁,到QSearchResultDelegate::paint()将接收到空的参数
		//SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"C:\\Users\\rye\\Desktop\\Understanding_the_LFH.pdf"));
		SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"C:\\Users\\rye\\Desktop\\囧.txt"));
		if (i == 50)
		{
			itemData.type = TYPE_HEADER;
		}
		singleItem->setData(QVariant::fromValue(itemData), Qt::UserRole + 1);
		m_currentModel->appendRow(singleItem);
	}
	//todo:处理搜索结果

	this->selectionModel()->setCurrentIndex(m_currentModel->index(0, 0), QItemSelectionModel::SelectCurrent);
	this->show();
}

void QFileListView::keyEventForward(QKeyEvent *event)
{
	unsigned int row = this->selectionModel()->currentIndex().row();
	unsigned int count = m_currentModel->rowCount();
	if (count == 0)
	{
		return;
	}
	switch (event->key()) {  //默认就接受上下键,但这里需要处理拉到头应返回尾端的问题,还要接受左右键
	case Qt::Key_Up:
	{
		if (row == 0)
		{
			row = count - 1;
		}
		else
		{
			row--;
		}
		this->selectionModel()->setCurrentIndex(m_currentModel->index(row, 0), QItemSelectionModel::SelectCurrent);
		break;
	}

	case Qt::Key_Down:
	{
		row = (row + 1) % count;
		this->selectionModel()->setCurrentIndex(m_currentModel->index(row, 0), QItemSelectionModel::SelectCurrent);
		break;
	}
	case Qt::Key_Left:
	{
		this->Collapse();
		break;
	}
	case Qt::Key_Right:
	{
		this->Expand();
		break;
	}
	default:
		return;
	}
}

void QFileListView::Expand()
{
	do
	{
		QModelIndex cur = this->currentIndex();
		if (!cur.isValid()) break;

		if (m_depth == 0)  //展开菜单
		{
			this->SwitchMode(FH::MODE_ACTIONS);
			if (this->QueryContextMenu(cur))  //todo:解析菜单挺花时间的,应该以异步的方式进行
			{
				m_depth++;
				this->ShowMenu(m_hMenu);
			}
		}
		else if (m_depth > 0) //展开子菜单
		{
			FH::MenuItemInfo menuItemInfo = cur.data(Qt::UserRole + 1).value<FH::MenuItemInfo>();
			if (menuItemInfo.hSubMenu)
			{
				m_depth++;
				this->ShowMenu(menuItemInfo.hSubMenu);
			}
		}
	} while (false);
}

void QFileListView::Collapse()
{
	if (m_depth > 0)
	{
		m_depth--;

		if (m_depth == 0)
		{
			this->clearResults();
			this->SwitchMode(FH::MODE_FILES);
		}
		else
		{
			m_CollapseStack.pop();//弹出当前菜单
			HMENU prev = m_CollapseStack.top();
			if (prev)
			{
				this->ShowMenu(prev);
			}
			else
			{
				this->ShowMenu(m_hMenu);
			}
		}
	}
}

void QFileListView::ClearActionString()
{
	//释放字符串的内存
	int count = m_actionsModel->rowCount();
	for (int i = 0; i < count; i++)
	{
		QStandardItem* item = m_actionsModel->item(i);
		FH::MenuItemInfo info = item->data(Qt::UserRole + 1).value<FH::MenuItemInfo>();
		if (info.lpCommandString)
		{
			HeapFree(GetProcessHeap(), 0, info.lpCommandString);
			info.lpCommandString = NULL;
		}
	}
}

void QFileListView::clearResults()
{
	switch (m_viewMode)
	{
	case FH::MODE_FILES:
	{
		m_currentModel->clear();
		this->hide();
		break;
	}
	case FH::MODE_ACTIONS:
	{
		ClearActionString();
		m_currentModel->clear();
		break;
	}
	default:
		break;
	}

	m_currentModel->clear(); //查看源码,发现clear()将释放model里的各个item,我们自己再调用delete的话,会出现错误
	//todo: 两种模式的clear动作不同
}

void QFileListView::indexMouseOver(const QModelIndex &index)
{
	this->setCurrentIndex(index);
}

void QFileListView::indexMousePressed(const QModelIndex &index)
{
	switch (QApplication::mouseButtons())
	{
	case Qt::LeftButton:
	{
		//左键命令
		//根据当前的模式,需要执行以下行为
		//打开文件/文件夹 file模式
		//执行命令/展开子菜单 action模式
		//todo:点击完左键后(打开了该文件,或点击了菜单的某执行项后),界面该隐藏
		if (!index.isValid())
		{
			break;
		}
		switch (m_viewMode)
		{
		case FH::MODE_FILES:
		{
			SEARCH_RESULT_ITEM_DATA itemFile = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
			WCHAR path[MAX_PATH] = { 0 };
			itemFile.filePath.toWCharArray(path);
			ShellExecute(NULL, TEXT("open"), path, NULL, NULL, SW_SHOWDEFAULT);
			break;
		}
		case FH::MODE_ACTIONS:
		{
			FH::MenuItemInfo itemMenu = index.data(Qt::UserRole + 1).value<FH::MenuItemInfo>();
			if (itemMenu.hSubMenu)
			{
				Expand();
			}
			else
			{
				if (this->m_cm)
				{
					CMINVOKECOMMANDINFOEX invokeCommand = { 0 };
					invokeCommand.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
					invokeCommand.lpVerb = MAKEINTRESOURCEA(itemMenu.uItemID);
					invokeCommand.lpVerbW = MAKEINTRESOURCEW(itemMenu.uItemID);
					invokeCommand.nShow = SW_SHOWNORMAL;
					//invokeCommand.hwnd = (HWND)this->winId();
					invokeCommand.fMask = CMIC_MASK_ASYNCOK;
					m_cm->InvokeCommand((CMINVOKECOMMANDINFO*)&invokeCommand);
				}
			}
			break;
		}
		default:
			break;
		}
		break;
	}
		
	case Qt::RightButton:
	{
		//右键命令
		//展开新的listview以显示对文件的操作
		Expand();
		break;
	}
	default:
		break;
	}
}

void QFileListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	SEARCH_RESULT_ITEM_DATA itemData = current.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
	if (itemData.type == TYPE_HEADER)  //这一行是表头,不希望被选中
	{
		int nextRow = current.row() * 2 - previous.row();
		this->selectionModel()->setCurrentIndex(m_currentModel->index(nextRow, 0), QItemSelectionModel::SelectCurrent);
		return;
	}
	return QListView::currentChanged(current, previous);
}

// LRESULT CALLBACK CallWndProc(
// 	_In_ int    nCode,
// 	_In_ WPARAM wParam,
// 	_In_ LPARAM lParam
// ) {
//
// 	if (QFileListView::currentList)
// 	{
// 		CWPSTRUCT* cw = (CWPSTRUCT*)lParam;
// 		HWND wnd = cw->hwnd;
//
// 		if (wnd==(HWND)QFileListView::currentList->winId())
// 		{
// 			switch (cw->message)
// 			{
// 			case WM_MENUCHAR:
// 			{
// 				if (QFileListView::currentList->m_cm3)
// 				{
// 					LRESULT lres;
// 					if (SUCCEEDED(QFileListView::currentList->m_cm3->HandleMenuMsg2(cw->message, cw->wParam, cw->lParam, &lres)))
// 					{
// 						return lres;
// 					}
// 				}
// 				break;
// 			}
// 			case WM_DRAWITEM:
// 			case WM_MEASUREITEM:
// 			{
// 				if (cw->wParam)
// 				{
// 					break;
// 				}
// 			}
// 			case WM_INITMENUPOPUP:
// 			{
// 				if (QFileListView::currentList->m_cm3)
// 				{
// 					QFileListView::currentList->m_cm3->HandleMenuMsg(cw->message, cw->wParam, cw->lParam);
// 				}
// 				else if (QFileListView::currentList->m_cm2)
// 				{
// 					QFileListView::currentList->m_cm2->HandleMenuMsg(cw->message, cw->wParam, cw->lParam);
// 				}
// 				break;
// 			}
// 			default:
//				break;
// 			}
// 		}
//
//
// 	}
// 		return CallNextHookEx(0, nCode, wParam, lParam);
// }

bool QFileListView::QueryContextMenu(QModelIndex& index)
{
	bool bRet = false;
	if (m_prevIndex.isValid() && m_prevIndex == index) //获取目标上下文菜单挺耗时间的,储存上一个,用户再次点击同一个文件时直接返回
	{
		return true;
	}
	else
	{
		m_prevIndex = index;
		if (NULL != m_cm2)  //清理上一次的结果
		{
			m_cm2->Release();
			m_cm2 = NULL;
		}
		if (NULL != m_cm3)
		{
			m_cm3->Release();
			m_cm3 = NULL;
		}
		if (NULL != m_hMenu)
		{
			DestroyMenu(m_hMenu);
			m_hMenu = NULL;
		}
	}

	SEARCH_RESULT_ITEM_DATA itemData = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
	if (itemData.filePath.isEmpty())
		return bRet;
	
	HMENU hMenu = CreateMenu();

	if (NULL == hMenu)
		return bRet;

	ULONG len = (itemData.filePath.length() + 1) * sizeof(WCHAR);
	WCHAR* filePath = new WCHAR[len];
	ZeroMemory(filePath, len);
	itemData.filePath.toWCharArray(filePath);

	//////////////////////////////////////////////////////////////////////////
	HRESULT hr = S_FALSE;
	IContextMenu * pcm = NULL;
	IContextMenu2* pcm2 = NULL;
	IContextMenu3* pcm3 = NULL;
	IShellFolder * psfMain = NULL;
	LPITEMIDLIST ppidl;
	LPCITEMIDLIST ppidlast;
	SFGAOF sfgao;

	do
	{
		if (!SUCCEEDED(hr = SHParseDisplayName(filePath, NULL, &ppidl, NULL, &sfgao))) break;

		if (!SUCCEEDED(hr = SHBindToParent(ppidl, IID_IShellFolder, (void**)&psfMain, &ppidlast))) break;

		if (!SUCCEEDED(hr = psfMain->GetUIObjectOf(NULL, 1, &ppidlast, IID_IContextMenu, NULL, (void**)&pcm))) break;

		IQueryAssociations* iqa = NULL;
		hr = psfMain->GetUIObjectOf(NULL, 1, &ppidlast, IID_IQueryAssociations, NULL, (void**)&iqa);

		pcm->QueryInterface(IID_IContextMenu2, (void**)&pcm2);
		if (NULL != pcm2)
		{
			m_cm2 = pcm2;
			m_cm = pcm2;
		}
		pcm->QueryInterface(IID_IContextMenu3, (void**)&pcm3);
		if (NULL != pcm3)
		{
			m_cm3 = pcm3;
			m_cm = pcm3;
		}

		if (!SUCCEEDED(m_cm->QueryContextMenu(hMenu, 0, 0, 0x7FF, CMF_EXPLORE))) break;  //获得上下文菜单成功,开始解析并显示到我们的listview上

		m_hMenu = hMenu;
		bRet = true;
	} while (false);
	//需要我们负责释放
	if (ppidl)
	{
		CoTaskMemFree(ppidl);
	}
	if (psfMain)
	{
		psfMain->Release();
		psfMain = NULL;
	}
	if (pcm)
	{
		pcm->Release();
		pcm = NULL;
	}
	return bRet;
}

void QFileListView::ParseMenu(std::vector<FH::MenuItemInfo>& actionList, HMENU hTargetMenu)
{
	if (hTargetMenu == NULL)
	{
		actionList.clear();
		return;
	}
	int count = GetMenuItemCount(hTargetMenu);
	if (count < 0) return;
	for (int pos = 0; pos < count; pos++)
	{
		UINT flags = GetMenuState(hTargetMenu, pos, MF_BYPOSITION);
		if (flags&(MF_SEPARATOR | MF_DISABLED | MF_GRAYED))
		{
			if (!(flags&MF_POPUP)) //第二次进入时,"发送到"选项会带上MF_SEPARATOR的属性,迷,只好多加一层判断
			{
				continue;
			}
		}

		FH::MenuItemInfo item = { 0 };
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = MIIM_STRING;

		if (GetMenuItemInfo(hTargetMenu, pos, true, &info))
		{
			info.cch += 1;
			size_t textLength = sizeof(WCHAR)*info.cch;

			WCHAR* text = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, textLength);      //todo:回收内存

			if (NULL == text)
				continue;

			ZeroMemory(text, textLength);
			info.fMask |= (MIIM_SUBMENU | MIIM_ID | MIIM_BITMAP);
			info.dwTypeData = text;
			if (!GetMenuItemInfo(hTargetMenu, pos, true, &info))
				continue;

			item.lpCommandString = text;
			item.hBitmap = info.hbmpItem;
			item.uItemID = info.wID;
			item.hSubMenu = info.hSubMenu;

			if (info.hSubMenu)
			{
				MENUINFO a = { 0 };
				a.fMask = MIM_BACKGROUND | MIM_HELPID | MIM_MAXHEIGHT | MIM_MENUDATA | MIM_STYLE;
				GetMenuInfo(info.hSubMenu, &a);
				int b = GetMenuItemCount(info.hSubMenu);
				a = { 0 };
			}
			actionList.push_back(item);
		}
	}
}

void QFileListView::ShowMenu(HMENU hTargetMenu)
{
	this->clearResults();

	std::vector<FH::MenuItemInfo> actionList;
	this->ParseMenu(actionList, hTargetMenu);

	int size = actionList.size();
	if (size > 0)
	{
		m_CollapseStack.push(hTargetMenu);
		for (auto i : actionList)
		{
			QStandardItem*  singleItem = new QStandardItem;   //show
			singleItem->setData(QVariant::fromValue(i), Qt::UserRole + 1);
			m_currentModel->appendRow(singleItem);
		}
	}
	this->selectionModel()->setCurrentIndex(m_currentModel->index(0, 0), QItemSelectionModel::SelectCurrent);
}