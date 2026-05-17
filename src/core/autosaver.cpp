#include "autosaver.h"
#include "wobjectimpl.h"

#include "core/novelproject.h"
#include "projectmanager.h"

import PlotEngine.Core.RevisionManager;

W_OBJECT_IMPL(AutoSaver)

AutoSaver::AutoSaver(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setSingleShot(false);
    QObject::connect(m_timer, &QTimer::timeout, this, [this]() {
        trySave();
    });
}

void AutoSaver::setIntervalSeconds(int seconds)
{
    m_intervalSeconds = seconds;
    if (m_enabled && m_timer->isActive()) {
        m_timer->start(seconds * 1000);
    }
}

void AutoSaver::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        m_timer->start(m_intervalSeconds * 1000);
    } else {
        m_timer->stop();
    }
}

void AutoSaver::trySave()
{
    if (!m_enabled || !m_project || m_project->filePath.isEmpty())
        return;

    auto now = QDateTime::currentDateTime();
    if (m_lastSaveTime.isValid() && m_lastSaveTime.msecsTo(now) < m_intervalSeconds * 1000)
        return;

    QString backupDir = m_project->filePath + ".bak";
    QDir dir;
    if (!dir.exists(backupDir))
        dir.mkpath(backupDir);

    QString backupPath = backupDir + "/" + now.toString("yyyyMMdd_HHmmss") + ".plotproj";
    if (ProjectManager::save(*m_project, backupPath)) {
        if (m_revisionCallback) {
            m_revisionCallback(m_project->filePath, backupPath,
                PlotEngine::Revisions::RevisionType::ManualEdit);
        }
        m_lastSaveTime = now;
        emit autoSaved(backupPath);
    }
}
