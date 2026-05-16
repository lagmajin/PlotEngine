module;

#include <QString>
#include <QVector>

export module PlotEngine.AI.RevisionPrompt;
import PlotEngine.Core.NovelProject;

export namespace PlotEngine::AI {

struct ProtectedSnippet {
    QString label;
    QString text;
};

struct RevisionPromptRequest {
    QString desiredChanges;
    QVector<ProtectedSnippet> protectedSnippets;
};

QString buildRevisionPrompt(const NovelProject &project, const QString &chapterId,
                            const QString &episodeId, const QString &currentContent,
                            const RevisionPromptRequest &request);

QString buildRevisionDiffSummary(const QString &before, const QString &after, int maxChanges = 20);

}
