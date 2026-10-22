#pragma once

#include <QString>
#include <QMap>
#include <QSettings>
#include "backend/SshSessionWorker.h"

struct ProfileData {
    QString name;
    SshConnectionParams params;
    QString cipher = "<Default>";
    QString mac = "<Default>";
    QString compression = "<None>";
    QString terminal = "vt100";

    // Appearance & Font
    QString fontFamily = "SF Mono";
    int fontSize = 11;
    int scrollbackBuffer = 1000;
    bool scrollOnOutput = true;
    bool pasteOnRightClick = false;

    // Colors
    QString fgColor = "#1F2328";
    QString bgColor = "#FAFBFC";
    QString selColor = "#B6D7FF";
    QString cursorColor = "#0969DA";
    bool enableAnsiColors = true;
    bool reverseVideo = false;
};

struct GlobalAppearanceSettings {
    QString fontFamily = "SF Mono";
    int fontSize = 11;
    int scrollbackBuffer = 1000;
    bool scrollOnOutput = true;
    bool pasteOnRightClick = false;

    QString fgColor = "#1F2328";
    QString bgColor = "#FAFBFC";
    QString selColor = "#B6D7FF";
    QString cursorColor = "#0969DA";
    bool enableAnsiColors = true;
    bool reverseVideo = false;
};

class ProfileManager {
public:
    static ProfileManager& instance() {
        static ProfileManager s_inst;
        return s_inst;
    }

    GlobalAppearanceSettings getGlobalAppearance() const {
        return m_globalAppearance;
    }

    void saveGlobalAppearance(const GlobalAppearanceSettings &g) {
        m_globalAppearance = g;
        if (m_globalAppearance.fontFamily.startsWith("TT  ")) {
            m_globalAppearance.fontFamily = m_globalAppearance.fontFamily.mid(4);
        }
        QSettings s("RetroSSH", "Settings");
        s.setValue("fontFamily", m_globalAppearance.fontFamily);
        s.setValue("fontSize", g.fontSize);
        s.setValue("scrollbackBuffer", g.scrollbackBuffer);
        s.setValue("scrollOnOutput", g.scrollOnOutput);
        s.setValue("pasteOnRightClick", g.pasteOnRightClick);
        s.setValue("fgColor", g.fgColor);
        s.setValue("bgColor", g.bgColor);
        s.setValue("selColor", g.selColor);
        s.setValue("cursorColor", g.cursorColor);
        s.setValue("enableAnsiColors", g.enableAnsiColors);
        s.setValue("reverseVideo", g.reverseVideo);
        s.sync();
    }

    void loadGlobalAppearance() {
        QSettings s("RetroSSH", "Settings");
        m_globalAppearance.fontFamily = s.value("fontFamily", "SF Mono").toString();
        if (m_globalAppearance.fontFamily.startsWith("TT  ")) {
            m_globalAppearance.fontFamily = m_globalAppearance.fontFamily.mid(4);
        }
        m_globalAppearance.fontSize = s.value("fontSize", 11).toInt();
        m_globalAppearance.scrollbackBuffer = s.value("scrollbackBuffer", 1000).toInt();
        m_globalAppearance.scrollOnOutput = s.value("scrollOnOutput", true).toBool();
        m_globalAppearance.pasteOnRightClick = s.value("pasteOnRightClick", false).toBool();
        m_globalAppearance.fgColor = s.value("fgColor", "#1F2328").toString();
        m_globalAppearance.bgColor = s.value("bgColor", "#FAFBFC").toString();
        m_globalAppearance.selColor = s.value("selColor", "#B6D7FF").toString();
        m_globalAppearance.cursorColor = s.value("cursorColor", "#0969DA").toString();
        m_globalAppearance.enableAnsiColors = s.value("enableAnsiColors", true).toBool();
        m_globalAppearance.reverseVideo = s.value("reverseVideo", false).toBool();
    }

