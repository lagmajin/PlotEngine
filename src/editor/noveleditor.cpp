#include "noveleditor.h"
#include "syntaxhighlighter.h"
#include "wobjectimpl.h"

#include <QColor>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextOption>
#include <QTextEdit>
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QMimeData>
#include <QKeySequence>
#include <QScrollBar>
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

static bool isKinsokuProhibitedStart(const QString &text);

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
    applyWritingMode();

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

void NovelEditor::setTextLayoutDirection(Qt::LayoutDirection direction)
{
    const WritingMode mode = direction == Qt::RightToLeft
        ? WritingMode::RightToLeft
        : WritingMode::LeftToRight;
    setWritingMode(mode);
}

void NovelEditor::setWritingMode(WritingMode mode)
{
    if (m_writingMode == mode)
        return;

    m_writingMode = mode;
    applyWritingMode();
    m_verticalColumnOffset = 0;
    m_verticalRowOffset = 0;
    clampVerticalOffsets();
    updateViewportMargins();
    updateSearchBarGeometry();
    updateSearchHighlights();
    viewport()->update();
}

void NovelEditor::setContent(const QString &text)
{
    m_loading = true;
    setPlainText(text);
    m_loading = false;
    updateSearchHighlights();
}

void NovelEditor::applyTextLayoutDirection()
{
    applyWritingMode();
}

void NovelEditor::applyWritingMode()
{
    switch (m_writingMode) {
    case WritingMode::LeftToRight:
        m_textLayoutDirection = Qt::LeftToRight;
        break;
    case WritingMode::RightToLeft:
        m_textLayoutDirection = Qt::RightToLeft;
        break;
    case WritingMode::VerticalRightToLeft:
        m_textLayoutDirection = Qt::RightToLeft;
        break;
    }

    setLayoutDirection(m_textLayoutDirection);

    QTextOption option = document()->defaultTextOption();
    option.setTextDirection(m_textLayoutDirection);
    option.setAlignment(m_textLayoutDirection == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft);
    document()->setDefaultTextOption(option);
    document()->setDefaultCursorMoveStyle(
        m_textLayoutDirection == Qt::RightToLeft ? Qt::VisualMoveStyle : Qt::LogicalMoveStyle);
}

bool NovelEditor::isVerticalWriting() const
{
    return m_writingMode == WritingMode::VerticalRightToLeft;
}

void NovelEditor::clampVerticalOffsets()
{
    if (!isVerticalWriting())
        return;

    const int blockCountValue = qMax(1, blockCount());
    const int maxColumns = qMax(0, blockCountValue - 1);
    m_verticalColumnOffset = qBound(0, m_verticalColumnOffset, maxColumns);
    const int maxRows = qMax(0, document()->characterCount());
    m_verticalRowOffset = qBound(0, m_verticalRowOffset, maxRows);
}

int NovelEditor::verticalCharacterAdvance() const
{
    return qMax(fontMetrics().height() + 2, fontMetrics().horizontalAdvance(QLatin1Char('M')));
}

int NovelEditor::verticalColumnAdvance() const
{
    return verticalCharacterAdvance() + 10;
}

void NovelEditor::paintEvent(QPaintEvent *event)
{
    if (!isVerticalWriting()) {
        QPlainTextEdit::paintEvent(event);
        return;
    }

    paintVerticalDocument(event);
}

