#include "ModuleRequests.h"

namespace ModuleRequests::Detail
{
    RequestResult request(CURL *curl, const std::string &url, const std::string &data, bool isJson, const headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        curl_slist *sendHeaders = nullptr;

        // generate headers
        for (auto header : headers)
            sendHeaders = curl_slist_append(sendHeaders, (header.first + ": " + header.second).c_str());
        if (headers.end() == headers.find("Accept"))
            sendHeaders = curl_slist_append(sendHeaders, "Accept: */*");
        if (headers.end() == headers.find("Accept-Language"))
            sendHeaders = curl_slist_append(sendHeaders, "Accept-Language: zh-cn");
        if (!data.empty())
        {
            if (headers.end() == headers.find("Content-Type"))
            {
                if (isJson)
                    sendHeaders = curl_slist_append(sendHeaders, "Content-Type: application/json");
                else
                    sendHeaders = curl_slist_append(sendHeaders, "Content-Type :application/x-www-form-urlencoded");
            }
        }
        if (headers.end() == headers.find("User-Agent"))
            sendHeaders = curl_slist_append(sendHeaders, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Safari/537.36");

        // set redirect
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, redirect);
        curl_easy_setopt(curl, CURLOPT_AUTOREFERER, redirect);

        // set url
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // set headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, sendHeaders);
        // set request body
        if (!data.empty())
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        // set proxy
        if (!proxy.empty())
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
        // set timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);

        // set response body callback
        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            +[](char *data, size_t elementSize, size_t elementCount, std::string *userData)
            {
                auto readedSize = elementSize * elementCount;

                if (readedSize > 0)
                    userData->append(data, readedSize);

                return readedSize;
            });

        // set response header callback
        curl_easy_setopt(
            curl,
            CURLOPT_HEADERFUNCTION,
            +[](char *data, size_t elementSize, size_t elementCount, headers_t *userData)
            {
                auto readedSize = elementSize * elementCount;

                if (readedSize > 2)
                {
                    std::string_view header(data, readedSize - 2);

                    auto splitPos = header.find(": ");
                    if (std::string_view::npos == splitPos)
                        return readedSize;

                    auto key = header.substr(0, splitPos);
                    auto value = header.substr(splitPos + 2);

                    if (std::string_view::npos == header.find("Set-Cookie"))
                        userData->emplace(std::string{key.data(), key.size()}, std::string{value.data(), value.size()});
                    else
                    {
                        if (userData->contains("Set-Cookie"))
                            userData->at("Set-Cookie") += "; " + std::string(value.data(), value.size());
                        else
                            userData->emplace("Set-Cookie", std::string{value.data(), value.size()});
                    }
                }

                return readedSize;
            });

        // set response user data
        RequestResult result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.content);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result.headers);

        // perform request
        curl_easy_perform(curl);

        // get response code
        curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &result.code);
        curl_slist_free_all(sendHeaders);

        return result;
    }

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

