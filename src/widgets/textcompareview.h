#pragma once

#include <QWidget>

class QLabel;
class QPlainTextEdit;

class TextCompareView : public QWidget {
public:
    explicit TextCompareView(QWidget *parent = nullptr);

    void clear();
    void setLeftPane(const QString &title, const QString &content);
    void setRightPane(const QString &title, const QString &content);
    void setComparison(const QString &leftTitle, const QString &leftContent,
                       const QString &rightTitle, const QString &rightContent);

private:
    void syncScrollBars(QPlainTextEdit *source, QPlainTextEdit *target);

    QLabel *m_leftTitle = nullptr;
    QLabel *m_rightTitle = nullptr;
    QPlainTextEdit *m_leftEdit = nullptr;
    QPlainTextEdit *m_rightEdit = nullptr;
    bool m_syncingScroll = false;
};
