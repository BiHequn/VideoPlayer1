/********************************************************************************
** Form generated from reading UI file 'onlinedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ONLINEDIALOG_H
#define UI_ONLINEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OnlineDialog
{
public:
    QPushButton *pushButton;
    QLabel *label;
    QPushButton *pushButton_2;
    QLineEdit *lineEdit;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QPushButton *pushButton_6;
    QPushButton *pushButton_7;
    QPushButton *pushButton_8;
    QPushButton *pushButton_9;
    QPushButton *pushButton_10;
    QLabel *lb_uploadStatus;
    QProgressBar *progressBar;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QListWidget *listWidget;
    QLabel *lb_gifPreview;
    QWidget *page_2;
    QListWidget *listWidget_download;
    QPushButton *pushButton_11;

    void setupUi(QDialog *OnlineDialog)
    {
        if (OnlineDialog->objectName().isEmpty())
            OnlineDialog->setObjectName(QString::fromUtf8("OnlineDialog"));
        OnlineDialog->resize(900, 700);
        OnlineDialog->setMinimumSize(QSize(900, 700));
        pushButton = new QPushButton(OnlineDialog);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(20, 10, 60, 60));
        QFont font;
        font.setPointSize(20);
        pushButton->setFont(font);
        label = new QLabel(OnlineDialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(100, 25, 300, 30));
        QFont font1;
        font1.setPointSize(14);
        label->setFont(font1);
        pushButton_2 = new QPushButton(OnlineDialog);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(20, 80, 75, 23));
        lineEdit = new QLineEdit(OnlineDialog);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
        lineEdit->setGeometry(QRect(140, 80, 521, 25));
        pushButton_3 = new QPushButton(OnlineDialog);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(690, 80, 81, 25));
        pushButton_4 = new QPushButton(OnlineDialog);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
        pushButton_4->setGeometry(QRect(20, 110, 75, 23));
        pushButton_5 = new QPushButton(OnlineDialog);
        pushButton_5->setObjectName(QString::fromUtf8("pushButton_5"));
        pushButton_5->setGeometry(QRect(110, 110, 75, 23));
        pushButton_6 = new QPushButton(OnlineDialog);
        pushButton_6->setObjectName(QString::fromUtf8("pushButton_6"));
        pushButton_6->setGeometry(QRect(200, 110, 75, 23));
        pushButton_7 = new QPushButton(OnlineDialog);
        pushButton_7->setObjectName(QString::fromUtf8("pushButton_7"));
        pushButton_7->setGeometry(QRect(290, 110, 75, 23));
        pushButton_8 = new QPushButton(OnlineDialog);
        pushButton_8->setObjectName(QString::fromUtf8("pushButton_8"));
        pushButton_8->setGeometry(QRect(380, 110, 75, 23));
        pushButton_9 = new QPushButton(OnlineDialog);
        pushButton_9->setObjectName(QString::fromUtf8("pushButton_9"));
        pushButton_9->setGeometry(QRect(470, 110, 75, 23));
        pushButton_10 = new QPushButton(OnlineDialog);
        pushButton_10->setObjectName(QString::fromUtf8("pushButton_10"));
        pushButton_10->setGeometry(QRect(560, 110, 75, 23));
        lb_uploadStatus = new QLabel(OnlineDialog);
        lb_uploadStatus->setObjectName(QString::fromUtf8("lb_uploadStatus"));
        lb_uploadStatus->setGeometry(QRect(660, 110, 200, 23));
        lb_uploadStatus->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        progressBar = new QProgressBar(OnlineDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(660, 140, 220, 20));
        progressBar->setValue(0);
        progressBar->setVisible(false);
        stackedWidget = new QStackedWidget(OnlineDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(10, 170, 881, 521));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        listWidget = new QListWidget(page);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setGeometry(QRect(10, 10, 280, 501));
        listWidget->setIconSize(QSize(64, 64));
        lb_gifPreview = new QLabel(page);
        lb_gifPreview->setObjectName(QString::fromUtf8("lb_gifPreview"));
        lb_gifPreview->setGeometry(QRect(310, 10, 560, 501));
        lb_gifPreview->setFrameShape(QFrame::Box);
        lb_gifPreview->setAlignment(Qt::AlignCenter);
        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        listWidget_download = new QListWidget(page_2);
        listWidget_download->setObjectName(QString::fromUtf8("listWidget_download"));
        listWidget_download->setGeometry(QRect(10, 10, 861, 501));
        pushButton_11 = new QPushButton(page_2);
        pushButton_11->setObjectName(QString::fromUtf8("pushButton_11"));
        pushButton_11->setGeometry(QRect(760, 480, 100, 28));
        stackedWidget->addWidget(page_2);

        retranslateUi(OnlineDialog);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(OnlineDialog);
    } // setupUi

    void retranslateUi(QDialog *OnlineDialog)
    {
        OnlineDialog->setWindowTitle(QApplication::translate("OnlineDialog", "\345\234\250\347\272\277\350\247\206\351\242\221\345\271\263\345\217\260", nullptr));
        pushButton->setText(QApplication::translate("OnlineDialog", "U", nullptr));
        label->setText(QApplication::translate("OnlineDialog", "\346\234\252\347\231\273\345\275\225", nullptr));
        pushButton_2->setText(QApplication::translate("OnlineDialog", "\351\200\200\345\207\272\347\231\273\345\275\225", nullptr));
        lineEdit->setText(QString());
        lineEdit->setPlaceholderText(QApplication::translate("OnlineDialog", "\350\276\223\345\205\245rtmp\345\234\260\345\235\200", nullptr));
        pushButton_3->setText(QApplication::translate("OnlineDialog", "\346\222\255\346\224\276rtmp", nullptr));
        pushButton_4->setText(QApplication::translate("OnlineDialog", "\345\234\260\346\226\271\345\217\260\347\233\264\346\222\255", nullptr));
        pushButton_5->setText(QApplication::translate("OnlineDialog", "\346\216\250\350\215\220\345\275\261\350\247\206", nullptr));
        pushButton_6->setText(QApplication::translate("OnlineDialog", "\345\210\267\346\226\260", nullptr));
        pushButton_7->setText(QApplication::translate("OnlineDialog", "\344\270\212\344\274\240GIF", nullptr));
        pushButton_8->setText(QApplication::translate("OnlineDialog", "\344\270\212\344\274\240\350\247\206\351\242\221", nullptr));
        pushButton_9->setText(QApplication::translate("OnlineDialog", "\344\270\213\350\275\275\346\226\207\344\273\266", nullptr));
        pushButton_10->setText(QApplication::translate("OnlineDialog", "\344\270\212\344\274\240\345\216\206\345\217\262", nullptr));
        lb_uploadStatus->setText(QString());
        lb_gifPreview->setText(QApplication::translate("OnlineDialog", "GIF\351\242\204\350\247\210\345\214\272", nullptr));
        pushButton_11->setText(QApplication::translate("OnlineDialog", "\350\277\224\345\233\236\344\270\273\351\241\265", nullptr));
    } // retranslateUi

};

namespace Ui {
    class OnlineDialog: public Ui_OnlineDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ONLINEDIALOG_H
