#include "pythonscriptrunner.h"

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonObject>

#include <cmath>
#include <limits>
#include <memory>
#include <mutex>

#ifdef slots
#pragma push_macro("slots")
#undef slots
#define PLOTENGINE_RESTORE_SLOTS_MACRO
#endif
#ifdef signals
#pragma push_macro("signals")
#undef signals
#define PLOTENGINE_RESTORE_SIGNALS_MACRO
#endif
#ifdef emit
#pragma push_macro("emit")
#undef emit
#define PLOTENGINE_RESTORE_EMIT_MACRO
#endif
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#ifdef PLOTENGINE_RESTORE_EMIT_MACRO
#pragma pop_macro("emit")
#undef PLOTENGINE_RESTORE_EMIT_MACRO
#endif
#ifdef PLOTENGINE_RESTORE_SIGNALS_MACRO
#pragma pop_macro("signals")
#undef PLOTENGINE_RESTORE_SIGNALS_MACRO
#endif
#ifdef PLOTENGINE_RESTORE_SLOTS_MACRO
#pragma pop_macro("slots")
#undef PLOTENGINE_RESTORE_SLOTS_MACRO
#endif

import PlotEngine.Core.NovelProject;

namespace py = pybind11;

namespace {

py::object qjsonValueToPython(const QJsonValue &value);
QJsonValue pythonToQJsonValue(const py::handle &value);

py::object qjsonObjectToPython(const QJsonObject &object)
{
    py::dict dict;
    for (auto it = object.begin(); it != object.end(); ++it)
    {
        const QByteArray keyBytes = it.key().toUtf8();
        dict[py::str(keyBytes.constData())] = qjsonValueToPython(it.value());
    }
    return std::move(dict);
}

py::object qjsonArrayToPython(const QJsonArray &array)
{
    py::list list;
    for (const auto &entry : array)
        list.append(qjsonValueToPython(entry));
    return std::move(list);
}

py::object qjsonValueToPython(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined())
        return py::none();
    if (value.isBool())
        return py::bool_(value.toBool());
    if (value.isDouble()) {
        const double number = value.toDouble();
        const double truncated = std::trunc(number);
        if (std::isfinite(number) && truncated == number &&
            number >= static_cast<double>(std::numeric_limits<long long>::min()) &&
            number <= static_cast<double>(std::numeric_limits<long long>::max())) {
            return py::int_(static_cast<long long>(number));
        }
        return py::float_(number);
    }
    if (value.isString()) {
        const QByteArray stringBytes = value.toString().toUtf8();
        return py::str(stringBytes.constData());
    }
    if (value.isArray())
        return qjsonArrayToPython(value.toArray());
    if (value.isObject())
        return qjsonObjectToPython(value.toObject());
    return py::none();
}

QJsonValue pythonToQJsonValue(const py::handle &value)
{
    if (value.is_none())
        return QJsonValue();
    if (py::isinstance<py::bool_>(value))
        return QJsonValue(value.cast<bool>());
    if (py::isinstance<py::int_>(value)) {
        const auto number = value.cast<long long>();
        return QJsonValue(static_cast<double>(number));
    }
    if (py::isinstance<py::float_>(value))
        return QJsonValue(value.cast<double>());
    if (py::isinstance<py::str>(value)) {
        const std::string stringValue = value.cast<std::string>();
        return QJsonValue(QString::fromUtf8(stringValue.data(), static_cast<int>(stringValue.size())));
    }
    if (py::isinstance<py::dict>(value)) {
        QJsonObject object;
        py::dict dict = value.cast<py::dict>();
        for (auto item : dict) {
            const std::string keyValue = item.first.cast<std::string>();
            const QString key = QString::fromUtf8(keyValue.data(), static_cast<int>(keyValue.size()));
            object.insert(key, pythonToQJsonValue(item.second));
        }
        return object;
    }
    if (py::isinstance<py::list>(value) || py::isinstance<py::tuple>(value)) {
        QJsonArray array;
        for (auto item : value)
            array.append(pythonToQJsonValue(item));
        return array;
    }
    throw py::type_error("Unsupported Python type for JSON conversion");
}

