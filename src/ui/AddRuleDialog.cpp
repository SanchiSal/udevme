#include "AddRuleDialog.h"
#include "DeviceScanner.h"
#include "AppScanner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QApplication>
#include <QStyle>

namespace udevme {

AddRuleDialog::AddRuleDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add udev Rule");
    setMinimumSize(700, 600);
    resize(850, 650);
    
    setupUi();
    loadDevices();
    loadApplications();
}

void AddRuleDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Top section: Device and App lists side by side
    QHBoxLayout* listsLayout = new QHBoxLayout();
    
    // Device list group
    QGroupBox* deviceGroup = new QGroupBox("Devices (select one or more)", this);
    QVBoxLayout* deviceLayout = new QVBoxLayout(deviceGroup);
    
    QHBoxLayout* deviceSearchLayout = new QHBoxLayout();
    m_deviceSearch = new QLineEdit(this);
    m_deviceSearch->setPlaceholderText("Search devices...");
    m_deviceSearch->setClearButtonEnabled(true);
    deviceSearchLayout->addWidget(m_deviceSearch);
    
    m_refreshDevicesBtn = new QPushButton(this);
    m_refreshDevicesBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_refreshDevicesBtn->setToolTip("Refresh device list");
    m_refreshDevicesBtn->setFixedWidth(32);
    connect(m_refreshDevicesBtn, &QPushButton::clicked, this, &AddRuleDialog::refreshDevices);
    deviceSearchLayout->addWidget(m_refreshDevicesBtn);
    
    deviceLayout->addLayout(deviceSearchLayout);
    
    m_deviceList = new QListWidget(this);
    m_deviceList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    deviceLayout->addWidget(m_deviceList);
    
    listsLayout->addWidget(deviceGroup);
    
    // App list group
    QGroupBox* appGroup = new QGroupBox("Applications (optional - reminder of what app this rule is for)", this);
    QVBoxLayout* appLayout = new QVBoxLayout(appGroup);
    
    m_appSearch = new QLineEdit(this);
    m_appSearch->setPlaceholderText("Search applications...");
    m_appSearch->setClearButtonEnabled(true);
    appLayout->addWidget(m_appSearch);
    
    m_appList = new QListWidget(this);
    m_appList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    appLayout->addWidget(m_appList);
    
    listsLayout->addWidget(appGroup);
    
    mainLayout->addLayout(listsLayout, 1);
    
    // Notes section
    QGroupBox* notesGroup = new QGroupBox("Notes (optional)", this);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);
    m_notesEdit = new QPlainTextEdit(this);
    m_notesEdit->setPlaceholderText("Add any notes about this rule...");
    m_notesEdit->setMaximumHeight(80);
    notesLayout->addWidget(m_notesEdit);
    mainLayout->addWidget(notesGroup);
    
    // Info label
    m_warningLabel = new QLabel(this);
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setText(
        "<b>How it works:</b> This creates a udev rule with <code>TAG+=\"uaccess\"</code> which grants "
        "the logged-in user access to the device for WebHID in browsers.<br>"
        "<b>Note:</b> The app selection above is just a reminder for you - udev rules apply system-wide."
    );
    mainLayout->addWidget(m_warningLabel);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelBtn);
    
    m_addBtn = new QPushButton("Add", this);
    m_addBtn->setEnabled(false);
    m_addBtn->setDefault(true);
    connect(m_addBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_addBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_deviceSearch, &QLineEdit::textChanged, this, &AddRuleDialog::onDeviceSearchChanged);
    connect(m_appSearch, &QLineEdit::textChanged, this, &AddRuleDialog::onAppSearchChanged);
    connect(m_deviceList, &QListWidget::itemSelectionChanged, this, &AddRuleDialog::onSelectionChanged);
}

