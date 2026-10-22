#include "test_profile_manager.h"
#include "backend/ProfileManager.h"
#include <QSettings>

void TestProfileManager::initTestCase() {
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

void TestProfileManager::cleanupTestCase() {
    QSettings s1("RetroSSH", "Settings");
    s1.clear();
    QSettings s2("SSH Secure Shell", "Profiles");
    s2.clear();
}

void TestProfileManager::testGlobalAppearanceDefaults() {
    auto &pm = ProfileManager::instance();
    pm.loadGlobalAppearance();
    auto app = pm.getGlobalAppearance();

    QVERIFY(!app.fontFamily.isEmpty());
    QVERIFY(app.fontSize > 0);
    QVERIFY(app.scrollbackBuffer >= 100);
}

void TestProfileManager::testSaveAndLoadGlobalAppearance() {
    auto &pm = ProfileManager::instance();

    GlobalAppearanceSettings g;
    g.fontFamily = "Courier New";
    g.fontSize = 14;
    g.scrollbackBuffer = 5000;
    g.scrollOnOutput = false;
    g.pasteOnRightClick = true;
    g.fgColor = "White";
    g.bgColor = "Black";
    g.selColor = "Blue";
    g.cursorColor = "Red";
    g.enableAnsiColors = false;
    g.reverseVideo = true;

    pm.saveGlobalAppearance(g);

    pm.loadGlobalAppearance();
    auto loaded = pm.getGlobalAppearance();

    QCOMPARE(loaded.fontFamily, QString("Courier New"));
    QCOMPARE(loaded.fontSize, 14);
    QCOMPARE(loaded.scrollbackBuffer, 5000);
    QCOMPARE(loaded.scrollOnOutput, false);
    QCOMPARE(loaded.pasteOnRightClick, true);
    QCOMPARE(loaded.fgColor, QString("White"));
    QCOMPARE(loaded.bgColor, QString("Black"));
    QCOMPARE(loaded.selColor, QString("Blue"));
    QCOMPARE(loaded.cursorColor, QString("Red"));
    QCOMPARE(loaded.enableAnsiColors, false);
    QCOMPARE(loaded.reverseVideo, true);
}

void TestProfileManager::testFontFamilyPrefixStripping() {
    auto &pm = ProfileManager::instance();
    GlobalAppearanceSettings g;
    g.fontFamily = "TT  Monaco";
    g.fontSize = 12;

    pm.saveGlobalAppearance(g);
    auto loaded = pm.getGlobalAppearance();
    QCOMPARE(loaded.fontFamily, QString("Monaco"));
}

void TestProfileManager::testProfileCRUD() {
    auto &pm = ProfileManager::instance();

    ProfileData p;
    p.name = "TestHost";
    p.params.host = "192.168.1.100";
    p.params.port = 2222;
    p.params.username = "root";
    p.params.authMethod = "Password";
    p.keepAlive = true;

    pm.addOrUpdateProfile(p);
    QVERIFY(pm.hasProfile("TestHost"));

    auto fetched = pm.getProfile("TestHost");
    QCOMPARE(fetched.name, QString("TestHost"));
    QCOMPARE(fetched.params.host, QString("192.168.1.100"));
    QCOMPARE(fetched.params.port, 2222);
    QCOMPARE(fetched.params.username, QString("root"));
    QCOMPARE(fetched.keepAlive, true);

    QVERIFY(pm.getProfiles().keys().contains("TestHost"));

    pm.removeProfile("TestHost");
    QVERIFY(!pm.hasProfile("TestHost"));
}

void TestProfileManager::testGlobalAppearanceSyncsToProfiles() {
    auto &pm = ProfileManager::instance();

    ProfileData p1;
    p1.name = "ServerA";
    p1.params.host = "10.0.0.1";
    p1.fontFamily = "OldFont";
    p1.fontSize = 9;
    pm.addOrUpdateProfile(p1);

    GlobalAppearanceSettings g;
    g.fontFamily = "Menlo";
    g.fontSize = 15;
    g.bgColor = "Black";
    g.fgColor = "White";
    pm.saveGlobalAppearance(g);

    auto updatedP1 = pm.getProfile("ServerA");
    QCOMPARE(updatedP1.fontFamily, QString("Menlo"));
    QCOMPARE(updatedP1.fontSize, 15);
    QCOMPARE(updatedP1.bgColor, QString("Black"));
    QCOMPARE(updatedP1.fgColor, QString("White"));

    pm.removeProfile("ServerA");
}
