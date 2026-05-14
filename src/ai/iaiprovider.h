#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include "airequest.h"

class IAiProvider : public QObject {
    Q_OBJECT
public:
    explicit IAiProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAiProvider() = default;

    virtual QString providerName() const = 0;
    virtual void generate(const AiRequest &request) = 0;
    virtual void generateStream(const AiRequest &request) = 0;
    virtual void cancel() = 0;
    virtual bool isStreaming() const = 0;
    virtual QStringList availableModels() const = 0;

signals:
    void responseReceived(const AiResponse &response);
    void streamChunk(const QString &chunk);
    void streamFinished();
    void error(const QString &message);
};
