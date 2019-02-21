#pragma once
#include<QStyledItemDelegate>
#include <windows.h>
namespace FH
{
	typedef enum ViewMode
	{
		MODE_FILES,
		MODE_ACTIONS
	};

	typedef struct FHMENUITEMINFO
	{
		HBITMAP hBitmap;
		HMENU hSubMenu;
		UINT uItemID;
		WCHAR* lpCommandString;
	}MenuItemInfo;

	

}

void CleanUpItemInfo(FH::MenuItemInfo& m);


Q_DECLARE_METATYPE(FH::MenuItemInfo)

class QSearchResultDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	QSearchResultDelegate(QObject *parent = 0);
	//const :表明成员函数不能修改其他成员,也不能调用非const成员函数
	//override :让编译器保证我们的声明与原函数相同,否则向我们报错

	//重载paint,用于绘制我们的自定义item,由于我们显示的数据较简单,而且不用考虑用户的输入,直接绘制效率比使用widget要高
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	//重载sizeHint,用于控制item的大小
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;


	void SwitchMode(enum FH::ViewMode);

private:
	enum FH::ViewMode m_mode;

	void paint_files(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void paint_actions(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

