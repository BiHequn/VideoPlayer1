#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
#include <QBuffer>

#define NET_CHUNK_SIZE 4096
#define NET_RECONNECT_INTERVAL 3000
#define NET_HEARTBEAT_INTERVAL 30000
#define NET_REQUEST_TIMEOUT 10000
#define NET_MAX_RETRY 3

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    void connectToServer(const QString &host, quint16 port);
    void connectToServerWithAccessToken(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendRequest(int cmd, const QJsonObject &data);
    void sendRequestWithRetry(int cmd, const QJsonObject &data, int maxRetry = NET_MAX_RETRY);

    void registerUser(const QString &username, const QString &email, const QString &password,
                      const QString &phone = QString());
    void login(const QString &username, const QString &password, const QString &account = QString());
    void refreshToken(const QString &refreshToken);
    void authenticateWithAccessToken();

    void uploadFileInit(const QString &filename, qint64 filesize, const QString &fileMd5);
    void uploadFileChunk(int uploadId, const QString &fileMd5, qint64 offset, const QByteArray &chunkData);
    void uploadFileFinish(int uploadId, const QString &fileMd5, const QString &filename, qint64 filesize);

    void startChunkedUpload(const QString &filePath);
    void resumeUpload(int uploadId, const QString &fileMd5, qint64 uploadedSize, const QString &filePath);

    void requestVideoList();
    void requestVideoRecommend();
    void requestGifList();
    void requestStreamList();
    void reportVideoPlay(int videoId);
    void likeVideo(int videoId);

    void setAccessToken(const QString &token);
    void setRefreshToken(const QString &token);
    QString accessToken() const;
    QString refreshToken() const;
    int userId() const;
    void setUserId(int uid);

    QByteArray aesEncrypt(const QByteArray &data, const QByteArray &key);
    QByteArray aesDecrypt(const QByteArray &data, const QByteArray &key);

signals:
    void serverConnected();
    void serverDisconnected();
    void connectionError(const QString &error);

    void registerResult(bool success, const QString &msg);
    void loginResult(bool success, const QString &msg, int uid, const QString &username,
                     const QString &accessToken, const QString &refreshToken);
    void tokenRefreshResult(bool success, const QString &accessToken, const QString &refreshToken);
    void accessTokenAuthResult(bool success, int uid, const QString &username,
                               const QString &message);

    void uploadInitResult(const QString &result, int uploadId, qint64 uploadedSize, int chunkSize);
    void uploadChunkResult(int uploadId, qint64 uploadedSize);
    void uploadFinishResult(bool success, const QString &filepath);
    void uploadProgress(int percent);

    void videoListReceived(const QJsonArray &videos);
    void videoRecommendReceived(const QJsonArray &videos);
    void gifListReceived(const QJsonArray &gifs);
    void streamListReceived(const QJsonArray &streams);
    void videoPlayReported(bool success);
    void videoLikeResult(bool success, bool liked);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onHeartbeat();
    void onReconnectTimeout();
    void onRequestTimeout();

private:
    void processBuffer();
    void sendMessage(int cmd, const QJsonObject &data);
    void handleResponse(int cmd, const QJsonObject &resp);
    void sendNextUploadChunk();

    QTcpSocket *m_socket;
    QTimer *m_heartbeatTimer;
    QTimer *m_reconnectTimer;
    QTimer *m_requestTimer;

    QByteArray m_recvBuffer;

    QString m_host;
    quint16 m_port;
    bool m_isConnected;
    bool m_authenticateOnConnect;

    QString m_accessToken;
    QString m_refreshToken;
    int m_userId;

    QString m_uploadFilePath;
    int m_currentUploadId;
    qint64 m_uploadFileSize;
    qint64 m_uploadedSize;
    int m_chunkSize;
    QString m_uploadFileMd5;
    QFile *m_uploadFile;

    int m_retryCount;
    int m_lastCmd;
    QJsonObject m_lastData;

    QByteArray m_aesKey;
};

#endif
