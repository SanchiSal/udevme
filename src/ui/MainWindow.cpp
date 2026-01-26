#include "MainWindow.h"
#include "AddRuleDialog.h"
#include "ConfigStore.h"
#include "RuleGenerator.h"
#include "RuleParser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QSplitter>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QApplication>
#include <QFile>

namespace udevme {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("udevme - udev Rule Manager");
    setMinimumSize(800, 500);
    resize(1000, 600);
    
    setupMenuBar();
    setupUi();
    loadRules();
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* refreshAction = fileMenu->addAction("&Refresh");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::loadRules);
    
    fileMenu->addSeparator();
    
    QAction* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Splitter for table and log
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    
    // Table view
    m_tableView = new QTableView(this);
    m_ruleModel = new RuleModel(this);
    m_tableView->setModel(m_ruleModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setVisible(false);
    
    // Set reasonable column widths
    m_tableView->setColumnWidth(RuleModel::ColEnabled, 70);
    m_tableView->setColumnWidth(RuleModel::ColDevices, 350);
    m_tableView->setColumnWidth(RuleModel::ColApps, 200);
    
    splitter->addWidget(m_tableView);
    
    // Log widget (collapsible)
    m_logWidget = new LogWidget(this);
    splitter->addWidget(m_logWidget);
    
    // Set initial splitter sizes (table gets more space)
    splitter->setSizes({400, 150});
    
    mainLayout->addWidget(splitter, 1);
    
    // Action bar
    QHBoxLayout* actionBar = new QHBoxLayout();
    actionBar->setSpacing(8);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    actionBar->addWidget(m_statusLabel, 1);
    
    m_addBtn = new QPushButton("Add Rule", this);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddRule);
    actionBar->addWidget(m_addBtn);
    
    m_editBtn = new QPushButton("Edit Rule", this);
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &MainWindow::onEditRule);
    actionBar->addWidget(m_editBtn);
    
    m_removeBtn = new QPushButton("Remove Rule", this);
    m_removeBtn->setEnabled(false);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveRule);
    actionBar->addWidget(m_removeBtn);
    
    m_applyBtn = new QPushButton("Apply", this);
    m_applyBtn->setEnabled(false);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    actionBar->addWidget(m_applyBtn);
    
    mainLayout->addLayout(actionBar);
    
    // Connect signals
    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(m_tableView, &QTableView::doubleClicked, this, &MainWindow::onDoubleClicked);
    connect(m_ruleModel, &RuleModel::dirtyChanged, this, &MainWindow::onDirtyChanged);
}

void MainWindow::loadRules() {
    m_logWidget->appendLog("Loading rules...");
    
    auto result = ConfigStore::load();
    
    if (!result.success) {
        QMessageBox::critical(this, "Load Error", result.error);
        m_logWidget->appendLog("ERROR: " + result.error);
        return;
    }
    
    m_ruleModel->setRules(result.rules);
    m_ruleModel->clearDirty();
    m_rulesHashAtLoad = result.syncInfo.rulesFileHashAtLoad;
    
    if (!result.warning.isEmpty()) {
        QMessageBox::warning(this, "Warning", result.warning);
        m_logWidget->appendLog("WARNING: " + result.warning);
    }
    
    QString status;
    if (result.loadedFromSystem) {
        status = QString("Loaded %1 rule(s) from system rules file")
            .arg(result.rules.size());
    } else {
        status = QString("Loaded %1 rule(s) from config")
            .arg(result.rules.size());
    }
    
    updateStatus(status);
    m_logWidget->appendLog(status);
}

void MainWindow::updateStatus(const QString& message) {
    m_statusLabel->setText(message);
}

void MainWindow::onSelectionChanged() {
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    bool hasSelection = !selection.isEmpty();
    bool hasSingleSelection = selection.size() == 1;
    
    m_editBtn->setEnabled(hasSingleSelection);
    m_removeBtn->setEnabled(hasSelection);
}

void MainWindow::onDoubleClicked(const QModelIndex& index) {
    if (index.isValid()) {
        editRuleAtRow(index.row());
    }
}

