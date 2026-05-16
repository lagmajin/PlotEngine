module;

#include "ai/revisionprompt_helpers.h"

module PlotEngine.AI.RevisionPrompt;

namespace PlotEngine::AI {

QString buildRevisionDiffSummary(const QString &before, const QString &after, int maxChanges)
{
    return buildRevisionDiffSummaryHelper(before, after, maxChanges);
}

}
