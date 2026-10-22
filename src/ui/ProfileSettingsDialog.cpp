#include "ProfileSettingsDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QGroupBox>
#include <QFontDatabase>

ProfileSettingsDialog::ProfileSettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Settings");
    resize(580, 490);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);

    // Left navigation area: Tree on top, SSH logo at bottom
    QVBoxLayout *leftVl = new QVBoxLayout();
    leftVl->setSpacing(8);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setFixedWidth(170);
    setupTree();
    leftVl->addWidget(m_tree, 1);

    // Iconic blue "ssh" logo at bottom left
    QLabel *logoLabel = new QLabel(this);
    logoLabel->setPixmap(QIcon(":/icons/ssh_logo.svg").pixmap(125, 55));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("border: 1px solid #716F64; background-color: #001A6E;");
    leftVl->addWidget(logoLabel);

    contentLayout->addLayout(leftVl);

    // Right stacked pages
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createConnectionPage());       // Index 0
    m_pages->addWidget(createCipherPage());           // Index 1
    m_pages->addWidget(createAuthenticationPage());   // Index 2
    m_pages->addWidget(createFontPage());             // Index 3
    m_pages->addWidget(createAppearancePage());       // Index 4
    m_pages->addWidget(createColorsPage());           // Index 5

    contentLayout->addWidget(m_pages, 1);
    rootLayout->addLayout(contentLayout, 1);

    // Bottom buttons (OK, Cancel, Help)
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *okBtn = new QPushButton("OK", this);
    okBtn->setDefault(true);
    okBtn->setFixedWidth(75);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setFixedWidth(75);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton *helpBtn = new QPushButton("Help", this);
    helpBtn->setFixedWidth(75);
    btnLayout->addWidget(helpBtn);

    rootLayout->addLayout(btnLayout);

    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current) {
        if (!current) return;
        int pageIdx = current->data(0, Qt::UserRole).toInt();
        m_pages->setCurrentIndex(pageIdx);
    });

    m_tree->setCurrentItem(m_tree->topLevelItem(0)->child(0));
}

void ProfileSettingsDialog::setupTree() {
    auto *profileRoot = new QTreeWidgetItem(m_tree, QStringList("Profile Settings"));
    profileRoot->setExpanded(true);

    auto *itemConn = new QTreeWidgetItem(profileRoot, QStringList("Connection"));
    itemConn->setData(0, Qt::UserRole, 0);

    auto *itemCipher = new QTreeWidgetItem(itemConn, QStringList("Cipher List"));
    itemCipher->setData(0, Qt::UserRole, 1);

    auto *itemAuth = new QTreeWidgetItem(profileRoot, QStringList("Authentication"));
    itemAuth->setData(0, Qt::UserRole, 2);

    auto *itemColors = new QTreeWidgetItem(profileRoot, QStringList("Colors"));
    itemColors->setData(0, Qt::UserRole, 5);

    auto *itemKb = new QTreeWidgetItem(profileRoot, QStringList("Keyboard"));
    itemKb->setData(0, Qt::UserRole, 0);

    auto *itemTunnel = new QTreeWidgetItem(profileRoot, QStringList("Tunneling"));
    itemTunnel->setData(0, Qt::UserRole, 0);

    auto *itemFt = new QTreeWidgetItem(profileRoot, QStringList("File Transfer"));
    itemFt->setExpanded(true);
    auto *itemRemFav = new QTreeWidgetItem(itemFt, QStringList("Remote Favorites"));
    itemRemFav->setData(0, Qt::UserRole, 0);

    auto *globalRoot = new QTreeWidgetItem(m_tree, QStringList("Global Settings"));
    globalRoot->setExpanded(true);
    auto *itemApp = new QTreeWidgetItem(globalRoot, QStringList("Appearance"));
    itemApp->setExpanded(true);
    itemApp->setData(0, Qt::UserRole, 4); // Index 4: Appearance Page

    auto *itemFont = new QTreeWidgetItem(itemApp, QStringList("Font"));
    itemFont->setData(0, Qt::UserRole, 3); // Index 3: Font page

    auto *itemAppColors = new QTreeWidgetItem(itemApp, QStringList("Colors"));
    itemAppColors->setData(0, Qt::UserRole, 5);

    auto *itemMsg = new QTreeWidgetItem(itemApp, QStringList("Messages"));
    itemMsg->setData(0, Qt::UserRole, 4);

    auto *itemUserAuth = new QTreeWidgetItem(globalRoot, QStringList("User Authentication"));
    auto *itemKeys = new QTreeWidgetItem(itemUserAuth, QStringList("Keys"));
    itemKeys->setData(0, Qt::UserRole, 2);

    auto *itemServerAuth = new QTreeWidgetItem(globalRoot, QStringList("Server Authentication"));
    auto *itemGlobalFt = new QTreeWidgetItem(globalRoot, QStringList("File Transfer"));
    itemGlobalFt->setData(0, Qt::UserRole, 0);
}

