#include "searchpanel.h"
#include "wobjectimpl.h"

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPair>
#include <QStandardItem>
#include <QVBoxLayout>

W_OBJECT_IMPL(SearchPanel)

namespace {
QString makeExcerpt(const QString &text, const QString &query)
{
    QString normalized = text;
    normalized.replace('\n', ' ');
    normalized.replace('\r', ' ');
    normalized = normalized.simplified();
    if (normalized.isEmpty())
        return QString();

    const int match = query.isEmpty() ? -1 : normalized.indexOf(query, 0, Qt::CaseInsensitive);
    const int start = match < 0 ? 0 : qMax(0, match - 36);
    const int length = match < 0 ? 120 : query.size() + 96;
    QString excerpt = normalized.mid(start, length).trimmed();
    if (start > 0)
        excerpt.prepend("...");
    if (start + length < normalized.size())
        excerpt.append("...");
    return excerpt;
}

QStandardItem *makePrimaryItem(const QString &kind, const QString &id,
                               const QString &title, const QString &type, const QString &excerpt)
{
    auto *item = new QStandardItem(title);
    item->setEditable(false);
    item->setData(kind, Qt::UserRole);
    item->setData(id, Qt::UserRole + 1);
    item->setData(type, Qt::UserRole + 2);
    item->setToolTip(excerpt.isEmpty() ? type : type + "\n" + excerpt);
    return item;
}

void appendResult(QStandardItemModel *model, const QString &kind, const QString &id,
                  const QString &title, const QString &type, const QString &excerpt)
{
    auto *item = makePrimaryItem(kind, id, title, type, excerpt);
    auto *typeItem = new QStandardItem(type);
    typeItem->setEditable(false);

    auto *excerptItem = new QStandardItem(excerpt);
    excerptItem->setEditable(false);

    model->appendRow({item, typeItem, excerptItem});
}
}

DockPaneSpec SearchPanel::dockSpec()
{
    return { QStringLiteral("検索"), DockPlacement::Bottom, true };
}

SearchPanel::SearchPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_queryEdit = new QLineEdit(this);
    m_queryEdit->setPlaceholderText("プロジェクト全体を検索...");
    layout->addWidget(m_queryEdit);

    m_resultsModel = new QStandardItemModel(this);
    m_resultsModel->setHorizontalHeaderLabels({"結果", "種類", "抜粋"});

    m_resultsView = new QTreeView(this);
    m_resultsView->setModel(m_resultsModel);
    m_resultsView->setRootIsDecorated(true);
    m_resultsView->setAlternatingRowColors(true);
    m_resultsView->setUniformRowHeights(true);
    m_resultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsView->header()->setStretchLastSection(true);
    m_resultsView->setStyleSheet(
        "QTreeView { background: #1e1e1e; color: #cccccc; border: 1px solid #333333; }"
        "QTreeView::item:selected { background: #094771; }"
        "QTreeView::item:hover { background: #2a2d2e; }"
    );
    layout->addWidget(m_resultsView, 1);

    connect(m_queryEdit, &QLineEdit::textChanged, this, &SearchPanel::rebuildResults);
    connect(m_queryEdit, &QLineEdit::returnPressed, this, [this]() {
        onResultActivated(m_resultsView->currentIndex());
    });
    connect(m_resultsView, &QTreeView::doubleClicked, this, &SearchPanel::onResultActivated);
}

void SearchPanel::loadProject(const NovelProject &project)
{
    m_project = project;
    rebuildResults();
}

void SearchPanel::setQuery(const QString &query)
{
    if (m_queryEdit->text() != query)
        m_queryEdit->setText(query);
    m_queryEdit->setFocus();
    m_queryEdit->selectAll();
}

void SearchPanel::rebuildResults()
{
    const QString query = m_queryEdit->text().trimmed();
    m_resultsModel->removeRows(0, m_resultsModel->rowCount());
    if (query.isEmpty())
        return;

    for (const auto &chapter : m_project.chapters) {
        if (chapter.title.contains(query, Qt::CaseInsensitive) || chapter.summary.contains(query, Qt::CaseInsensitive))
            appendResult(m_resultsModel, "chapter", chapter.id, chapter.title, "章", makeExcerpt(chapter.summary, query));

        for (const auto &episode : chapter.episodes) {
            const bool titleMatch = episode.title.contains(query, Qt::CaseInsensitive);
            const bool summaryMatch = episode.summary.contains(query, Qt::CaseInsensitive);
            const bool contentMatch = episode.content.contains(query, Qt::CaseInsensitive);
            const bool metaMatch = episode.povCharacter.contains(query, Qt::CaseInsensitive)
                || episode.timePeriod.contains(query, Qt::CaseInsensitive)
                || episode.sceneType.contains(query, Qt::CaseInsensitive)
                || episode.revisionNotes.contains(query, Qt::CaseInsensitive);
            if (!titleMatch && !summaryMatch && !contentMatch && !metaMatch)
                continue;

            const QString textForExcerpt = contentMatch ? episode.content : summaryMatch ? episode.summary : episode.revisionNotes;
            appendResult(m_resultsModel, "episode", episode.id, chapter.title + " / " + episode.title,
                         "エピソード", makeExcerpt(textForExcerpt, query));
        }
    }

    for (const auto &character : m_project.characters) {
        if (character.name.contains(query, Qt::CaseInsensitive)
            || character.role.contains(query, Qt::CaseInsensitive)
            || character.description.contains(query, Qt::CaseInsensitive)
            || character.notes.contains(query, Qt::CaseInsensitive)) {
            const QString textForExcerpt = character.notes.contains(query, Qt::CaseInsensitive) ? character.notes : character.description;
            appendResult(m_resultsModel, "character", character.id, "キャラ: " + character.name,
                         "キャラクター", makeExcerpt(textForExcerpt, query));
        }
    }

    for (const auto &location : m_project.locations) {
        if (location.name.contains(query, Qt::CaseInsensitive)
            || location.description.contains(query, Qt::CaseInsensitive)
            || location.notes.contains(query, Qt::CaseInsensitive)) {
            const QString textForExcerpt = location.notes.contains(query, Qt::CaseInsensitive) ? location.notes : location.description;
            appendResult(m_resultsModel, "location", location.id, "場所: " + location.name,
                         "ロケーション", makeExcerpt(textForExcerpt, query));
        }
    }

    appendReferenceResults(query);

    m_resultsView->resizeColumnToContents(0);
    m_resultsView->resizeColumnToContents(1);
    m_resultsView->expandAll();
    if (m_resultsModel->rowCount() > 0)
        m_resultsView->setCurrentIndex(m_resultsModel->index(0, 0));
}

