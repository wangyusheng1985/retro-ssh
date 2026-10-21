#include "TerminalWidget.h"
#include "ui/FindDialog.h"
#include <QPainter>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QTextCodec>
#include <QTextDecoder>
#include <QDebug>

static const QColor ANSI_COLORS[8] = {
    QColor(0, 0, 0),        // 0: Black
    QColor(180, 0, 0),      // 1: Red
    QColor(0, 140, 0),      // 2: Green
    QColor(160, 130, 0),    // 3: Yellow
    QColor(0, 0, 180),      // 4: Blue
    QColor(160, 0, 160),    // 5: Magenta
    QColor(0, 140, 140),    // 6: Cyan
    QColor(240, 240, 240)   // 7: White
};

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_InputMethodEnabled, true);

    setTerminalFont("Courier New", 10);

    m_codec = QTextCodec::codecForName("UTF-8");
    if (m_codec) {
        m_decoder = m_codec->makeDecoder(QTextCodec::IgnoreHeader);
    }

    m_buffer.resize(m_rows);
    for (auto &row : m_buffer) {
        row.resize(m_cols);
    }

    resetToDefaultBanner();

    m_cursorTimer = new QTimer(this);
    connect(m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_cursorTimer->start(500);
}

TerminalWidget::~TerminalWidget() {
    delete m_decoder;
}

void TerminalWidget::bindScrollBar(QScrollBar *scrollBar) {
    m_scrollBar = scrollBar;
    if (m_scrollBar) {
        connect(m_scrollBar, &QScrollBar::valueChanged, this, &TerminalWidget::onScrollBarMoved);
        updateScrollRange();
    }
}

bool TerminalWidget::isWideChar(QChar ch) {
    ushort u = ch.unicode();
    return ((u >= 0x1100 && u <= 0x115F) ||
            (u >= 0x2E80 && u <= 0xA4CF && u != 0x303F) ||
            (u >= 0xAC00 && u <= 0xD7A3) ||
            (u >= 0xF900 && u <= 0xFAFF) ||
            (u >= 0xFE10 && u <= 0xFE19) ||
            (u >= 0xFE30 && u <= 0xFE6F) ||
            (u >= 0xFF00 && u <= 0xFF60) ||
            (u >= 0xFFE0 && u <= 0xFFE6));
}

void TerminalWidget::setTerminalFont(const QString &family, int pointSize) {
    m_fontFamily = family;
    m_fontSize = pointSize;

    QString actualFamily = family;
    if (actualFamily.startsWith("TT  ")) {
        actualFamily = actualFamily.mid(4);
    }

    QFont termFont(actualFamily, pointSize);
    termFont.setStyleHint(QFont::Monospace);
    termFont.setFixedPitch(true);
    termFont.setKerning(false);
    termFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
    setFont(termFont);

    QFontMetrics fm(termFont);
    m_charWidth = qMax(1, fm.averageCharWidth());
    m_charHeight = qMax(1, fm.lineSpacing());

    updateDimensions();
    update();
}

void TerminalWidget::setMaxScrollback(int lines) {
    m_maxScrollback = qMax(100, lines);
    if (m_buffer.size() > m_maxScrollback) {
        int toRemove = m_buffer.size() - m_maxScrollback;
        m_buffer.remove(0, toRemove);
        m_cursorRow = qMax(0, m_cursorRow - toRemove);
        updateScrollRange();
        update();
    }
}

void TerminalWidget::resetToDefaultBanner() {
    clearScreen();
    m_showingBanner = true;
    QString banner =
        "RetroSSH Client 1.0 (Build 2026)\n"
        "Native High-Performance SSH2 / SFTP Client for macOS\n\n"
        "Powered by modern OpenSSL 3.x & libssh2 Cryptography Engine.\n"
        "Authentic Clean-Room Win32 Desktop Experience.\n\n"
        "Press Enter or Space to Quick Connect, or choose a Profile from the toolbar.\n\n";

    m_currentFg = QColor(60, 60, 60);
    m_currentBg = Qt::white;
    parseAnsi(banner);
    m_currentFg = Qt::black;
}

void TerminalWidget::setTerminalColors(const QColor &fg, const QColor &bg, const QColor &sel,
                                       const QColor &cursor, bool enableAnsi, bool reverseVideo) {
    m_defaultFg = fg;
    m_defaultBg = bg;
    m_selectionColor = sel;
    m_cursorColor = cursor;
    m_enableAnsiColors = enableAnsi;
    m_reverseVideo = reverseVideo;

    m_currentFg = m_reverseVideo ? m_defaultBg : m_defaultFg;
    m_currentBg = m_reverseVideo ? m_defaultFg : m_defaultBg;

    update();
}

