#ifndef RULEMODEL_H
#define RULEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "Types.h"

namespace udevme {

class RuleModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColEnabled = 0,
        ColDevices,
        ColApps,
        ColNotes,
        ColCount
    };

    explicit RuleModel(QObject* parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    // Rule management
    void addRule(const UdevRule& rule);
    void removeRule(int row);
    void removeRules(const QVector<int>& rows);
    void updateRule(int row, const UdevRule& rule);
    UdevRule getRule(int row) const;
    QVector<UdevRule> getAllRules() const;
    QVector<UdevRule> getEnabledRules() const;
    void setRules(const QVector<UdevRule>& rules);
    void clear();
    
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty);
    void clearDirty() { setDirty(false); }

signals:
    void dirtyChanged(bool dirty);
    void rulesChanged();

private:
    QVector<UdevRule> m_rules;
    bool m_dirty = false;
};

} // namespace udevme

#endif // RULEMODEL_H
