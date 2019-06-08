#include "CustomMenu.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#define SHADOW_WIDTH 15					// ������Ӱ���;
#define WINDOW_WIDTH 180				// �˵����;
#define SEPARATOR_HEIGHT 12				// �ָ��߸߶�;
#define VMARGIN 10						// QListWidget����ı߾�;
#define BORDER_RADIUS 5					// �˵��Ľ�Բ�ǵĻ���;
#define WINDPW_OPACITY 0.9				// �˵�����͸����;






QCustomMenu::QCustomMenu(QWidget *parent /*= Q_NULLPTR*/)
	:QWidget(parent)
	, m_menuItemIndex(0)
{
	initWidget();

	// ���ô�������;
	setWindowFlags(Qt::FramelessWindowHint| Qt::Popup| Qt::NoDropShadowWindowHint);		// ��ʽӦ��ʱҪ���� Qt::Tool;
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowOpacity(WINDPW_OPACITY);
	setFixedWidth(WINDOW_WIDTH);
	setStyleSheet("QListView{border:none} QListView::item{background:transparent;}");

	// ������Ӱ�߿�;
	auto shadowEffect = new QGraphicsDropShadowEffect(this);
	shadowEffect->setOffset(0, 0);
	shadowEffect->setColor(Qt::gray);
	shadowEffect->setBlurRadius(SHADOW_WIDTH);
	this->setGraphicsEffect(shadowEffect);

	// ����Ĭ�ϸ߶�;
	m_currentMenuHeight = SHADOW_WIDTH * 2 + VMARGIN * 2;
	this->setFixedHeight(m_currentMenuHeight);
}

void QCustomMenu::addMenuItem(const QIcon &icon, const QString &text, const int& ID)
{
	QMenuItem* menuItem = new QMenuItem;
	menuItem->setMenuItemIcon(icon);
	menuItem->setMenuItemText(text);
	menuItem->SetMenuItemIndex(ID);

	connect(menuItem, &QMenuItem::signalMenuClicked, this, &QCustomMenu::onMenuItemClicked);

	QListWidgetItem* item = new QListWidgetItem;
	item->setSizeHint(QSize(WINDOW_WIDTH - 2 * SHADOW_WIDTH, MENU_ITEM_HEIGHT));
	m_menuItemListWidget->addItem(item);
	m_menuItemListWidget->setItemWidget(item, menuItem);

	m_menuItemIndex++;
	m_currentMenuHeight += MENU_ITEM_HEIGHT;
	this->setFixedHeight(m_currentMenuHeight);

}

void QCustomMenu::addSeparator()
{
	QFrame* separatorLabel = new QFrame;
	separatorLabel->setFrameShape(QFrame::HLine);
	separatorLabel->setLineWidth(1);
	separatorLabel->setFrameShadow(QFrame::Sunken);
	// ���÷ָ�����ɫ;
	QPalette palette = separatorLabel->palette();
	palette.setColor(QPalette::Dark, QColor(219, 219, 219, 150));
	separatorLabel->setPalette(palette);

	QListWidgetItem* item = new QListWidgetItem;
	item->setSizeHint(QSize(WINDOW_WIDTH - 2 * SHADOW_WIDTH, SEPARATOR_HEIGHT));
	m_menuItemListWidget->addItem(item);
	m_menuItemListWidget->setItemWidget(item, separatorLabel);

	m_currentMenuHeight += SEPARATOR_HEIGHT;
	this->setFixedHeight(m_currentMenuHeight);
}

void QCustomMenu::initWidget()
{
	m_menuItemListWidget = new QListWidget;
	m_menuItemListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_menuItemListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QHBoxLayout* hMainLayout = new QHBoxLayout(this);
	hMainLayout->addWidget(m_menuItemListWidget);
	hMainLayout->setContentsMargins(SHADOW_WIDTH, SHADOW_WIDTH + VMARGIN, SHADOW_WIDTH, SHADOW_WIDTH + VMARGIN);
}

void QCustomMenu::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::white);
	painter.drawRoundedRect(QRect(SHADOW_WIDTH, SHADOW_WIDTH, \
		width() - SHADOW_WIDTH * 2, height() - SHADOW_WIDTH * 2), BORDER_RADIUS, BORDER_RADIUS);
}

void QCustomMenu::onMenuItemClicked(int menuItemIndex)
{
	emit menuItemClicked(menuItemIndex);
	// ���ݵ����ͬ�Ĳ˵�����в�ͬ����������Ӧ�����ź�ȥ֪ͨ���ò˵���widget;
}


QCustomMenu::~QCustomMenu()
{
}
