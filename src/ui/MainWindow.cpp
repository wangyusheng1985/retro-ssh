#include "MainWindow.h"
#include "ui/AddProfileDialog.h"
#include "ui/EditProfilesDialog.h"
#include "backend/ProfileManager.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QApplication>
#include <QScrollBar>
#include <QShortcut>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QTemporaryFile>
#include <QDateTime>
#include <QtConcurrent/QtConcurrent>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("RetroSSH");
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    resize(820, 560);

    // Central Stacked Widget holding terminal sessions
    m_sessionStack = new QStackedWidget(this);
    setCentralWidget(m_sessionStack);

    setupMenus();
    setupToolBars();
    setupStatusBar();

    // Default connection parameters
    m_currentParams.host = "10.9.34.84";
    m_currentParams.port = 22;
    m_currentParams.username = "root";
    m_currentParams.authMethod = "Public Key";

    // Show blank initial terminal without adding any tabs to ProfileBar
    createNewSessionTab("", m_currentParams);

    // Setup shortcuts for Cmd+1..9 to switch profile tabs
    for (int i = 1; i <= 9; ++i) {
        new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i)), this, [this, i]() {
            if (m_profileTabBar && m_profileTabBar->count() > 0) {
                int targetTab = i - 1;
                if (targetTab >= 0 && targetTab < m_profileTabBar->count()) {
                    m_profileTabBar->setCurrentIndex(targetTab);
                }
            }
        });
    }
}

MainWindow::~MainWindow() {
    for (auto &s : m_sessions) {
        if (s.worker) {
            s.worker->disconnectFromServer();
            s.worker->wait(500);
            delete s.worker;
        }
    }
}

SessionTabInfo* MainWindow::currentSession() {
    QWidget *curr = m_sessionStack ? m_sessionStack->currentWidget() : nullptr;
    if (!curr) return nullptr;
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].containerWidget == curr) {
            return &m_sessions[i];
        }
    }
    return nullptr;
}

