#include "RuleParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCryptographicHash>

namespace udevme {

QString RuleParser::computeHash(const QString& content) {
    QByteArray hash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

bool RuleParser::isUdevmeComment(const QString& line) {
    return line.trimmed().startsWith("# udevme:");
}

UdevRule RuleParser::parseMetadataComment(const QString& comment) {
    UdevRule rule;
    
    // Parse: # udevme: id=<uuid> devices=<vid:pid,...> apps=<...> level=<...> types=<...> enabled=<bool>
    // Notes are stored separately in notes.json, not in the rules file
    QString line = comment.mid(comment.indexOf("udevme:") + 7).trimmed();
    
    // Extract key=value pairs
    QRegularExpression kvRe("(\\w+)=([^\\s]+|\"[^\"]*\")");
    QRegularExpressionMatchIterator it = kvRe.globalMatch(line);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);
        
        // Remove quotes if present
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }
        
        if (key == "id") {
            rule.id = QUuid::fromString(value);
            if (rule.id.isNull()) rule.id = QUuid::createUuid();
        } else if (key == "devices") {
            for (const QString& dev : value.split(',', Qt::SkipEmptyParts)) {
                QStringList parts = dev.split(':');
                if (parts.size() == 2) {
                    DeviceInfo di;
                    di.vendorId = parts[0];
                    di.productId = parts[1];
                    rule.devices.append(di);
                }
            }
        } else if (key == "apps") {
            if (value != "all") {
                for (const QString& app : value.split(',', Qt::SkipEmptyParts)) {
                    AppInfo ai;
                    ai.desktopId = app;
                    ai.name = QString(app).replace(".desktop", "");
                    rule.applications.append(ai);
                }
            }
        } else if (key == "level") {
            rule.permissionLevel = permissionLevelFromString(value);
        } else if (key == "types") {
            QStringList types = value.split(',', Qt::SkipEmptyParts);
            rule.ruleTypes.hidraw = types.contains("hidraw");
            rule.ruleTypes.usb = types.contains("usb");
            rule.ruleTypes.uaccess = types.contains("uaccess");
            rule.ruleTypes.seat = types.contains("seat");
        } else if (key == "enabled") {
            rule.enabled = (value.toLower() == "true");
        }
        // Notes are loaded from notes.json by ConfigStore, not from rules file
    }
    
    return rule;
}

DeviceInfo RuleParser::parseDeviceFromRule(const QString& line) {
    DeviceInfo dev;
    
    // Extract ATTRS{idVendor} or ATTR{idVendor}
    QRegularExpression vidRe("ATTRS?\\{idVendor\\}==\"([0-9a-fA-F]+)\"");
    QRegularExpressionMatch vidMatch = vidRe.match(line);
    if (vidMatch.hasMatch()) {
        dev.vendorId = vidMatch.captured(1);
    }
    
    // Extract ATTRS{idProduct} or ATTR{idProduct}
    QRegularExpression pidRe("ATTRS?\\{idProduct\\}==\"([0-9a-fA-F]+)\"");
    QRegularExpressionMatch pidMatch = pidRe.match(line);
    if (pidMatch.hasMatch()) {
        dev.productId = pidMatch.captured(1);
    }
    
    // Check subsystem
    if (line.contains("hidraw")) {
        dev.hasHidraw = true;
    }
    if (line.contains("SUBSYSTEM==\"usb\"")) {
        dev.hasUsb = true;
    }
    
    return dev;
}

RuleParser::ParseResult RuleParser::parseRulesFile(const QString& content) {
    ParseResult result;
    result.success = true;
    
    QStringList lines = content.split('\n');
    UdevRule currentRule;
    bool hasCurrentRule = false;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        
        if (line.isEmpty()) continue;
        
        // Check for udevme metadata comment
        if (isUdevmeComment(line)) {
            // Save previous rule if exists
            if (hasCurrentRule && !currentRule.devices.isEmpty()) {
                result.rules.append(currentRule);
            }
            
            // Start new rule from metadata
            currentRule = parseMetadataComment(line);
            hasCurrentRule = true;
            continue;
        }
        
        // Skip other comments
        if (line.startsWith('#')) continue;
        
        // Parse actual udev rule line to extract/verify device info
        if (hasCurrentRule && (line.contains("idVendor") || line.contains("idProduct"))) {
            DeviceInfo dev = parseDeviceFromRule(line);
            
            // Update existing device info or add if not found
            bool found = false;
            for (auto& d : currentRule.devices) {
                if (d.vendorId == dev.vendorId && d.productId == dev.productId) {
                    d.hasHidraw = d.hasHidraw || dev.hasHidraw;
                    d.hasUsb = d.hasUsb || dev.hasUsb;
                    found = true;
                    break;
                }
            }
            
            if (!found && !dev.vendorId.isEmpty() && !dev.productId.isEmpty()) {
                // If we have metadata but device wasn't listed (shouldn't happen), add it
                bool alreadyInRule = false;
                for (const auto& d : currentRule.devices) {
                    if (d.vendorId == dev.vendorId && d.productId == dev.productId) {
                        alreadyInRule = true;
                        break;
                    }
                }
                if (!alreadyInRule) {
                    currentRule.devices.append(dev);
                }
            }
        }
    }
    
    // Save last rule
    if (hasCurrentRule && !currentRule.devices.isEmpty()) {
        result.rules.append(currentRule);
    }
    
    return result;
}

RuleParser::ParseResult RuleParser::parseRulesFromPath(const QString& path) {
    ParseResult result;
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.warnings << QString("Cannot open file: %1").arg(path);
        return result;
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    return parseRulesFile(content);
}

} // namespace udevme
