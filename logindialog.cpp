#include "logindialog.h"
#include "ui_logindialog.h"
#include <QMessageBox>
#include <QIntValidator>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {
const char kDefaultApiHost[] = "127.0.0.1";
const int kDefaultApiPort = 8000;
const int kDefaultMediaPort = 8888;

QString responseDetail(const QByteArray &body)
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    return document.isObject() ? document.object().value("detail").toString() : QString();
}
}

LoginDialog::LoginDialog(NetworkClient *netClient, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , m_settings("VideoPlayer", "VideoPlayer1")
    , m_netClient(netClient)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_waitingForMediaAuthentication(false)
{
    ui->setupUi(this);
    this->setWindowTitle("用户登录");
    ui->stackedWidget->setCurrentIndex(0);
    const QString serverHost = m_settings.value("api/host", kDefaultApiHost).toString();
    ui->le_serverHost->setText(serverHost);
    ui->le_serverPort->setText(QString::number(m_settings.value("api/port", kDefaultApiPort).toInt()));
    ui->le_serverPort->setValidator(new QIntValidator(1, 65535, this));
    ui->le_mediaPort->setText(QString::number(m_settings.value("media/port", kDefaultMediaPort).toInt()));
    ui->le_mediaPort->setValidator(new QIntValidator(1, 65535, this));
    loadRememberedUser();
    if (m_netClient) {
        connect(m_netClient, &NetworkClient::accessTokenAuthResult,
                this, &LoginDialog::onAccessTokenAuthResult);
        connect(m_netClient, &NetworkClient::connectionError,
                this, &LoginDialog::onConnectionError);
        connect(m_netClient, &NetworkClient::serverDisconnected,
                this, &LoginDialog::onMediaServerDisconnected);
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::loadRememberedUser()
{
    if(m_settings.contains("login/username"))
    {
        ui->le_loginUser->setText(m_settings.value("login/username").toString());
        ui->cb_rememberPwd->setChecked(m_settings.value("login/rememberUsername", false).toBool());
    }
}

void LoginDialog::saveRememberedUser()
{
    if(ui->cb_rememberPwd->isChecked())
    {
        m_settings.setValue("login/username", ui->le_loginUser->text());
        m_settings.setValue("login/rememberUsername", true);
    }
    else
    {
        m_settings.remove("login/username");
        m_settings.remove("login/rememberUsername");
    }
    m_settings.remove("login/account");
    m_settings.remove("login/password");
    m_settings.remove("login/rememberPassword");
    m_settings.remove("login/accessToken");
    m_settings.remove("login/refreshToken");
    m_settings.sync();
}

void LoginDialog::on_pb_login_clicked()
{
    QString username = ui->le_loginUser->text().trimmed();
    QString password = ui->le_loginPwd->text();
    const QString host = ui->le_serverHost->text().trimmed();
    const int apiPort = ui->le_serverPort->text().toInt();
    const int mediaPort = ui->le_mediaPort->text().toInt();

    if(username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }

    if(host.isEmpty() || apiPort <= 0 || apiPort > 65535 ||
       mediaPort <= 0 || mediaPort > 65535)
    {
        QMessageBox::warning(this, "提示", "请输入正确的服务器地址和端口");
        return;
    }

    m_settings.setValue("api/host", host);
    m_settings.setValue("api/port", apiPort);
    m_settings.setValue("media/port", mediaPort);
    m_settings.sync();

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QNetworkRequest request(apiUrl("/api/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    ui->pb_login->setEnabled(false);
    QNetworkReply *reply = m_networkManager->post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray responseBody = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(responseBody);
        const QJsonObject response = document.object();
        ui->pb_login->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            const QString detail = responseDetail(responseBody);
            QMessageBox::warning(this, "登录失败", detail.isEmpty()
                                 ? QString("无法连接 API 服务：%1").arg(reply->errorString())
                                 : detail);
            reply->deleteLater();
            return;
        }

        m_accessToken = response.value("access_token").toString();
        const QString username = response.value("username").toString();
        if (m_accessToken.isEmpty() || username.isEmpty()) {
            QMessageBox::warning(this, "登录失败", "服务端返回了无效的登录响应。");
            reply->deleteLater();
            return;
        }
        if (!m_netClient) {
            QMessageBox::warning(this, "登录失败", "媒体服务客户端不可用。");
            reply->deleteLater();
            return;
        }

        m_pendingUsername = username;
        m_waitingForMediaAuthentication = true;
        m_netClient->setAccessToken(m_accessToken);
        m_netClient->setRefreshToken(QString());
        m_netClient->setUserId(0);
        ui->pb_login->setEnabled(false);

        const QString mediaHost = ui->le_serverHost->text().trimmed();
        const int mediaPort = ui->le_mediaPort->text().toInt();
        if (m_netClient->isConnected()) {
            m_netClient->authenticateWithAccessToken();
        } else {
            m_netClient->connectToServerWithAccessToken(
                mediaHost, static_cast<quint16>(mediaPort));
        }
        reply->deleteLater();
    });
}

void LoginDialog::on_pb_register_clicked()
{
    QString username = ui->le_regUser->text().trimmed();
    QString password = ui->le_regPwd->text();
    QString confirmPwd = ui->le_regPwdConfirm->text();

    if(username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }

    if(password != confirmPwd)
    {
        QMessageBox::warning(this, "提示", "两次密码不一致");
        return;
    }

    if(username.length() < 3)
    {
        QMessageBox::warning(this, "提示", "用户名长度至少为3位");
        return;
    }

    if(password.length() < 8)
    {
        QMessageBox::warning(this, "提示", "密码长度至少为8位");
        return;
    }

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QNetworkRequest request(apiUrl("/api/auth/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    ui->pb_register->setEnabled(false);
    QNetworkReply *reply = m_networkManager->post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray responseBody = reply->readAll();
        ui->pb_register->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            const QString detail = responseDetail(responseBody);
            QMessageBox::warning(this, "注册失败", detail.isEmpty()
                                 ? QString("无法连接 API 服务：%1").arg(reply->errorString())
                                 : detail);
            reply->deleteLater();
            return;
        }

        QMessageBox::information(this, "注册成功", "账号创建成功，请登录。");
        ui->le_loginUser->setText(ui->le_regUser->text().trimmed());
        ui->le_loginPwd->clear();
        ui->stackedWidget->setCurrentIndex(0);
        reply->deleteLater();
    });
}

void LoginDialog::on_pb_switchToRegister_clicked()
{
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void LoginDialog::on_pb_switchToLogin_clicked()
{
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(0);
}

void LoginDialog::on_pb_connectServer_clicked()
{
    QString host = ui->le_serverHost->text().trimmed();
    int port = ui->le_serverPort->text().toInt();
    int mediaPort = ui->le_mediaPort->text().toInt();

    if(host.isEmpty() || port <= 0 || port > 65535 ||
       mediaPort <= 0 || mediaPort > 65535)
    {
        QMessageBox::warning(this, "提示", "请输入正确的服务器地址和端口");
        return;
    }

    m_settings.setValue("api/host", host);
    m_settings.setValue("api/port", port);
    m_settings.setValue("media/port", mediaPort);
    m_settings.sync();

    ui->pb_connectServer->setEnabled(false);
    ui->pb_connectServer->setText("检测中...");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(apiUrl("/health")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray responseBody = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(responseBody);
        const bool healthy = reply->error() == QNetworkReply::NoError
                             && document.object().value("status").toString() == "ok";
        ui->pb_connectServer->setEnabled(true);
        ui->pb_connectServer->setText("检测 API 服务");
        if (healthy) {
            QMessageBox::information(this, "提示", "API 服务连接成功。");
        } else {
            QMessageBox::warning(this, "网络错误", QString("API 服务不可用：%1")
                                 .arg(reply->error() == QNetworkReply::NoError
                                      ? QString("健康检查响应无效") : reply->errorString()));
        }
        reply->deleteLater();
    });
}

void LoginDialog::onConnectionError(const QString &error)
{
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("检测 API 服务");
    if (m_waitingForMediaAuthentication) {
        m_waitingForMediaAuthentication = false;
        m_pendingUsername.clear();
        m_netClient->disconnectFromServer();
        QMessageBox::warning(this, "登录失败",
                             QString("API 登录成功，但媒体服务连接失败：%1").arg(error));
        return;
    }
    if(error.contains("remote host closed", Qt::CaseInsensitive))
    {
        return;
    }
    QMessageBox::warning(this, "网络错误", error);
}

void LoginDialog::onAccessTokenAuthResult(bool success, int uid, const QString &username,
                                          const QString &message)
{
    if (!m_waitingForMediaAuthentication) {
        return;
    }

    m_waitingForMediaAuthentication = false;
    ui->pb_login->setEnabled(true);
    if (!success) {
        m_pendingUsername.clear();
        m_netClient->disconnectFromServer();
        QMessageBox::warning(this, "登录失败",
                             message.isEmpty() ? "媒体服务拒绝了登录令牌。" : message);
        return;
    }

    m_netClient->setUserId(uid);
    const QString authenticatedUsername = username.isEmpty() ? m_pendingUsername : username;
    m_pendingUsername.clear();
    saveRememberedUser();
    emit SIG_loginSuccess(authenticatedUsername);
    accept();
}

void LoginDialog::onMediaServerDisconnected()
{
    if (!m_waitingForMediaAuthentication) {
        return;
    }

    m_waitingForMediaAuthentication = false;
    m_pendingUsername.clear();
    ui->pb_login->setEnabled(true);
    QMessageBox::warning(this, "登录失败", "媒体服务在登录验证期间断开连接。");
}

void LoginDialog::clearFields()
{
    ui->le_loginUser->clear();
    ui->le_loginPwd->clear();
    ui->le_regUser->clear();
    ui->le_regPwd->clear();
    ui->le_regPwdConfirm->clear();
    ui->stackedWidget->setCurrentIndex(0);
}

QUrl LoginDialog::apiUrl(const QString &path) const
{
    return QUrl(QString("http://%1:%2%3")
                    .arg(ui->le_serverHost->text().trimmed())
                    .arg(ui->le_serverPort->text().toInt())
                    .arg(path));
}
