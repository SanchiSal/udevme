#ifndef APPSCANNER_H
#define APPSCANNER_H

#include <QObject>
#include <QVector>
#include "Types.h"

namespace udevme {

class AppScanner : public QObject {
    Q_OBJECT
public:
    explicit AppScanner(QObject* parent = nullptr);
    
    QVector<AppInfo> scanApplications();
    QVector<AppInfo> getBrowsers() const;

signals:
    void scanProgress(const QString& message);
    void scanComplete(const QVector<AppInfo>& apps);

private:
    AppInfo parseDesktopFile(const QString& path);
    QStringList getDesktopDirs() const;
    
    static const QStringList s_browserDesktopIds;
};

} // namespace udevme

#endif // APPSCANNER_H
