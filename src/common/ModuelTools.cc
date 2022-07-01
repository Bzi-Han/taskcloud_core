#include "ModuleTools.h"

namespace ModuleTools::Detail
{
    void defaultLogger(LogType logType, const std::string_view &message, void *userData)
    {
        switch (logType)
        {
        case LogType::operation:
            std::cout << "[*] ";
            break;
        case LogType::info:
            std::cout << "[=] ";
            break;
        case LogType::failed:
            std::cout << "[-] ";
            break;
        case LogType::succeed:
            std::cout << "[+] ";
            break;
        }

        std::cout << message << std::endl;
    }

    std::vector<std::string> luaArguments(lua_State *luaState, int index)
    {
        std::vector<std::string> result;

        int argumentsCount = lua_gettop(luaState);
        for (int i = 1; i <= argumentsCount; i++)
        {
            if (lua_isnil(luaState, i))
                result.emplace_back("nil");
            else if (lua_istable(luaState, i))
                result.emplace_back("table");
            else
                result.push_back(lua_tostring(luaState, i));
        }

        return result;
    }

    std::vector<std::string> pythonArguments(const pybind11::args &args)
    {
        std::vector<std::string> result;

        for (auto arg : args)
        {
            result.push_back(pybind11::str(arg).cast<std::string>());
        }

        return result;
    }

    std::vector<std::string> javascriptArguments(JSContext *context, int argc, JSValueConst *argv)
    {
        std::vector<std::string> result;

        for (int i = 0; i < argc; i++)
        {
            result.push_back(JS_ToCString(context, argv[i]));
        }

        return result;
    }
}

namespace ModuleTools::Logger
{
    void operation(const std::vector<std::string> &items, void *userData)
    {
        std::stringstream ss;

        for (const auto &item : items)
            ss << item << " ";

        logger(Detail::LogType::operation, ss.str(), userData);
    }

    void info(const std::vector<std::string> &items, void *userData)
    {
        std::stringstream ss;

        for (const auto &item : items)
            ss << item << " ";

        logger(Detail::LogType::info, ss.str(), userData);
    }

    void failed(const std::vector<std::string> &items, void *userData)
    {
        std::stringstream ss;

        for (const auto &item : items)
            ss << item << " ";

        logger(Detail::LogType::failed, ss.str(), userData);
    }

    void succeed(const std::vector<std::string> &items, void *userData)
    {
        std::stringstream ss;

        for (const auto &item : items)
            ss << item << " ";

        logger(Detail::LogType::succeed, ss.str(), userData);
    }
}

namespace ModuleTools::Bindings
{
    namespace Logger
    {
        void luaLogger(lua_State *luaState)
        {
            lua_Debug info{};

            if (0 == lua_getstack(luaState, 0, &info))
                luaL_error(luaState, "cannot get call stack");
            if (0 == lua_getinfo(luaState, "n", &info))
                luaL_error(luaState, "cannot get call info");

            lua_getglobal(luaState, "logger");
            lua_getfield(luaState, -1, "__logger_options");
            auto userData = reinterpret_cast<void *>(lua_tointeger(luaState, -1));
            lua_pop(luaState, 2);

            if (std::string_view("operation") == info.name)
                ModuleTools::Logger::operation(Detail::luaArguments(luaState, 1), userData);
            else if (std::string_view("info") == info.name)
                ModuleTools::Logger::info(Detail::luaArguments(luaState, 1), userData);
            else if (std::string_view("failed") == info.name)
                ModuleTools::Logger::failed(Detail::luaArguments(luaState, 1), userData);
            else if (std::string_view("succeed") == info.name)
                ModuleTools::Logger::succeed(Detail::luaArguments(luaState, 1), userData);
            else
                luaL_error(luaState, "unknown logger type");
        }

        void pythonLogger(pybind11::args args)
        {
            auto currentFrame = PyEval_GetFrame();
            std::string name = PyUnicode_AsUTF8(PyTuple_GetItem(currentFrame->f_code->co_names, 1));
            auto userData = reinterpret_cast<void *>(pybind11::cast<pybind11::dict>(PyEval_GetLocals())["logger"].attr("__logger_options").cast<size_t>());

            if ("operation" == name)
                ModuleTools::Logger::operation(Detail::pythonArguments(args), userData);
            else if ("info" == name)
                ModuleTools::Logger::info(Detail::pythonArguments(args), userData);
            else if ("failed" == name)
                ModuleTools::Logger::failed(Detail::pythonArguments(args), userData);
            else if ("succeed" == name)
                ModuleTools::Logger::succeed(Detail::pythonArguments(args), userData);
            else
                throw std::runtime_error("unknown logger type");
        }

