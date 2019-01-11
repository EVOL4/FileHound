#include "QFileListView.h"

QFileListView::QFileListView(QWidget *parent/*=0*/) :QListView(parent)
{
	auto shadowEffectCopy = new QGraphicsDropShadowEffect();
	shadowEffectCopy->setOffset(0, 0);
	shadowEffectCopy->setColor(QColor(90, 78, 85));
	shadowEffectCopy->setBlurRadius(10);

	this->setGraphicsEffect(shadowEffectCopy);
	this->setMinimumHeight(520);
	this->setMaximumHeight(520);
	this->setMouseTracking(true);
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setFocusPolicy(Qt::NoFocus);
	m_Delegate = new QSearchResultDelegate(this);
	m_model = new QStandardItemModel();
	util::setStyle(":/qss/scrollbar.qss", (QWidget *)(this->verticalScrollBar()));
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

void QFileListView::showResults()
{
	
	for (int i = 0; i < 100; i++)
	{
		QStandardItem*  singleItem = new QStandardItem; //必须申请内存,局部内存将被销毁,到QSearchResultDelegate::paint()将接收到空的参数
		//todo:可以在初始化时多申请几个QStandardItem,采取类似线程池的做法,在程序结束时delete这些内存,反正多的结果我们也不显示
		SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"F:\\哈.png"));
		singleItem->setData(QVariant::fromValue(itemData), Qt::UserRole + 1);
		m_model->appendRow(singleItem);
		//delete singleItem;
	}
	this->setCurrentIndex(m_model->index(0, 0));
	this->show();
}

void QFileListView::keyEventForward(QKeyEvent *event)
{
	unsigned int row = this->selectionModel()->currentIndex().row();
	unsigned int count = m_model->rowCount();

	switch (event->key()) {
	case Qt::Key_Up:
		row = (row - 1) % count; //不用自己写,默认就接受上下键,但这里需要处理拉到头应返回尾端的问题
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::NoUpdate);
		this->selectionModel()->setCurrentIndex(this->selectionModel()->currentIndex(), QItemSelectionModel::SelectCurrent);
		break;

	case Qt::Key_Down:
		row = (row + 1) % count;
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::NoUpdate);
		this->selectionModel()->setCurrentIndex(this->selectionModel()->currentIndex(), QItemSelectionModel::SelectCurrent);
		break;

	case Qt::Key_Left:
		//后退
		break;

	case Qt::Key_Right:
		//前进
		break;

	default:
		return this->keyPressEvent(event);
	}
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
		qDebug() << "left button pressed!" ;
		break;
	case Qt::RightButton:
		qDebug() << "right button pressed!";
		break;
	default:
		break;
	}
}