void MainWindow::onEditRule() {
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    if (selection.size() == 1) {
        editRuleAtRow(selection.first().row());
    }
}

void MainWindow::editRuleAtRow(int row) {
    UdevRule rule = m_ruleModel->getRule(row);
    
    AddRuleDialog dialog(this);
    dialog.setRule(rule);
    
    if (dialog.exec() == QDialog::Accepted) {
        UdevRule updatedRule = dialog.getRule();
        m_ruleModel->updateRule(row, updatedRule);
        m_logWidget->appendLog(QString("Updated rule for %1 device(s)")
            .arg(updatedRule.devices.size()));
    }
}

void MainWindow::onDirtyChanged(bool dirty) {
    m_applyBtn->setEnabled(dirty);
    
    if (dirty) {
        setWindowTitle("udevme - udev Rule Manager *");
        updateStatus("Pending changes...");
    } else {
        setWindowTitle("udevme - udev Rule Manager");
    }
}

void MainWindow::onAddRule() {
    AddRuleDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        UdevRule rule = dialog.getRule();
        m_ruleModel->addRule(rule);
        m_logWidget->appendLog(QString("Added rule for %1 device(s)")
            .arg(rule.devices.size()));
    }
}

void MainWindow::onRemoveRule() {
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    
    if (selection.isEmpty()) return;
    
    int count = selection.size();
    QString msg = count == 1 
        ? "Remove selected rule?" 
        : QString("Remove %1 selected rules?").arg(count);
    
    if (QMessageBox::question(this, "Confirm Remove", msg) != QMessageBox::Yes) {
        return;
    }
    
    QVector<int> rows;
    for (const auto& idx : selection) {
        rows.append(idx.row());
    }
    
    m_ruleModel->removeRules(rows);
    m_logWidget->appendLog(QString("Removed %1 rule(s)").arg(count));
}

void MainWindow::onApply() {
    m_applyBtn->setEnabled(false);
    m_addBtn->setEnabled(false);
    m_editBtn->setEnabled(false);
    m_removeBtn->setEnabled(false);
    updateStatus("Applying rules...");
    m_logWidget->appendLog("Starting apply process...");
    
    applyRulesAsync();
}

