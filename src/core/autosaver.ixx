module;

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QDir>
#include <QFile>
#include <functional>
#include <utility>
#include "wobjectdefs.h"
#include "wobjectimpl.h"

export module PlotEngine.Core.AutoSaver;
import PlotEngine.Core.NovelProject;
import PlotEngine.Core.ProjectManager;
import PlotEngine.Core.RevisionManager;

namespace PlotEngine::Revisions {
enum class RevisionType;
}

export class AutoSaver : public QObject {
    W_OBJECT(AutoSaver)
public:
    explicit AutoSaver(QObject *parent = nullptr);

    void setIntervalSeconds(int seconds);
    int intervalSeconds() const { return m_intervalSeconds; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setProject(NovelProject *project) { m_project = project; }
    void setRevisionCallback(std::function<void(const QString &, const QString &, PlotEngine::Revisions::RevisionType)> cb) {
        m_revisionCallback = std::move(cb);
    }

    void trySave();

    QDateTime lastSaveTime() const { return m_lastSaveTime; }
    int secondsSinceLastSave() const {
        if (!m_lastSaveTime.isValid())
            return -1;
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

W_OBJECT_IMPL(AutoSaver)

inline AutoSaver::AutoSaver(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setSingleShot(false);
    QObject::connect(m_timer, &QTimer::timeout, this, [this]() { trySave(); });
}

inline void AutoSaver::setIntervalSeconds(int seconds)
{
    m_intervalSeconds = seconds;
    if (m_enabled && m_timer->isActive())
        m_timer->start(seconds * 1000);
}

inline void AutoSaver::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled)
        m_timer->start(m_intervalSeconds * 1000);
    else
        m_timer->stop();
}

inline void AutoSaver::trySave()
{
    if (!m_enabled || !m_project || m_project->filePath.isEmpty())
        return;

    const QDateTime now = QDateTime::currentDateTime();
    if (m_lastSaveTime.isValid() && m_lastSaveTime.msecsTo(now) < m_intervalSeconds * 1000)
        return;

    const QString backupDir = m_project->filePath + ".bak";
    QDir dir;
    if (!dir.exists(backupDir))
        dir.mkpath(backupDir);

    const QString backupPath = backupDir + "/" + now.toString("yyyyMMdd_HHmmss") + ".plotproj";
    if (ProjectManager::save(*m_project, backupPath)) {
        if (m_revisionCallback) {
            m_revisionCallback(m_project->filePath, backupPath,
                PlotEngine::Revisions::RevisionType::ManualEdit);
        }
        m_lastSaveTime = now;
        emit autoSaved(backupPath);
    }
}
