#pragma once

#include <QString>

class NovelProject;

namespace PlotEngine::Scripting {

struct ScriptExecutionResult {
    bool success = false;
    bool changed = false;
    QString output;
    QString error;
};

class PythonScriptRunner {
public:
    static ScriptExecutionResult runFile(const QString &scriptPath, NovelProject &project);
};

}
