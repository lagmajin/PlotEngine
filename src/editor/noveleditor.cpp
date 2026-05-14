#include "noveleditor.h"
#include "syntaxhighlighter.h"

#include <QColor>
#include <QFontDatabase>
#include <QHBoxLayout>
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

    m_searchPrevButton = new QPushButton("前へ", m_searchBar);
    m_searchNextButton = new QPushButton("次へ", m_searchBar);
    m_replaceOneButton = new QPushButton("置換", m_searchBar);
    m_replaceAllButton = new QPushButton("全置換", m_searchBar);
    m_closeSearchButton = new QPushButton("閉じる", m_searchBar);

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
            this, &NovelEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged,
            this, &NovelEditor::onTextChanged);

    connect(m_searchEdit, &QLineEdit::textEdited, this, &NovelEditor::onSearchTextEdited);
    connect(m_replaceEdit, &QLineEdit::textEdited, this, &NovelEditor::onReplaceTextEdited);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &NovelEditor::onSearchNext);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &NovelEditor::onReplaceOne);
    connect(m_searchNextButton, &QPushButton::clicked, this, &NovelEditor::onSearchNext);
    connect(m_searchPrevButton, &QPushButton::clicked, this, &NovelEditor::onSearchPrevious);
    connect(m_replaceOneButton, &QPushButton::clicked, this, &NovelEditor::onReplaceOne);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &NovelEditor::onReplaceAll);
    connect(m_closeSearchButton, &QPushButton::clicked, this, &NovelEditor::hideSearchBar);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void NovelEditor::setContent(const QString &text)
{
    m_loading = true;
    setPlainText(text);
    m_loading = false;
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
    return findInternal(text, {});
}

bool NovelEditor::findPrevious()
{
    const QString text = currentSearchText();
    if (text.isEmpty())
        return false;
    return findInternal(text, QTextDocument::FindBackward);
}

void NovelEditor::replaceCurrent()
{
    QString search = currentSearchText();
    if (search.isEmpty())
        return;

    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == search) {
        cursor.insertText(m_replaceEdit->text());
        setTextCursor(cursor);
        findNext();
    } else if (findNext()) {
        cursor = textCursor();
        if (cursor.hasSelection() && cursor.selectedText() == search) {
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
    QTextCursor editCursor(document());
    editCursor.beginEditBlock();
    QTextCursor cursor = editCursor;
    cursor.movePosition(QTextCursor::Start);

    int count = 0;
    while (true) {
        cursor = document()->find(search, cursor);
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
}

QString NovelEditor::currentSearchText() const
{
    return m_searchEdit->text().trimmed();
}

void NovelEditor::onTextChanged()
{
    if (!m_loading)
        emit contentChanged(toPlainText());
}

void NovelEditor::onSearchTextEdited(const QString &text)
{
    if (!text.trimmed().isEmpty())
        findNext();
}

void NovelEditor::onReplaceTextEdited(const QString &)
{
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
}

void NovelEditor::onReplaceAll()
{
    replaceAll();
}

void NovelEditor::hideSearchBar()
{
    m_searchBar->hide();
    m_searchBarHeight = 0;
    updateViewportMargins();
    updateSearchBarGeometry();
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
    QList<QPlainTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QPlainTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor("#313244"));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
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

    QPlainTextEdit::keyPressEvent(event);
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
