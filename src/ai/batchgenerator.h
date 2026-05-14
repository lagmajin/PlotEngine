#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>
#include <utility>
#include <functional>
#include "ai/airequest.h"
#include "ai/iaiprovider.h"

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
    Q_OBJECT
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
    void onTaskCompleted(const AiResponse &response);
    void onTaskError(const QString &error);

    IAiProvider *m_provider = nullptr;
    QVector<BatchTask> m_tasks;
    int m_currentIndex = -1;
    bool m_running = false;
    bool m_paused = false;
    bool m_cancelled = false;

signals:
    void batchStarted();
    void batchFinished();
    void batchCancelled();
    void taskStarted(int index, const QString &episodeId);
    void taskCompleted(int index, const QString &episodeId, const QString &result);
    void taskFailed(int index, const QString &episodeId, const QString &error);
    void taskRetrying(int index, const QString &episodeId, int retryCount);
    void progressChanged(int completed, int total);
};
