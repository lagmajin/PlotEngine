#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>
#include <QWidget>
#include "wobjectdefs.h"

import PlotEngine.UI.DockPane;
import PlotEngine.Core.NovelProject;

class QListWidget;
class QListWidgetItem;
class QTabWidget;

class SceneBoardPanel : public QWidget {
    W_OBJECT(SceneBoardPanel)
public:
    struct Card {
        QString chapterId;
        QString episodeId;
        QString title;
        QString summary;
        QString detail;
        bool dirty = false;
    };

    explicit SceneBoardPanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void loadProject(const NovelProject &project);
    void setCurrentEpisode(const QString &chapterId, const QString &episodeId);

public:
    void episodeSelected(const QString &chapterId, const QString &episodeId)
    W_SIGNAL(episodeSelected, (const QString &, const QString &), chapterId, episodeId)
    void episodeOrderChanged(const QString &chapterId, const QStringList &orderedEpisodeIds)
    W_SIGNAL(episodeOrderChanged, (const QString &, const QStringList &), chapterId, orderedEpisodeIds)

private:
    void onCardActivated(QListWidgetItem *item);
    W_SLOT(onCardActivated, (QListWidgetItem *))
    void onOrderChanged(const QString &chapterId);
    W_SLOT(onOrderChanged, (const QString &))

private:
    QListWidget *createChapterBoard(const QString &chapterId, const QString &chapterTitle);
    void refreshTabLabels();
    void selectItem(QListWidget *list, const QString &episodeId);

    QTabWidget *m_tabs = nullptr;
    NovelProject m_project;
    QHash<QString, QListWidget*> m_listsByChapter;
    bool m_loading = false;
};
