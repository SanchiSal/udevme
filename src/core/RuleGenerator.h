#ifndef RULEGENERATOR_H
#define RULEGENERATOR_H

#include <QString>
#include <QVector>
#include "Types.h"

namespace udevme {

class RuleGenerator {
public:
    static QString generateRulesFile(const QVector<UdevRule>& rules);
    static QString generateSingleRule(const UdevRule& rule);
    static QString generateMetadataComment(const UdevRule& rule);
    static bool checkPlugdevGroup();
    
private:
    static QString generatePermissionPart(const UdevRule& rule);
};

} // namespace udevme

#endif // RULEGENERATOR_H
