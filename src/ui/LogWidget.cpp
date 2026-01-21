#include "LogWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QStyle>
#include <QScrollBar>

namespace udevme {

LogWidget::LogWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    // Header bar
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    
    m_collapseBtn = new QPushButton(this);
    m_collapseBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
    m_collapseBtn->setFixedSize(24, 24);
    m_collapseBtn->setToolTip("Toggle log panel");
    m_collapseBtn->setFlat(true);
    connect(m_collapseBtn, &QPushButton::clicked, this, &LogWidget::onToggleCollapse);
    headerLayout->addWidget(m_collapseBtn);
    
    m_headerLabel = new QLabel("Log", this);
    headerLayout->addWidget(m_headerLabel);
    
    headerLayout->addStretch();
    
    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setFixedHeight(24);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::onClear);
    headerLayout->addWidget(m_clearBtn);
    
    layout->addLayout(headerLayout);
    
    // Log text area
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(1000); // Limit log size
    m_logEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    layout->addWidget(m_logEdit, 1);
}

void LogWidget::appendLog(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(timestamp, message));
    
    // Auto-scroll to bottom
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

void LogWidget::clear() {
    m_logEdit->clear();
}

void LogWidget::onToggleCollapse() {
    m_collapsed = !m_collapsed;
    m_logEdit->setVisible(!m_collapsed);
    m_clearBtn->setVisible(!m_collapsed);
    
    if (m_collapsed) {
        m_collapseBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    } else {
        m_collapseBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
    }
}

void LogWidget::onClear() {
    clear();
}

} // namespace udevme
