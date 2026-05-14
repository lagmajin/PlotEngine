#pragma once

#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

struct Episode {
    QString id;
    QString title;
    QString summary;
    QString content;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QString revisionNotes;

    QString povCharacter;
    QString timePeriod;
    int emotionalIntensity = 5;
    QString sceneType;
    int targetWordCount = 0;

    QJsonObject toJson() const;
    static Episode fromJson(const QJsonObject &json);
};

struct Chapter {
    QString id;
    QString title;
    QString summary;
    QVector<Episode> episodes;
    int sortOrder = 0;

    QJsonObject toJson() const;
    static Chapter fromJson(const QJsonObject &json);
};

struct CharacterEntry {
    QString id;
    QString name;
    QString role;
    QString description;
    QString notes;

    QJsonObject toJson() const;
    static CharacterEntry fromJson(const QJsonObject &json);
};

struct LocationEntry {
    QString id;
    QString name;
    QString description;
    QString notes;

    QJsonObject toJson() const;
    static LocationEntry fromJson(const QJsonObject &json);
};

class NovelProject {
public:
    NovelProject() = default;

    QString name;
    QString author;
    QString filePath;
    QVector<Chapter> chapters;
    QVector<CharacterEntry> characters;
    QVector<LocationEntry> locations;

    QJsonObject toJson() const;
    static NovelProject fromJson(const QJsonObject &json);

    int totalWordCount() const;
};