static QWidget* createPageHeader(const QString &title) {
    QWidget *header = new QWidget();
    header->setFixedHeight(26);
    header->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4A90E2, stop:1 #2175D9);"
        "border: 1px solid #104E9E;"
    );
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(8, 0, 8, 0);
    QLabel *lbl = new QLabel(title, header);
    lbl->setStyleSheet("color: #000000; font-weight: bold; font-size: 13px; font-family: Tahoma, sans-serif;");
    hl->addWidget(lbl);
    return header;
}

QWidget* ProfileSettingsDialog::createAppearancePage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    vl->addWidget(createPageHeader("Appearance"));

    vl->addWidget(new QLabel("Configure the appearance of the application.", w));

    m_officeXpCb = new QCheckBox("Office XP look", w);
    vl->addWidget(m_officeXpCb);

    m_showAddProfileCb = new QCheckBox("Show the Add Profile dialog when using Quick Connect", w);
    m_showAddProfileCb->setChecked(true);
    vl->addWidget(m_showAddProfileCb);

    // GroupBox: Terminal settings (Screenshot 1:1)
    QGroupBox *termGb = new QGroupBox("Terminal settings", w);
    QVBoxLayout *termVl = new QVBoxLayout(termGb);
    termVl->setContentsMargins(8, 8, 8, 8);
    termVl->setSpacing(5);

    m_pasteOnRightClickCb = new QCheckBox("Paste on right mouse click", termGb);
    termVl->addWidget(m_pasteOnRightClickCb);

    m_scrollBottomOnOutputCb = new QCheckBox("Scroll bottom on output", termGb);
    m_scrollBottomOnOutputCb->setChecked(true);
    termVl->addWidget(m_scrollBottomOnOutputCb);

    QHBoxLayout *sbHl = new QHBoxLayout();
    sbHl->addWidget(new QLabel("Scrollback buffer", termGb));
    m_scrollbackEdit = new QLineEdit("500", termGb);
    m_scrollbackEdit->setFixedWidth(100);
    sbHl->addWidget(m_scrollbackEdit);
    sbHl->addWidget(new QLabel("lines", termGb));
    sbHl->addStretch();
    termVl->addLayout(sbHl);

    vl->addWidget(termGb);

    // GroupBox: Window caption
    QGroupBox *capGb = new QGroupBox("Window caption", w);
    QVBoxLayout *capVl = new QVBoxLayout(capGb);
    capVl->setContentsMargins(8, 8, 8, 8);
    capVl->setSpacing(4);

    m_dispProfileNameCb = new QCheckBox("Display profile name", capGb);
    m_dispProfileNameCb->setChecked(true);
    capVl->addWidget(m_dispProfileNameCb);

    m_dispHostNameCb = new QCheckBox("Display host name", capGb);
    m_dispHostNameCb->setChecked(true);
    capVl->addWidget(m_dispHostNameCb);

    vl->addWidget(capGb);

    // GroupBox: Window layout
    QGroupBox *layGb = new QGroupBox("Window layout", w);
    QVBoxLayout *layVl = new QVBoxLayout(layGb);
    layVl->setContentsMargins(8, 8, 8, 8);
    layVl->setSpacing(4);

    QLabel *layDesc = new QLabel(
        "Check the box to open all windows immediately when the profile is chosen. Uncheck to open windows on demand in their previously configured positions.", layGb);
    layDesc->setWordWrap(true);
    layVl->addWidget(layDesc);

    m_openAllWindowsCb = new QCheckBox("Open all windows of the profile", layGb);
    m_openAllWindowsCb->setChecked(true);
    layVl->addWidget(m_openAllWindowsCb);

    vl->addWidget(layGb);
    vl->addStretch();
    return w;
}