namespace ModuleRequests::Bindings
{
    luabridge::LuaRef luaGet(lua_State *luaState)
    {
        auto paramsCount = lua_gettop(luaState);
        if (1 > paramsCount)
            luaL_error(luaState, "requests.get(...){...} ==> requires 1 parameter of url");
        if (1 < paramsCount && LUA_TTABLE != lua_type(luaState, 2))
            luaL_error(luaState, "requests.get(...){...} ==> the 2 parameter \"headers\" must a table");

        auto response = get(
            lua_tostring(luaState, 1),
            2 < paramsCount ? Detail::luaTableToMap(luaState, 2) : std::unordered_map<std::string, std::string>{},
            3 < paramsCount ? lua_tostring(luaState, 3) : "",
            4 < paramsCount ? lua_toboolean(luaState, 4) : true,
            5 < paramsCount ? lua_tointeger(luaState, 5) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["code"] = response.code;
        result["headers"] = luabridge::newTable(luaState);
        for (auto header : response.headers)
            result["headers"][header.first] = header.second;
        result["content"] = response.content;

        return result;
    }

    luabridge::LuaRef luaPost(lua_State *luaState)
    {
        auto paramsCount = lua_gettop(luaState);
        if (2 > paramsCount)
            luaL_error(luaState, "requests.post(...){...} ==> requires 2 parameters of url and data");
        if (2 < paramsCount && LUA_TTABLE != lua_type(luaState, 3))
            luaL_error(luaState, "requests.post(...){...} ==> the 3 parameter \"headers\" must a table");

        auto isDataJson = LUA_TSTRING == lua_type(luaState, 2);
        auto response = post(
            lua_tostring(luaState, 1),
            isDataJson ? lua_tostring(luaState, 2) : Detail::luaTableToJson(luaState, 2),
            isDataJson,
            3 < paramsCount ? Detail::luaTableToMap(luaState, 3) : std::unordered_map<std::string, std::string>{},
            4 < paramsCount ? lua_tostring(luaState, 4) : "",
            5 < paramsCount ? lua_toboolean(luaState, 5) : true,
            6 < paramsCount ? lua_tointeger(luaState, 6) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["code"] = response.code;
        result["headers"] = luabridge::newTable(luaState);
        for (auto header : response.headers)
            result["headers"][header.first] = header.second;
        result["content"] = response.content;

        return result;
    }

    pybind11::dict pyGet(pybind11::args args)
    {
        if (1 > args.size())
            throw std::runtime_error("requests.get(...){...} ==> requires 1 parameter of url");
        if (1 < args.size() && !PyDict_Check(args[1].ptr()))
            throw std::runtime_error("requests.get(...){...} ==> the 2 parameter \"headers\" must a dict");

        auto response = get(
            args[0].cast<std::string>(),
            1 < args.size() ? Detail::pythonDictToMap(args[1].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            2 < args.size() ? args[2].cast<std::string>() : "",
            3 < args.size() ? args[3].cast<bool>() : true,
            4 < args.size() ? args[4].cast<int>() : 100000);

        pybind11::dict result;
        result["code"] = response.code;
        result["headers"] = pybind11::dict();
        for (auto header : response.headers)
            result["headers"][header.first.c_str()] = header.second;
        result["content"] = response.content;

        return result;
    }

    pybind11::dict pyPost(pybind11::args args)
    {
        if (2 > args.size())
            throw std::runtime_error("requests.post(...){...} ==> requires 2 parameters of url and data");
        if (2 < args.size() && !PyDict_Check(args[2].ptr()))
            throw std::runtime_error("requests.post(...){...} ==> the 3 parameter \"headers\" must a dict");

        auto isDataJson = PyDict_Check(args[1].ptr());
        auto response = post(
            args[0].cast<std::string>(),
            isDataJson ? Detail::pythonDictToJson(args[1].cast<pybind11::dict>()) : PyUnicode_AsUTF8(args[1].ptr()),
            isDataJson,
            3 < args.size() ? Detail::pythonDictToMap(args[2].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            4 < args.size() ? args[3].cast<std::string>() : "",
            5 < args.size() ? args[4].cast<bool>() : true,
            6 < args.size() ? args[5].cast<int>() : 100000);

        pybind11::dict result;
        result["code"] = response.code;
        result["headers"] = pybind11::dict();
        for (auto header : response.headers)
            result["headers"][header.first.c_str()] = header.second;
        result["content"] = response.content;

        return result;
    }

    JSValue jsGet(quickjs::args args)
    {
        if (1 > args.size())
            return JS_ThrowSyntaxError(args, "requests.get(...){...} ==> requires 1 parameter of url");
        if (1 < args.size() && !args[1].isObject())
            return JS_ThrowSyntaxError(args, "requests.get(...){...} ==> the 2 parameter \"headers\" must an object");

        auto response = get(
            args[0].cast<std::string>(),
            1 < args.size() ? Detail::quickjsObjectToMap(args[1]) : std::unordered_map<std::string, std::string>{},
            2 < args.size() ? args[2].cast<std::string>() : "",
            3 < args.size() ? args[3].cast<bool>() : true,
            4 < args.size() ? args[4].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("code", response.code);
        result.addObject("headers", headers);
        result.setProperty("content", response.content);

        return result;
    }

    JSValue jsPost(quickjs::args args)
    {
        if (2 > args.size())
            return JS_ThrowSyntaxError(args, "requests.post(...){...} ==> requires 2 parameters of url and data");
        if (2 < args.size() && !args[2].isObject())
            return JS_ThrowSyntaxError(args, "requests.post(...){...} ==> the 3 parameter \"headers\" must an object");

        auto isDataJson = args[1].isObject();
        auto response = post(
            args[0].cast<std::string>(),
            isDataJson ? Detail::quickjsObjectToJson(args[1]) : args[1].cast<std::string>(),
            isDataJson,
            3 < args.size() ? Detail::quickjsObjectToMap(args[2]) : std::unordered_map<std::string, std::string>{},
            4 < args.size() ? args[3].cast<std::string>() : "",
            5 < args.size() ? args[4].cast<bool>() : true,
            6 < args.size() ? args[5].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("code", response.code);
        result.addObject("headers", headers);
        result.setProperty("content", response.content);

        return result;
    }
}

namespace ModuleRequests
{
    void bind(lua_State *luaState)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("requests")
            .addFunction("get", &Bindings::luaGet)
            .addFunction("post", &Bindings::luaPost)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto requestModule = module.def_submodule("requests");
        requestModule.def("get", &Bindings::pyGet);
        requestModule.def("post", &Bindings::pyPost);
    }

    void bind(JSContext *context)
    {
        auto requestModule = quickjs::object(context);

        requestModule.addFunction<Bindings::jsGet>("get");
        requestModule.addFunction<Bindings::jsPost>("post");

        quickjs::object::getGlobal(context).addObject("requests", requestModule);
    }

    Detail::RequestResult get(const std::string &url, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        auto result = Detail::request(curl, url, "", false, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }

    Detail::RequestResult post(const std::string &url, const std::string &data, bool isJson, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        auto result = Detail::request(curl, url, data, isJson, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }
}