void NovelEditor::paintVerticalDocument(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.setClipRect(event->rect());
    painter.fillRect(event->rect(), QColor("#1e1e2e"));
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(QColor("#cdd6f4"));

    const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
    const int leftMargin = 8;
    const int rightMargin = 8;
    const int charAdvance = verticalCharacterAdvance();
    const int columnAdvance = verticalColumnAdvance();
    const int lineHeight = fontMetrics().height();
    const int availableHeight = qMax(1, viewport()->height() - topMargin - 12);
    const int maxCharsPerColumn = qMax(1, availableHeight / lineHeight);
    const int visibleColumns = qMax(1, (viewport()->width() - leftMargin - rightMargin) / columnAdvance + 2);
    const QTextCursor selectionCursor = textCursor();
    const int selectionStart = selectionCursor.hasSelection() ? selectionCursor.selectionStart() : -1;
    const int selectionEnd = selectionCursor.hasSelection() ? selectionCursor.selectionEnd() : -1;

    QTextBlock block = document()->begin();
    const int startColumn = m_verticalColumnOffset;
    for (int columnIndex = 0; block.isValid(); ++columnIndex, block = block.next()) {
        if (columnIndex < startColumn)
            continue;
        const int visibleIndex = columnIndex - startColumn;
        if (visibleIndex >= visibleColumns)
            break;

        const int x = viewport()->width() - rightMargin - visibleIndex * columnAdvance;
        if (x + charAdvance < event->rect().left())
            continue;
        if (x > event->rect().right() + columnAdvance)
            continue;

        const QVector<VerticalUnit> units = parseVerticalUnits(block.text());
        int row = 0;
        for (const VerticalUnit &unit : units) {
            if (row < m_verticalRowOffset) {
                ++row;
                continue;
            }
            if (row - m_verticalRowOffset >= maxCharsPerColumn)
                break;

            const int displayRow = (row > 0 && isKinsokuProhibitedStart(unit.text)) ? row - 1 : row;
            const int y = topMargin + 4 + (displayRow - m_verticalRowOffset) * lineHeight;
            if (y > event->rect().bottom())
                break;

            const QRect unitRect = verticalUnitRect(x, y, unit);
            const int unitEnd = unit.start + unit.length;
            const bool selected = selectionStart >= 0 && selectionEnd >= 0
                && unit.start < selectionEnd && unitEnd > selectionStart;

            if (selected) {
                painter.fillRect(unitRect, QColor("#264653"));
            }

            if (unit.ruby) {
                const int baseHeight = qMax(1, lineHeight - 6);
                painter.drawText(QRect(unitRect.left(), unitRect.top(), unitRect.width(), baseHeight),
                                 Qt::AlignHCenter | Qt::AlignTop,
                                 unit.text);
                QFont rubyFont = painter.font();
                rubyFont.setPointSizeF(qMax(6.0, rubyFont.pointSizeF() - 2.0));
                painter.setFont(rubyFont);
                painter.setPen(QColor("#a6adc8"));
                painter.drawText(QRect(unitRect.right() - charAdvance / 2, unitRect.top(), charAdvance, lineHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter, unit.rubyText);
                painter.setPen(QColor("#cdd6f4"));
                painter.setFont(font());
            } else if (unit.tateChuYoko) {
                painter.save();
                painter.translate(unitRect.center());
                painter.rotate(-90);
                painter.drawText(QRect(-unitRect.height() / 2, -unitRect.width() / 2, unitRect.height(), unitRect.width()),
                                 Qt::AlignCenter, unit.text);
                painter.restore();
            } else {
                painter.drawText(unitRect, Qt::AlignHCenter | Qt::AlignVCenter, unit.text);
            }

            ++row;
        }
    }

    paintVerticalCursor(painter);
    paintVerticalPreedit(painter);
}

QVector<NovelEditor::VerticalUnit> NovelEditor::parseVerticalUnits(const QString &text) const
{
    QVector<VerticalUnit> units;
    units.reserve(text.size());

    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch == QChar(0xFF5C)) {
            const int rubyStart = text.indexOf(QChar(0x3008), i + 1);
            const int rubyEnd = rubyStart >= 0 ? text.indexOf(QChar(0x3009), rubyStart + 1) : -1;
            if (rubyStart > i + 1 && rubyEnd > rubyStart + 1) {
                const QString base = text.mid(i + 1, rubyStart - i - 1);
                if (!base.isEmpty()) {
                    units.push_back({base, i, rubyEnd - i + 1, true, text.mid(rubyStart + 1, rubyEnd - rubyStart - 1), false});
                    i = rubyEnd;
                    continue;
                }
            }
        }

        if (ch.isDigit()) {
            const int start = i;
            int len = 1;
            while (i + len < text.size() && text.at(i + len).isDigit() && len < 4)
                ++len;
            if (len >= 2) {
                units.push_back({text.mid(start, len), start, len, false, {}, true});
                i += len - 1;
                continue;
            }
        }

        if (i + 1 < text.size() && text.at(i + 1) == QChar(0x300c)) {
            units.push_back({ch, i, 1, false, {}, false});
            continue;
        }

        if (ch == QChar(0x3008)) {
            const int close = text.indexOf(QChar(0x3009), i + 1);
            if (close > i + 1 && !units.isEmpty()) {
                VerticalUnit &prev = units.last();
                if (!prev.ruby && !prev.tateChuYoko) {
                    prev.ruby = true;
                    prev.rubyText = text.mid(i + 1, close - i - 1);
                    prev.length += close - i + 1;
                    i = close;
                    continue;
                }
            }
        }

        units.push_back({QString(ch), i, 1, false, {}, false});
    }

    return units;
}