QWidget* ProfileSettingsDialog::createConnectionPage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    vl->addWidget(createPageHeader("Connection"));

    QLabel *desc1 = new QLabel("Configure protocol settings for the connection. New settings will take effect upon next login.", w);
    desc1->setWordWrap(true);
    vl->addWidget(desc1);

    QLabel *desc2 = new QLabel("Specify * as the host name or the user name to be prompted for the information when the profile is chosen for connecting.", w);
    desc2->setWordWrap(true);
    desc2->setStyleSheet("color: #333333; margin-bottom: 4px;");
    vl->addWidget(desc2);

    QGridLayout *g = new QGridLayout();
    g->setHorizontalSpacing(10);
    g->setVerticalSpacing(5);

    g->addWidget(new QLabel("Host name:", w), 0, 0);
    m_hostEdit = new QLineEdit(w);
    m_hostEdit->setPlaceholderText("e.g. 192.168.1.1 or example.com");
    g->addWidget(m_hostEdit, 0, 1, 1, 2);

    g->addWidget(new QLabel("User name:", w), 1, 0);
    m_userEdit = new QLineEdit(w);
    m_userEdit->setPlaceholderText("e.g. root");
    g->addWidget(m_userEdit, 1, 1, 1, 2);

    g->addWidget(new QLabel("Port", w), 2, 0);
    m_portEdit = new QLineEdit("22", w);
    m_portEdit->setFixedWidth(160);
    g->addWidget(m_portEdit, 2, 1, 1, 2, Qt::AlignLeft);

    g->addWidget(new QLabel("Encryption", w), 3, 0);
    m_encryptionCombo = new QComboBox(w);
    m_encryptionCombo->addItems({"<Default>", "AES-256-GCM", "AES-128-CTR", "ChaCha20-Poly1305", "3DES"});
    g->addWidget(m_encryptionCombo, 3, 1);

    QComboBox *bitDepth = new QComboBox(w);
    bitDepth->addItem("128");
    bitDepth->setEnabled(false);
    bitDepth->setFixedWidth(65);
    g->addWidget(bitDepth, 3, 2);

    g->addWidget(new QLabel("MAC", w), 4, 0);
    m_macCombo = new QComboBox(w);
    m_macCombo->addItems({"<Default>", "HMAC-SHA2-256", "HMAC-SHA2-512", "HMAC-SHA1"});
    g->addWidget(m_macCombo, 4, 1, 1, 2);

    g->addWidget(new QLabel("Compression:", w), 5, 0);
    QComboBox *compCombo = new QComboBox(w);
    compCombo->addItems({"<None>", "zlib"});
    g->addWidget(compCombo, 5, 1, 1, 2);

    g->addWidget(new QLabel("Terminal", w), 6, 0);
    QComboBox *termCombo = new QComboBox(w);
    termCombo->addItems({"vt100", "xterm", "ansi"});
    g->addWidget(termCombo, 6, 1, 1, 2);

    vl->addLayout(g);

    QCheckBox *fwCb = new QCheckBox("Connect through firewall", w);
    vl->addWidget(fwCb);

    QCheckBox *tunnelOnlyCb = new QCheckBox("Request tunnels only (disable terminal)", w);
    vl->addWidget(tunnelOnlyCb);

    vl->addStretch();
    return w;
}

