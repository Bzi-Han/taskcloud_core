#ifndef MODULE_JSON_H // !MODULE_JSON_H
#define MODULE_JSON_H

#include "common.h"

#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>
#include <quickjsbind.h>
#include <rapidjson/include/rapidjson/reader.h>
#include <rapidjson/include/rapidjson/document.h>
#include <rapidjson/include/rapidjson/error/en.h>
#include <rapidjson/include/rapidjson/stringbuffer.h>
#include <rapidjson/include/rapidjson/writer.h>

#include <stack>

namespace ModuleJson
{
    namespace Detail
    {
        class JsonReader : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, JsonReader>
        {
        public:
            JsonReader(lua_State *luaState);
            JsonReader(PyObject *mainModule);
            JsonReader(JSContext *jsContext);

            bool Default();
            bool Null();
            bool Bool(bool);
            bool Int(int);
            bool Uint(unsigned);
            bool Int64(int64_t);
            bool Uint64(uint64_t);
            bool Double(double);
            bool RawNumber(const Ch *str, rapidjson::SizeType len, bool copy);
            bool String(const Ch *, rapidjson::SizeType, bool);
            bool StartObject();
            bool Key(const Ch *str, rapidjson::SizeType len, bool copy);
            bool EndObject(rapidjson::SizeType);
            bool StartArray();
            bool EndArray(rapidjson::SizeType);

            operator luabridge::LuaRef() const
            {
                return m_luaReaderStack.top();
            }

            operator pybind11::object() const
            {
                return m_pythonReaderStack.top();
            }

            operator JSValue() const
            {
                return m_jsReaderStack.top();
            }

        private:
            enum class ReaderType
            {
                lua,
                python,
                javascript,
            };
            struct NodeInfo
            {
                bool isArray;
                size_t arraySize;
                std::string objectKey;
            };

            bool beginNode(bool isArray);

            bool endNode();

            template <typename any_t>
            bool value(any_t value);

        private:
            ReaderType m_readerType;

            lua_State *m_luaState;
            std::stack<luabridge::LuaRef> m_luaReaderStack;

            PyObject *m_mainModule;
            std::stack<pybind11::object> m_pythonReaderStack;

            JSContext *m_jsContext;
            std::stack<quickjs::object> m_jsReaderStack;

            std::stack<NodeInfo> m_readerInfoStack;
        };
    }

    namespace Bindings
    {
        luabridge::LuaRef lua_loads(const luabridge::LuaRef &data);
        std::string lua_dumps(const luabridge::LuaRef &data);

        pybind11::object python_loads(const std::string &data);
        std::string python_dumps(const pybind11::object &data);

        JSValue javascript_loads(const quickjs::value<std::string> &data);
        std::string javascript_dumps(const quickjs::value<JSValue> &data);
    }

    void bind(lua_State *luaState);

    void bind(PyObject *mainModule);

    void bind(JSContext *context);

    rapidjson::Document loads(const std::string &data);
    std::string dumps(const rapidjson::Document &data);
}

#endif // !MODULE_JSON_H