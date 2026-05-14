#pragma once

#include "iaiprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class AnthropicProvider : public IAiProvider {
    Q_OBJECT
public:
    explicit AnthropicProvider(const QString &apiKey, QObject *parent = nullptr);

    QString providerName() const override { return "Anthropic"; }
    void generate(const AiRequest &request) override;
    void generateStream(const AiRequest &request) override;
    void cancel() override;
    bool isStreaming() const override { return m_streaming; }
    QStringList availableModels() const override;

    void setApiKey(const QString &key) { m_apiKey = key; }

private:
    QString buildRequestBody(const AiRequest &request) const;
    void handleResponse(QNetworkReply *reply);
    void handleStreamChunk(const QByteArray &data);

    QString m_apiKey;
    QNetworkAccessManager m_network;
    QNetworkReply *m_currentReply = nullptr;
    bool m_streaming = false;
    QString m_streamBuffer;
};