static QHBoxLayout* createSubToolbar(QWidget *parent, QListWidget *list) {
    QHBoxLayout *hl = new QHBoxLayout();
    hl->setSpacing(2);
    hl->addStretch();

    QPushButton *newBtn = new QPushButton("✦", parent);
    newBtn->setFixedSize(22, 20);
    newBtn->setToolTip("Add");
    hl->addWidget(newBtn);

    QPushButton *delBtn = new QPushButton("✕", parent);
    delBtn->setFixedSize(22, 20);
    delBtn->setToolTip("Delete");
    QObject::connect(delBtn, &QPushButton::clicked, parent, [list]() {
        delete list->takeItem(list->currentRow());
    });
    hl->addWidget(delBtn);

    QPushButton *upBtn = new QPushButton("▲", parent);
    upBtn->setFixedSize(22, 20);
    upBtn->setToolTip("Move Up");
    QObject::connect(upBtn, &QPushButton::clicked, parent, [list]() {
        int r = list->currentRow();
        if (r > 0) {
            auto *item = list->takeItem(r);
            list->insertItem(r - 1, item);
            list->setCurrentRow(r - 1);
        }
    });
    hl->addWidget(upBtn);

    QPushButton *downBtn = new QPushButton("▼", parent);
    downBtn->setFixedSize(22, 20);
    downBtn->setToolTip("Move Down");
    QObject::connect(downBtn, &QPushButton::clicked, parent, [list]() {
        int r = list->currentRow();
        if (r >= 0 && r < list->count() - 1) {
            auto *item = list->takeItem(r);
            list->insertItem(r + 1, item);
            list->setCurrentRow(r + 1);
        }
    });
    hl->addWidget(downBtn);

    return hl;
}

QWidget* ProfileSettingsDialog::createAuthenticationPage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    vl->addWidget(createPageHeader("Authentication"));

    vl->addWidget(new QLabel("Configure user authentication methods and their preferred order.", w));

    QHBoxLayout *bar1 = new QHBoxLayout();
    bar1->addWidget(new QLabel("Authentication methods:", w));
    m_authList = new QListWidget(w);
    m_authList->setFixedHeight(70);
    m_authList->addItems({"Public Key", "Keyboard Interactive", "Password"});
    bar1->addLayout(createSubToolbar(w, m_authList));
    vl->addLayout(bar1);
    vl->addWidget(m_authList);

    vl->addWidget(new QLabel("Configure public-key authentication methods and their preferred order.", w));

    QListWidget *pkList = new QListWidget(w);
    pkList->setFixedHeight(85);
    pkList->addItems({
        "SSH Accession Certificates",
        "SSH Accession Keys",
        "PKCS #11 Certificates",
        "PKCS #11 Keys",
        "User Certificates",
        "User Keys"
    });
    QHBoxLayout *bar2 = new QHBoxLayout();
    bar2->addWidget(new QLabel("Public-key authentication methods:", w));
    bar2->addLayout(createSubToolbar(w, pkList));
    vl->addLayout(bar2);
    vl->addWidget(pkList);

    QGroupBox *gb = new QGroupBox("Authentication Agent forwarding", w);
    QVBoxLayout *gbVl = new QVBoxLayout(gb);
    gbVl->setContentsMargins(6, 6, 6, 6);
    gbVl->setSpacing(4);

    QLabel *gbDesc = new QLabel(
        "Configure agent forwarding for SSH2 and SSH1 connections. This feature is available with SSH Accession, which provides single sign-on to servers that allow public-key authentication.", gb);
    gbDesc->setWordWrap(true);
    gbVl->addWidget(gbDesc);

    QCheckBox *ssh2Cb = new QCheckBox("Enable for SSH2 connections", gb);
    ssh2Cb->setChecked(true);
    gbVl->addWidget(ssh2Cb);

    QCheckBox *ssh1For2Cb = new QCheckBox("Enable SSH1 agent forwarding for SSH2 connections", gb);
    gbVl->addWidget(ssh1For2Cb);

    QCheckBox *ssh1Cb = new QCheckBox("Enable for SSH1 connections", gb);
    gbVl->addWidget(ssh1Cb);

    vl->addWidget(gb);
    vl->addStretch();
    return w;
}

QWidget* ProfileSettingsDialog::createFontPage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    vl->addWidget(createPageHeader("Font"));

    QLabel *desc = new QLabel("Select the font to be used in the terminal window. The font list contains non-proportional fonts currently installed on the local computer.", w);
    desc->setWordWrap(true);
    vl->addWidget(desc);

    QHBoxLayout *listsHl = new QHBoxLayout();

    // Left Font Family
    QVBoxLayout *fVl = new QVBoxLayout();
    fVl->addWidget(new QLabel("Font", w));
    m_fontFamilyList = new QListWidget(w);
    QFontDatabase db;
    for (const QString &family : db.families()) {
        if (db.isFixedPitch(family)) {
            m_fontFamilyList->addItem("TT  " + family);
        }
    }
    if (m_fontFamilyList->count() == 0) {
        m_fontFamilyList->addItems({"TT  Courier New", "TT  Consolas", "TT  Monaco", "TT  Menlo"});
    }
    m_fontFamilyList->setCurrentRow(0);
    fVl->addWidget(m_fontFamilyList);
    listsHl->addLayout(fVl, 1);

    // Right Font Size
    QVBoxLayout *sVl = new QVBoxLayout();
    sVl->addWidget(new QLabel("Font", w));
    m_fontSizeList = new QListWidget(w);
    m_fontSizeList->setFixedWidth(70);
    m_fontSizeList->addItems({"7", "8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24"});
    m_fontSizeList->setCurrentRow(3); // 10
    sVl->addWidget(m_fontSizeList);
    listsHl->addLayout(sVl);

    vl->addLayout(listsHl, 1);
    return w;
}

