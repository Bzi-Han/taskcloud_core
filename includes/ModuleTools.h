#ifndef MODULE_TOOLS_H // !MODULE_TOOLS_H
#define MODULE_TOOLS_H

#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <functional>
#include <iostream>

namespace ModuleTools
{
    namespace Detail
    {
        enum class LogType : uint8_t
        {
            operation,
            info,
            failed,
            succeed,
        };

        void defaultLogger(LogType logType, const std::string_view &message, void *userData);

        std::vector<std::string> luaArguments(lua_State *luaState, int index = 1);

        std::vector<std::string> pythonArguments(const pybind11::args &args);

        std::vector<std::string> javascriptArguments(JSContext *context, int argc, JSValueConst *argv);
    }

    namespace Logger
    {
        void operation(const std::vector<std::string> &items, void *userData);
        void info(const std::vector<std::string> &items, void *userData);
        void failed(const std::vector<std::string> &items, void *userData);
        void succeed(const std::vector<std::string> &items, void *userData);
    }

    namespace Bindings
    {
        namespace Logger
        {
            void luaLogger(lua_State *luaState);

            void pythonLogger(pybind11::args args);

            JSValue javascriptLogger(Detail::LogType logType, JSContext *context, JSValueConst this_val, int argc, JSValueConst *argv);
        }
    }

    using log_t = Detail::LogType;

    extern std::function<void(Detail::LogType, const std::string_view &, void *)> logger;

    void bind(lua_State *luaState, void *loggerOptions = nullptr);

    void bind(PyObject *mainModule, void *loggerOptions = nullptr);

    void bind(JSContext *context, void *loggerOptions = nullptr);
}

#endif // !MODULE_TOOLS_H