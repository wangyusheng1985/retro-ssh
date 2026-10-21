#include "SftpWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QInputDialog>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

SftpWindow::SftpWindow(const SshConnectionParams &params, const QString &initialRemotePath, QWidget *parent)
    : QMainWindow(parent), m_currentRemotePath(initialRemotePath) {
    setWindowTitle(QString("2: %1 - default - RetroSSH File Transfer").arg(params.host));
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    resize(860, 620);

    m_currentLocalPath = QDir::homePath();

    // Establish dedicated independent SSH + SFTP connection
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    QString portStr = QString::number(params.port);

    if (getaddrinfo(params.host.toUtf8().constData(), portStr.toUtf8().constData(), &hints, &res) == 0 && res) {
        m_socket = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (m_socket >= 0) {
            struct timeval tv{3, 0};
            setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

            if (::connect(m_socket, res->ai_addr, res->ai_addrlen) == 0) {
                m_session = libssh2_session_init();
                if (m_session && libssh2_session_handshake(m_session, m_socket) == 0) {
                    QByteArray userBytes = params.username.toUtf8();
                    QByteArray passBytes = params.password.toUtf8();

                    // Authenticate
                    bool authOk = false;
                    QString privKey = params.privateKeyPath;
                    if (privKey.isEmpty()) {
                        QString home = QDir::homePath();
                        for (const QString &c : {home + "/.ssh/id_rsa", home + "/.ssh/id_ed25519", home + "/.ssh/id_ecdsa"}) {
                            if (QFile::exists(c)) { privKey = c; break; }
                        }
                    }

                    if (!privKey.isEmpty() && QFile::exists(privKey)) {
                        QString pubKey = privKey + ".pub";
                        authOk = (libssh2_userauth_publickey_fromfile(
                            m_session,
                            userBytes.constData(),
                            QFile::exists(pubKey) ? pubKey.toUtf8().constData() : nullptr,
                            privKey.toUtf8().constData(),
                            passBytes.isEmpty() ? nullptr : passBytes.constData()
                        ) == 0);
                    }

                    if (!authOk && !passBytes.isEmpty()) {
                        authOk = (libssh2_userauth_password(m_session, userBytes.constData(), passBytes.constData()) == 0);
                    }

                    if (authOk) {
                        m_worker = new SftpWorker();
                        m_worker->initSftp(m_session);

                        m_workerThread = new QThread(this);
                        m_worker->moveToThread(m_workerThread);
                        m_workerThread->start();
                    }
                }
            }
        }
        freeaddrinfo(res);
    }

    if (m_worker) {
        connect(m_worker, &SftpWorker::dirListed, this, &SftpWindow::onRemoteDirListed);
        connect(m_worker, &SftpWorker::transferProgress, this, &SftpWindow::onTransferProgress);
        connect(m_worker, &SftpWorker::transferFinished, this, &SftpWindow::onTransferFinished);
        connect(m_worker, &SftpWorker::sftpError, this, [this](const QString &err) {
            m_statusLabel->setText("Error: " + err);
        });
    }

    setupMenus();
    setupToolBars();
    setupUi();

    if (m_worker) {
        navigateRemote(m_currentRemotePath);
    } else {
        m_statusLabel->setText("Could not open separate SFTP session.");
    }
}

SftpWindow::~SftpWindow() {
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }
    if (m_worker) {
        m_worker->closeSftp();
        delete m_worker;
        m_worker = nullptr;
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
}

void SftpWindow::setupMenus() {
    QMenuBar *mb = menuBar();
    mb->setNativeMenuBar(false);
    mb->addMenu("&File");
    mb->addMenu("&Edit");
    mb->addMenu("&View");
    mb->addMenu("&Operation");
    mb->addMenu("&Window");
    mb->addMenu("&Help");
}

