#include "QFileListView.h"

QFileListView::QFileListView(QWidget *parent/*=0*/) :QListView(parent)
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
	util::setStyle(":/qss/scrollbar.qss", (QWidget *)(this->verticalScrollBar()));

	m_Delegate = new QSearchResultDelegate(this);
	m_model = new QStandardItemModel();
	this->setModel(m_model);
	this->setItemDelegate(m_Delegate);

	connect(this, SIGNAL(entered(const QModelIndex &)), this, SLOT(indexMouseOver(const QModelIndex &)));//当鼠标从items上划过时,将所选项设置为鼠标指向的index,
	//这将触发委托类的paint()里的if判断,使得所选项的背景上色,让用户看见
	connect(this, SIGNAL(pressed(const QModelIndex &)), this, SLOT(indexMousePressed(const QModelIndex &)));
	this->hide();
}

QFileListView::~QFileListView()
{
	delete m_Delegate;
	delete m_model;
}

void QFileListView::showResults(const QString & input)
{
	Q_UNUSED(input)

		for (int i = 0; i < 100; i++)
		{
			//QStandardItemModel::clear()将自动回收其拥有的所有item,所以我们只管申请,不管释放内存
			QStandardItem*  singleItem = new QStandardItem;//必须申请内存,局部内存将被销毁,到QSearchResultDelegate::paint()将接收到空的参数
			SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"F:\\哈.png"));
			if (i == 50)
			{
				itemData.type = TYPE_HEADER;
			}
			singleItem->setData(QVariant::fromValue(itemData), Qt::UserRole + 1);
			m_model->appendRow(singleItem);
		}
	//todo:处理搜索结果

	this->setCurrentIndex(m_model->index(0, 0));
	this->show();
}

void QFileListView::keyEventForward(QKeyEvent *event)
{
	unsigned int row = this->selectionModel()->currentIndex().row();
	unsigned int count = m_model->rowCount();

	switch (event->key()) {
	case Qt::Key_Up:
	{
		row = (row - 1) % count; //不用自己写,默认就接受上下键,但这里需要处理拉到头应返回尾端的问题,还要接受左右键
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::SelectCurrent);
		break;
	}

	case Qt::Key_Down:
	{
		row = (row + 1) % count;
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::SelectCurrent);
		break;
	}

	case Qt::Key_Left:
	{
		//后退
		break;
	}
		
	case Qt::Key_Right:
	{
		QPoint pos = this->pos();
		//前进
		break;
	}

	default:
		return this->keyPressEvent(event);
	}
}

void QFileListView::clearResults()
{
	m_model->clear(); //查看源码,发现clear()将释放model里的各个item,我们自己再调用delete的话,会出现错误
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
		qDebug() << "left button pressed!";
		break;
	case Qt::RightButton:
		qDebug() << "right button pressed!";
		break;
	default:
		break;
	}
}

void QFileListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	SEARCH_RESULT_ITEM_DATA itemData = current.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
	if (itemData.type == TYPE_HEADER)
	{
		int nextRow = current.row() * 2 - previous.row();
		this->selectionModel()->setCurrentIndex(m_model->index(nextRow, 0), QItemSelectionModel::SelectCurrent);
		return;
	}
	return QListView::currentChanged(current, previous);
}