void SearchPanel::appendReferenceResults(const QString &query)
{
    auto appendEpisodeChild = [&](QStandardItem *parent, const QString &episodeId,
                                  const QString &title, const QString &excerpt) {
        auto *item = makePrimaryItem(QStringLiteral("episode"), episodeId, title, QStringLiteral("参照エピソード"), excerpt);
        auto *typeItem = new QStandardItem(QStringLiteral("参照"));
        typeItem->setEditable(false);
        auto *excerptItem = new QStandardItem(excerpt);
        excerptItem->setEditable(false);
        parent->appendRow({item, typeItem, excerptItem});
    };

    for (const auto &character : m_project.characters) {
        if (!character.name.contains(query, Qt::CaseInsensitive))
            continue;

        QVector<QPair<QString, QString>> references;
        for (const auto &chapter : m_project.chapters) {
            for (const auto &episode : chapter.episodes) {
                const bool referenced = episode.content.contains(character.name, Qt::CaseInsensitive)
                    || episode.summary.contains(character.name, Qt::CaseInsensitive)
                    || episode.povCharacter.contains(character.name, Qt::CaseInsensitive);
                if (!referenced)
                    continue;
                references.append({episode.id, chapter.title + " / " + episode.title});
            }
        }
        if (references.isEmpty())
            continue;

        auto *parent = makePrimaryItem(QStringLiteral("character"), character.id,
                                       QStringLiteral("参照: キャラ %1").arg(character.name),
                                       QStringLiteral("参照ナビ"), character.description);
        auto *typeItem = new QStandardItem(QStringLiteral("キャラ参照"));
        typeItem->setEditable(false);
        auto *excerptItem = new QStandardItem(QStringLiteral("%1 件").arg(references.size()));
        excerptItem->setEditable(false);
        m_resultsModel->appendRow({parent, typeItem, excerptItem});

        for (const auto &reference : references) {
            const QString episodeTitle = reference.second;
            QString excerpt;
            for (const auto &chapter : m_project.chapters) {
                for (const auto &episode : chapter.episodes) {
                    if (episode.id != reference.first)
                        continue;
                    excerpt = makeExcerpt(episode.content.isEmpty() ? episode.summary : episode.content, character.name);
                    break;
                }
            }
            appendEpisodeChild(parent, reference.first, episodeTitle, excerpt);
        }
    }

    for (const auto &location : m_project.locations) {
        if (!location.name.contains(query, Qt::CaseInsensitive))
            continue;

        QVector<QPair<QString, QString>> references;
        for (const auto &chapter : m_project.chapters) {
            for (const auto &episode : chapter.episodes) {
                const bool referenced = episode.content.contains(location.name, Qt::CaseInsensitive)
                    || episode.summary.contains(location.name, Qt::CaseInsensitive)
                    || episode.sceneType.contains(location.name, Qt::CaseInsensitive);
                if (!referenced)
                    continue;
                references.append({episode.id, chapter.title + " / " + episode.title});
            }
        }
        if (references.isEmpty())
            continue;

        auto *parent = makePrimaryItem(QStringLiteral("location"), location.id,
                                       QStringLiteral("参照: 場所 %1").arg(location.name),
                                       QStringLiteral("参照ナビ"), location.description);
        auto *typeItem = new QStandardItem(QStringLiteral("場所参照"));
        typeItem->setEditable(false);
        auto *excerptItem = new QStandardItem(QStringLiteral("%1 件").arg(references.size()));
        excerptItem->setEditable(false);
        m_resultsModel->appendRow({parent, typeItem, excerptItem});

        for (const auto &reference : references) {
            const QString episodeTitle = reference.second;
            QString excerpt;
            for (const auto &chapter : m_project.chapters) {
                for (const auto &episode : chapter.episodes) {
                    if (episode.id != reference.first)
                        continue;
                    excerpt = makeExcerpt(episode.content.isEmpty() ? episode.summary : episode.content, location.name);
                    break;
                }
            }
            appendEpisodeChild(parent, reference.first, episodeTitle, excerpt);
        }
    }
}

void SearchPanel::onResultActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    const QModelIndex firstColumn = index.sibling(index.row(), 0);
    const QString kind = firstColumn.data(Qt::UserRole).toString();
    const QString id = firstColumn.data(Qt::UserRole + 1).toString();
    const QString query = m_queryEdit->text().trimmed();
    if (!kind.isEmpty() && !id.isEmpty()) {
        emit openDocumentRequested(kind, id);
        emit searchResultActivated(kind, id, query);
    }
}
