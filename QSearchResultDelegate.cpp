#include <QPainter>
#include "QSearchResultDelegate.h"
#include "util/util.h"

QSearchResultDelegate::QSearchResultDelegate(QObject *parent /*= 0*/) :
	QStyledItemDelegate(parent)
{
	m_mode = FH::MODE_FILES; //Ĭ�ϻ����ļ�
}

void QSearchResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	switch (m_mode)
	{
	case FH::MODE_FILES:
		paint_files(painter, option, index);
		break;

	case FH::MODE_ACTIONS:
		paint_actions(painter, option, index);
		break;
	default:
		break;
	}
}

QSize QSearchResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size(option.rect.width(), 0);

	switch (m_mode)
	{
	case FH::MODE_FILES:
	{
		SEARCH_RESULT_ITEM_DATA itemData = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
		unsigned int height = 55;
		if (itemData.type == TYPE_HEADER)
		{
			height = 30;
		}
		size.setHeight(height);
		break;
	}

	case FH::MODE_ACTIONS:
	{
		size.setHeight(35);
		break;
	}

	default:
		break;
	}

	return size;
}

void QSearchResultDelegate::SwitchMode(enum FH::ViewMode mode)
{
	m_mode = mode;
}

void QSearchResultDelegate::paint_files(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	do
	{
		if (!index.isValid())
			break;

		SEARCH_RESULT_ITEM_DATA itemData = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();

		if (itemData.fileName.isEmpty())
			break;

		painter->save();

		QRectF rect;  //ÿ��item�ľ�������
		rect.setX(option.rect.x());
		rect.setY(option.rect.y());
		rect.setWidth(option.rect.width() - 1);
		rect.setHeight(option.rect.height() - 1);

		switch (itemData.type)
		{
		case TYPE_HEADER:
		{
			QRectF staticTextRec = QRect(rect.left() + 1, rect.top() + 2, rect.width() - 40, rect.height());
			QRectF dirCount = QRect(staticTextRec.right(), staticTextRec.top(), rect.width() - staticTextRec.width(), rect.height());

			painter->setPen(QPen(Qt::gray));
			painter->setFont(QFont("Microsoft Yahei", 12));
			painter->drawText(staticTextRec, QString::fromWCharArray(L"Ŀ¼"));
			painter->drawText(dirCount, QString::fromWCharArray(L"50")); //todo:��̬����

			QPen pen;
			pen.setCapStyle(Qt::RoundCap);
			pen.setStyle(Qt::DashLine);
			pen.setColor(QColor(Qt::gray));
			painter->setPen(pen);
			painter->drawLine(staticTextRec.topLeft(), dirCount.topRight());
			// 			painter->drawLine(staticTextRec.bottomLeft(), staticTextRec.bottomRight());
			break;
		}

		default:
		{
			if (option.state.testFlag(QStyle::State_Selected)) { //QStyle::State_Selected�ɴ���������/�Ҽ�/�м����
				QPainterPath path;	//ѡ��ʱ����������Ҫ��ɫ
				path.moveTo(rect.topRight());
				path.lineTo(rect.topLeft());
				path.quadTo(rect.topLeft(), rect.topLeft());
				path.lineTo(rect.bottomLeft());
				path.quadTo(rect.bottomLeft(), rect.bottomLeft());
				path.lineTo(rect.bottomRight());
				path.quadTo(rect.bottomRight(), rect.bottomRight());
				path.lineTo(rect.topRight());
				path.quadTo(rect.topRight(), rect.topRight());

				painter->setPen(QPen(QColor("#433e64")));
				painter->setBrush(QColor("#433e64"));
				painter->drawPath(path);
			}

			// ����ͼƬ������
			QRectF iconRect = QRect(rect.left() + 5, rect.top() + 6, 40, 40);
			QRectF fileNameRect = QRect(iconRect.right() + 5, iconRect.top() + 1, rect.width() - 10 - iconRect.width(), 35);
			QRectF filePathRect = QRect(fileNameRect.left(), fileNameRect.bottom() - 10, rect.width() - 10 - iconRect.width(), 20);

			painter->drawImage(iconRect, itemData.icon);

			painter->setPen(QPen(Qt::gray));
			painter->setFont(QFont("Microsoft Yahei", 10));
			//itemData.fileName = QString::fromWCharArray(L"&��") + itemData.fileName; //todo:�����뾭����,��Ȼ����..
			painter->drawText(fileNameRect, Qt::TextShowMnemonic, '&' + itemData.fileName);  // Qt::TextShowMnemonic:�»��ߵĴ��� &+Ҫ���»��ߵ���ĸ
			painter->setFont(QFont("Microsoft Yahei", 8));
			painter->drawText(filePathRect, Qt::TextShowMnemonic, itemData.filePath);

			break;
		}
		}

		painter->restore();
	} while (false);
}

