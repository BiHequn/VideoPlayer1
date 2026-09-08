#include "logindialog.h"
#include "onlinedialog.h"
#include "videodialog.h"
#include "network/networkclient.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <iostream>
using namespace std;

#undef main

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("VideoPlayer1");
    a.setOrganizationName("VideoPlayer");

    cout << "Hello FFmpeg!" << endl;
    av_register_all();
    unsigned version = avcodec_version();
    cout << "version is:" << version << endl;

    NetworkClient netClient;

    QSettings settings;
    LoginDialog loginDlg(&netClient);
    OnlineDialog onlineDlg(&netClient);
    VideoDialog videoDlg;

    loginDlg.show();

    QObject::connect(&loginDlg, &LoginDialog::SIG_loginSuccess,
                     [&](const QString &username){
        onlineDlg.setLoginUser(username);
        onlineDlg.show();
        loginDlg.hide();
    });

    QObject::connect(&onlineDlg, &OnlineDialog::SIG_openVideoPlayer,
                     [&](const QString &path){
        videoDlg.playFile(path);
        videoDlg.show();
        onlineDlg.hide();
    });

    QObject::connect(&videoDlg, &VideoDialog::SIG_backToOnline,
                     [&](){
        onlineDlg.show();
        videoDlg.hide();
    });

    QObject::connect(&onlineDlg, &OnlineDialog::SIG_logout,
                     [&](){
        settings.remove("login/username");
        settings.remove("login/account");
        settings.remove("login/password");
        settings.remove("login/rememberPassword");
        settings.remove("login/accessToken");
        settings.remove("login/refreshToken");
        settings.sync();

        netClient.setAccessToken("");
        netClient.setRefreshToken("");
        netClient.setUserId(0);
        netClient.disconnectFromServer();

        onlineDlg.hide();
        videoDlg.hide();
        loginDlg.clearFields();
        loginDlg.show();
    });

    QObject::connect(&loginDlg, &QDialog::rejected, [&](){
        onlineDlg.close();
        videoDlg.close();
    });

    return a.exec();
}