QWidget* ProfileSettingsDialog::createCipherPage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(8);

    vl->addWidget(createPageHeader("Cipher List"));

    QLabel *desc = new QLabel("Select cryptographic ciphers enabled for this profile.", w);
    vl->addWidget(desc);

    m_cipherList = new QListWidget(w);
    m_cipherList->addItems({
        "aes256-gcm@openssh.com",
        "chacha20-poly1305@openssh.com",
        "aes128-gcm@openssh.com",
        "aes256-ctr",
        "aes192-ctr",
        "aes128-ctr",
        "3des-cbc"
    });
    for (int i = 0; i < m_cipherList->count(); ++i) {
        m_cipherList->item(i)->setCheckState(Qt::Checked);
    }
    vl->addWidget(m_cipherList);

    return w;
}

QComboBox* ProfileSettingsDialog::createColorComboBox(const QStringList &colors, const QString &defaultColor, QWidget *parent) {
    QComboBox *cb = new QComboBox(parent);
    cb->addItems(colors);
    int idx = cb->findText(defaultColor);
    if (idx >= 0) cb->setCurrentIndex(idx);
    return cb;
}

QWidget* ProfileSettingsDialog::createColorsPage() {
    QWidget *w = new QWidget();
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    vl->addWidget(createPageHeader("Colors"));
    vl->addWidget(new QLabel("Configure the color settings used in the terminal window.", w));

    QStringList colorNames = {"Black", "White", "Gray", "Blue", "Aqua", "Red", "Green", "Yellow", "Magenta"};

    // GroupBox: Text colors
    QGroupBox *textGb = new QGroupBox("Text colors", w);
    QGridLayout *textGrid = new QGridLayout(textGb);
    textGrid->setContentsMargins(8, 8, 8, 8);
    textGrid->setHorizontalSpacing(10);
    textGrid->setVerticalSpacing(6);

    textGrid->addWidget(new QLabel("Foreground:", textGb), 0, 0);
    m_fgColorCombo = createColorComboBox(colorNames, "Black", textGb);
    textGrid->addWidget(m_fgColorCombo, 0, 1);

    textGrid->addWidget(new QLabel("Selection:", textGb), 0, 2);
    m_selColorCombo = createColorComboBox(colorNames, "Aqua", textGb);
    textGrid->addWidget(m_selColorCombo, 0, 3);

    textGrid->addWidget(new QLabel("Background:", textGb), 1, 0);
    m_bgColorCombo = createColorComboBox(colorNames, "White", textGb);
    textGrid->addWidget(m_bgColorCombo, 1, 1);

    textGrid->addWidget(new QLabel("Disconnected:", textGb), 1, 2);
    m_discColorCombo = createColorComboBox(colorNames, "Gray", textGb);
    textGrid->addWidget(m_discColorCombo, 1, 3);

    vl->addWidget(textGb);

    // GroupBox: Cursor color
    QGroupBox *cursorGb = new QGroupBox("Cursor color", w);
    QHBoxLayout *cursorHl = new QHBoxLayout(cursorGb);
    cursorHl->setContentsMargins(8, 8, 8, 8);
    cursorHl->addWidget(new QLabel("Cursor:", cursorGb));
    m_cursorColorCombo = createColorComboBox(colorNames, "Blue", cursorGb);
    cursorHl->addWidget(m_cursorColorCombo);
    cursorHl->addStretch();
    vl->addWidget(cursorGb);

    // GroupBox: ANSI colors
    QGroupBox *ansiGb = new QGroupBox("ANSI colors", w);
    QHBoxLayout *ansiHl = new QHBoxLayout(ansiGb);
    ansiHl->setContentsMargins(8, 8, 8, 8);
    m_enableAnsiCb = new QCheckBox("Enable ANSI colors", ansiGb);
    m_enableAnsiCb->setChecked(true);
    ansiHl->addWidget(m_enableAnsiCb);
    QLabel *ansiDesc = new QLabel("Enable ANSI Colors for color support.\nDisable to use only foreground and background colors.", ansiGb);
    ansiHl->addWidget(ansiDesc);
    ansiHl->addStretch();
    vl->addWidget(ansiGb);

    // GroupBox: Reverse colors
    QGroupBox *revGb = new QGroupBox("Reverse colors", w);
    QHBoxLayout *revHl = new QHBoxLayout(revGb);
    revHl->setContentsMargins(8, 8, 8, 8);
    m_reverseVideoCb = new QCheckBox("Reverse video", revGb);
    revHl->addWidget(m_reverseVideoCb);
    QLabel *revDesc = new QLabel("The Reverse Video setting switches\nforeground and background colors.", revGb);
    revHl->addWidget(revDesc);
    revHl->addStretch();
    vl->addWidget(revGb);

    vl->addStretch();
    return w;
}