int MainWindow::createNewSessionTab(const QString &title, const SshConnectionParams &params) {
    SessionTabInfo s;
    s.title = title;
    s.params = params;

    // Outer container with 3D sunken border
    QFrame *container = new QFrame(this);
    container->setObjectName("TerminalContainer");
    QVBoxLayout *cvl = new QVBoxLayout(container);
    cvl->setContentsMargins(1, 1, 1, 1);
    cvl->setSpacing(0);

    // Terminal layout with right-hand classic scrollbar
    QWidget *termBox = new QWidget(container);
    QHBoxLayout *tbHl = new QHBoxLayout(termBox);
    tbHl->setContentsMargins(0, 0, 0, 0);
    tbHl->setSpacing(0);

    s.terminal = new TerminalWidget(termBox);
    tbHl->addWidget(s.terminal, 1);

    QScrollBar *scrollBar = new QScrollBar(Qt::Vertical, termBox);
    s.terminal->bindScrollBar(scrollBar);
    tbHl->addWidget(scrollBar);

    cvl->addWidget(termBox);
    s.containerWidget = container;

    // Always apply global font and appearance settings
    GlobalAppearanceSettings g = ProfileManager::instance().getGlobalAppearance();
    s.terminal->setTerminalFont(g.fontFamily, g.fontSize);
    s.terminal->setMaxScrollback(g.scrollbackBuffer);
    s.terminal->setScrollOnOutput(g.scrollOnOutput);
    s.terminal->setPasteOnRightClick(g.pasteOnRightClick);
    s.terminal->setTerminalColors(
        ProfileSettingsDialog::colorNameToQColor(g.fgColor),
        ProfileSettingsDialog::colorNameToQColor(g.bgColor),
        ProfileSettingsDialog::colorNameToQColor(g.selColor),
        ProfileSettingsDialog::colorNameToQColor(g.cursorColor),
        g.enableAnsiColors,
        g.reverseVideo
    );

    // Apply saved profile specific appearance/colors if available
    QString lookupName = title.isEmpty() ? params.host : title;
    if (ProfileManager::instance().hasProfile(lookupName)) {
        ProfileData p = ProfileManager::instance().getProfile(lookupName);
        if (!p.fontFamily.isEmpty()) {
            s.terminal->setTerminalFont(p.fontFamily, p.fontSize);
        }
        s.terminal->setMaxScrollback(p.scrollbackBuffer);
        s.terminal->setScrollOnOutput(p.scrollOnOutput);
        s.terminal->setPasteOnRightClick(p.pasteOnRightClick);
        s.terminal->setTerminalColors(
            ProfileSettingsDialog::colorNameToQColor(p.fgColor),
            ProfileSettingsDialog::colorNameToQColor(p.bgColor),
            ProfileSettingsDialog::colorNameToQColor(p.selColor),
            ProfileSettingsDialog::colorNameToQColor(p.cursorColor),
            p.enableAnsiColors,
            p.reverseVideo
        );
    }

    // Create worker for this session
    s.worker = new SshSessionWorker(this);

    int newIndex = m_sessions.size();
    m_sessions.append(s);
    m_sessionStack->addWidget(container);

    // Add Tab to ProfileBar only if title is not empty
    if (m_profileTabBar && !title.isEmpty()) {
        m_profileTabBar->blockSignals(true);
        int tabIdx = m_profileTabBar->addTab(QIcon(":/icons/connect.svg"), title);
        m_profileTabBar->setTabData(tabIdx, QVariant::fromValue(reinterpret_cast<quintptr>(container)));
        m_profileTabBar->setCurrentIndex(tabIdx);
        m_profileTabBar->blockSignals(false);
    }

    m_sessionStack->setCurrentWidget(container);

    // Wire terminal and worker
    TerminalWidget *term = s.terminal;
    SshSessionWorker *worker = s.worker;

    connect(term, &TerminalWidget::dataToSend, this, [this, worker](const QByteArray &data) {
        if (worker && worker->isConnected()) {
            worker->sendData(data);
        }
    });

    connect(term, &TerminalWidget::terminalResized, this, [this, worker](int c, int r) {
        m_dimensionsLabel->setText(QString("%1x%2").arg(c).arg(r));
        if (worker && worker->isConnected()) {
            worker->resizePty(c, r);
        }
    });

    connect(term, &TerminalWidget::newTerminalRequested, this, &MainWindow::onConnectTriggered);
    connect(term, &TerminalWidget::newFileTransferRequested, this, &MainWindow::onSftpTriggered);
    connect(term, &TerminalWidget::quickConnectRequested, this, &MainWindow::onQuickConnectTriggered);
    connect(term, &TerminalWidget::closeWindowRequested, this, &QWidget::close);
    connect(term, &TerminalWidget::settingsRequested, this, &MainWindow::onSettingsTriggered);
    connect(term, &TerminalWidget::uploadClipboardImageRequested, this, [this]() {
        uploadClipboardImage(currentSession());
    });
    connect(term, &TerminalWidget::windowTitleChanged, this, [this, container](const QString &t) {
        if (m_sessionStack->currentWidget() == container) {
            setWindowTitle(QString("%1 - RetroSSH").arg(t));
        }
    });

    connect(worker, &SshSessionWorker::dataReceived, term, &TerminalWidget::appendRawData);

    connect(worker, &SshSessionWorker::connected, this, [this, container](const QString &banner) {
        if (m_sessionStack->currentWidget() == container) {
            auto *s = currentSession();
            if (s) {
                QString winTitle = s->title.isEmpty() ?
                    QString("%1 - RetroSSH").arg(s->params.host) :
                    QString("%1 - %2 - RetroSSH").arg(s->params.host, s->title);
                setWindowTitle(winTitle);
                m_statusLabel->setText(QString("Connected to %1").arg(s->params.host));
                m_emptyLabel->setText("Connected");
                m_connectAct->setEnabled(false);
                m_disconnectAct->setEnabled(true);
            }
        }
    });

    connect(worker, &SshSessionWorker::disconnected, this, [this, container, term]() {
        term->resetToDefaultBanner();
        if (m_sessionStack->currentWidget() == container) {
            auto *s = currentSession();
            QString t = s ? s->title : "";
            setWindowTitle(QString("- %1 - RetroSSH").arg(t));
            m_statusLabel->setText("Not connected - press Enter or Space to connect");
            m_emptyLabel->setText("");
            m_connectAct->setEnabled(true);
            m_disconnectAct->setEnabled(false);
        }
    });

    connect(worker, &SshSessionWorker::connectionError, this, [this](const QString &err) {
        m_statusLabel->setText(err);
        QMessageBox::critical(this, "Connection Error", err);
    });

    connect(worker, &SshSessionWorker::statusMessage, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
    });

    return newIndex;
}

void MainWindow::showDefaultStartingPage() {
    if (m_sessions.isEmpty()) return;

    auto &def = m_sessions[0];
    m_sessionStack->setCurrentWidget(def.containerWidget);
    m_currentParams = def.params;

    if (def.terminal) {
        def.terminal->resetToDefaultBanner();
        def.terminal->setFocus();
        m_dimensionsLabel->setText(QString("%1x%2").arg(def.terminal->cols()).arg(def.terminal->rows()));
    }

    setWindowTitle("RetroSSH");
    m_statusLabel->setText("Ready. Connect quickly without profiles.");
    m_emptyLabel->setText("");
    m_connectAct->setEnabled(true);
    m_disconnectAct->setEnabled(false);
}

