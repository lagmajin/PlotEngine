#pragma once

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include "wobjectdefs.h"

class QLineEdit;
class QPushButton;
class QLabel;
class QCheckBox;
class SyntaxHighlighter;
class LineNumberArea;

class NovelEditor : public QPlainTextEdit {
    W_OBJECT(NovelEditor)
public:
    struct ProtectedSnippet {
        QString label;
        QString text;
        int start = -1;
        int length = 0;

        bool operator==(const ProtectedSnippet &other) const = default;
    };

    explicit NovelEditor(const QString &sceneId, QWidget *parent = nullptr);

    QString sceneId() const { return m_sceneId; }
    void setContent(const QString &text);
    void showSearchBar();
    void showReplaceBar();
    bool findNext();
    bool findPrevious();
    void replaceCurrent();
    int replaceAll();
    void setSearchText(const QString &text);
    QString currentSearchText() const;
    void setProtectedSnippets(const QVector<ProtectedSnippet> &snippets);
    QVector<ProtectedSnippet> protectedSnippets() const;

public:
    void contentChanged(const QString &text)
    W_SIGNAL(contentChanged, (const QString &), text)
    void protectedSnippetsEdited()
    W_SIGNAL(protectedSnippetsEdited)

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void onTextChanged();
    W_SLOT(onTextChanged)
    void updateLineNumberAreaWidth(int newBlockCount);
    W_SLOT(updateLineNumberAreaWidth, (int))
    void highlightCurrentLine();
    W_SLOT(highlightCurrentLine)
    void updateLineNumberArea(const QRect &rect, int dy);
    W_SLOT(updateLineNumberArea, (const QRect &, int))
    void onSearchTextEdited(const QString &text);
    W_SLOT(onSearchTextEdited, (const QString &))
    void onReplaceTextEdited(const QString &text);
    W_SLOT(onReplaceTextEdited, (const QString &))
    void onSearchNext();
    W_SLOT(onSearchNext)
    void onSearchPrevious();
    W_SLOT(onSearchPrevious)
    void onReplaceOne();
    W_SLOT(onReplaceOne)
    void onReplaceAll();
    W_SLOT(onReplaceAll)
    void onContentsChange(int position, int charsRemoved, int charsAdded);
    W_SLOT(onContentsChange, (int, int, int))
    void hideSearchBar();
    W_SLOT(hideSearchBar)

private:
    friend class LineNumberArea;
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void updateSearchBarGeometry();
    void updateViewportMargins();
    bool findInternal(const QString &text, QTextDocument::FindFlags flags);
    void updateSearchHighlights();
    int countSearchHits(const QString &text) const;
    QTextDocument::FindFlags searchFlags() const;
    QString textForRange(int start, int length) const;
    void normalizeProtectedSnippets();

    QString m_sceneId;
    SyntaxHighlighter *m_highlighter = nullptr;
    LineNumberArea *m_lineNumberArea = nullptr;
    QWidget *m_searchBar = nullptr;
    QLabel *m_searchStatusLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QCheckBox *m_caseSensitiveCheck = nullptr;
    QCheckBox *m_wholeWordCheck = nullptr;
    QPushButton *m_searchNextButton = nullptr;
    QPushButton *m_searchPrevButton = nullptr;
    QPushButton *m_replaceOneButton = nullptr;
    QPushButton *m_replaceAllButton = nullptr;
    QPushButton *m_closeSearchButton = nullptr;
    int m_searchBarHeight = 0;
    bool m_loading = false;
    bool m_updatingProtectedSnippets = false;
    QVector<ProtectedSnippet> m_protectedSnippets;
};
