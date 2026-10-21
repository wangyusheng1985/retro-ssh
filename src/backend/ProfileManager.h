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
    QString fontFamily = "Courier New";
    int fontSize = 10;
    int scrollbackBuffer = 1000;
    bool scrollOnOutput = true;
    bool pasteOnRightClick = false;

    // Colors
    QString fgColor = "Black";
    QString bgColor = "White";
    QString selColor = "Aqua";
    QString cursorColor = "Blue";
    bool enableAnsiColors = true;
    bool reverseVideo = false;
};

struct GlobalAppearanceSettings {
    QString fontFamily = "Courier New";
    int fontSize = 10;
    int scrollbackBuffer = 1000;
    bool scrollOnOutput = true;
    bool pasteOnRightClick = false;

    QString fgColor = "Black";
    QString bgColor = "White";
    QString selColor = "Aqua";
    QString cursorColor = "Blue";
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
        QSettings s("RetroSSH", "Settings");
        s.setValue("fontFamily", g.fontFamily);
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
    }

    void loadGlobalAppearance() {
        QSettings s("RetroSSH", "Settings");
        m_globalAppearance.fontFamily = s.value("fontFamily", "Courier New").toString();
        m_globalAppearance.fontSize = s.value("fontSize", 10).toInt();
        m_globalAppearance.scrollbackBuffer = s.value("scrollbackBuffer", 1000).toInt();
        m_globalAppearance.scrollOnOutput = s.value("scrollOnOutput", true).toBool();
        m_globalAppearance.pasteOnRightClick = s.value("pasteOnRightClick", false).toBool();
        m_globalAppearance.fgColor = s.value("fgColor", "Black").toString();
        m_globalAppearance.bgColor = s.value("bgColor", "White").toString();
        m_globalAppearance.selColor = s.value("selColor", "Aqua").toString();
        m_globalAppearance.cursorColor = s.value("cursorColor", "Blue").toString();
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

            p.fontFamily = s.value("fontFamily", "Courier New").toString();
            p.fontSize = s.value("fontSize", 10).toInt();
            p.scrollbackBuffer = s.value("scrollbackBuffer", 1000).toInt();
            p.scrollOnOutput = s.value("scrollOnOutput", true).toBool();
            p.pasteOnRightClick = s.value("pasteOnRightClick", false).toBool();
            p.fgColor = s.value("fgColor", "Black").toString();
            p.bgColor = s.value("bgColor", "White").toString();
            p.selColor = s.value("selColor", "Aqua").toString();
            p.cursorColor = s.value("cursorColor", "Blue").toString();
            p.enableAnsiColors = s.value("enableAnsiColors", true).toBool();
            p.reverseVideo = s.value("reverseVideo", false).toBool();

            if (!p.name.isEmpty()) {
                m_profiles.insert(p.name, p);
            }
        }
        s.endArray();

        // Ensure default initial entry if empty
        if (m_profiles.isEmpty()) {
            ProfileData def;
            def.name = "10.9.34.84";
            def.params.host = "10.9.34.84";
            def.params.username = "root";
            def.params.port = 22;
            def.params.authMethod = "Public Key";
            m_profiles.insert(def.name, def);
            saveProfiles();
        }
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
