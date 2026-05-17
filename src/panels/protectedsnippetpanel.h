#pragma once

#include <QString>
#include <QVector>
#include <QWidget>
#include "wobjectdefs.h"

#include "ui/dockpane.h"

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QPlainTextEdit;

class ProtectedSnippetPanel : public QWidget {
    W_OBJECT(ProtectedSnippetPanel)
public:
    struct Snippet {
        QString label;
        QString text;
    };

    explicit ProtectedSnippetPanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void setDocumentTitle(const QString &title);
    void setSnippets(const QVector<Snippet> &snippets);
    void clearPanel();

public:
    void addCurrentSelectionRequested()
    W_SIGNAL(addCurrentSelectionRequested)
    void removeSnippetRequested(int index)
    W_SIGNAL(removeSnippetRequested, (int), index)
    void clearSnippetsRequested()
    W_SIGNAL(clearSnippetsRequested)

private:
    void onSelectionChanged();
    W_SLOT(onSelectionChanged)
    void onAddClicked();
    W_SLOT(onAddClicked)
    void onRemoveClicked();
    W_SLOT(onRemoveClicked)
    void onClearClicked();
    W_SLOT(onClearClicked)

private:
    void refreshDetail();

    QLabel *m_titleLabel = nullptr;
    QListWidget *m_list = nullptr;
    QPlainTextEdit *m_detail = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QVector<Snippet> m_snippets;
};
