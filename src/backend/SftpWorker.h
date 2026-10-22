#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <libssh2.h>
#include <libssh2_sftp.h>

struct RemoteFileInfo {
    QString name;
    qint64 size = 0;
    bool isDir = false;
    QDateTime lastModified;
    unsigned long permissions = 0;
};
Q_DECLARE_METATYPE(RemoteFileInfo)
Q_DECLARE_METATYPE(QList<RemoteFileInfo>)

class SftpWorker : public QObject {
    Q_OBJECT

public:
    explicit SftpWorker(QObject *parent = nullptr);
    ~SftpWorker();

    bool initSftp(LIBSSH2_SESSION *session);
    void closeSftp();

public slots:
    void listRemoteDir(const QString &remotePath);
    void uploadFile(const QString &localFilePath, const QString &targetRemotePath);
    void downloadFile(const QString &remoteFilePath, const QString &targetLocalPath);
    void uploadDirectory(const QString &localDirPath, const QString &targetRemoteDirPath);
    void downloadDirectory(const QString &remoteDirPath, const QString &targetLocalDirPath);
    bool createRemoteDir(const QString &dirPath);
    bool deleteRemoteFile(const QString &filePath);
    bool deleteRemoteDir(const QString &dirPath);
    bool renameRemote(const QString &oldPath, const QString &newPath);

signals:
    void dirListed(const QString &path, const QList<RemoteFileInfo> &files);
    void transferProgress(const QString &filename, qint64 bytesTransferred, qint64 totalBytes);
    void transferFinished(const QString &filename, bool success, const QString &message, qint64 totalBytes);
    void sftpError(const QString &error);

private:
    LIBSSH2_SESSION *m_session = nullptr;
    LIBSSH2_SFTP *m_sftp = nullptr;
};
