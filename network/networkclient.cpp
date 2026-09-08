#include "networkclient.h"
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

static const char *AES_KEY = "VideoPlayer2026!";

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_requestTimer(new QTimer(this))
    , m_port(0)
    , m_isConnected(false)
    , m_authenticateOnConnect(false)
    , m_userId(0)
    , m_currentUploadId(0)
    , m_uploadFileSize(0)
    , m_uploadedSize(0)
    , m_chunkSize(NET_CHUNK_SIZE)
    , m_uploadFile(nullptr)
    , m_retryCount(0)
    , m_lastCmd(0)
{
    m_aesKey = QByteArray(AES_KEY, 16);

    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &NetworkClient::onSocketError);

    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkClient::onHeartbeat);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkClient::onReconnectTimeout);
    connect(m_requestTimer, &QTimer::timeout, this, &NetworkClient::onRequestTimeout);

    m_reconnectTimer->setSingleShot(true);
    m_requestTimer->setSingleShot(true);
}

NetworkClient::~NetworkClient()
{
    if (m_uploadFile) {
        m_uploadFile->close();
        delete m_uploadFile;
    }
    disconnectFromServer();
}

void NetworkClient::connectToServer(const QString &host, quint16 port)
{
    m_authenticateOnConnect = false;
    m_host = host;
    m_port = port;
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->connectToHost(host, port);
}

void NetworkClient::connectToServerWithAccessToken(const QString &host, quint16 port)
{
    m_authenticateOnConnect = true;
    m_host = host;
    m_port = port;
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->connectToHost(host, port);
}

void NetworkClient::disconnectFromServer()
{
    m_heartbeatTimer->stop();
    m_reconnectTimer->stop();
    m_requestTimer->stop();
    m_host.clear();
    m_port = 0;
    m_authenticateOnConnect = false;
    m_socket->disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return m_isConnected;
}

void NetworkClient::onSocketConnected()
{
    m_isConnected = true;
    m_retryCount = 0;
    m_heartbeatTimer->start(NET_HEARTBEAT_INTERVAL);
    emit serverConnected();
    if (m_authenticateOnConnect && !m_accessToken.isEmpty()) {
        authenticateWithAccessToken();
    }
}

void NetworkClient::onSocketDisconnected()
{
    m_isConnected = false;
    m_heartbeatTimer->stop();
    m_requestTimer->stop();
    emit serverDisconnected();

    if (!m_host.isEmpty() && m_port > 0) {
        m_reconnectTimer->start(NET_RECONNECT_INTERVAL);
    }
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit connectionError(m_socket->errorString());

    if (!m_host.isEmpty() && m_port > 0) {
        m_reconnectTimer->start(NET_RECONNECT_INTERVAL);
    }
}

void NetworkClient::onHeartbeat()
{
    if (m_isConnected) {
        QJsonObject data;
        data["client_time"] = QDateTime::currentSecsSinceEpoch();
        sendMessage(5, data);
    }
}

void NetworkClient::onReconnectTimeout()
{
    if (!m_isConnected && !m_host.isEmpty() && m_port > 0) {
        m_socket->connectToHost(m_host, m_port);
    }
}

void NetworkClient::onRequestTimeout()
{
    if (m_retryCount < NET_MAX_RETRY) {
        m_retryCount++;
        sendRequest(m_lastCmd, m_lastData);
    } else {
        m_retryCount = 0;
        emit connectionError("请求超时，重试次数已达上限");
    }
}

void NetworkClient::sendRequest(int cmd, const QJsonObject &data)
{
    if (!m_isConnected) {
        emit connectionError("未连接到服务器");
        return;
    }
    sendMessage(cmd, data);
    m_requestTimer->start(NET_REQUEST_TIMEOUT);
}

void NetworkClient::sendRequestWithRetry(int cmd, const QJsonObject &data, int maxRetry)
{
    m_lastCmd = cmd;
    m_lastData = data;
    m_retryCount = 0;
    sendRequest(cmd, data);
    Q_UNUSED(maxRetry);
}

