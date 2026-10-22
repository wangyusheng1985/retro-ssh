#include "test_ui_dialogs.h"
#include "backend/ProfileManager.h"
#include "ui/MainWindow.h"
#include "ui/ProfileSettingsDialog.h"
#include "ui/QuickConnectDialog.h"
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>

void TestUiDialogs::testProfileSettingsDialogCreationAndPages() {
    ProfileSettingsDialog dlg;
    QVERIFY(dlg.findChild<QListWidget *>() != nullptr);

    ProfileData p;
    p.name = "DialogTestProfile";
    p.params.host = "10.0.0.2";
    p.params.port = 22;
    p.params.username = "user";
    dlg.setProfileData(p);

    ProfileData out = dlg.getProfileData();
    QCOMPARE(out.name, QString("DialogTestProfile"));
    QCOMPARE(out.params.host, QString("10.0.0.2"));
    QCOMPARE(out.params.port, 22);
}

void TestUiDialogs::testProfileSettingsDialogLoadAndApply() {
    ProfileData p;
    p.name = "CustomColorsProfile";
    p.fgColor = "Green";
    p.bgColor = "Black";
    p.fontFamily = "Menlo";
    p.fontSize = 13;

    ProfileSettingsDialog dlg;
    dlg.setProfileData(p);

    ProfileData out = dlg.getProfileData();
    QCOMPARE(out.name, QString("CustomColorsProfile"));
    QCOMPARE(out.fgColor, QString("Green"));
    QCOMPARE(out.bgColor, QString("Black"));
    QCOMPARE(dlg.getSelectedFontFamily(), QString("Menlo"));
    QCOMPARE(dlg.getSelectedFontSize(), 13);
}

void TestUiDialogs::testQuickConnectDialogInputs() {
    QuickConnectDialog qc;
    SshConnectionParams params;
    params.host = "test.retro-ssh.org";
    params.username = "admin";
    params.port = 2222;
    params.authMethod = "Password";
    qc.setParams(params);

    SshConnectionParams res = qc.getParams();
    QCOMPARE(res.host, QString("test.retro-ssh.org"));
    QCOMPARE(res.username, QString("admin"));
    QCOMPARE(res.port, 2222);
    QCOMPARE(res.authMethod, QString("Password"));
}

void TestUiDialogs::testMainWindowCreationAndActions() {
    MainWindow mw;
    QVERIFY(mw.findChild<TerminalWidget *>() != nullptr);
}
