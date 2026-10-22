#include "backend/SftpWorker.h"
#include "backend/SshSessionWorker.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    // Ensure on macOS physical Control is Qt::ControlModifier and Command is Qt::MetaModifier
    QApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);

    qRegisterMetaType<SshConnectionParams>("SshConnectionParams");
    qRegisterMetaType<RemoteFileInfo>("RemoteFileInfo");
    qRegisterMetaType<QList<RemoteFileInfo>>("QList<RemoteFileInfo>");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("RetroSSH");
    app.setApplicationDisplayName("RetroSSH Client");
    app.setWindowIcon(QIcon(":/icons/app_icon.svg"));

    // Apply Fusion / Windows style base
    if (QStyleFactory::keys().contains("Windows")) {
        app.setStyle(QStyleFactory::create("Windows"));
    } else if (QStyleFactory::keys().contains("Fusion")) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    // Load retro Win32 QSS stylesheet
    QFile qssFile(":/classic_win32.qss");
    if (qssFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    }

    MainWindow w;
    w.show();

    return app.exec();
}