void SftpWindow::setupToolBars() {
    // Tier 1: Action bar
    QToolBar *tb1 = addToolBar("SFTP Standard");
    tb1->setMovable(false);
    tb1->setIconSize(QSize(18, 18));

    tb1->addAction(QIcon(":/icons/save.svg"), "Save");
    tb1->addAction(QIcon(":/icons/disconnect.svg"), "Disconnect");

    tb1->addSeparator();

    tb1->addAction(QIcon(":/icons/copy.svg"), "Copy");
    tb1->addAction(QIcon(":/icons/paste.svg"), "Paste");

    tb1->addSeparator();

    tb1->addAction(QIcon(":/icons/tunnel.svg"), "Tunneling");
    tb1->addAction(QIcon(":/icons/sftp.svg"), "File Transfer Window");

    tb1->addSeparator();

    tb1->addAction(QIcon(":/icons/settings.svg"), "Settings");

    tb1->addSeparator();

    // Download arrow (Down)
    QAction *downAct = tb1->addAction("⬇ Download");
    connect(downAct, &QAction::triggered, this, &SftpWindow::onDownloadClicked);

    // Upload arrow (Up)
    QAction *upAct = tb1->addAction("⬆ Upload");
    connect(upAct, &QAction::triggered, this, &SftpWindow::onUploadClicked);
}

QWidget* SftpWindow::createPaneHeader(bool isLocal) {
    QWidget *header = new QWidget(this);
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(4, 2, 4, 2);
    hl->setSpacing(4);

    QLabel *lbl = new QLabel(isLocal ? "Local:" : "Remote:", header);
    lbl->setStyleSheet("font-weight: bold;");
    hl->addWidget(lbl);

    QComboBox *combo = new QComboBox(header);
    combo->setEditable(true);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    if (isLocal) {
        m_localCombo = combo;
        m_localCombo->addItem(m_currentLocalPath);
        connect(m_localCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
            navigateLocal(m_localCombo->currentText());
        });
    } else {
        m_remoteCombo = combo;
        m_remoteCombo->addItem(m_currentRemotePath);
        connect(m_remoteCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
            navigateRemote(m_remoteCombo->currentText());
        });
    }
    hl->addWidget(combo, 1);

    QPushButton *upBtn = new QPushButton("▲ Up", header);
    upBtn->setFixedWidth(45);
    connect(upBtn, &QPushButton::clicked, this, [this, isLocal]() {
        if (isLocal) {
            QDir d(m_currentLocalPath);
            if (d.cdUp()) {
                navigateLocal(d.absolutePath());
            }
        } else {
            QString p = m_currentRemotePath;
            int lastSlash = p.lastIndexOf('/');
            if (lastSlash > 0) {
                p = p.left(lastSlash);
            } else if (lastSlash == 0) {
                p = "/";
            }
            navigateRemote(p);
        }
    });
    hl->addWidget(upBtn);

    return header;
}

