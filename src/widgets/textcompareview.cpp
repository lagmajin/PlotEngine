#include "widgets/textcompareview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

TextCompareView::TextCompareView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(splitter, 1);

    auto *leftPane = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    m_leftTitle = new QLabel(QStringLiteral("Before"), leftPane);
    m_leftTitle->setStyleSheet(QStringLiteral("QLabel { color: #cbd5e1; font-weight: 600; }"));
    leftLayout->addWidget(m_leftTitle);

    m_leftEdit = new QPlainTextEdit(leftPane);
    m_leftEdit->setReadOnly(true);
    m_leftEdit->setPlaceholderText(QStringLiteral("比較元"));
    leftLayout->addWidget(m_leftEdit, 1);

    auto *rightPane = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    m_rightTitle = new QLabel(QStringLiteral("After"), rightPane);
    m_rightTitle->setStyleSheet(QStringLiteral("QLabel { color: #cbd5e1; font-weight: 600; }"));
    rightLayout->addWidget(m_rightTitle);

    m_rightEdit = new QPlainTextEdit(rightPane);
    m_rightEdit->setReadOnly(true);
    m_rightEdit->setPlaceholderText(QStringLiteral("比較先"));
    rightLayout->addWidget(m_rightEdit, 1);

    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setChildrenCollapsible(false);
    splitter->setSizes({1, 1});

    connect(m_leftEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        syncScrollBars(m_leftEdit, m_rightEdit);
    });
    connect(m_rightEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        syncScrollBars(m_rightEdit, m_leftEdit);
    });
}

void TextCompareView::clear()
{
    setComparison(QStringLiteral("Before"), QString(), QStringLiteral("After"), QString());
}

void TextCompareView::setLeftPane(const QString &title, const QString &content)
{
    m_leftTitle->setText(title.trimmed().isEmpty() ? QStringLiteral("Before") : title.trimmed());
    m_leftEdit->setPlainText(content);
}

void TextCompareView::setRightPane(const QString &title, const QString &content)
{
    m_rightTitle->setText(title.trimmed().isEmpty() ? QStringLiteral("After") : title.trimmed());
    m_rightEdit->setPlainText(content);
}

void TextCompareView::setComparison(const QString &leftTitle, const QString &leftContent,
                                    const QString &rightTitle, const QString &rightContent)
{
    m_leftTitle->setText(leftTitle.trimmed().isEmpty() ? QStringLiteral("Before") : leftTitle.trimmed());
    m_rightTitle->setText(rightTitle.trimmed().isEmpty() ? QStringLiteral("After") : rightTitle.trimmed());
    m_leftEdit->setPlainText(leftContent);
    m_rightEdit->setPlainText(rightContent);
    m_leftEdit->verticalScrollBar()->setValue(0);
    m_rightEdit->verticalScrollBar()->setValue(0);
}

void TextCompareView::syncScrollBars(QPlainTextEdit *source, QPlainTextEdit *target)
{
    if (!source || !target || m_syncingScroll)
        return;

    auto *sourceBar = source->verticalScrollBar();
    auto *targetBar = target->verticalScrollBar();
    if (!sourceBar || !targetBar)
        return;

    m_syncingScroll = true;
    targetBar->setValue(sourceBar->value());
    m_syncingScroll = false;
}