void MainWindow::applyRulesAsync() {
    // Generate rules content
    QVector<UdevRule> allRules = m_ruleModel->getAllRules();
    QString rulesContent = RuleGenerator::generateRulesFile(allRules);
    
    // Save staged file
    if (!ConfigStore::saveStagedRules(rulesContent)) {
        QMessageBox::critical(this, "Error", "Failed to save staged rules file");
        m_logWidget->appendLog("ERROR: Failed to save staged rules file");
        m_applyBtn->setEnabled(true);
        m_addBtn->setEnabled(true);
        onSelectionChanged(); // Re-enable edit/remove based on selection
        return;
    }
    
    m_logWidget->appendLog("Staged rules saved to: " + ConfigStore::getStagedRulesPath());
    m_logWidget->appendLog("Rules content preview:\n" + rulesContent.left(500) + 
        (rulesContent.length() > 500 ? "..." : ""));
    
    // Find pkexec or sudo
    QString elevateCmd;
    QStringList elevateArgs;
    
    if (QFile::exists("/usr/bin/pkexec")) {
        elevateCmd = "pkexec";
    } else if (QFile::exists("/usr/bin/sudo")) {
        elevateCmd = "sudo";
    } else {
        QMessageBox::critical(this, "Error", 
            "No privilege escalation tool found (pkexec or sudo).\n"
            "Please install polkit or sudo.");
        m_logWidget->appendLog("ERROR: No pkexec or sudo found");
        m_applyBtn->setEnabled(true);
        m_addBtn->setEnabled(true);
        onSelectionChanged(); // Re-enable edit/remove based on selection
        return;
    }
    
    // Create a script to run with elevated privileges
    QString script = QString(
        "#!/bin/bash\n"
        "set -e\n"
        "cp '%1' '%2'\n"
        "udevadm control --reload-rules\n"
        "udevadm trigger\n"
        "echo ''\n"
        "echo 'Rules applied successfully!'\n"
        "echo ''\n"
        "echo 'Current hidraw device permissions:'\n"
        "ls -l /dev/hidraw* 2>/dev/null || echo 'No hidraw devices found'\n"
    ).arg(ConfigStore::getStagedRulesPath(), ConfigStore::getSystemRulesPath());
    
    QString scriptPath = ConfigStore::getInstallDir() + "/apply_rules.sh";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        scriptFile.write(script.toUtf8());
        scriptFile.close();
        scriptFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                  QFile::ReadGroup | QFile::ReadOther);
    } else {
        QMessageBox::critical(this, "Error", "Failed to create apply script");
        m_applyBtn->setEnabled(true);
        m_addBtn->setEnabled(true);
        onSelectionChanged(); // Re-enable edit/remove based on selection
        return;
    }
    
    m_logWidget->appendLog(QString("Running: %1 %2").arg(elevateCmd, scriptPath));
    
    QProcess* process = new QProcess(this);
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, rulesContent, allRules](int exitCode, QProcess::ExitStatus status) {
        
        QString output = QString::fromUtf8(process->readAllStandardOutput());
        QString errorOutput = QString::fromUtf8(process->readAllStandardError());
        
        if (!output.isEmpty()) {
            m_logWidget->appendLog("Output: " + output);
        }
        if (!errorOutput.isEmpty()) {
            m_logWidget->appendLog("Errors: " + errorOutput);
        }
        
        if (exitCode == 0 && status == QProcess::NormalExit) {
            // Verify the system rules match what we generated
            QString systemContent = ConfigStore::readSystemRules();
            QString systemHash = RuleParser::computeHash(systemContent);
            QString expectedHash = RuleParser::computeHash(rulesContent);
            
            if (systemHash == expectedHash) {
                // Update sync info
                SyncInfo syncInfo;
                syncInfo.rulesFileHashAtLoad = systemHash;
                syncInfo.rulesFileHashAfterApply = systemHash;
                syncInfo.lastAppliedAt = QDateTime::currentDateTime();
                syncInfo.lastSyncedFromRulesAt = QDateTime::currentDateTime();
                
                ConfigStore::saveConfig(allRules, syncInfo);
                m_ruleModel->clearDirty();
                
                m_logWidget->appendLog("Rules applied and verified successfully!");
                updateStatus("Rules applied successfully. You may need to unplug/replug devices.");
                
                QMessageBox::information(this, "Success", 
                    "Rules applied successfully!\n\n"
                    "You may need to unplug and replug your devices for the new rules to take effect.");
            } else {
                m_logWidget->appendLog("WARNING: System rules hash mismatch after apply");
                updateStatus("Applied but verification failed - please check manually");
            }
        } else {
            m_logWidget->appendLog(QString("Apply failed with exit code %1").arg(exitCode));
            updateStatus("Apply failed - check log for details");
            QMessageBox::warning(this, "Apply Failed", 
                "Failed to apply rules. Check the log for details.\n"
                "You may need to run with proper permissions.");
        }
        
        m_applyBtn->setEnabled(m_ruleModel->isDirty());
        m_addBtn->setEnabled(true);
        QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
        m_editBtn->setEnabled(sel.size() == 1);
        m_removeBtn->setEnabled(!sel.isEmpty());
        
        process->deleteLater();
    });
    
    process->start(elevateCmd, {scriptPath});
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About udevme",
        "<h3>udevme</h3>"
        "<p>Version 1.0.1</p>"
        "<p>A udev rule manager for WebHID / hidraw device access.</p>"
        "<p>This tool helps you create and manage udev rules so that browsers "
        "(Brave, Chrome, Chromium, etc.) can access HID devices via WebHID.</p>"
        "<hr>"
        "<p>Created by Sanchi with AI assistance.</p>"
        "<hr>"
        "<p>Config location: ~/.local/bin/udevme/</p>"
        "<p>System rules: /etc/udev/rules.d/99-udevme.rules</p>");
}

} // namespace udevme
