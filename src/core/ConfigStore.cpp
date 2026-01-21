#include "ConfigStore.h"
#include "RuleParser.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSet>

namespace udevme {

QString ConfigStore::getInstallDir() {
    return QDir::homePath() + "/.local/bin/udevme";
}

QString ConfigStore::getConfigPath() {
    return getInstallDir() + "/udevme.json";
}

QString ConfigStore::getNotesPath() {
    return getInstallDir() + "/notes.json";
}

QString ConfigStore::getStagedRulesPath() {
    return getInstallDir() + "/99-udevme.rules";
}

QString ConfigStore::getSystemRulesPath() {
    return "/etc/udev/rules.d/99-udevme.rules";
}

bool ConfigStore::ensureInstallDir() {
    QDir dir(getInstallDir());
    if (!dir.exists()) {
        return dir.mkpath(".");
    }
    return true;
}

bool ConfigStore::systemRulesExist() {
    return QFile::exists(getSystemRulesPath());
}

QString ConfigStore::readSystemRules() {
    QFile file(getSystemRulesPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

QString ConfigStore::computeSystemRulesHash() {
    QString content = readSystemRules();
    if (content.isEmpty()) return QString();
    return RuleParser::computeHash(content);
}

// Notes management
QMap<QString, QString> ConfigStore::loadNotes() {
    QMap<QString, QString> notes;
    
    QFile file(getNotesPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return notes;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        notes[it.key()] = it.value().toString();
    }
    
    return notes;
}

bool ConfigStore::saveNotes(const QMap<QString, QString>& notes) {
    ensureInstallDir();
    
    QJsonObject root;
    for (auto it = notes.begin(); it != notes.end(); ++it) {
        if (!it.value().isEmpty()) {
            root[it.key()] = it.value();
        }
    }
    
    QJsonDocument doc(root);
    
    QFile file(getNotesPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ConfigStore::saveNote(const QString& ruleId, const QString& note) {
    QMap<QString, QString> notes = loadNotes();
    if (note.isEmpty()) {
        notes.remove(ruleId);
    } else {
        notes[ruleId] = note;
    }
    return saveNotes(notes);
}

QString ConfigStore::getNote(const QString& ruleId) {
    QMap<QString, QString> notes = loadNotes();
    return notes.value(ruleId);
}

bool ConfigStore::deleteNote(const QString& ruleId) {
    QMap<QString, QString> notes = loadNotes();
    if (notes.contains(ruleId)) {
        notes.remove(ruleId);
        return saveNotes(notes);
    }
    return true;
}

void ConfigStore::cleanupOrphanedNotes(const QVector<UdevRule>& currentRules) {
    QMap<QString, QString> notes = loadNotes();
    if (notes.isEmpty()) return;
    
    // Build set of current rule IDs
    QSet<QString> ruleIds;
    for (const auto& rule : currentRules) {
        ruleIds.insert(rule.id.toString(QUuid::WithoutBraces));
    }
    
    // Remove notes for rules that no longer exist
    QStringList toRemove;
    for (auto it = notes.begin(); it != notes.end(); ++it) {
        if (!ruleIds.contains(it.key())) {
            toRemove.append(it.key());
        }
    }
    
    if (!toRemove.isEmpty()) {
        for (const QString& id : toRemove) {
            notes.remove(id);
        }
        saveNotes(notes);
    }
}

ConfigStore::LoadResult ConfigStore::load() {
    LoadResult result;
    result.success = true;
    
    ensureInstallDir();
    
    // Load notes first
    QMap<QString, QString> notes = loadNotes();
    
    // Check if system rules exist - they are source of truth
    if (systemRulesExist()) {
        QString systemContent = readSystemRules();
        QString systemHash = RuleParser::computeHash(systemContent);
        
        // Parse system rules
        auto parseResult = RuleParser::parseRulesFile(systemContent);
        
        if (parseResult.success) {
            result.rules = parseResult.rules;
            result.loadedFromSystem = true;
            result.syncInfo.rulesFileHashAtLoad = systemHash;
            result.syncInfo.lastSyncedFromRulesAt = QDateTime::currentDateTime();
            
            // Apply notes to rules
            for (auto& rule : result.rules) {
                QString ruleId = rule.id.toString(QUuid::WithoutBraces);
                if (notes.contains(ruleId)) {
                    rule.notes = notes[ruleId];
                }
            }
            
            // Check if we have a config file with different hash
            QFile configFile(getConfigPath());
            if (configFile.exists() && configFile.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
                configFile.close();
                
                QJsonObject root = doc.object();
                QString savedHash = root["sync_info"].toObject()["rules_file_hash_at_load"].toString();
                
                if (!savedHash.isEmpty() && savedHash != systemHash) {
                    result.warning = "System rules differ from saved config; loaded system rules.";
                }
            }
            
            // Update config to match system rules
            saveConfig(result.rules, result.syncInfo);
            
            for (const QString& w : parseResult.warnings) {
                if (!result.warning.isEmpty()) result.warning += "\n";
                result.warning += w;
            }
            
            return result;
        }
    }
    
    // No system rules, try loading from config
    QFile configFile(getConfigPath());
    if (!configFile.exists()) {
        // Fresh start
        result.rules.clear();
        result.syncInfo = SyncInfo();
        return result;
    }
    
    if (!configFile.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.error = "Cannot open config file: " + getConfigPath();
        return result;
    }
    
    QByteArray data = configFile.readAll();
    configFile.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        result.success = false;
        result.error = "JSON parse error: " + parseError.errorString();
        return result;
    }
    
    QJsonObject root = doc.object();
    
    // Load rules
    QJsonArray rulesArray = root["rules"].toArray();
    for (const QJsonValue& v : rulesArray) {
        result.rules.append(UdevRule::fromJson(v.toObject()));
    }
    
    // Apply notes to rules
    for (auto& rule : result.rules) {
        QString ruleId = rule.id.toString(QUuid::WithoutBraces);
        if (notes.contains(ruleId)) {
            rule.notes = notes[ruleId];
        }
    }
    
    // Load sync info
    result.syncInfo = SyncInfo::fromJson(root["sync_info"].toObject());
    
    return result;
}

bool ConfigStore::saveConfig(const QVector<UdevRule>& rules, const SyncInfo& syncInfo) {
    ensureInstallDir();
    
    // Save notes separately
    QMap<QString, QString> notes;
    for (const auto& rule : rules) {
        if (!rule.notes.isEmpty()) {
            notes[rule.id.toString(QUuid::WithoutBraces)] = rule.notes;
        }
    }
    saveNotes(notes);
    
    // Cleanup orphaned notes
    cleanupOrphanedNotes(rules);
    
    // Save config (without notes in rule JSON - they're stored separately now)
    QJsonObject root;
    root["schema_version"] = SCHEMA_VERSION;
    
    QJsonArray rulesArray;
    for (const auto& rule : rules) {
        QJsonObject ruleObj = rule.toJson();
        ruleObj.remove("notes"); // Don't store notes in config, they're in notes.json
        rulesArray.append(ruleObj);
    }
    root["rules"] = rulesArray;
    root["sync_info"] = syncInfo.toJson();
    
    QJsonDocument doc(root);
    
    QFile file(getConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ConfigStore::saveStagedRules(const QString& content) {
    ensureInstallDir();
    
    QFile file(getStagedRulesPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    
    file.write(content.toUtf8());
    file.close();
    return true;
}

} // namespace udevme
