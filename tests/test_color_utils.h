#pragma once

#include <QObject>
#include <QTest>

class TestColorUtils : public QObject {
    Q_OBJECT

private slots:
    void testColorNameToQColorNamedColors();
    void testColorNameToQColorHexCodes();
    void testColorNameToQColorFallbacks();
    void testQColorToColorName();
};