void TerminalWidget::resetTerminal() {
    clearScreen(false);
    emit dataToSend("\n");
}

void TerminalWidget::setEncoding(const QString &encodingName) {
    QTextCodec *c = QTextCodec::codecForName(encodingName.toUtf8());
    if (c) {
        m_codec = c;
        delete m_decoder;
        m_decoder = m_codec->makeDecoder(QTextCodec::IgnoreHeader);
    }
}

void TerminalWidget::clearScreen(bool keepBanner) {
    m_showingBanner = keepBanner;
    m_buffer.clear();
    m_buffer.resize(m_rows);
    for (auto &row : m_buffer) {
        row.resize(m_cols);
        for (auto &ch : row) {
            ch = AnsiChar();
            ch.fg = Qt::black;
            ch.bg = Qt::white;
        }
    }
    m_cursorCol = 0;
    m_cursorRow = 0;
    m_scrollOffset = 0;
    if (keepBanner) {
        resetToDefaultBanner();
    }
    updateScrollRange();
    update();
}

void TerminalWidget::appendRawData(const QByteArray &data) {
    m_showingBanner = false;
    QString text;
    if (m_decoder) {
        text = m_decoder->toUnicode(data.constData(), data.size());
    } else if (m_codec) {
        text = m_codec->toUnicode(data);
    } else {
        text = QString::fromUtf8(data);
    }

    parseAnsi(text);

    if (m_scrollOnOutput) {
        m_scrollOffset = 0; // Auto scroll to bottom
    }

    updateScrollRange();
    update();
}

void TerminalWidget::parseAnsi(const QString &text) {
    for (int i = 0; i < text.size(); ++i) {
        QChar ch = text[i];

        // OSC sequence handling: ESC ] ... BEL (0x07) or ST (ESC \)
        if (m_inOsc) {
            if (ch == 0x07) { // BEL terminates OSC
                m_inOsc = false;
                // e.g. "0;root@OPS-4722: ~" or "2;root@OPS-4722: /var/log"
                if (m_oscBuffer.startsWith("0;") || m_oscBuffer.startsWith("2;")) {
                    QString title = m_oscBuffer.mid(2);
                    emit windowTitleChanged(title);

                    // Extract working directory from user@host: <dir>
                    int colonIdx = title.lastIndexOf(':');
                    if (colonIdx >= 0) {
                        m_lastWorkingDir = title.mid(colonIdx + 1).trimmed();
                    }
                }
                m_oscBuffer.clear();
                continue;
            } else if (ch == 0x1B) {
                // Potential ST (ESC \)
                if (i + 1 < text.size() && text[i + 1] == '\\') {
                    i++;
                    m_inOsc = false;
                    m_oscBuffer.clear();
                    continue;
                }
            }
            m_oscBuffer.append(ch);
            continue;
        }

        if (m_inEscape) {
            if (ch == ']') {
                m_inEscape = false;
                m_inOsc = true;
                m_oscBuffer.clear();
                continue;
            }

            m_ansiBuffer.append(ch);
            if (ch.isLetter() || ch == '~') {
                handleEscapeSequence(m_ansiBuffer);
                m_ansiBuffer.clear();
                m_inEscape = false;
            }
            continue;
        }

        if (ch == 0x1B) { // ESC
            m_inEscape = true;
            m_ansiBuffer.clear();
            continue;
        }

        if (ch == '\r') {
            m_cursorCol = 0;
        } else if (ch == '\n') {
            newLine();
        } else if (ch == '\b') {
            if (m_cursorCol > 0) {
                m_cursorCol--;
                if (m_cursorRow < m_buffer.size() && m_cursorCol < m_buffer[m_cursorRow].size()) {
                    AnsiChar &ac = m_buffer[m_cursorRow][m_cursorCol];
                    ac.ch = ' ';
                    ac.fg = m_currentFg;
                    ac.bg = m_currentBg;
                    ac.isWideContinuation = false;
                }
            }
        } else if (ch == '\t') {
            m_cursorCol = (m_cursorCol + 8) & ~7;
            if (m_cursorCol >= m_cols) newLine();
        } else if (ch == 0x07) {
            QApplication::beep();
        } else {
            if (m_cursorRow >= m_buffer.size()) {
                m_buffer.resize(m_cursorRow + 10);
            }
            if (m_cursorCol + 2 >= m_buffer[m_cursorRow].size()) {
                m_buffer[m_cursorRow].resize(m_cursorCol + 20);
            }

            bool wide = isWideChar(ch);

            // If wide character doesn't fit on this row, wrap to new line first
            if (wide && m_cursorCol >= m_cols - 1) {
                newLine();
                if (m_cursorRow >= m_buffer.size()) {
                    m_buffer.resize(m_cursorRow + 10);
                }
                if (m_cursorCol + 2 >= m_buffer[m_cursorRow].size()) {
                    m_buffer[m_cursorRow].resize(m_cursorCol + 20);
                }
            }

            AnsiChar &ac = m_buffer[m_cursorRow][m_cursorCol];
            ac.ch = ch;
            ac.fg = m_currentFg;
            ac.bg = m_currentBg;
            ac.bold = m_currentBold;
            ac.underline = m_currentUnderline;
            ac.isWideContinuation = false;

            m_cursorCol++;

            // If it is a wide (CJK) character, fill continuation placeholder cell
            if (wide && m_cursorCol < m_cols) {
                AnsiChar &cont = m_buffer[m_cursorRow][m_cursorCol];
                cont.ch = QChar();
                cont.fg = m_currentFg;
                cont.bg = m_currentBg;
                cont.bold = m_currentBold;
                cont.underline = m_currentUnderline;
                cont.isWideContinuation = true;
                m_cursorCol++;
            }

            if (m_cursorCol >= m_cols) {
                newLine();
            }
        }
    }
}

