#pragma once

#include <QTextEdit>
#include <QString>

class SyntaxHighlighter;

class NovelEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit NovelEditor(const QString &sceneId, QWidget *parent = nullptr);

    QString sceneId() const { return m_sceneId; }
    void setContent(const QString &text);

signals:
    void contentChanged(const QString &text);

private slots:
    void onTextChanged();

private:
    QString m_sceneId;
    SyntaxHighlighter *m_highlighter = nullptr;
    bool m_loading = false;
};