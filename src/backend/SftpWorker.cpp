#include "SftpWorker.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>

SftpWorker::SftpWorker(QObject *parent)
    : QObject(parent) {
}

SftpWorker::~SftpWorker() {
    closeSftp();
}

bool SftpWorker::initSftp(LIBSSH2_SESSION *session) {
    if (!session) return false;
    m_session = session;
    libssh2_session_set_blocking(m_session, 1);
    m_sftp = libssh2_sftp_init(m_session);
    return m_sftp != nullptr;
}

void SftpWorker::closeSftp() {
    if (m_sftp) {
        libssh2_sftp_shutdown(m_sftp);
        m_sftp = nullptr;
    }
}

void SftpWorker::listRemoteDir(const QString &remotePath) {
    if (!m_sftp) {
        emit sftpError("SFTP session not initialized");
        return;
    }

    QByteArray pathBytes = remotePath.toUtf8();
    LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_opendir(m_sftp, pathBytes.constData());
    if (!handle) {
        emit sftpError("Failed to open remote directory: " + remotePath);
        return;
    }

    QList<RemoteFileInfo> results;
    char mem[512];
    char longentry[1024];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (libssh2_sftp_readdir_ex(handle, mem, sizeof(mem), longentry, sizeof(longentry), &attrs) > 0) {
        QString name = QString::fromUtf8(mem);
        if (name == "." || name == "..") continue;

        RemoteFileInfo info;
        info.name = name;
        info.size = attrs.filesize;
        info.isDir = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
        info.permissions = attrs.permissions;
        info.lastModified = QDateTime::fromSecsSinceEpoch(attrs.mtime);
        results.append(info);
    }

    libssh2_sftp_closedir(handle);
    emit dirListed(remotePath, results);
}

void SftpWorker::uploadFile(const QString &localFilePath, const QString &targetRemotePath) {
    if (!m_sftp) return;

    QFile localFile(localFilePath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        emit transferFinished(localFilePath, false, "Cannot open local file", 0);
        return;
    }

    QFileInfo fi(localFilePath);
    QString remotePath = targetRemotePath;
    if (remotePath.endsWith("/")) {
        remotePath += fi.fileName();
    }

    LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_open(
        m_sftp,
        remotePath.toUtf8().constData(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH
    );

    if (!handle) {
        emit transferFinished(fi.fileName(), false, "Failed to create remote file", 0);
        return;
    }

    qint64 total = localFile.size();
    qint64 sent = 0;
    char buffer[16384];

    while (!localFile.atEnd()) {
        qint64 readBytes = localFile.read(buffer, sizeof(buffer));
        if (readBytes <= 0) break;

        char *ptr = buffer;
        qint64 remaining = readBytes;
        while (remaining > 0) {
            ssize_t rc = libssh2_sftp_write(handle, ptr, remaining);
            if (rc < 0) {
                libssh2_sftp_close(handle);
                emit transferFinished(fi.fileName(), false, "Write error during upload", sent);
                return;
            }
            ptr += rc;
            remaining -= rc;
            sent += rc;
        }
        emit transferProgress(fi.fileName(), sent, total);
    }

    libssh2_sftp_close(handle);
    emit transferFinished(fi.fileName(), true, "Upload complete", total);
}

void SftpWorker::downloadFile(const QString &remoteFilePath, const QString &targetLocalPath) {
    if (!m_sftp) return;

    QFileInfo rfi(remoteFilePath);
    QString localPath = targetLocalPath;
    if (QFileInfo(localPath).isDir() || localPath.endsWith('/') || localPath.endsWith('\\')) {
        localPath = QDir(localPath).filePath(rfi.fileName());
    }

    LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_open(
        m_sftp,
        remoteFilePath.toUtf8().constData(),
        LIBSSH2_FXF_READ,
        0
    );

    if (!handle) {
        emit transferFinished(rfi.fileName(), false, "Failed to open remote file", 0);
        return;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    libssh2_sftp_fstat(handle, &attrs);
    qint64 total = attrs.filesize;

    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        libssh2_sftp_close(handle);
        emit transferFinished(rfi.fileName(), false, "Cannot create local file", 0);
        return;
    }

    qint64 received = 0;
    char buffer[16384];

    while (true) {
        ssize_t rc = libssh2_sftp_read(handle, buffer, sizeof(buffer));
        if (rc < 0) {
            libssh2_sftp_close(handle);
            emit transferFinished(rfi.fileName(), false, "Read error during download", received);
            return;
        }
        if (rc == 0) break; // EOF

        localFile.write(buffer, rc);
        received += rc;
        emit transferProgress(rfi.fileName(), received, total);
    }

    libssh2_sftp_close(handle);
    localFile.close();
    emit transferFinished(rfi.fileName(), true, "Download complete", total);
}

void SftpWorker::uploadDirectory(const QString &localDirPath, const QString &targetRemoteDirPath) {
    if (!m_sftp) return;

    QDir localDir(localDirPath);
    if (!localDir.exists()) return;

    QString dirName = localDir.dirName();
    QString remoteTarget = targetRemoteDirPath;
    if (!remoteTarget.endsWith('/')) remoteTarget += '/';
    remoteTarget += dirName;

    // Create remote folder (ignore error if already exists)
    libssh2_sftp_mkdir(m_sftp, remoteTarget.toUtf8().constData(),
                       LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);

    // Recursively process entries
    QFileInfoList entries = localDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            uploadDirectory(entry.absoluteFilePath(), remoteTarget);
        } else {
            uploadFile(entry.absoluteFilePath(), remoteTarget);
        }
    }
}

