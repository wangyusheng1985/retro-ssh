#include "test_terminal_widget.h"
#include "ui/TerminalWidget.h"
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>

void TestTerminalWidget::testInitialState() {
    TerminalWidget term;
    QVERIFY(term.cols() > 0);
    QVERIFY(term.rows() > 0);
    QVERIFY(!term.fontFamily().isEmpty());
    QVERIFY(term.fontSize() > 0);
    QVERIFY(term.defaultFg().isValid());
    QVERIFY(term.defaultBg().isValid());
}

void TestTerminalWidget::testFontAndAppearanceProperties() {
    TerminalWidget term;
    term.setTerminalFont("Menlo", 14);
    QCOMPARE(term.fontFamily(), QString("Menlo"));
    QCOMPARE(term.fontSize(), 14);

    term.setTerminalColors(Qt::green, Qt::black, Qt::yellow, Qt::red, false, true);
    QCOMPARE(term.defaultFg(), QColor(Qt::green));
    QCOMPARE(term.defaultBg(), QColor(Qt::black));
    QCOMPARE(term.selectionColor(), QColor(Qt::yellow));
    QCOMPARE(term.cursorColor(), QColor(Qt::red));
    QCOMPARE(term.enableAnsiColors(), false);
    QCOMPARE(term.reverseVideo(), true);

    term.setMaxScrollback(2000);
    QCOMPARE(term.maxScrollback(), 2000);

    term.setScrollOnOutput(false);
    QCOMPARE(term.scrollOnOutput(), false);

    term.setPasteOnRightClick(true);
    QCOMPARE(term.pasteOnRightClick(), true);

    term.setEncoding("GBK");
}

void TestTerminalWidget::testBasicTextOutput() {
    TerminalWidget term;
    term.clearScreen();
    term.appendRawData("Hello RetroSSH World");
    QVERIFY(term.findText("Hello RetroSSH World"));
}

void TestTerminalWidget::testLineFeedsAndCarriageReturns() {
    TerminalWidget term;
    term.clearScreen();
    term.appendRawData("Line 1\r\nLine 2\nLine 3\r");
    QVERIFY(term.findText("Line 1"));
    QVERIFY(term.findText("Line 2"));
    QVERIFY(term.findText("Line 3"));
}

void TestTerminalWidget::testBackspaceAndTab() {
    TerminalWidget term;
    term.clearScreen();
    // Tab
    term.appendRawData("A\tB\r\n");
    // Backspace: overwrite character
    term.appendRawData("ABC\b\bXY");
    QVERIFY(term.findText("A\tB") || term.findText("B"));
    QVERIFY(term.findText("AXY"));
}

void TestTerminalWidget::testAnsiColorEscapeSequences() {
    TerminalWidget term;
    term.setTerminalColors(Qt::black, Qt::white, Qt::blue, Qt::red, true, false);
    term.clearScreen();

    // Standard 16 colors: 31=Red, 32=Green, 41=Bg Red, 1=Bold, 4=Underline
    term.appendRawData("\033[31;1mBoldRed\033[0mNormal\033[32;4mUnderlineGreen\033[0m\r\n");
    QVERIFY(term.findText("BoldRed"));
    QVERIFY(term.findText("Normal"));
    QVERIFY(term.findText("UnderlineGreen"));
}

void TestTerminalWidget::test256AndTrueColorSequences() {
    TerminalWidget term;
    term.clearScreen();

    // 256 Color foreground: 38;5;196m
    // 256 Color background: 48;5;22m
    term.appendRawData("\033[38;5;196mColor256\033[0m\r\n");
    QVERIFY(term.findText("Color256"));

    // 24-bit TrueColor foreground: 38;2;12;34;56m
    // 24-bit TrueColor background: 48;2;78;90;12m
    term.appendRawData("\033[38;2;12;34;56;48;2;78;90;12mTrueColorText\033[0m\r\n");
    QVERIFY(term.findText("TrueColorText"));
}

void TestTerminalWidget::testCursorMovementSequences() {
    TerminalWidget term;
    term.clearScreen();

    // Home, Pos, Up, Down, Forward, Back
    term.appendRawData("\033[1;1HStart");
    term.appendRawData("\033[5;10HPos5_10");
    term.appendRawData("\033[2AUp2\033[3BDown3");
    term.appendRawData("\033[4CForward4\033[2DBack2");

    QVERIFY(term.findText("Pos5_10"));
    QVERIFY(term.findText("Start"));
}

