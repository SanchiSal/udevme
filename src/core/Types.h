#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QVector>
#include <QUuid>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace udevme {

struct DeviceInfo {
    QString vendorId;
    QString productId;
    QString name;
    QString manufacturer;
    QString sysPath;
    bool hasHidraw = false;
    bool hasUsb = false;

    QString displayName() const {
        if (!name.isEmpty()) return name;
        if (!manufacturer.isEmpty()) return manufacturer;
        return QString("%1:%2").arg(vendorId, productId);
    }

    QString vidPid() const {
        return QString("%1:%2").arg(vendorId, productId);
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["vid"] = vendorId;
        obj["pid"] = productId;
        obj["name"] = name;
        obj["manufacturer"] = manufacturer;
        obj["has_hidraw"] = hasHidraw;
        obj["has_usb"] = hasUsb;
        return obj;
    }

    static DeviceInfo fromJson(const QJsonObject& obj) {
        DeviceInfo d;
        d.vendorId = obj["vid"].toString();
        d.productId = obj["pid"].toString();
        d.name = obj["name"].toString();
        d.manufacturer = obj["manufacturer"].toString();
        d.hasHidraw = obj["has_hidraw"].toBool();
        d.hasUsb = obj["has_usb"].toBool();
        return d;
    }

    bool operator==(const DeviceInfo& other) const {
        return vendorId == other.vendorId && productId == other.productId;
    }
};

struct AppInfo {
    QString desktopId;
    QString name;
    QString exec;
    QString icon;
    QString genericName;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["desktop_id"] = desktopId;
        obj["name"] = name;
        obj["exec"] = exec;
        obj["icon"] = icon;
        return obj;
    }

    static AppInfo fromJson(const QJsonObject& obj) {
        AppInfo a;
        a.desktopId = obj["desktop_id"].toString();
        a.name = obj["name"].toString();
        a.exec = obj["exec"].toString();
        a.icon = obj["icon"].toString();
        return a;
    }

    bool operator==(const AppInfo& other) const {
        return desktopId == other.desktopId;
    }
};

enum class PermissionLevel {
    Safe,       // TAG+="uaccess", TAG+="seat"
    Balanced,   // TAG+="uaccess", MODE="0660", GROUP="plugdev"
    Open        // MODE="0666", TAG+="uaccess"
};

inline QString permissionLevelToString(PermissionLevel level) {
    switch (level) {
        case PermissionLevel::Safe: return "Safe";
        case PermissionLevel::Balanced: return "Balanced";
        case PermissionLevel::Open: return "Open";
    }
    return "Safe";
}

inline PermissionLevel permissionLevelFromString(const QString& str) {
    if (str == "Balanced") return PermissionLevel::Balanced;
    if (str == "Open") return PermissionLevel::Open;
    return PermissionLevel::Safe;
}

struct RuleTypes {
    bool hidraw = true;
    bool usb = false;
    bool uaccess = true;
    bool seat = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["hidraw"] = hidraw;
        obj["usb"] = usb;
        obj["uaccess"] = uaccess;
        obj["seat"] = seat;
        return obj;
    }

    static RuleTypes fromJson(const QJsonObject& obj) {
        RuleTypes t;
        t.hidraw = obj["hidraw"].toBool(true);
        t.usb = obj["usb"].toBool(false);
        t.uaccess = obj["uaccess"].toBool(true);
        t.seat = obj["seat"].toBool(false);
        return t;
    }

    QString summary() const {
        QStringList parts;
        if (hidraw) parts << "hidraw";
        if (usb) parts << "usb";
        return parts.join(", ");
    }
};

struct UdevRule {
    QUuid id;
    QVector<DeviceInfo> devices;
    QVector<AppInfo> applications;
    PermissionLevel permissionLevel = PermissionLevel::Safe;
    RuleTypes ruleTypes;
    bool enabled = true;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;

    UdevRule() : id(QUuid::createUuid()), createdAt(QDateTime::currentDateTime()), updatedAt(createdAt) {}

    QString devicesSummary() const {
        if (devices.isEmpty()) return "(none)";
        QStringList names;
        for (const auto& d : devices) {
            names << QString("%1 (%2)").arg(d.displayName(), d.vidPid());
        }
        return names.join("; ");
    }

    QString appsSummary() const {
        if (applications.isEmpty()) return "(all)";
        QStringList names;
        for (const auto& a : applications) {
            names << a.name;
        }
        return names.join(", ");
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id.toString(QUuid::WithoutBraces);
        
        QJsonArray devArr;
        for (const auto& d : devices) devArr.append(d.toJson());
        obj["devices"] = devArr;
        
        QJsonArray appArr;
        for (const auto& a : applications) appArr.append(a.toJson());
        obj["applications"] = appArr;
        
        obj["permission_level"] = permissionLevelToString(permissionLevel);
        obj["rule_types"] = ruleTypes.toJson();
        obj["enabled"] = enabled;
        obj["notes"] = notes;
        obj["created_at"] = createdAt.toString(Qt::ISODate);
        obj["updated_at"] = updatedAt.toString(Qt::ISODate);
        return obj;
    }

    static UdevRule fromJson(const QJsonObject& obj) {
        UdevRule r;
        r.id = QUuid::fromString(obj["id"].toString());
        if (r.id.isNull()) r.id = QUuid::createUuid();
        
        for (const auto& v : obj["devices"].toArray()) {
            r.devices.append(DeviceInfo::fromJson(v.toObject()));
        }
        for (const auto& v : obj["applications"].toArray()) {
            r.applications.append(AppInfo::fromJson(v.toObject()));
        }
        
        r.permissionLevel = permissionLevelFromString(obj["permission_level"].toString());
        r.ruleTypes = RuleTypes::fromJson(obj["rule_types"].toObject());
        r.enabled = obj["enabled"].toBool(true);
        r.notes = obj["notes"].toString();
        r.createdAt = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
        r.updatedAt = QDateTime::fromString(obj["updated_at"].toString(), Qt::ISODate);
        if (!r.createdAt.isValid()) r.createdAt = QDateTime::currentDateTime();
        if (!r.updatedAt.isValid()) r.updatedAt = r.createdAt;
        return r;
    }
};

struct SyncInfo {
    QString rulesFileHashAtLoad;
    QDateTime lastSyncedFromRulesAt;
    QDateTime lastAppliedAt;
    QString rulesFileHashAfterApply;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["rules_file_hash_at_load"] = rulesFileHashAtLoad;
        obj["last_synced_from_rules_at"] = lastSyncedFromRulesAt.toString(Qt::ISODate);
        obj["last_applied_at"] = lastAppliedAt.toString(Qt::ISODate);
        obj["rules_file_hash_after_apply"] = rulesFileHashAfterApply;
        return obj;
    }

    static SyncInfo fromJson(const QJsonObject& obj) {
        SyncInfo s;
        s.rulesFileHashAtLoad = obj["rules_file_hash_at_load"].toString();
        s.lastSyncedFromRulesAt = QDateTime::fromString(obj["last_synced_from_rules_at"].toString(), Qt::ISODate);
        s.lastAppliedAt = QDateTime::fromString(obj["last_applied_at"].toString(), Qt::ISODate);
        s.rulesFileHashAfterApply = obj["rules_file_hash_after_apply"].toString();
        return s;
    }
};

} // namespace udevme

#endif // TYPES_H