void NetworkClient::sendMessage(int cmd, const QJsonObject &data)
{
    QJsonObject msg = data;
    msg["cmd"] = cmd;
    if (!m_accessToken.isEmpty() && cmd != 1 && cmd != 2 && cmd != 4 && cmd != 6) {
        msg["access_token"] = m_accessToken;
    }

    QJsonDocument doc(msg);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);
    qDebug() << "[NetworkClient] send cmd =" << cmd << "json =" << QString::fromUtf8(jsonBytes);

    QByteArray encrypted = aesEncrypt(jsonBytes, m_aesKey);

    int len = encrypted.size();
    QByteArray header(4, 0);
    header[0] = (len >> 24) & 0xFF;
    header[1] = (len >> 16) & 0xFF;
    header[2] = (len >> 8) & 0xFF;
    header[3] = len & 0xFF;

    m_socket->write(header);
    m_socket->write(encrypted);
    m_socket->flush();
}

void NetworkClient::onReadyRead()
{
    m_recvBuffer.append(m_socket->readAll());

    while (m_recvBuffer.size() >= 4) {
        int msgLen = ((unsigned char)m_recvBuffer[0] << 24) |
                     ((unsigned char)m_recvBuffer[1] << 16) |
                     ((unsigned char)m_recvBuffer[2] << 8) |
                     (unsigned char)m_recvBuffer[3];

        if (msgLen <= 0 || msgLen > 10 * 1024 * 1024) {
            m_recvBuffer.clear();
            return;
        }

        if (m_recvBuffer.size() < 4 + msgLen) break;

        QByteArray encData = m_recvBuffer.mid(4, msgLen);
        QByteArray jsonBytes = aesDecrypt(encData, m_aesKey);

        m_recvBuffer.remove(0, 4 + msgLen);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);
        if (err.error != QJsonParseError::NoError) continue;

        QJsonObject resp = doc.object();
        int cmd = resp["cmd"].toInt();
        qDebug() << "[NetworkClient] recv cmd =" << cmd << "json =" << QString::fromUtf8(jsonBytes);
        handleResponse(cmd, resp);
    }
}

