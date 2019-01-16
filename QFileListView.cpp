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
	this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);//�������Ŀհ�,�Լ��Զ��¹�ʱ����β��������
	this->setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->setTabKeyNavigation(true);
	this->setFocusPolicy(Qt::NoFocus);
	util::setStyle(":/qss/scrollbar.qss", (QWidget *)(this->verticalScrollBar()));

	m_Delegate = new QSearchResultDelegate(this);
	m_model = new QStandardItemModel();
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

void QFileListView::showResults(const QString & input)
{
	Q_UNUSED(input)

		for (int i = 0; i < 100; i++)
		{
			//QStandardItemModel::clear()���Զ�������ӵ�е�����item,��������ֻ������,�����ͷ��ڴ�
			QStandardItem*  singleItem = new QStandardItem;//���������ڴ�,�ֲ��ڴ潫������,��QSearchResultDelegate::paint()�����յ��յĲ���
			SEARCH_RESULT_ITEM_DATA itemData = util::GetFileData(QString::fromWCharArray(L"F:\\��.png"));
			if (i == 50)
			{
				itemData.type = TYPE_HEADER;
			}
			singleItem->setData(QVariant::fromValue(itemData), Qt::UserRole + 1);
			m_model->appendRow(singleItem);
		}
	//todo:�����������

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
		row = (row - 1) % count; //�����Լ�д,Ĭ�Ͼͽ������¼�,��������Ҫ��������ͷӦ����β�˵�����,��Ҫ�������Ҽ�
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
		//����
		break;
	}
		
	case Qt::Key_Right:
	{
		QPoint pos = this->pos();
		//ǰ��
		break;
	}

	default:
		return this->keyPressEvent(event);
	}
}

void QFileListView::clearResults()
{
	m_model->clear(); //�鿴Դ��,����clear()���ͷ�model��ĸ���item,�����Լ��ٵ���delete�Ļ�,����ִ���
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
		//���ļ�
		break;
	case Qt::RightButton:
		//չ���µ�listview����ʾ���ļ��Ĳ���
		qDebug() << "right button pressed!";
		break;
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
		this->selectionModel()->setCurrentIndex(m_model->index(nextRow, 0), QItemSelectionModel::SelectCurrent);
		return;
	}
	return QListView::currentChanged(current, previous);
}