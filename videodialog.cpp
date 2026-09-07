#include "videodialog.h"
#include "ui_videodialog.h"
#include<QDebug>
#include "videoplayer.h"
#include <QFileDialog>
#include <QStyle>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileInfo>

#define _DEF_PATH  "http://111.40.196.9/PLTV/88888888/224/3221225610/index.m3u8"
#define _DEF_LIVE_PATH  "rtmp://192.168.62.132/videotest/user=100"

VideoDialog::VideoDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VideoDialog)
    , isFullScreen(false)
    , m_playbackSpeed(1.0)
{
    ui->setupUi(this);
    m_player=new VideoPlayer;
    connect(m_player,SIGNAL(SIG_getOneImage(QImage)),
            this,SLOT(slot_setImage(QImage)));

    slot_PlayerStateChanged(PlayerState::Stop);

    connect(m_player,SIGNAL(SIG_PlayerStateChanged(int)),
            this,SLOT(slot_PlayerStateChanged(int)));

    connect(m_player,SIGNAL(SIG_TotalTime(qint64)),
            this,SLOT(slot_getTotalTime(qint64)) );
    connect(m_player,SIGNAL(SIG_OpenFailed(QString)),
            this,SLOT(slot_OpenFailed(QString)) );

    connect(&m_timer,SIGNAL(timeout()),this,SLOT(slot_TimerTimeOut()));
    m_timer.setInterval(500);

    ui->slider_progress->installEventFilter(this);

    ui->cb_speed->setCurrentIndex(1);
}


VideoDialog::~VideoDialog()
{
    delete ui;
    delete m_player;
}

void VideoDialog::on_pb_start_clicked()
{
     QString path=QFileDialog::getOpenFileName(this,"选择要播放的文件" , "./",
    "视频文件 (*.flv *.rmvb *.avi *.mp4 *.MP4 *.mkv *.mov *.wmv);; 所有文件(*.*);;");

    if(path.isEmpty()) return;
    if(!QFileInfo::exists(path))
    {
        QMessageBox::warning(this, "播放失败", "文件不存在：" + path);
        return;
    }

    if(m_player->playerState()!=PlayerState::Stop){
        m_player->stop(true);
    }

    qDebug() << "open local file:" << path;
    m_player->setFileName(path);
    m_player->setPlaybackSpeed(m_playbackSpeed);

    slot_PlayerStateChanged(PlayerState::Playing);
}

void VideoDialog::playFile(const QString &filePath)
{
    if(filePath.isEmpty()) return;
    if(!filePath.startsWith("http://") && !filePath.startsWith("https://") &&
       !filePath.startsWith("rtmp://") && !QFileInfo::exists(filePath))
    {
        QMessageBox::warning(this, "播放失败", "文件或地址不存在：" + filePath);
        return;
    }

    if(m_player->playerState()!=PlayerState::Stop){
        m_player->stop(true);
    }

    qDebug() << "play file:" << filePath;
    m_player->setFileName(filePath);
    m_player->setPlaybackSpeed(m_playbackSpeed);

    slot_PlayerStateChanged(PlayerState::Playing);
}

void VideoDialog::slot_setImage(QImage img)
{
        ui->wdg_show->slot_setImage(img);
}

void VideoDialog::on_pb_resume_clicked()
{
    if(m_player->playerState() != PlayerState::Pause) return;

    m_player->play();
    ui->pb_resume->hide();
    ui->pb_pause->show();
}
void VideoDialog::on_pb_pause_clicked()
{
    if(m_player->playerState() != PlayerState::Playing) return;
    m_player->pause();

    ui->pb_resume->show();
    ui->pb_pause->hide();

}


void VideoDialog::on_pb_stop_clicked()
{
    m_player->stop(true);

}

void VideoDialog::slot_PlayerStateChanged(int state)
{
    switch( state )
    {
    case PlayerState::Stop:
        qDebug()<< "VideoPlayer::Stop";
        m_timer.stop();
        ui->slider_progress->setValue(0);
        ui->lb_totalTime->setText("00:00:00");
        ui->lb_curTime->setText("00:00:00");

        ui->pb_pause->hide();
        ui->pb_resume->show();
        this->update();
        isStop = true;
        break;
    case PlayerState::Playing:
        qDebug()<< "VideoPlayer::Playing";
        ui->pb_resume->hide();
        ui->pb_pause->show();
        m_timer.start();
        this->update();
        isStop = false;
        break;
    }
}

