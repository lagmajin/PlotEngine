#include "novelproject.h"

QJsonObject Episode::toJson() const
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

Episode Episode::fromJson(const QJsonObject &json)
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

QJsonObject Chapter::toJson() const
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

Chapter Chapter::fromJson(const QJsonObject &json)
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

QJsonObject CharacterEntry::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["role"] = role;
    obj["description"] = description;
    obj["notes"] = notes;
    return obj;
}

CharacterEntry CharacterEntry::fromJson(const QJsonObject &json)
{
    CharacterEntry c;
    c.id = json["id"].toString();
    c.name = json["name"].toString();
    c.role = json["role"].toString();
    c.description = json["description"].toString();
    c.notes = json["notes"].toString();
    return c;
}

QJsonObject LocationEntry::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["description"] = description;
    obj["notes"] = notes;
    return obj;
}

LocationEntry LocationEntry::fromJson(const QJsonObject &json)
{
    LocationEntry l;
    l.id = json["id"].toString();
    l.name = json["name"].toString();
    l.description = json["description"].toString();
    l.notes = json["notes"].toString();
    return l;
}

QJsonObject NovelProject::toJson() const
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

NovelProject NovelProject::fromJson(const QJsonObject &json)
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

int NovelProject::totalWordCount() const
{
    int count = 0;
    for (const auto &ch : chapters) {
        for (const auto &ep : ch.episodes) {
            count += ep.content.size();
        }
    }
    return count;
}
