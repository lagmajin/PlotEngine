#pragma once

#include <QWidget>
#include <QString>
#include <QVector>
#include "wobjectdefs.h"

#include "ui/dockpane.h"

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class TextCompareView;

class RevisionHistoryPanel : public QWidget {
    W_OBJECT(RevisionHistoryPanel)
public:
    struct Entry {
        QString id;
        QString title;
        QString detail;
        QString content;
        QString diffSummary;
        QString compareLeftTitle;
        QString compareLeftContent;
        QString compareRightTitle;
        QString compareRightContent;
        bool current = false;
    };

    explicit RevisionHistoryPanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void setEmptyState(const QString &message);
    void setEntries(const QVector<Entry> &entries);
    QString selectedRevisionId() const;

public:
    void restoreRequested(const QString &revisionId)
    W_SIGNAL(restoreRequested, (const QString &), revisionId)

private:
    void onSelectionChanged();
    W_SLOT(onSelectionChanged)
    void onRestoreClicked();
    W_SLOT(onRestoreClicked)

private:
    void refreshDetail();

    QLabel *m_titleLabel = nullptr;
    QTreeWidget *m_tree = nullptr;
    QTabWidget *m_contentTabs = nullptr;
    QPlainTextEdit *m_detail = nullptr;
    QPlainTextEdit *m_diff = nullptr;
    TextCompareView *m_compareView = nullptr;
    QPushButton *m_restoreButton = nullptr;
    QVector<Entry> m_entries;
};
