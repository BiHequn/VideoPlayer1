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
    , m_manualConnectRequested(false)
    , m_networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    this->setWindowTitle("用户登录");
    ui->stackedWidget->setCurrentIndex(0);
    const QString serverHost = m_settings.value("api/host", kDefaultApiHost).toString();
    ui->le_serverHost->setText(serverHost);
    ui->le_serverPort->setText(QString::number(m_settings.value("api/port", kDefaultApiPort).toInt()));
    ui->le_serverPort->setValidator(new QIntValidator(1, 65535, this));
    loadRememberedUser();
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

    if(username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }

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
        if (m_netClient) {
            m_netClient->setAccessToken(m_accessToken);
            m_netClient->setRefreshToken(QString());
            m_netClient->setUserId(0);
        }
        saveRememberedUser();
        emit SIG_loginSuccess(username);
        accept();
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

    if(host.isEmpty() || port <= 0 || port > 65535)
    {
        QMessageBox::warning(this, "提示", "请输入正确的服务器地址和端口");
        return;
    }

    m_settings.setValue("api/host", host);
    m_settings.setValue("api/port", port);
    m_settings.sync();

    m_manualConnectRequested = true;
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
        m_manualConnectRequested = false;
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

void LoginDialog::onRegisterResult(bool success, const QString &msg)
{
    ui->pb_register->setEnabled(true);
    if(success)
    {
        QMessageBox::information(this, "成功", "注册成功，请登录");
        ui->stackedWidget->setCurrentIndex(0);
    }
    else
    {
        QMessageBox::warning(this, "注册失败", msg);
    }
}

void LoginDialog::onLoginResult(bool success, const QString &msg, int uid, const QString &username,
                                const QString &accessToken, const QString &refreshToken)
{
    ui->pb_login->setEnabled(true);
    if(success)
    {
        m_netClient->setUserId(uid);
        m_netClient->setAccessToken(accessToken);
        m_netClient->setRefreshToken(refreshToken);
        saveRememberedUser();
        emit SIG_loginSuccess(username);
        this->accept();
    }
    else
    {
        QMessageBox::warning(this, "登录失败", msg);
    }
}

void LoginDialog::onServerConnected()
{
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("检测 API 服务");
    if(m_manualConnectRequested)
    {
        m_manualConnectRequested = false;
        QMessageBox::information(this, "提示", "服务器连接成功");
    }
}

void LoginDialog::onServerDisconnected()
{
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("检测 API 服务");
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    m_manualConnectRequested = false;
}

void LoginDialog::onConnectionError(const QString &error)
{
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("检测 API 服务");
    m_manualConnectRequested = false;
    if(error.contains("remote host closed", Qt::CaseInsensitive))
    {
        return;
    }
    QMessageBox::warning(this, "网络错误", error);
}

void LoginDialog::clearFields()
{
    ui->le_loginUser->clear();
    ui->le_loginEmail->clear();
    ui->le_loginPwd->clear();
    ui->le_regUser->clear();
    ui->le_regEmail->clear();
    ui->le_regPwd->clear();
    ui->le_regPwdConfirm->clear();
    ui->stackedWidget->setCurrentIndex(0);
}

bool LoginDialog::tryAutoLogin(const QString &username, const QString &password)
{
    Q_UNUSED(username);
    Q_UNUSED(password);
    return false;
}

void LoginDialog::setNetworkClient(NetworkClient *client)
{
    m_netClient = client;
}

QUrl LoginDialog::apiUrl(const QString &path) const
{
    return QUrl(QString("http://%1:%2%3")
                    .arg(ui->le_serverHost->text().trimmed())
                    .arg(ui->le_serverPort->text().toInt())
                    .arg(path));
}