void SftpWindow::setupUi() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *cvl = new QVBoxLayout(central);
    cvl->setContentsMargins(4, 4, 4, 4);
    cvl->setSpacing(4);

    // Main vertical splitter (Top: Dual panes, Bottom: Queue/Transfer log)
    QSplitter *vSplitter = new QSplitter(Qt::Vertical, central);

    // Horizontal splitter for dual panes (Left: Local Tree, Right: Remote Tree)
    QSplitter *hSplitter = new QSplitter(Qt::Horizontal, vSplitter);

    // --- Left Pane: Local Files Tree ---
    QWidget *localWidget = new QWidget(hSplitter);
    QVBoxLayout *localVl = new QVBoxLayout(localWidget);
    localVl->setContentsMargins(0, 0, 0, 0);
    localVl->setSpacing(2);

    localVl->addWidget(createPaneHeader(true));

    m_localModel = new QFileSystemModel(this);
    m_localModel->setRootPath(m_currentLocalPath);

    m_localTree = new QTreeView(localWidget);
    m_localTree->setModel(m_localModel);
    m_localTree->setRootIndex(m_localModel->index(m_currentLocalPath));
    m_localTree->setSortingEnabled(true);
    m_localTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_localTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_localTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_localTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_localTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_localTree, &QTreeView::doubleClicked, this, &SftpWindow::onLocalItemDoubleClicked);
    connect(m_localTree, &QTreeView::customContextMenuRequested, this, &SftpWindow::onLocalContextMenu);
    localVl->addWidget(m_localTree);
    hSplitter->addWidget(localWidget);

    // --- Right Pane: Remote Files Tree (matching Left Pane Tree format) ---
    QWidget *remoteWidget = new QWidget(hSplitter);
    QVBoxLayout *remoteVl = new QVBoxLayout(remoteWidget);
    remoteVl->setContentsMargins(0, 0, 0, 0);
    remoteVl->setSpacing(2);

    remoteVl->addWidget(createPaneHeader(false));

    m_remoteTree = new QTreeWidget(remoteWidget);
    m_remoteTree->setColumnCount(4);
    m_remoteTree->setHeaderLabels({"Name", "Size", "Type", "Date Modified"});
    m_remoteTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_remoteTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_remoteTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_remoteTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_remoteTree->setSortingEnabled(true);
    m_remoteTree->setRootIsDecorated(false);
    m_remoteTree->setAlternatingRowColors(false);
    m_remoteTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_remoteTree, &QTreeWidget::itemDoubleClicked, this, &SftpWindow::onRemoteItemDoubleClicked);
    connect(m_remoteTree, &QTreeWidget::customContextMenuRequested, this, &SftpWindow::onRemoteContextMenu);
    remoteVl->addWidget(m_remoteTree);
    hSplitter->addWidget(remoteWidget);

    vSplitter->addWidget(hSplitter);

    // --- Bottom: Queue & Transfer Tabs ---
    m_tabWidget = new QTabWidget(vSplitter);

    m_queueTable = new QTableWidget(m_tabWidget);
    m_queueTable->setColumnCount(4);
    m_queueTable->setHorizontalHeaderLabels({"Local File", "Remote File", "Operation", "Status"});
    m_queueTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabWidget->addTab(m_queueTable, "Queue");

    m_transferLog = new QTextEdit(m_tabWidget);
    m_transferLog->setReadOnly(true);
    m_transferLog->setFont(QFont("Monospace", 9));
    m_transferLog->setStyleSheet("background-color: #FFFFFF; color: #000000; border: none;");
    m_tabWidget->addTab(m_transferLog, "Transfer Log");

    vSplitter->addWidget(m_tabWidget);
    vSplitter->setStretchFactor(0, 3);
    vSplitter->setStretchFactor(1, 1);

    cvl->addWidget(vSplitter);
    setCentralWidget(central);

    // StatusBar
    QStatusBar *sb = statusBar();
    m_statusLabel = new QLabel("Ready.", this);
    sb->addWidget(m_statusLabel, 1);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QIcon(":/icons/net_status.svg").pixmap(16, 16));
    sb->addPermanentWidget(m_iconLabel);
}

void SftpWindow::navigateLocal(const QString &path) {
    if (QDir(path).exists()) {
        m_currentLocalPath = QDir(path).absolutePath();
        if (m_localCombo) m_localCombo->setEditText(m_currentLocalPath);
        m_localTree->setRootIndex(m_localModel->index(m_currentLocalPath));
    }
}

void SftpWindow::onLocalItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;
    QString path = m_localModel->filePath(index);
    if (QFileInfo(path).isDir()) {
        navigateLocal(path);
    }
}

void SftpWindow::addQueueItem(const QString &local, const QString &remote, const QString &op) {
    if (!m_queueTable) return;
    int r = m_queueTable->rowCount();
    m_queueTable->insertRow(r);
    m_queueTable->setItem(r, 0, new QTableWidgetItem(local));
    m_queueTable->setItem(r, 1, new QTableWidgetItem(remote));
    m_queueTable->setItem(r, 2, new QTableWidgetItem(op));
    m_queueTable->setItem(r, 3, new QTableWidgetItem("In Progress"));
    m_queueTable->scrollToBottom();
}

void SftpWindow::updateQueueItem(const QString &file, const QString &status) {
    if (!m_queueTable) return;
    for (int r = 0; r < m_queueTable->rowCount(); ++r) {
        auto *it0 = m_queueTable->item(r, 0);
        auto *it1 = m_queueTable->item(r, 1);
        if ((it0 && it0->text().endsWith(file)) || (it1 && it1->text().endsWith(file))) {
            m_queueTable->setItem(r, 3, new QTableWidgetItem(status));
            break;
        }
    }
}

void SftpWindow::appendLogText(const QString &text) {
    if (!m_transferLog) return;
    QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ");
    m_transferLog->append(timestamp + text);
}

