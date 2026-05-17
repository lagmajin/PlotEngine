#pragma once

#include <QString>
#include <QVector>

struct PromptSnippetData {
    QString label;
    QString text;
};

QString buildRevisionPromptTextHelper(const QString &chapterContext,
                                      const QString &episodeContext,
                                      const QString &currentContent,
                                      const QString &desiredChanges,
                                      const QVector<PromptSnippetData> &protectedSnippets);

QString buildRevisionDiffSummaryHelper(const QString &before, const QString &after, int maxChanges);
