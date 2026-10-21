#include "SshSessionWorker.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/tcp.h>

static SshSessionWorker* s_activeWorker = nullptr;

static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract) {
    (void)name; (void)name_len;
    (void)instruction; (void)instruction_len;
    (void)prompts;
    (void)abstract;

    if (num_prompts == 1 && responses && s_activeWorker) {
        QByteArray pass = s_activeWorker->currentPassword().toUtf8();
        responses[0].text = strdup(pass.constData());
        responses[0].length = (unsigned int)pass.length();
    }
}

SshSessionWorker::SshSessionWorker(QObject *parent)
    : QThread(parent) {
    libssh2_init(0);
    if (::pipe(m_wakePipe) == 0) {
        fcntl(m_wakePipe[0], F_SETFL, O_NONBLOCK);
        fcntl(m_wakePipe[1], F_SETFL, O_NONBLOCK);
    }
}

SshSessionWorker::~SshSessionWorker() {
    disconnectFromServer();
    wait(1000);
    if (m_wakePipe[0] >= 0) { ::close(m_wakePipe[0]); m_wakePipe[0] = -1; }
    if (m_wakePipe[1] >= 0) { ::close(m_wakePipe[1]); m_wakePipe[1] = -1; }
    libssh2_exit();
}

void SshSessionWorker::startConnection(const QString &host, int port, const QString &username,
                                       const QString &password, const QString &authMethod) {
    if (isRunning()) {
        disconnectFromServer();
        wait(500);
    }

    m_params.host = host;
    m_params.port = port;
    m_params.username = username;
    m_params.password = password;
    m_params.authMethod = authMethod;

    m_stopRequested = false;
    start(); // Launch run() in background thread
}

bool SshSessionWorker::tryPublicKeyAuth(const QByteArray &userBytes, const QString &keyPath) {
    if (!m_session) return false;

    QString privKey = keyPath;
    if (privKey.isEmpty()) {
        QString home = QDir::homePath();
        QStringList candidates = {
            home + "/.ssh/id_rsa",
            home + "/.ssh/id_ed25519",
            home + "/.ssh/id_ecdsa"
        };
        for (const QString &c : candidates) {
            if (QFile::exists(c)) {
                privKey = c;
                break;
            }
        }
    }

    if (privKey.isEmpty() || !QFile::exists(privKey)) {
        return false;
    }

    QString pubKey = privKey + ".pub";
    QByteArray privBytes = privKey.toUtf8();
    QByteArray pubBytes = QFile::exists(pubKey) ? pubKey.toUtf8() : QByteArray();
    const char *pubPtr = pubBytes.isEmpty() ? nullptr : pubBytes.constData();

    emit statusMessage("Authenticating with key: " + QFileInfo(privKey).fileName() + "...");

    QByteArray passBytes = m_params.password.toUtf8();
    const char *passPtr = passBytes.isEmpty() ? nullptr : passBytes.constData();

    int rc = 0;
    {
        QMutexLocker locker(&m_sshMutex);
        rc = libssh2_userauth_publickey_fromfile(
            m_session,
            userBytes.constData(),
            pubPtr,
            privBytes.constData(),
            passPtr
        );
    }

    return rc == 0;
}

