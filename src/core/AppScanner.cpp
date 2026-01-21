#include "AppScanner.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDebug>

namespace udevme {

const QStringList AppScanner::s_browserDesktopIds = {
    "brave-browser.desktop",
    "brave.desktop",
    "google-chrome.desktop",
    "chromium.desktop",
    "chromium-browser.desktop",
    "firefox.desktop",
    "firefox-esr.desktop",
    "microsoft-edge.desktop",
    "vivaldi-stable.desktop",
    "opera.desktop",
    "zen-browser.desktop",
    "librewolf.desktop",
    "ungoogled-chromium.desktop"
};

AppScanner::AppScanner(QObject* parent) : QObject(parent) {}

QStringList AppScanner::getDesktopDirs() const {
    QStringList dirs;
    
    // User-local applications
    QString homeDir = QDir::homePath();
    dirs << homeDir + "/.local/share/applications";
    
    // Flatpak user
    dirs << homeDir + "/.local/share/flatpak/exports/share/applications";
    
    // System applications
    dirs << "/usr/share/applications";
    dirs << "/usr/local/share/applications";
    
    // Flatpak system
    dirs << "/var/lib/flatpak/exports/share/applications";
    
    // Snap
    dirs << "/var/lib/snapd/desktop/applications";
    
    return dirs;
}

AppInfo AppScanner::parseDesktopFile(const QString& path) {
    AppInfo app;
    QFile file(path);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return app;
    }
    
    QFileInfo fi(path);
    app.desktopId = fi.fileName();
    
    QTextStream in(&file);
    bool inDesktopEntry = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.startsWith('[')) {
            inDesktopEntry = (line == "[Desktop Entry]");
            continue;
        }
        
        if (!inDesktopEntry) continue;
        
        int eq = line.indexOf('=');
        if (eq <= 0) continue;
        
        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();
        
        if (key == "Name" && app.name.isEmpty()) {
            app.name = value;
        } else if (key == "Exec") {
            app.exec = value;
        } else if (key == "Icon") {
            app.icon = value;
        } else if (key == "GenericName" && app.genericName.isEmpty()) {
            app.genericName = value;
        } else if (key == "Type" && value != "Application") {
            return AppInfo(); // Not an application
        } else if (key == "NoDisplay" && value.toLower() == "true") {
            return AppInfo(); // Hidden app
        }
    }
    
    file.close();
    return app;
}

QVector<AppInfo> AppScanner::scanApplications() {
    emit scanProgress("Scanning installed applications...");
    
    QVector<AppInfo> apps;
    QSet<QString> seenIds;
    
    for (const QString& dirPath : getDesktopDirs()) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        
        for (const QString& entry : dir.entryList({"*.desktop"}, QDir::Files)) {
            if (seenIds.contains(entry)) continue;
            
            AppInfo app = parseDesktopFile(dir.filePath(entry));
            if (!app.name.isEmpty() && !app.exec.isEmpty()) {
                apps.append(app);
                seenIds.insert(entry);
            }
        }
    }
    
    // Sort: browsers first, then alphabetically
    std::sort(apps.begin(), apps.end(), [](const AppInfo& a, const AppInfo& b) {
        bool aIsBrowser = s_browserDesktopIds.contains(a.desktopId);
        bool bIsBrowser = s_browserDesktopIds.contains(b.desktopId);
        
        if (aIsBrowser != bIsBrowser) {
            return aIsBrowser; // Browsers come first
        }
        return a.name.toLower() < b.name.toLower();
    });
    
    emit scanComplete(apps);
    return apps;
}

QVector<AppInfo> AppScanner::getBrowsers() const {
    QVector<AppInfo> browsers;
    
    for (const QString& dirPath : getDesktopDirs()) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        
        for (const QString& browserId : s_browserDesktopIds) {
            QString path = dir.filePath(browserId);
            if (QFile::exists(path)) {
                AppInfo app = const_cast<AppScanner*>(this)->parseDesktopFile(path);
                if (!app.name.isEmpty()) {
                    bool found = false;
                    for (const auto& b : browsers) {
                        if (b.desktopId == app.desktopId) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) browsers.append(app);
                }
            }
        }
    }
    
    return browsers;
}

} // namespace udevme
