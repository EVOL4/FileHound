#pragma once
#include<QStyledItemDelegate>

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


};

