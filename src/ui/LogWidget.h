#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

namespace udevme {

class LogWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);
    
    void appendLog(const QString& message);
    void clear();

private slots:
    void onToggleCollapse();
    void onClear();

private:
    QPlainTextEdit* m_logEdit;
    QPushButton* m_collapseBtn;
    QPushButton* m_clearBtn;
    QLabel* m_headerLabel;
    bool m_collapsed = false;
};

} // namespace udevme

#endif // LOGWIDGET_H