static bool isKinsokuProhibitedStart(const QString &text)
{
    if (text.isEmpty())
        return false;

    switch (text.front().unicode()) {
    case 0x3001: // 、
    case 0x3002: // 。
    case 0x300D: // 」
    case 0x300F: // 』
    case 0x3011: // 】
    case 0x3015: // 〉
    case 0xFF09: // ）
    case 0xFF3D: // ］
    case 0xFF5D: // ｝
        return true;
    default:
        return false;
    }
}

QRect NovelEditor::verticalUnitRect(int columnX, int rowY, const VerticalUnit &unit) const
{
    const int lineHeight = fontMetrics().height();
    const int charAdvance = verticalCharacterAdvance();
    if (unit.tateChuYoko)
        return QRect(columnX - charAdvance / 2, rowY, charAdvance * 2, lineHeight);
    if (unit.ruby)
        return QRect(columnX - charAdvance / 2, rowY, charAdvance * 2, lineHeight);
    return QRect(columnX, rowY, charAdvance, lineHeight);
}

void NovelEditor::paintVerticalCursor(QPainter &painter) const
{
    const QTextCursor cursor = textCursor();
    if (!cursor.isNull() && isVerticalWriting()) {
        const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
        const int rightMargin = 8;
        const int lineHeight = fontMetrics().height();
        const int columnAdvance = verticalColumnAdvance();
        const int x = viewport()->width() - rightMargin - (cursor.blockNumber() - m_verticalColumnOffset) * columnAdvance;
        const int y = topMargin + 4 + qMax(0, cursor.positionInBlock() - m_verticalRowOffset) * lineHeight;
        painter.fillRect(QRect(x - 1, y, 2, lineHeight), QColor("#f38ba8"));
    }
}

void NovelEditor::paintVerticalPreedit(QPainter &painter) const
{
    if (m_preeditText.isEmpty())
        return;

    const QTextCursor cursor = textCursor();
    if (!cursor.document())
        return;

    const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
    const int rightMargin = 8;
    const int lineHeight = fontMetrics().height();
    const int columnAdvance = verticalColumnAdvance();
    const int x = viewport()->width() - rightMargin - (cursor.blockNumber() - m_verticalColumnOffset) * columnAdvance;
    const int y = topMargin + 4 + qMax(0, cursor.positionInBlock() - m_verticalRowOffset) * lineHeight;

    painter.save();
    painter.setPen(QColor("#f9e2af"));
    painter.setBrush(QColor("#45475a"));
    painter.drawRect(QRect(x - 2, y - 2, verticalCharacterAdvance() + 4, lineHeight + 4));
    painter.drawText(QRect(x, y, verticalCharacterAdvance(), lineHeight), Qt::AlignHCenter | Qt::AlignVCenter, m_preeditText);
    painter.restore();
}

QTextCursor NovelEditor::cursorForVerticalPosition(const QPoint &pos) const
{
    const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
    const int rightMargin = 8;
    const int lineHeight = fontMetrics().height();
    const int columnAdvance = verticalColumnAdvance();
    const int availableHeight = qMax(1, viewport()->height() - topMargin - 12);
    const int maxCharsPerColumn = qMax(1, availableHeight / lineHeight);

    const int column = qMax(0, (viewport()->width() - rightMargin - pos.x()) / columnAdvance + m_verticalColumnOffset);
    const QTextBlock block = document()->findBlockByNumber(column);
    if (!block.isValid())
        return QTextCursor(document());

    const int row = qBound(0, (pos.y() - topMargin - 4) / lineHeight + m_verticalRowOffset, maxCharsPerColumn - 1);
    const int offset = qMin(row, qMax(0, block.length() - 1));

    QTextCursor cursor(document());
    cursor.setPosition(block.position() + offset);
    return cursor;
}