        JSValue javascriptLogger(Detail::LogType logType, JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv)
        {
            int64_t rawPointer = 0;
            if (0 != JS_ToInt64(context, &rawPointer, JS_GetPropertyStr(context, this_val, "__logger_options")))
                return JS_ThrowSyntaxError(context, "cannot get logger options");
            auto userData = reinterpret_cast<void *>(rawPointer);

            switch (logType)
            {
            case Detail::LogType::operation:
                ModuleTools::Logger::operation(Detail::javascriptArguments(context, argc, argv), userData);
                break;
            case Detail::LogType::info:
                ModuleTools::Logger::info(Detail::javascriptArguments(context, argc, argv), userData);
                break;
            case Detail::LogType::failed:
                ModuleTools::Logger::failed(Detail::javascriptArguments(context, argc, argv), userData);
                break;
            case Detail::LogType::succeed:
                ModuleTools::Logger::succeed(Detail::javascriptArguments(context, argc, argv), userData);
                break;
            }

            return JS_UNDEFINED;
        }
    }
}

namespace ModuleTools
{
    std::function<void(Detail::LogType, const std::string_view &, void *)> logger = Detail::defaultLogger;

    void bind(lua_State *luaState, void *loggerOptions)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("logger")
            .addConstant("__logger_options", reinterpret_cast<size_t>(loggerOptions))
            .addFunction("operation", &Bindings::Logger::luaLogger)
            .addFunction("info", &Bindings::Logger::luaLogger)
            .addFunction("failed", &Bindings::Logger::luaLogger)
            .addFunction("succeed", &Bindings::Logger::luaLogger)
            .endNamespace();
    }

    void bind(PyObject *mainModule, void *loggerOptions)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto loggerModule = module.def_submodule("logger");
        loggerModule.add_object("__logger_options", pybind11::int_(reinterpret_cast<size_t>(loggerOptions)));
        loggerModule.def("operation", &Bindings::Logger::pythonLogger);
        loggerModule.def("info", &Bindings::Logger::pythonLogger);
        loggerModule.def("failed", &Bindings::Logger::pythonLogger);
        loggerModule.def("succeed", &Bindings::Logger::pythonLogger);
    }

    void bind(JSContext *context, void *loggerOptions)
    {
        auto global = JS_GetGlobalObject(context);

        auto loggerModule = JS_NewObject(context);
        auto loggerModule_operation = JS_NewCFunction(
            context, +[](JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv)
                     { return Bindings::Logger::javascriptLogger(Detail::LogType::operation, context, this_val, argc, argv); },
            nullptr, 0);
        auto loggerModule_info = JS_NewCFunction(
            context, +[](JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv)
                     { return Bindings::Logger::javascriptLogger(Detail::LogType::info, context, this_val, argc, argv); },
            nullptr, 0);
        auto loggerModule_failed = JS_NewCFunction(
            context, +[](JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv)
                     { return Bindings::Logger::javascriptLogger(Detail::LogType::failed, context, this_val, argc, argv); },
            nullptr, 0);
        auto loggerModule_succeed = JS_NewCFunction(
            context, +[](JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv)
                     { return Bindings::Logger::javascriptLogger(Detail::LogType::succeed, context, this_val, argc, argv); },
            nullptr, 0);

        JS_SetPropertyStr(context, loggerModule, "__logger_options", JS_NewInt64(context, reinterpret_cast<size_t>(loggerOptions)));
        JS_SetPropertyStr(context, loggerModule, "operation", loggerModule_operation);
        JS_SetPropertyStr(context, loggerModule, "info", loggerModule_info);
        JS_SetPropertyStr(context, loggerModule, "failed", loggerModule_failed);
        JS_SetPropertyStr(context, loggerModule, "succeed", loggerModule_succeed);
        JS_SetPropertyStr(context, global, "logger", loggerModule);

        JS_FreeValue(context, global);
    }
}