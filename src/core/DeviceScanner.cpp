#include "DeviceScanner.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

namespace udevme {

DeviceScanner::DeviceScanner(QObject* parent) : QObject(parent) {}

bool DeviceScanner::commandExists(const QString& cmd) const {
    QProcess proc;
    proc.start("which", {cmd});
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

bool DeviceScanner::isUdevadmAvailable() const {
    return commandExists("udevadm");
}

bool DeviceScanner::isLsusbAvailable() const {
    return commandExists("lsusb");
}

QString DeviceScanner::runCommand(const QString& cmd, const QStringList& args) {
    QProcess proc;
    proc.start(cmd, args);
    proc.waitForFinished(10000);
    return QString::fromUtf8(proc.readAllStandardOutput());
}

QVector<DeviceInfo> DeviceScanner::scanDevices() {
    emit scanProgress("Scanning USB devices...");
    
    QVector<DeviceInfo> devices;
    
    // Primary: scan /sys/bus/usb/devices
    devices = scanViaSys();
    
    // Enrich with hidraw info
    enrichWithHidrawInfo(devices);
    
    // Remove duplicates by vid:pid
    QVector<DeviceInfo> unique;
    QSet<QString> seen;
    for (const auto& d : devices) {
        QString key = d.vidPid();
        if (!seen.contains(key) && !d.vendorId.isEmpty() && !d.productId.isEmpty()) {
            seen.insert(key);
            unique.append(d);
        }
    }
    
    emit scanComplete(unique);
    return unique;
}

QVector<DeviceInfo> DeviceScanner::scanViaSys() {
    QVector<DeviceInfo> devices;
    QDir usbDir("/sys/bus/usb/devices");
    
    for (const QString& entry : usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString path = usbDir.filePath(entry);
        
        // Read vendor and product IDs
        QFile vidFile(path + "/idVendor");
        QFile pidFile(path + "/idProduct");
        
        if (!vidFile.exists() || !pidFile.exists()) continue;
        
        DeviceInfo dev;
        
        if (vidFile.open(QIODevice::ReadOnly)) {
            dev.vendorId = QString::fromUtf8(vidFile.readAll()).trimmed();
            vidFile.close();
        }
        
        if (pidFile.open(QIODevice::ReadOnly)) {
            dev.productId = QString::fromUtf8(pidFile.readAll()).trimmed();
            pidFile.close();
        }
        
        // Skip root hubs and empty IDs
        if (dev.vendorId.isEmpty() || dev.productId.isEmpty()) continue;
        if (dev.vendorId == "1d6b") continue; // Linux Foundation root hub
        
        // Read product name
        QFile prodFile(path + "/product");
        if (prodFile.open(QIODevice::ReadOnly)) {
            dev.name = QString::fromUtf8(prodFile.readAll()).trimmed();
            prodFile.close();
        }
        
        // Read manufacturer
        QFile mfgFile(path + "/manufacturer");
        if (mfgFile.open(QIODevice::ReadOnly)) {
            dev.manufacturer = QString::fromUtf8(mfgFile.readAll()).trimmed();
            mfgFile.close();
        }
        
        dev.sysPath = path;
        dev.hasUsb = true;
        
        devices.append(dev);
    }
    
    return devices;
}

QVector<DeviceInfo> DeviceScanner::scanViaUdevadm() {
    QVector<DeviceInfo> devices;
    
    if (!isUdevadmAvailable()) return devices;
    
    QString output = runCommand("udevadm", {"info", "--export-db"});
    
    DeviceInfo current;
    bool inUsbDevice = false;
    
    for (const QString& line : output.split('\n')) {
        if (line.startsWith("P:")) {
            // New device path
            if (inUsbDevice && !current.vendorId.isEmpty()) {
                devices.append(current);
            }
            current = DeviceInfo();
            inUsbDevice = line.contains("/usb");
        } else if (line.startsWith("E:")) {
            QString prop = line.mid(2).trimmed();
            int eq = prop.indexOf('=');
            if (eq > 0) {
                QString key = prop.left(eq);
                QString val = prop.mid(eq + 1);
                
                if (key == "ID_VENDOR_ID") current.vendorId = val;
                else if (key == "ID_MODEL_ID") current.productId = val;
                else if (key == "ID_MODEL") current.name = val.replace('_', ' ');
                else if (key == "ID_VENDOR") current.manufacturer = val.replace('_', ' ');
            }
        }
    }
    
    if (inUsbDevice && !current.vendorId.isEmpty()) {
        devices.append(current);
    }
    
    return devices;
}

void DeviceScanner::enrichWithHidrawInfo(QVector<DeviceInfo>& devices) {
    // Scan /sys/class/hidraw to find which devices have hidraw
    QDir hidrawDir("/sys/class/hidraw");
    
    if (!hidrawDir.exists()) return;
    
    for (const QString& entry : hidrawDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString devicePath = hidrawDir.filePath(entry + "/device");
        QFileInfo linkInfo(devicePath);
        
        if (!linkInfo.isSymLink()) continue;
        
        QString realPath = linkInfo.symLinkTarget();
        
        // Walk up to find USB device info
        QDir parent(realPath);
        for (int i = 0; i < 10; ++i) {
            parent.cdUp();
            QString vidPath = parent.filePath("idVendor");
            QString pidPath = parent.filePath("idProduct");
            
            if (QFile::exists(vidPath) && QFile::exists(pidPath)) {
                QString vid, pid;
                QFile vf(vidPath);
                if (vf.open(QIODevice::ReadOnly)) {
                    vid = QString::fromUtf8(vf.readAll()).trimmed();
                    vf.close();
                }
                QFile pf(pidPath);
                if (pf.open(QIODevice::ReadOnly)) {
                    pid = QString::fromUtf8(pf.readAll()).trimmed();
                    pf.close();
                }
                
                // Mark matching device as having hidraw
                for (auto& dev : devices) {
                    if (dev.vendorId.compare(vid, Qt::CaseInsensitive) == 0 &&
                        dev.productId.compare(pid, Qt::CaseInsensitive) == 0) {
                        dev.hasHidraw = true;
                    }
                }
                break;
            }
        }
    }
}

} // namespace udevme
