#pragma once

#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

struct Scene {
    QString id;
    QString title;
    QString content;
    QDateTime createdAt;
    QDateTime modifiedAt;

    QJsonObject toJson() const;
    static Scene fromJson(const QJsonObject &json);
};

struct Chapter {
    QString id;
    QString title;
    QString summary;
    QVector<Scene> scenes;
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
