module;

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QSet>

export module PlotEngine.Core.DocumentCommands;
import PlotEngine.Core.NovelProject;

namespace {
QString makeGeneratedId(const char *prefix)
{
    static quint64 counter = 0;
    return QStringLiteral("%1_%2_%3")
        .arg(QLatin1String(prefix))
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(++counter);
}
}

export namespace PlotEngine::Docs {

Chapter *findChapterById(NovelProject &project, const QString &chapterId);
const Chapter *findChapterById(const NovelProject &project, const QString &chapterId);

Episode *findEpisodeById(NovelProject &project, const QString &chapterId, const QString &episodeId);
const Episode *findEpisodeById(const NovelProject &project, const QString &chapterId, const QString &episodeId);
Episode *findEpisodeById(NovelProject &project, const QString &episodeId, QString *chapterId = nullptr);
const Episode *findEpisodeById(const NovelProject &project, const QString &episodeId, QString *chapterId = nullptr);

CharacterEntry *findCharacterById(NovelProject &project, const QString &characterId);
const CharacterEntry *findCharacterById(const NovelProject &project, const QString &characterId);
LocationEntry *findLocationById(NovelProject &project, const QString &locationId);
const LocationEntry *findLocationById(const NovelProject &project, const QString &locationId);

QString documentTitleForProject(const NovelProject &project, const QString &kind, const QString &id);
bool renameChapter(NovelProject &project, const QString &chapterId, const QString &title);
bool renameEpisode(NovelProject &project, const QString &chapterId, const QString &episodeId, const QString &title);
QString addChapter(NovelProject &project, QString *createdEpisodeId = nullptr);
QString addEpisode(NovelProject &project, const QString &chapterId);
bool reorderEpisodes(NovelProject &project, const QString &chapterId, const QStringList &orderedEpisodeIds);

}

namespace PlotEngine::Docs {

Chapter *findChapterById(NovelProject &project, const QString &chapterId)
{
    for (auto &chapter : project.chapters) {
        if (chapter.id == chapterId)
            return &chapter;
    }
    return nullptr;
}

const Chapter *findChapterById(const NovelProject &project, const QString &chapterId)
{
    for (const auto &chapter : project.chapters) {
        if (chapter.id == chapterId)
            return &chapter;
    }
    return nullptr;
}

Episode *findEpisodeById(NovelProject &project, const QString &chapterId, const QString &episodeId)
{
    Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return nullptr;

    for (auto &episode : chapter->episodes) {
        if (episode.id == episodeId)
            return &episode;
    }
    return nullptr;
}

const Episode *findEpisodeById(const NovelProject &project, const QString &chapterId, const QString &episodeId)
{
    const Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return nullptr;

    for (const auto &episode : chapter->episodes) {
        if (episode.id == episodeId)
            return &episode;
    }
    return nullptr;
}

Episode *findEpisodeById(NovelProject &project, const QString &episodeId, QString *chapterId)
{
    for (auto &chapter : project.chapters) {
        for (auto &episode : chapter.episodes) {
            if (episode.id != episodeId)
                continue;
            if (chapterId)
                *chapterId = chapter.id;
            return &episode;
        }
    }
    return nullptr;
}

const Episode *findEpisodeById(const NovelProject &project, const QString &episodeId, QString *chapterId)
{
    for (const auto &chapter : project.chapters) {
        for (const auto &episode : chapter.episodes) {
            if (episode.id != episodeId)
                continue;
            if (chapterId)
                *chapterId = chapter.id;
            return &episode;
        }
    }
    return nullptr;
}

CharacterEntry *findCharacterById(NovelProject &project, const QString &characterId)
{
    for (auto &character : project.characters) {
        if (character.id == characterId)
            return &character;
    }
    return nullptr;
}

const CharacterEntry *findCharacterById(const NovelProject &project, const QString &characterId)
{
    for (const auto &character : project.characters) {
        if (character.id == characterId)
            return &character;
    }
    return nullptr;
}

LocationEntry *findLocationById(NovelProject &project, const QString &locationId)
{
    for (auto &location : project.locations) {
        if (location.id == locationId)
            return &location;
    }
    return nullptr;
}

const LocationEntry *findLocationById(const NovelProject &project, const QString &locationId)
{
    for (const auto &location : project.locations) {
        if (location.id == locationId)
            return &location;
    }
    return nullptr;
}

QString documentTitleForProject(const NovelProject &project, const QString &kind, const QString &id)
{
    if (kind == "episode") {
        for (const auto &chapter : project.chapters) {
            for (const auto &episode : chapter.episodes) {
                if (episode.id == id)
                    return chapter.title + " / " + episode.title;
            }
        }
    } else if (kind == "character") {
        if (const CharacterEntry *character = findCharacterById(project, id))
            return QStringLiteral("キャラ: ") + character->name;
    } else if (kind == "location") {
        if (const LocationEntry *location = findLocationById(project, id))
            return QStringLiteral("場所: ") + location->name;
    }

    return QString();
}

bool renameChapter(NovelProject &project, const QString &chapterId, const QString &title)
{
    Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return false;

    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty())
        return false;

    chapter->title = trimmed;
    return true;
}

bool renameEpisode(NovelProject &project, const QString &chapterId, const QString &episodeId, const QString &title)
{
    Episode *episode = findEpisodeById(project, chapterId, episodeId);
    if (!episode)
        return false;

    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty())
        return false;

    episode->title = trimmed;
    episode->modifiedAt = QDateTime::currentDateTime();
    return true;
}

QString addChapter(NovelProject &project, QString *createdEpisodeId)
{
    Chapter chapter;
    chapter.id = makeGeneratedId("chapter");
    chapter.title = QStringLiteral("第%1章").arg(project.chapters.size() + 1);
    chapter.sortOrder = project.chapters.size();

    Episode episode;
    episode.id = makeGeneratedId("episode");
    episode.title = QStringLiteral("エピソード1");
    episode.createdAt = QDateTime::currentDateTime();
    episode.modifiedAt = episode.createdAt;
    chapter.episodes.append(episode);

    if (createdEpisodeId)
        *createdEpisodeId = episode.id;

    const QString createdChapterId = chapter.id;
    project.chapters.append(chapter);
    return createdChapterId;
}

QString addEpisode(NovelProject &project, const QString &chapterId)
{
    Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return QString();

    Episode episode;
    episode.id = makeGeneratedId("episode");
    episode.title = QStringLiteral("エピソード%1").arg(chapter->episodes.size() + 1);
    episode.createdAt = QDateTime::currentDateTime();
    episode.modifiedAt = episode.createdAt;

    const QString createdEpisodeId = episode.id;
    chapter->episodes.append(episode);
    return createdEpisodeId;
}

bool reorderEpisodes(NovelProject &project, const QString &chapterId, const QStringList &orderedEpisodeIds)
{
    Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return false;

    if (orderedEpisodeIds.isEmpty() || chapter->episodes.size() != orderedEpisodeIds.size())
        return false;

    QVector<Episode> reordered;
    reordered.reserve(chapter->episodes.size());
    QSet<QString> seen;

    for (const auto &episodeId : orderedEpisodeIds) {
        if (seen.contains(episodeId))
            return false;
        seen.insert(episodeId);
        auto it = std::find_if(chapter->episodes.begin(), chapter->episodes.end(),
            [&](const Episode &episode) { return episode.id == episodeId; });
        if (it == chapter->episodes.end())
            return false;
        reordered.append(*it);
    }

    if (reordered.size() != chapter->episodes.size())
        return false;

    chapter->episodes = std::move(reordered);
    return true;
}

}
