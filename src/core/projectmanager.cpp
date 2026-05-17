#include "projectmanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QUuid>
#include <QDateTime>

bool ProjectManager::save(const NovelProject &project, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(project.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

std::optional<NovelProject> ProjectManager::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject())
        return std::nullopt;

    NovelProject project = NovelProject::fromJson(doc.object());
    project.filePath = path;
    return project;
}

NovelProject ProjectManager::createNew(const QString &name, const QString &author)
{
    NovelProject project;
    project.name = name;
    project.author = author;

    Chapter ch;
    ch.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ch.title = "第1章";
    ch.sortOrder = 0;

    Episode ep;
    ep.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ep.title = "エピソード1";
    ep.createdAt = QDateTime::currentDateTime();
    ep.modifiedAt = QDateTime::currentDateTime();

    ch.episodes.append(ep);
    project.chapters.append(ch);

    return project;
}

QString ProjectManager::defaultProjectExtension()
{
    return QStringLiteral(".plotproj");
}
