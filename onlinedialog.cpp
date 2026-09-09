#include "onlinedialog.h"
#include "ui_onlinedialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QMovie>
#include <QLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QSaveFile>
#include <QDateTime>
#include <QThread>
#include <QMetaObject>

class FileCopyWorker : public QObject
{
    Q_OBJECT
public:
    FileCopyWorker(const QString &src, const QString &dest)
        : m_srcPath(src), m_destPath(dest) {}

public slots:
    void doCopy()
    {
        QFile sourceFile(m_srcPath);
        if (!sourceFile.open(QIODevice::ReadOnly))
        {
            emit error("无法打开源文件");
            return;
        }

        QFile destFile(m_destPath);
        if (!destFile.open(QIODevice::WriteOnly))
        {
            emit error("无法创建目标文件");
            return;
        }

        qint64 totalSize = sourceFile.size();
        qint64 bytesCopied = 0;
        char buffer[8192];

        while (!sourceFile.atEnd())
        {
            qint64 bytesRead = sourceFile.read(buffer, sizeof(buffer));
            if (bytesRead <= 0)
                break;

            qint64 bytesWritten = destFile.write(buffer, bytesRead);
            if (bytesWritten != bytesRead)
            {
                emit error("写入文件失败");
                return;
            }

            bytesCopied += bytesRead;
            int progress = static_cast<int>((bytesCopied * 100) / totalSize);
            emit progressChanged(progress);
        }

        emit completed();
    }

signals:
    void progressChanged(int progress);
    void completed();
    void error(const QString &msg);

private:
    QString m_srcPath;
    QString m_destPath;
};

#include "onlinedialog.moc"

OnlineDialog::OnlineDialog(NetworkClient *netClient, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OnlineDialog)
    , m_gifMovie(nullptr)
    , m_netClient(netClient)
    , m_pendingDownloadVideoId(0)
    , m_playAfterDownload(false)
{
    ui->setupUi(this);
    this->setWindowTitle("在线视频平台");
    initUploadDir();
    loadGifList();
    loadFileList();
    if(m_netClient)
    {
        connect(m_netClient, &NetworkClient::videoListReceived,
                this, &OnlineDialog::onVideoListReceived);
        connect(m_netClient, &NetworkClient::videoRecommendReceived,
                this, &OnlineDialog::onVideoRecommendReceived);
        connect(m_netClient, &NetworkClient::gifListReceived,
                this, &OnlineDialog::onGifListReceived);
        connect(m_netClient, &NetworkClient::streamListReceived,
                this, &OnlineDialog::onStreamListReceived);
        connect(m_netClient, &NetworkClient::uploadProgress,
                this, &OnlineDialog::onUploadProgress);
        connect(m_netClient, &NetworkClient::uploadFinishResult,
                this, &OnlineDialog::onUploadFinishResult);
        connect(m_netClient, &NetworkClient::uploadInitResult,
                this, &OnlineDialog::onUploadInitResult);
        connect(m_netClient, &NetworkClient::videoDownloadProgress,
                this, &OnlineDialog::onVideoDownloadProgress);
        connect(m_netClient, &NetworkClient::videoDownloadFinished,
                this, &OnlineDialog::onVideoDownloadFinished);
        connect(m_netClient, &NetworkClient::videoDownloadFailed,
                this, &OnlineDialog::onVideoDownloadFailed);
    }
}

OnlineDialog::~OnlineDialog()
{
    if (m_netClient && m_pendingDownloadVideoId > 0) {
        disconnect(m_netClient, nullptr, this, nullptr);
        m_netClient->cancelVideoDownload();
    }
    if(m_gifMovie)
    {
        m_gifMovie->stop();
        delete m_gifMovie;
    }
    delete ui;
}

void OnlineDialog::initUploadDir()
{
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_uploadDir = docPath + "/VideoPlayerUploads";
    QDir dir;
    if(!dir.exists(m_uploadDir))
    {
        dir.mkpath(m_uploadDir);
    }
    dir.setPath(m_uploadDir);
    if(!dir.exists("gif"))
        dir.mkdir("gif");
    if(!dir.exists("files"))
        dir.mkdir("files");
}

void OnlineDialog::setLoginUser(const QString &username)
{
    m_username = username;
    ui->label->setText("欢迎, " + username);
    ui->pushButton->setText(QString(username[0]).toUpper());

    if(m_netClient && m_netClient->isConnected())
    {
        loadServerVideoList();
        loadServerGifList();
        loadServerStreamList();
    }
}

