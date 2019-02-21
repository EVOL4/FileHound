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
	this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);//�������Ŀհ�,�Լ��Զ��¹�ʱ����β��������
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

	connect(this, SIGNAL(entered(const QModelIndex &)), this, SLOT(indexMouseOver(const QModelIndex &)));//������items�ϻ���ʱ,����ѡ������Ϊ���ָ���index,
	//�⽫����ί�����paint()���if�ж�,ʹ����ѡ��ı�����ɫ,���û�����
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
		//QStandardItemModel::clear()���Զ�������ӵ�е�����item,��������ֻ������,�����ͷ��ڴ�
		QStandardItem*  singleItem = new QStandardItem;//���������ڴ�,�ֲ��ڴ潫������,��QSearchResultDelegate::paint()�����յ��յĲ���
		//SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"C:\\Users\\rye\\Desktop\\Understanding_the_LFH.pdf"));
		SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"C:\\Users\\rye\\Desktop\\1.txt"));
		if (i == 50)
		{
			itemData.type = TYPE_HEADER;
		}
		singleItem->setData(QVariant::fromValue(itemData), Qt::UserRole + 1);
		m_currentModel->appendRow(singleItem);
	}
	//todo:�����������

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
	switch (event->key()) {  //Ĭ�Ͼͽ������¼�,��������Ҫ��������ͷӦ����β�˵�����,��Ҫ�������Ҽ�
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

		if (m_depth == 0)  //չ���˵�
		{
			this->SwitchMode(FH::MODE_ACTIONS);
			if (this->QueryContextMenu(cur))  //todo:�����˵�ͦ��ʱ���,Ӧ�����첽�ķ�ʽ����
			{
				m_depth++;
				this->ShowMenu(m_hMenu);
			}
		}
		else if (m_depth > 0) //չ���Ӳ˵�
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
			m_CollapseStack.pop();//������ǰ�˵�
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
	//�ͷ��ַ������ڴ�
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

	m_currentModel->clear(); //�鿴Դ��,����clear()���ͷ�model��ĸ���item,�����Լ��ٵ���delete�Ļ�,����ִ���
	//todo: ����ģʽ��clear������ͬ
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
		//�������
		//���ݵ�ǰ��ģʽ,��Ҫִ��������Ϊ
		//���ļ�/�ļ��� fileģʽ
		//ִ������/չ���Ӳ˵� actionģʽ
		//todo:����������(���˸��ļ�,�����˲˵���ĳִ�����),���������
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
		//�Ҽ�����
		//չ���µ�listview����ʾ���ļ��Ĳ���
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
	if (itemData.type == TYPE_HEADER)  //��һ���Ǳ�ͷ,��ϣ����ѡ��
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
	if (m_prevIndex.isValid() && m_prevIndex == index) //��ȡĿ�������Ĳ˵�ͦ��ʱ���,������һ��,�û��ٴε��ͬһ���ļ�ʱֱ�ӷ���
	{
		return true;
	}
	else
	{
		m_prevIndex = index;
		if (NULL != m_cm2)  //������һ�εĽ��
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

		if (!SUCCEEDED(m_cm->QueryContextMenu(hMenu, 0, 0, 0x7FF, CMF_EXPLORE))) break;  //��������Ĳ˵��ɹ�,��ʼ��������ʾ�����ǵ�listview��

		m_hMenu = hMenu;
		bRet = true;
	} while (false);
	//��Ҫ���Ǹ����ͷ�
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
			if (!(flags&MF_POPUP)) //�ڶ��ν���ʱ,"���͵�"ѡ������MF_SEPARATOR������,��,ֻ�ö��һ���ж�
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

			WCHAR* text = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, textLength);      //todo:�����ڴ�

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