void TerminalWidget::handleEscapeSequence(const QString &seq) {
    if (seq.startsWith("[")) {
        QString cmd = seq.mid(1);
        QChar type = cmd.at(cmd.length() - 1);
        QString params = cmd.left(cmd.length() - 1);
        QStringList parts = params.split(';');

        if (type == 'm') {
            if (params.isEmpty() || params == "0") {
                m_currentFg = Qt::black;
                m_currentBg = Qt::white;
                m_currentBold = false;
                m_currentUnderline = false;
            } else {
                for (const QString &p : parts) {
                    int val = p.toInt();
                    if (val == 0) {
                        m_currentFg = Qt::black;
                        m_currentBg = Qt::white;
                    } else if (val == 1) {
                        m_currentBold = true;
                    } else if (val >= 30 && val <= 37) {
                        m_currentFg = ANSI_COLORS[val - 30];
                    } else if (val >= 40 && val <= 47) {
                        m_currentBg = ANSI_COLORS[val - 40];
                    } else if (val == 39) {
                        m_currentFg = Qt::black;
                    } else if (val == 49) {
                        m_currentBg = Qt::white;
                    }
                }
            }
        } else if (type == 'J') {
            if (params == "2") {
                clearScreen(false);
            }
        } else if (type == 'H' || type == 'f') {
            int r = parts.value(0).toInt();
            int c = parts.value(1).toInt();
            m_cursorRow = qMax(0, r - 1);
            m_cursorCol = qMax(0, c - 1);
        } else if (type == 'A') {
            m_cursorRow = qMax(0, m_cursorRow - qMax(1, params.toInt()));
        } else if (type == 'B') {
            m_cursorRow += qMax(1, params.toInt());
        } else if (type == 'C') {
            m_cursorCol = qMin(m_cols - 1, m_cursorCol + qMax(1, params.toInt()));
        } else if (type == 'D') {
            m_cursorCol = qMax(0, m_cursorCol - qMax(1, params.toInt()));
        } else if (type == 'K') { // Erase in line
            int mode = params.isEmpty() ? 0 : params.toInt();
            if (m_cursorRow < m_buffer.size()) {
                auto &row = m_buffer[m_cursorRow];
                if (mode == 0) { // Clear cursor to end of line
                    for (int c = m_cursorCol; c < m_cols && c < row.size(); ++c) {
                        row[c] = AnsiChar();
                        row[c].fg = m_currentFg;
                        row[c].bg = m_currentBg;
                    }
                } else if (mode == 1) { // Clear from start of line to cursor
                    for (int c = 0; c <= m_cursorCol && c < row.size(); ++c) {
                        row[c] = AnsiChar();
                        row[c].fg = m_currentFg;
                        row[c].bg = m_currentBg;
                    }
                } else if (mode == 2) { // Clear entire line
                    for (int c = 0; c < m_cols && c < row.size(); ++c) {
                        row[c] = AnsiChar();
                        row[c].fg = m_currentFg;
                        row[c].bg = m_currentBg;
                    }
                }
            }
        } else if (type == 'P') { // Delete characters at cursor
            int count = qMax(1, params.isEmpty() ? 1 : params.toInt());
            if (m_cursorRow < m_buffer.size()) {
                auto &row = m_buffer[m_cursorRow];
                for (int c = m_cursorCol; c < m_cols - count; ++c) {
                    if (c + count < row.size()) {
                        row[c] = row[c + count];
                    }
                }
                for (int c = qMax(m_cursorCol, m_cols - count); c < m_cols && c < row.size(); ++c) {
                    row[c] = AnsiChar();
                    row[c].fg = m_currentFg;
                    row[c].bg = m_currentBg;
                }
            }
        } else if (type == 'X') { // Erase characters at cursor
            int count = qMax(1, params.isEmpty() ? 1 : params.toInt());
            if (m_cursorRow < m_buffer.size()) {
                auto &row = m_buffer[m_cursorRow];
                for (int c = m_cursorCol; c < m_cursorCol + count && c < m_cols && c < row.size(); ++c) {
                    row[c] = AnsiChar();
                    row[c].fg = m_currentFg;
                    row[c].bg = m_currentBg;
                }
            }
        }
    }
}