void MainWindow::onCurrentTabChanged(int tabIndex) {
    if (tabIndex < 0 || !m_profileTabBar || tabIndex >= m_profileTabBar->count()) {
        showDefaultStartingPage();
        return;
    }

    QWidget *container = reinterpret_cast<QWidget*>(m_profileTabBar->tabData(tabIndex).value<quintptr>());
    if (!container) return;

    int sessIdx = -1;
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].containerWidget == container) {
            sessIdx = i;
            break;
        }
    }

    if (sessIdx >= 0) {
        m_sessionStack->setCurrentWidget(container);
        auto &s = m_sessions[sessIdx];
        m_currentParams = s.params;

        if (s.worker && s.worker->isConnected()) {
            setWindowTitle(QString("%1 - %2 - RetroSSH").arg(s.params.host, s.title));
            m_statusLabel->setText(QString("Connected to %1").arg(s.params.host));
            m_emptyLabel->setText("Connected");
            m_connectAct->setEnabled(false);
            m_disconnectAct->setEnabled(true);
        } else {
            setWindowTitle(QString("- %1 - RetroSSH").arg(s.title));
            m_statusLabel->setText("Not connected - press Enter or Space to connect");
            m_emptyLabel->setText("");
            m_connectAct->setEnabled(true);
            m_disconnectAct->setEnabled(false);
        }

        m_dimensionsLabel->setText(QString("%1x%2").arg(s.terminal->cols()).arg(s.terminal->rows()));
        s.terminal->setFocus();
    }
}

void MainWindow::onTabCloseRequested(int tabIndex) {
    if (tabIndex < 0 || !m_profileTabBar || tabIndex >= m_profileTabBar->count()) return;

    QWidget *container = reinterpret_cast<QWidget*>(m_profileTabBar->tabData(tabIndex).value<quintptr>());

    int sessIdx = -1;
    for (int i = 1; i < m_sessions.size(); ++i) { // i=0 is default session, tabs are i >= 1
        if (m_sessions[i].containerWidget == container) {
            sessIdx = i;
            break;
        }
    }

    if (sessIdx >= 1) {
        auto s = m_sessions[sessIdx];
        if (s.worker) {
            s.worker->disconnectFromServer();
            s.worker->wait(300);
            delete s.worker;
        }

        m_sessionStack->removeWidget(s.containerWidget);
        delete s.containerWidget;
        m_sessions.removeAt(sessIdx);
    }

    m_profileTabBar->blockSignals(true);
    m_profileTabBar->removeTab(tabIndex);
    m_profileTabBar->blockSignals(false);

    if (m_profileTabBar->count() == 0) {
        showDefaultStartingPage();
    } else {
        int nextTab = qBound(0, tabIndex, m_profileTabBar->count() - 1);
        m_profileTabBar->setCurrentIndex(nextTab);
        onCurrentTabChanged(nextTab);
    }
}