QColor ProfileSettingsDialog::colorNameToQColor(const QString &name) {
    if (name == "Black") return Qt::black;
    if (name == "White") return Qt::white;
    if (name == "Gray") return QColor(112, 112, 112);
    if (name == "Blue") return Qt::blue;
    if (name == "Aqua") return QColor(0, 255, 255);
    if (name == "Red") return Qt::red;
    if (name == "Green") return Qt::green;
    if (name == "Yellow") return Qt::yellow;
    if (name == "Magenta") return Qt::magenta;
    return Qt::black;
}

QString ProfileSettingsDialog::qcolorToColorName(const QColor &c) {
    if (c == Qt::black) return "Black";
    if (c == Qt::white) return "White";
    if (c == QColor(112, 112, 112) || c == Qt::gray) return "Gray";
    if (c == Qt::blue) return "Blue";
    if (c == QColor(0, 255, 255) || c == Qt::cyan) return "Aqua";
    if (c == Qt::red) return "Red";
    if (c == Qt::green) return "Green";
    if (c == Qt::yellow) return "Yellow";
    if (c == Qt::magenta) return "Magenta";
    return "Black";
}

QColor ProfileSettingsDialog::getForegroundColor() const {
    return m_fgColorCombo ? colorNameToQColor(m_fgColorCombo->currentText()) : Qt::black;
}

QColor ProfileSettingsDialog::getBackgroundColor() const {
    return m_bgColorCombo ? colorNameToQColor(m_bgColorCombo->currentText()) : Qt::white;
}

QColor ProfileSettingsDialog::getSelectionColor() const {
    return m_selColorCombo ? colorNameToQColor(m_selColorCombo->currentText()) : QColor(0, 255, 255);
}

QColor ProfileSettingsDialog::getCursorColor() const {
    return m_cursorColorCombo ? colorNameToQColor(m_cursorColorCombo->currentText()) : Qt::blue;
}

bool ProfileSettingsDialog::getEnableAnsiColors() const {
    return m_enableAnsiCb ? m_enableAnsiCb->isChecked() : true;
}

bool ProfileSettingsDialog::getReverseVideo() const {
    return m_reverseVideoCb ? m_reverseVideoCb->isChecked() : false;
}

QString ProfileSettingsDialog::getSelectedFontFamily() const {
    if (m_fontFamilyList && m_fontFamilyList->currentItem()) {
        QString f = m_fontFamilyList->currentItem()->text();
        if (f.startsWith("TT  ")) {
            f = f.mid(4);
        }
        return f;
    }
    return "SF Mono";
}

int ProfileSettingsDialog::getSelectedFontSize() const {
    if (m_fontSizeList && m_fontSizeList->currentItem()) {
        return m_fontSizeList->currentItem()->text().toInt();
    }
    return 10;
}

int ProfileSettingsDialog::getScrollbackBuffer() const {
    if (m_scrollbackEdit) {
        int v = m_scrollbackEdit->text().toInt();
        if (v > 0) return v;
    }
    return 500;
}

