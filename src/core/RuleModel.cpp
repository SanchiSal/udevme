#include "RuleModel.h"

namespace udevme {

RuleModel::RuleModel(QObject* parent) : QAbstractTableModel(parent) {}

int RuleModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_rules.size();
}

int RuleModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant RuleModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rules.size()) {
        return QVariant();
    }

    const UdevRule& rule = m_rules[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColEnabled: return QVariant();
            case ColDevices: return rule.devicesSummary();
            case ColApps: return rule.appsSummary();
            case ColNotes: return rule.notes;
        }
    } else if (role == Qt::CheckStateRole && index.column() == ColEnabled) {
        return rule.enabled ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::ToolTipRole) {
        switch (index.column()) {
            case ColDevices: {
                QStringList tips;
                for (const auto& d : rule.devices) {
                    tips << QString("%1 (%2:%3)%4")
                        .arg(d.displayName(), d.vendorId, d.productId,
                             d.hasHidraw ? " [hidraw]" : "");
                }
                return tips.join("\n");
            }
            case ColApps: {
                if (rule.applications.isEmpty()) {
                    return "No app selected (reminder only)";
                }
                QStringList tips;
                for (const auto& a : rule.applications) {
                    tips << QString("%1 (%2)").arg(a.name, a.desktopId);
                }
                return tips.join("\n");
            }
        }
    }

    return QVariant();
}

QVariant RuleModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case ColEnabled: return "Enabled";
        case ColDevices: return "Devices";
        case ColApps: return "For App";
        case ColNotes: return "Notes";
    }
    return QVariant();
}

Qt::ItemFlags RuleModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    
    if (index.column() == ColEnabled) {
        f |= Qt::ItemIsUserCheckable;
    }
    
    return f;
}

bool RuleModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_rules.size()) {
        return false;
    }

    if (role == Qt::CheckStateRole && index.column() == ColEnabled) {
        m_rules[index.row()].enabled = (value.toInt() == Qt::Checked);
        m_rules[index.row()].updatedAt = QDateTime::currentDateTime();
        setDirty(true);
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

void RuleModel::addRule(const UdevRule& rule) {
    beginInsertRows(QModelIndex(), m_rules.size(), m_rules.size());
    m_rules.append(rule);
    endInsertRows();
    setDirty(true);
    emit rulesChanged();
}

void RuleModel::removeRule(int row) {
    if (row < 0 || row >= m_rules.size()) return;
    
    beginRemoveRows(QModelIndex(), row, row);
    m_rules.removeAt(row);
    endRemoveRows();
    setDirty(true);
    emit rulesChanged();
}

void RuleModel::removeRules(const QVector<int>& rows) {
    if (rows.isEmpty()) return;
    
    // Sort descending to remove from end first
    QVector<int> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    
    for (int row : sorted) {
        if (row >= 0 && row < m_rules.size()) {
            beginRemoveRows(QModelIndex(), row, row);
            m_rules.removeAt(row);
            endRemoveRows();
        }
    }
    setDirty(true);
    emit rulesChanged();
}

void RuleModel::updateRule(int row, const UdevRule& rule) {
    if (row < 0 || row >= m_rules.size()) return;
    
    m_rules[row] = rule;
    m_rules[row].updatedAt = QDateTime::currentDateTime();
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
    setDirty(true);
    emit rulesChanged();
}

UdevRule RuleModel::getRule(int row) const {
    if (row < 0 || row >= m_rules.size()) return UdevRule();
    return m_rules[row];
}

QVector<UdevRule> RuleModel::getAllRules() const {
    return m_rules;
}

QVector<UdevRule> RuleModel::getEnabledRules() const {
    QVector<UdevRule> enabled;
    for (const auto& r : m_rules) {
        if (r.enabled) enabled.append(r);
    }
    return enabled;
}

void RuleModel::setRules(const QVector<UdevRule>& rules) {
    beginResetModel();
    m_rules = rules;
    endResetModel();
    emit rulesChanged();
}

void RuleModel::clear() {
    beginResetModel();
    m_rules.clear();
    endResetModel();
    setDirty(true);
    emit rulesChanged();
}

void RuleModel::setDirty(bool dirty) {
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

} // namespace udevme