void MainWindow::setupMenus() {
    QMenuBar *mb = menuBar();
    mb->setNativeMenuBar(false);

    // File Menu
    QMenu *fileMenu = mb->addMenu("&File");
    fileMenu->addAction(QIcon(":/icons/save.svg"), "&Save Settings", this, &MainWindow::onSaveSettingsTriggered, QKeySequence("Ctrl+S"));
    fileMenu->addSeparator();

    fileMenu->addAction(QIcon(":/icons/quick_connect.svg"), "&Quick Connect...", this, &MainWindow::onQuickConnectTriggered);

    m_profilesMenu = fileMenu->addMenu("&Profiles");
    rebuildProfilesMenu();

    fileMenu->addSeparator();

    m_connectAct = fileMenu->addAction(QIcon(":/icons/connect.svg"), "&Connect...", this, &MainWindow::onConnectTriggered);
    m_disconnectAct = fileMenu->addAction(QIcon(":/icons/disconnect.svg"), "&Disconnect", this, &MainWindow::onDisconnectTriggered);
    m_disconnectAct->setEnabled(false);
    fileMenu->addSeparator();

    fileMenu->addAction("New &Profile...", this, &MainWindow::onAddProfileTriggered, QKeySequence("Ctrl+N"));
    fileMenu->addSeparator();

    fileMenu->addAction("E&xit", this, &QWidget::close);

    // Edit Menu
    QMenu *editMenu = mb->addMenu("&Edit");
    QAction *copyAct = editMenu->addAction(QIcon(":/icons/copy.svg"), "&Copy\tCtrl+Insert", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->copySelection();
    }, QKeySequence::Copy);
    editMenu->addAction(QIcon(":/icons/paste.svg"), "&Paste\tShift+Insert", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->pasteClipboard();
    }, QKeySequence::Paste);
    editMenu->addAction(QIcon(":/icons/paste.svg"), "Paste &Selection", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->pasteSelection();
    });
    editMenu->addSeparator();

    editMenu->addAction("Select &All", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->selectAll();
    }, QKeySequence("Ctrl+A"));
    editMenu->addAction("Select Sc&reen", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->selectScreen();
    });
    editMenu->addAction("Select &None", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->selectNone();
    });
    editMenu->addSeparator();

    editMenu->addAction(QIcon(":/icons/find.svg"), "&Find...", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->showFindDialog();
    }, QKeySequence("Ctrl+F"));
    editMenu->addSeparator();

    editMenu->addAction(QIcon(":/icons/settings.svg"), "&Settings...", this, &MainWindow::onSettingsTriggered);

    connect(editMenu, &QMenu::aboutToShow, this, [this, copyAct]() {
        auto *s = currentSession();
        copyAct->setEnabled(s && s->terminal && s->terminal->hasSelection());
    });

    // View Menu
    QMenu *viewMenu = mb->addMenu("&View");
    QAction *tbAct = viewMenu->addAction("&Toolbar");
    tbAct->setCheckable(true);
    tbAct->setChecked(true);
    connect(tbAct, &QAction::toggled, this, [this](bool visible) {
        if (m_tier1) m_tier1->setVisible(visible);
    });

    QAction *sbAct = viewMenu->addAction("&Status Bar");
    sbAct->setCheckable(true);
    sbAct->setChecked(true);
    connect(sbAct, &QAction::toggled, this, [this](bool visible) {
        statusBar()->setVisible(visible);
    });

    QAction *pbAct = viewMenu->addAction("&Profiles Bar");
    pbAct->setCheckable(true);
    pbAct->setChecked(true);
    connect(pbAct, &QAction::toggled, this, [this](bool visible) {
        if (m_tier2) m_tier2->setVisible(visible);
    });

    viewMenu->addSeparator();

    viewMenu->addAction("Reset &Terminal", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->resetTerminal();
    });

    // Window Menu
    QMenu *windowMenu = mb->addMenu("&Window");
    m_sftpAct = windowMenu->addAction(QIcon(":/icons/sftp.svg"), "New &File Transfer Window", this, &MainWindow::onSftpTriggered, QKeySequence("Ctrl+F"));

    // Help Menu
    QMenu *helpMenu = mb->addMenu("&Help");
    helpMenu->addAction("&Help Topics", [this]() {
        QMessageBox::information(this, "Help Topics", "RetroSSH Client Help\nPress Space or Enter to Quick Connect.");
    });
    helpMenu->addAction("&About RetroSSH...", [this]() {
        QMessageBox::about(this, "About RetroSSH",
            "RetroSSH Client 1.0 (Build 2026)\n\n"
            "An authentic classic SSH2 client with modern cryptographic engines.\n"
            "Built with Qt C++, libssh2 and OpenSSL 3.x.\n\n"
            "Fast, reliable and native on macOS.");
    });
}