void TerminalWidget::newLine() {
    m_cursorCol = 0;
    m_cursorRow++;
    if (m_cursorRow >= m_buffer.size()) {
        m_buffer.append(QVector<AnsiChar>(m_cols));
    }

    // Prune buffer if exceeding max scrollback
    if (m_buffer.size() > m_maxScrollback + m_rows) {
        int excess = m_buffer.size() - (m_maxScrollback + m_rows);
        m_buffer.remove(0, excess);
        m_cursorRow = qMax(0, m_cursorRow - excess);
    }
}

void TerminalWidget::scrollUp(int lines) {
    int maxOffset = qMax(0, m_cursorRow - m_rows + 1);
    m_scrollOffset = qMin(maxOffset, m_scrollOffset + lines);
    updateScrollRange();
    update();
}

void TerminalWidget::scrollDown(int lines) {
    m_scrollOffset = qMax(0, m_scrollOffset - lines);
    updateScrollRange();
    update();
}

void TerminalWidget::scrollToBottom() {
    m_scrollOffset = 0;
    updateScrollRange();
    update();
}

void TerminalWidget::wheelEvent(QWheelEvent *event) {
    int deltaY = 0;

    // Support both macOS precision trackpad (pixelDelta) and classic mouse wheel (angleDelta)
    if (!event->pixelDelta().isNull()) {
        deltaY = event->pixelDelta().y();
    } else {
        // angleDelta: 120 units per notch. Scale to a smooth ~3 lines per notch
        deltaY = (event->angleDelta().y() * m_charHeight * 3) / 120;
    }

    m_wheelAccumulator += deltaY;

    // Step by single line height for maximum responsiveness and smoothness
    int stepHeight = qMax(4, m_charHeight);
    int lines = m_wheelAccumulator / stepHeight;

    if (lines != 0) {
        m_wheelAccumulator -= lines * stepHeight;
        if (lines > 0) {
            scrollUp(lines);
        } else {
            scrollDown(-lines);
        }
    }

    event->accept();
}

void TerminalWidget::updateScrollRange() {
    if (!m_scrollBar) return;

    int totalLines = qMax(m_buffer.size(), m_cursorRow + 1);
    int maxScroll = qMax(0, totalLines - m_rows);

    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, maxScroll);
    m_scrollBar->setPageStep(m_rows);
    m_scrollBar->setValue(maxScroll - m_scrollOffset);
    m_scrollBar->blockSignals(false);
}

void TerminalWidget::onScrollBarMoved(int value) {
    if (!m_scrollBar) return;
    int maxScroll = m_scrollBar->maximum();
    m_scrollOffset = qMax(0, maxScroll - value);
    update();
}

void TerminalWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    QColor baseBg = m_reverseVideo ? m_defaultFg : m_defaultBg;
    QColor baseFg = m_reverseVideo ? m_defaultBg : m_defaultFg;

    p.fillRect(rect(), baseBg);
    p.setFont(font());
    QFontMetrics fm(font());
    int ascent = fm.ascent();

    int totalLines = qMax(m_buffer.size(), m_cursorRow + 1);
    int viewBottom = qMax(m_rows - 1, (totalLines - 1) - m_scrollOffset);
    int startRow = qMax(0, viewBottom - m_rows + 1);
    int endRow = qMin(totalLines, startRow + m_rows);

    for (int r = startRow; r < endRow; ++r) {
        int y = (r - startRow) * m_charHeight + ascent;
        int rowTopY = (r - startRow) * m_charHeight;

        const QVector<AnsiChar> *rowPtr = (r < m_buffer.size()) ? &m_buffer[r] : nullptr;

        // Pass 1: Draw cell backgrounds (normal ANSI background and solid continuous selection highlight)
        for (int c = 0; c < m_cols; ++c) {
            int x = c * m_charWidth;

            bool isCellSelected = false;
            if (m_selectStart != m_selectEnd) {
                QPoint p1 = m_selectStart;
                QPoint p2 = m_selectEnd;
                if (p1.y() > p2.y() || (p1.y() == p2.y() && p1.x() > p2.x())) {
                    std::swap(p1, p2);
                }

                if (r > p1.y() && r < p2.y()) {
                    isCellSelected = true;
                } else if (r == p1.y() && r == p2.y()) {
                    isCellSelected = (c >= p1.x() && c < p2.x());
                } else if (r == p1.y()) {
                    isCellSelected = (c >= p1.x());
                } else if (r == p2.y()) {
                    isCellSelected = (c < p2.x());
                }
            }

            const AnsiChar *cellAc = (rowPtr && c < rowPtr->size()) ? &(*rowPtr)[c] : nullptr;
            QColor bg = cellAc ? cellAc->bg : baseBg;

            if (isCellSelected) {
                p.fillRect(x, rowTopY, m_charWidth, m_charHeight, m_selectionColor);
            } else if (m_enableAnsiColors && bg != baseBg) {
                p.fillRect(x, rowTopY, m_charWidth, m_charHeight, bg);
            }
        }

        // Pass 2: Draw text characters on top of background
        for (int c = 0; c < m_cols; ++c) {
            int x = c * m_charWidth;

            const AnsiChar *cellAc = (rowPtr && c < rowPtr->size()) ? &(*rowPtr)[c] : nullptr;
            if (!cellAc || cellAc->isWideContinuation || cellAc->ch.isNull() || cellAc->ch == ' ') {
                continue;
            }

            QChar ch = cellAc->ch;
            QColor fg = cellAc->fg;
            bool wide = isWideChar(ch);

            // Determine if the character is selected
            bool isCellSelected = false;
            if (m_selectStart != m_selectEnd) {
                QPoint p1 = m_selectStart;
                QPoint p2 = m_selectEnd;
                if (p1.y() > p2.y() || (p1.y() == p2.y() && p1.x() > p2.x())) {
                    std::swap(p1, p2);
                }

                if (r > p1.y() && r < p2.y()) {
                    isCellSelected = true;
                } else if (r == p1.y() && r == p2.y()) {
                    isCellSelected = (c >= p1.x() && c < p2.x());
                } else if (r == p1.y()) {
                    isCellSelected = (c >= p1.x());
                } else if (r == p2.y()) {
                    isCellSelected = (c < p2.x());
                }
            }

            p.setPen(isCellSelected ? Qt::black : (m_enableAnsiColors ? fg : baseFg));
            if (wide) {
                QRect targetRect(x, rowTopY, m_charWidth * 2, m_charHeight);
                p.drawText(targetRect, Qt::AlignCenter, QString(ch));
            } else {
                p.drawText(x, y, QString(ch));
            }
        }
    }

    // Classic solid vertical cursor block (with configured cursor color)
    if (m_cursorVisible && m_scrollOffset == 0) {
        int cx = m_cursorCol * m_charWidth;
        int cy = (m_cursorRow - startRow) * m_charHeight + 1;
        p.fillRect(cx, cy, 7, m_charHeight - 2, m_cursorColor);
    }
}