void SshSessionWorker::run() {
    s_activeWorker = this;

    emit statusMessage(QString("Connecting to %1:%2...").arg(m_params.host).arg(m_params.port));

    // Resolve host
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    QString portStr = QString::number(m_params.port);
    if (getaddrinfo(m_params.host.toUtf8().constData(), portStr.toUtf8().constData(), &hints, &res) != 0 || !res) {
        emit connectionError("Failed to resolve hostname: " + m_params.host);
        return;
    }

    m_socket = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_socket < 0) {
        freeaddrinfo(res);
        emit connectionError("Failed to create socket.");
        return;
    }

    // Disable Nagle's algorithm (TCP_NODELAY) for ultra-low interactive latency
    int one = 1;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));

    // Set connection timeout (5 seconds)
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    emit statusMessage(QString("Connecting TCP socket to %1...").arg(m_params.host));

    if (::connect(m_socket, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        ::close(m_socket);
        m_socket = -1;
        emit connectionError("Connection refused or timed out connecting to " + m_params.host);
        return;
    }
    freeaddrinfo(res);

    emit statusMessage("Connected. Handshaking SSH2 session...");

    {
        QMutexLocker locker(&m_sshMutex);
        m_session = libssh2_session_init();
    }

    if (!m_session) {
        ::close(m_socket);
        m_socket = -1;
        emit connectionError("Failed to initialize libssh2 session.");
        return;
    }

    int rc = 0;
    {
        QMutexLocker locker(&m_sshMutex);
        rc = libssh2_session_handshake(m_session, m_socket);
    }

    if (rc != 0) {
        char *errmsg = nullptr;
        {
            QMutexLocker locker(&m_sshMutex);
            libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        }
        QString err = errmsg ? QString::fromUtf8(errmsg) : "Unknown handshake error";
        disconnectFromServer();
        emit connectionError("SSH handshake failed: " + err);
        return;
    }

    emit statusMessage("Authenticating as " + m_params.username + "...");

    bool authSuccess = false;
    QByteArray userBytes = m_params.username.toUtf8();
    QByteArray passBytes = m_params.password.toUtf8();

    if (m_params.authMethod == "Public Key") {
        authSuccess = tryPublicKeyAuth(userBytes, m_params.privateKeyPath);
    } else if (m_params.authMethod == "Keyboard Interactive") {
        QMutexLocker locker(&m_sshMutex);
        authSuccess = (libssh2_userauth_keyboard_interactive(m_session, userBytes.constData(), &kbd_callback) == 0);
    } else if (m_params.authMethod == "Password") {
        {
            QMutexLocker locker(&m_sshMutex);
            if (libssh2_userauth_password(m_session, userBytes.constData(), passBytes.constData()) == 0) {
                authSuccess = true;
            }
        }
        if (!authSuccess) {
            QMutexLocker locker(&m_sshMutex);
            authSuccess = (libssh2_userauth_keyboard_interactive(m_session, userBytes.constData(), &kbd_callback) == 0);
        }
    } else {
        // Auto / Profile Settings: Public key first, then password
        authSuccess = tryPublicKeyAuth(userBytes, m_params.privateKeyPath);
        if (!authSuccess && !passBytes.isEmpty()) {
            QMutexLocker locker(&m_sshMutex);
            if (libssh2_userauth_password(m_session, userBytes.constData(), passBytes.constData()) == 0) {
                authSuccess = true;
            } else if (libssh2_userauth_keyboard_interactive(m_session, userBytes.constData(), &kbd_callback) == 0) {
                authSuccess = true;
            }
        }
    }

    if (!authSuccess) {
        char *errmsg = nullptr;
        {
            QMutexLocker locker(&m_sshMutex);
            libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        }
        QString err = errmsg ? QString::fromUtf8(errmsg) : "Authentication failed. Check credentials or key.";
        disconnectFromServer();
        emit connectionError("Authentication failed: " + err);
        return;
    }

    emit statusMessage("Opening terminal channel...");

    {
        QMutexLocker locker(&m_sshMutex);
        m_channel = libssh2_channel_open_session(m_session);
    }

    if (!m_channel) {
        disconnectFromServer();
        emit connectionError("Failed to open channel session.");
        return;
    }

    // Request PTY
    int ptyRc = 0;
    {
        QMutexLocker locker(&m_sshMutex);
        ptyRc = libssh2_channel_request_pty(m_channel, "xterm");
    }
    if (ptyRc != 0) {
        disconnectFromServer();
        emit connectionError("Failed to allocate PTY.");
        return;
    }

    // Start shell
    int shellRc = 0;
    {
        QMutexLocker locker(&m_sshMutex);
        shellRc = libssh2_channel_shell(m_channel);
    }
    if (shellRc != 0) {
        disconnectFromServer();
        emit connectionError("Failed to start remote shell.");
        return;
    }

    m_connected = true;
    const char *banner = nullptr;
    {
        QMutexLocker locker(&m_sshMutex);
        banner = libssh2_session_banner_get(m_session);
    }
    QString bannerStr = banner ? QString::fromUtf8(banner) : "SSH-2.0-RetroSSH";
    emit connected(bannerStr);
    emit statusMessage(QString("Connected to %1").arg(m_params.host));

    // Send initial newline so shell prints prompt immediately
    sendData("\n");

    // Start background I/O loop
    runIoLoop();
}