void MainWindow::setupToolBars() {
    // Tier 1: Standard Icon Toolbar
    m_tier1 = addToolBar("Standard");
    m_tier1->setMovable(false);
    m_tier1->setIconSize(QSize(18, 18));

    m_tier1->addAction(QIcon(":/icons/save.svg"), "Save Settings", this, &MainWindow::onSaveSettingsTriggered);
    m_tier1->addSeparator();

    m_tier1->addAction(m_connectAct);
    m_tier1->addAction(m_disconnectAct);

    m_tier1->addSeparator();

    m_tier1->addAction(QIcon(":/icons/copy.svg"), "Copy", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->copySelection();
    });
    m_tier1->addAction(QIcon(":/icons/paste.svg"), "Paste", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->pasteClipboard();
    });
    m_tier1->addAction(QIcon(":/icons/find.svg"), "Find", [this]() {
        auto *s = currentSession();
        if (s && s->terminal) s->terminal->showFindDialog();
    });

    m_tier1->addSeparator();

    m_tier1->addAction(QIcon(":/icons/tunnel.svg"), "Tunneling");
    m_tier1->addAction(QIcon(":/icons/sftp.svg"), "New File Transfer Window", this, &MainWindow::onSftpTriggered);

    m_tier1->addSeparator();

    m_tier1->addAction(QIcon(":/icons/settings.svg"), "Settings", this, &MainWindow::onSettingsTriggered);

    addToolBarBreak();

    // -------------------------------------------------------------
    // Tier 2: Profiles Bar (Quick Connect + Profiles + Open Profile Tabs)
    // -------------------------------------------------------------
    m_tier2 = addToolBar("Profiles Bar");
    m_tier2->setMovable(false);
    m_tier2->setIconSize(QSize(18, 18));

    // Quick Connect Button
    auto *qcBtn = new QToolButton(m_tier2);
    qcBtn->setText("Quick Connect");
    qcBtn->setIcon(QIcon(":/icons/quick_connect.svg"));
    qcBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(qcBtn, &QToolButton::clicked, this, &MainWindow::onQuickConnectTriggered);
    m_tier2->addWidget(qcBtn);

    // Profiles Button with Dropdown Arrow
    auto *profBtn = new QToolButton(m_tier2);
    profBtn->setText("Profiles");
    profBtn->setIcon(QIcon(":/icons/sftp.svg"));
    profBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    profBtn->setPopupMode(QToolButton::InstantPopup);
    profBtn->setMenu(m_profilesMenu);
    m_tier2->addWidget(profBtn);

    // Separator between Profiles button and dynamic Profile Tabs
    m_tier2->addSeparator();

    // Embedded Classic Windows QTabBar for open session profiles
    m_profileTabBar = new QTabBar(m_tier2);
    m_profileTabBar->setDrawBase(false);
    m_profileTabBar->setTabsClosable(true);
    m_profileTabBar->setMovable(true);
    m_profileTabBar->setStyleSheet(
        "QTabBar::tab { height: 22px; padding: 2px 10px; margin-left: 2px; }"
    );
    connect(m_profileTabBar, &QTabBar::currentChanged, this, &MainWindow::onCurrentTabChanged);
    connect(m_profileTabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    m_tier2->addWidget(m_profileTabBar);
}

void MainWindow::setupStatusBar() {
    QStatusBar *sb = statusBar();

    m_statusLabel = new QLabel("Connect quickly without profiles", this);
    sb->addWidget(m_statusLabel, 1);

    m_emptyLabel = new QLabel("", this);
    m_emptyLabel->setFixedWidth(120);
    sb->addPermanentWidget(m_emptyLabel);

    m_dimensionsLabel = new QLabel("80x24", this);
    m_dimensionsLabel->setFixedWidth(60);
    m_dimensionsLabel->setAlignment(Qt::AlignCenter);
    sb->addPermanentWidget(m_dimensionsLabel);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QIcon(":/icons/net_status.svg").pixmap(16, 16));
    m_iconLabel->setFixedWidth(24);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    sb->addPermanentWidget(m_iconLabel);
}

void MainWindow::rebuildProfilesMenu() {
    if (!m_profilesMenu) return;
    m_profilesMenu->clear();

    m_profilesMenu->addAction("Add Profile...", this, &MainWindow::onAddProfileTriggered);
    m_profilesMenu->addAction("Edit Profiles...", this, &MainWindow::onEditProfilesTriggered);
    m_profilesMenu->addSeparator();

    auto profiles = ProfileManager::instance().getProfiles();
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        QString pName = it.key();
        m_profilesMenu->addAction(pName, this, [this, pName]() {
            auto p = ProfileManager::instance().getProfile(pName);

            // Always open a new tab session for the profile
            int newIdx = createNewSessionTab(pName, p.params);
            connectSession(newIdx);
        });
    }
}

void MainWindow::connectSession(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= m_sessions.size()) return;
    auto &s = m_sessions[tabIndex];
    m_statusLabel->setText(QString("Connecting to %1 (%2)...").arg(s.title, s.params.host));
    s.worker->startConnection(s.params.host, s.params.port,
                              s.params.username, s.params.password,
                              s.params.authMethod);
}

void MainWindow::onAddProfileTriggered() {
    QString defName = m_currentParams.host.isEmpty() ? "New Profile" : m_currentParams.host;
    AddProfileDialog dlg(defName, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.profileName();
        if (!name.isEmpty()) {
            ProfileData p;
            p.name = name;
            p.params = m_currentParams;
            if (p.params.host.isEmpty()) {
                p.params.host = "10.9.34.84";
                p.params.username = "root";
                p.params.port = 22;
                p.params.authMethod = "Public Key";
            }
            ProfileManager::instance().addOrUpdateProfile(p);
            rebuildProfilesMenu();
            m_statusLabel->setText(QString("Profile '%1' added.").arg(name));
        }
    }
}

