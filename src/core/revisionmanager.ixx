module;

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QUuid>
#include <algorithm>
#include <optional>

import std;

export module PlotEngine.Core.RevisionManager;

export enum class RevisionType {
    ManualEdit,
    AiGenerated,
    AiPolished,
    Import,
    Rollback
};

export struct Revision {
    QString id;
    QDateTime timestamp;
    QString content;
    RevisionType type = RevisionType::ManualEdit;
    QString notes;
    QString aiModel;
    int wordCount = 0;

    QJsonObject toJson() const;
    static Revision fromJson(const QJsonObject &json);
};

inline QString revisionTypeToString(RevisionType type)
{
    switch (type) {
        case RevisionType::ManualEdit: return "manual";
        case RevisionType::AiGenerated: return "ai_generated";
        case RevisionType::AiPolished: return "ai_polished";
        case RevisionType::Import: return "import";
        case RevisionType::Rollback: return "rollback";
    }
    return "unknown";
}

inline RevisionType revisionTypeFromString(const QString &str)
{
    if (str == "manual") return RevisionType::ManualEdit;
    if (str == "ai_generated") return RevisionType::AiGenerated;
    if (str == "ai_polished") return RevisionType::AiPolished;
    if (str == "import") return RevisionType::Import;
    if (str == "rollback") return RevisionType::Rollback;
    return RevisionType::ManualEdit;
}

inline QJsonObject Revision::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["content"] = content;
    obj["type"] = revisionTypeToString(type);
    obj["notes"] = notes;
    obj["aiModel"] = aiModel;
    obj["wordCount"] = wordCount;
    return obj;
}

inline Revision Revision::fromJson(const QJsonObject &json)
{
    Revision r;
    r.id = json["id"].toString();
    r.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    r.content = json["content"].toString();
    r.type = revisionTypeFromString(json["type"].toString());
    r.notes = json["notes"].toString();
    r.aiModel = json["aiModel"].toString();
    r.wordCount = json["wordCount"].toInt();
    return r;
}

export namespace PlotEngine::Revisions {

QVector<Revision> createRevision(const QString &episodeId, const QString &content,
                                  RevisionType type, const QString &notes = QString(),
                                  const QString &aiModel = QString())
{
    Q_UNUSED(episodeId);

    QVector<Revision> revisions;
    Revision rev;
    rev.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rev.timestamp = QDateTime::currentDateTime();
    rev.content = content;
    rev.type = type;
    rev.notes = notes;
    rev.aiModel = aiModel;
    rev.wordCount = content.size();
    revisions.append(rev);
    return revisions;
}

std::optional<Revision> findRevision(const QVector<Revision> &revisions, const QString &revisionId)
{
    for (const auto &rev : revisions) {
        if (rev.id == revisionId)
            return rev;
    }
    return std::nullopt;
}

QVector<Revision> filterByType(const QVector<Revision> &revisions, RevisionType type)
{
    QVector<Revision> filtered;
    for (const auto &rev : revisions) {
        if (rev.type == type)
            filtered.append(rev);
    }
    return filtered;
}

QVector<Revision> pruneRevisions(QVector<Revision> revisions, int maxCount = 50)
{
    if (revisions.size() <= maxCount)
        return revisions;

    std::sort(revisions.begin(), revisions.end(),
        [](const Revision &a, const Revision &b) {
            return a.timestamp > b.timestamp;
        });

    return revisions.mid(0, maxCount);
}

int computeDiffLineCount(const QString &oldContent, const QString &newContent)
{
    auto oldLines = oldContent.split('\n');
    auto newLines = newContent.split('\n');

    int added = 0;
    int removed = 0;

    auto maxLen = std::max(oldLines.size(), newLines.size());
    for (int i = 0; i < maxLen; ++i) {
        QString oldLine = (i < oldLines.size()) ? oldLines[i] : QString();
        QString newLine = (i < newLines.size()) ? newLines[i] : QString();
        if (oldLine != newLine) {
            if (!oldLine.isEmpty()) ++removed;
            if (!newLine.isEmpty()) ++added;
        }
    }
    return added + removed;
}

}
