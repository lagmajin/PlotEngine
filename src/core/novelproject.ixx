module;

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

export module PlotEngine.Core.NovelProject;

export struct Episode {
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

export struct Chapter {
    QString id;
    QString title;
    QString summary;
    QVector<Episode> episodes;
    int sortOrder = 0;

    QJsonObject toJson() const;
    static Chapter fromJson(const QJsonObject &json);
};

export struct CharacterEntry {
    QString id;
    QString name;
    QString role;
    QString description;
    QString notes;

    QJsonObject toJson() const;
    static CharacterEntry fromJson(const QJsonObject &json);
};

export struct LocationEntry {
    QString id;
    QString name;
    QString description;
    QString notes;

    QJsonObject toJson() const;
    static LocationEntry fromJson(const QJsonObject &json);
};

export class NovelProject {
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

inline QJsonObject Episode::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["summary"] = summary;
    obj["content"] = content;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["modifiedAt"] = modifiedAt.toString(Qt::ISODate);
    obj["revisionNotes"] = revisionNotes;
    obj["povCharacter"] = povCharacter;
    obj["timePeriod"] = timePeriod;
    obj["emotionalIntensity"] = emotionalIntensity;
    obj["sceneType"] = sceneType;
    obj["targetWordCount"] = targetWordCount;
    return obj;
}

inline Episode Episode::fromJson(const QJsonObject &json)
{
    Episode e;
    e.id = json["id"].toString();
    e.title = json["title"].toString();
    e.summary = json["summary"].toString();
    e.content = json["content"].toString();
    e.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    e.modifiedAt = QDateTime::fromString(json["modifiedAt"].toString(), Qt::ISODate);
    e.revisionNotes = json["revisionNotes"].toString();
    e.povCharacter = json["povCharacter"].toString();
    e.timePeriod = json["timePeriod"].toString();
    e.emotionalIntensity = json["emotionalIntensity"].toInt(5);
    e.sceneType = json["sceneType"].toString();
    e.targetWordCount = json["targetWordCount"].toInt(0);
    return e;
}

inline QJsonObject Chapter::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["summary"] = summary;
    obj["sortOrder"] = sortOrder;
    QJsonArray arr;
    for (const auto &ep : episodes)
        arr.append(ep.toJson());
    obj["episodes"] = arr;
    obj["scenes"] = arr;
    return obj;
}

inline Chapter Chapter::fromJson(const QJsonObject &json)
{
    Chapter ch;
    ch.id = json["id"].toString();
    ch.title = json["title"].toString();
    ch.summary = json["summary"].toString();
    ch.sortOrder = json["sortOrder"].toInt();
    const auto arr = json.contains("episodes") ? json["episodes"].toArray() : json["scenes"].toArray();
    for (const auto &v : arr)
        ch.episodes.append(Episode::fromJson(v.toObject()));
    return ch;
}

inline QJsonObject CharacterEntry::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["role"] = role;
    obj["description"] = description;
    obj["notes"] = notes;
    return obj;
}

inline CharacterEntry CharacterEntry::fromJson(const QJsonObject &json)
{
    CharacterEntry c;
    c.id = json["id"].toString();
    c.name = json["name"].toString();
    c.role = json["role"].toString();
    c.description = json["description"].toString();
    c.notes = json["notes"].toString();
    return c;
}

inline QJsonObject LocationEntry::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["notes"] = notes;
    return obj;
}

inline LocationEntry LocationEntry::fromJson(const QJsonObject &json)
{
    LocationEntry l;
    l.id = json["id"].toString();
    l.name = json["name"].toString();
    l.description = json["description"].toString();
    l.notes = json["notes"].toString();
    return l;
}

inline QJsonObject NovelProject::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["author"] = author;
    QJsonArray chArr;
    for (const auto &ch : chapters)
        chArr.append(ch.toJson());
    obj["chapters"] = chArr;
    QJsonArray charArr;
    for (const auto &c : characters)
        charArr.append(c.toJson());
    obj["characters"] = charArr;
    QJsonArray locArr;
    for (const auto &l : locations)
        locArr.append(l.toJson());
    obj["locations"] = locArr;
    return obj;
}

inline NovelProject NovelProject::fromJson(const QJsonObject &json)
{
    NovelProject p;
    p.name = json["name"].toString();
    p.author = json["author"].toString();
    for (const auto &v : json["chapters"].toArray())
        p.chapters.append(Chapter::fromJson(v.toObject()));
    for (const auto &v : json["characters"].toArray())
        p.characters.append(CharacterEntry::fromJson(v.toObject()));
    for (const auto &v : json["locations"].toArray())
        p.locations.append(LocationEntry::fromJson(v.toObject()));
    return p;
}

inline int NovelProject::totalWordCount() const
{
    int count = 0;
    for (const auto &ch : chapters) {
        for (const auto &ep : ch.episodes) {
            count += ep.content.size();
        }
    }
    return count;
}
