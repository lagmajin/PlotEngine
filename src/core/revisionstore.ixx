module;

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QVector>

export module PlotEngine.Core.RevisionStore;
import PlotEngine.Core.RevisionManager;

export namespace PlotEngine::RevisionStore {

QString historyDirectory(const QString &projectPath);
QVector<PlotEngine::Revisions::Revision> loadEpisodeHistory(const QString &projectPath, const QString &episodeId);
bool saveEpisodeHistory(const QString &projectPath, const QString &episodeId, const QVector<PlotEngine::Revisions::Revision> &revisions);
bool appendRevision(const QString &projectPath, const QString &episodeId, const QString &content,
                    PlotEngine::Revisions::RevisionType type, const QString &notes = QString(),
                    const QString &aiModel = QString(), int maxCount = 100);

}

namespace {

QString revisionFilePath(const QString &projectPath, const QString &episodeId)
{
    if (projectPath.isEmpty() || episodeId.isEmpty())
        return QString();

    return PlotEngine::RevisionStore::historyDirectory(projectPath) + QLatin1Char('/') + episodeId + QStringLiteral(".json");
}

}

namespace PlotEngine::RevisionStore {

QString historyDirectory(const QString &projectPath)
{
    if (projectPath.isEmpty())
        return QString();

    return QFileInfo(projectPath).absoluteFilePath() + QStringLiteral(".history");
}

QVector<PlotEngine::Revisions::Revision> loadEpisodeHistory(const QString &projectPath, const QString &episodeId)
{
    QVector<PlotEngine::Revisions::Revision> revisions;
    const QString filePath = revisionFilePath(projectPath, episodeId);
    if (filePath.isEmpty())
        return revisions;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return revisions;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray())
        return revisions;

    for (const auto &value : doc.array()) {
        if (value.isObject())
            revisions.append(PlotEngine::Revisions::Revision::fromJson(value.toObject()));
    }
    return revisions;
}

bool saveEpisodeHistory(const QString &projectPath, const QString &episodeId, const QVector<PlotEngine::Revisions::Revision> &revisions)
{
    const QString dirPath = historyDirectory(projectPath);
    const QString filePath = revisionFilePath(projectPath, episodeId);
    if (dirPath.isEmpty() || filePath.isEmpty())
        return false;

    QDir dir;
    if (!dir.exists(dirPath) && !dir.mkpath(dirPath))
        return false;

    QJsonArray array;
    for (const auto &revision : revisions)
        array.append(revision.toJson());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

bool appendRevision(const QString &projectPath, const QString &episodeId, const QString &content,
                    PlotEngine::Revisions::RevisionType type, const QString &notes,
                    const QString &aiModel, int maxCount)
{
    if (projectPath.isEmpty() || episodeId.isEmpty())
        return false;

    QVector<PlotEngine::Revisions::Revision> revisions = loadEpisodeHistory(projectPath, episodeId);
    if (!revisions.isEmpty()) {
        const auto &last = revisions.constLast();
        if (last.content == content && last.type == type && last.notes == notes && last.aiModel == aiModel)
            return true;
    }

    auto created = PlotEngine::Revisions::createRevision(episodeId, content, type, notes, aiModel);
    revisions += created;
    revisions = PlotEngine::Revisions::pruneRevisions(revisions, maxCount);
    return saveEpisodeHistory(projectPath, episodeId, revisions);
}

}