void SftpWindow::onLocalContextMenu(const QPoint &pos) {
    QModelIndex idx = m_localTree->indexAt(pos);
    QMenu menu(this);

    if (idx.isValid()) {
        QString localPath = m_localModel->filePath(idx);
        QFileInfo fi(localPath);

        if (fi.isDir()) {
            QAction *openAct = menu.addAction(QString("Open '%1'").arg(fi.fileName()));
            connect(openAct, &QAction::triggered, this, [this, localPath]() {
                navigateLocal(localPath);
            });

            QAction *upDirAct = menu.addAction(QIcon(":/icons/save.svg"), QString("Upload Directory '%1' to Remote").arg(fi.fileName()));
            connect(upDirAct, &QAction::triggered, this, [this, localPath, fi]() {
                addQueueItem(localPath, m_currentRemotePath + "/" + fi.fileName(), "Upload Dir");
                m_statusLabel->setText("Uploading directory " + fi.fileName() + "...");
                if (m_worker) {
                    QMetaObject::invokeMethod(m_worker, "uploadDirectory", Qt::QueuedConnection,
                                              Q_ARG(QString, localPath),
                                              Q_ARG(QString, m_currentRemotePath));
                }
            });
        } else {
            QAction *uploadAct = menu.addAction(QIcon(":/icons/save.svg"), QString("Upload '%1' to Remote").arg(fi.fileName()));
            connect(uploadAct, &QAction::triggered, this, [this, localPath, fi]() {
                QString remotePath = m_currentRemotePath;
                if (!remotePath.endsWith('/')) remotePath += '/';
                remotePath += fi.fileName();
                addQueueItem(localPath, remotePath, "Upload");
                m_statusLabel->setText("Uploading " + fi.fileName() + "...");
                if (m_worker) {
                    QMetaObject::invokeMethod(m_worker, "uploadFile", Qt::QueuedConnection,
                                              Q_ARG(QString, localPath),
                                              Q_ARG(QString, remotePath));
                }
            });
        }

        menu.addSeparator();

        QAction *deleteAct = menu.addAction("Delete");
        connect(deleteAct, &QAction::triggered, this, [this, localPath, fi]() {
            auto ans = QMessageBox::question(this, "Delete", QString("Are you sure you want to delete '%1'?").arg(fi.fileName()));
            if (ans == QMessageBox::Yes) {
                if (fi.isDir()) {
                    QDir(localPath).removeRecursively();
                } else {
                    QFile::remove(localPath);
                }
                m_localModel->setRootPath(m_currentLocalPath);
            }
        });

        QAction *renameAct = menu.addAction("Rename...");
        connect(renameAct, &QAction::triggered, this, [this, localPath, fi]() {
            bool ok = false;
            QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, fi.fileName(), &ok);
            if (ok && !newName.isEmpty() && newName != fi.fileName()) {
                QString newPath = fi.dir().filePath(newName);
                QFile::rename(localPath, newPath);
            }
        });

        menu.addSeparator();
    }

    QAction *newFolderAct = menu.addAction("New Folder...");
    connect(newFolderAct, &QAction::triggered, this, [this]() {
        bool ok = false;
        QString folderName = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "New Folder", &ok);
        if (ok && !folderName.isEmpty()) {
            QDir(m_currentLocalPath).mkdir(folderName);
        }
    });

    QAction *refreshAct = menu.addAction("Refresh");
    connect(refreshAct, &QAction::triggered, this, [this]() {
        navigateLocal(m_currentLocalPath);
    });

    menu.exec(m_localTree->viewport()->mapToGlobal(pos));
}

