#include "ai/revisionprompt_helpers.h"

#include <QStringList>

QString buildRevisionPromptTextHelper(const QString &chapterContext,
                                      const QString &episodeContext,
                                      const QString &currentContent,
                                      const QString &desiredChanges,
                                      const QVector<PromptSnippetData> &protectedSnippets)
{
    QString prompt;

    if (!chapterContext.isEmpty())
        prompt += chapterContext + QStringLiteral("\n\n");

    if (!episodeContext.isEmpty())
        prompt += episodeContext + QStringLiteral("\n\n");

    prompt += QStringLiteral("## 現在の原稿\n```text\n");
    prompt += currentContent;
    prompt += QStringLiteral("\n```\n\n");

    if (!protectedSnippets.isEmpty()) {
        prompt += QStringLiteral("## 保護対象\n");
        prompt += QStringLiteral("以下の内容は、意味・語順・語尾・固有名詞を含めて変更しないでください。\n");

        for (int i = 0; i < protectedSnippets.size(); ++i) {
            const PromptSnippetData &snippet = protectedSnippets.at(i);
            const QString label = snippet.label.isEmpty() ? QStringLiteral("無題") : snippet.label;
            prompt += QStringLiteral("### 保護ブロック %1: %2\n").arg(i + 1).arg(label);
            prompt += QStringLiteral("```text\n");
            prompt += snippet.text;
            prompt += QStringLiteral("\n```\n");
        }
        prompt += QStringLiteral("\n");
    }

    if (!desiredChanges.trimmed().isEmpty()) {
        prompt += QStringLiteral("## 変更したい内容\n");
        prompt += desiredChanges.trimmed();
        prompt += QStringLiteral("\n\n");
    } else {
        prompt += QStringLiteral(
            "## 変更したい内容\n"
            "読みやすさを上げつつ、物語の事実関係を崩さずに推敲してください。\n\n");
    }

    prompt += QStringLiteral(
        "## 返答形式\n"
        "1. `修正版本文` として、全文を書き出してください。\n"
        "2. `変更点 / 差分要約` として、何をどう直したかを箇条書きでまとめてください。\n"
        "3. 保護対象に触れていないことを明示してください。\n"
        "4. 要望が衝突する場合は、どの制約を優先したかを簡潔に説明してください。\n\n");

    prompt += QStringLiteral(
        "## 補足\n"
        "- 保護対象は、必要であっても言い換えや順序変更をしないでください。\n"
        "- 直したい箇所は、自然な文章になるように本文全体を再構成して構いません。\n"
        "- 出力は日本語でお願いします。");

    return prompt;
}

namespace {

QString clipDiffLine(QString line)
{
    line.replace('\r', ' ');
    line.replace('\n', ' ');
    line.replace('\t', ' ');
    return line.simplified().left(120);
}

}

QString buildRevisionDiffSummaryHelper(const QString &before, const QString &after, int maxChanges)
{
    if (before == after)
        return QStringLiteral("差分なし");

    const QStringList beforeLines = before.split('\n');
    const QStringList afterLines = after.split('\n');
    const int maxLines = beforeLines.size() > afterLines.size() ? beforeLines.size() : afterLines.size();

    QStringList summary;
    for (int i = 0; i < maxLines && summary.size() < maxChanges; ++i) {
        const QString beforeLine = i < beforeLines.size() ? beforeLines.at(i) : QString();
        const QString afterLine = i < afterLines.size() ? afterLines.at(i) : QString();

        if (beforeLine == afterLine)
            continue;

        if (i < beforeLines.size()) {
            summary.append(QStringLiteral("- L%1: %2").arg(i + 1).arg(clipDiffLine(beforeLine)));
            if (summary.size() >= maxChanges)
                break;
        }

        if (i < afterLines.size())
            summary.append(QStringLiteral("+ L%1: %2").arg(i + 1).arg(clipDiffLine(afterLine)));
    }

    if (summary.isEmpty())
        return QStringLiteral("差分なし");
    return summary.join('\n');
}
