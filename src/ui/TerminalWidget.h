#pragma once

#include <QWidget>
#include <QTextCodec>
#include <QTimer>
#include <QScrollBar>
#include <QColor>
#include <QVector>

struct AnsiChar {
    QChar ch;
    QColor fg = QColor(0x1F, 0x23, 0x28);
    QColor bg = QColor(0xFA, 0xFB, 0xFC);
    bool bold = false;
    bool underline = false;
    bool isWideContinuation = false; // Second half of a 2-column wide character (e.g. CJK)
};

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget();

    void bindScrollBar(QScrollBar *scrollBar);
    void setTerminalFont(const QString &family, int pointSize);
    void setMaxScrollback(int lines);
    void setScrollOnOutput(bool enable) { m_scrollOnOutput = enable; }
    void setPasteOnRightClick(bool enable) { m_pasteOnRightClick = enable; }

    void setTerminalColors(const QColor &fg, const QColor &bg, const QColor &sel,
                           const QColor &cursor, bool enableAnsi, bool reverseVideo);

    QString fontFamily() const { return m_fontFamily; }
    int fontSize() const { return m_fontSize; }
    int maxScrollback() const { return m_maxScrollback; }
    bool scrollOnOutput() const { return m_scrollOnOutput; }
    bool pasteOnRightClick() const { return m_pasteOnRightClick; }

    QColor defaultFg() const { return m_defaultFg; }
    QColor defaultBg() const { return m_defaultBg; }
    QColor selectionColor() const { return m_selectionColor; }
    QColor cursorColor() const { return m_cursorColor; }
    bool enableAnsiColors() const { return m_enableAnsiColors; }
    bool reverseVideo() const { return m_reverseVideo; }

    bool hasSelection() const { return m_selectStart != m_selectEnd && m_selectStart.x() >= 0; }
    bool findText(const QString &text, bool matchCase = false, bool matchWord = false, bool isRegex = false, bool searchUp = true);

    void setEncoding(const QString &encodingName);
    void clearScreen(bool keepBanner = false);
    void resetTerminal();
    void resetToDefaultBanner();
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }

public slots:
    void appendRawData(const QByteArray &data);
    void copySelection();
    void pasteClipboard();
    void pasteSelection();
    void selectAll();
    void selectScreen();
    void selectNone();
    void showFindDialog();
    void scrollUp(int lines);
    void scrollDown(int lines);
    void scrollToBottom();
    void onScrollBarMoved(int value);
    QString lastWorkingDir() const { return m_lastWorkingDir; }

signals:
    void dataToSend(const QByteArray &data);
    void terminalResized(int cols, int rows);
    void windowTitleChanged(const QString &title);
    void newTerminalRequested();
    void newFileTransferRequested();
    void closeWindowRequested();
    void settingsRequested();
    void uploadClipboardImageRequested();
    void quickConnectRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:
    void parseAnsi(const QString &text);
    void handleEscapeSequence(const QString &seq);
    void newLine();
    void updateDimensions();
    void updateScrollRange();
    static bool isWideChar(QChar ch);
    static QColor colorFrom256Index(int idx);

    int m_cols = 80;
    int m_rows = 24;
    int m_cursorCol = 0;
    int m_cursorRow = 0;
    bool m_cursorVisible = true;
    QTimer *m_cursorTimer = nullptr;

    int m_charWidth = 8;
    int m_charHeight = 15;
    QString m_fontFamily = "Courier New";
    int m_fontSize = 10;
    QFont m_terminalFont;

    // Scrollback buffer and view offset
    int m_maxScrollback = 1000;
    int m_scrollOffset = 0;
    int m_wheelAccumulator = 0;
    bool m_scrollOnOutput = true;
    bool m_pasteOnRightClick = false;

    // Configurable terminal colors (from Settings -> Colors)
    QColor m_defaultFg = QColor(0x1F, 0x23, 0x28);
    QColor m_defaultBg = QColor(0xFA, 0xFB, 0xFC);
    QColor m_selectionColor = QColor(0xB6, 0xD7, 0xFF); // Soft sky-blue selection
    QColor m_cursorColor = QColor(0x09, 0x69, 0xDA);    // Tech blue solid block cursor
    bool m_enableAnsiColors = true;
    bool m_reverseVideo = false;

    // Viewport padding for modern breathing room
    int m_paddingX = 14;
    int m_paddingY = 14;

    bool m_showingBanner = false;

    QScrollBar *m_scrollBar = nullptr;

    QVector<QVector<AnsiChar>> m_buffer;
    QColor m_currentFg = QColor(0x1F, 0x23, 0x28);
    QColor m_currentBg = QColor(0xFA, 0xFB, 0xFC);
    bool m_currentBold = false;
    bool m_currentUnderline = false;

    QTextCodec *m_codec = nullptr;
    QTextDecoder *m_decoder = nullptr;
    QString m_ansiBuffer;
    bool m_inEscape = false;
    bool m_inOsc = false;
    QString m_oscBuffer;
    QString m_lastWorkingDir;

    // Selection
    QPoint m_selectStart{-1, -1};
    QPoint m_selectEnd{-1, -1};

    // Find state
    class FindDialog *m_findDialog = nullptr;
    int m_lastFoundRow = -1;
    int m_lastFoundCol = -1;

    // Mouse tracking and Alternate Screen (DECSET/DECRST)
    bool m_mouseTracking = false;   // Mode 1000 / 1002
    bool m_sgrMouseMode = false;    // Mode 1006
    bool m_altScreenActive = false; // Mode 1049 / 47
    QVector<QVector<AnsiChar>> m_primaryBuffer;
    int m_primaryCursorRow = 0;
    int m_primaryCursorCol = 0;
};