void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    // Ignore pure modifier keys (Fn / CapsLock / Shift / Control / Meta / Alt) to allow macOS system shortcuts (like Fn input source switch)
    if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control ||
        event->key() == Qt::Key_Meta || event->key() == Qt::Key_Alt ||
        event->key() == Qt::Key_AltGr || event->key() == Qt::Key_CapsLock ||
        event->key() == Qt::Key_Mode_switch || event->key() == Qt::Key_NumLock ||
        event->key() == 0) {
        event->ignore();
        return;
    }

    // PageUp / PageDown for scrolling history
    if (event->key() == Qt::Key_PageUp) {
        scrollUp(m_rows - 1);
        return;
    } else if (event->key() == Qt::Key_PageDown) {
        scrollDown(m_rows - 1);
        return;
    }

    QByteArray toSend;

    // 0. Check control characters in text (covers terminal shortcuts like Ctrl+C (\x03), Ctrl+D (\x04), Ctrl+R (\x12))
    QString evText = event->text();
    if (!evText.isEmpty() && (event->modifiers() & Qt::ControlModifier)) {
        ushort u = evText[0].unicode();
        if (u >= 1 && u <= 26) {
            emit dataToSend(QByteArray(1, static_cast<char>(u)));
            return;
        }
    }

    // 1. macOS Command shortcuts (Cmd+C / Cmd+V / Cmd+A)
    if (event->modifiers() & Qt::MetaModifier) {
        if (event->key() == Qt::Key_C) {
            copySelection();
            return;
        } else if (event->key() == Qt::Key_V) {
            pasteClipboard();
            return;
        } else if (event->key() == Qt::Key_A) {
            selectAll();
            return;
        } else if (event->key() == Qt::Key_F) {
            showFindDialog();
            return;
        }
    }

    // 2. Standard Unix Shell Ctrl+Key shortcuts (Ctrl+C, Ctrl+R, Ctrl+D, Ctrl+Z, Ctrl+L, etc.)
    if (event->modifiers() & Qt::ControlModifier) {
        int key = event->key();
        if (key == Qt::Key_V) {
            pasteClipboard();
            return;
        } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            char ctrlChar = static_cast<char>(key - Qt::Key_A + 1);
            emit dataToSend(QByteArray(1, ctrlChar));
            return;
        } else if (key >= 'a' && key <= 'z') {
            char ctrlChar = static_cast<char>(key - 'a' + 1);
            emit dataToSend(QByteArray(1, ctrlChar));
            return;
        } else if (key == Qt::Key_BracketLeft) { // Ctrl+[ (ESC)
            emit dataToSend("\x1B");
            return;
        } else if (key == Qt::Key_Backslash) { // Ctrl+\ (SIGQUIT)
            emit dataToSend("\x1C");
            return;
        } else if (key == Qt::Key_BracketRight) { // Ctrl+]
            emit dataToSend("\x1D");
            return;
        } else if (key == Qt::Key_Underscore) { // Ctrl+_
            emit dataToSend("\x1F");
            return;
        }
    }

    // If on default banner page and not in an active session, Space or Enter triggers Quick Connect
    if (m_showingBanner && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space)) {
        emit quickConnectRequested();
        return;
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        toSend = "\r";
        break;
    case Qt::Key_Space:
        toSend = " ";
        break;
    case Qt::Key_Backspace:
        toSend = "\x7F";
        break;
    case Qt::Key_Tab:
        toSend = "\t";
        break;
    case Qt::Key_Escape:
        toSend = "\x1B";
        break;
    case Qt::Key_Up:
        toSend = "\x1B[A";
        break;
    case Qt::Key_Down:
        toSend = "\x1B[B";
        break;
    case Qt::Key_Right:
        toSend = "\x1B[C";
        break;
    case Qt::Key_Left:
        toSend = "\x1B[D";
        break;
    case Qt::Key_Home:
        toSend = "\x1B[1~";
        break;
    case Qt::Key_End:
        toSend = "\x1B[4~";
        break;
    case Qt::Key_Insert:
        if (event->modifiers() & Qt::ShiftModifier) {
            pasteClipboard();
            return;
        } else if (event->modifiers() & Qt::ControlModifier) {
            copySelection();
            return;
        }
        break;
    default:
        toSend = event->text().toUtf8();
        break;
    }

    if (!toSend.isEmpty()) {
        emit dataToSend(toSend);
    }
}

void TerminalWidget::mousePressEvent(QMouseEvent *event) {
    setFocus();
    if (event->button() == Qt::RightButton && m_pasteOnRightClick) {
        pasteClipboard();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int col = event->x() / m_charWidth;
        int viewBottom = qMax(m_rows - 1, (m_buffer.size() - 1) - m_scrollOffset);
        int startRow = qMax(0, viewBottom - m_rows + 1);
        int row = (event->y() / m_charHeight) + startRow;
        m_selectStart = QPoint(col, row);
        m_selectEnd = m_selectStart;
        update();
    }
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent *event) {
    if (!event->commitString().isEmpty()) {
        emit dataToSend(event->commitString().toUtf8());
    }
    event->accept();
}

QVariant TerminalWidget::inputMethodQuery(Qt::InputMethodQuery query) const {
    if (query == Qt::ImCursorRectangle || query == Qt::ImCursorPosition) {
        int cx = m_cursorCol * m_charWidth;
        int viewBottom = qMax(m_rows - 1, (m_buffer.size() - 1) - m_scrollOffset);
        int startRow = qMax(0, viewBottom - m_rows + 1);
        int cy = (m_cursorRow - startRow) * m_charHeight;
        return QRect(cx, cy, m_charWidth, m_charHeight);
    } else if (query == Qt::ImFont) {
        return font();
    }
    return QWidget::inputMethodQuery(query);
}

void TerminalWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        int col = event->x() / m_charWidth;
        int viewBottom = qMax(m_rows - 1, (m_buffer.size() - 1) - m_scrollOffset);
        int startRow = qMax(0, viewBottom - m_rows + 1);
        int row = (event->y() / m_charHeight) + startRow;
        m_selectEnd = QPoint(col, row);
        update();
    }
}

void TerminalWidget::resizeEvent(QResizeEvent *) {
    updateDimensions();
}

void TerminalWidget::updateDimensions() {
    int newCols = qMax(10, width() / m_charWidth);
    int newRows = qMax(5, height() / m_charHeight);
    if (newCols != m_cols || newRows != m_rows) {
        bool wasShowingBanner = m_showingBanner;
        m_cols = newCols;
        m_rows = newRows;
        emit terminalResized(m_cols, m_rows);
        if (wasShowingBanner) {
            resetToDefaultBanner();
        } else {
            updateScrollRange();
        }
    }
}

void TerminalWidget::copySelection() {
    QString selected;
    if (m_selectStart != m_selectEnd) {
        int minR = qMin(m_selectStart.y(), m_selectEnd.y());
        int maxR = qMax(m_selectStart.y(), m_selectEnd.y());

        for (int r = minR; r <= maxR && r < m_buffer.size(); ++r) {
            const auto &row = m_buffer[r];
            int minC = 0;
            int maxC = row.size();

            if (r == minR && r == maxR) {
                minC = qBound(0, qMin(m_selectStart.x(), m_selectEnd.x()), row.size());
                maxC = qBound(0, qMax(m_selectStart.x(), m_selectEnd.x()), row.size());
            } else if (r == minR) {
                int c1 = (m_selectStart.y() < m_selectEnd.y()) ? m_selectStart.x() : m_selectEnd.x();
                minC = qBound(0, c1, row.size());
            } else if (r == maxR) {
                int c2 = (m_selectStart.y() > m_selectEnd.y()) ? m_selectStart.x() : m_selectEnd.x();
                maxC = qBound(0, c2, row.size());
            }

            QString lineText;
            for (int c = minC; c < maxC; ++c) {
                if (row[c].isWideContinuation) continue; // Skip continuation cell when copying
                lineText.append(row[c].ch.isNull() ? ' ' : row[c].ch);
            }
            selected.append(lineText.trimmed());
            if (r < maxR) selected.append('\n');
        }
    }
    if (!selected.isEmpty()) {
        QApplication::clipboard()->setText(selected);
    }
}

void TerminalWidget::pasteClipboard() {
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData && mimeData->hasImage()) {
        emit uploadClipboardImageRequested();
        return;
    }

    QString clip = clipboard->text();
    if (!clip.isEmpty()) {
        emit dataToSend(clip.toUtf8());
    }
}

void TerminalWidget::pasteSelection() {
    pasteClipboard();
}

bool TerminalWidget::findText(const QString &text, bool matchCase, bool matchWord, bool isRegex, bool searchUp) {
    if (text.isEmpty() || m_buffer.isEmpty()) return false;

    int totalRows = m_buffer.size();
    int startR = searchUp ? m_cursorRow : 0;

    // Continue from next row if repeatedly clicking Find Next
    if (m_lastFoundRow >= 0) {
        startR = searchUp ? (m_lastFoundRow - 1) : (m_lastFoundRow + 1);
    }

    int endR = searchUp ? -1 : totalRows;
    int step = searchUp ? -1 : 1;

    QRegularExpression re;
    if (isRegex) {
        auto patternOptions = matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
        re.setPattern(text);
        re.setPatternOptions(patternOptions);
        if (!re.isValid()) {
            QMessageBox::warning(this, "Find", "Invalid regular expression pattern.");
            return false;
        }
    }

    for (int r = startR; r != endR; r += step) {
        if (r < 0 || r >= m_buffer.size()) continue;

        QString lineStr;
        for (const auto &ac : m_buffer[r]) {
            lineStr.append(ac.ch.isNull() ? ' ' : ac.ch);
        }

        int matchCol = -1;
        int matchLen = 0;

        if (isRegex) {
            auto match = re.match(lineStr);
            if (match.hasMatch()) {
                matchCol = match.capturedStart();
                matchLen = match.capturedLength();
            }
        } else {
            Qt::CaseSensitivity cs = matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
            int searchIdx = 0;
            while (searchIdx < lineStr.length()) {
                int found = lineStr.indexOf(text, searchIdx, cs);
                if (found < 0) break;

                if (matchWord) {
                    bool leftOk = (found == 0) || !lineStr.at(found - 1).isLetterOrNumber();
                    int rightEdge = found + text.length();
                    bool rightOk = (rightEdge >= lineStr.length()) || !lineStr.at(rightEdge).isLetterOrNumber();
                    if (leftOk && rightOk) {
                        matchCol = found;
                        matchLen = text.length();
                        break;
                    }
                    searchIdx = found + 1;
                } else {
                    matchCol = found;
                    matchLen = text.length();
                    break;
                }
            }
        }

        if (matchCol >= 0 && matchLen > 0) {
            int maxOffset = qMax(0, m_cursorRow - m_rows + 1);
            m_scrollOffset = qBound(0, m_cursorRow - r, maxOffset);
            m_selectStart = QPoint(matchCol, r);
            m_selectEnd = QPoint(matchCol + matchLen, r);
            m_lastFoundRow = r;
            m_lastFoundCol = matchCol;
            updateScrollRange();
            update();
            return true;
        }
    }

    // Wrapped search or reach end
    m_lastFoundRow = -1;
    return false;
}

