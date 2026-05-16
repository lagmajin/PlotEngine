#include "noveleditor.h"
#include "syntaxhighlighter.h"
#include "wobjectimpl.h"

#include <QColor>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextEdit>
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QMimeData>
#include <QKeySequence>
#include <algorithm>
#include <QVBoxLayout>

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(NovelEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    NovelEditor *m_editor = nullptr;
};

W_OBJECT_IMPL(NovelEditor)

NovelEditor::NovelEditor(const QString &sceneId, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_sceneId(sceneId)
{
    setReadOnly(false);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setStyleSheet(
        "QPlainTextEdit { background: #1e1e2e; color: #cdd6f4; border: none; padding: 8px; }"
        "QPlainTextEdit:focus { border: none; }"
        "QWidget#searchBar { background: #181825; border-bottom: 1px solid #313244; }"
        "QLineEdit { background: #11111b; color: #cdd6f4; border: 1px solid #313244; padding: 4px 6px; }"
        "QPushButton { background: #313244; color: #cdd6f4; border: 1px solid #45475a; padding: 4px 8px; }"
        "QPushButton:hover { background: #45475a; }"
        "QLabel#searchStatus { color: #a6adc8; padding: 0 6px; }"
    );
    setPlaceholderText("ここに本文を書いてください...");
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    m_highlighter = new SyntaxHighlighter(document());
    m_lineNumberArea = new LineNumberArea(this);

    m_searchBar = new QWidget(this);
    m_searchBar->setObjectName("searchBar");
    auto *searchLayout = new QHBoxLayout(m_searchBar);
    searchLayout->setContentsMargins(8, 6, 8, 6);
    searchLayout->setSpacing(6);

    searchLayout->addWidget(new QLabel("検索:", m_searchBar));
    m_searchEdit = new QLineEdit(m_searchBar);
    m_searchEdit->setPlaceholderText("検索語");
    searchLayout->addWidget(m_searchEdit, 2);

    searchLayout->addWidget(new QLabel("置換:", m_searchBar));
    m_replaceEdit = new QLineEdit(m_searchBar);
    m_replaceEdit->setPlaceholderText("置換後");
    searchLayout->addWidget(m_replaceEdit, 2);

    m_caseSensitiveCheck = new QCheckBox("Aa", m_searchBar);
    m_caseSensitiveCheck->setToolTip("大文字小文字を区別");
    m_wholeWordCheck = new QCheckBox("Word", m_searchBar);
    m_wholeWordCheck->setToolTip("単語単位で検索");
    m_searchPrevButton = new QPushButton("前へ", m_searchBar);
    m_searchNextButton = new QPushButton("次へ", m_searchBar);
    m_replaceOneButton = new QPushButton("置換", m_searchBar);
    m_replaceAllButton = new QPushButton("全置換", m_searchBar);
    m_closeSearchButton = new QPushButton("閉じる", m_searchBar);
    m_searchStatusLabel = new QLabel("0 件", m_searchBar);
    m_searchStatusLabel->setObjectName("searchStatus");

    searchLayout->addWidget(m_searchStatusLabel);
    searchLayout->addWidget(m_caseSensitiveCheck);
    searchLayout->addWidget(m_wholeWordCheck);
    searchLayout->addWidget(m_searchPrevButton);
    searchLayout->addWidget(m_searchNextButton);
    searchLayout->addWidget(m_replaceOneButton);
    searchLayout->addWidget(m_replaceAllButton);
    searchLayout->addWidget(m_closeSearchButton);

    m_searchBar->hide();

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &NovelEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &NovelEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &NovelEditor::updateSearchHighlights);
    connect(this, &QPlainTextEdit::textChanged,
            this, &NovelEditor::onTextChanged);
    connect(document(), &QTextDocument::contentsChange,
            this, &NovelEditor::onContentsChange);

    connect(m_searchEdit, &QLineEdit::textEdited, this, &NovelEditor::onSearchTextEdited);
    connect(m_replaceEdit, &QLineEdit::textEdited, this, &NovelEditor::onReplaceTextEdited);
    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, &NovelEditor::updateSearchHighlights);
    connect(m_wholeWordCheck, &QCheckBox::toggled, this, &NovelEditor::updateSearchHighlights);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &NovelEditor::onSearchNext);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &NovelEditor::onReplaceOne);
    connect(m_searchNextButton, &QPushButton::clicked, this, &NovelEditor::onSearchNext);
    connect(m_searchPrevButton, &QPushButton::clicked, this, &NovelEditor::onSearchPrevious);
    connect(m_replaceOneButton, &QPushButton::clicked, this, &NovelEditor::onReplaceOne);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &NovelEditor::onReplaceAll);
    connect(m_closeSearchButton, &QPushButton::clicked, this, &NovelEditor::hideSearchBar);

    updateLineNumberAreaWidth(0);
    updateSearchHighlights();
}

