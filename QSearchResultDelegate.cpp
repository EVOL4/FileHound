#include <QPainter>
#include "QSearchResultDelegate.h"
#include "util/util.h"

QSearchResultDelegate::QSearchResultDelegate(QObject *parent /*= 0*/) :
	QStyledItemDelegate(parent)
{
}

void QSearchResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
			QRectF staticTextRec = QRect(rect.left() + 1, rect.top() + 2, rect.width()-40, rect.height());
			QRectF dirCount = QRect(staticTextRec.right(), staticTextRec.top(), rect.width()- staticTextRec.width(), rect.height());
			
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

QSize QSearchResultDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	SEARCH_RESULT_ITEM_DATA itemData = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();
	unsigned int height = 55;
	if (itemData.type == TYPE_HEADER)
	{
		height = 30;
	}
	return QSize(option.rect.width(), height);
}