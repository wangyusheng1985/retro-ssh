#pragma once

#include "backend/ProfileManager.h"
#include "backend/SshSessionWorker.h"
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QTreeWidget>

class ProfileSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileSettingsDialog(QWidget *parent = nullptr);
    SshConnectionParams getParams() const;
    void setParams(const SshConnectionParams &params);
    void setProfileData(const ProfileData &data);
    ProfileData getProfileData() const;

    static QColor colorNameToQColor(const QString &name);
    static QString qcolorToColorName(const QColor &color);

    QString getSelectedFontFamily() const;
    int getSelectedFontSize() const;
    int getScrollbackBuffer() const;
    bool getScrollOnOutput() const;
    bool getPasteOnRightClick() const;

    QColor getForegroundColor() const;
    QColor getBackgroundColor() const;
    QColor getSelectionColor() const;
    QColor getCursorColor() const;
    bool getEnableAnsiColors() const;
    bool getReverseVideo() const;

private:
    void setupTree();
    QWidget *createConnectionPage();
    QWidget *createAuthenticationPage();
    QWidget *createCipherPage();
    QWidget *createFontPage();
    QWidget *createColorsPage();
    QWidget *createMessagesPage();

    QComboBox *createColorComboBox(const QStringList &colors, const QString &defaultColor, QWidget *parent);

    QTreeWidget *m_tree = nullptr;
    QStackedWidget *m_pages = nullptr;

    // Connection widgets
    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QComboBox *m_encryptionCombo = nullptr;
    QComboBox *m_macCombo = nullptr;
    QCheckBox *m_keepAliveCb = nullptr;

    // Auth widgets
    QListWidget *m_authList = nullptr;

    // Font & Terminal widgets
    QListWidget *m_fontFamilyList = nullptr;
    QListWidget *m_fontSizeList = nullptr;
    QCheckBox *m_pasteOnRightClickCb = nullptr;
    QCheckBox *m_scrollBottomOnOutputCb = nullptr;
    QLineEdit *m_scrollbackEdit = nullptr;

    // Colors widgets (Screenshot 1:1)
    QComboBox *m_fgColorCombo = nullptr;
    QComboBox *m_bgColorCombo = nullptr;
    QComboBox *m_selColorCombo = nullptr;
    QComboBox *m_discColorCombo = nullptr;
    QComboBox *m_cursorColorCombo = nullptr;
    QCheckBox *m_enableAnsiCb = nullptr;
    QCheckBox *m_reverseVideoCb = nullptr;

    // Messages widgets
    QCheckBox *m_confirmDisconnectCb = nullptr;
    QCheckBox *m_confirmExitCb = nullptr;
    QCheckBox *m_showStatusBarMsgCb = nullptr;
    QCheckBox *m_showErrorDialogsCb = nullptr;

    // Profile metadata
    QString m_profileName;

    // Cipher widgets
    QListWidget *m_cipherList = nullptr;
};
