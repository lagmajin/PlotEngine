#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    QTextCharFormat dialogueFormat;
    dialogueFormat.setForeground(QColor("#a6e3a1"));
    dialogueFormat.setFontWeight(QFont::Bold);

QTextCharFormat thoughtFormat;
    thoughtFormat.setForeground(QColor("#89dceb"));

    QTextCharFormat noteFormat;
    noteFormat.setForeground(QColor("#6c7086"));
    noteFormat.setFontItalic(true);

    QTextCharFormat emphasisFormat;
    emphasisFormat.setForeground(QColor("#f9e2af"));

    HighlightRule r1{ QRegularExpression("「[^」]*」"), dialogueFormat };
    HighlightRule r2{ QRegularExpression("『[^』]*』"), thoughtFormat };
    HighlightRule r3{ QRegularExpression("《[^》]*》"), noteFormat };
    HighlightRule r4{ QRegularExpression("【[^】]*】"), emphasisFormat };

    m_rules << r1 << r2 << r3 << r4;
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}