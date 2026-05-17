#include "anthropicprovider.h"
#include "wobjectimpl.h"
#include <QUrl>
#include <QNetworkRequest>

W_OBJECT_IMPL(AnthropicProvider)

AnthropicProvider::AnthropicProvider(const QString &apiKey, QObject *parent)
    : IAiProvider(parent), m_apiKey(apiKey)
{
}

QStringList AnthropicProvider::availableModels() const
{
    return {
        "claude-sonnet-4-20250514",
        "claude-opus-4-20250514",
        "claude-3-5-sonnet-20241022",
        "claude-3-5-haiku-20241022",
    };
}

void AnthropicProvider::generate(const AiRequest &request)
{
    if (m_streaming) {
        emit error("A stream is already in progress");
        return;
    }

    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", m_apiKey.toUtf8());
    req.setRawHeader("anthropic-version", "2023-06-01");

    m_currentReply = m_network.post(req, buildRequestBody(request).toUtf8());

    QObject::connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        handleResponse(m_currentReply);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    });
}

void AnthropicProvider::generateStream(const AiRequest &request)
{
    if (m_streaming) {
        emit error("A stream is already in progress");
        return;
    }

    m_streaming = true;
    m_streamBuffer.clear();

    AiRequest streamRequest = request;
    streamRequest.stream = true;

    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", m_apiKey.toUtf8());
    req.setRawHeader("anthropic-version", "2023-06-01");

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
        this, [this](QNetworkReply::NetworkError) {
            m_streaming = false;
            emit error("Stream error: " + m_currentReply->errorString());
        });
}

void AnthropicProvider::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_streaming = false;
        m_currentReply = nullptr;
    }
}

QString AnthropicProvider::buildRequestBody(const AiRequest &request) const
{
    QJsonObject body;
    body["model"] = request.model.isEmpty() ? "claude-sonnet-4-20250514" : request.model;
    body["max_tokens"] = request.maxTokens;
    body["temperature"] = request.temperature;
    body["stream"] = request.stream;

    QJsonArray messages;
    for (const auto &msg : request.messages) {
        if (msg.role == "system")
            continue;
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        QJsonArray contentArr;
        QJsonObject textObj;
        textObj["type"] = "text";
        textObj["text"] = msg.content;
        contentArr.append(textObj);
        msgObj["content"] = contentArr;
        messages.append(msgObj);
    }
    body["messages"] = messages;

    QString systemPrompt;
    for (const auto &msg : request.messages) {
        if (msg.role == "system") {
            systemPrompt = msg.content;
            break;
        }
    }
    if (!systemPrompt.isEmpty())
        body["system"] = systemPrompt;

    for (auto it = request.extraParams.begin(); it != request.extraParams.end(); ++it) {
        body[it.key()] = it.value();
    }

    return QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void AnthropicProvider::handleResponse(QNetworkReply *reply)
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
    QJsonArray contentArr = root["content"].toArray();
    for (const auto &val : contentArr) {
        QJsonObject block = val.toObject();
        if (block["type"].toString() == "text") {
            response.content = block["text"].toString();
            break;
        }
    }

    response.finishReason = root["stop_reason"].toString();
    response.model = root["model"].toString();

    QJsonObject usage = root["usage"].toObject();
    response.promptTokens = usage["input_tokens"].toInt();
    response.completionTokens = usage["output_tokens"].toInt();
    response.totalTokens = response.promptTokens + response.completionTokens;

    emit responseReceived(response);
}

void AnthropicProvider::handleStreamChunk(const QByteArray &data)
{
    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.startsWith("data: "))
            continue;

        QString payload = trimmed.mid(6);
        QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (doc.isNull() || !doc.isObject())
            continue;

        QJsonObject root = doc.object();
        QString type = root["type"].toString();

        if (type == "message_stop") {
            emit streamFinished();
            return;
        }

        if (type == "content_block_delta") {
            QJsonObject delta = root["delta"].toObject();
            if (delta["type"].toString() == "text_delta") {
                QString content = delta["text"].toString();
                m_streamBuffer += content;
                emit streamChunk(content);
            }
        }
    }
}