void SftpWindow::onRemoteContextMenu(const QPoint &pos) {
    auto *item = m_remoteTree->itemAt(pos);
    QMenu menu(this);

    if (item) {
        bool isDir = item->data(0, Qt::UserRole).toBool();
        QString filename = item->text(0);
        QString remotePath = m_currentRemotePath;
        if (!remotePath.endsWith('/')) remotePath += '/';
        remotePath += filename;

        if (isDir) {
            QAction *openAct = menu.addAction(QString("Open '%1'").arg(filename));
            connect(openAct, &QAction::triggered, this, [this, remotePath]() {
                navigateRemote(remotePath);
            });

            QAction *dlDirAct = menu.addAction(QIcon(":/icons/save.svg"), QString("Download Directory '%1' to Local").arg(filename));
            connect(dlDirAct, &QAction::triggered, this, [this, remotePath, filename]() {
                addQueueItem(m_currentLocalPath + "/" + filename, remotePath, "Download Dir");
                m_statusLabel->setText("Downloading directory " + filename + "...");
                if (m_worker) {
                    QMetaObject::invokeMethod(m_worker, "downloadDirectory", Qt::QueuedConnection,
                                              Q_ARG(QString, remotePath),
                                              Q_ARG(QString, m_currentLocalPath));
                }
            });
        } else {
            QAction *dlAct = menu.addAction(QIcon(":/icons/save.svg"), QString("Download '%1' to Local").arg(filename));
            connect(dlAct, &QAction::triggered, this, [this, filename, remotePath]() {
                QString localPath = m_currentLocalPath + "/" + filename;
                addQueueItem(localPath, remotePath, "Download");
                m_statusLabel->setText("Downloading " + filename + "...");
                if (m_worker) {
                    QMetaObject::invokeMethod(m_worker, "downloadFile", Qt::QueuedConnection,
                                              Q_ARG(QString, remotePath),
                                              Q_ARG(QString, localPath));
                }
            });
        }

        menu.addSeparator();

        QAction *deleteAct = menu.addAction("Delete");
        connect(deleteAct, &QAction::triggered, this, [this, filename, remotePath, isDir]() {
            auto ans = QMessageBox::question(this, "Delete Remote", QString("Are you sure you want to delete '%1' on remote?").arg(filename));
            if (ans == QMessageBox::Yes && m_worker) {
                if (isDir) {
                    QMetaObject::invokeMethod(m_worker, "deleteRemoteDir", Qt::QueuedConnection, Q_ARG(QString, remotePath));
                } else {
                    QMetaObject::invokeMethod(m_worker, "deleteRemoteFile", Qt::QueuedConnection, Q_ARG(QString, remotePath));
                }
                navigateRemote(m_currentRemotePath);
            }
        });

        QAction *renameAct = menu.addAction("Rename...");
        connect(renameAct, &QAction::triggered, this, [this, filename, remotePath]() {
            bool ok = false;
            QString newName = QInputDialog::getText(this, "Rename Remote", "New name:", QLineEdit::Normal, filename, &ok);
            if (ok && !newName.isEmpty() && newName != filename && m_worker) {
                QString newRemotePath = m_currentRemotePath;
                if (!newRemotePath.endsWith('/')) newRemotePath += '/';
                newRemotePath += newName;
                QMetaObject::invokeMethod(m_worker, "renameRemote", Qt::QueuedConnection,
                                          Q_ARG(QString, remotePath),
                                          Q_ARG(QString, newRemotePath));
                navigateRemote(m_currentRemotePath);
            }
        });

        menu.addSeparator();
    }

    QAction *newFolderAct = menu.addAction("New Folder...");
    connect(newFolderAct, &QAction::triggered, this, [this]() {
        bool ok = false;
        QString folderName = QInputDialog::getText(this, "New Remote Folder", "Folder name:", QLineEdit::Normal, "new_folder", &ok);
        if (ok && !folderName.isEmpty() && m_worker) {
            QString newPath = m_currentRemotePath;
            if (!newPath.endsWith('/')) newPath += '/';
            newPath += folderName;
            QMetaObject::invokeMethod(m_worker, "createRemoteDir", Qt::QueuedConnection, Q_ARG(QString, newPath));
            navigateRemote(m_currentRemotePath);
        }
    });

    QAction *refreshAct = menu.addAction("Refresh");
    connect(refreshAct, &QAction::triggered, this, [this]() {
        navigateRemote(m_currentRemotePath);
    });

    menu.exec(m_remoteTree->viewport()->mapToGlobal(pos));
}

void SftpWindow::navigateRemote(const QString &path) {
    m_currentRemotePath = path;
    if (m_remoteCombo) m_remoteCombo->setEditText(path);
    m_statusLabel->setText("Listing " + path + "...");
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "listRemoteDir", Qt::QueuedConnection, Q_ARG(QString, path));
    }
}

