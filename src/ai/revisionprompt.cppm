module;

#include <QString>
#include <QStringList>
#include <QVector>

import std;

export module PlotEngine.AI.RevisionPrompt;

import PlotEngine.Core.ContextBuilder;
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

inline QString buildRevisionPrompt(const NovelProject &project, const QString &chapterId,
                                   const QString &episodeId, const QString &currentContent,
                                   const RevisionPromptRequest &request)
{
    QStringList parts;

    QString chapterContext = PlotEngine::Context::buildChapterContext(project, chapterId);
    if (!chapterContext.isEmpty())
        parts.append(chapterContext);

    QString episodeContext = PlotEngine::Context::buildEpisodeContext(project, episodeId);
    if (!episodeContext.isEmpty())
        parts.append(episodeContext);

    parts.append(QStringLiteral("## 現在の原稿\n```text\n") + currentContent + QStringLiteral("\n```"));

    if (!request.protectedSnippets.isEmpty()) {
        QStringList protectedParts;
        protectedParts.append(QStringLiteral("## 保護対象"));
        protectedParts.append(QStringLiteral("以下の内容は、意味・語順・語尾・固有名詞を含めて変更しないでください。"));

        for (int i = 0; i < request.protectedSnippets.size(); ++i) {
            const auto &snippet = request.protectedSnippets[i];
            protectedParts.append(QStringLiteral("### 保護ブロック %1: %2")
                                 .arg(i + 1)
                                 .arg(snippet.label.isEmpty() ? QStringLiteral("無題") : snippet.label));
            protectedParts.append(QStringLiteral("```text\n") + snippet.text + QStringLiteral("\n```"));
        }

        parts.append(protectedParts.join("\n"));
    }

    if (!request.desiredChanges.trimmed().isEmpty()) {
        parts.append(QStringLiteral("## 変更したい内容\n") + request.desiredChanges.trimmed());
    } else {
        parts.append(QStringLiteral(
            "## 変更したい内容\n"
            "読みやすさを上げつつ、物語の事実関係を崩さずに推敲してください。"
        ));
    }

    parts.append(QStringLiteral(
        "## 返答形式\n"
        "1. `修正版本文` として、全文を書き出してください。\n"
        "2. `変更点 / 差分要約` として、何をどう直したかを箇条書きでまとめてください。\n"
        "3. 保護対象に触れていないことを明示してください。\n"
        "4. 要望が衝突する場合は、どの制約を優先したかを簡潔に説明してください。\n"
    ));

    parts.append(QStringLiteral(
        "## 補足\n"
        "- 保護対象は、必要であっても言い換えや順序変更をしないでください。\n"
        "- 直したい箇所は、自然な文章になるように本文全体を再構成して構いません。\n"
        "- 出力は日本語でお願いします。"
    ));

    return parts.join("\n\n");
}

inline QString buildRevisionDiffSummary(const QString &before, const QString &after, int maxChanges = 20)
{
    if (before == after)
        return QStringLiteral("差分なし");

    auto clipLine = [](QString line) {
        line.replace('\r', ' ');
        line.replace('\n', ' ');
        line.replace('\t', ' ');
        return line.simplified().left(120);
    };

    const QStringList beforeLines = before.split('\n');
    const QStringList afterLines = after.split('\n');

    QStringList summary;
    const int maxLines = std::max(beforeLines.size(), afterLines.size());
    for (int i = 0; i < maxLines && summary.size() < maxChanges; ++i) {
        const QString beforeLine = (i < beforeLines.size()) ? beforeLines[i] : QString();
        const QString afterLine = (i < afterLines.size()) ? afterLines[i] : QString();

        if (beforeLine == afterLine)
            continue;

        if (i < beforeLines.size())
            summary.append(QStringLiteral("- L%1: %2").arg(i + 1).arg(clipLine(beforeLine)));
        if (summary.size() >= maxChanges)
            break;
        if (i < afterLines.size())
            summary.append(QStringLiteral("+ L%1: %2").arg(i + 1).arg(clipLine(afterLine)));
    }

    if (summary.isEmpty())
        return QStringLiteral("差分なし");

    return summary.join('\n');
}

}
