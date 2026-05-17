#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QDir>
#include <QFile>
#include <functional>
#include "wobjectdefs.h"

class NovelProject;

namespace PlotEngine::Revisions {
enum class RevisionType;
}

class AutoSaver : public QObject {
    W_OBJECT(AutoSaver)
public:
    explicit AutoSaver(QObject *parent = nullptr);

    void setIntervalSeconds(int seconds);

    int intervalSeconds() const { return m_intervalSeconds; }

    void setEnabled(bool enabled);

    bool isEnabled() const { return m_enabled; }

    void setProject(NovelProject *project) { m_project = project; }

    void setRevisionCallback(std::function<void(const QString &, const QString &, PlotEngine::Revisions::RevisionType)> cb) {
        m_revisionCallback = cb;
    }

    void trySave();

    QDateTime lastSaveTime() const { return m_lastSaveTime; }
    int secondsSinceLastSave() const {
        if (!m_lastSaveTime.isValid()) return -1;
        return m_lastSaveTime.secsTo(QDateTime::currentDateTime());
    }

public:
    void autoSaved(const QString &backupPath)
    W_SIGNAL(autoSaved, (const QString &), backupPath)

private:
    QTimer *m_timer = nullptr;
    NovelProject *m_project = nullptr;
    int m_intervalSeconds = 60;
    bool m_enabled = true;
    QDateTime m_lastSaveTime;
    std::function<void(const QString &, const QString &, PlotEngine::Revisions::RevisionType)> m_revisionCallback;
};
