#pragma once

#include "backend/SftpWorker.h"
#include "backend/SshSessionWorker.h"
#include <QComboBox>
#include <QFileSystemModel>
#include <QLabel>
#include <QMainWindow>
#include <QMutex>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QThread>
#include <QTreeView>
#include <QTreeWidget>

class SftpWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SftpWindow(const SshConnectionParams &params, const QString &initialRemotePath = "/root",
                        QWidget *parent = nullptr);
    ~SftpWindow();

    void navigateRemote(const QString &path);
    void navigateLocal(const QString &path);

private slots:
    void onRemoteDirListed(const QString &path, const QList<RemoteFileInfo> &files);
    void onLocalItemDoubleClicked(const QModelIndex &index);
    void onRemoteItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onLocalContextMenu(const QPoint &pos);
    void onRemoteContextMenu(const QPoint &pos);
    void onUploadClicked();
    void onDownloadClicked();
    void onTransferProgress(const QString &filename, qint64 bytes, qint64 total);
    void onTransferFinished(const QString &filename, bool success, const QString &message, qint64 totalBytes);

private:
    void setupMenus();
    void setupToolBars();
    void setupUi();
    QWidget *createPaneHeader(bool isLocal);

    int m_socket = -1;
    LIBSSH2_SESSION *m_session = nullptr;

    QThread *m_workerThread = nullptr;
    SftpWorker *m_worker = nullptr;
    QString m_currentRemotePath = "/root";
    QString m_currentLocalPath;

    QComboBox *m_localCombo = nullptr;
    QComboBox *m_remoteCombo = nullptr;

    QTreeView *m_localTree = nullptr;
    QFileSystemModel *m_localModel = nullptr;

    QTreeWidget *m_remoteTree = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    QTextEdit *m_transferLog = nullptr;
    QTableWidget *m_queueTable = nullptr;

    void addQueueItem(const QString &local, const QString &remote, const QString &op);
    void updateQueueItem(const QString &file, const QString &status);
    void appendLogText(const QString &text);

    QLabel *m_statusLabel = nullptr;
    QLabel *m_iconLabel = nullptr;
};
