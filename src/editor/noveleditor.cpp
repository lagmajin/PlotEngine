#include "noveleditor.h"
#include "syntaxhighlighter.h"

NovelEditor::NovelEditor(const QString &sceneId, QWidget *parent)
    : QTextEdit(parent)
    , m_sceneId(sceneId)
{
    setAcceptRichText(false);
    setFont(QFont("MS Gothic", 12));
    setStyleSheet(
        "QTextEdit { background: #1e1e2e; color: #cdd6f4; border: none; padding: 8px; }"
        "QTextEdit:focus { border: none; }"
    );
    setPlaceholderText("ここに本文を書いてください...");

    m_highlighter = new SyntaxHighlighter(document());

    connect(this, &QTextEdit::textChanged, this, &NovelEditor::onTextChanged);
}

void NovelEditor::setContent(const QString &text)
{
    m_loading = true;
    setPlainText(text);
    m_loading = false;
}

void NovelEditor::onTextChanged()
{
    if (!m_loading)
        emit contentChanged(toPlainText());
}