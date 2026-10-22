#pragma once

#include "backend/SshSessionWorker.h"
#include "ui/ProfileSettingsDialog.h"
#include "ui/QuickConnectDialog.h"
#include "ui/SftpWindow.h"
#include "ui/TerminalWidget.h"
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>

struct SessionTabInfo {
    QString title;
    TerminalWidget *terminal = nullptr;
    SshSessionWorker *worker = nullptr;
    SshConnectionParams params;
    QWidget *containerWidget = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onQuickConnectTriggered();
    void onConnectTriggered();
    void onDisconnectTriggered();
    void onSaveSettingsTriggered();
    void onSettingsTriggered();
    void onSftpTriggered();
    void onAddProfileTriggered();
    void onEditProfilesTriggered();
    void rebuildProfilesMenu();

    void onCurrentTabChanged(int index);
    void onTabCloseRequested(int index);

private:
    void setupMenus();
    void setupToolBars();
    void setupStatusBar();

    int createNewSessionTab(const QString &title, const SshConnectionParams &params);
    void connectSession(int tabIndex);
    void showDefaultStartingPage();
    void uploadClipboardImage(SessionTabInfo *s);
    SessionTabInfo *currentSession();

    QToolBar *m_tier1 = nullptr;
    QToolBar *m_tier2 = nullptr;

    QTabBar *m_profileTabBar = nullptr;
    QStackedWidget *m_sessionStack = nullptr;
    QVector<SessionTabInfo> m_sessions;

    SshConnectionParams m_currentParams;
    SftpWindow *m_sftpWindow = nullptr;

    QLabel *m_statusLabel = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_dimensionsLabel = nullptr;
    QLabel *m_iconLabel = nullptr;

    QAction *m_connectAct = nullptr;
    QAction *m_disconnectAct = nullptr;
    QAction *m_sftpAct = nullptr;
    QMenu *m_profilesMenu = nullptr;
};
