module;

#include <QByteArray>
#include <QSettings>
#include <QString>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dpapi.h>
#include <wincrypt.h>
#endif

export module PlotEngine.AI.SecretStore;
import PlotEngine.App.Metadata;

namespace {

QString secretSettingsKey(const QString &service)
{
    return QStringLiteral("ai/secrets/%1").arg(service);
}

}

export namespace PlotEngine::AI {

bool saveSecret(const QString &service, const QString &secret)
{
    QSettings settings = PlotEngine::App::makeSettings();

#ifdef Q_OS_WIN
    QByteArray plain = secret.toUtf8();
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(plain.data());
    in.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB out = {};
    if (!CryptProtectData(&in, L"PlotEngine AI secret", nullptr, nullptr, nullptr, 0, &out))
        return false;

    QByteArray encrypted(reinterpret_cast<const char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    settings.setValue(secretSettingsKey(service), encrypted.toBase64());
    return true;
#else
    settings.setValue(secretSettingsKey(service), secret);
    return true;
#endif
}

QString loadSecret(const QString &service)
{
    QSettings settings = PlotEngine::App::makeSettings();
    const QByteArray stored = settings.value(secretSettingsKey(service)).toByteArray();
    if (stored.isEmpty())
        return {};

#ifdef Q_OS_WIN
    const QByteArray encrypted = QByteArray::fromBase64(stored);
    if (encrypted.isEmpty())
        return {};

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(encrypted.constData()));
    in.cbData = static_cast<DWORD>(encrypted.size());

    DATA_BLOB out = {};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
        return {};

    QString secret = QString::fromUtf8(reinterpret_cast<const char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return secret;
#else
    return QString::fromUtf8(stored);
#endif
}

bool clearSecret(const QString &service)
{
    QSettings settings = PlotEngine::App::makeSettings();
    settings.remove(secretSettingsKey(service));
    return true;
}

}
