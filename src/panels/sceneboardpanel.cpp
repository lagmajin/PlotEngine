#include "sceneboardpanel.h"
#include "wobjectimpl.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QIcon>

W_OBJECT_IMPL(SceneBoardPanel)

namespace {
QIcon cardIcon(bool dirty)
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(dirty ? "#ff6b6b" : "#4f8cff"));
    p.drawRoundedRect(2, 2, 16, 16, 3, 3);
    p.setBrush(QColor("#1e1e2e"));
    p.drawRect(5, 5, 10, 2);
    p.drawRect(5, 9, 8, 2);
    return QIcon(pix);
}
}

SceneBoardPanel::SceneBoardPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("Scene Cards"), this);
    title->setStyleSheet("QLabel { color: #ffffff; font-weight: 700; font-size: 15px; }");
    layout->addWidget(title);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    layout->addWidget(m_tabs, 1);

    auto *hint = new QLabel(QStringLiteral("カードを並べ替えると章内順序が変わります。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("QLabel { color: #a6adc8; }");
    layout->addWidget(hint);

    setStyleSheet(
        "QTabWidget::pane { border: 1px solid #333333; background: #1e1e1e; }"
        "QTabBar::tab { background: #252526; color: #cdd6f4; padding: 6px 10px; }"
        "QTabBar::tab:selected { background: #2d2d2d; }"
        "QListWidget { background: #1e1e1e; border: none; }"
        "QListWidget::item { background: #262833; border: 1px solid #3c3f52; border-radius: 8px; margin: 6px; padding: 10px; color: #e2e8f0; }"
        "QListWidget::item:selected { background: #094771; border-color: #3b82f6; }"
        "QListWidget::item:hover { background: #2d3142; }"
    );
}

DockPaneSpec SceneBoardPanel::dockSpec()
{
    return { QStringLiteral("Scene Cards"), DockPlacement::Bottom, false };
}

void SceneBoardPanel::loadProject(const NovelProject &project)
{
    m_loading = true;
    m_project = project;
    m_tabs->clear();
    m_listsByChapter.clear();

    if (project.chapters.isEmpty()) {
        auto *page = new QWidget(m_tabs);
        auto *pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(12, 12, 12, 12);
        auto *label = new QLabel(QStringLiteral("カードを表示するには章を追加してください。"), page);
        label->setStyleSheet("QLabel { color: #a6adc8; }");
        label->setWordWrap(true);
        pageLayout->addWidget(label);
        pageLayout->addStretch(1);
        m_tabs->addTab(page, QStringLiteral("空"));
        m_loading = false;
        return;
    }

    for (const auto &chapter : project.chapters) {
        QListWidget *list = createChapterBoard(chapter.id, chapter.title);
        m_listsByChapter.insert(chapter.id, list);

        for (const auto &episode : chapter.episodes) {
            QString detail = episode.summary.trimmed();
            if (detail.isEmpty())
                detail = QStringLiteral("要約なし");
            detail += QStringLiteral("\n");
            if (!episode.povCharacter.trimmed().isEmpty())
                detail += QStringLiteral("POV: %1\n").arg(episode.povCharacter.trimmed());
            if (!episode.timePeriod.trimmed().isEmpty())
                detail += QStringLiteral("時: %1\n").arg(episode.timePeriod.trimmed());
            detail += QStringLiteral("%1 文字").arg(episode.content.size());

            auto *item = new QListWidgetItem(cardIcon(false), episode.title, list);
            item->setData(Qt::UserRole, episode.id);
            item->setData(Qt::UserRole + 1, chapter.id);
            item->setToolTip(detail);
            item->setSizeHint(QSize(220, 112));
            item->setText(episode.title + "\n" + detail);
            list->addItem(item);
        }

        if (chapter.episodes.isEmpty()) {
            auto *item = new QListWidgetItem(QStringLiteral("エピソードがありません"), list);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
            item->setSizeHint(QSize(220, 112));
            item->setToolTip(QStringLiteral("章にエピソードを追加してください。"));
            list->addItem(item);
        }

        m_tabs->addTab(list, chapter.title);
    }

    refreshTabLabels();
    m_loading = false;
}

void SceneBoardPanel::setCurrentEpisode(const QString &chapterId, const QString &episodeId)
{
    QListWidget *list = m_listsByChapter.value(chapterId, nullptr);
    if (!list)
        return;

    m_tabs->setCurrentWidget(list);
    selectItem(list, episodeId);
}

QListWidget *SceneBoardPanel::createChapterBoard(const QString &chapterId, const QString &chapterTitle)
{
    auto *list = new QListWidget(m_tabs);
    list->setProperty("chapterId", chapterId);
    list->setProperty("chapterTitle", chapterTitle);
    list->setViewMode(QListView::IconMode);
    list->setDragDropMode(QAbstractItemView::InternalMove);
    list->setMovement(QListView::Free);
    list->setDefaultDropAction(Qt::MoveAction);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setResizeMode(QListView::Adjust);
    list->setWrapping(true);
    list->setWordWrap(true);
    list->setSpacing(6);
    list->setGridSize(QSize(240, 128));
    list->setIconSize(QSize(20, 20));
    list->setUniformItemSizes(false);

    connect(list, &QListWidget::itemDoubleClicked, this, &SceneBoardPanel::onCardActivated);
    connect(list->model(), &QAbstractItemModel::rowsMoved, this, [this, chapterId]() {
        onOrderChanged(chapterId);
    });
    return list;
}

void SceneBoardPanel::refreshTabLabels()
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *list = qobject_cast<QListWidget*>(m_tabs->widget(i));
        if (!list)
            continue;
        const QString chapterTitle = list->property("chapterTitle").toString();
        const int cardCount = qMax(0, list->count());
        m_tabs->setTabText(i, chapterTitle + QStringLiteral(" (%1)").arg(cardCount));
    }
}

void SceneBoardPanel::selectItem(QListWidget *list, const QString &episodeId)
{
    if (!list || episodeId.isEmpty())
        return;

    for (int i = 0; i < list->count(); ++i) {
        auto *item = list->item(i);
        if (!item)
            continue;
        if (item->data(Qt::UserRole).toString() != episodeId)
            continue;
        list->setCurrentItem(item);
        list->scrollToItem(item);
        return;
    }
}

void SceneBoardPanel::onCardActivated(QListWidgetItem *item)
{
    if (!item || m_loading)
        return;

    auto *list = qobject_cast<QListWidget*>(item->listWidget());
    if (!list)
        return;

    const QString chapterId = list->property("chapterId").toString();
    const QString episodeId = item->data(Qt::UserRole).toString();
    if (chapterId.isEmpty() || episodeId.isEmpty())
        return;

    emit episodeSelected(chapterId, episodeId);
}

void SceneBoardPanel::onOrderChanged(const QString &chapterId)
{
    if (m_loading)
        return;

    QListWidget *list = m_listsByChapter.value(chapterId, nullptr);
    if (!list)
        return;

    QStringList orderedEpisodeIds;
    orderedEpisodeIds.reserve(list->count());
    for (int i = 0; i < list->count(); ++i) {
        auto *item = list->item(i);
        if (!item)
            continue;
        const QString episodeId = item->data(Qt::UserRole).toString();
        if (!episodeId.isEmpty())
            orderedEpisodeIds.append(episodeId);
    }

    if (!orderedEpisodeIds.isEmpty())
        emit episodeOrderChanged(chapterId, orderedEpisodeIds);
}
