#pragma once

#include <QObject>
#include <QTest>

class TestUiDialogs : public QObject {
    Q_OBJECT

private slots:
    void testProfileSettingsDialogCreationAndPages();
    void testProfileSettingsDialogLoadAndApply();
    void testQuickConnectDialogInputs();
    void testMainWindowCreationAndActions();
};
