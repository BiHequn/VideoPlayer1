#ifndef ONLINEDIALOG_H
#define ONLINEDIALOG_H

#include <QDialog>
#include <QMovie>
#include <QListWidgetItem>
#include <QTimer>
#include "network/networkclient.h"

namespace Ui {
class OnlineDialog;
}

class OnlineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OnlineDialog(NetworkClient *netClient, QWidget *parent = nullptr);
    ~OnlineDialog();

    void setLoginUser(const QString &username);
    void setNetworkClient(NetworkClient *client);

signals:
    void SIG_openVideoPlayer(const QString &path);
    void SIG_logout();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_10_clicked();
    void on_pushButton_11_clicked();
    void on_pb_cancelDownload_clicked();
    void on_listWidget_itemClicked(QListWidgetItem *item);
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void onVideoListReceived(const QJsonArray &videos);
    void onVideoRecommendReceived(const QJsonArray &videos);
    void onGifListReceived(const QJsonArray &gifs);
    void onStreamListReceived(const QJsonArray &streams);
    void onUploadProgress(int percent);
    void onUploadFinishResult(bool success, const QString &filepath);
    void onUploadInitResult(const QString &result, int uploadId, qint64 uploadedSize, int chunkSize);
    void onVideoDownloadProgress(int videoId, int percent);
    void onVideoDownloadFinished(int videoId, const QString &localPath);
    void onVideoDownloadFailed(int videoId, const QString &message);

private:
    Ui::OnlineDialog *ui;
    QMovie *m_gifMovie;
    QString m_username;
    QString m_uploadDir;
    NetworkClient *m_netClient;
    int m_pendingDownloadVideoId;
    bool m_playAfterDownload;
    QString m_downloadSavePath;

    void initUploadDir();
    void loadGifList();
    void loadFileList();
    void loadLocalRecommendedVideos();
    void loadServerVideoList();
    void loadServerGifList();
    void loadServerStreamList();
    void startServerVideoDownload(QListWidgetItem *item, bool playAfterDownload,
                                  const QString &savePath = QString());
    void showUploadProgress(const QString &fileName);
    void updateUploadProgress(int progress);
    void hideUploadProgress(bool success, const QString &msg);
};

#endif
