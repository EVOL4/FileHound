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
	//const :������Ա���������޸�������Ա,Ҳ���ܵ��÷�const��Ա����
	//override :�ñ�������֤���ǵ�������ԭ������ͬ,���������Ǳ���

	//����paint,���ڻ������ǵ��Զ���item,����������ʾ�����ݽϼ�,���Ҳ��ÿ����û�������,ֱ�ӻ���Ч�ʱ�ʹ��widgetҪ��
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	//����sizeHint,���ڿ���item�Ĵ�С
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;


	void SwitchMode(enum FH::ViewMode);

private:
	enum FH::ViewMode m_mode;

	void paint_files(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void paint_actions(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