bool NovelEditor::moveVerticalCursor(int blockDelta, int charDelta, bool keepAnchor)
{
    QTextCursor cursor = textCursor();
    const QTextBlock currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const int targetBlockNumber = qMax(0, currentBlock.blockNumber() + blockDelta);
    const QTextBlock targetBlock = document()->findBlockByNumber(targetBlockNumber);
    if (!targetBlock.isValid())
        return false;

    const int targetOffset = qBound(0, cursor.positionInBlock() + charDelta, qMax(0, targetBlock.length() - 1));
    const int targetPos = targetBlock.position() + targetOffset;
    cursor.setPosition(targetPos, keepAnchor ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
    setTextCursor(cursor);
    return true;
}

void NovelEditor::wheelEvent(QWheelEvent *event)
{
    if (!isVerticalWriting()) {
        QPlainTextEdit::wheelEvent(event);
        return;
    }

    if (!event)
        return;

    const QPoint angle = event->angleDelta();
    if (event->modifiers().testFlag(Qt::ShiftModifier) || qAbs(angle.x()) > qAbs(angle.y())) {
        m_verticalColumnOffset = qMax(0, m_verticalColumnOffset - (angle.x() != 0 ? angle.x() : angle.y()) / 120);
    } else {
        m_verticalRowOffset = qMax(0, m_verticalRowOffset - angle.y() / 120);
    }
    clampVerticalOffsets();
    viewport()->update();
    event->accept();
}

void NovelEditor::inputMethodEvent(QInputMethodEvent *event)
{
    if (!event) {
        QPlainTextEdit::inputMethodEvent(event);
        return;
    }

    if (isVerticalWriting()) {
        m_preeditText = event->preeditString();

        if (!event->commitString().isEmpty()) {
            QTextCursor cursor = textCursor();
            if (cursor.hasSelection())
                cursor.removeSelectedText();
            cursor.insertText(event->commitString());
            setTextCursor(cursor);
            m_preeditText.clear();
        }

        viewport()->update();
        event->accept();
        return;
    }

    QPlainTextEdit::inputMethodEvent(event);
}

QVariant NovelEditor::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (!isVerticalWriting())
        return QPlainTextEdit::inputMethodQuery(query);

    switch (query) {
    case Qt::ImCursorRectangle: {
        const int topMargin = m_searchBar->isVisible() ? m_searchBarHeight : 0;
        const int rightMargin = 8;
        const int lineHeight = fontMetrics().height();
        const int columnAdvance = verticalColumnAdvance();
        const QTextCursor cursor = textCursor();
        const int x = viewport()->width() - rightMargin - (cursor.blockNumber() - m_verticalColumnOffset) * columnAdvance;
        const int y = topMargin + 4 + qMax(0, cursor.positionInBlock() - m_verticalRowOffset) * lineHeight;
        return QRect(x, y, verticalCharacterAdvance(), lineHeight);
    }
    case Qt::ImCursorPosition:
        return textCursor().position();
    case Qt::ImSurroundingText:
        return toPlainText();
    case Qt::ImCurrentSelection:
        return textCursor().selectedText();
    default:
        return QPlainTextEdit::inputMethodQuery(query);
    }
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

QVector<NovelEditor::OutlineEntry> NovelEditor::outlineEntries() const
{
    QVector<OutlineEntry> entries;
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        const QString text = block.text().trimmed();
        if (text.isEmpty())
            continue;

        int level = -1;
        QString title = text;

        int hashCount = 0;
        while (hashCount < text.size() && text.at(hashCount) == QLatin1Char('#'))
            ++hashCount;
        if (hashCount > 0 && hashCount <= 6 && hashCount < text.size() && text.at(hashCount).isSpace()) {
            level = hashCount;
            title = text.mid(hashCount).trimmed();
        } else if (text.startsWith(QStringLiteral("第")) && text.contains(QStringLiteral("章"))) {
            level = 1;
            title = text;
        } else if (text.startsWith(QStringLiteral("第")) && text.contains(QStringLiteral("話"))) {
            level = 2;
            title = text;
        } else if (text.startsWith(QStringLiteral("章 ")) || text.startsWith(QStringLiteral("■")) || text.startsWith(QStringLiteral("●"))) {
            level = 1;
            title = text.mid(1).trimmed();
        } else if (text.size() <= 40 && !text.endsWith(QStringLiteral("。")) && !text.endsWith(QStringLiteral("、"))) {
            if (text.contains(QStringLiteral("章")) || text.contains(QStringLiteral("話")) || text.contains(QStringLiteral("見出し")))
                level = 1;
        }

        if (level < 0)
            continue;

        entries.append({title.isEmpty() ? text : title, block.blockNumber() + 1, level});
    }

    return entries;
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

void NovelEditor::changeEvent(QEvent *event)
{
    if (event && event->type() == QEvent::LayoutDirectionChange) {
        updateViewportMargins();
        updateSearchBarGeometry();
    }

    QPlainTextEdit::changeEvent(event);
}

void NovelEditor::mousePressEvent(QMouseEvent *event)
{
    if (isVerticalWriting() && event && event->button() == Qt::LeftButton) {
        setTextCursor(cursorForVerticalPosition(event->position().toPoint()));
        event->accept();
        return;
    }

    QPlainTextEdit::mousePressEvent(event);
}

void NovelEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_searchBar->isVisible() && event->key() == Qt::Key_Escape) {
        hideSearchBar();
        event->accept();
        return;
    }

    if (handleDuplicateLineShortcut(event))
        return;
    if (handleOutlineNavigationShortcut(event))
        return;
    if (handleTabKey(event))
        return;
    if (handleReturnKey(event))
        return;

    if (isVerticalWriting()) {
        const bool keepAnchor = event->modifiers().testFlag(Qt::ShiftModifier);
        switch (event->key()) {
        case Qt::Key_Up:
            if (moveVerticalCursor(0, -1, keepAnchor)) {
                event->accept();
                return;
            }
            break;
        case Qt::Key_Down:
            if (moveVerticalCursor(0, 1, keepAnchor)) {
                event->accept();
                return;
            }
            break;
        case Qt::Key_Left:
            if (moveVerticalCursor(-1, 0, keepAnchor)) {
                event->accept();
                return;
            }
            break;
        case Qt::Key_Right:
            if (moveVerticalCursor(1, 0, keepAnchor)) {
                event->accept();
                return;
            }
            break;
        default:
            break;
        }
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

    if (handleClosingBracketSkip(event))
        return;
    if (handleAutoPairKey(event))
        return;

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

bool NovelEditor::handleDuplicateLineShortcut(QKeyEvent *event)
{
    if (!event || event->key() != Qt::Key_D || !event->modifiers().testFlag(Qt::ControlModifier))
        return false;
    if (event->modifiers().testFlag(Qt::AltModifier))
        return false;

    duplicateCurrentLine();
    event->accept();
    return true;
}

bool NovelEditor::handleOutlineNavigationShortcut(QKeyEvent *event)
{
    if (!event || !event->modifiers().testFlag(Qt::ControlModifier) || !event->modifiers().testFlag(Qt::ShiftModifier))
        return false;

    if (event->key() == Qt::Key_BracketLeft) {
        moveToAdjacentParagraph(false);
        event->accept();
        return true;
    }
    if (event->key() == Qt::Key_BracketRight) {
        moveToAdjacentParagraph(true);
        event->accept();
        return true;
    }

    return false;
}

void NovelEditor::applyIndentationToSelection(bool outdent)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        QTextCursor lineCursor = cursor;
        lineCursor.movePosition(QTextCursor::StartOfLine);
        if (outdent) {
            const QString lineText = lineCursor.block().text();
            int removeCount = 0;
            while (removeCount < 4 && removeCount < lineText.size() && lineText.at(removeCount).isSpace())
                ++removeCount;
            if (removeCount > 0) {
                lineCursor.setPosition(lineCursor.position() + removeCount, QTextCursor::KeepAnchor);
                lineCursor.removeSelectedText();
            }
            return;
        }
        lineCursor.insertText(QStringLiteral("    "));
        return;
    }

    const int selectionStart = cursor.selectionStart();
    const int selectionEnd = cursor.selectionEnd();
    const int firstBlock = document()->findBlock(selectionStart).blockNumber();
    const int lastBlock = document()->findBlock(qMax(selectionStart, selectionEnd - 1)).blockNumber();
    struct DeltaEntry { int pos; int delta; };
    QVector<DeltaEntry> deltas;

    QTextCursor editCursor(document());
    editCursor.beginEditBlock();
    for (int blockNumber = firstBlock; blockNumber <= lastBlock; ++blockNumber) {
        QTextBlock block = document()->findBlockByNumber(blockNumber);
        if (!block.isValid())
            continue;

        const int blockPos = block.position();
        QTextCursor lineCursor(document());
        lineCursor.setPosition(blockPos);
        lineCursor.movePosition(QTextCursor::StartOfLine);

        if (outdent) {
            const QString lineText = block.text();
            int removeCount = 0;
            while (removeCount < 4 && removeCount < lineText.size() && lineText.at(removeCount).isSpace())
                ++removeCount;
            if (removeCount <= 0)
                continue;

            lineCursor.setPosition(blockPos + removeCount, QTextCursor::KeepAnchor);
            lineCursor.removeSelectedText();
            deltas.append({blockPos, -removeCount});
        } else {
            lineCursor.insertText(QStringLiteral("    "));
            deltas.append({blockPos, 4});
        }
    }
    editCursor.endEditBlock();

    auto adjustPos = [&deltas](int pos) {
        int adjusted = pos;
        for (const auto &delta : deltas) {
            if (delta.pos < pos)
                adjusted += delta.delta;
        }
        return qMax(0, adjusted);
    };

    QTextCursor newCursor(document());
    newCursor.setPosition(adjustPos(selectionStart));
    newCursor.setPosition(adjustPos(selectionEnd), QTextCursor::KeepAnchor);
    setTextCursor(newCursor);
}