void NetworkClient::handleResponse(int cmd, const QJsonObject &resp)
{
    m_requestTimer->stop();
    m_retryCount = 0;

    switch (cmd) {
    case 1001: {
        bool ok = resp["result"].toString() == "ok";
        emit registerResult(ok, resp["msg"].toString());
        break;
    }
    case 1002: {
        bool ok = resp["result"].toString() == "ok";
        if (ok) {
            m_userId = resp["uid"].toInt();
            m_accessToken = resp["access_token"].toString();
            m_refreshToken = resp["refresh_token"].toString();
        }
        emit loginResult(ok, resp["msg"].toString(),
                         resp["uid"].toInt(), resp["username"].toString(),
                         resp["access_token"].toString(), resp["refresh_token"].toString());
        break;
    }
    case 1004: {
        bool ok = resp["result"].toString() == "ok";
        if (ok) {
            m_accessToken = resp["access_token"].toString();
            m_refreshToken = resp["refresh_token"].toString();
        }
        emit tokenRefreshResult(ok, resp["access_token"].toString(), resp["refresh_token"].toString());
        break;
    }
    case 1005: {
        break;
    }
    case 1006: {
        const bool ok = resp["result"].toString() == "ok";
        if (ok) {
            m_userId = resp["uid"].toInt();
        }
        emit accessTokenAuthResult(ok, resp["uid"].toInt(),
                                   resp["username"].toString(),
                                   resp["msg"].toString());
        break;
    }
    case 1010: {
        QString result = resp["result"].toString();
        int uploadId = resp["upload_id"].toInt();
        qint64 uploadedSize = resp["uploaded_size"].toVariant().toLongLong();
        int chunkSize = resp["chunk_size"].toInt();
        m_currentUploadId = uploadId;
        m_uploadedSize = uploadedSize;
        m_chunkSize = chunkSize;
        emit uploadInitResult(result, uploadId, uploadedSize, chunkSize);

        if (result == "ok" || result == "resume") {
            if (m_uploadFile && m_uploadFile->isOpen()) {
                m_uploadFile->seek(uploadedSize);
                sendNextUploadChunk();
            }
        }
        break;
    }
    case 1011: {
        int uploadId = resp["upload_id"].toInt();
        qint64 uploadedSize = resp["uploaded_size"].toVariant().toLongLong();
        m_uploadedSize = uploadedSize;
        emit uploadChunkResult(uploadId, uploadedSize);

        int percent = m_uploadFileSize > 0 ? static_cast<int>((m_uploadedSize * 100) / m_uploadFileSize) : 0;
        emit uploadProgress(percent);
        sendNextUploadChunk();
        break;
    }
    case 1012: {
        bool ok = resp["result"].toString() == "ok";
        emit uploadFinishResult(ok, resp["filepath"].toString());
        break;
    }
    case 1030: {
        emit videoListReceived(resp["videos"].toArray());
        break;
    }
    case 1031: {
        emit videoLikeResult(resp["result"].toString() == "ok", resp["liked"].toBool());
        break;
    }
    case 1032: {
        emit videoPlayReported(resp["result"].toString() == "ok");
        break;
    }
    case 1033: {
        emit videoRecommendReceived(resp["videos"].toArray());
        break;
    }
    case 1040: {
        emit gifListReceived(resp["gifs"].toArray());
        break;
    }
    case 1052: {
        emit streamListReceived(resp["streams"].toArray());
        break;
    }
    }
}

void NetworkClient::registerUser(const QString &username, const QString &email, const QString &password,
                                 const QString &phone)
{
    QJsonObject data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!phone.isEmpty()) {
        data["phone"] = phone;
    }
    sendRequestWithRetry(1, data);
}

void NetworkClient::login(const QString &username, const QString &password, const QString &account)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    if (!account.isEmpty()) {
        data["account"] = account;
    }
    sendRequestWithRetry(2, data);
}

void NetworkClient::refreshToken(const QString &rt)
{
    QJsonObject data;
    data["refresh_token"] = rt;
    sendRequest(4, data);
}

void NetworkClient::authenticateWithAccessToken()
{
    if (m_accessToken.isEmpty()) {
        emit accessTokenAuthResult(false, 0, QString(), "访问令牌为空");
        return;
    }
    QJsonObject data;
    data["access_token"] = m_accessToken;
    sendRequestWithRetry(6, data);
}

void NetworkClient::uploadFileInit(const QString &filename, qint64 filesize, const QString &fileMd5)
{
    QJsonObject data;
    data["filename"] = filename;
    data["filesize"] = filesize;
    data["file_md5"] = fileMd5;
    sendRequest(10, data);
}

void NetworkClient::uploadFileChunk(int uploadId, const QString &fileMd5, qint64 offset, const QByteArray &chunkData)
{
    QJsonObject data;
    data["upload_id"] = uploadId;
    data["file_md5"] = fileMd5;
    data["offset"] = QString::number(offset);
    data["chunk_data"] = QString(chunkData.toBase64());
    sendMessage(11, data);
}

void NetworkClient::uploadFileFinish(int uploadId, const QString &fileMd5, const QString &filename, qint64 filesize)
{
    QJsonObject data;
    data["upload_id"] = uploadId;
    data["file_md5"] = fileMd5;
    data["filename"] = filename;
    data["filesize"] = QString::number(filesize);
    sendRequest(12, data);
}

