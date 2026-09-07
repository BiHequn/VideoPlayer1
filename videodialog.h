#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include "videoplayer.h"
#include <QTimer>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class VideoDialog; }
QT_END_NAMESPACE

class VideoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoDialog(QWidget *parent = nullptr);
    ~VideoDialog();

signals:
    void SIG_backToOnline();

private slots:
    void on_pb_start_clicked();
    void slot_setImage(QImage img);
    void on_pb_resume_clicked();
    void on_pb_pause_clicked();
    void on_pb_stop_clicked();
    void slot_PlayerStateChanged(int state);
    void slot_getTotalTime(qint64 uSec);
    void slot_OpenFailed(QString reason);
    void slot_TimerTimeOut();
    bool eventFilter(QObject * obj, QEvent * event);
    void on_pb_online_clicked();
    void on_pb_back_clicked();
    void on_cb_speed_currentIndexChanged(int index);
    void on_slider_volume_valueChanged(int value);
    void on_pb_fullscreen_clicked();

public:
    void playFile(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::VideoDialog *ui;
    VideoPlayer * m_player;
    QTimer m_timer;
    bool isStop;
    bool isFullScreen;
    double m_playbackSpeed;
    QSize m_normalSize;
    QPoint m_normalPos;

    void updatePlaybackSpeed();
};
#endif // VIDEODIALOG_H
