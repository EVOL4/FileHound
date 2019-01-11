﻿#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QWinHotKey/qwinhotkey.h"



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
	virtual void MainWindow::keyPressEvent(QKeyEvent *event)  override;

private:
    Ui::MainWindow *ui;
    util* tools;

public slots:
    void showUp();

private slots:
    void on_lineEdit_textChanged(const QString &input);

signals:
	void user_input();
};

#endif // MAINWINDOW_H