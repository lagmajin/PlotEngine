#pragma once

#include <QString>
#include <QStringList>

inline QString buildRevisionPromptText(const QString &chapterContext,
                                       const QString &episodeContext,
                                       const QString &currentContent,
                                       const QString &desiredChanges,
                                       const QStringList &protectedBlocks)
{
    QString prompt;

    if (!chapterContext.isEmpty())
        prompt += chapterContext + QStringLiteral("\n\n");

    if (!episodeContext.isEmpty())
        prompt += episodeContext + QStringLiteral("\n\n");

    prompt += QStringLiteral("## 現在の原稿\n```text\n");
    prompt += currentContent;
    prompt += QStringLiteral("\n```\n\n");

    if (!protectedBlocks.isEmpty()) {
        prompt += QStringLiteral("## 保護対象\n");
        prompt += QStringLiteral("以下の内容は、意味・語順・語尾・固有名詞を含めて変更しないでください。\n");
        prompt += protectedBlocks.join(QStringLiteral("\n"));
        prompt += QStringLiteral("\n\n");
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
