#pragma once

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QContextMenuEvent>
#include <QEvent>
#include <QInputMethodEvent>
#include <QInputMethodQueryEvent>
#include <QWheelEvent>
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
    struct OutlineEntry {
        QString title;
        int lineNumber = 1;
        int level = 0;
    };

    enum class WritingMode {
        LeftToRight,
        RightToLeft,
        VerticalRightToLeft,
    };

    struct ProtectedSnippet {
        QString label;
        QString text;
        int start = -1;
        int length = 0;

        bool operator==(const ProtectedSnippet &other) const = default;
    };

    explicit NovelEditor(const QString &sceneId, QWidget *parent = nullptr);

    QString sceneId() const { return m_sceneId; }
    void setWritingMode(WritingMode mode);
    WritingMode writingMode() const { return m_writingMode; }
    void setTextLayoutDirection(Qt::LayoutDirection direction);
    Qt::LayoutDirection textLayoutDirection() const { return m_textLayoutDirection; }
    void setContent(const QString &text);
    void showSearchBar();
    void showReplaceBar();
    bool findNext();
    bool findPrevious();
    void replaceCurrent();
    int replaceAll();
    void gotoLine(int lineNumber);
    void setSearchText(const QString &text);
    QString currentSearchText() const;
    void setProtectedSnippets(const QVector<ProtectedSnippet> &snippets);
    QVector<ProtectedSnippet> protectedSnippets() const;
    QVector<OutlineEntry> outlineEntries() const;

public:
    void contentChanged(const QString &text)
    W_SIGNAL(contentChanged, (const QString &), text)
    void protectedSnippetsEdited()
    W_SIGNAL(protectedSnippetsEdited)

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;
    void wheelEvent(QWheelEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

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
    bool selectionIntersectsProtected(const QTextCursor &cursor) const;
    bool cursorTouchesProtectedForDelete(int position, int charsRemoved) const;
    bool cursorIsInsideProtected(int position) const;
    bool isProtectedEditContext() const;
    QString textForRange(int start, int length) const;
    void normalizeProtectedSnippets();
    void notifyProtectedEditBlocked() const;
    void applyTextLayoutDirection();
    void applyWritingMode();
    bool isVerticalWriting() const;
    void paintVerticalDocument(QPaintEvent *event);
    QTextCursor cursorForVerticalPosition(const QPoint &pos) const;
    bool moveVerticalCursor(int blockDelta, int charDelta, bool keepAnchor);
    int verticalColumnAdvance() const;
    int verticalCharacterAdvance() const;
    void clampVerticalOffsets();
    struct VerticalUnit {
        QString text;
        int start = -1;
        int length = 0;
        bool ruby = false;
        QString rubyText;
        bool tateChuYoko = false;
    };
    QVector<VerticalUnit> parseVerticalUnits(const QString &text) const;
    QRect verticalUnitRect(int columnX, int rowY, const VerticalUnit &unit) const;
    void paintVerticalCursor(QPainter &painter) const;
    void paintVerticalPreedit(QPainter &painter) const;
    void applyIndentationToSelection(bool outdent);
    bool handleReturnKey(QKeyEvent *event);
    bool handleTabKey(QKeyEvent *event);
    bool handleAutoPairKey(QKeyEvent *event);
    bool handleClosingBracketSkip(QKeyEvent *event);
    bool handleDuplicateLineShortcut(QKeyEvent *event);
    bool handleOutlineNavigationShortcut(QKeyEvent *event);
    QString currentLineIndentation() const;
    int currentLineNumber() const;
    void duplicateCurrentLine();
    bool moveToAdjacentParagraph(bool forward);
    static QChar matchingClosingChar(QChar open);
    static bool isAutoPairOpenChar(QChar ch);
    static bool isAutoPairCloseChar(QChar ch);

    QString m_sceneId;
    WritingMode m_writingMode = WritingMode::LeftToRight;
    Qt::LayoutDirection m_textLayoutDirection = Qt::LeftToRight;
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
    int m_verticalColumnOffset = 0;
    int m_verticalRowOffset = 0;
    QString m_preeditText;
    bool m_loading = false;
    bool m_updatingProtectedSnippets = false;
    QVector<ProtectedSnippet> m_protectedSnippets;
};
