#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include "backend/ProfileManager.h"

class EditProfilesDialog : public QDialog {
public:
    explicit EditProfilesDialog(const QString &selectedProfile = "", QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("Profiles");
        resize(640, 500);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QVBoxLayout *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(10, 10, 10, 10);
        rootLayout->setSpacing(10);

        QHBoxLayout *centerLayout = new QHBoxLayout();
        centerLayout->setSpacing(10);

        // Left Tree: Quick Connect / Profiles root with child profile items
        m_tree = new QTreeWidget(this);
        m_tree->setHeaderHidden(true);
        m_tree->setFixedWidth(190);
        m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &EditProfilesDialog::showTreeContextMenu);

        setupTree(selectedProfile);
        centerLayout->addWidget(m_tree);

        // Right Tabs (Connection, Cipher List, Authentication, Keyboard, Colors, Tunneling, File Transfer, Favorite Folders)
        m_tabWidget = new QTabWidget(this);
        m_tabWidget->addTab(createConnectionTab(), "Connection");
        m_tabWidget->addTab(createCipherListTab(), "Cipher List");
        m_tabWidget->addTab(createAuthTab(), "Authentication");
        m_tabWidget->addTab(createKeyboardTab(), "Keyboard");
        m_tabWidget->addTab(createColorsTab(), "Colors");
        m_tabWidget->addTab(createTunnelingTab(), "Tunneling");
        m_tabWidget->addTab(createFileTransferTab(), "File Transfer");
        m_tabWidget->addTab(createFavoritesTab(), "Favorite Folders");

        centerLayout->addWidget(m_tabWidget, 1);
        rootLayout->addLayout(centerLayout, 1);

        // Bottom OK & Cancel
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *okBtn = new QPushButton("OK", this);
        okBtn->setDefault(true);
        okBtn->setFixedWidth(80);
        connect(okBtn, &QPushButton::clicked, this, &EditProfilesDialog::onOkClicked);
        btnLayout->addWidget(okBtn);

        QPushButton *cancelBtn = new QPushButton("Cancel", this);
        cancelBtn->setFixedWidth(80);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(cancelBtn);

        rootLayout->addLayout(btnLayout);

        connect(m_tree, &QTreeWidget::currentItemChanged, this, &EditProfilesDialog::onTreeSelectionChanged);

        // Initialize current selection
        if (!selectedProfile.isEmpty() && m_profileItems.contains(selectedProfile)) {
            m_tree->setCurrentItem(m_profileItems.value(selectedProfile));
        } else if (m_tree->topLevelItemCount() > 0) {
            m_tree->setCurrentItem(m_tree->topLevelItem(0));
        }
    }

    bool shouldConnect() const { return m_shouldConnect; }
    QString connectProfileName() const { return m_connectProfileName; }

