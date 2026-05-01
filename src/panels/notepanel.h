#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include "core/novelproject.h"

class NotePanel : public QWidget {
    Q_OBJECT
public:
    explicit NotePanel(QWidget *parent = nullptr);

    void loadProject(const NovelProject &project);

signals:
    void characterSelected(const QString &characterId);
    void locationSelected(const QString &locationId);

private slots:
    void onCharacterDoubleClick(QTreeWidgetItem *item, int column);
    void onLocationDoubleClick(QTreeWidgetItem *item, int column);

private:
    QTabWidget *m_tabs = nullptr;
    QTreeWidget *m_characterTree = nullptr;
    QTreeWidget *m_locationTree = nullptr;
    NovelProject m_project;
};