void MainWindow::onEditProfilesTriggered() {
    QString currentName = m_currentParams.host;
    EditProfilesDialog dlg(currentName, this);
    if (dlg.exec() == QDialog::Accepted) {
        rebuildProfilesMenu();
        if (dlg.shouldConnect() && !dlg.connectProfileName().isEmpty()) {
            auto p = ProfileManager::instance().getProfile(dlg.connectProfileName());
            int newIdx = createNewSessionTab(p.name, p.params);
            connectSession(newIdx);
        }
    }
}

void MainWindow::onQuickConnectTriggered() {
    QuickConnectDialog dlg(this);
    dlg.setParams(m_currentParams);
    if (dlg.exec() == QDialog::Accepted) {
        m_currentParams = dlg.getParams();

        // If we are currently on the initial blank default session (no tabs open),
        // create a new profile tab session and connect it.
        if (!m_profileTabBar || m_profileTabBar->count() == 0) {
            int newIdx = createNewSessionTab(m_currentParams.host, m_currentParams);
            connectSession(newIdx);
            return;
        }

        auto *s = currentSession();
        if (s) {
            s->params = m_currentParams;
            s->title = m_currentParams.host;
            if (m_profileTabBar) {
                m_profileTabBar->setTabText(m_profileTabBar->currentIndex(), s->title);
            }

            // Apply saved profile settings for host if available
            if (ProfileManager::instance().hasProfile(s->title)) {
                ProfileData p = ProfileManager::instance().getProfile(s->title);
                if (s->terminal) {
                    s->terminal->setTerminalFont(p.fontFamily, p.fontSize);
                    s->terminal->setMaxScrollback(p.scrollbackBuffer);
                    s->terminal->setScrollOnOutput(p.scrollOnOutput);
                    s->terminal->setPasteOnRightClick(p.pasteOnRightClick);
                    s->terminal->setTerminalColors(
                        ProfileSettingsDialog::colorNameToQColor(p.fgColor),
                        ProfileSettingsDialog::colorNameToQColor(p.bgColor),
                        ProfileSettingsDialog::colorNameToQColor(p.selColor),
                        ProfileSettingsDialog::colorNameToQColor(p.cursorColor),
                        p.enableAnsiColors,
                        p.reverseVideo
                    );
                }
            }

            int sessIdx = -1;
            for (int i = 0; i < m_sessions.size(); ++i) {
                if (m_sessions[i].containerWidget == s->containerWidget) {
                    sessIdx = i;
                    break;
                }
            }
            if (sessIdx >= 0) {
                connectSession(sessIdx);
            }
        }
    }
}

void MainWindow::onConnectTriggered() {
    auto *s = currentSession();
    if (!s) return;

    if (s->params.host.isEmpty()) {
        onQuickConnectTriggered();
        return;
    }

    int sessIdx = -1;
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].containerWidget == s->containerWidget) {
            sessIdx = i;
            break;
        }
    }
    if (sessIdx >= 0) {
        connectSession(sessIdx);
    }
}

void MainWindow::onDisconnectTriggered() {
    auto *s = currentSession();
    if (s && s->worker) {
        s->worker->disconnectFromServer();
    }
}

void MainWindow::onSaveSettingsTriggered() {
    auto *s = currentSession();
    if (!s) return;

    QString profileName = s->title;
    if (profileName.isEmpty()) {
        profileName = s->params.host.isEmpty() ? "default" : s->params.host;
    }

    ProfileData p;
    if (ProfileManager::instance().hasProfile(profileName)) {
        p = ProfileManager::instance().getProfile(profileName);
    } else {
        p.name = profileName;
    }

    p.params = s->params;
    if (s->terminal) {
        p.fontFamily = s->terminal->fontFamily();
        p.fontSize = s->terminal->fontSize();
        p.scrollbackBuffer = s->terminal->maxScrollback();
        p.scrollOnOutput = s->terminal->scrollOnOutput();
        p.pasteOnRightClick = s->terminal->pasteOnRightClick();

        p.fgColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->defaultFg());
        p.bgColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->defaultBg());
        p.selColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->selectionColor());
        p.cursorColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->cursorColor());
        p.enableAnsiColors = s->terminal->enableAnsiColors();
        p.reverseVideo = s->terminal->reverseVideo();
    }

    GlobalAppearanceSettings g;
    g.fontFamily = p.fontFamily;
    g.fontSize = p.fontSize;
    g.scrollbackBuffer = p.scrollbackBuffer;
    g.scrollOnOutput = p.scrollOnOutput;
    g.pasteOnRightClick = p.pasteOnRightClick;
    g.fgColor = p.fgColor;
    g.bgColor = p.bgColor;
    g.selColor = p.selColor;
    g.cursorColor = p.cursorColor;
    g.enableAnsiColors = p.enableAnsiColors;
    g.reverseVideo = p.reverseVideo;
    ProfileManager::instance().saveGlobalAppearance(g);

    ProfileManager::instance().addOrUpdateProfile(p);
    rebuildProfilesMenu();

    m_statusLabel->setText(QString("Configuration for profile '%1' saved.").arg(profileName));
    QMessageBox::information(this, "Save Settings", QString("All settings for profile '%1' (font, colors, appearance, connection) have been saved.").arg(profileName));
}