QString readScriptFile(const QString &scriptPath, QString *error)
{
    QFile file(scriptPath);
    if (!file.exists()) {
        if (error)
            *error = QStringLiteral("スクリプトが見つかりません: %1").arg(scriptPath);
        return {};
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("スクリプトを開けません: %1").arg(scriptPath);
        return {};
    }
    const QByteArray data = file.readAll();
    return QString::fromUtf8(data);
}

class PythonInterpreterGuard {
public:
    PythonInterpreterGuard()
    {
        std::call_once(s_once, []() {
            s_guard = std::make_unique<py::scoped_interpreter>();
        });
    }

private:
    static inline std::once_flag s_once;
    static inline std::unique_ptr<py::scoped_interpreter> s_guard;
};

class PythonStreamRedirectGuard {
public:
    PythonStreamRedirectGuard(py::module_ &sys, py::object stdoutBuffer, py::object stderrBuffer)
        : m_sys(sys)
        , m_oldStdout(sys.attr("stdout"))
        , m_oldStderr(sys.attr("stderr"))
    {
        m_sys.attr("stdout") = stdoutBuffer;
        m_sys.attr("stderr") = stderrBuffer;
    }

    ~PythonStreamRedirectGuard()
    {
        try {
            m_sys.attr("stdout") = m_oldStdout;
            m_sys.attr("stderr") = m_oldStderr;
        } catch (...) {
        }
    }

private:
    py::module_ &m_sys;
    py::object m_oldStdout;
    py::object m_oldStderr;
};

void bindPlotEngineModule(py::module_ &m)
{
    m.def("project_to_json", [](const NovelProject &project) {
        return qjsonObjectToPython(project.toJson());
    }, "Convert a NovelProject to a Python JSON-like dict.");
}

} // namespace

PYBIND11_EMBEDDED_MODULE(plotengine, m)
{
    bindPlotEngineModule(m);
}

namespace PlotEngine::Scripting {

ScriptExecutionResult PythonScriptRunner::runFile(const QString &scriptPath, NovelProject &project)
{
    ScriptExecutionResult result;
    QString errorMessage;
    const QString source = readScriptFile(scriptPath, &errorMessage);
    if (source.isEmpty() && !errorMessage.isEmpty()) {
        result.error = errorMessage;
        return result;
    }

    PythonInterpreterGuard interpreterGuard;
    py::gil_scoped_acquire gil;

    py::module_ sys = py::module_::import("sys");
    py::module_ io = py::module_::import("io");
    py::object stdoutBuffer = io.attr("StringIO")();
    py::object stderrBuffer = io.attr("StringIO")();
    PythonStreamRedirectGuard redirectGuard(sys, stdoutBuffer, stderrBuffer);

    try {
        const QByteArray sourceBytes = source.toUtf8();
        const QByteArray scriptPathBytes = scriptPath.toUtf8();
        const QJsonObject originalProject = project.toJson();
        py::dict globals;
        globals["__name__"] = "__main__";
        globals["__file__"] = scriptPathBytes.constData();
        globals["plotengine"] = py::module_::import("plotengine");
        globals["project"] = qjsonObjectToPython(project.toJson());

        py::exec(sourceBytes.constData(), globals, globals);

        if (globals.contains("project")) {
            const QJsonValue updated = pythonToQJsonValue(globals["project"]);
            if (!updated.isObject()) {
                result.error = QStringLiteral("script の project は dict である必要があります");
                return result;
            }
            const QJsonObject updatedProject = updated.toObject();
            result.changed = (updatedProject != originalProject);
            if (result.changed)
                project = NovelProject::fromJson(updatedProject);
        }

        result.success = true;
    } catch (const py::error_already_set &e) {
        result.error = QString::fromUtf8(e.what());
    } catch (const std::exception &e) {
        result.error = QString::fromUtf8(e.what());
    }

    const QString stdoutText = QString::fromUtf8(stdoutBuffer.attr("getvalue")().cast<std::string>().c_str());
    const QString stderrText = QString::fromUtf8(stderrBuffer.attr("getvalue")().cast<std::string>().c_str());
    result.output = stdoutText;
    if (!result.error.isEmpty()) {
        if (!stderrText.isEmpty())
            result.error += QStringLiteral("\n") + stderrText;
    } else if (!stderrText.isEmpty()) {
        result.error = stderrText;
    }

    return result;
}

} // namespace PlotEngine::Scripting