void OnlineDialog::setNetworkClient(NetworkClient *client)
{
    m_netClient = client;
    if(m_netClient)
    {
        connect(m_netClient, &NetworkClient::videoListReceived,
                this, &OnlineDialog::onVideoListReceived);
        connect(m_netClient, &NetworkClient::videoRecommendReceived,
                this, &OnlineDialog::onVideoRecommendReceived);
        connect(m_netClient, &NetworkClient::gifListReceived,
                this, &OnlineDialog::onGifListReceived);
        connect(m_netClient, &NetworkClient::streamListReceived,
                this, &OnlineDialog::onStreamListReceived);
        connect(m_netClient, &NetworkClient::uploadProgress,
                this, &OnlineDialog::onUploadProgress);
        connect(m_netClient, &NetworkClient::uploadFinishResult,
                this, &OnlineDialog::onUploadFinishResult);
        connect(m_netClient, &NetworkClient::uploadInitResult,
                this, &OnlineDialog::onUploadInitResult);
        connect(m_netClient, &NetworkClient::videoDownloadProgress,
                this, &OnlineDialog::onVideoDownloadProgress);
        connect(m_netClient, &NetworkClient::videoDownloadFinished,
                this, &OnlineDialog::onVideoDownloadFinished);
        connect(m_netClient, &NetworkClient::videoDownloadFailed,
                this, &OnlineDialog::onVideoDownloadFailed);
    }
}

void OnlineDialog::on_pushButton_2_clicked()
{
    if(!m_username.isEmpty())
    {
        if (m_netClient && m_pendingDownloadVideoId > 0) {
            m_netClient->cancelVideoDownload();
        }
        emit SIG_logout();
    }
}

void OnlineDialog::on_pushButton_3_clicked()
{
    QString url = ui->lineEdit->text().trimmed();
    if(url.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入rtmp/hls地址");
        return;
    }
    emit SIG_openVideoPlayer(url);
}

void OnlineDialog::on_pushButton_4_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->listWidget->clear();
    if(m_netClient && m_netClient->isConnected())
    {
        loadServerStreamList();
    }
    else
    {
        QMessageBox::information(this, "提示", "当前未连接服务器，无法加载直播列表");
    }
}

void OnlineDialog::on_pushButton_5_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->listWidget->clear();
    if(m_netClient && m_netClient->isConnected())
    {
        m_netClient->requestVideoRecommend();
    }
    else
    {
        loadLocalRecommendedVideos();
    }
}

void OnlineDialog::showUploadProgress(const QString &fileName)
{
    ui->lb_uploadStatus->setText("正在上传: " + fileName);
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
}

void OnlineDialog::updateUploadProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void OnlineDialog::hideUploadProgress(bool success, const QString &msg)
{
    if(success)
    {
        ui->lb_uploadStatus->setText("上传成功");
    }
    else
    {
        ui->lb_uploadStatus->setText("上传失败: " + msg);
    }
    ui->progressBar->setVisible(false);

    QTimer::singleShot(3000, this, [this]() {
        ui->lb_uploadStatus->clear();
    });
}

void OnlineDialog::on_pushButton_7_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择GIF文件", "./",
        "GIF文件 (*.gif);;图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)");
    if(path.isEmpty()) return;

    QFileInfo fi(path);

    if(m_netClient && m_netClient->isConnected())
    {
        showUploadProgress(fi.fileName());
        m_netClient->startChunkedUpload(path);
    }
    else
    {
        QMessageBox::warning(this, "提示", "当前未连接服务端，不能上传GIF");
    }
}

void OnlineDialog::on_pushButton_8_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择文件", "./",
        "视频文件 (*.flv *.rmvb *.avi *.MP4 *.mkv *.mov *.wmv);;音频文件 (*.mp3);;所有文件 (*.*)");
    if(path.isEmpty()) return;

    QFileInfo fi(path);

    if(m_netClient && m_netClient->isConnected())
    {
        showUploadProgress(fi.fileName());
        m_netClient->startChunkedUpload(path);
    }
    else
    {
        QMessageBox::warning(this, "提示", "当前未连接服务端，不能上传视频");
    }
}

