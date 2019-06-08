#pragma once
#include <QtWidgets/QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QListWidget>

#define MENU_ITEM_HEIGHT 30			// 菜单项的高度;

class QMenuItem :public QWidget
{
	Q_OBJECT
public:

	 QMenuItem() 
		:m_isEnter(false)
		,m_itemIndex(0)
	{
		m_iconLabel = new QLabel;
		m_iconLabel->setFixedSize(QSize(20, 20));

		m_textLabel = new QLabel;
		m_textLabel->setStyleSheet("font-family:Microsoft YaHei;font-size:14px;");

		QHBoxLayout* hLayout = new QHBoxLayout(this);
		hLayout->addWidget(m_iconLabel);
		hLayout->addWidget(m_textLabel);
		hLayout->addStretch();
		hLayout->setSpacing(10);
		hLayout->setContentsMargins(10, 0, 0, 0);

		this->setFixedHeight(MENU_ITEM_HEIGHT);
		this->setAttribute(Qt::WA_TranslucentBackground);
		
	}

	~QMenuItem()
	{

	}

	// 设置菜单项文字;
	void setMenuItemText(const QString& text)
	{
		m_textLabel->setText(text);
		m_textLabel->setScaledContents(true);
	}

	// 设置菜单项图标;
	void setMenuItemIcon(const QIcon& icon)
	{
		m_iconLabel->setPixmap(icon.pixmap(QSize(20, 20)));
	}

	// 设置菜单项Index;
	void SetMenuItemIndex(const int& index)
	{
		m_itemIndex = index;
	}

private:
	void mouseReleaseEvent(QMouseEvent *event)
	{
		emit signalMenuClicked(m_itemIndex);
	}

	void enterEvent(QEvent *event)
	{
		m_isEnter = true;
		update();
	}

	void leaveEvent(QEvent *event)
	{
		m_isEnter = false;
		update();
	}

	void paintEvent(QPaintEvent *event)
	{
		if (m_isEnter)
		{
			QPainter painter(this);
			painter.fillRect(this->rect(), QColor(215, 215, 215, 150));
		}
	}

signals:
	// 通知菜单项被点击;
	void signalMenuClicked(int);

	
private:
	QLabel* m_iconLabel;
	QLabel* m_textLabel;
	bool m_isEnter;
	int m_itemIndex;
};


class QCustomMenu : public QWidget
{
	Q_OBJECT
public:
	

		QCustomMenu(QWidget *parent = Q_NULLPTR);
		~QCustomMenu();
	// 添加菜单项;
	void addMenuItem(const QIcon &icon, const QString &text,const int& ID);
	// 添加分隔项;
	void addSeparator();
signals:
	void menuItemClicked(int ID);


private:
	void initWidget();
	void paintEvent(QPaintEvent *event);

private slots:
	// 根据Index判断当前点击了那个菜单项;
	void onMenuItemClicked(int menuItemIndex);

private:
	QListWidget* m_menuItemListWidget;
	// 根据当前添加的菜单项和分隔项自动计算菜单高度;
	int m_currentMenuHeight;
	// 记录菜单项Index;
	int m_menuItemIndex;

};

