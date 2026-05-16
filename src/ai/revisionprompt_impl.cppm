module;

#include "ai/revisionprompt_helpers.h"

module PlotEngine.AI.RevisionPrompt;
import PlotEngine.Core.ContextBuilder;

namespace PlotEngine::AI {

QString buildRevisionPrompt(const NovelProject &project, const QString &chapterId,
                            const QString &episodeId, const QString &currentContent,
                            const RevisionPromptRequest &request)
{
    const QString chapterContext = PlotEngine::Context::buildChapterContext(project, chapterId);
    const QString episodeContext = PlotEngine::Context::buildEpisodeContext(project, episodeId);

    QVector<PromptSnippetData> protectedSnippets;
    protectedSnippets.reserve(request.protectedSnippets.size());
    for (int i = 0; i < request.protectedSnippets.size(); ++i) {
        const ProtectedSnippet &snippet = request.protectedSnippets.at(i);
        protectedSnippets.append({snippet.label, snippet.text});
    }

    return buildRevisionPromptTextHelper(
        chapterContext,
        episodeContext,
        currentContent,
        request.desiredChanges,
        protectedSnippets);
}

}