void TerminalWidget::showFindDialog() {
    if (!m_findDialog) {
        m_findDialog = new FindDialog(this);
        m_findDialog->setOnFindNext([this]() {
            QString target = m_findDialog->findWhat();
            if (target.isEmpty()) return;

            bool found = findText(target, m_findDialog->matchCase(),
                                  m_findDialog->matchWholeWord(),
                                  m_findDialog->isRegex(),
                                  m_findDialog->isDirectionUp());
            if (!found) {
                QMessageBox::information(m_findDialog, "Find", QString("Cannot find \"%1\"").arg(target));
            }
        });
    }

    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void TerminalWidget::selectAll() {
    m_selectStart = QPoint(0, 0);
    m_selectEnd = QPoint(m_cols, m_buffer.size() - 1);
    update();
}

void TerminalWidget::selectScreen() {
    int viewBottom = qMax(m_rows - 1, (m_buffer.size() - 1) - m_scrollOffset);
    int startRow = qMax(0, viewBottom - m_rows + 1);
    m_selectStart = QPoint(0, startRow);
    m_selectEnd = QPoint(m_cols, startRow + m_rows);
    update();
}

void TerminalWidget::selectNone() {
    m_selectStart = QPoint(-1, -1);
    m_selectEnd = QPoint(-1, -1);
    update();
}

void TerminalWidget::contextMenuEvent(QContextMenuEvent *event) {
    if (m_pasteOnRightClick) return;

    QMenu menu(this);

    // 1. Copy (Ctrl+Insert)
    QAction *copyAct = menu.addAction(QIcon(":/icons/quick_connect.svg"), "&Copy\tCtrl+Insert", this, &TerminalWidget::copySelection);
    copyAct->setEnabled(m_selectStart != m_selectEnd);

    // 2. Paste (Shift+Insert)
    menu.addAction(QIcon(":/icons/connect.svg"), "&Paste\tShift+Insert", this, &TerminalWidget::pasteClipboard);

    // 3. Paste Selection
    menu.addAction(QIcon(":/icons/connect.svg"), "Paste &Selection", this, &TerminalWidget::pasteSelection);

    menu.addSeparator();

    // 4. Select All
    menu.addAction("Select &All", this, &TerminalWidget::selectAll);

    // 5. Select Screen
    menu.addAction("Select Sc&reen", this, &TerminalWidget::selectScreen);

    // 6. Select None
    menu.addAction("Select &None", this, &TerminalWidget::selectNone);

    menu.addSeparator();

    // 7. Find...
    menu.addAction(QIcon(":/icons/find.svg"), "&Find...", this, &TerminalWidget::showFindDialog);

    menu.addSeparator();

    // 8. New Terminal
    menu.addAction(QIcon(":/icons/quick_connect.svg"), "New &Terminal", this, &TerminalWidget::newTerminalRequested);

    // 9. New File Transfer
    menu.addAction(QIcon(":/icons/sftp.svg"), "New &File Transfer", this, &TerminalWidget::newFileTransferRequested);

    // 10. Close Window
    menu.addAction("✕  Close &Window", this, &TerminalWidget::closeWindowRequested);

    menu.addSeparator();

    // 11. Settings...
    menu.addAction(QIcon(":/icons/settings.svg"), "&Settings...", this, &TerminalWidget::settingsRequested);

    menu.exec(event->globalPos());
}
