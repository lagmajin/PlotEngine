module;

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

export module PlotEngine.AI.DataAction;
import PlotEngine.Core.ContextBuilder;
import PlotEngine.Core.NovelProject;
import PlotEngine.AI.RevisionPrompt;

export namespace PlotEngine::AI {

enum class EditActionType {
    UpdateEpisodeContent,
    UpdateEpisodeMetadata,
    RenameChapter,
    RenameEpisode,
    UpdateCharacterNotes,
    UpdateLocationNotes,
};

struct EditAction {
    EditActionType type = EditActionType::UpdateEpisodeContent;
    QString chapterId;
    QString episodeId;
    QString characterId;
    QString locationId;
    QString title;
    QString summary;
    QString content;
    QString notes;
    QString povCharacter;
    QString timePeriod;
    QString sceneType;
    int emotionalIntensity = -1;
    int targetWordCount = -1;
};

struct EditPlan {
    QString rationale;
    QVector<EditAction> actions;
};

QString buildDirectEditPrompt(const NovelProject &project, const QString &chapterId,
                             const QString &episodeId, const QString &currentContent,
                             const QString &instructions,
                             const QVector<ProtectedSnippet> &protectedSnippets = {});

std::optional<EditPlan> parseEditPlan(const QString &responseText, QString *errorMessage = nullptr);

QStringList applyEditPlan(NovelProject &project, const EditPlan &plan);

}

namespace {
QString stripCodeFences(const QString &text)
{
    QString trimmed = text.trimmed();
    if (!trimmed.startsWith("```"))
        return trimmed;

    const int firstNewline = trimmed.indexOf('\n');
    if (firstNewline < 0)
        return trimmed;

    int start = firstNewline + 1;
    int end = trimmed.lastIndexOf("```");
    if (end <= start)
        return trimmed;

    return trimmed.mid(start, end - start).trimmed();
}

PlotEngine::AI::EditActionType parseActionType(const QString &type)
{
    const QString normalized = type.trimmed().toLower();
    if (normalized == "update_episode_metadata")
        return PlotEngine::AI::EditActionType::UpdateEpisodeMetadata;
    if (normalized == "rename_chapter")
        return PlotEngine::AI::EditActionType::RenameChapter;
    if (normalized == "rename_episode")
        return PlotEngine::AI::EditActionType::RenameEpisode;
    if (normalized == "update_character_notes")
        return PlotEngine::AI::EditActionType::UpdateCharacterNotes;
    if (normalized == "update_location_notes")
        return PlotEngine::AI::EditActionType::UpdateLocationNotes;
    return PlotEngine::AI::EditActionType::UpdateEpisodeContent;
}

QString actionTypeName(PlotEngine::AI::EditActionType type)
{
    switch (type) {
    case PlotEngine::AI::EditActionType::UpdateEpisodeContent: return QStringLiteral("update_episode_content");
    case PlotEngine::AI::EditActionType::UpdateEpisodeMetadata: return QStringLiteral("update_episode_metadata");
    case PlotEngine::AI::EditActionType::RenameChapter: return QStringLiteral("rename_chapter");
    case PlotEngine::AI::EditActionType::RenameEpisode: return QStringLiteral("rename_episode");
    case PlotEngine::AI::EditActionType::UpdateCharacterNotes: return QStringLiteral("update_character_notes");
    case PlotEngine::AI::EditActionType::UpdateLocationNotes: return QStringLiteral("update_location_notes");
    }
    return QStringLiteral("update_episode_content");
}

bool setStringField(const QJsonObject &obj, const char *key, QString *target)
{
    if (!obj.contains(key))
        return false;
    *target = obj.value(key).toString();
    return true;
}
}

namespace PlotEngine::AI {

QString buildDirectEditPrompt(const NovelProject &project, const QString &chapterId,
                             const QString &episodeId, const QString &currentContent,
                             const QString &instructions,
                             const QVector<ProtectedSnippet> &protectedSnippets)
{
    QStringList parts;
    parts.append(QStringLiteral(
        "あなたは小説エディタ内の編集エンジンです。"
        "出力は JSON のみ。説明文、markdown、コードフェンスは禁止です。\n"
        "目的は、指定された本文とメタデータを安全に更新することです。"
        "既存の事実を壊さず、保護対象は必ず守ってください。"
    ));
    parts.append(QStringLiteral(
        "返却形式:\n"
        "{\n"
        "  \"rationale\": \"短い説明\",\n"
        "  \"actions\": [\n"
        "    {\n"
        "      \"type\": \"update_episode_content | update_episode_metadata | rename_chapter | rename_episode | update_character_notes | update_location_notes\",\n"
        "      \"chapterId\": \"...\",\n"
        "      \"episodeId\": \"...\",\n"
        "      \"characterId\": \"...\",\n"
        "      \"locationId\": \"...\",\n"
        "      \"title\": \"...\",\n"
        "      \"summary\": \"...\",\n"
        "      \"content\": \"...\",\n"
        "      \"notes\": \"...\",\n"
        "      \"povCharacter\": \"...\",\n"
        "      \"timePeriod\": \"...\",\n"
        "      \"sceneType\": \"...\",\n"
        "      \"emotionalIntensity\": 1,\n"
        "      \"targetWordCount\": 1200\n"
        "    }\n"
        "  ]\n"
        "}\n"
        "必要なフィールドだけ入れてください。"
    ));

    QString episodeContext = PlotEngine::Context::buildEpisodeContext(project, episodeId);
    if (!episodeContext.isEmpty())
        parts.append(QStringLiteral("\n## エピソード文脈\n") + episodeContext);

    QString chapterContext = PlotEngine::Context::buildChapterContext(project, chapterId);
    if (!chapterContext.isEmpty())
        parts.append(QStringLiteral("\n## 章文脈\n") + chapterContext);

    if (!currentContent.isEmpty())
        parts.append(QStringLiteral("\n## 現在の本文\n") + currentContent);

    if (!instructions.isEmpty())
        parts.append(QStringLiteral("\n## 指示\n") + instructions);

    if (!protectedSnippets.isEmpty()) {
        QStringList protectedParts;
        protectedParts.append(QStringLiteral("## 保護対象"));
        for (const auto &snippet : protectedSnippets) {
            protectedParts.append(QStringLiteral("- ") + snippet.label + QStringLiteral(": ") + snippet.text);
        }
        parts.append(protectedParts.join("\n"));
    }

    return parts.join("\n");
}

std::optional<EditPlan> parseEditPlan(const QString &responseText, QString *errorMessage)
{
    const QString payload = stripCodeFences(responseText);
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("AI 応答が JSON ではありません");
        return std::nullopt;
    }

