#pragma once

#include <QObject>
#include <QTest>

class TestProfileManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testGlobalAppearanceDefaults();
    void testSaveAndLoadGlobalAppearance();
    void testFontFamilyPrefixStripping();
    void testProfileCRUD();
    void testGlobalAppearanceSyncsToProfiles();
};