private:
    void setupTree(const QString &selectedToFocus = "") {
        m_tree->clear();
        m_profileItems.clear();

        // Top-level 1: Quick Connect
        auto *qcItem = new QTreeWidgetItem(m_tree, QStringList("Quick Connect"));
        qcItem->setIcon(0, QIcon(":/icons/quick_connect.svg"));
        qcItem->setData(0, Qt::UserRole, "__quick_connect__");

        // Top-level 2: Profiles
        m_profRoot = new QTreeWidgetItem(m_tree, QStringList("Profiles"));
        m_profRoot->setIcon(0, QIcon(":/icons/sftp.svg"));
        m_profRoot->setData(0, Qt::UserRole, "__root__");
        m_profRoot->setExpanded(true);

        auto profiles = ProfileManager::instance().getProfiles();
        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            auto *pItem = new QTreeWidgetItem(m_profRoot, QStringList(it.key()));
            pItem->setIcon(0, QIcon(":/icons/connect.svg"));
            pItem->setData(0, Qt::UserRole, it.key());
            m_profileItems.insert(it.key(), pItem);
        }

        if (!selectedToFocus.isEmpty() && m_profileItems.contains(selectedToFocus)) {
            m_tree->setCurrentItem(m_profileItems.value(selectedToFocus));
        }
    }

    void showTreeContextMenu(const QPoint &pos) {
        QTreeWidgetItem *item = m_tree->itemAt(pos);
        if (!item) return;

        QString key = item->data(0, Qt::UserRole).toString();
        if (key.startsWith("__")) return; // Only show for child profiles

        QMenu menu(this);
        // Exact 1:1 items per screenshot:
        // Connect
        // ---
        // Copy
        // Cut
        // ---
        // Delete
        // Rename
        // ---
        // Create Shortcut

        QAction *connectAct = menu.addAction("&Connect");
        menu.addSeparator();
        QAction *copyAct = menu.addAction("&Copy");
        QAction *cutAct = menu.addAction("C&ut");
        menu.addSeparator();
        QAction *delAct = menu.addAction("&Delete");
        QAction *renameAct = menu.addAction("&Rename");
        menu.addSeparator();
        QAction *shortcutAct = menu.addAction("Create &Shortcut");

        QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == connectAct) {
            saveCurrentProfileData();
            m_shouldConnect = true;
            m_connectProfileName = key;
            accept();
        } else if (chosen == copyAct) {
            m_clipboardProfile = ProfileManager::instance().getProfile(key);
            m_isCut = false;
        } else if (chosen == cutAct) {
            m_clipboardProfile = ProfileManager::instance().getProfile(key);
            m_isCut = true;
            m_cutKey = key;
        } else if (chosen == delAct) {
            int ret = QMessageBox::question(this, "Confirm Profile Delete",
                                            QString("Are you sure you want to delete profile '%1'?").arg(key),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                ProfileManager::instance().removeProfile(key);
                setupTree();
            }
        } else if (chosen == renameAct) {
            bool ok = false;
            QString newName = QInputDialog::getText(this, "Rename Profile", "New profile name:",
                                                    QLineEdit::Normal, key, &ok);
            if (ok && !newName.trimmed().isEmpty() && newName.trimmed() != key) {
                newName = newName.trimmed();
                if (ProfileManager::instance().hasProfile(newName)) {
                    QMessageBox::warning(this, "Rename Profile", QString("A profile named '%1' already exists.").arg(newName));
                } else {
                    ProfileData p = ProfileManager::instance().getProfile(key);
                    p.name = newName;
                    ProfileManager::instance().removeProfile(key);
                    ProfileManager::instance().addOrUpdateProfile(p);
                    m_currentProfileKey = newName;
                    setupTree(newName);
                }
            }
        } else if (chosen == shortcutAct) {
            QMessageBox::information(this, "Create Shortcut",
                                     QString("Desktop shortcut created for profile '%1'.").arg(key));
        }
    }

    QWidget* createConnectionTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->setSpacing(6);

        QLabel *desc1 = new QLabel("Configure protocol settings for the connection. New settings will take effect upon next login.", w);
        desc1->setWordWrap(true);
        vl->addWidget(desc1);

        QLabel *desc2 = new QLabel("Specify * as the host name or the user name to be prompted for the information when the profile is chosen for connecting.", w);
        desc2->setWordWrap(true);
        desc2->setStyleSheet("color: #333333; margin-bottom: 6px;");
        vl->addWidget(desc2);

        QGridLayout *g = new QGridLayout();
        g->setHorizontalSpacing(10);
        g->setVerticalSpacing(6);

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
        m_compCombo = new QComboBox(w);
        m_compCombo->addItems({"<None>", "zlib"});
        g->addWidget(m_compCombo, 5, 1, 1, 2);

        g->addWidget(new QLabel("Terminal", w), 6, 0);
        m_termCombo = new QComboBox(w);
        m_termCombo->addItems({"vt100", "xterm", "ansi"});
        g->addWidget(m_termCombo, 6, 1, 1, 2);

        vl->addLayout(g);

        QCheckBox *fwCb = new QCheckBox("Connect through firewall", w);
        vl->addWidget(fwCb);

        QCheckBox *tunnelOnlyCb = new QCheckBox("Request tunnels only (disable terminal)", w);
        vl->addWidget(tunnelOnlyCb);

        m_keepAliveCb = new QCheckBox("Keep alive (prevent timeout)", w);
        m_keepAliveCb->setChecked(true);
        vl->addWidget(m_keepAliveCb);

        vl->addStretch();
        return w;
    }

    QWidget* createCipherListTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Cryptographic ciphers enabled for this profile:", w));
        QListWidget *lw = new QListWidget(w);
        lw->addItems({"aes256-gcm@openssh.com", "chacha20-poly1305@openssh.com", "aes128-gcm@openssh.com", "aes256-ctr", "aes128-ctr"});
        for (int i = 0; i < lw->count(); ++i) lw->item(i)->setCheckState(Qt::Checked);
        vl->addWidget(lw);
        return w;
    }

    QWidget* createAuthTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Authentication methods:", w));
        QListWidget *lw = new QListWidget(w);
        lw->addItems({"Public Key", "Keyboard Interactive", "Password"});
        vl->addWidget(lw);
        return w;
    }

    QWidget* createKeyboardTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Keyboard mappings and Backspace/Delete preferences.", w));
        vl->addStretch();
        return w;
    }

    QWidget* createColorsTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Profile specific colors and ANSI palette override.", w));
        vl->addStretch();
        return w;
    }

    QWidget* createTunnelingTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Port forwarding and secure tunnel definitions.", w));
        vl->addStretch();
        return w;
    }

    QWidget* createFileTransferTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("File Transfer options (default local / remote start folders).", w));
        vl->addStretch();
        return w;
    }

    QWidget* createFavoritesTab() {
        QWidget *w = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(w);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->addWidget(new QLabel("Favorite remote directories for quick navigation.", w));
        vl->addStretch();
        return w;
    }

    void loadProfileData(const QString &name) {
        if (!ProfileManager::instance().hasProfile(name)) return;
        auto p = ProfileManager::instance().getProfile(name);
        m_hostEdit->setText(p.params.host);
        m_userEdit->setText(p.params.username);
        m_portEdit->setText(QString::number(p.params.port));
        m_encryptionCombo->setCurrentText(p.cipher);
        m_macCombo->setCurrentText(p.mac);
        m_compCombo->setCurrentText(p.compression);
        m_termCombo->setCurrentText(p.terminal);
        if (m_keepAliveCb) {
            m_keepAliveCb->setChecked(p.keepAlive);
        }
    }

    void saveCurrentProfileData() {
        if (m_currentProfileKey.isEmpty() || m_currentProfileKey.startsWith("__")) return;
        ProfileData p = ProfileManager::instance().getProfile(m_currentProfileKey);
        p.name = m_currentProfileKey;
        p.params.host = m_hostEdit->text().trimmed();
        p.params.username = m_userEdit->text().trimmed();
        p.params.port = m_portEdit->text().toInt();
        p.cipher = m_encryptionCombo->currentText();
        p.mac = m_macCombo->currentText();
        p.compression = m_compCombo->currentText();
        p.terminal = m_termCombo->currentText();
        if (m_keepAliveCb) {
            p.keepAlive = m_keepAliveCb->isChecked();
            p.params.keepAlive = p.keepAlive;
        }
        ProfileManager::instance().addOrUpdateProfile(p);
    }

    void onTreeSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (previous) {
            saveCurrentProfileData();
        }
        if (!current) return;
        m_currentProfileKey = current->data(0, Qt::UserRole).toString();
        if (m_currentProfileKey.startsWith("__")) {
            m_hostEdit->clear();
            m_userEdit->clear();
            m_portEdit->setText("22");
        } else {
            loadProfileData(m_currentProfileKey);
        }
    }

    void onOkClicked() {
        saveCurrentProfileData();
        accept();
    }

    QTreeWidget *m_tree = nullptr;
    QTreeWidgetItem *m_profRoot = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    QMap<QString, QTreeWidgetItem*> m_profileItems;
    QString m_currentProfileKey;

    ProfileData m_clipboardProfile;
    bool m_isCut = false;
    QString m_cutKey;

    bool m_shouldConnect = false;
    QString m_connectProfileName;

    // Fields
    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QComboBox *m_encryptionCombo = nullptr;
    QComboBox *m_macCombo = nullptr;
    QComboBox *m_compCombo = nullptr;
    QComboBox *m_termCombo = nullptr;
    QCheckBox *m_keepAliveCb = nullptr;
};
