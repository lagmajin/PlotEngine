#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>
#include <utility>
#include <functional>
#include "ai/airequest.h"
#include "ai/iaiprovider.h"
#include "wobjectdefs.h"

struct BatchTask {
    QString episodeId;
    QString chapterId;
    QString instruction;
    bool completed = false;
    bool failed = false;
    QString result;
    QString error;
    int retryCount = 0;
    int maxRetries = 3;
};

class BatchGenerator : public QObject {
    W_OBJECT(BatchGenerator)
public:
    explicit BatchGenerator(IAiProvider *provider, QObject *parent = nullptr);

    void enqueue(const QString &chapterId, const QString &episodeId, const QString &instruction = QString());
    void enqueueBatch(const QVector<std::pair<QString, QString>> &episodeIds, const QString &instruction = QString());
    void start();
    void cancel();
    void pause();
    void resume();

    int totalTasks() const { return m_tasks.size(); }
    int completedTasks() const;
    int failedTasks() const;
    int pendingTasks() const;
    bool isRunning() const { return m_running; }
    bool isPaused() const { return m_paused; }

private:
    void processNext();
    W_SLOT(processNext)
    void onTaskCompleted(const AiResponse &response);
    W_SLOT(onTaskCompleted, (const AiResponse &))
    void onTaskError(const QString &error);
    W_SLOT(onTaskError, (const QString &))

    IAiProvider *m_provider = nullptr;
    QVector<BatchTask> m_tasks;
    int m_currentIndex = -1;
    bool m_running = false;
    bool m_paused = false;
    bool m_cancelled = false;

public:
    void batchStarted()
    W_SIGNAL(batchStarted)
    void batchFinished()
    W_SIGNAL(batchFinished)
    void batchCancelled()
    W_SIGNAL(batchCancelled)
    void taskStarted(int index, const QString &episodeId)
    W_SIGNAL(taskStarted, (int, const QString &), index, episodeId)
    void taskCompleted(int index, const QString &episodeId, const QString &result)
    W_SIGNAL(taskCompleted, (int, const QString &, const QString &), index, episodeId, result)
    void taskFailed(int index, const QString &episodeId, const QString &error)
    W_SIGNAL(taskFailed, (int, const QString &, const QString &), index, episodeId, error)
    void taskRetrying(int index, const QString &episodeId, int retryCount)
    W_SIGNAL(taskRetrying, (int, const QString &, int), index, episodeId, retryCount)
    void progressChanged(int completed, int total)
    W_SIGNAL(progressChanged, (int, int), completed, total)
};
