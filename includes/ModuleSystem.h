#ifndef MODULE_SYSTEM_H // !MODULE_SYSTEM_H
#define MODULE_SYSTEM_H

#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>
#include <quickjsbind.h>

#include <thread>

namespace ModuleSystem
{
    void bind(lua_State *luaState);

    void bind(PyObject *mainModule);

    void bind(JSContext *context);

    void delay(size_t milliseconds);
}

#endif // !MODULE_SYSTEM_H