#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QString>
#include <QVector>
#include <QMap>
#include "Types.h"

namespace udevme {

class ConfigStore {
public:
    static QString getInstallDir();
    static QString getConfigPath();
    static QString getNotesPath();
    static QString getStagedRulesPath();
    static QString getSystemRulesPath();
    
    struct LoadResult {
        QVector<UdevRule> rules;
        SyncInfo syncInfo;
        bool loadedFromSystem = false;
        bool success = false;
        QString error;
        QString warning;
    };
    
    static LoadResult load();
    static bool saveConfig(const QVector<UdevRule>& rules, const SyncInfo& syncInfo);
    static bool saveStagedRules(const QString& content);
    
    // Notes management (stored separately from rules)
    static QMap<QString, QString> loadNotes();
    static bool saveNotes(const QMap<QString, QString>& notes);
    static bool saveNote(const QString& ruleId, const QString& note);
    static QString getNote(const QString& ruleId);
    static bool deleteNote(const QString& ruleId);
    static void cleanupOrphanedNotes(const QVector<UdevRule>& currentRules);
    
    static QString readSystemRules();
    static QString computeSystemRulesHash();
    static bool systemRulesExist();
    
    static bool ensureInstallDir();
    
private:
    static constexpr int SCHEMA_VERSION = 1;
};

} // namespace udevme

#endif // CONFIGSTORE_H