void MainWindow::onSettingsTriggered() {
    ProfileSettingsDialog dlg(this);
    auto *s = currentSession();
    QString pName = s ? s->title : "";
    if (pName.isEmpty() && s) pName = s->params.host;

    ProfileData p;
    if (ProfileManager::instance().hasProfile(pName)) {
        p = ProfileManager::instance().getProfile(pName);
    } else {
        p.params = m_currentParams;
    }

    if (s && s->terminal) {
        p.fontFamily = s->terminal->fontFamily();
        p.fontSize = s->terminal->fontSize();
        p.scrollbackBuffer = s->terminal->maxScrollback();
        p.scrollOnOutput = s->terminal->scrollOnOutput();
        p.pasteOnRightClick = s->terminal->pasteOnRightClick();
        p.fgColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->defaultFg());
        p.bgColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->defaultBg());
        p.selColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->selectionColor());
        p.cursorColor = ProfileSettingsDialog::qcolorToColorName(s->terminal->cursorColor());
        p.enableAnsiColors = s->terminal->enableAnsiColors();
        p.reverseVideo = s->terminal->reverseVideo();
    }

    dlg.setProfileData(p);
    if (dlg.exec() == QDialog::Accepted) {
        ProfileData updated = dlg.getProfileData();
        m_currentParams = updated.params;

        GlobalAppearanceSettings g;
        g.fontFamily = updated.fontFamily;
        g.fontSize = updated.fontSize;
        g.scrollbackBuffer = updated.scrollbackBuffer;
        g.scrollOnOutput = updated.scrollOnOutput;
        g.pasteOnRightClick = updated.pasteOnRightClick;
        g.fgColor = updated.fgColor;
        g.bgColor = updated.bgColor;
        g.selColor = updated.selColor;
        g.cursorColor = updated.cursorColor;
        g.enableAnsiColors = updated.enableAnsiColors;
        g.reverseVideo = updated.reverseVideo;
        ProfileManager::instance().saveGlobalAppearance(g);

        // Apply updated font/colors to all existing terminal sessions
        for (auto &sess : m_sessions) {
            if (sess.terminal) {
                sess.terminal->setTerminalFont(updated.fontFamily, updated.fontSize);
                sess.terminal->setMaxScrollback(updated.scrollbackBuffer);
                sess.terminal->setScrollOnOutput(updated.scrollOnOutput);
                sess.terminal->setPasteOnRightClick(updated.pasteOnRightClick);
                sess.terminal->setTerminalColors(
                    dlg.getForegroundColor(),
                    dlg.getBackgroundColor(),
                    dlg.getSelectionColor(),
                    dlg.getCursorColor(),
                    updated.enableAnsiColors,
                    updated.reverseVideo
                );
            }
        }

        if (s) {
            s->params = m_currentParams;
        }
    }
}

void MainWindow::onSftpTriggered() {
    auto *s = currentSession();
    SshConnectionParams params;
    QString remotePath = "/root";

    if (s && s->worker && s->worker->isConnected()) {
        params = s->params;
        if (s->terminal) {
            QString lastDir = s->terminal->lastWorkingDir();
            if (!lastDir.isEmpty()) {
                if (lastDir == "~") {
                    remotePath = (s->params.username == "root") ? "/root" : ("/home/" + s->params.username);
                } else if (lastDir.startsWith("~/")) {
                    QString home = (s->params.username == "root") ? "/root" : ("/home/" + s->params.username);
                    remotePath = home + lastDir.mid(1);
                } else if (lastDir.startsWith("/")) {
                    remotePath = lastDir;
                }
            } else {
                remotePath = (s->params.username == "root") ? "/root" : ("/home/" + s->params.username);
            }
        }
    } else {
        // Fallback to default or remembered params if no session is connected
        params = m_currentParams;
        if (params.host.isEmpty()) {
            params.host = "10.9.34.84";
            params.username = "root";
            params.port = 22;
            params.authMethod = "Public Key";
        }
        remotePath = (params.username == "root") ? "/root" : ("/home/" + params.username);
    }

    if (m_sftpWindow) {
        m_sftpWindow->close();
        delete m_sftpWindow;
        m_sftpWindow = nullptr;
    }

    m_sftpWindow = new SftpWindow(params, remotePath, this);
    m_sftpWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    m_sftpWindow->show();
}