void SshSessionWorker::sendData(const QByteArray &data) {
    if (!m_connected) return;

    {
        QMutexLocker locker(&m_writeQueueMutex);
        m_outgoingQueue.append(data);
    }

    // Instantly wake the I/O thread poll via pipe so it writes immediately with zero delay
    if (m_wakePipe[1] >= 0) {
        char sig = 'w';
        (void)::write(m_wakePipe[1], &sig, 1);
    }
}

void SshSessionWorker::resizePty(int cols, int rows) {
    if (!m_connected || !m_channel) return;
    QMutexLocker locker(&m_sshMutex);
    libssh2_channel_request_pty_size(m_channel, cols, rows);
}

void SshSessionWorker::disconnectFromServer() {
    m_stopRequested = true;
    m_connected = false;

    // Wake I/O thread from poll
    if (m_wakePipe[1] >= 0) {
        char sig = 'q';
        (void)::write(m_wakePipe[1], &sig, 1);
    }

    QMutexLocker locker(&m_sshMutex);
    if (m_channel) {
        libssh2_channel_send_eof(m_channel);
        libssh2_channel_close(m_channel);
        libssh2_channel_free(m_channel);
        m_channel = nullptr;
    }

    if (m_session) {
        libssh2_session_disconnect(m_session, "Normal Shutdown");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }

    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }

    {
        QMutexLocker qLocker(&m_writeQueueMutex);
        m_outgoingQueue.clear();
    }

    emit disconnected();
    emit statusMessage("Connect quickly without profiles");
}

void SshSessionWorker::runIoLoop() {
    {
        QMutexLocker locker(&m_sshMutex);
        if (m_session) libssh2_session_set_blocking(m_session, 0);
    }

    char buffer[16384];
    struct pollfd pfds[2];
    pfds[0].fd = m_socket;
    pfds[0].events = POLLIN;
    pfds[1].fd = m_wakePipe[0];
    pfds[1].events = POLLIN;

    while (!m_stopRequested && m_connected && m_channel) {
        // 1. Flush any pending outbound data queued by sendData()
        QByteArray chunkToSend;
        {
            QMutexLocker qLocker(&m_writeQueueMutex);
            if (!m_outgoingQueue.isEmpty()) {
                chunkToSend = m_outgoingQueue;
                m_outgoingQueue.clear();
            }
        }

        if (!chunkToSend.isEmpty()) {
            QMutexLocker locker(&m_sshMutex);
            const char *buf = chunkToSend.constData();
            size_t len = chunkToSend.size();
            size_t sent = 0;
            while (sent < len && m_connected && m_channel) {
                ssize_t rc = libssh2_channel_write(m_channel, buf + sent, len - sent);
                if (rc > 0) {
                    sent += rc;
                } else if (rc == LIBSSH2_ERROR_EAGAIN) {
                    // Channel buffer full, re-enqueue remaining data
                    QMutexLocker qLocker(&m_writeQueueMutex);
                    m_outgoingQueue.prepend(chunkToSend.mid(sent));
                    break;
                } else {
                    break;
                }
            }
        }

        // 2. Poll socket for incoming packets AND wakePipe for instant input notifications
        int pollRes = poll(pfds, 2, 50);
        if (pollRes > 0) {
            // Drain wake pipe if triggered
            if (pfds[1].revents & POLLIN) {
                char dummy[64];
                while (::read(m_wakePipe[0], dummy, sizeof(dummy)) > 0) {}
            }

            // Read SSH channel if data arrived
            if (pfds[0].revents & POLLIN) {
                while (true) {
                    ssize_t bytes = 0;
                    {
                        QMutexLocker locker(&m_sshMutex);
                        if (!m_channel || !m_connected) break;
                        bytes = libssh2_channel_read(m_channel, buffer, sizeof(buffer));
                    }

                    if (bytes > 0) {
                        emit dataReceived(QByteArray(buffer, bytes));
                    } else if (bytes == LIBSSH2_ERROR_EAGAIN) {
                        break;
                    } else {
                        bool isEof = false;
                        {
                            QMutexLocker locker(&m_sshMutex);
                            if (m_channel) isEof = (libssh2_channel_eof(m_channel) != 0);
                        }
                        if (isEof) {
                            m_connected = false;
                        }
                        break;
                    }
                }
            }
        } else if (pollRes < 0) {
            if (errno == EINTR) continue;
            break;
        }
    }

    if (m_connected) {
        disconnectFromServer();
    }
}
