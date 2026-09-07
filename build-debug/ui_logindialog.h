/********************************************************************************
** Form generated from reading UI file 'logindialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QStackedWidget *stackedWidget;
    QWidget *page_login;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *le_loginUser;
    QLabel *label_email_login;
    QLineEdit *le_loginEmail;
    QLabel *label_3;
    QLineEdit *le_loginPwd;
    QCheckBox *cb_rememberPwd;
    QPushButton *pb_login;
    QPushButton *pb_switchToRegister;
    QLabel *label_serverHost;
    QLineEdit *le_serverHost;
    QLabel *label_serverPort;
    QLineEdit *le_serverPort;
    QPushButton *pb_connectServer;
    QWidget *page_register;
    QLabel *label_4;
    QLabel *label_5;
    QLineEdit *le_regUser;
    QLabel *label_email_reg;
    QLineEdit *le_regEmail;
    QLabel *label_6;
    QLineEdit *le_regPwd;
    QLabel *label_7;
    QLineEdit *le_regPwdConfirm;
    QPushButton *pb_register;
    QPushButton *pb_switchToLogin;

    void setupUi(QDialog *LoginDialog)
    {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName(QString::fromUtf8("LoginDialog"));
        LoginDialog->resize(400, 420);
        stackedWidget = new QStackedWidget(LoginDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(40, 30, 321, 351));
        page_login = new QWidget();
        page_login->setObjectName(QString::fromUtf8("page_login"));
        label = new QLabel(page_login);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(100, 10, 121, 31));
        QFont font;
        font.setPointSize(14);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);
        label_2 = new QLabel(page_login);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(30, 60, 72, 21));
        le_loginUser = new QLineEdit(page_login);
        le_loginUser->setObjectName(QString::fromUtf8("le_loginUser"));
        le_loginUser->setGeometry(QRect(100, 57, 191, 25));
        label_email_login = new QLabel(page_login);
        label_email_login->setObjectName(QString::fromUtf8("label_email_login"));
        label_email_login->setGeometry(QRect(30, 100, 72, 21));
        le_loginEmail = new QLineEdit(page_login);
        le_loginEmail->setObjectName(QString::fromUtf8("le_loginEmail"));
        le_loginEmail->setGeometry(QRect(100, 97, 191, 25));
        label_3 = new QLabel(page_login);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(30, 140, 72, 21));
        le_loginPwd = new QLineEdit(page_login);
        le_loginPwd->setObjectName(QString::fromUtf8("le_loginPwd"));
        le_loginPwd->setGeometry(QRect(100, 137, 191, 25));
        le_loginPwd->setEchoMode(QLineEdit::Password);
        cb_rememberPwd = new QCheckBox(page_login);
        cb_rememberPwd->setObjectName(QString::fromUtf8("cb_rememberPwd"));
        cb_rememberPwd->setGeometry(QRect(100, 175, 121, 21));
        pb_login = new QPushButton(page_login);
        pb_login->setObjectName(QString::fromUtf8("pb_login"));
        pb_login->setGeometry(QRect(60, 220, 93, 28));
        pb_switchToRegister = new QPushButton(page_login);
        pb_switchToRegister->setObjectName(QString::fromUtf8("pb_switchToRegister"));
        pb_switchToRegister->setGeometry(QRect(170, 220, 93, 28));
        label_serverHost = new QLabel(page_login);
        label_serverHost->setObjectName(QString::fromUtf8("label_serverHost"));
        label_serverHost->setGeometry(QRect(30, 270, 72, 21));
        le_serverHost = new QLineEdit(page_login);
        le_serverHost->setObjectName(QString::fromUtf8("le_serverHost"));
        le_serverHost->setGeometry(QRect(100, 267, 191, 25));
        label_serverPort = new QLabel(page_login);
        label_serverPort->setObjectName(QString::fromUtf8("label_serverPort"));
        label_serverPort->setGeometry(QRect(30, 305, 72, 21));
        le_serverPort = new QLineEdit(page_login);
        le_serverPort->setObjectName(QString::fromUtf8("le_serverPort"));
        le_serverPort->setGeometry(QRect(100, 302, 80, 25));
        pb_connectServer = new QPushButton(page_login);
        pb_connectServer->setObjectName(QString::fromUtf8("pb_connectServer"));
        pb_connectServer->setGeometry(QRect(195, 301, 96, 28));
        stackedWidget->addWidget(page_login);
        page_register = new QWidget();
        page_register->setObjectName(QString::fromUtf8("page_register"));
        label_4 = new QLabel(page_register);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(100, 10, 121, 31));
        label_4->setFont(font);
        label_4->setAlignment(Qt::AlignCenter);
        label_5 = new QLabel(page_register);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(30, 50, 72, 21));
        le_regUser = new QLineEdit(page_register);
        le_regUser->setObjectName(QString::fromUtf8("le_regUser"));
        le_regUser->setGeometry(QRect(100, 47, 191, 25));
        label_email_reg = new QLabel(page_register);
        label_email_reg->setObjectName(QString::fromUtf8("label_email_reg"));
        label_email_reg->setGeometry(QRect(30, 90, 72, 21));
        le_regEmail = new QLineEdit(page_register);
        le_regEmail->setObjectName(QString::fromUtf8("le_regEmail"));
        le_regEmail->setGeometry(QRect(100, 87, 191, 25));
        label_6 = new QLabel(page_register);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(30, 130, 72, 21));
        le_regPwd = new QLineEdit(page_register);
        le_regPwd->setObjectName(QString::fromUtf8("le_regPwd"));
        le_regPwd->setGeometry(QRect(100, 127, 191, 25));
        le_regPwd->setEchoMode(QLineEdit::Password);
        label_7 = new QLabel(page_register);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(20, 170, 81, 21));
        le_regPwdConfirm = new QLineEdit(page_register);
        le_regPwdConfirm->setObjectName(QString::fromUtf8("le_regPwdConfirm"));
        le_regPwdConfirm->setGeometry(QRect(100, 167, 191, 25));
        le_regPwdConfirm->setEchoMode(QLineEdit::Password);
        pb_register = new QPushButton(page_register);
        pb_register->setObjectName(QString::fromUtf8("pb_register"));
        pb_register->setGeometry(QRect(60, 220, 93, 28));
        pb_switchToLogin = new QPushButton(page_register);
        pb_switchToLogin->setObjectName(QString::fromUtf8("pb_switchToLogin"));
        pb_switchToLogin->setGeometry(QRect(170, 220, 93, 28));
        stackedWidget->addWidget(page_register);

        retranslateUi(LoginDialog);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog)
    {
        LoginDialog->setWindowTitle(QApplication::translate("LoginDialog", "\347\224\250\346\210\267\347\231\273\345\275\225", nullptr));
        label->setText(QApplication::translate("LoginDialog", "\347\224\250\346\210\267\347\231\273\345\275\225", nullptr));
        label_2->setText(QApplication::translate("LoginDialog", "\347\224\250\346\210\267\345\220\215", nullptr));
        le_loginUser->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\347\224\250\346\210\267\345\220\215", nullptr));
        label_email_login->setText(QApplication::translate("LoginDialog", "\351\202\256\347\256\261", nullptr));
        le_loginEmail->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\351\202\256\347\256\261", nullptr));
        label_3->setText(QApplication::translate("LoginDialog", "\345\257\206\347\240\201", nullptr));
        le_loginPwd->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\345\257\206\347\240\201", nullptr));
        cb_rememberPwd->setText(QApplication::translate("LoginDialog", "\350\256\260\344\275\217\345\257\206\347\240\201", nullptr));
        pb_login->setText(QApplication::translate("LoginDialog", "\347\231\273\345\275\225", nullptr));
        pb_switchToRegister->setText(QApplication::translate("LoginDialog", "\345\216\273\346\263\250\345\206\214", nullptr));
        label_serverHost->setText(QApplication::translate("LoginDialog", "\346\234\215\345\212\241\345\231\250", nullptr));
        le_serverHost->setPlaceholderText(QApplication::translate("LoginDialog", "\344\276\213\345\246\202 192.168.62.132", nullptr));
        label_serverPort->setText(QApplication::translate("LoginDialog", "\347\253\257\345\217\243", nullptr));
        le_serverPort->setPlaceholderText(QApplication::translate("LoginDialog", "8888", nullptr));
        pb_connectServer->setText(QApplication::translate("LoginDialog", "\350\277\236\346\216\245\346\234\215\345\212\241\345\231\250", nullptr));
        label_4->setText(QApplication::translate("LoginDialog", "\347\224\250\346\210\267\346\263\250\345\206\214", nullptr));
        label_5->setText(QApplication::translate("LoginDialog", "\347\224\250\346\210\267\345\220\215", nullptr));
        le_regUser->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\347\224\250\346\210\267\345\220\215", nullptr));
        label_email_reg->setText(QApplication::translate("LoginDialog", "\351\202\256\347\256\261", nullptr));
        le_regEmail->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\351\202\256\347\256\261", nullptr));
        label_6->setText(QApplication::translate("LoginDialog", "\345\257\206\347\240\201", nullptr));
        le_regPwd->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\350\276\223\345\205\245\345\257\206\347\240\201", nullptr));
        label_7->setText(QApplication::translate("LoginDialog", "\347\241\256\350\256\244\345\257\206\347\240\201", nullptr));
        le_regPwdConfirm->setPlaceholderText(QApplication::translate("LoginDialog", "\350\257\267\345\206\215\346\254\241\350\276\223\345\205\245\345\257\206\347\240\201", nullptr));
        pb_register->setText(QApplication::translate("LoginDialog", "\346\263\250\345\206\214", nullptr));
        pb_switchToLogin->setText(QApplication::translate("LoginDialog", "\345\216\273\347\231\273\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoginDialog: public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINDIALOG_H
