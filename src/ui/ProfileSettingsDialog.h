#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QColor>
#include "backend/SshSessionWorker.h"
#include "backend/ProfileManager.h"

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
    QWidget* createConnectionPage();
    QWidget* createAuthenticationPage();
    QWidget* createCipherPage();
    QWidget* createFontPage();
    QWidget* createAppearancePage();
    QWidget* createColorsPage();

    QComboBox* createColorComboBox(const QStringList &colors, const QString &defaultColor, QWidget *parent);

    QTreeWidget *m_tree = nullptr;
    QStackedWidget *m_pages = nullptr;

    // Connection widgets
    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QComboBox *m_encryptionCombo = nullptr;
    QComboBox *m_macCombo = nullptr;

    // Auth widgets
    QListWidget *m_authList = nullptr;

    // Font widgets
    QListWidget *m_fontFamilyList = nullptr;
    QListWidget *m_fontSizeList = nullptr;

    // Appearance widgets (Screenshot 1:1)
    QCheckBox *m_officeXpCb = nullptr;
    QCheckBox *m_showAddProfileCb = nullptr;
    QCheckBox *m_pasteOnRightClickCb = nullptr;
    QCheckBox *m_scrollBottomOnOutputCb = nullptr;
    QLineEdit *m_scrollbackEdit = nullptr;
    QCheckBox *m_dispProfileNameCb = nullptr;
    QCheckBox *m_dispHostNameCb = nullptr;
    QCheckBox *m_openAllWindowsCb = nullptr;

    // Colors widgets (Screenshot 1:1)
    QComboBox *m_fgColorCombo = nullptr;
    QComboBox *m_bgColorCombo = nullptr;
    QComboBox *m_selColorCombo = nullptr;
    QComboBox *m_discColorCombo = nullptr;
    QComboBox *m_cursorColorCombo = nullptr;
    QCheckBox *m_enableAnsiCb = nullptr;
    QCheckBox *m_reverseVideoCb = nullptr;

    // Cipher widgets
    QListWidget *m_cipherList = nullptr;
};
