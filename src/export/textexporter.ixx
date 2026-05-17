module;

#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

export module PlotEngine.Core.TextExporter;
import PlotEngine.Core.NovelProject;

export namespace PlotEngine::Export {

QString buildPlainText(const NovelProject &project, bool includeSummary = true,
                        bool includeMetadata = false)
{
    QStringList parts;

    parts.append(project.name);
    parts.append(QStringLiteral("著者: ") + project.author);
    parts.append(QStringLiteral("最終更新: ") + QDateTime::currentDateTime().toString(Qt::ISODate));
    parts.append("");
    parts.append(QString(40, '='));
    parts.append("");

    for (const auto &ch : project.chapters) {
        parts.append(QStringLiteral("第") + ch.title);
        parts.append("");

        if (includeSummary && !ch.summary.isEmpty()) {
            parts.append(QStringLiteral("[章の要約]"));
            parts.append(ch.summary);
            parts.append("");
        }

        for (const auto &ep : ch.episodes) {
            parts.append(QStringLiteral("  ") + ep.title);

            if (includeMetadata) {
                if (!ep.povCharacter.isEmpty())
                    parts.append(QStringLiteral("    POV: ") + ep.povCharacter);
                if (!ep.timePeriod.isEmpty())
                    parts.append(QStringLiteral("    時間帯: ") + ep.timePeriod);
                if (!ep.sceneType.isEmpty())
                    parts.append(QStringLiteral("    シーン種別: ") + ep.sceneType);
                if (ep.emotionalIntensity != 5)
                    parts.append(QStringLiteral("    感情強度: ") + QString::number(ep.emotionalIntensity) + "/10");
                if (!ep.summary.isEmpty())
                    parts.append(QStringLiteral("    要約: ") + ep.summary);
            }

            parts.append("");

            if (!ep.content.isEmpty()) {
                QStringList contentLines = ep.content.split('\n');
                for (const auto &line : contentLines) {
                    parts.append(QStringLiteral("    ") + line);
                }
            }

            parts.append("");
            parts.append(QString(20, '-'));
            parts.append("");
        }
    }

    if (!project.characters.isEmpty()) {
        parts.append(QString(40, '='));
        parts.append("");
        parts.append("登場キャラクター");
        parts.append("");

        for (const auto &c : project.characters) {
            parts.append(QStringLiteral("- ") + c.name);
            if (!c.role.isEmpty())
                parts.append(QStringLiteral("  役割: ") + c.role);
            if (!c.description.isEmpty())
                parts.append(QStringLiteral("  説明: ") + c.description);
            parts.append("");
        }
    }

    if (!project.locations.isEmpty()) {
        parts.append(QString(40, '='));
        parts.append("");
        parts.append("舞台・場所");
        parts.append("");

        for (const auto &l : project.locations) {
            parts.append(QStringLiteral("- ") + l.name);
            if (!l.description.isEmpty())
                parts.append(QStringLiteral("  説明: ") + l.description);
            parts.append("");
        }
    }

    return parts.join('\n');
}

bool exportToTextFile(const NovelProject &project, const QString &filePath,
                       bool includeSummary = true, bool includeMetadata = false)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << buildPlainText(project, includeSummary, includeMetadata);
    file.close();
    return true;
}

QString buildEpisodeText(const NovelProject &project, const QString &episodeId)
{
    for (const auto &ch : project.chapters) {
        for (const auto &ep : ch.episodes) {
            if (ep.id != episodeId)
                continue;

            QStringList parts;
            parts.append(ch.title + " / " + ep.title);
            parts.append("");
            parts.append(ep.content);
            return parts.join('\n');
        }
    }
    return QString();
}

}
