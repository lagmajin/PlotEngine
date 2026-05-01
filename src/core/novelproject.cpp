#include "novelproject.h"

QJsonObject Scene::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["content"] = content;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["modifiedAt"] = modifiedAt.toString(Qt::ISODate);
    return obj;
}

Scene Scene::fromJson(const QJsonObject &json)
{
    Scene s;
    s.id = json["id"].toString();
    s.title = json["title"].toString();
    s.content = json["content"].toString();
    s.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    s.modifiedAt = QDateTime::fromString(json["modifiedAt"].toString(), Qt::ISODate);
    return s;
}

QJsonObject Chapter::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["summary"] = summary;
    obj["sortOrder"] = sortOrder;
    QJsonArray arr;
    for (const auto &sc : scenes)
        arr.append(sc.toJson());
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
    const auto arr = json["scenes"].toArray();
    for (const auto &v : arr)
        ch.scenes.append(Scene::fromJson(v.toObject()));
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
        for (const auto &sc : ch.scenes) {
            count += sc.content.size();
        }
    }
    return count;
}
