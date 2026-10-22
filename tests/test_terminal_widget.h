#pragma once

#include <QObject>
#include <QTest>

class TestTerminalWidget : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testFontAndAppearanceProperties();
    void testBasicTextOutput();
    void testLineFeedsAndCarriageReturns();
    void testBackspaceAndTab();
    void testAnsiColorEscapeSequences();
    void test256AndTrueColorSequences();
    void testCursorMovementSequences();
    void testScreenClearingSequences();
    void testOscWindowAndPaletteSequences();
    void testAlternateScreenBuffer();
    void testTextSelectionAndClipboard();
    void testFindTextFunctionality();
    void testScrollAndResize();
    void testKeyboardAndBannerInteractions();
};
