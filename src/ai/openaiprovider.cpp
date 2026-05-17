#include "openaiprovider.h"
#include "wobjectimpl.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>

W_OBJECT_IMPL(OpenAiProvider)

OpenAiProvider::OpenAiProvider(const QString &apiKey, QObject *parent)
    : IAiProvider(parent), m_apiKey(apiKey)
{
}

QStringList OpenAiProvider::availableModels() const
{
    return {
        "gpt-4o",
        "gpt-4o-mini",
        "gpt-4-turbo",
        "gpt-3.5-turbo",
    };
}

void OpenAiProvider::generate(const AiRequest &request)
{
    if (m_streaming) {
        emit error("A stream is already in progress");
        return;
    }

    QUrl url(m_baseUrl + "/v1/chat/completions");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    m_currentReply = m_network.post(req, buildRequestBody(request).toUtf8());

    QObject::connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        handleResponse(m_currentReply);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    });
}

void OpenAiProvider::generateStream(const AiRequest &request)
{
    if (m_streaming) {
        emit error("A stream is already in progress");
        return;
    }

    m_streaming = true;
    m_streamBuffer.clear();

    AiRequest streamRequest = request;
    streamRequest.stream = true;

    QUrl url(m_baseUrl + "/v1/chat/completions");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    m_currentReply = m_network.post(req, buildRequestBody(streamRequest).toUtf8());

    QObject::connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
        QByteArray data = m_currentReply->readAll();
        handleStreamChunk(data);
    });

    QObject::connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        m_streaming = false;
        emit streamFinished();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    });

    QObject::connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
        this, [this](QNetworkReply::NetworkError code) {
            m_streaming = false;
            emit error("Stream error: " + m_currentReply->errorString());
        });
}

void OpenAiProvider::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_streaming = false;
        m_currentReply = nullptr;
    }
}

QString OpenAiProvider::buildRequestBody(const AiRequest &request) const
{
    QJsonObject body;
    body["model"] = request.model.isEmpty() ? "gpt-4o" : request.model;
    body["temperature"] = request.temperature;
    body["max_tokens"] = request.maxTokens;
    body["stream"] = request.stream;

    QJsonArray messages;
    for (const auto &msg : request.messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        messages.append(msgObj);
    }
    body["messages"] = messages;

    for (auto it = request.extraParams.begin(); it != request.extraParams.end(); ++it) {
        body[it.key()] = it.value();
    }

    return QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void OpenAiProvider::handleResponse(QNetworkReply *reply)
{
    AiResponse response;

    if (reply->error() != QNetworkReply::NoError) {
        response.success = false;
        response.error = reply->errorString();
        emit error(response.error);
        emit responseReceived(response);
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        response.success = false;
        response.error = "Invalid JSON response";
        emit error(response.error);
        emit responseReceived(response);
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray choices = root["choices"].toArray();
    if (!choices.isEmpty()) {
        QJsonObject choice = choices[0].toObject();
        QJsonObject message = choice["message"].toObject();
        response.content = message["content"].toString();
        response.finishReason = choice["finish_reason"].toString();
    }

    QJsonObject usage = root["usage"].toObject();
    response.promptTokens = usage["prompt_tokens"].toInt();
    response.completionTokens = usage["completion_tokens"].toInt();
    response.totalTokens = usage["total_tokens"].toInt();
    response.model = root["model"].toString();

    emit responseReceived(response);
}

void OpenAiProvider::handleStreamChunk(const QByteArray &data)
{
    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.startsWith("data: "))
            continue;

        QString payload = trimmed.mid(6);
        if (payload == "[DONE]") {
            emit streamFinished();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (doc.isNull() || !doc.isObject())
            continue;

        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty())
            continue;

        QJsonObject delta = choices[0].toObject()["delta"].toObject();
        QString content = delta["content"].toString();
        if (!content.isEmpty()) {
            m_streamBuffer += content;
            emit streamChunk(content);
        }
    }
}