    void loadProfiles() {
        loadGlobalAppearance();
        QSettings s("SSH Secure Shell", "Profiles");
        int size = s.beginReadArray("profiles");
        m_profiles.clear();
        for (int i = 0; i < size; ++i) {
            s.setArrayIndex(i);
            ProfileData p;
            p.name = s.value("name").toString();
            p.params.host = s.value("host").toString();
            p.params.username = s.value("username").toString();
            p.params.port = s.value("port", 22).toInt();
            p.params.authMethod = s.value("authMethod", "Public Key").toString();
            p.cipher = s.value("cipher", "<Default>").toString();
            p.mac = s.value("mac", "<Default>").toString();
            p.compression = s.value("compression", "<None>").toString();
            p.terminal = s.value("terminal", "vt100").toString();

            p.fontFamily = s.value("fontFamily", "SF Mono").toString();
            if (p.fontFamily.startsWith("TT  ")) {
                p.fontFamily = p.fontFamily.mid(4);
            }
            p.fontSize = s.value("fontSize", 11).toInt();
            p.scrollbackBuffer = s.value("scrollbackBuffer", 1000).toInt();
            p.scrollOnOutput = s.value("scrollOnOutput", true).toBool();
            p.pasteOnRightClick = s.value("pasteOnRightClick", false).toBool();
            p.fgColor = s.value("fgColor", "#1F2328").toString();
            p.bgColor = s.value("bgColor", "#FAFBFC").toString();
            p.selColor = s.value("selColor", "#B6D7FF").toString();
            p.cursorColor = s.value("cursorColor", "#0969DA").toString();
            p.enableAnsiColors = s.value("enableAnsiColors", true).toBool();
            p.reverseVideo = s.value("reverseVideo", false).toBool();

            if (!p.name.isEmpty()) {
                m_profiles.insert(p.name, p);
            }
        }
        s.endArray();
    }

    void saveProfiles() {
        QSettings s("SSH Secure Shell", "Profiles");
        s.beginWriteArray("profiles");
        int idx = 0;
        for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it, ++idx) {
            s.setArrayIndex(idx);
            s.setValue("name", it.value().name);
            s.setValue("host", it.value().params.host);
            s.setValue("username", it.value().params.username);
            s.setValue("port", it.value().params.port);
            s.setValue("authMethod", it.value().params.authMethod);
            s.setValue("cipher", it.value().cipher);
            s.setValue("mac", it.value().mac);
            s.setValue("compression", it.value().compression);
            s.setValue("terminal", it.value().terminal);

            s.setValue("fontFamily", it.value().fontFamily);
            s.setValue("fontSize", it.value().fontSize);
            s.setValue("scrollbackBuffer", it.value().scrollbackBuffer);
            s.setValue("scrollOnOutput", it.value().scrollOnOutput);
            s.setValue("pasteOnRightClick", it.value().pasteOnRightClick);
            s.setValue("fgColor", it.value().fgColor);
            s.setValue("bgColor", it.value().bgColor);
            s.setValue("selColor", it.value().selColor);
            s.setValue("cursorColor", it.value().cursorColor);
            s.setValue("enableAnsiColors", it.value().enableAnsiColors);
            s.setValue("reverseVideo", it.value().reverseVideo);
        }
        s.endArray();
        s.sync();
    }

    QMap<QString, ProfileData> getProfiles() const { return m_profiles; }

    void addOrUpdateProfile(const ProfileData &p) {
        m_profiles.insert(p.name, p);
        saveProfiles();
    }

    void removeProfile(const QString &name) {
        m_profiles.remove(name);
        saveProfiles();
    }

    bool hasProfile(const QString &name) const {
        return m_profiles.contains(name);
    }

    ProfileData getProfile(const QString &name) const {
        return m_profiles.value(name);
    }

private:
    ProfileManager() {
        loadProfiles();
    }
    GlobalAppearanceSettings m_globalAppearance;
    QMap<QString, ProfileData> m_profiles;
};