void OnlineDialog::on_pushButton_9_clicked()
{
    QListWidget *currentList = nullptr;
    if(ui->stackedWidget->currentIndex() == 1)
        currentList = ui->listWidget_download;
    else
        currentList = ui->listWidget;

    QListWidgetItem *item = currentList->currentItem();
    if(!item)
    {
        QMessageBox::warning(this, "提示", "请先选择要下载的文件");
        return;
    }
    const QString itemType = item->data(Qt::UserRole + 1).toString();
    const int videoId = item->data(Qt::UserRole + 2).toInt();
    const QString remoteFileName = item->data(Qt::UserRole + 3).toString();
    if ((itemType == "server_video" || itemType == "recommend") && videoId > 0)
    {
        const QString defaultName = remoteFileName.isEmpty()
            ? QString("video_%1").arg(videoId) : remoteFileName;
        const QString savePath = QFileDialog::getSaveFileName(
            this, "保存文件",
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
            "所有文件 (*.*)");
        if(savePath.isEmpty()) return;
        startServerVideoDownload(item, false, savePath);
        return;
    }

    QString srcPath = item->data(Qt::UserRole).toString();
    if(srcPath.isEmpty())
    {
        QMessageBox::warning(this, "提示", "该文件无法下载");
        return;
    }
    QFileInfo fi(srcPath);
    QString savePath = QFileDialog::getSaveFileName(this, "保存文件",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + fi.fileName(),
        "所有文件 (*.*)");
    if(savePath.isEmpty()) return;

    ui->lb_uploadStatus->setText("正在下载: " + fi.fileName());
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);

    FileCopyWorker *worker = new FileCopyWorker(srcPath, savePath);
    QThread *thread = new QThread;
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &FileCopyWorker::doCopy);
    connect(worker, &FileCopyWorker::progressChanged, this, &OnlineDialog::updateUploadProgress);
    connect(worker, &FileCopyWorker::completed, this, [this, worker, thread]() {
        ui->lb_uploadStatus->setText("下载成功");
        ui->progressBar->setVisible(false);
        QMessageBox::information(this, "成功", "文件下载成功！");
        QTimer::singleShot(3000, this, [this]() {
            ui->lb_uploadStatus->clear();
        });
        worker->deleteLater();
        thread->quit();
        thread->wait();
        thread->deleteLater();
    });
    connect(worker, &FileCopyWorker::error, this, [this, worker, thread](const QString &msg) {
        ui->lb_uploadStatus->setText("下载失败");
        ui->progressBar->setVisible(false);
        QMessageBox::warning(this, "失败", "文件下载失败: " + msg);
        QTimer::singleShot(3000, this, [this]() {
            ui->lb_uploadStatus->clear();
        });
        worker->deleteLater();
        thread->quit();
        thread->wait();
        thread->deleteLater();
    });

    thread->start();
}

void OnlineDialog::on_pushButton_6_clicked()
{
    loadGifList();
    loadFileList();
    if(m_netClient && m_netClient->isConnected())
    {
        loadServerVideoList();
        loadServerGifList();
        loadServerStreamList();
    }
}

void OnlineDialog::on_pushButton_10_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    loadFileList();
    if(m_netClient && m_netClient->isConnected())
    {
        m_netClient->requestVideoList();
    }
}

void OnlineDialog::on_pushButton_11_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void OnlineDialog::on_listWidget_itemClicked(QListWidgetItem *item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if(filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    if(fi.suffix().toLower() == "gif")
    {
        if(m_gifMovie)
        {
            m_gifMovie->stop();
            delete m_gifMovie;
            m_gifMovie = nullptr;
        }
        m_gifMovie = new QMovie(filePath);
        ui->lb_gifPreview->setMovie(m_gifMovie);
        ui->lb_gifPreview->setScaledContents(true);
        m_gifMovie->start();
    }
    else
    {
        if(m_gifMovie)
        {
            m_gifMovie->stop();
            delete m_gifMovie;
            m_gifMovie = nullptr;
        }
        QPixmap pix(filePath);
        if(!pix.isNull())
        {
            ui->lb_gifPreview->setPixmap(pix.scaled(ui->lb_gifPreview->size(), Qt::KeepAspectRatio));
        }
    }
}

void OnlineDialog::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if(filePath.isEmpty()) return;

    int videoId = item->data(Qt::UserRole + 2).toInt();
    QString itemType = item->data(Qt::UserRole + 1).toString();
    if ((itemType == "server_video" || itemType == "recommend") && videoId > 0)
    {
        startServerVideoDownload(item, true);
        return;
    }
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if(suffix == "mp4" || suffix == "avi" || suffix == "mkv" || suffix == "flv" ||
       suffix == "rmvb" || suffix == "mp3" || suffix == "mov" || suffix == "wmv")
    {
        emit SIG_openVideoPlayer(filePath);
        return;
    }

    if(itemType == "stream")
    {
        emit SIG_openVideoPlayer(filePath);
    }
}