void MainWindow::uploadClipboardImage(SessionTabInfo *s) {
    if (!s || !s->worker || !s->worker->isConnected() || !s->terminal) {
        return;
    }

    const QClipboard *clipboard = QApplication::clipboard();
    QImage img = clipboard->image();
    if (img.isNull()) return;

    // Save image to temporary local PNG file
    QTemporaryFile tempFile(QDir::tempPath() + "/retro_ssh_clip_XXXXXX.png");
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) return;
    QString localPath = tempFile.fileName();
    tempFile.close();

    if (!img.save(localPath, "PNG")) {
        QFile::remove(localPath);
        return;
    }

    // Prepare remote target directory and path (e.g. /tmp/retro_img_20260330_123456.png)
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString remoteFileName = QString("retro_img_%1.png").arg(timestamp);
    QString remotePath = "/tmp/" + remoteFileName;

    m_statusLabel->setText(QString("Uploading clipboard image to %1...").arg(remotePath));

    auto params = s->params;
    auto *worker = s->worker;

    QtConcurrent::run([this, params, localPath, remotePath, worker]() {
        bool uploadSuccess = false;
        QString errorMsg;

        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        QString portStr = QString::number(params.port);

        int sock = -1;
        LIBSSH2_SESSION *session = nullptr;
        LIBSSH2_SFTP *sftp = nullptr;

        if (getaddrinfo(params.host.toUtf8().constData(), portStr.toUtf8().constData(), &hints, &res) == 0 && res) {
            sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sock >= 0) {
                struct timeval tv{5, 0};
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

                if (::connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
                    session = libssh2_session_init();
                    if (session && libssh2_session_handshake(session, sock) == 0) {
                        QByteArray userBytes = params.username.toUtf8();
                        QByteArray passBytes = params.password.toUtf8();

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
                                session,
                                userBytes.constData(),
                                QFile::exists(pubKey) ? pubKey.toUtf8().constData() : nullptr,
                                privKey.toUtf8().constData(),
                                passBytes.isEmpty() ? nullptr : passBytes.constData()
                            ) == 0);
                        }

                        if (!authOk && !passBytes.isEmpty()) {
                            authOk = (libssh2_userauth_password(session, userBytes.constData(), passBytes.constData()) == 0);
                        }

                        if (authOk) {
                            sftp = libssh2_sftp_init(session);
                            if (sftp) {
                                QFile localF(localPath);
                                if (localF.open(QIODevice::ReadOnly)) {
                                    LIBSSH2_SFTP_HANDLE *h = libssh2_sftp_open(
                                        sftp,
                                        remotePath.toUtf8().constData(),
                                        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH
                                    );

                                    if (h) {
                                        char buf[16384];
                                        bool writeOk = true;
                                        while (!localF.atEnd()) {
                                            qint64 r = localF.read(buf, sizeof(buf));
                                            if (r <= 0) break;
                                            char *p = buf;
                                            qint64 rem = r;
                                            while (rem > 0) {
                                                ssize_t w = libssh2_sftp_write(h, p, rem);
                                                if (w < 0) { writeOk = false; break; }
                                                p += w;
                                                rem -= w;
                                            }
                                            if (!writeOk) break;
                                        }
                                        libssh2_sftp_close(h);
                                        uploadSuccess = writeOk;
                                    } else {
                                        errorMsg = "Failed to create remote file";
                                    }
                                    localF.close();
                                } else {
                                    errorMsg = "Cannot read local file";
                                }
                                libssh2_sftp_shutdown(sftp);
                            } else {
                                errorMsg = "Failed to init SFTP session";
                            }
                        } else {
                            errorMsg = "SFTP auth failed";
                        }
                    } else {
                        errorMsg = "SFTP handshake failed";
                    }
                    if (session) libssh2_session_free(session);
                } else {
                    errorMsg = "SFTP connect failed";
                }
                ::close(sock);
            }
            freeaddrinfo(res);
        }

        QFile::remove(localPath);

        QMetaObject::invokeMethod(this, [this, uploadSuccess, remotePath, errorMsg, worker]() {
            if (uploadSuccess) {
                m_statusLabel->setText(QString("Image uploaded: %1").arg(remotePath));
                if (worker) {
                    worker->sendData(remotePath.toUtf8());
                }
            } else {
                m_statusLabel->setText(QString("Image upload failed: %1").arg(errorMsg));
            }
        });
    });
}
