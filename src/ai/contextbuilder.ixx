module;

#include <QString>
#include <QStringList>
#include <QVector>

export module PlotEngine.Core.ContextBuilder;
import PlotEngine.Core.NovelProject;

export namespace PlotEngine::Context {

QString buildCharacterContext(const NovelProject &project)
{
    if (project.characters.isEmpty())
        return QString();

    QStringList parts;
    parts.append(QStringLiteral("## 登場キャラクター"));

    for (const auto &c : project.characters) {
        QStringList lines;
        lines.append(QStringLiteral("- 名前: ") + c.name);
        if (!c.role.isEmpty())
            lines.append(QStringLiteral("  役割: ") + c.role);
        if (!c.description.isEmpty())
            lines.append(QStringLiteral("  説明: ") + c.description);
        if (!c.notes.isEmpty())
            lines.append(QStringLiteral("  ノート: ") + c.notes);
        parts.append(lines.join('\n'));
    }

    return parts.join("\n\n");
}

QString buildLocationContext(const NovelProject &project)
{
    if (project.locations.isEmpty())
        return QString();

    QStringList parts;
    parts.append(QStringLiteral("## 舞台・場所"));

    for (const auto &l : project.locations) {
        QStringList lines;
        lines.append(QStringLiteral("- 場所: ") + l.name);
        if (!l.description.isEmpty())
            lines.append(QStringLiteral("  説明: ") + l.description);
        if (!l.notes.isEmpty())
            lines.append(QStringLiteral("  ノート: ") + l.notes);
        parts.append(lines.join('\n'));
    }

    return parts.join("\n\n");
}

QString buildChapterContext(const NovelProject &project, const QString &chapterId)
{
    for (const auto &ch : project.chapters) {
        if (ch.id != chapterId)
            continue;

        QStringList parts;
        parts.append(QStringLiteral("## 現在の章: ") + ch.title);
        if (!ch.summary.isEmpty())
            parts.append(QStringLiteral("章の要約: ") + ch.summary);

        parts.append(QStringLiteral("\n### エピソード一覧"));
        for (int epIdx = 0; epIdx < ch.episodes.size(); ++epIdx) {
            const auto &ep = ch.episodes[epIdx];
            QStringList epLines;
            epLines.append(QStringLiteral("- ") + ep.title);
            if (!ep.summary.isEmpty())
                epLines.append(QStringLiteral("  要約: ") + ep.summary);
            if (!ep.povCharacter.isEmpty())
                epLines.append(QStringLiteral("  POV: ") + ep.povCharacter);
            if (!ep.timePeriod.isEmpty())
                epLines.append(QStringLiteral("  時間帯: ") + ep.timePeriod);
            if (ep.emotionalIntensity != 5)
                epLines.append(QStringLiteral("  感情強度: ") + QString::number(ep.emotionalIntensity) + "/10");
            if (!ep.sceneType.isEmpty())
                epLines.append(QStringLiteral("  シーン種別: ") + ep.sceneType);
            parts.append(epLines.join('\n'));
        }

        return parts.join("\n");
    }

    return QString();
}

QString buildEpisodeContext(const NovelProject &project, const QString &episodeId)
{
    for (const auto &ch : project.chapters) {
        for (int epIdx = 0; epIdx < ch.episodes.size(); ++epIdx) {
            const auto &ep = ch.episodes[epIdx];
            if (ep.id != episodeId)
                continue;

            QStringList parts;
            parts.append(QStringLiteral("## 現在のエピソード: ") + ep.title);
            parts.append(QStringLiteral("章: ") + ch.title);
            if (!ep.summary.isEmpty())
                parts.append(QStringLiteral("要約: ") + ep.summary);
            if (!ep.povCharacter.isEmpty())
                parts.append(QStringLiteral("POV: ") + ep.povCharacter);
            if (!ep.timePeriod.isEmpty())
                parts.append(QStringLiteral("時間帯: ") + ep.timePeriod);
            if (ep.emotionalIntensity != 5)
                parts.append(QStringLiteral("感情強度: ") + QString::number(ep.emotionalIntensity) + "/10");
            if (!ep.sceneType.isEmpty())
                parts.append(QStringLiteral("シーン種別: ") + ep.sceneType);
            if (ep.targetWordCount > 0)
                parts.append(QStringLiteral("目標文字数: ") + QString::number(ep.targetWordCount));

            if (epIdx > 0) {
                const auto &prev = ch.episodes[epIdx - 1];
                parts.append(QStringLiteral("\n### 前のエピソード: ") + prev.title);
                if (!prev.summary.isEmpty())
                    parts.append(QStringLiteral("要約: ") + prev.summary);
                else if (!prev.content.isEmpty())
                    parts.append(QStringLiteral("内容（抜粋）: ") + prev.content.left(200) + "...");
            }

            if (epIdx < ch.episodes.size() - 1) {
                const auto &next = ch.episodes[epIdx + 1];
                parts.append(QStringLiteral("\n### 次のエピソード: ") + next.title);
                if (!next.summary.isEmpty())
                    parts.append(QStringLiteral("要約: ") + next.summary);
            }

            return parts.join("\n");
        }
    }

    return QString();
}

QString buildSystemPrompt(const NovelProject &project, const QString &customStyleGuide = QString())
{
    QStringList parts;

    parts.append(QStringLiteral(
        "あなたは小説執筆を支援するAIアシスタントです。\n"
        "以下の制約を守って、高品質な小説原稿を生成・推敲してください。\n\n"
        "制約:\n"
        "- 日本語で出力する\n"
        "- 物語の事実関係（キャラクター設定、世界観、時系列）を維持する\n"
        "- 文体は一貫性を保つ\n"
        "- 会話文は「」で、内心は『』で囲む\n"
        "- 地の文と会話文のバランスを保つ\n"
    ));

    if (!customStyleGuide.isEmpty()) {
        parts.append(QStringLiteral("\n## スタイルガイド\n") + customStyleGuide);
    }

    QString charCtx = buildCharacterContext(project);
    if (!charCtx.isEmpty())
        parts.append("\n" + charCtx);

    QString locCtx = buildLocationContext(project);
    if (!locCtx.isEmpty())
        parts.append("\n" + locCtx);

    return parts.join("\n");
}

QString buildGenerationPrompt(const NovelProject &project, const QString &chapterId,
                               const QString &episodeId, const QString &instruction = QString())
{
    QStringList parts;

    parts.append(buildEpisodeContext(project, episodeId));

    if (!instruction.isEmpty()) {
        parts.append(QStringLiteral("\n## 生成指示\n") + instruction);
    } else {
        parts.append(QStringLiteral(
            "\n## 生成指示\n"
            "上記のメタデータと文脈に基づいて、エピソードの本文を生成してください。\n"
            "キャラクターの設定と世界観を尊重し、自然な展開で作成してください。"
        ));
    }

    return parts.join("\n");
}

QString buildPolishPrompt(const NovelProject &project, const QString &episodeId,
                           const QString &currentContent, const QString &instruction = QString())
{
    QStringList parts;

    parts.append(buildEpisodeContext(project, episodeId));

    parts.append(QStringLiteral("\n## 現在の原稿\n") + currentContent);

    if (!instruction.isEmpty()) {
        parts.append(QStringLiteral("\n## 推敲指示\n") + instruction);
    } else {
        parts.append(QStringLiteral(
            "\n## 推敲指示\n"
            "文体を保ちながら、冗長な表現を削って読みやすくしてください。\n"
            "物語の事実関係は変更しないでください。"
        ));
    }

    return parts.join("\n");
}

}
