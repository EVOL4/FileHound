#include <QPainter>
#include "match.h"
#include "QSearchResultDelegate.h"
#include "util/util.h"

QSearchResultDelegate::QSearchResultDelegate(QObject *parent /*= 0*/) :
	QStyledItemDelegate(parent)
{
	m_mode = FH::MODE_FILES; //默认绘制文件
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

void QSearchResultDelegate::setQuery(const QString& query)
{

	
	m_query = query;
}

void QSearchResultDelegate::SwitchMode(enum FH::ViewMode mode)
{
	m_mode = mode;
}



void QSearchResultDelegate::paint_files(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	do
	{
		if (m_query.isEmpty())
		{
			return;
		}
		
		if (!index.isValid())
			break;

		SEARCH_RESULT_ITEM_DATA itemData = index.data(Qt::UserRole + 1).value<SEARCH_RESULT_ITEM_DATA>();

		if (itemData.fileName.isEmpty())
			break;

		WCHAR szFileName[MAX_PATH] = { 0 };
		WCHAR szQuery[MAX_PATH] = { 0 };
		int size = m_query.length();
		size_t pos[MAX_PATH] = { 0 };
// 		size_t*  pos = new size_t[size];
// 		ZeroMemory(pos, size*sizeof(size_t));
		itemData.fileName.toWCharArray(szFileName);
		m_query.toWCharArray(szQuery);


		QString underlinedName(itemData.fileName);
		
		match_positions(szQuery, szFileName, pos);
		
		for (int  i = size-1;i>-1;i--)
		{
			if (pos[i]!=0)
			{
				int j = pos[ i] - 1;
				underlinedName.insert(j, '&');
			}
		}


		

		painter->save();

		QRectF rect;  //每个item的矩形区域
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
			painter->drawText(staticTextRec, QString::fromWCharArray(L"目录"));
			painter->drawText(dirCount, itemData.fileName); //todo:动态计数

			QPen pen;
			pen.setCapStyle(Qt::RoundCap);
			pen.setStyle(Qt::DashLine);
			pen.setColor(QColor(Qt::gray));
			painter->setPen(pen);
			painter->drawLine(staticTextRec.topLeft(), dirCount.topRight());
	
			break;
		}

		default:
		{
			if (option.state.testFlag(QStyle::State_Selected)) { //QStyle::State_Selected可处理鼠标左键/右键/中键点击
				QPainterPath path;	//选中时整个矩形需要变色
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

			// 绘制图片，文字
			QRectF iconRect = QRect(rect.left() + 5, rect.top() + 6, 40, 40);
			QRectF fileNameRect = QRect(iconRect.right() + 5, iconRect.top() + 1, rect.width() - 10 - iconRect.width(), 35);
			QRectF filePathRect = QRect(fileNameRect.left(), fileNameRect.bottom() - 10, rect.width() - 10 - iconRect.width(), 20);

			painter->drawImage(iconRect, itemData.icon);

			painter->setPen(QPen(Qt::gray));
			QFont font("Microsoft Yahei", 10);
			font.setLetterSpacing(QFont::PercentageSpacing,102.4);
			painter->setFont(font);
			auto o = painter->fontInfo();

			painter->drawText(fileNameRect, Qt::TextShowMnemonic, underlinedName);  // Qt::TextShowMnemonic:下划线的处理 &+要加下划线的字母
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
		//初始化数据
		if (!index.isValid()) break;

		FH::MenuItemInfo menuItemInfo = index.data(Qt::UserRole + 1).value<FH::MenuItemInfo>();

		QString CommandString = QString::fromWCharArray(menuItemInfo.lpCommandString);
		
		if (CommandString.isEmpty()) break;
	

		//确认矩形区域
		painter->save();

		QRectF rect;  //每个item的矩形区域
		rect.setX(option.rect.x());
		rect.setY(option.rect.y());
		rect.setWidth(option.rect.width() - 1);
		rect.setHeight(option.rect.height() - 1);

		QRectF bmpRect = QRect(rect.left() + 8, rect.top() + 6, 20, 20);
		QRectF cmdStringRect = QRect(bmpRect.right() + 8, rect.top() + 6, rect.width() - bmpRect.width() - 10, 20);
		
		
		
		//设置painter的行为
		painter->setPen(QPen(Qt::gray));
		painter->setFont(QFont("Microsoft Yahei", 10));

		if (option.state.testFlag(QStyle::State_Selected)) { 
			QPainterPath path;	//选中时整个矩形需要变色
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
			
			//同时文字也需要变色
			painter->setPen(QPen(QColor("#33ffcc")));
		}

		//draw

		painter->drawText(cmdStringRect, Qt::TextHideMnemonic, CommandString);

		if (menuItemInfo.hBitmap) //图片会被多次绘制,这要求当前用户正在浏览菜单时,我们绝对不能回收菜单相关的内存
		{
			QImage bmp = QtWin::fromHBITMAP(menuItemInfo.hBitmap, QtWin::HBitmapAlpha).toImage();
			painter->drawImage(bmpRect, bmp);
		}
		if (menuItemInfo.hSubMenu) //在末尾绘制一个箭头,以示可以展开一个子菜单
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
			painter->setRenderHint(QPainter::Antialiasing, true);//反锯齿
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