void OnlineDialog::loadGifList()
{
    QDir gifDir(m_uploadDir + "/gif");
    QStringList filters;
    filters << "*.gif" << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QFileInfoList fileList = gifDir.entryInfoList(filters, QDir::Files, QDir::Time);

    ui->listWidget->clear();
    for(int i = 0; i < fileList.size(); ++i)
    {
        QFileInfo &fi = fileList[i];
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());

        QPixmap pix(fi.absoluteFilePath());
        if(!pix.isNull())
        {
            item->setIcon(QIcon(pix.scaled(64, 64, Qt::KeepAspectRatio)));
        }
        ui->listWidget->addItem(item);
    }
}

void OnlineDialog::loadFileList()
{
    QDir fileDir(m_uploadDir + "/files");
    QStringList filters;
    filters << "*.mp4" << "*.avi" << "*.mkv" << "*.flv" << "*.rmvb" << "*.mp3" << "*.mov" << "*.wmv";
    QFileInfoList fileList = fileDir.entryInfoList(filters, QDir::Files, QDir::Time);

    ui->listWidget_download->clear();
    for(int i = 0; i < fileList.size(); ++i)
    {
        QFileInfo &fi = fileList[i];
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fi.fileName() + "  (" + QString::number(fi.size() / 1024) + " KB)");
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        ui->listWidget_download->addItem(item);
    }
}

void OnlineDialog::loadServerVideoList()
{
    if(m_netClient && m_netClient->isConnected())
    {
        m_netClient->requestVideoList();
    }
}

void OnlineDialog::loadServerGifList()
{
    if(m_netClient && m_netClient->isConnected())
    {
        m_netClient->requestGifList();
    }
}

void OnlineDialog::loadServerStreamList()
{
    if(m_netClient && m_netClient->isConnected())
    {
        m_netClient->requestStreamList();
    }
}

void OnlineDialog::startServerVideoDownload(QListWidgetItem *item, bool playAfterDownload,
                                            const QString &savePath)
{
    if (!item || !m_netClient || !m_netClient->isConnected()) {
        QMessageBox::warning(this, "下载失败", "当前未连接媒体服务。");
        return;
    }
    if (m_pendingDownloadVideoId > 0) {
        QMessageBox::information(this, "提示", "已有视频正在下载，请稍候。");
        return;
    }

    const int videoId = item->data(Qt::UserRole + 2).toInt();
    if (videoId <= 0) {
        QMessageBox::warning(this, "下载失败", "视频编号无效。");
        return;
    }
    m_pendingDownloadVideoId = videoId;
    m_playAfterDownload = playAfterDownload;
    m_downloadSavePath = savePath;
    ui->lb_uploadStatus->setText("正在下载: " + item->data(Qt::UserRole + 3).toString());
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->pb_cancelDownload->setVisible(true);
    m_netClient->startVideoDownload(videoId);
}

void OnlineDialog::onVideoListReceived(const QJsonArray &videos)
{
    ui->listWidget_download->clear();
    loadFileList();
    int insertPos = ui->listWidget_download->count();
    for(int i = 0; i < videos.size(); ++i)
    {
        QJsonObject obj = videos[i].toObject();
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(obj["filename"].toString() + "  [服务器]");
        item->setData(Qt::UserRole, obj["filepath"].toString());
        item->setData(Qt::UserRole + 1, "server_video");
        item->setData(Qt::UserRole + 2, obj["id"].toInt());
        item->setData(Qt::UserRole + 3, obj["filename"].toString());
        ui->listWidget_download->insertItem(insertPos++, item);
    }
}

void OnlineDialog::loadLocalRecommendedVideos()
{
    QDir fileDir(m_uploadDir + "/files");
    QStringList filters;
    filters << "*.mp4" << "*.avi" << "*.mkv" << "*.flv" << "*.rmvb" << "*.mp3" << "*.mov" << "*.wmv";
    QFileInfoList fileList = fileDir.entryInfoList(filters, QDir::Files, QDir::Time);

    ui->listWidget->clear();
    for(int i = 0; i < fileList.size(); ++i)
    {
        QFileInfo &fi = fileList[i];
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fi.fileName() + "  [本地推荐]");
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setData(Qt::UserRole + 1, "local_recommend");
        ui->listWidget->addItem(item);
    }

    if(fileList.isEmpty())
    {
        QMessageBox::information(this, "提示", "当前没有可推荐的本地视频，请先上传视频");
    }
}

void OnlineDialog::onVideoRecommendReceived(const QJsonArray &videos)
{
    ui->listWidget->clear();
    for(int i = 0; i < videos.size(); ++i)
    {
        QJsonObject obj = videos[i].toObject();
        QListWidgetItem *item = new QListWidgetItem;
        QString source = obj["source"].toString();
        QString tag = source == "collaborative" ? " [协同推荐]" : " [热度推荐]";
        item->setText(obj["filename"].toString() + tag);
        item->setData(Qt::UserRole, obj["filepath"].toString());
        item->setData(Qt::UserRole + 1, "recommend");
        item->setData(Qt::UserRole + 2, obj["id"].toInt());
        item->setData(Qt::UserRole + 3, obj["filename"].toString());
        ui->listWidget->addItem(item);
    }
}

