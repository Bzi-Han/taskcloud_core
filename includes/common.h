#ifndef COMMON_H // !COMMON_H
#define COMMON_H

#include <luajit/src/lua.hpp>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjsbind.h>

#include <string>
#include <sstream>
#include <unordered_map>

namespace common
{
    std::unordered_map<std::string, std::string> luaTableToMap(lua_State *luaState, int index);
    std::string luaTableToJson(lua_State *luaState, int index);

    std::unordered_map<std::string, std::string> pythonDictToMap(const pybind11::dict &dict);
    std::string pythonDictToJson(const pybind11::dict &dict);
    std::string pythonListToJson(const pybind11::list &list);

    std::unordered_map<std::string, std::string> quickjsObjectToMap(const quickjs::value<JSValue> &object);
    std::string quickjsObjectToJson(const quickjs::value<JSValue> &object);
}

#endif // !COMMON_H