bool NovelEditor::handleReturnKey(QKeyEvent *event)
{
    if (!event || (event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter))
        return false;

    const QTextCursor cursor = textCursor();
    if (selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position())) {
        notifyProtectedEditBlocked();
        event->accept();
        return true;
    }

    const QString indent = currentLineIndentation();
    QPlainTextEdit::keyPressEvent(event);
    if (!indent.isEmpty())
        insertPlainText(indent);
    return true;
}

bool NovelEditor::handleTabKey(QKeyEvent *event)
{
    if (!event)
        return false;

    if (event->key() != Qt::Key_Tab && event->key() != Qt::Key_Backtab)
        return false;

    const QTextCursor cursor = textCursor();
    if (selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position())) {
        notifyProtectedEditBlocked();
        event->accept();
        return true;
    }

    const bool outdent = event->key() == Qt::Key_Backtab || event->modifiers().testFlag(Qt::ShiftModifier);
    if (cursor.hasSelection() || outdent) {
        applyIndentationToSelection(outdent);
    } else if (isVerticalWriting()) {
        QPlainTextEdit::keyPressEvent(event);
    } else {
        insertPlainText(QStringLiteral("    "));
    }

    event->accept();
    return true;
}

bool NovelEditor::handleAutoPairKey(QKeyEvent *event)
{
    if (!event || event->text().size() != 1)
        return false;

    const QChar ch = event->text().front();
    if (!isAutoPairOpenChar(ch))
        return false;

    const QTextCursor cursor = textCursor();
    if (selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position())) {
        notifyProtectedEditBlocked();
        event->accept();
        return true;
    }

    const QChar close = matchingClosingChar(ch);
    if (cursor.hasSelection()) {
        QTextCursor editCursor = cursor;
        const QString selected = cursor.selectedText();
        editCursor.insertText(QString(ch) + selected + QString(close));
        event->accept();
        return true;
    }

    QTextCursor editCursor = cursor;
    editCursor.insertText(QString(ch) + QString(close));
    editCursor.movePosition(QTextCursor::Left);
    setTextCursor(editCursor);
    event->accept();
    return true;
}

