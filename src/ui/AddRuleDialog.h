#ifndef ADDRULEDIALOG_H
#define ADDRULEDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include "Types.h"

namespace udevme {

class AddRuleDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddRuleDialog(QWidget* parent = nullptr);
    
    // Set existing rule for editing
    void setRule(const UdevRule& rule);
    UdevRule getRule() const;
    
    bool isEditMode() const { return m_editMode; }

private slots:
    void onDeviceSearchChanged(const QString& text);
    void onAppSearchChanged(const QString& text);
    void onSelectionChanged();
    void refreshDevices();

private:
    void setupUi();
    void loadDevices();
    void loadApplications();
    void updateAddButton();
    void filterDeviceList(const QString& filter);
    void filterAppList(const QString& filter);
    
    // Device list
    QLineEdit* m_deviceSearch;
    QListWidget* m_deviceList;
    QPushButton* m_refreshDevicesBtn;
    QVector<DeviceInfo> m_allDevices;
    
    // App list
    QLineEdit* m_appSearch;
    QListWidget* m_appList;
    QVector<AppInfo> m_allApps;
    
    // Notes
    QPlainTextEdit* m_notesEdit;
    
    // Buttons
    QPushButton* m_addBtn;
    QPushButton* m_cancelBtn;
    
    // Info label
    QLabel* m_warningLabel;
    
    // Edit mode
    bool m_editMode = false;
    QUuid m_editRuleId;
    QDateTime m_editCreatedAt;
};

} // namespace udevme

#endif // ADDRULEDIALOG_H
