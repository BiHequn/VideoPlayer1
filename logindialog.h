#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QSettings>
#include "network/networkclient.h"
#include <QString>

class QNetworkAccessManager;
class QUrl;

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

signals:
    void SIG_loginSuccess(const QString &username);

private slots:
    void on_pb_login_clicked();
    void on_pb_register_clicked();
    void on_pb_switchToRegister_clicked();
    void on_pb_switchToLogin_clicked();
    void on_pb_connectServer_clicked();

    void onConnectionError(const QString &error);
    void onMediaServerDisconnected();
    void onAccessTokenAuthResult(bool success, int uid, const QString &username,
                                 const QString &message);

private:
    Ui::LoginDialog *ui;
    QSettings m_settings;
    NetworkClient *m_netClient;

    void loadRememberedUser();
    void saveRememberedUser();
    QUrl apiUrl(const QString &path) const;

    QNetworkAccessManager *m_networkManager;
    QString m_accessToken;
    QString m_pendingUsername;
    bool m_waitingForMediaAuthentication;
};

#endif