void OnlineDialog::onGifListReceived(const QJsonArray &gifs)
{
    for(int i = 0; i < gifs.size(); ++i)
    {
        QJsonObject obj = gifs[i].toObject();
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(obj["filename"].toString() + "  [服务器]");
        item->setData(Qt::UserRole, obj["filepath"].toString());
        item->setData(Qt::UserRole + 1, "server_gif");
        ui->listWidget->addItem(item);
    }
}

void OnlineDialog::onStreamListReceived(const QJsonArray &streams)
{
    ui->listWidget->clear();
    for(int i = 0; i < streams.size(); ++i)
    {
        QJsonObject obj = streams[i].toObject();
        QString playUrl = obj["hls_url"].toString().trimmed();
        if(playUrl.isEmpty())
        {
            playUrl = obj["rtmp_url"].toString();
        }
        QListWidgetItem *item = new QListWidgetItem;
        item->setText("直播: " + obj["title"].toString() + " (" + QString::number(obj["viewer_count"].toInt()) + "人观看)");
        item->setData(Qt::UserRole, playUrl);
        item->setData(Qt::UserRole + 1, "stream");
        ui->listWidget->addItem(item);
    }
}

void OnlineDialog::onUploadProgress(int percent)
{
    updateUploadProgress(percent);
}

void OnlineDialog::onUploadFinishResult(bool success, const QString &filepath)
{
    hideUploadProgress(success, filepath);
    if(success)
    {
        loadFileList();
        loadGifList();
    }
}

void OnlineDialog::onUploadInitResult(const QString &result, int uploadId, qint64 uploadedSize, int chunkSize)
{
    Q_UNUSED(result);
    Q_UNUSED(uploadId);
    Q_UNUSED(uploadedSize);
    Q_UNUSED(chunkSize);
}

void OnlineDialog::onVideoDownloadProgress(int videoId, int percent)
{
    if (videoId != m_pendingDownloadVideoId) {
        return;
    }
    ui->progressBar->setValue(percent);
}

void OnlineDialog::onVideoDownloadFinished(int videoId, const QString &localPath)
{
    if (videoId != m_pendingDownloadVideoId) {
        return;
    }

    const bool shouldPlay = m_playAfterDownload;
    const QString savePath = m_downloadSavePath;
    m_pendingDownloadVideoId = 0;
    m_playAfterDownload = false;
    m_downloadSavePath.clear();
    ui->progressBar->setVisible(false);
    ui->pb_cancelDownload->setVisible(false);
    ui->lb_uploadStatus->setText("下载完成");

    if (!savePath.isEmpty()) {
        QFile source(localPath);
        QSaveFile destination(savePath);
        if (!source.open(QIODevice::ReadOnly) ||
            !destination.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "保存失败", "无法将缓存视频复制到指定位置。");
            return;
        }
        while (!source.atEnd()) {
            const QByteArray chunk = source.read(64 * 1024);
            if (chunk.isEmpty() || destination.write(chunk) != chunk.size()) {
                destination.cancelWriting();
                QMessageBox::warning(this, "保存失败", "无法将缓存视频复制到指定位置。");
                return;
            }
        }
        if (!destination.commit()) {
            QMessageBox::warning(this, "保存失败", "无法将缓存视频复制到指定位置。");
            return;
        }
        QMessageBox::information(this, "下载完成", "视频已保存到：" + savePath);
    }
    if (shouldPlay) {
        if (m_netClient && m_netClient->isConnected()) {
            m_netClient->reportVideoPlay(videoId);
        }
        emit SIG_openVideoPlayer(localPath);
    }
}

void OnlineDialog::onVideoDownloadFailed(int videoId, const QString &message)
{
    if (videoId != m_pendingDownloadVideoId) {
        return;
    }
    m_pendingDownloadVideoId = 0;
    m_playAfterDownload = false;
    m_downloadSavePath.clear();
    ui->progressBar->setVisible(false);
    ui->pb_cancelDownload->setVisible(false);
    ui->lb_uploadStatus->setText("下载失败");
    QMessageBox::warning(this, "下载失败", message);
}

void OnlineDialog::on_pb_cancelDownload_clicked()
{
    if (m_netClient) {
        m_netClient->cancelVideoDownload();
    }
}
