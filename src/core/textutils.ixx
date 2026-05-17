module;

#include <QString>
#include <QStringList>

export module PlotEngine.Core.TextUtils;

export namespace PlotEngine::Text {

QString normalizeEpisodeText(const QString &text)
{
    QString normalized = text;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');

    QStringList lines = normalized.split('\n');
    for (QString &line : lines)
        line = line.trimmed();

    QString result;
    int blankCount = 0;
    for (const QString &line : lines) {
        if (line.isEmpty()) {
            ++blankCount;
            if (blankCount > 2)
                continue;
        } else {
            blankCount = 0;
        }
        if (!result.isEmpty())
            result += '\n';
        result += line;
    }
    return result.trimmed();
}

QString buildPolishPrompt(const QString &chapterTitle, const QString &episodeTitle,
                          const QString &content, const QString &instruction)
{
    return QStringLiteral(
        "次の小説原稿を、指定された意図に沿って推敲してください。\n"
        "章: %1\n"
        "エピソード: %2\n"
        "要望: %3\n"
        "\n"
        "制約:\n"
        "- 物語の事実関係は維持する\n"
        "- 文体は崩さず、読みやすさを上げる\n"
        "- 変更理由があれば簡潔にまとめる\n"
        "\n"
        "原稿:\n%4")
        .arg(chapterTitle, episodeTitle, instruction, content);
}

}