bool NovelEditor::handleClosingBracketSkip(QKeyEvent *event)
{
    if (!event || event->text().size() != 1)
        return false;

    const QChar ch = event->text().front();
    if (!isAutoPairCloseChar(ch))
        return false;

    QTextCursor cursor = textCursor();
    if (cursor.hasSelection())
        return false;

    const QString text = toPlainText();
    const int pos = cursor.position();
    if (pos >= 0 && pos < text.size() && text.at(pos) == ch) {
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        event->accept();
        return true;
    }

    return false;
}

QString NovelEditor::currentLineIndentation() const
{
    const QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    QString indent;
    for (QChar ch : line) {
        if (!ch.isSpace())
            break;
        indent.append(ch);
    }
    return indent;
}

int NovelEditor::currentLineNumber() const
{
    return textCursor().blockNumber() + 1;
}

void NovelEditor::gotoLine(int lineNumber)
{
    const int targetLine = qMax(1, lineNumber);
    QTextBlock block = document()->findBlockByNumber(targetLine - 1);
    if (!block.isValid())
        block = document()->lastBlock();

    QTextCursor cursor(block);
    setTextCursor(cursor);
    centerCursor();
}

void NovelEditor::duplicateCurrentLine()
{
    const QTextCursor cursor = textCursor();
    if (selectionIntersectsProtected(cursor) || cursorIsInsideProtected(cursor.position())) {
        notifyProtectedEditBlocked();
        return;
    }

    QTextCursor editCursor = cursor;
    editCursor.beginEditBlock();

    if (cursor.hasSelection()) {
        const QString selected = cursor.selectedText().replace(QChar(0x2029), '\n');
        editCursor.setPosition(cursor.selectionEnd());
        editCursor.insertText(QStringLiteral("\n") + selected);
    } else {
        QTextCursor lineCursor(cursor.block());
        lineCursor.select(QTextCursor::LineUnderCursor);
        const QString lineText = lineCursor.selectedText();
        lineCursor.movePosition(QTextCursor::EndOfLine);
        lineCursor.insertText(QStringLiteral("\n") + lineText);
    }

    editCursor.endEditBlock();
}

