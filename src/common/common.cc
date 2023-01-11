
#include "common.h"

namespace common
{
    std::unordered_map<std::string, std::string> luaTableToMap(lua_State *luaState, int index)
    {
        std::unordered_map<std::string, std::string> result;

        lua_pushnil(luaState);
        while (lua_next(luaState, index))
        {
            int keyType = lua_type(luaState, -2);

            switch (keyType)
            {
            case LUA_TNUMBER:
                result.emplace(
                    std::to_string(lua_tointeger(luaState, -2)),
                    lua_tostring(luaState, -1));
                break;
            case LUA_TSTRING:
                result.emplace(
                    lua_tostring(luaState, -2),
                    lua_tostring(luaState, -1));
                break;
            }

            lua_pop(luaState, 1);
        }

        return result;
    }

    std::string luaTableToJson(lua_State *luaState, int index)
    {
        std::stringstream result;
        bool isFirst = true;

        auto isArray = 0 < lua_objlen(luaState, index);
        result << (isArray ? "[" : "{");

        lua_pushnil(luaState);
        while (lua_next(luaState, index))
        {
            auto valueType = lua_type(luaState, -1);

            if (!isFirst)
                result << ",";
            if (!isArray)
            {
                result << "\""
                       << lua_tostring(luaState, -2)
                       << "\":";
            }

            switch (valueType)
            {
            case LUA_TNUMBER:
            {
                auto value = lua_tonumber(luaState, -1);
                if (value != static_cast<int>(value))
                    result << value;
                else
                    result << static_cast<int>(value);

                break;
            }
            case LUA_TSTRING:
                result << "\""
                       << lua_tostring(luaState, -1)
                       << "\"";

                break;
            case LUA_TBOOLEAN:
                result << (lua_toboolean(luaState, -1) ? "true" : "false");

                break;
            case LUA_TTABLE:
                result << luaTableToJson(luaState, lua_gettop(luaState));

                break;
            }

            lua_pop(luaState, 1);
            isFirst = false;
        }

        result << (isArray ? "]" : "}");

        return result.str();
    }

    std::unordered_map<std::string, std::string> pythonDictToMap(const pybind11::dict &dict)
    {
        std::unordered_map<std::string, std::string> result;

        for (auto &&item : dict)
        {
            result.emplace(
                item.first.cast<std::string>(),
                item.second.cast<std::string>());
        }

        return result;
    }

    std::string pythonDictToJson(const pybind11::dict &dict)
    {
        std::stringstream result;
        bool isFirst = true;

        result << "{";

        for (auto &&item : dict)
        {
            if (!isFirst)
                result << ",";

            result << "\""
                   << item.first.cast<std::string>()
                   << "\":";

            if (PyFloat_Check(item.second.ptr()))
                result << PyFloat_AsDouble(item.second.ptr());
            else if (PyLong_Check(item.second.ptr()))
                result << PyLong_AsLong(item.second.ptr());
            else if (PyUnicode_Check(item.second.ptr()))
                result << "\"" << PyUnicode_AsUTF8(item.second.ptr()) << "\"";
            else if (PyDict_Check(item.second.ptr()))
                result << pythonDictToJson(item.second.cast<pybind11::dict>());
            else if (PyList_Check(item.second.ptr()))
                result << pythonListToJson(item.second.cast<pybind11::list>());
            else
                result << "\""
                       << item.second.cast<std::string>()
                       << "\"";

            isFirst = false;
        }

        result << "}";

        return result.str();
    }

    std::string pythonListToJson(const pybind11::list &list)
    {
        std::stringstream result;
        bool isFirst = true;

        result << "[";

        for (auto &&item : list)
        {
            if (!isFirst)
            {
                result << ",";
                isFirst = false;
            }

            if (PyFloat_Check(item.ptr()))
                result << PyFloat_AsDouble(item.ptr());
            else if (PyLong_Check(item.ptr()))
                result << PyLong_AsLong(item.ptr());
            else if (PyUnicode_Check(item.ptr()))
                result << PyUnicode_AsUTF8(item.ptr());
            else if (PyDict_Check(item.ptr()))
                result << pythonDictToJson(item.cast<pybind11::dict>());
            else if (PyList_Check(item.ptr()))
                result << pythonListToJson(item.cast<pybind11::list>());
            else
                result << "\""
                       << item.cast<std::string>()
                       << "\"";
        }

        result << "]";

        return result.str();
    }

    std::unordered_map<std::string, std::string> quickjsObjectToMap(const quickjs::value<JSValue> &value)
    {
        std::unordered_map<std::string, std::string> result;
        JSPropertyEnum *properties = nullptr;
        uint32_t propertyCount = 0;

        if (0 != JS_GetOwnPropertyNames(value.context, &properties, &propertyCount, value.value, JS_TAG_OBJECT))
            return result;

        for (uint32_t i = 0; i < propertyCount; ++i)
        {
            std::string key = JS_AtomToCString(value.context, properties[i].atom);

            result.emplace(
                std::move(key),
                quickjs::value<JSValue>{value.context, JS_GetPropertyStr(value.context, value.value, key.c_str())}.cast<std::string>());
        }

        return result;
    }

    std::string quickjsObjectToJson(const quickjs::value<JSValue> &object)
    {
        quickjs::value<JSValue> result{object.context, JS_JSONStringify(object.context, object.value, JS_UNDEFINED, JS_UNDEFINED)};

        return result.cast<std::string>();
    }
}