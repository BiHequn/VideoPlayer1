#include "logindialog.h"
#include "ui_logindialog.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QDateTime>
#include <QRandomGenerator>
#include <QIntValidator>

LoginDialog::LoginDialog(NetworkClient *netClient, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , m_settings("VideoPlayer", "VideoPlayer1")
    , m_netClient(netClient)
    , m_manualConnectRequested(false)
{
    ui->setupUi(this);
    this->setWindowTitle("用户登录");
    ui->stackedWidget->setCurrentIndex(0);
    QString serverHost = m_settings.value("server/host", "192.168.62.132").toString();
    if (serverHost == "127.0.0.1" || serverHost.compare("localhost", Qt::CaseInsensitive) == 0) {
        serverHost = "192.168.62.132";
    }
    ui->le_serverHost->setText(serverHost);
    ui->le_serverPort->setText(QString::number(m_settings.value("server/port", 8888).toInt()));
    ui->le_serverPort->setValidator(new QIntValidator(1, 65535, this));
    initDatabase();
    loadRememberedUser();
    connect(&m_smsCodeTimer, &QTimer::timeout, this, [this]() {
        m_smsCode.clear();
    });

    connect(m_netClient, &NetworkClient::registerResult,
            this, &LoginDialog::onRegisterResult);
    connect(m_netClient, &NetworkClient::loginResult,
            this, &LoginDialog::onLoginResult);
    connect(m_netClient, &NetworkClient::serverConnected,
            this, &LoginDialog::onServerConnected);
    connect(m_netClient, &NetworkClient::serverDisconnected,
            this, &LoginDialog::onServerDisconnected);
    connect(m_netClient, &NetworkClient::connectionError,
            this, &LoginDialog::onConnectionError);
}

LoginDialog::~LoginDialog()
{
    if(m_db.isOpen())
        m_db.close();
    delete ui;
}

bool LoginDialog::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("videoplayer_users.db");
    if(!m_db.open())
    {
        QMessageBox::critical(this, "错误", "数据库打开失败");
        return false;
    }
    QSqlQuery query;
    QString sql = "CREATE TABLE IF NOT EXISTS users ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "username TEXT UNIQUE NOT NULL,"
                  "email TEXT UNIQUE NOT NULL,"
                  "phone TEXT UNIQUE,"
                  "password TEXT NOT NULL,"
                  "created_at TEXT NOT NULL)";
    if(!query.exec(sql))
    {
        qDebug() << "创建表失败:" << query.lastError().text();
        return false;
    }

    QString videoSql = "CREATE TABLE IF NOT EXISTS videos ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "user_id INTEGER NOT NULL,"
                       "filename TEXT NOT NULL,"
                       "filepath TEXT NOT NULL,"
                       "filesize INTEGER NOT NULL,"
                       "duration REAL DEFAULT 0,"
                       "created_at TEXT NOT NULL,"
                       "FOREIGN KEY(user_id) REFERENCES users(id))";
    if(!query.exec(videoSql))
    {
        qDebug() << "创建videos表失败:" << query.lastError().text();
    }

    query.exec("ALTER TABLE users ADD COLUMN phone TEXT");

    return true;
}

