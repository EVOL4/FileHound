#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QWinHotKey/QWinHotkey.h"
#include "QFileListView.h"
#include "QInputLineEdit.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
	virtual void keyPressEvent(QKeyEvent *event)  override;

private:
    Ui::MainWindow *ui;
    util* tools;
	QFileListView* m_listView;
	QInputLineEdit* m_lineEdit;

public slots:
    void showUp();

private slots:
    void on_lineEdit_textChanged(const QString &input);
	void arrowKeyTakeover(QKeyEvent *event);
	void on_quit();

signals:
	void user_input(const QString &input);
	void empty_input();
};

#endif // MAINWINDOW_H