void NovelEditor::setContent(const QString &text)
{
    m_loading = true;
    setPlainText(text);
    m_loading = false;
    updateSearchHighlights();
}

void NovelEditor::showSearchBar()
{
    if (!m_searchBar->isVisible())
        m_searchBar->show();
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
    m_searchBarHeight = m_searchBar->sizeHint().height();
    updateViewportMargins();
    updateSearchBarGeometry();
    updateSearchHighlights();
}

void NovelEditor::showReplaceBar()
{
    showSearchBar();
    m_replaceEdit->setFocus();
    m_replaceEdit->selectAll();
}

bool NovelEditor::findNext()
{
    const QString text = currentSearchText();
    if (text.isEmpty())
        return false;
    return findInternal(text, searchFlags());
}

bool NovelEditor::findPrevious()
{
    const QString text = currentSearchText();
    if (text.isEmpty())
        return false;
    return findInternal(text, searchFlags() | QTextDocument::FindBackward);
}

void NovelEditor::replaceCurrent()
{
    QString search = currentSearchText();
    if (search.isEmpty())
        return;

    const auto flags = searchFlags();
    const Qt::CaseSensitivity sensitivity = flags.testFlag(QTextDocument::FindCaseSensitively)
        ? Qt::CaseSensitive
        : Qt::CaseInsensitive;
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && QString::compare(cursor.selectedText(), search, sensitivity) == 0) {
        cursor.insertText(m_replaceEdit->text());
        setTextCursor(cursor);
        findNext();
    } else if (findNext()) {
        cursor = textCursor();
        if (cursor.hasSelection() && QString::compare(cursor.selectedText(), search, sensitivity) == 0) {
            cursor.insertText(m_replaceEdit->text());
            setTextCursor(cursor);
        }
    }
}

int NovelEditor::replaceAll()
{
    const QString search = currentSearchText();
    if (search.isEmpty())
        return 0;

    const QString replacement = m_replaceEdit->text();
    const auto flags = searchFlags();
    QTextCursor editCursor(document());
    editCursor.beginEditBlock();
    QTextCursor cursor = editCursor;
    cursor.movePosition(QTextCursor::Start);

    int count = 0;
    while (true) {
        cursor = document()->find(search, cursor, flags);
        if (cursor.isNull())
            break;
        cursor.insertText(replacement);
        ++count;
    }
    editCursor.endEditBlock();
    return count;
}

void NovelEditor::setSearchText(const QString &text)
{
    m_searchEdit->setText(text);
    updateSearchHighlights();
}

QString NovelEditor::currentSearchText() const
{
    return m_searchEdit->text().trimmed();
}

void NovelEditor::setProtectedSnippets(const QVector<ProtectedSnippet> &snippets)
{
    if (m_protectedSnippets == snippets)
        return;
    m_updatingProtectedSnippets = true;
    m_protectedSnippets = snippets;
    normalizeProtectedSnippets();
    m_updatingProtectedSnippets = false;
    updateSearchHighlights();
}

