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

	connect(this, SIGNAL(entered(const QModelIndex &)), this, SLOT(indexMouseOver(const QModelIndex &)));//������items�ϻ���ʱ,����ѡ������Ϊ���ָ���index,
	//�⽫����ί�����paint()���if�ж�,ʹ����ѡ��ı�����ɫ,���û�����
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
		QStandardItem*  singleItem = new QStandardItem; //���������ڴ�,�ֲ��ڴ潫������,��QSearchResultDelegate::paint()�����յ��յĲ���
		//todo:�����ڳ�ʼ��ʱ�����뼸��QStandardItem,��ȡ�����̳߳ص�����,�ڳ������ʱdelete��Щ�ڴ�,������Ľ������Ҳ����ʾ
		SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"F:\\��.png"));
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
		row = (row - 1) % count; //�����Լ�д,Ĭ�Ͼͽ������¼�,��������Ҫ��������ͷӦ����β�˵�����
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::NoUpdate);
		this->selectionModel()->setCurrentIndex(this->selectionModel()->currentIndex(), QItemSelectionModel::SelectCurrent);
		break;

	case Qt::Key_Down:
		row = (row + 1) % count;
		this->selectionModel()->setCurrentIndex(m_model->index(row, 0), QItemSelectionModel::NoUpdate);
		this->selectionModel()->setCurrentIndex(this->selectionModel()->currentIndex(), QItemSelectionModel::SelectCurrent);
		break;

	case Qt::Key_Left:
		//����
		break;

	case Qt::Key_Right:
		//ǰ��
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



