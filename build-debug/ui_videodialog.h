/********************************************************************************
** Form generated from reading UI file 'videodialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIDEODIALOG_H
#define UI_VIDEODIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSlider>
#include "myopenglwidget.h"

QT_BEGIN_NAMESPACE

class Ui_VideoDialog
{
public:
    QPushButton *pb_start;
    QPushButton *pb_resume;
    QPushButton *pb_pause;
    QPushButton *pb_stop;
    QScrollBar *slider_progress;
    QLabel *lb_curTime;
    QLabel *lb_totalTime;
    MyOpenGLWidget *wdg_show;
    QPushButton *pb_back;
    QComboBox *cb_speed;
    QPushButton *pb_fullscreen;
    QSlider *slider_volume;
    QLabel *lb_volume;

    void setupUi(QDialog *VideoDialog)
    {
        if (VideoDialog->objectName().isEmpty())
            VideoDialog->setObjectName(QString::fromUtf8("VideoDialog"));
        VideoDialog->resize(800, 520);
        pb_start = new QPushButton(VideoDialog);
        pb_start->setObjectName(QString::fromUtf8("pb_start"));
        pb_start->setGeometry(QRect(10, 470, 81, 23));
        pb_resume = new QPushButton(VideoDialog);
        pb_resume->setObjectName(QString::fromUtf8("pb_resume"));
        pb_resume->setGeometry(QRect(90, 470, 81, 23));
        pb_pause = new QPushButton(VideoDialog);
        pb_pause->setObjectName(QString::fromUtf8("pb_pause"));
        pb_pause->setGeometry(QRect(170, 470, 81, 23));
        pb_stop = new QPushButton(VideoDialog);
        pb_stop->setObjectName(QString::fromUtf8("pb_stop"));
        pb_stop->setGeometry(QRect(250, 470, 81, 23));
        slider_progress = new QScrollBar(VideoDialog);
        slider_progress->setObjectName(QString::fromUtf8("slider_progress"));
        slider_progress->setGeometry(QRect(90, 450, 501, 20));
        slider_progress->setOrientation(Qt::Horizontal);
        lb_curTime = new QLabel(VideoDialog);
        lb_curTime->setObjectName(QString::fromUtf8("lb_curTime"));
        lb_curTime->setGeometry(QRect(10, 450, 91, 16));
        QFont font;
        font.setFamily(QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221"));
        font.setPointSize(11);
        lb_curTime->setFont(font);
        lb_totalTime = new QLabel(VideoDialog);
        lb_totalTime->setObjectName(QString::fromUtf8("lb_totalTime"));
        lb_totalTime->setGeometry(QRect(590, 450, 91, 20));
        lb_totalTime->setFont(font);
        wdg_show = new MyOpenGLWidget(VideoDialog);
        wdg_show->setObjectName(QString::fromUtf8("wdg_show"));
        wdg_show->setGeometry(QRect(20, 10, 761, 441));
        pb_back = new QPushButton(VideoDialog);
        pb_back->setObjectName(QString::fromUtf8("pb_back"));
        pb_back->setGeometry(QRect(710, 470, 81, 23));
        cb_speed = new QComboBox(VideoDialog);
        cb_speed->addItem(QString());
        cb_speed->addItem(QString());
        cb_speed->addItem(QString());
        cb_speed->addItem(QString());
        cb_speed->addItem(QString());
        cb_speed->setObjectName(QString::fromUtf8("cb_speed"));
        cb_speed->setGeometry(QRect(330, 470, 81, 23));
        cb_speed->setEditable(false);
        pb_fullscreen = new QPushButton(VideoDialog);
        pb_fullscreen->setObjectName(QString::fromUtf8("pb_fullscreen"));
        pb_fullscreen->setGeometry(QRect(630, 470, 81, 23));
        slider_volume = new QSlider(VideoDialog);
        slider_volume->setObjectName(QString::fromUtf8("slider_volume"));
        slider_volume->setGeometry(QRect(480, 470, 80, 22));
        slider_volume->setOrientation(Qt::Horizontal);
        slider_volume->setMinimum(0);
        slider_volume->setMaximum(100);
        slider_volume->setValue(80);
        lb_volume = new QLabel(VideoDialog);
        lb_volume->setObjectName(QString::fromUtf8("lb_volume"));
        lb_volume->setGeometry(QRect(415, 470, 61, 20));

        retranslateUi(VideoDialog);

        QMetaObject::connectSlotsByName(VideoDialog);
    } // setupUi

    void retranslateUi(QDialog *VideoDialog)
    {
        VideoDialog->setWindowTitle(QApplication::translate("VideoDialog", "\350\247\206\351\242\221\346\222\255\346\224\276\345\231\250", nullptr));
        pb_start->setText(QApplication::translate("VideoDialog", "\346\211\223\345\274\200", nullptr));
        pb_resume->setText(QApplication::translate("VideoDialog", "\346\222\255\346\224\276", nullptr));
        pb_pause->setText(QApplication::translate("VideoDialog", "\346\232\202\345\201\234", nullptr));
        pb_stop->setText(QApplication::translate("VideoDialog", "\345\201\234\346\255\242", nullptr));
        lb_curTime->setText(QApplication::translate("VideoDialog", "00:00:00", nullptr));
        lb_totalTime->setText(QApplication::translate("VideoDialog", "00:00:00", nullptr));
        pb_back->setText(QApplication::translate("VideoDialog", "\350\277\224\345\233\236\344\270\273\351\241\265", nullptr));
        cb_speed->setItemText(0, QApplication::translate("VideoDialog", "0.5x", nullptr));
        cb_speed->setItemText(1, QApplication::translate("VideoDialog", "1x", nullptr));
        cb_speed->setItemText(2, QApplication::translate("VideoDialog", "1.25x", nullptr));
        cb_speed->setItemText(3, QApplication::translate("VideoDialog", "1.5x", nullptr));
        cb_speed->setItemText(4, QApplication::translate("VideoDialog", "2x", nullptr));

        pb_fullscreen->setText(QApplication::translate("VideoDialog", "\345\205\250\345\261\217", nullptr));
        lb_volume->setText(QApplication::translate("VideoDialog", "\351\237\263\351\207\217:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class VideoDialog: public Ui_VideoDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIDEODIALOG_H