void AddRuleDialog::setRule(const UdevRule& rule) {
    m_editMode = true;
    m_editRuleId = rule.id;
    m_editCreatedAt = rule.createdAt;
    
    setWindowTitle("Edit udev Rule");
    m_addBtn->setText("Save");
    
    // Set notes
    m_notesEdit->setPlainText(rule.notes);
    
    // Select devices that match the rule
    for (int i = 0; i < m_deviceList->count(); ++i) {
        QListWidgetItem* item = m_deviceList->item(i);
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_allDevices.size()) {
            const DeviceInfo& dev = m_allDevices[idx];
            for (const auto& ruleDev : rule.devices) {
                if (dev.vendorId.compare(ruleDev.vendorId, Qt::CaseInsensitive) == 0 &&
                    dev.productId.compare(ruleDev.productId, Qt::CaseInsensitive) == 0) {
                    item->setSelected(true);
                    break;
                }
            }
        }
    }
    
    // Select apps that match the rule
    for (int i = 0; i < m_appList->count(); ++i) {
        QListWidgetItem* item = m_appList->item(i);
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_allApps.size()) {
            const AppInfo& app = m_allApps[idx];
            for (const auto& ruleApp : rule.applications) {
                if (app.desktopId == ruleApp.desktopId) {
                    item->setSelected(true);
                    break;
                }
            }
        }
    }
    
    updateAddButton();
}

void AddRuleDialog::loadDevices() {
    m_deviceList->clear();
    m_allDevices.clear();
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    DeviceScanner scanner;
    m_allDevices = scanner.scanDevices();
    
    for (const auto& dev : m_allDevices) {
        QString text = QString("%1 (%2:%3)")
            .arg(dev.displayName(), dev.vendorId, dev.productId);
        
        if (dev.hasHidraw) {
            text += " [hidraw]";
        }
        
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, QVariant::fromValue(m_deviceList->count()));
        item->setToolTip(QString("Vendor: %1\nProduct: %2\nName: %3\nManufacturer: %4\nHidraw: %5")
            .arg(dev.vendorId, dev.productId, dev.name, dev.manufacturer,
                 dev.hasHidraw ? "Yes" : "No"));
        m_deviceList->addItem(item);
    }
    
    QApplication::restoreOverrideCursor();
}

void AddRuleDialog::loadApplications() {
    m_appList->clear();
    m_allApps.clear();
    
    AppScanner scanner;
    m_allApps = scanner.scanApplications();
    
    for (int i = 0; i < m_allApps.size(); ++i) {
        const auto& app = m_allApps[i];
        QString text = app.name;
        if (!app.genericName.isEmpty() && app.genericName != app.name) {
            text += QString(" (%1)").arg(app.genericName);
        }
        
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString("Desktop ID: %1\nExec: %2")
            .arg(app.desktopId, app.exec));
        m_appList->addItem(item);
    }
}

void AddRuleDialog::refreshDevices() {
    loadDevices();
}

void AddRuleDialog::onDeviceSearchChanged(const QString& text) {
    filterDeviceList(text);
}

void AddRuleDialog::onAppSearchChanged(const QString& text) {
    filterAppList(text);
}

void AddRuleDialog::filterDeviceList(const QString& filter) {
    for (int i = 0; i < m_deviceList->count(); ++i) {
        QListWidgetItem* item = m_deviceList->item(i);
        bool match = filter.isEmpty() || 
            item->text().contains(filter, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

void AddRuleDialog::filterAppList(const QString& filter) {
    for (int i = 0; i < m_appList->count(); ++i) {
        QListWidgetItem* item = m_appList->item(i);
        bool match = filter.isEmpty() || 
            item->text().contains(filter, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

void AddRuleDialog::onSelectionChanged() {
    updateAddButton();
}

void AddRuleDialog::updateAddButton() {
    bool hasDeviceSelected = !m_deviceList->selectedItems().isEmpty();
    m_addBtn->setEnabled(hasDeviceSelected);
}

UdevRule AddRuleDialog::getRule() const {
    UdevRule rule;
    
    // Preserve ID and created date if editing
    if (m_editMode) {
        rule.id = m_editRuleId;
        rule.createdAt = m_editCreatedAt;
    }
    
    // Get selected devices
    for (QListWidgetItem* item : m_deviceList->selectedItems()) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_allDevices.size()) {
            rule.devices.append(m_allDevices[idx]);
        }
    }
    
    // Get selected apps
    for (QListWidgetItem* item : m_appList->selectedItems()) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_allApps.size()) {
            rule.applications.append(m_allApps[idx]);
        }
    }
    
    // Get notes
    rule.notes = m_notesEdit->toPlainText().trimmed();
    
    // Use standard WebHID settings - uaccess only
    rule.permissionLevel = PermissionLevel::Open;
    rule.ruleTypes.hidraw = true;
    rule.ruleTypes.usb = false;
    rule.ruleTypes.seat = false;
    rule.ruleTypes.uaccess = true;
    
    return rule;
}

} // namespace udevme
