/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include "qfilelistview.h"
#include "qinputlineedit.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QFileListView *listView;
    QGroupBox *groupBox_main;
    QInputLineEdit *lineEdit;
    QGroupBox *groupBox;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1093, 658);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        listView = new QFileListView(centralWidget);
        listView->setObjectName(QStringLiteral("listView"));
        listView->setEnabled(true);
        listView->setGeometry(QRect(52, 130, 571, 12));
        listView->setStyleSheet(QLatin1String("QListView{\n"
" background-color:rgb(27, 25, 56);\n"
"}"));
        listView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        listView->setMovement(QListView::Free);
        listView->setResizeMode(QListView::Adjust);
        groupBox_main = new QGroupBox(centralWidget);
        groupBox_main->setObjectName(QStringLiteral("groupBox_main"));
        groupBox_main->setGeometry(QRect(32, 60, 621, 70));
        groupBox_main->setStyleSheet(QLatin1String("QGroupBox{\n"
" border-radius:8px;\n"
" background-color:rgb(27, 25, 56);\n"
"}"));
        groupBox_main->setFlat(true);
        lineEdit = new QInputLineEdit(groupBox_main);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setGeometry(QRect(0, 0, 480, 69));
        lineEdit->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
" font-family:\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
" font-size:20px;\n"
" color:#FFFFFF;\n"
" border-top-left-radius:8px;\n"
" border-bottom-left-radius:8px;\n"
" border-color: rgb(60, 70, 255);\n"
" border-width:2px;\n"
" background-color:rgb(27, 25, 56);\n"
" padding-left:10px;\n"
" padding-right:10px;\n"
"}\n"
""));
        lineEdit->setClearButtonEnabled(true);
        groupBox = new QGroupBox(groupBox_main);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(480, 0, 132, 69));
        groupBox->setStyleSheet(QLatin1String("QGroupBox{\n"
" border-top-right-radius:8px;\n"
" border-bottom-right-radius:8px;\n"
" background-color:rgb(27, 25, 56);\n"
"}"));
        groupBox->setAlignment(Qt::AlignJustify|Qt::AlignVCenter);
        groupBox->setFlat(true);
        pushButton = new QPushButton(groupBox);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(0, 0, 44, 69));
        pushButton->setStyleSheet(QLatin1String("QPushButton{\n"
" color:#FFFFFF;\n"
" background-color:rgb(27, 25, 56);\n"
" border-color:rgb(255, 255, 255);\n"
" border-radius:5px;\n"
" \n"
"}\n"
"\n"
"QPushButton:hover{\n"
" border:2px groove #FFFFFF;\n"
"}"));
        pushButton_2 = new QPushButton(groupBox);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(44, 0, 44, 69));
        pushButton_2->setStyleSheet(QLatin1String("QPushButton{\n"
" color:#FFFFFF;\n"
" background-color:rgb(27, 25, 56);\n"
" border-color:rgb(255, 255, 255);\n"
" border-radius:5px;\n"
" \n"
"}\n"
"\n"
"QPushButton:hover{\n"
" border:2px groove #FFFFFF;\n"
"}"));
        pushButton_3 = new QPushButton(groupBox);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));
        pushButton_3->setGeometry(QRect(88, 0, 44, 69));
        pushButton_3->setStyleSheet(QLatin1String("QPushButton{\n"
" color:#FFFFFF;\n"
" background-color:rgb(27, 25, 56);\n"
" border-color:rgb(255, 255, 255);\n"
" border-radius:5px;\n"
" \n"
"}\n"
"\n"
"QPushButton:hover{\n"
" border:2px groove #FFFFFF;\n"
"}"));
        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", Q_NULLPTR));
        groupBox_main->setTitle(QString());
        lineEdit->setPlaceholderText(QApplication::translate("MainWindow", "Search", Q_NULLPTR));
        groupBox->setTitle(QString());
        pushButton->setText(QApplication::translate("MainWindow", "A", Q_NULLPTR));
        pushButton_2->setText(QApplication::translate("MainWindow", "B", Q_NULLPTR));
        pushButton_3->setText(QApplication::translate("MainWindow", "C", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