void VideoDialog::slot_getTotalTime(qint64 uSec)
{
    if (uSec <= 0) {
        ui->slider_progress->setRange(0, 0);
        ui->lb_totalTime->setText("00:00:00");
        return;
    }
    qint64 Sec = uSec/1000000;
    ui->slider_progress->setRange(0,Sec);
    QString hStr = QString("00%1").arg(Sec/3600);
    QString mStr = QString("00%1").arg(Sec/60);
    QString sStr = QString("00%1").arg(Sec%60);
    QString str =
            QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));
    ui->lb_totalTime->setText(str);
}

void VideoDialog::slot_OpenFailed(QString reason)
{
    m_timer.stop();
    slot_PlayerStateChanged(PlayerState::Stop);
    QMessageBox::warning(this, "播放失败", reason);
}

void VideoDialog::slot_TimerTimeOut()
{
    if (  QObject::sender() ==  &m_timer)
    {
        qint64 Sec = m_player->getCurrentTime()/1000000;
        ui->slider_progress->setValue(Sec);
        QString hStr = QString("00%1").arg(Sec/3600);
        QString mStr = QString("00%1").arg(Sec/60%60);
        QString sStr = QString("00%1").arg(Sec%60);
        QString str =
                QString("%1:%2:%3").arg(hStr.right(2)).arg(mStr.right(2)).arg(sStr.right(2));

        ui->lb_curTime->setText(str);

        if(ui->slider_progress->value() == ui->slider_progress->maximum()
                && m_player->playerState() == PlayerState::Stop)
        {
            slot_PlayerStateChanged( PlayerState::Stop );
        }else if(ui->slider_progress->value() + 1 ==
                 ui->slider_progress->maximum()
                 && m_player->playerState() == PlayerState::Stop)
        {
            slot_PlayerStateChanged( PlayerState::Stop );
        }
    }
}

bool VideoDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->slider_progress ){
        if ( event->type() == QEvent::MouseButtonPress ){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            int min=ui->slider_progress->minimum();
            int max=ui->slider_progress->maximum();
            int value = QStyle::sliderValueFromPosition(
                        min, max,mouseEvent->pos().x(), ui->slider_progress->width());

            m_timer.stop();
            ui->slider_progress->setValue(value);
            m_player->seek((qint64)value*1000000);
            m_timer.start();

            return true;
        }else {
            return false;
        }
    }
   return QDialog::eventFilter(obj,event);
}


void VideoDialog::on_pb_online_clicked()
{

}

void VideoDialog::on_pb_back_clicked()
{
    if(m_player->playerState() != PlayerState::Stop){
        m_player->stop(true);
    }
    if(isFullScreen)
    {
        on_pb_fullscreen_clicked();
    }
    emit SIG_backToOnline();
    this->hide();
}

void VideoDialog::on_cb_speed_currentIndexChanged(int index)
{
    double speeds[] = {0.5, 1.0, 1.25, 1.5, 2.0};
    m_playbackSpeed = speeds[index];
    if(m_player)
    {
        m_player->setPlaybackSpeed(m_playbackSpeed);
    }
}

void VideoDialog::on_slider_volume_valueChanged(int value)
{
    if(m_player)
    {
        m_player->setVolume(value);
    }
}

void VideoDialog::on_pb_fullscreen_clicked()
{
    if(!isFullScreen)
    {
        m_normalSize = this->size();
        m_normalPos = this->pos();
        this->showFullScreen();
        ui->pb_fullscreen->setText("退出全屏");
        isFullScreen = true;
    }
    else
    {
        this->showNormal();
        this->resize(m_normalSize);
        this->move(m_normalPos);
        ui->pb_fullscreen->setText("全屏");
        isFullScreen = false;
    }
}

void VideoDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape && isFullScreen)
    {
        on_pb_fullscreen_clicked();
    }
    QDialog::keyPressEvent(event);
}

void VideoDialog::closeEvent(QCloseEvent *event)
{
    if(m_player->playerState() != PlayerState::Stop){
        m_player->stop(true);
    }
    if(isFullScreen)
    {
        this->showNormal();
    }
    QDialog::closeEvent(event);
}
