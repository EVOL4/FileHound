
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileListView.h"




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    tools(util::getInstance())
{
    ui->setupUi(this);

	m_lineEdit = ui->lineEdit;
	m_listView = ui->listView;


    //去除除文本输入以外所有控件的focus
    ui->pushButton->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_2->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_3->setFocusPolicy(Qt::NoFocus);

	m_listView->hide();

    //安装事件过滤器,要求本QObject覆写bool eventFilter(QObject *watched, QEvent *event)虚函数
    this->installEventFilter(this);

    //注册快捷键
    //todo:移到util里去,util应该负责管理多个热键
    QWinHotkey * hotkey = new QWinHotkey();
    hotkey->setShortcut(tr("ctrl+h")); //目标快捷键组合
    connect(hotkey, SIGNAL(activated()), this, SLOT(showUp()));//连接该信号与我们的槽

    //炫酷的阴影效果
    auto shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setOffset(0, 0);
    shadowEffect->setColor(QColor(90,78,85));
    shadowEffect->setBlurRadius(15);
    ui->groupBox_main->setGraphicsEffect(shadowEffect);

	connect(this, SIGNAL(user_input(const QString &)), ui->listView, SLOT(showResults(const QString &)));
	connect(this, SIGNAL(empty_input()), ui->listView, SLOT(clearResults()));
	connect(ui->lineEdit, SIGNAL(arrowKeyPressed(QKeyEvent *)), this, SLOT(arrowKeyTakeover(QKeyEvent *)));

	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(on_quit()));
	//tools->SetUpTrayIcon();

}

MainWindow::~MainWindow()
{
    delete ui;
}



bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type()==QEvent::ActivationChange)
    {
        if(QApplication::activeWindow()!=this&&!this->isHidden())
        {
            m_lineEdit->clear();
             //在输入框有字的情况下,程序隐藏又出现时,listview会留下残影,猜想是on_lineEdit_textChanged没来得及执行主界面就隐藏了,这样可以解决
            tools->sleep(50); //ps:调用windows的Sleep在调试外好像没作用
            this->hide(); //qt貌似不支持全局热键,这种方法注册的快捷键在窗口没激活时是不起效果的,自己调用windows的api解决吧
        }
        else if(QApplication::activeWindow()==this)
        {

            //util::getInstance()->_SwitchToThisWindow((HWND)this->winId(),FALSE); //能切换窗口至前台,但会对其他窗口造成影响,不要用
            SetForegroundWindow((HWND)this->winId());//windows api

            //this->activateWindow();  //在Windows上,一般不允许程序打断用户操作,即不能切换窗口,所以该函数没用,得自己调用windows api来搞定
            //话说qt怎么什么都不支持..

        }
    }
	return QWidget::eventFilter(watched, event); 
}

void MainWindow::showUp()
{
   this->show();
}

void MainWindow::on_lineEdit_textChanged(const QString &input)
{
	HWND hwnd = (HWND)this->winId();

    if(m_lineEdit->text().isEmpty())
    {
		emit empty_input();	//为了避免堵塞界面线程,像这种可能花费时间的操作,统统发送信号,以异步方式处理
    }
    else if(!m_lineEdit->text().isEmpty())
    {

		emit user_input(input);
    }
}

void MainWindow::arrowKeyTakeover(QKeyEvent *event)
{
	return m_listView->keyEventForward(event);
}

void MainWindow::on_quit()
{
	tools->HideTrayIcon();
	this->close();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	return m_listView->keyEventForward(event);
}