QVector<NovelEditor::ProtectedSnippet> NovelEditor::protectedSnippets() const
{
    return m_protectedSnippets;
}

void NovelEditor::onTextChanged()
{
    if (!m_loading) {
        emit contentChanged(toPlainText());
        updateSearchHighlights();
    }
}

void NovelEditor::onSearchTextEdited(const QString &text)
{
    updateSearchHighlights();
    if (!text.trimmed().isEmpty())
        findNext();
}

void NovelEditor::onReplaceTextEdited(const QString &)
{
    updateSearchHighlights();
}

void NovelEditor::onSearchNext()
{
    findNext();
}

void NovelEditor::onSearchPrevious()
{
    findPrevious();
}

void NovelEditor::onReplaceOne()
{
    replaceCurrent();
    updateSearchHighlights();
}

void NovelEditor::onReplaceAll()
{
    replaceAll();
    updateSearchHighlights();
}

void NovelEditor::onContentsChange(int position, int charsRemoved, int charsAdded)
{
    if (m_loading || m_updatingProtectedSnippets)
        return;

    bool changed = false;
    const int delta = charsAdded - charsRemoved;
    const int changeEnd = position + charsRemoved;

    for (auto &snippet : m_protectedSnippets) {
        const int originalStart = snippet.start;
        const int originalEnd = snippet.start + snippet.length;

        if (snippet.start < 0 || snippet.length < 0)
            continue;

        if (changeEnd <= originalStart) {
            snippet.start += delta;
        } else if (position < originalEnd) {
            if (position < snippet.start)
                snippet.start = position;
            snippet.length += delta;
            if (snippet.length < 0)
                snippet.length = 0;
        }

        snippet.text = textForRange(snippet.start, snippet.length);
        if (snippet.start != originalStart || (snippet.start + snippet.length) != originalEnd
            || snippet.text != textForRange(originalStart, originalEnd - originalStart)) {
            changed = true;
        }
    }

    const auto oldSize = m_protectedSnippets.size();
    m_protectedSnippets.erase(
        std::remove_if(m_protectedSnippets.begin(), m_protectedSnippets.end(),
            [](const ProtectedSnippet &snippet) {
                return snippet.length <= 0 || snippet.text.trimmed().isEmpty();
            }),
        m_protectedSnippets.end());

    if (m_protectedSnippets.size() != oldSize)
        changed = true;

    if (changed) {
        updateSearchHighlights();
        emit protectedSnippetsEdited();
    }
}

void NovelEditor::hideSearchBar()
{
    m_searchBar->hide();
    m_searchBarHeight = 0;
    updateViewportMargins();
    updateSearchBarGeometry();
    updateSearchHighlights();
    setFocus();
}

int NovelEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int maxBlocks = qMax(1, blockCount());
    while (maxBlocks >= 10) {
        maxBlocks /= 10;
        ++digits;
    }

    return 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void NovelEditor::updateLineNumberAreaWidth(int)
{
    updateViewportMargins();
}

void NovelEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void NovelEditor::highlightCurrentLine()
{
    updateSearchHighlights();
}

void NovelEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateSearchBarGeometry();
}

void NovelEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_searchBar->isVisible() && event->key() == Qt::Key_Escape) {
        hideSearchBar();
        event->accept();
        return;
    }

    const QTextCursor cursor = textCursor();
    const bool selectionBlocked = selectionIntersectsProtected(cursor);
    const bool cursorBlocked = cursorIsInsideProtected(cursor.position());

    if (event->matches(QKeySequence::Cut)) {
        if (selectionBlocked || cursorBlocked) {
            notifyProtectedEditBlocked();
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Backspace) {
        if (selectionBlocked || cursorTouchesProtectedForDelete(cursor.position(), -1)) {
            notifyProtectedEditBlocked();
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Delete) {
        if (selectionBlocked || cursorTouchesProtectedForDelete(cursor.position(), 1)) {
            notifyProtectedEditBlocked();
            event->accept();
            return;
        }
    }

    const QString inputText = event->text();
    if (!inputText.isEmpty() && !event->matches(QKeySequence::Paste)) {
        if (selectionBlocked || cursorBlocked) {
            notifyProtectedEditBlocked();
            event->accept();
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void NovelEditor::contextMenuEvent(QContextMenuEvent *event)
{
    if (!isProtectedEditContext()) {
        QPlainTextEdit::contextMenuEvent(event);
        return;
    }

    QMenu *menu = createStandardContextMenu();
    if (!menu) {
        QPlainTextEdit::contextMenuEvent(event);
        return;
    }

    for (QAction *action : menu->actions()) {
        if (!action)
            continue;

        const QString text = action->text().toLower();
        const bool isCut = action->shortcut() == QKeySequence::Cut || text.contains("cut") || text.contains("切り取り");
        const bool isPaste = action->shortcut() == QKeySequence::Paste || text.contains("paste") || text.contains("貼り付け");
        const bool isDelete = text.contains("delete") || text.contains("削除");
        if (isCut || isPaste || isDelete)
            action->setEnabled(false);
    }

    menu->exec(event->globalPos());
    delete menu;
}

void NovelEditor::insertFromMimeData(const QMimeData *source)
{
    const QTextCursor cursor = textCursor();
    if (selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position())) {
        notifyProtectedEditBlocked();
        return;
    }

    QPlainTextEdit::insertFromMimeData(source);
}

void NovelEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#181825"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#6c7086"));
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void NovelEditor::updateSearchBarGeometry()
{
    const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
    setViewportMargins(lineNumberAreaWidth(), topMargin, 0, 0);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    if (m_searchBar->isVisible()) {
        m_searchBar->setGeometry(0, 0, width(), m_searchBarHeight);
        m_searchBar->raise();
    }
}

void NovelEditor::updateViewportMargins()
{
    updateSearchBarGeometry();
}

bool NovelEditor::findInternal(const QString &text, QTextDocument::FindFlags flags)
{
    if (text.isEmpty())
        return false;

    if (!QPlainTextEdit::find(text, flags)) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(flags.testFlag(QTextDocument::FindBackward)
            ? QTextCursor::End
            : QTextCursor::Start);
        setTextCursor(cursor);
        return QPlainTextEdit::find(text, flags);
    }

    return true;
}

int NovelEditor::countSearchHits(const QString &text) const
{
    if (text.isEmpty())
        return 0;

    int hits = 0;
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::Start);
    while (true) {
        cursor = document()->find(text, cursor, searchFlags());
        if (cursor.isNull())
            break;
        ++hits;
    }
    return hits;
}

void NovelEditor::updateSearchHighlights()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection currentLine;
        currentLine.format.setBackground(QColor("#313244"));
        currentLine.format.setProperty(QTextFormat::FullWidthSelection, true);
        currentLine.cursor = textCursor();
        currentLine.cursor.clearSelection();
        selections.append(currentLine);
    }

    for (const auto &snippet : m_protectedSnippets) {
        if (snippet.start < 0 || snippet.length <= 0)
            continue;
        QTextCursor cursor(document());
        cursor.setPosition(snippet.start);
        cursor.setPosition(snippet.start + snippet.length, QTextCursor::KeepAnchor);
        if (!cursor.hasSelection())
            continue;

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        QTextCharFormat format;
        format.setBackground(QColor("#264653"));
        format.setForeground(QColor("#d8f3dc"));
        selection.format = format;
        selections.append(selection);
    }

    const QString search = currentSearchText();
    if (!search.isEmpty()) {
        const int hitCount = countSearchHits(search);
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::Start);

        while (true) {
            cursor = document()->find(search, cursor, searchFlags());
            if (cursor.isNull())
                break;

            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            QTextCharFormat format;
            format.setBackground(QColor("#665c00"));
            format.setForeground(QColor("#ffffff"));
            selection.format = format;
            selections.append(selection);
        }

        if (m_searchStatusLabel)
            m_searchStatusLabel->setText(QString::number(hitCount) + " 件");
    } else if (m_searchStatusLabel) {
        m_searchStatusLabel->setText("0 件");
    }

    setExtraSelections(selections);
}