void QSearchResultDelegate::paint_actions(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	do
	{
		//��ʼ������
		if (!index.isValid()) break;

		FH::MenuItemInfo menuItemInfo = index.data(Qt::UserRole + 1).value<FH::MenuItemInfo>();

		QString CommandString = QString::fromWCharArray(menuItemInfo.lpCommandString);
		
		if (CommandString.isEmpty()) break;
	

		//ȷ�Ͼ�������
		painter->save();

		QRectF rect;  //ÿ��item�ľ�������
		rect.setX(option.rect.x());
		rect.setY(option.rect.y());
		rect.setWidth(option.rect.width() - 1);
		rect.setHeight(option.rect.height() - 1);

		QRectF bmpRect = QRect(rect.left() + 8, rect.top() + 6, 20, 20);
		QRectF cmdStringRect = QRect(bmpRect.right() + 8, rect.top() + 6, rect.width() - bmpRect.width() - 10, 20);
		
		
		
		//����painter����Ϊ
		painter->setPen(QPen(Qt::gray));
		painter->setFont(QFont("Microsoft Yahei", 10));

		if (option.state.testFlag(QStyle::State_Selected)) { 
			QPainterPath path;	//ѡ��ʱ����������Ҫ��ɫ
			path.moveTo(rect.topRight());
			path.lineTo(rect.topLeft());
			path.quadTo(rect.topLeft(), rect.topLeft());
			path.lineTo(rect.bottomLeft());
			path.quadTo(rect.bottomLeft(), rect.bottomLeft());
			path.lineTo(rect.bottomRight());
			path.quadTo(rect.bottomRight(), rect.bottomRight());
			path.lineTo(rect.topRight());
			path.quadTo(rect.topRight(), rect.topRight());

			painter->setPen(QPen(QColor("#433e64")));
			painter->setBrush(QColor("#433e64"));
			painter->drawPath(path);
			
			//ͬʱ����Ҳ��Ҫ��ɫ
			painter->setPen(QPen(QColor("#33ffcc")));
		}

		//draw

		painter->drawText(cmdStringRect, Qt::TextHideMnemonic, CommandString);

		if (menuItemInfo.hBitmap) //ͼƬ�ᱻ��λ���,��Ҫ��ǰ�û���������˵�ʱ,���Ǿ��Բ��ܻ��ղ˵���ص��ڴ�
		{
			QImage bmp = QtWin::fromHBITMAP(menuItemInfo.hBitmap, QtWin::HBitmapAlpha).toImage();
			painter->drawImage(bmpRect, bmp);
		}
		if (menuItemInfo.hSubMenu) //��ĩβ����һ����ͷ,��ʾ����չ��һ���Ӳ˵�
		{
			QRectF arrowRect = QRect(rect.right() - 21, rect.top() + 12, 10, 10);
			QPen arrowPen(painter->pen());
			arrowPen.setCapStyle(Qt::RoundCap);
			arrowPen.setJoinStyle(Qt::RoundJoin);
			arrowPen.setWidth(2.5);
			
			QPainterPath path;
			path.moveTo(arrowRect.topLeft());
			path.lineTo(arrowRect.x()+6, arrowRect.y()+5);
			path.lineTo(arrowRect.bottomLeft());

			painter->setPen(arrowPen);
			painter->setRenderHint(QPainter::Antialiasing, true);//�����
			painter->drawPath(path);

		}
		
		
		painter->restore();

	} while (false);
}

void CleanUpItemInfo(FH::MenuItemInfo& m)
{
	if (m.hBitmap)
	{
		DeleteObject(m.hBitmap);
		m.hBitmap = NULL;
	}
	if (m.hSubMenu)
	{
		DestroyMenu(m.hSubMenu);
		m.hSubMenu = NULL;
	}
// 	if (m.lpCommandString)
// 	{
// 		HeapFree(GetProcessHeap(), 0, m.lpCommandString);
// 		m.lpCommandString = NULL;
// 	}
}