void SftpWindow::onRemoteDirListed(const QString &path, const QList<RemoteFileInfo> &files) {
    m_currentRemotePath = path;
    m_remoteTree->clear();

    for (const auto &f : files) {
        auto *item = new QTreeWidgetItem(m_remoteTree);
        item->setText(0, f.name);
        item->setData(0, Qt::UserRole, f.isDir);
        item->setIcon(0, QIcon(f.isDir ? ":/icons/sftp.svg" : ":/icons/save.svg"));

        item->setText(1, f.isDir ? "" : QString::number(f.size));
        item->setText(2, f.isDir ? "Folder" : "File");
        item->setText(3, f.lastModified.toString("yyyy-MM-dd hh:mm"));
    }
    m_statusLabel->setText(QString("%1 objects listed in %2").arg(files.size()).arg(path));
}

void SftpWindow::onRemoteItemDoubleClicked(QTreeWidgetItem *item, int) {
    if (!item) return;

    bool isDir = item->data(0, Qt::UserRole).toBool();
    QString name = item->text(0);

    if (isDir) {
        QString nextPath = m_currentRemotePath;
        if (!nextPath.endsWith('/')) nextPath += '/';
        nextPath += name;
        navigateRemote(nextPath);
    }
}

void SftpWindow::onUploadClicked() {
    QModelIndex idx = m_localTree->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "Upload", "Please select a local file or directory to upload.");
        return;
    }

    QString localPath = m_localModel->filePath(idx);
    bool isDir = QFileInfo(localPath).isDir();
    QString filename = QFileInfo(localPath).fileName();
    QString remotePath = m_currentRemotePath;
    if (!remotePath.endsWith('/')) remotePath += '/';
    remotePath += filename;

    addQueueItem(localPath, remotePath, isDir ? "Upload Dir" : "Upload");
    m_statusLabel->setText(QString("Uploading %1...").arg(filename));
    if (m_worker) {
        if (isDir) {
            QMetaObject::invokeMethod(m_worker, "uploadDirectory", Qt::QueuedConnection,
                                      Q_ARG(QString, localPath),
                                      Q_ARG(QString, m_currentRemotePath));
        } else {
            QMetaObject::invokeMethod(m_worker, "uploadFile", Qt::QueuedConnection,
                                      Q_ARG(QString, localPath),
                                      Q_ARG(QString, remotePath));
        }
    }
}

void SftpWindow::onDownloadClicked() {
    auto *item = m_remoteTree->currentItem();
    if (!item) {
        QMessageBox::information(this, "Download", "Please select a remote file or directory to download.");
        return;
    }

    bool isDir = item->data(0, Qt::UserRole).toBool();
    QString filename = item->text(0);
    QString remotePath = m_currentRemotePath;
    if (!remotePath.endsWith('/')) remotePath += '/';
    remotePath += filename;

    QString localPath = m_currentLocalPath + "/" + filename;
    addQueueItem(localPath, remotePath, isDir ? "Download Dir" : "Download");
    m_statusLabel->setText(QString("Downloading %1...").arg(filename));
    if (m_worker) {
        if (isDir) {
            QMetaObject::invokeMethod(m_worker, "downloadDirectory", Qt::QueuedConnection,
                                      Q_ARG(QString, remotePath),
                                      Q_ARG(QString, m_currentLocalPath));
        } else {
            QMetaObject::invokeMethod(m_worker, "downloadFile", Qt::QueuedConnection,
                                      Q_ARG(QString, remotePath),
                                      Q_ARG(QString, localPath));
        }
    }
}

void SftpWindow::onTransferProgress(const QString &filename, qint64 bytes, qint64 total) {
    m_statusLabel->setText(QString("Transferring %1: %2 / %3 bytes").arg(filename).arg(bytes).arg(total));
}

void SftpWindow::onTransferFinished(const QString &filename, bool success, const QString &message, qint64 totalBytes) {
    QString status = success ? "Complete" : ("Failed: " + message);
    updateQueueItem(filename, status);

    QString logMsg;
    if (success) {
        logMsg = QString("File '%1' transferred successfully (%2 bytes).").arg(filename).arg(totalBytes);
    } else {
        logMsg = QString("Transfer of '%1' failed: %2 (transferred %3 bytes).").arg(filename, message).arg(totalBytes);
    }
    appendLogText(logMsg);

    if (success) {
        m_statusLabel->setText(QString("Transfer complete: %1 (%2 bytes)").arg(filename).arg(totalBytes));
        navigateRemote(m_currentRemotePath);
    } else {
        m_statusLabel->setText("Transfer failed: " + message);
    }
}
