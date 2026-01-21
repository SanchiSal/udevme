#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include "ui/MainWindow.h"
#include "core/ConfigStore.h"

using namespace udevme;

bool checkFirstRun() {
    QString installDir = ConfigStore::getInstallDir();
    QDir dir(installDir);
    
    if (!dir.exists()) {
        int result = QMessageBox::question(
            nullptr,
            "First Run Setup",
            QString("udevme needs to create its configuration directory:\n\n"
                    "%1\n\n"
                    "This directory will store:\n"
                    "• udevme.json (configuration)\n"
                    "• 99-udevme.rules (staged rules)\n\n"
                    "Create directory now?").arg(installDir),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        
        if (result != QMessageBox::Yes) {
            return false;
        }
        
        if (!ConfigStore::ensureInstallDir()) {
            QMessageBox::critical(
                nullptr,
                "Error",
                QString("Failed to create directory:\n%1\n\n"
                        "Please create it manually or check permissions.").arg(installDir)
            );
            return false;
        }
    }
    
    return true;
}

void showStartupWarnings() {
    // Check for udevadm
    if (!QFile::exists("/usr/bin/udevadm") && !QFile::exists("/bin/udevadm")) {
        QMessageBox::warning(
            nullptr,
            "Warning",
            "udevadm not found. Device scanning may be limited.\n\n"
            "Install udev/systemd-udev package if device detection has issues."
        );
    }
    
    // Check for pkexec or sudo
    bool hasPkexec = QFile::exists("/usr/bin/pkexec");
    bool hasSudo = QFile::exists("/usr/bin/sudo");
    
    if (!hasPkexec && !hasSudo) {
        QMessageBox::warning(
            nullptr,
            "Warning",
            "Neither pkexec nor sudo found.\n\n"
            "You will not be able to apply rules without privilege escalation.\n"
            "Please install polkit (pkexec) or sudo."
        );
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("udevme");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("udevme");
    app.setDesktopFileName("udevme");
    
    // First run check
    if (!checkFirstRun()) {
        return 1;
    }
    
    // Show any startup warnings
    showStartupWarnings();
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
