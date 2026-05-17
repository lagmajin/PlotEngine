#pragma once

#include <QString>
#include <optional>

class NovelProject;

class ProjectManager {
public:
    static bool save(const NovelProject &project, const QString &path);
    static std::optional<NovelProject> load(const QString &path);
    static NovelProject createNew(const QString &name, const QString &author);

    static QString defaultProjectExtension();
};