bool NovelEditor::moveToAdjacentParagraph(bool forward)
{
    const QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    if (!block.isValid())
        return false;

    if (forward) {
        block = block.next();
        while (block.isValid()) {
            if (block.text().trimmed().isEmpty()) {
                block = block.next();
                continue;
            }
            if (!block.previous().isValid() || block.previous().text().trimmed().isEmpty()) {
                QTextCursor nextCursor(block);
                setTextCursor(nextCursor);
                centerCursor();
                return true;
            }
            block = block.next();
        }
    } else {
        block = block.previous();
        while (block.isValid()) {
            if (block.text().trimmed().isEmpty()) {
                block = block.previous();
                continue;
            }
            if (!block.next().isValid() || block.next().text().trimmed().isEmpty()) {
                QTextCursor prevCursor(block);
                setTextCursor(prevCursor);
                centerCursor();
                return true;
            }
            block = block.previous();
        }
    }

    return false;
}

QChar NovelEditor::matchingClosingChar(QChar open)
{
    switch (open.unicode()) {
    case '(': return QChar(')');
    case '[': return QChar(']');
    case '{': return QChar('}');
    case 0x300C: return QChar(0x300D);
    case 0x300E: return QChar(0x300F);
    case 0x3010: return QChar(0x3011);
    case 0xFF08: return QChar(0xFF09);
    case 0xFF3B: return QChar(0xFF3D);
    case 0xFF5B: return QChar(0xFF5D);
    case 0x3008: return QChar(0x3009);
    case 0x300A: return QChar(0x300B);
    default:
        return QChar();
    }
}

bool NovelEditor::isAutoPairOpenChar(QChar ch)
{
    switch (ch.unicode()) {
    case '(':
    case '[':
    case '{':
    case 0x300C:
    case 0x300E:
    case 0x3010:
    case 0xFF08:
    case 0xFF3B:
    case 0xFF5B:
    case 0x3008:
    case 0x300A:
        return true;
    default:
        return false;
    }
}

bool NovelEditor::isAutoPairCloseChar(QChar ch)
{
    switch (ch.unicode()) {
    case ')':
    case ']':
    case '}':
    case 0x300D:
    case 0x300F:
    case 0x3011:
    case 0xFF09:
    case 0xFF3D:
    case 0xFF5D:
    case 0x3009:
    case 0x300B:
        return true;
    default:
        return false;
    }
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
    const int gutterWidth = lineNumberAreaWidth();
    if (m_textLayoutDirection == Qt::RightToLeft)
        setViewportMargins(0, topMargin, gutterWidth, 0);
    else
        setViewportMargins(gutterWidth, topMargin, 0, 0);

    QRect cr = contentsRect();
    if (m_textLayoutDirection == Qt::RightToLeft) {
        m_lineNumberArea->setGeometry(QRect(cr.right() - gutterWidth + 1, cr.top(), gutterWidth, cr.height()));
    } else {
        m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), gutterWidth, cr.height()));
    }
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
