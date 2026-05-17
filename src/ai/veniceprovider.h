#pragma once

#include "iaiprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "wobjectdefs.h"

class VeniceProvider : public IAiProvider {
    W_OBJECT(VeniceProvider)
public:
    explicit VeniceProvider(const QString &apiKey, QObject *parent = nullptr);

    QString providerName() const override { return "Venice"; }
    void generate(const AiRequest &request) override;
    void generateStream(const AiRequest &request) override;
    void cancel() override;
    bool isStreaming() const override { return m_streaming; }
    QStringList availableModels() const override;

    void setApiKey(const QString &key) { m_apiKey = key; }
    void setBaseUrl(const QString &url) { m_baseUrl = url; }

private:
    QString buildRequestBody(const AiRequest &request) const;
    void handleResponse(QNetworkReply *reply);
    void handleStreamChunk(const QByteArray &data);

    QString m_apiKey;
    QString m_baseUrl = "https://api.venice.ai/api/v1";
    QNetworkAccessManager m_network;
    QNetworkReply *m_currentReply = nullptr;
    bool m_streaming = false;
    QString m_streamBuffer;
};