void TestTerminalWidget::testScreenClearingSequences() {
    TerminalWidget term;
    term.appendRawData("Before Clear");
    QVERIFY(term.findText("Before Clear"));

    // 2J = Clear entire screen
    term.appendRawData("\033[2J");
    // Clear line: 0K, 1K, 2K
    term.appendRawData("LineText\033[2K");
    term.appendRawData("NewText");
    QVERIFY(term.findText("NewText"));
}

void TestTerminalWidget::testOscWindowAndPaletteSequences() {
    TerminalWidget term;
    bool titleReceived = false;
    QString newTitle;
    QObject::connect(&term, &TerminalWidget::windowTitleChanged, [&](const QString &title) {
        titleReceived = true;
        newTitle = title;
    });

    // OSC 0 / OSC 2: Set window title and current working directory
    term.appendRawData("\033]0;root@server: /home/developer/retro-ssh\007");
    QVERIFY(titleReceived);
    QCOMPARE(newTitle, QString("root@server: /home/developer/retro-ssh"));
    QCOMPARE(term.lastWorkingDir(), QString("/home/developer/retro-ssh"));

    // OSC 10 / OSC 11: Palette queries (foreground / background)
    bool replySent = false;
    QByteArray sentPayload;
    QObject::connect(&term, &TerminalWidget::dataToSend, [&](const QByteArray &data) {
        replySent = true;
        sentPayload = data;
    });

    term.appendRawData("\033]10;?\007");
    QVERIFY(replySent);
    QVERIFY(sentPayload.contains("\033]10;rgb:"));

    replySent = false;
    term.appendRawData("\033]11;?\007");
    QVERIFY(replySent);
    QVERIFY(sentPayload.contains("\033]11;rgb:"));
}

void TestTerminalWidget::testAlternateScreenBuffer() {
    TerminalWidget term;
    term.clearScreen();
    term.appendRawData("Primary Buffer Content\r\n");
    QVERIFY(term.findText("Primary Buffer Content"));

    // Switch to Alternate Screen Buffer
    term.appendRawData("\033[?1049h");
    term.appendRawData("Alt Buffer Content\r\n");
    QVERIFY(term.findText("Alt Buffer Content"));

    // Switch back to Primary Screen Buffer
    term.appendRawData("\033[?1049l");
    QVERIFY(term.findText("Primary Buffer Content"));
}

void TestTerminalWidget::testTextSelectionAndClipboard() {
    TerminalWidget term;
    term.clearScreen();
    term.appendRawData("Selectable Text Sample\r\n");

    QVERIFY(!term.hasSelection());

    term.selectAll();
    QVERIFY(term.hasSelection());

    term.copySelection();
    QCOMPARE(QApplication::clipboard()->text().trimmed(), QString("Selectable Text Sample"));

    term.selectNone();
    QVERIFY(!term.hasSelection());

    term.selectScreen();
    QVERIFY(term.hasSelection());
    term.selectNone();
}

void TestTerminalWidget::testFindTextFunctionality() {
    TerminalWidget term;
    term.clearScreen();
    term.appendRawData("Alpha beta Gamma delta\r\n");

    // Case insensitive
    QVERIFY(term.findText("alpha", false));
    // Case sensitive
    QVERIFY(!term.findText("alpha", true));
    QVERIFY(term.findText("Alpha", true));

    // Regex
    QVERIFY(term.findText("G[a-z]+a", false, false, true));

    // Non-existent text
    QVERIFY(!term.findText("NotHere12345"));
}

void TestTerminalWidget::testScrollAndResize() {
    TerminalWidget term;
    QScrollBar sb;
    term.bindScrollBar(&sb);

    for (int i = 0; i < 50; ++i) {
        term.appendRawData(QString("Line %1\r\n").arg(i).toUtf8());
    }

    term.scrollUp(5);
    term.scrollDown(5);
    term.scrollToBottom();
    term.onScrollBarMoved(sb.value());

    term.resize(800, 600);
    QVERIFY(term.cols() > 0);
    QVERIFY(term.rows() > 0);
}

void TestTerminalWidget::testKeyboardAndBannerInteractions() {
    TerminalWidget term;
    term.resetToDefaultBanner();

    bool enterPressed = false;
    QObject::connect(&term, &TerminalWidget::dataToSend, [&](const QByteArray &data) {
        if (data == "\r" || data == "\n" || data == " ")
            enterPressed = true;
    });

    // Simulate Return key on banner
    QKeyEvent returnKey(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(&term, &returnKey);

    // Simulate key input
    QKeyEvent charKey(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "a");
    QApplication::sendEvent(&term, &charKey);

    // Reset Terminal
    term.resetTerminal();
}