bool LoginDialog::validateEmail(const QString &email)
{
    QRegularExpression re(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return re.match(email).hasMatch();
}

bool LoginDialog::validatePhone(const QString &phone)
{
    QRegularExpression re(R"(^1\d{10}$)");
    return re.match(phone).hasMatch();
}

QString LoginDialog::currentLoginAccount() const
{
    return ui->le_loginEmail->text().trimmed();
}

void LoginDialog::generateSmsCode()
{
    m_smsCode = QString::number(QRandomGenerator::global()->bounded(100000, 1000000));
    m_smsCodeExpireAt = QDateTime::currentDateTime().addSecs(300);
    m_smsCodeTimer.start(300000);
}

void LoginDialog::loadRememberedUser()
{
    if(m_settings.contains("login/username"))
    {
        ui->le_loginUser->setText(m_settings.value("login/username").toString());
        ui->le_loginEmail->setText(m_settings.value("login/account").toString());
        ui->le_loginPwd->setText(m_settings.value("login/password").toString());
        ui->cb_rememberPwd->setChecked(m_settings.value("login/rememberPassword", false).toBool());
    }
}

void LoginDialog::saveRememberedUser()
{
    if(ui->cb_rememberPwd->isChecked())
    {
        m_settings.setValue("login/username", ui->le_loginUser->text());
        m_settings.setValue("login/account", ui->le_loginEmail->text().trimmed());
        m_settings.setValue("login/password", ui->le_loginPwd->text());
        m_settings.setValue("login/rememberPassword", true);
        m_settings.setValue("login/accessToken", m_netClient->accessToken());
        m_settings.setValue("login/refreshToken", m_netClient->refreshToken());
    }
    else
    {
        m_settings.remove("login/username");
        m_settings.remove("login/account");
        m_settings.remove("login/password");
        m_settings.remove("login/rememberPassword");
        m_settings.remove("login/accessToken");
        m_settings.remove("login/refreshToken");
    }
    m_settings.sync();
}

void LoginDialog::on_pb_login_clicked()
{
    QString username = ui->le_loginUser->text().trimmed();
    QString account = currentLoginAccount();
    QString password = ui->le_loginPwd->text();

    if(username.isEmpty() || account.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名、手机号/邮箱和密码不能为空");
        return;
    }

    if(!validateEmail(account) && !validatePhone(account))
    {
        QMessageBox::warning(this, "提示", "请输入有效的手机号或邮箱");
        return;
    }

    if(m_netClient->isConnected())
    {
        ui->pb_login->setEnabled(false);
        m_netClient->login(username, password, account);
    }
    else
    {
        QMessageBox::warning(this, "提示", "当前未连接服务端，请先点击“连接服务器”后再登录");
    }
}

void LoginDialog::on_pb_register_clicked()
{
    QString username = ui->le_regUser->text().trimmed();
    QString account = ui->le_regEmail->text().trimmed();
    QString password = ui->le_regPwd->text();
    QString confirmPwd = ui->le_regPwdConfirm->text();
    QString email;
    QString phone;

    if(username.isEmpty() || account.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名、手机号/邮箱和密码不能为空");
        return;
    }

    if(validatePhone(account))
    {
        phone = account;
        email = account + "@phone.local";
        generateSmsCode();
    }
    else if(validateEmail(account))
    {
        email = account;
    }
    else
    {
        QMessageBox::warning(this, "提示", "请输入有效的手机号或邮箱");
        return;
    }

    if(password != confirmPwd)
    {
        QMessageBox::warning(this, "提示", "两次密码不一致");
        return;
    }

    if(password.length() < 6)
    {
        QMessageBox::warning(this, "提示", "密码长度至少为6位");
        return;
    }

    if(m_netClient->isConnected())
    {
        ui->pb_register->setEnabled(false);
        m_netClient->registerUser(username, email, password, phone);
    }
    else
    {
        QMessageBox::warning(this, "提示", "当前未连接服务端，请先点击“连接服务器”后再注册");
    }
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

    m_settings.setValue("server/host", host);
    m_settings.setValue("server/port", port);
    m_settings.sync();

    if(m_netClient)
    {
        m_manualConnectRequested = true;
        ui->pb_connectServer->setEnabled(false);
        ui->pb_connectServer->setText("连接中...");
        m_netClient->connectToServer(host, static_cast<quint16>(port));
    }
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
    ui->pb_connectServer->setText("连接服务器");
    if(m_manualConnectRequested)
    {
        m_manualConnectRequested = false;
        QMessageBox::information(this, "提示", "服务器连接成功");
    }
}

void LoginDialog::onServerDisconnected()
{
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("连接服务器");
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    m_manualConnectRequested = false;
}

void LoginDialog::onConnectionError(const QString &error)
{
    ui->pb_login->setEnabled(true);
    ui->pb_register->setEnabled(true);
    ui->pb_connectServer->setEnabled(true);
    ui->pb_connectServer->setText("连接服务器");
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
    if(username.isEmpty() || password.isEmpty())
        return false;

    if(!m_db.isOpen())
    {
        initDatabase();
    }

    QByteArray hashPwd = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex();

    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", QString(hashPwd));

    return query.exec() && query.next();
}

void LoginDialog::setNetworkClient(NetworkClient *client)
{
    m_netClient = client;
    if(m_netClient)
    {
        connect(m_netClient, &NetworkClient::registerResult,
                this, &LoginDialog::onRegisterResult);
        connect(m_netClient, &NetworkClient::loginResult,
                this, &LoginDialog::onLoginResult);
        connect(m_netClient, &NetworkClient::serverConnected,
                this, &LoginDialog::onServerConnected);
        connect(m_netClient, &NetworkClient::serverDisconnected,
                this, &LoginDialog::onServerDisconnected);
        connect(m_netClient, &NetworkClient::connectionError,
                this, &LoginDialog::onConnectionError);
    }
}