    QJsonObject root = doc.object();
    EditPlan plan;
    plan.rationale = root.value("rationale").toString();

    const QJsonArray actions = root.value("actions").toArray();
    for (const auto &value : actions) {
        if (!value.isObject())
            continue;

        const QJsonObject obj = value.toObject();
        EditAction action;
        action.type = parseActionType(obj.value("type").toString());
        setStringField(obj, "chapterId", &action.chapterId);
        setStringField(obj, "episodeId", &action.episodeId);
        setStringField(obj, "characterId", &action.characterId);
        setStringField(obj, "locationId", &action.locationId);
        setStringField(obj, "title", &action.title);
        setStringField(obj, "summary", &action.summary);
        setStringField(obj, "content", &action.content);
        setStringField(obj, "notes", &action.notes);
        setStringField(obj, "povCharacter", &action.povCharacter);
        setStringField(obj, "timePeriod", &action.timePeriod);
        setStringField(obj, "sceneType", &action.sceneType);
        if (obj.contains("emotionalIntensity"))
            action.emotionalIntensity = obj.value("emotionalIntensity").toInt(-1);
        if (obj.contains("targetWordCount"))
            action.targetWordCount = obj.value("targetWordCount").toInt(-1);
        plan.actions.append(action);
    }

    if (plan.actions.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("AI 応答に有効な actions がありません");
        return std::nullopt;
    }

    return plan;
}

QStringList applyEditPlan(NovelProject &project, const EditPlan &plan)
{
    QStringList summary;

    auto findChapter = [&project](const QString &chapterId) -> Chapter * {
        for (auto &ch : project.chapters) {
            if (ch.id == chapterId)
                return &ch;
        }
        return nullptr;
    };

    auto findEpisode = [&project, &findChapter](const QString &chapterId, const QString &episodeId) -> Episode * {
        Chapter *chapter = findChapter(chapterId);
        if (!chapter)
            return nullptr;
        for (auto &ep : chapter->episodes) {
            if (ep.id == episodeId)
                return &ep;
        }
        return nullptr;
    };

    for (const auto &action : plan.actions) {
        switch (action.type) {
        case EditActionType::UpdateEpisodeContent: {
            Episode *episode = findEpisode(action.chapterId, action.episodeId);
            if (!episode)
                break;
            episode->content = action.content;
            episode->modifiedAt = QDateTime::currentDateTime();
            summary.append(QStringLiteral("本文更新: %1").arg(episode->title));
            break;
        }
        case EditActionType::UpdateEpisodeMetadata: {
            Episode *episode = findEpisode(action.chapterId, action.episodeId);
            if (!episode)
                break;
            if (!action.summary.isEmpty())
                episode->summary = action.summary;
            if (!action.povCharacter.isEmpty())
                episode->povCharacter = action.povCharacter;
            if (!action.timePeriod.isEmpty())
                episode->timePeriod = action.timePeriod;
            if (!action.sceneType.isEmpty())
                episode->sceneType = action.sceneType;
            if (action.emotionalIntensity >= 0)
                episode->emotionalIntensity = action.emotionalIntensity;
            if (action.targetWordCount >= 0)
                episode->targetWordCount = action.targetWordCount;
            episode->modifiedAt = QDateTime::currentDateTime();
            summary.append(QStringLiteral("メタデータ更新: %1").arg(episode->title));
            break;
        }
        case EditActionType::RenameChapter: {
            Chapter *chapter = findChapter(action.chapterId);
            if (!chapter || action.title.trimmed().isEmpty())
                break;
            chapter->title = action.title.trimmed();
            summary.append(QStringLiteral("章名変更: %1").arg(chapter->title));
            break;
        }
        case EditActionType::RenameEpisode: {
            Episode *episode = findEpisode(action.chapterId, action.episodeId);
            if (!episode || action.title.trimmed().isEmpty())
                break;
            episode->title = action.title.trimmed();
            episode->modifiedAt = QDateTime::currentDateTime();
            summary.append(QStringLiteral("エピソード名変更: %1").arg(episode->title));
            break;
        }
        case EditActionType::UpdateCharacterNotes:
            for (auto &character : project.characters) {
                if (character.id != action.characterId)
                    continue;
                if (!action.notes.isEmpty())
                    character.notes = action.notes;
                summary.append(QStringLiteral("キャラノート更新: %1").arg(character.name));
                break;
            }
            break;
        case EditActionType::UpdateLocationNotes:
            for (auto &location : project.locations) {
                if (location.id != action.locationId)
                    continue;
                if (!action.notes.isEmpty())
                    location.notes = action.notes;
                summary.append(QStringLiteral("場所ノート更新: %1").arg(location.name));
                break;
            }
            break;
        }
    }

    return summary;
}

} // namespace PlotEngine::AI
