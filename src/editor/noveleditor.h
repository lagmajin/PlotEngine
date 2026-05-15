#pragma once

#include <QPlainTextEdit>
#include <QString>
#include <QWidget>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QKeyEvent>

class QLineEdit;
class QPushButton;
class QLabel;
class SyntaxHighlighter;
class LineNumberArea;

class NovelEditor : public QPlainTextEdit {
    Q_OBJECT
public:
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

signals:
    void contentChanged(const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTextChanged();
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onSearchTextEdited(const QString &text);
    void onReplaceTextEdited(const QString &text);
    void onSearchNext();
    void onSearchPrevious();
    void onReplaceOne();
    void onReplaceAll();
    void hideSearchBar();

private:
    friend class LineNumberArea;
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void updateSearchBarGeometry();
    void updateViewportMargins();
    bool findInternal(const QString &text, QTextDocument::FindFlags flags);
    void updateSearchHighlights();
    int countSearchHits(const QString &text) const;

    QString m_sceneId;
    SyntaxHighlighter *m_highlighter = nullptr;
    LineNumberArea *m_lineNumberArea = nullptr;
    QWidget *m_searchBar = nullptr;
    QLabel *m_searchStatusLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QPushButton *m_searchNextButton = nullptr;
    QPushButton *m_searchPrevButton = nullptr;
    QPushButton *m_replaceOneButton = nullptr;
    QPushButton *m_replaceAllButton = nullptr;
    QPushButton *m_closeSearchButton = nullptr;
    int m_searchBarHeight = 0;
    bool m_loading = false;
};
