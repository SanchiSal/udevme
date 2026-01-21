#ifndef RULEPARSER_H
#define RULEPARSER_H

#include <QString>
#include <QVector>
#include "Types.h"

namespace udevme {

class RuleParser {
public:
    struct ParseResult {
        QVector<UdevRule> rules;
        QStringList warnings;
        bool success = false;
    };
    
    static ParseResult parseRulesFile(const QString& content);
    static ParseResult parseRulesFromPath(const QString& path);
    static QString computeHash(const QString& content);
    
private:
    static UdevRule parseMetadataComment(const QString& comment);
    static bool isUdevmeComment(const QString& line);
    static DeviceInfo parseDeviceFromRule(const QString& line);
};

} // namespace udevme

#endif // RULEPARSER_H