void SftpWorker::downloadDirectory(const QString &remoteDirPath, const QString &targetLocalDirPath) {
    if (!m_sftp) return;

    // Extract directory name
    QString cleanRemote = remoteDirPath;
    while (cleanRemote.endsWith('/') && cleanRemote.length() > 1) {
        cleanRemote.chop(1);
    }
    QString dirName = cleanRemote.mid(cleanRemote.lastIndexOf('/') + 1);
    if (dirName.isEmpty()) dirName = "remote_dir";

    QString localTarget = QDir(targetLocalDirPath).filePath(dirName);
    QDir().mkpath(localTarget);

    // List remote directory
    QByteArray pathBytes = cleanRemote.toUtf8();
    LIBSSH2_SFTP_HANDLE *handle = libssh2_sftp_opendir(m_sftp, pathBytes.constData());
    if (!handle) return;

    char mem[512];
    char longentry[1024];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    struct Entry {
        QString name;
        bool isDir;
    };
    QList<Entry> subEntries;

    while (libssh2_sftp_readdir_ex(handle, mem, sizeof(mem), longentry, sizeof(longentry), &attrs) > 0) {
        QString name = QString::fromUtf8(mem);
        if (name == "." || name == "..") continue;
        subEntries.append({name, (bool)LIBSSH2_SFTP_S_ISDIR(attrs.permissions)});
    }
    libssh2_sftp_closedir(handle);

    for (const auto &sub : subEntries) {
        QString subRemote = cleanRemote + "/" + sub.name;
        if (sub.isDir) {
            downloadDirectory(subRemote, localTarget);
        } else {
            downloadFile(subRemote, localTarget);
        }
    }
}

bool SftpWorker::createRemoteDir(const QString &dirPath) {
    if (!m_sftp) return false;
    int rc = libssh2_sftp_mkdir(m_sftp, dirPath.toUtf8().constData(),
                                LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
    return rc == 0;
}

bool SftpWorker::deleteRemoteFile(const QString &filePath) {
    if (!m_sftp) return false;
    int rc = libssh2_sftp_unlink(m_sftp, filePath.toUtf8().constData());
    return rc == 0;
}

bool SftpWorker::deleteRemoteDir(const QString &dirPath) {
    if (!m_sftp) return false;
    int rc = libssh2_sftp_rmdir(m_sftp, dirPath.toUtf8().constData());
    return rc == 0;
}

bool SftpWorker::renameRemote(const QString &oldPath, const QString &newPath) {
    if (!m_sftp) return false;
    int rc = libssh2_sftp_rename(m_sftp, oldPath.toUtf8().constData(), newPath.toUtf8().constData());
    return rc == 0;
}
