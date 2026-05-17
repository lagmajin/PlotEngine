module;

#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <optional>

export module PlotEngine.Core.ProjectManager;
import PlotEngine.Core.NovelProject;

export class ProjectManager {
public:
    static bool save(const NovelProject &project, const QString &path);
    static std::optional<NovelProject> load(const QString &path);
    static NovelProject createNew(const QString &name, const QString &author);

    static QString defaultProjectExtension();
};

inline bool ProjectManager::save(const NovelProject &project, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(project.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

inline std::optional<NovelProject> ProjectManager::load(const QString &path)
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

inline NovelProject ProjectManager::createNew(const QString &name, const QString &author)
{
    auto makeGeneratedId = [](const char *prefix) {
        static quint64 counter = 0;
        return QStringLiteral("%1_%2_%3")
            .arg(QLatin1String(prefix))
            .arg(QDateTime::currentMSecsSinceEpoch())
            .arg(++counter);
    };

    NovelProject project;
    project.name = name;
    project.author = author;

    Chapter ch;
    ch.id = makeGeneratedId("chapter");
    ch.title = "第1章";
    ch.sortOrder = 0;

    Episode ep;
    ep.id = makeGeneratedId("episode");
    ep.title = "エピソード1";
    ep.createdAt = QDateTime::currentDateTime();
    ep.modifiedAt = QDateTime::currentDateTime();

    ch.episodes.append(ep);
    project.chapters.append(ch);

    return project;
}

inline QString ProjectManager::defaultProjectExtension()
{
    return QStringLiteral(".plotproj");
}
