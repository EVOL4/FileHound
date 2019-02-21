#include "QActionListView.h"

QActionListView::QActionListView(QWidget *parent/*=0*/) : QListView(parent), m_hMenu(NULL), m_cm(NULL), m_cm2(NULL), m_cm3(NULL)
{
	auto shadowEffectCopy = new QGraphicsDropShadowEffect();
	shadowEffectCopy->setOffset(0, 0);
	shadowEffectCopy->setColor(QColor(90, 78, 85));
	shadowEffectCopy->setBlurRadius(10);

	this->setGraphicsEffect(shadowEffectCopy);
	this->setMinimumHeight(498);
	this->setMaximumHeight(498);
	this->setMouseTracking(true);
	this->setTabKeyNavigation(true);

	this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);//解决多出的空白,以及自动下滚时滚到尾部的问题
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->setFocusPolicy(Qt::NoFocus);
	this->setFixedWidth(571);

	util::setStyle(":/listview.qss", this);

	m_Delegate = new QSearchResultDelegate(this);
	m_model = new QStandardItemModel();
	m_ole = new OleInitializer();

	this->setModel(m_model);
	this->setItemDelegate(m_Delegate);

	connect(this, SIGNAL(entered(const QModelIndex &)), this, SLOT(indexMouseOver(const QModelIndex &)));//当鼠标从items上划过时,将所选项设置为鼠标指向的index,
	//这将触发委托类的paint()里的if判断,使得所选项的背景上色,让用户看见
	connect(this, SIGNAL(pressed(const QModelIndex &)), this, SLOT(indexMousePressed(const QModelIndex &)));
}

QActionListView::~QActionListView()
{
	if (NULL != m_cm2)
	{
		m_cm2->Release();
		m_cm2 = NULL;
	}
	if (NULL != m_cm3)
	{
		m_cm3->Release();
		m_cm3 = NULL;
	}

	DELETE_IF_NOT_NULL(m_Delegate);
	DELETE_IF_NOT_NULL(m_model);
	DELETE_IF_NOT_NULL(m_ole);
}



void QActionListView::DisplayActions()
{
	return;
}

void QActionListView::showResults(const QString & input)
{
	return;
}

void QActionListView::clearResult()
{
	return;
}

void QActionListView::indexMouseOver(const QModelIndex &index)
{
	this->setCurrentIndex(index);
}

void QActionListView::indexMousePressed(const QModelIndex &index)
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