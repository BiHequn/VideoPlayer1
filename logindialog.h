#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include "network/networkclient.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(NetworkClient *netClient, QWidget *parent = nullptr);
    ~LoginDialog();

    void clearFields();
    bool tryAutoLogin(const QString &username, const QString &password);
    void setNetworkClient(NetworkClient *client);

signals:
    void SIG_loginSuccess(const QString &username);

private slots:
    void on_pb_login_clicked();
    void on_pb_register_clicked();
    void on_pb_switchToRegister_clicked();
    void on_pb_switchToLogin_clicked();
    void on_pb_connectServer_clicked();

    void onRegisterResult(bool success, const QString &msg);
    void onLoginResult(bool success, const QString &msg, int uid, const QString &username,
                       const QString &accessToken, const QString &refreshToken);
    void onServerConnected();
    void onServerDisconnected();
    void onConnectionError(const QString &error);

private:
    Ui::LoginDialog *ui;
    QSqlDatabase m_db;
    QSettings m_settings;
    NetworkClient *m_netClient;
    bool m_manualConnectRequested;

    bool initDatabase();
    bool validateEmail(const QString &email);
    bool validatePhone(const QString &phone);
    QString currentLoginAccount() const;
    void generateSmsCode();
    void loadRememberedUser();
    void saveRememberedUser();

    QString m_smsCode;
    QTimer m_smsCodeTimer;
    QDateTime m_smsCodeExpireAt;
};

#endif
