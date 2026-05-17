#include "batchgenerator.h"
#include "wobjectimpl.h"
import PlotEngine.Core.ContextBuilder;

W_OBJECT_IMPL(BatchGenerator)

BatchGenerator::BatchGenerator(IAiProvider *provider, QObject *parent)
    : QObject(parent), m_provider(provider)
{
    connect(m_provider, &IAiProvider::responseReceived, this, &BatchGenerator::onTaskCompleted);
    connect(m_provider, &IAiProvider::error, this, &BatchGenerator::onTaskError);
}

void BatchGenerator::enqueue(const QString &chapterId, const QString &episodeId, const QString &instruction)
{
    BatchTask task;
    task.chapterId = chapterId;
    task.episodeId = episodeId;
    task.instruction = instruction;
    m_tasks.append(task);
}

void BatchGenerator::enqueueBatch(const QVector<std::pair<QString, QString>> &episodeIds, const QString &instruction)
{
    for (const auto &pair : episodeIds) {
        enqueue(pair.first, pair.second, instruction);
    }
}

void BatchGenerator::start()
{
    if (m_tasks.isEmpty())
        return;

    m_running = true;
    m_paused = false;
    m_cancelled = false;
    m_currentIndex = 0;
    processNext();
}

void BatchGenerator::cancel()
{
    m_cancelled = true;
    m_running = false;
    m_provider->cancel();
    emit batchCancelled();
}

void BatchGenerator::pause()
{
    m_paused = true;
}

void BatchGenerator::resume()
{
    if (m_paused && !m_cancelled && m_currentIndex < m_tasks.size()) {
        m_paused = false;
        processNext();
    }
}

int BatchGenerator::completedTasks() const
{
    int count = 0;
    for (const auto &task : m_tasks) {
        if (task.completed) ++count;
    }
    return count;
}

int BatchGenerator::failedTasks() const
{
    int count = 0;
    for (const auto &task : m_tasks) {
        if (task.failed) ++count;
    }
    return count;
}

int BatchGenerator::pendingTasks() const
{
    int count = 0;
    for (const auto &task : m_tasks) {
        if (!task.completed && !task.failed) ++count;
    }
    return count;
}

void BatchGenerator::processNext()
{
    if (m_cancelled || m_paused)
        return;

    if (m_currentIndex >= m_tasks.size()) {
        m_running = false;
        emit batchFinished();
        return;
    }

    const auto &task = m_tasks[m_currentIndex];
    emit taskStarted(m_currentIndex, task.episodeId);
}

void BatchGenerator::onTaskCompleted(const AiResponse &response)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_tasks.size())
        return;

    auto &task = m_tasks[m_currentIndex];

    if (response.success) {
        task.completed = true;
        task.result = response.content;
        emit taskCompleted(m_currentIndex, task.episodeId, response.content);
    } else {
        task.retryCount++;
        if (task.retryCount < task.maxRetries) {
            emit taskRetrying(m_currentIndex, task.episodeId, task.retryCount);
            QTimer::singleShot(2000 * task.retryCount, this, [this]() {
                processNext();
            });
            return;
        }
        task.failed = true;
        task.error = response.error;
        emit taskFailed(m_currentIndex, task.episodeId, response.error);
    }

    m_currentIndex++;
    QTimer::singleShot(500, this, [this]() {
        processNext();
    });
}

void BatchGenerator::onTaskError(const QString &error)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_tasks.size())
        return;

    auto &task = m_tasks[m_currentIndex];
    task.retryCount++;

    if (task.retryCount < task.maxRetries && !m_cancelled) {
        emit taskRetrying(m_currentIndex, task.episodeId, task.retryCount);
        QTimer::singleShot(2000 * task.retryCount, this, [this]() {
            processNext();
        });
        return;
    }

    task.failed = true;
    task.error = error;
    emit taskFailed(m_currentIndex, task.episodeId, error);

    m_currentIndex++;
    QTimer::singleShot(500, this, [this]() {
        processNext();
    });
}
