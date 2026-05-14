#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>

struct AiMessage {
    QString role;
    QString content;

    static AiMessage system(const QString &content) {
        return {"system", content};
    }
    static AiMessage user(const QString &content) {
        return {"user", content};
    }
    static AiMessage assistant(const QString &content) {
        return {"assistant", content};
    }
};

struct AiRequest {
    QVector<AiMessage> messages;
    QString model;
    double temperature = 0.7;
    int maxTokens = 4096;
    bool stream = false;
    QJsonObject extraParams;

    static AiRequest forGeneration(const QString &systemPrompt, const QString &userPrompt,
                                    const QString &model = QString(),
                                    double temperature = 0.7,
                                    int maxTokens = 4096)
    {
        AiRequest req;
        req.messages.append(AiMessage::system(systemPrompt));
        req.messages.append(AiMessage::user(userPrompt));
        req.model = model;
        req.temperature = temperature;
        req.maxTokens = maxTokens;
        req.stream = false;
        return req;
    }

    static AiRequest forStreamGeneration(const QString &systemPrompt, const QString &userPrompt,
                                          const QString &model = QString(),
                                          double temperature = 0.7,
                                          int maxTokens = 4096)
    {
        auto req = forGeneration(systemPrompt, userPrompt, model, temperature, maxTokens);
        req.stream = true;
        return req;
    }
};

struct AiResponse {
    QString content;
    QString model;
    int promptTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
    QString finishReason;
    bool success = true;
    QString error;
};
