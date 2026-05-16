#pragma once

#include <QStandardItemModel>
#include <QTreeView>
#include <QWidget>
#include "wobjectdefs.h"

import PlotEngine.UI.DockPane;
import PlotEngine.Core.NovelProject;

class QLineEdit;

class SearchPanel : public QWidget {
    W_OBJECT(SearchPanel)
public:
    explicit SearchPanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void loadProject(const NovelProject &project);
    void setQuery(const QString &query);

public:
    void openDocumentRequested(const QString &kind, const QString &id)
    W_SIGNAL(openDocumentRequested, (const QString &, const QString &), kind, id)
    void searchResultActivated(const QString &kind, const QString &id, const QString &query)
    W_SIGNAL(searchResultActivated, (const QString &, const QString &, const QString &), kind, id, query)

private:
    void appendReferenceResults(const QString &query);
    void rebuildResults();
    W_SLOT(rebuildResults)
    void onResultActivated(const QModelIndex &index);
    W_SLOT(onResultActivated, (const QModelIndex &))

private:
    NovelProject m_project;
    QLineEdit *m_queryEdit = nullptr;
    QTreeView *m_resultsView = nullptr;
    QStandardItemModel *m_resultsModel = nullptr;
};
