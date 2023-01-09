#ifndef MODULE_REQUESTS_H // !MODULE_REQUESTS_H
#define MODULE_REQUESTS_H

#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>
#include <quickjsbind.h>
#include <curl/include/curl/curl.h>

#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>

namespace ModuleRequests
{
    namespace Detail
    {
        using headers_t = std::unordered_map<std::string, std::string>;

        struct RequestResult
        {
            bool success;
            std::string errorMessage;
            uint32_t code;
            headers_t headers;
            std::string content;
        };

        RequestResult request(CURL *curl, const std::string &url, const std::string &data, bool isJson, const headers_t &headers, const std::string &proxy, bool redirect, size_t timeout);

        std::unordered_map<std::string, std::string> luaTableToMap(lua_State *luaState, int index);
        std::string luaTableToJson(lua_State *luaState, int index);

        std::unordered_map<std::string, std::string> pythonDictToMap(const pybind11::dict &dict);
        std::string pythonDictToJson(const pybind11::dict &dict);
        std::string pythonListToJson(const pybind11::list &list);

        std::unordered_map<std::string, std::string> quickjsObjectToMap(const quickjs::value<JSValue> &object);
        std::string quickjsObjectToJson(const quickjs::value<JSValue> &object);
    }

    namespace Bindings
    {
        luabridge::LuaRef luaGet(lua_State *luaState);
        luabridge::LuaRef luaPost(lua_State *luaState);
        luabridge::LuaRef luaPut(lua_State *luaState);
        luabridge::LuaRef luaDelete(lua_State *luaState);

        pybind11::dict pyGet(pybind11::args args);
        pybind11::dict pyPost(pybind11::args args);
        pybind11::dict pyPut(pybind11::args args);
        pybind11::dict pyDelete(pybind11::args args);

        JSValue jsGet(quickjs::args args);
        JSValue jsPost(quickjs::args args);
        JSValue jsPut(quickjs::args args);
        JSValue jsDelete(quickjs::args args);
    }

    void bind(lua_State *luaState);

    void bind(PyObject *mainModule);

    void bind(JSContext *context);

    Detail::RequestResult get(const std::string &url, const Detail::headers_t &headers = {}, const std::string &proxy = "", bool redirect = true, size_t timeout = 100000);

    Detail::RequestResult post(const std::string &url, const std::string &data, bool isJson = false, const Detail::headers_t &headers = {}, const std::string &proxy = "", bool redirect = true, size_t timeout = 100000);

    Detail::RequestResult put(const std::string &url, const std::string &data, bool isJson = false, const Detail::headers_t &headers = {}, const std::string &proxy = "", bool redirect = true, size_t timeout = 100000);

    Detail::RequestResult delete_(const std::string &url, const Detail::headers_t &headers = {}, const std::string &proxy = "", bool redirect = true, size_t timeout = 100000);
}

#endif // !MODULE_REQUESTS_H