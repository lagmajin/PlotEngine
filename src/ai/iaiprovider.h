#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include "airequest.h"
#include "wobjectdefs.h"

class IAiProvider : public QObject {
    W_OBJECT(IAiProvider)
public:
    explicit IAiProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAiProvider() = default;

    virtual QString providerName() const = 0;
    virtual void generate(const AiRequest &request) = 0;
    virtual void generateStream(const AiRequest &request) = 0;
    virtual void cancel() = 0;
    virtual bool isStreaming() const = 0;
    virtual QStringList availableModels() const = 0;

public:
    void responseReceived(const AiResponse &response)
    W_SIGNAL(responseReceived, (const AiResponse &), response)
    void streamChunk(const QString &chunk)
    W_SIGNAL(streamChunk, (const QString &), chunk)
    void streamFinished()
    W_SIGNAL(streamFinished)
    void error(const QString &message)
    W_SIGNAL(error, (const QString &), message)
};
