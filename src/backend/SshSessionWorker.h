#pragma once

#include <QThread>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <libssh2.h>
#include <libssh2_sftp.h>

struct SshConnectionParams {
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString privateKeyPath;
    QString authMethod = "Password";
    bool keepAlive = true;
};

Q_DECLARE_METATYPE(SshConnectionParams)

class SshSessionWorker : public QThread {
    Q_OBJECT

public:
    explicit SshSessionWorker(QObject *parent = nullptr);
    ~SshSessionWorker() override;

    bool isConnected() const { return m_connected; }
    LIBSSH2_SESSION* rawSession() const { return m_session; }
    QString currentPassword() const { return m_params.password; }

    // Start connection in background thread
    void startConnection(const QString &host, int port, const QString &username,
                         const QString &password, const QString &authMethod,
                         bool keepAlive = true);

    // Thread-safe methods callable from GUI thread directly
    void sendData(const QByteArray &data);
    void resizePty(int cols, int rows);
    void disconnectFromServer();

signals:
    void statusMessage(const QString &msg);
    void connected(const QString &banner);
    void disconnected();
    void connectionError(const QString &error);
    void dataReceived(const QByteArray &data);

protected:
    void run() override;

private:
    void runIoLoop();
    bool tryPublicKeyAuth(const QByteArray &userBytes, const QString &keyPath);

    SshConnectionParams m_params;
    int m_socket = -1;
    LIBSSH2_SESSION *m_session = nullptr;
    LIBSSH2_CHANNEL *m_channel = nullptr;
    volatile bool m_connected = false;
    volatile bool m_stopRequested = false;

    mutable QMutex m_sshMutex;
    int m_wakePipe[2]{-1, -1};
    QMutex m_writeQueueMutex;
    QByteArray m_outgoingQueue;
};
