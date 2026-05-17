#pragma once

#include <QString>

namespace PlotEngine::AI {

bool saveSecret(const QString &service, const QString &secret);
QString loadSecret(const QString &service);
bool clearSecret(const QString &service);

}