bool NovelEditor::selectionIntersectsProtected(const QTextCursor &cursor) const
{
    if (!cursor.hasSelection())
        return false;

    const int start = cursor.selectionStart();
    const int end = cursor.selectionEnd();
    for (const auto &snippet : m_protectedSnippets) {
        const int snippetStart = snippet.start;
        const int snippetEnd = snippet.start + snippet.length;
        if (start < snippetEnd && end > snippetStart)
            return true;
    }
    return false;
}

bool NovelEditor::cursorTouchesProtectedForDelete(int position, int charsRemoved) const
{
    if (position < 0)
        return false;

    if (charsRemoved < 0) {
        const int deletePos = position - 1;
        for (const auto &snippet : m_protectedSnippets) {
            if (deletePos >= snippet.start && deletePos < snippet.start + snippet.length)
                return true;
        }
    } else if (charsRemoved > 0) {
        for (const auto &snippet : m_protectedSnippets) {
            if (position >= snippet.start && position < snippet.start + snippet.length)
                return true;
        }
    }

    return false;
}

bool NovelEditor::cursorIsInsideProtected(int position) const
{
    for (const auto &snippet : m_protectedSnippets) {
        if (position > snippet.start && position < snippet.start + snippet.length)
            return true;
    }
    return false;
}

bool NovelEditor::isProtectedEditContext() const
{
    const QTextCursor cursor = textCursor();
    return selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position());
}

QString NovelEditor::textForRange(int start, int length) const
{
    if (start < 0 || length <= 0)
        return QString();

    QTextCursor cursor(document());
    cursor.setPosition(start);
    cursor.setPosition(start + length, QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    text.replace(QChar(0x2029), '\n');
    text.replace(QChar(0x2028), '\n');
    return text;
}

void NovelEditor::normalizeProtectedSnippets()
{
    int searchFrom = 0;
    for (auto &snippet : m_protectedSnippets) {
        if (snippet.text.isEmpty() && snippet.start >= 0 && snippet.length > 0)
            snippet.text = textForRange(snippet.start, snippet.length);

        bool validRange = snippet.start >= 0
            && snippet.length > 0
            && snippet.start + snippet.length <= document()->characterCount() - 1
            && textForRange(snippet.start, snippet.length) == snippet.text;

        if (!validRange && !snippet.text.isEmpty()) {
            QTextCursor found = document()->find(snippet.text, searchFrom, QTextDocument::FindCaseSensitively);
            if (found.isNull())
                found = document()->find(snippet.text, 0, QTextDocument::FindCaseSensitively);
            if (!found.isNull()) {
                snippet.start = found.selectionStart();
                snippet.length = found.selectionEnd() - found.selectionStart();
                searchFrom = found.selectionEnd();
            }
        }

        if (snippet.start >= 0 && snippet.length > 0)
            snippet.text = textForRange(snippet.start, snippet.length);
    }

    m_protectedSnippets.erase(
        std::remove_if(m_protectedSnippets.begin(), m_protectedSnippets.end(),
            [](const ProtectedSnippet &snippet) {
                return snippet.start < 0 || snippet.length <= 0 || snippet.text.trimmed().isEmpty();
            }),
        m_protectedSnippets.end());
}

void NovelEditor::notifyProtectedEditBlocked() const
{
    QApplication::beep();
}

QTextDocument::FindFlags NovelEditor::searchFlags() const
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCheck && m_caseSensitiveCheck->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWordCheck && m_wholeWordCheck->isChecked())
        flags |= QTextDocument::FindWholeWords;
    return flags;
}