void NetworkClient::sendNextUploadChunk()
{
    if (!m_uploadFile || !m_uploadFile->isOpen()) {
        return;
    }

    if (m_uploadedSize >= m_uploadFileSize || m_uploadFile->atEnd()) {
        uploadFileFinish(m_currentUploadId, m_uploadFileMd5,
                         QFileInfo(m_uploadFilePath).fileName(), m_uploadFileSize);
        m_uploadFile->close();
        delete m_uploadFile;
        m_uploadFile = nullptr;
        return;
    }

    m_uploadFile->seek(m_uploadedSize);
    QByteArray chunk = m_uploadFile->read(m_chunkSize);
    if (chunk.isEmpty()) {
        emit uploadFinishResult(false, "读取上传分片失败");
        return;
    }

    uploadFileChunk(m_currentUploadId, m_uploadFileMd5, m_uploadedSize, chunk);
}

void NetworkClient::startChunkedUpload(const QString &filePath)
{
    m_uploadFilePath = filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadFinishResult(false, "无法打开文件");
        return;
    }
    m_uploadFileSize = file.size();

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    m_uploadFileMd5 = QString(hash.result().toHex());
    file.close();

    m_uploadFile = new QFile(filePath);
    if (!m_uploadFile->open(QIODevice::ReadOnly)) {
        delete m_uploadFile;
        m_uploadFile = nullptr;
        emit uploadFinishResult(false, "无法打开文件");
        return;
    }

    uploadFileInit(QFileInfo(filePath).fileName(), m_uploadFileSize, m_uploadFileMd5);
}

void NetworkClient::resumeUpload(int uploadId, const QString &fileMd5, qint64 uploadedSize, const QString &filePath)
{
    m_uploadFilePath = filePath;
    m_currentUploadId = uploadId;
    m_uploadFileMd5 = fileMd5;
    m_uploadedSize = uploadedSize;

    m_uploadFile = new QFile(filePath);
    if (!m_uploadFile->open(QIODevice::ReadOnly)) {
        delete m_uploadFile;
        m_uploadFile = nullptr;
        emit uploadFinishResult(false, "无法打开文件");
        return;
    }
    m_uploadFileSize = m_uploadFile->size();
    m_uploadFile->seek(uploadedSize);

    sendNextUploadChunk();
}

void NetworkClient::requestVideoList()
{
    QJsonObject data;
    sendRequest(30, data);
}

void NetworkClient::requestVideoRecommend()
{
    QJsonObject data;
    sendRequest(33, data);
}

void NetworkClient::reportVideoPlay(int videoId)
{
    if (videoId <= 0) return;
    QJsonObject data;
    data["video_id"] = videoId;
    sendMessage(32, data);
}

void NetworkClient::likeVideo(int videoId)
{
    if (videoId <= 0) return;
    QJsonObject data;
    data["video_id"] = videoId;
    sendRequest(31, data);
}

void NetworkClient::requestGifList()
{
    QJsonObject data;
    sendRequest(40, data);
}

void NetworkClient::requestStreamList()
{
    QJsonObject data;
    sendRequest(52, data);
}

void NetworkClient::setAccessToken(const QString &token) { m_accessToken = token; }
void NetworkClient::setRefreshToken(const QString &token) { m_refreshToken = token; }
QString NetworkClient::accessToken() const { return m_accessToken; }
QString NetworkClient::refreshToken() const { return m_refreshToken; }
int NetworkClient::userId() const { return m_userId; }
void NetworkClient::setUserId(int uid) { m_userId = uid; }

QByteArray NetworkClient::aesEncrypt(const QByteArray &data, const QByteArray &key)
{
    QByteArray result = data;
    for (int i = 0; i < result.size(); i++) {
        result[i] = result[i] ^ key[i % key.size()];
    }
    return result.toBase64();
}

QByteArray NetworkClient::aesDecrypt(const QByteArray &data, const QByteArray &key)
{
    QByteArray decoded = QByteArray::fromBase64(data);
    for (int i = 0; i < decoded.size(); i++) {
        decoded[i] = decoded[i] ^ key[i % key.size()];
    }
    return decoded;
}