bool ProfileSettingsDialog::getScrollOnOutput() const {
    return m_scrollBottomOnOutputCb ? m_scrollBottomOnOutputCb->isChecked() : true;
}

bool ProfileSettingsDialog::getPasteOnRightClick() const {
    return m_pasteOnRightClickCb ? m_pasteOnRightClickCb->isChecked() : false;
}

SshConnectionParams ProfileSettingsDialog::getParams() const {
    SshConnectionParams p;
    p.host = m_hostEdit->text().trimmed();
    p.username = m_userEdit->text().trimmed();
    p.port = m_portEdit->text().toInt();
    if (m_authList->count() > 0) {
        p.authMethod = m_authList->item(0)->text();
    }
    return p;
}

void ProfileSettingsDialog::setParams(const SshConnectionParams &params) {
    m_hostEdit->setText(params.host);
    m_userEdit->setText(params.username);
    m_portEdit->setText(QString::number(params.port));
}

void ProfileSettingsDialog::setProfileData(const ProfileData &data) {
    setParams(data.params);

    // Font family
    QString targetFamily = data.fontFamily;
    if (targetFamily.startsWith("TT  ")) {
        targetFamily = targetFamily.mid(4);
    }
    if (m_fontFamilyList) {
        for (int i = 0; i < m_fontFamilyList->count(); ++i) {
            QString itemText = m_fontFamilyList->item(i)->text();
            if (itemText == targetFamily || itemText == ("TT  " + targetFamily) || itemText.endsWith(targetFamily)) {
                m_fontFamilyList->setCurrentRow(i);
                break;
            }
        }
    }

    // Font size
    if (m_fontSizeList) {
        QString sizeStr = QString::number(data.fontSize);
        for (int i = 0; i < m_fontSizeList->count(); ++i) {
            if (m_fontSizeList->item(i)->text() == sizeStr) {
                m_fontSizeList->setCurrentRow(i);
                break;
            }
        }
    }

    // Appearance
    if (m_scrollbackEdit) m_scrollbackEdit->setText(QString::number(data.scrollbackBuffer));
    if (m_scrollBottomOnOutputCb) m_scrollBottomOnOutputCb->setChecked(data.scrollOnOutput);
    if (m_pasteOnRightClickCb) m_pasteOnRightClickCb->setChecked(data.pasteOnRightClick);

    // Colors
    if (m_fgColorCombo) {
        int idx = m_fgColorCombo->findText(data.fgColor);
        if (idx >= 0) m_fgColorCombo->setCurrentIndex(idx);
    }
    if (m_bgColorCombo) {
        int idx = m_bgColorCombo->findText(data.bgColor);
        if (idx >= 0) m_bgColorCombo->setCurrentIndex(idx);
    }
    if (m_selColorCombo) {
        int idx = m_selColorCombo->findText(data.selColor);
        if (idx >= 0) m_selColorCombo->setCurrentIndex(idx);
    }
    if (m_cursorColorCombo) {
        int idx = m_cursorColorCombo->findText(data.cursorColor);
        if (idx >= 0) m_cursorColorCombo->setCurrentIndex(idx);
    }
    if (m_enableAnsiCb) m_enableAnsiCb->setChecked(data.enableAnsiColors);
    if (m_reverseVideoCb) m_reverseVideoCb->setChecked(data.reverseVideo);
}

ProfileData ProfileSettingsDialog::getProfileData() const {
    ProfileData p;
    p.params = getParams();
    p.fontFamily = getSelectedFontFamily();
    p.fontSize = getSelectedFontSize();
    p.scrollbackBuffer = getScrollbackBuffer();
    p.scrollOnOutput = getScrollOnOutput();
    p.pasteOnRightClick = getPasteOnRightClick();
    if (m_fgColorCombo) p.fgColor = m_fgColorCombo->currentText();
    if (m_bgColorCombo) p.bgColor = m_bgColorCombo->currentText();
    if (m_selColorCombo) p.selColor = m_selColorCombo->currentText();
    if (m_cursorColorCombo) p.cursorColor = m_cursorColorCombo->currentText();
    p.enableAnsiColors = getEnableAnsiColors();
    p.reverseVideo = getReverseVideo();
    return p;
}
