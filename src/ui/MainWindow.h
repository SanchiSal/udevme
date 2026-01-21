#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include "RuleModel.h"
#include "LogWidget.h"

namespace udevme {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onAddRule();
    void onEditRule();
    void onRemoveRule();
    void onApply();
    void onSelectionChanged();
    void onDoubleClicked(const QModelIndex& index);
    void onDirtyChanged(bool dirty);
    void onAbout();

private:
    void setupUi();
    void setupMenuBar();
    void loadRules();
    void updateStatus(const QString& message);
    void applyRulesAsync();
    void editRuleAtRow(int row);
    
    QTableView* m_tableView;
    RuleModel* m_ruleModel;
    
    QPushButton* m_addBtn;
    QPushButton* m_editBtn;
    QPushButton* m_removeBtn;
    QPushButton* m_applyBtn;
    QLabel* m_statusLabel;
    
    LogWidget* m_logWidget;
    
    QString m_rulesHashAtLoad;
};

} // namespace udevme

#endif // MAINWINDOW_H
