
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileListView.h"




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    tools(util::getInstance())
{
    ui->setupUi(this);
    //ȥ�����ı������������пؼ���focus
    ui->pushButton->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_2->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_3->setFocusPolicy(Qt::NoFocus);
    //��װ�¼�������,Ҫ��QObject��дbool eventFilter(QObject *watched, QEvent *event)�麯��
    this->installEventFilter(this);



    //ע���ݼ�
    //todo:�Ƶ�util��ȥ,utilӦ�ø���������ȼ�
    QWinHotKey * hotkey = new QWinHotKey();
    //qApp->installNativeEventFilter(hotkey);  //����ʱ�����û��,���º����޷�����,�������������е�..
    hotkey->setShortcut(tr("ctrl+h")); //Ŀ���ݼ����
    connect(hotkey, SIGNAL(activated()), this, SLOT(showUp()));//���Ӹ��ź������ǵĲ�

    //�ſ����ӰЧ��
    auto shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setOffset(0, 0);
    shadowEffect->setColor(QColor(90,78,85));
    shadowEffect->setBlurRadius(15);
    ui->groupBox_main->setGraphicsEffect(shadowEffect);

	connect(this, SIGNAL(user_input()), ui->listView, SLOT(showResults()));


	tools->SetUpTrayIcon();

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
            ui->lineEdit->clear();
             //����������ֵ������,���������ֳ���ʱ,listview�����²�Ӱ,������on_lineEdit_textChangedû���ü�ִ���������������,�������Խ��
            tools->sleep(50); //ps:����windows��Sleep�ڵ��������û����
            this->hide(); //qtò�Ʋ�֧��ȫ���ȼ�,���ַ���ע��Ŀ�ݼ��ڴ���û����ʱ�ǲ���Ч����,�Լ�����windows��api�����
        }
        else if(QApplication::activeWindow()==this)
        {

            //util::getInstance()->_SwitchToThisWindow((HWND)this->winId(),FALSE); //���л�������ǰ̨,����������������Ӱ��,��Ҫ��
            SetForegroundWindow((HWND)this->winId());//windows api

            //this->activateWindow();  //��Windows��,һ�㲻����������û�����,�������л�����,���Ըú���û��,���Լ�����windows api���㶨
            //��˵qt��ôʲô����֧��..

        }
    }
    return QWidget::eventFilter(watched,event);
}

void MainWindow::showUp()
{
   this->show();
}

void MainWindow::on_lineEdit_textChanged(const QString &input)
{
    Q_UNUSED(input);
    if(ui->lineEdit->text().isEmpty()&& !ui->listView->isHidden())
    {
        ui->listView->reset();
        ui->listView->hide();
    }
    else if(ui->listView->isHidden()&&!ui->lineEdit->text().isEmpty())
    {
		//todo:��������ʱ������һ���ļ������������,����Ӧ�޸ĵø�����չ��
		emit user_input();
		
		
    }

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	return ui->listView->keyEventForward(event);
}

