#pragma once

#include <QWidget>
#include <QString>
#include <QVector>
#include "wobjectdefs.h"

#include "ui/dockpane.h"

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QListWidgetItem;
class TextCompareView;

class ReviewPanel : public QWidget {
    W_OBJECT(ReviewPanel)
public:
    struct ActionItem {
        QString title;
        QString detail;
        QString diffSummary;
        QString compareLeftTitle;
        QString compareLeftContent;
        QString compareRightTitle;
        QString compareRightContent;
        bool checked = true;
    };

    explicit ReviewPanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void clearReview();
    void setReviewTitle(const QString &title);
    void setRationale(const QString &rationale);
    void setActions(const QVector<ActionItem> &actions);
    void setDiffSummary(const QString &summary);
    void setRawResponse(const QString &response);
    QVector<int> selectedActionIndices() const;
    QString rawResponse() const;

public:
    void applyRequested()
    W_SIGNAL(applyRequested)
    void discardRequested()
    W_SIGNAL(discardRequested)

private:
    void onActionSelectionChanged();
    W_SLOT(onActionSelectionChanged)
    void onApplyClicked();
    W_SLOT(onApplyClicked)
    void onDiscardClicked();
    W_SLOT(onDiscardClicked)

private:
    void refreshActionDetail();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_rationaleLabel = nullptr;
    QListWidget *m_actionList = nullptr;
    QPlainTextEdit *m_actionDetail = nullptr;
    QPlainTextEdit *m_diffSummary = nullptr;
    TextCompareView *m_compareView = nullptr;
    QPlainTextEdit *m_rawResponse = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_discardButton = nullptr;
    QVector<ActionItem> m_actions;
    QString m_overallDiffSummary;
};
