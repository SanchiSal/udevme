#ifndef DEVICESCANNER_H
#define DEVICESCANNER_H

#include <QObject>
#include <QVector>
#include "Types.h"

namespace udevme {

class DeviceScanner : public QObject {
    Q_OBJECT
public:
    explicit DeviceScanner(QObject* parent = nullptr);
    
    QVector<DeviceInfo> scanDevices();
    bool isUdevadmAvailable() const;
    bool isLsusbAvailable() const;

signals:
    void scanProgress(const QString& message);
    void scanComplete(const QVector<DeviceInfo>& devices);
    void scanError(const QString& error);

private:
    QVector<DeviceInfo> scanViaSys();
    QVector<DeviceInfo> scanViaUdevadm();
    void enrichWithHidrawInfo(QVector<DeviceInfo>& devices);
    QString runCommand(const QString& cmd, const QStringList& args);
    bool commandExists(const QString& cmd) const;
};

} // namespace udevme

#endif // DEVICESCANNER_H
