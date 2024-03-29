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

        // do not verify the peer
        if (std::string::npos != url.find("https"))
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

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
        auto status = curl_easy_perform(curl);
        result.success = CURLE_OK == status;
        if (result.success)
            result.errorMessage = curl_easy_strerror(status);

        // get response code
        curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &result.code);
        curl_slist_free_all(sendHeaders);

        return result;
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
            2 <= paramsCount ? common::luaTableToMap(luaState, 2) : std::unordered_map<std::string, std::string>{},
            3 <= paramsCount ? lua_tostring(luaState, 3) : "",
            4 <= paramsCount ? lua_toboolean(luaState, 4) : true,
            5 <= paramsCount ? lua_tointeger(luaState, 5) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
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

        auto isDataJson = LUA_TTABLE == lua_type(luaState, 2);
        auto response = post(
            lua_tostring(luaState, 1),
            isDataJson ? common::luaTableToJson(luaState, 2) : lua_tostring(luaState, 2),
            isDataJson,
            3 <= paramsCount ? common::luaTableToMap(luaState, 3) : std::unordered_map<std::string, std::string>{},
            4 <= paramsCount ? lua_tostring(luaState, 4) : "",
            5 <= paramsCount ? lua_toboolean(luaState, 5) : true,
            6 <= paramsCount ? lua_tointeger(luaState, 6) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
        result["code"] = response.code;
        result["headers"] = luabridge::newTable(luaState);
        for (auto header : response.headers)
            result["headers"][header.first] = header.second;
        result["content"] = response.content;

        return result;
    }

    luabridge::LuaRef luaPut(lua_State *luaState)
    {
        auto paramsCount = lua_gettop(luaState);
        if (2 > paramsCount)
            luaL_error(luaState, "requests.put(...){...} ==> requires 2 parameters of url and data");
        if (2 < paramsCount && LUA_TTABLE != lua_type(luaState, 3))
            luaL_error(luaState, "requests.put(...){...} ==> the 3 parameter \"headers\" must a table");

        auto isDataJson = LUA_TTABLE == lua_type(luaState, 2);
        auto response = put(
            lua_tostring(luaState, 1),
            isDataJson ? common::luaTableToJson(luaState, 2) : lua_tostring(luaState, 2),
            isDataJson,
            3 <= paramsCount ? common::luaTableToMap(luaState, 3) : std::unordered_map<std::string, std::string>{},
            4 <= paramsCount ? lua_tostring(luaState, 4) : "",
            5 <= paramsCount ? lua_toboolean(luaState, 5) : true,
            6 <= paramsCount ? lua_tointeger(luaState, 6) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
        result["code"] = response.code;
        result["headers"] = luabridge::newTable(luaState);
        for (auto header : response.headers)
            result["headers"][header.first] = header.second;
        result["content"] = response.content;

        return result;
    }

    luabridge::LuaRef luaDelete(lua_State *luaState)
    {
        auto paramsCount = lua_gettop(luaState);
        if (1 > paramsCount)
            luaL_error(luaState, "requests.delete(...){...} ==> requires 1 parameter of url");
        if (1 < paramsCount && LUA_TTABLE != lua_type(luaState, 2))
            luaL_error(luaState, "requests.delete(...){...} ==> the 2 parameter \"headers\" must a table");

        auto response = delete_(
            lua_tostring(luaState, 1),
            2 <= paramsCount ? common::luaTableToMap(luaState, 2) : std::unordered_map<std::string, std::string>{},
            3 <= paramsCount ? lua_tostring(luaState, 3) : "",
            4 <= paramsCount ? lua_toboolean(luaState, 4) : true,
            5 <= paramsCount ? lua_tointeger(luaState, 5) : 100000);

        luabridge::LuaRef result(luaState, luabridge::newTable(luaState));
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
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
            2 <= args.size() ? common::pythonDictToMap(args[1].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            3 <= args.size() ? args[2].cast<std::string>() : "",
            4 <= args.size() ? args[3].cast<bool>() : true,
            5 <= args.size() ? args[4].cast<int>() : 100000);

        pybind11::dict result;
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
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
            isDataJson ? common::pythonDictToJson(args[1].cast<pybind11::dict>()) : PyUnicode_AsUTF8(args[1].ptr()),
            isDataJson,
            3 <= args.size() ? common::pythonDictToMap(args[2].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            4 <= args.size() ? args[3].cast<std::string>() : "",
            5 <= args.size() ? args[4].cast<bool>() : true,
            6 <= args.size() ? args[5].cast<int>() : 100000);

        pybind11::dict result;
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
        result["code"] = response.code;
        result["headers"] = pybind11::dict();
        for (auto header : response.headers)
            result["headers"][header.first.c_str()] = header.second;
        result["content"] = response.content;

        return result;
    }

    pybind11::dict pyPut(pybind11::args args)
    {
        if (2 > args.size())
            throw std::runtime_error("requests.put(...){...} ==> requires 2 parameters of url and data");
        if (2 < args.size() && !PyDict_Check(args[2].ptr()))
            throw std::runtime_error("requests.put(...){...} ==> the 3 parameter \"headers\" must a dict");

        auto isDataJson = PyDict_Check(args[1].ptr());
        auto response = put(
            args[0].cast<std::string>(),
            isDataJson ? common::pythonDictToJson(args[1].cast<pybind11::dict>()) : PyUnicode_AsUTF8(args[1].ptr()),
            isDataJson,
            3 <= args.size() ? common::pythonDictToMap(args[2].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            4 <= args.size() ? args[3].cast<std::string>() : "",
            5 <= args.size() ? args[4].cast<bool>() : true,
            6 <= args.size() ? args[5].cast<int>() : 100000);

        pybind11::dict result;
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
        result["code"] = response.code;
        result["headers"] = pybind11::dict();
        for (auto header : response.headers)
            result["headers"][header.first.c_str()] = header.second;
        result["content"] = response.content;

        return result;
    }

    pybind11::dict pyDelete(pybind11::args args)
    {
        if (1 > args.size())
            throw std::runtime_error("requests.delete(...){...} ==> requires 1 parameter of url");
        if (1 < args.size() && !PyDict_Check(args[1].ptr()))
            throw std::runtime_error("requests.delete(...){...} ==> the 2 parameter \"headers\" must a dict");

        auto response = delete_(
            args[0].cast<std::string>(),
            2 <= args.size() ? common::pythonDictToMap(args[1].cast<pybind11::dict>()) : std::unordered_map<std::string, std::string>{},
            3 <= args.size() ? args[2].cast<std::string>() : "",
            4 <= args.size() ? args[3].cast<bool>() : true,
            5 <= args.size() ? args[4].cast<int>() : 100000);

        pybind11::dict result;
        result["success"] = response.success;
        result["errorMessage"] = response.errorMessage;
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
            2 <= args.size() ? common::quickjsObjectToMap(args[1]) : std::unordered_map<std::string, std::string>{},
            3 <= args.size() ? args[2].cast<std::string>() : "",
            4 <= args.size() ? args[3].cast<bool>() : true,
            5 <= args.size() ? args[4].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("success", response.success);
        result.setProperty("errorMessage", response.errorMessage);
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
            isDataJson ? common::quickjsObjectToJson(args[1]) : args[1].cast<std::string>(),
            isDataJson,
            3 <= args.size() ? common::quickjsObjectToMap(args[2]) : std::unordered_map<std::string, std::string>{},
            4 <= args.size() ? args[3].cast<std::string>() : "",
            5 <= args.size() ? args[4].cast<bool>() : true,
            6 <= args.size() ? args[5].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("success", response.success);
        result.setProperty("errorMessage", response.errorMessage);
        result.setProperty("code", response.code);
        result.addObject("headers", headers);
        result.setProperty("content", response.content);

        return result;
    }

    JSValue jsPut(quickjs::args args)
    {
        if (2 > args.size())
            return JS_ThrowSyntaxError(args, "requests.put(...){...} ==> requires 2 parameters of url and data");
        if (2 < args.size() && !args[2].isObject())
            return JS_ThrowSyntaxError(args, "requests.put(...){...} ==> the 3 parameter \"headers\" must an object");

        auto isDataJson = args[1].isObject();
        auto response = put(
            args[0].cast<std::string>(),
            isDataJson ? common::quickjsObjectToJson(args[1]) : args[1].cast<std::string>(),
            isDataJson,
            3 <= args.size() ? common::quickjsObjectToMap(args[2]) : std::unordered_map<std::string, std::string>{},
            4 <= args.size() ? args[3].cast<std::string>() : "",
            5 <= args.size() ? args[4].cast<bool>() : true,
            6 <= args.size() ? args[5].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("success", response.success);
        result.setProperty("errorMessage", response.errorMessage);
        result.setProperty("code", response.code);
        result.addObject("headers", headers);
        result.setProperty("content", response.content);

        return result;
    }

    JSValue jsDelete(quickjs::args args)
    {
        if (1 > args.size())
            return JS_ThrowSyntaxError(args, "requests.delete(...){...} ==> requires 1 parameter of url");
        if (1 < args.size() && !args[1].isObject())
            return JS_ThrowSyntaxError(args, "requests.delete(...){...} ==> the 2 parameter \"headers\" must an object");

        auto response = delete_(
            args[0].cast<std::string>(),
            2 <= args.size() ? common::quickjsObjectToMap(args[1]) : std::unordered_map<std::string, std::string>{},
            3 <= args.size() ? args[2].cast<std::string>() : "",
            4 <= args.size() ? args[3].cast<bool>() : true,
            5 <= args.size() ? args[4].cast<int>() : 100000);

        auto result = quickjs::object(args);
        auto headers = quickjs::object(args);
        for (auto header : response.headers)
            headers.setProperty(header.first.c_str(), header.second);

        result.setProperty("success", response.success);
        result.setProperty("errorMessage", response.errorMessage);
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
            .addFunction("put", &Bindings::luaPut)
            .addFunction("delete", &Bindings::luaDelete)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto requestModule = module.def_submodule("requests");
        requestModule.def("get", &Bindings::pyGet);
        requestModule.def("post", &Bindings::pyPost);
        requestModule.def("put", &Bindings::pyPut);
        requestModule.def("delete", &Bindings::pyDelete);
    }

    void bind(JSContext *context)
    {
        auto requestModule = quickjs::object(context);

        requestModule.addFunction<Bindings::jsGet>("get");
        requestModule.addFunction<Bindings::jsPost>("post");
        requestModule.addFunction<Bindings::jsPut>("put");
        requestModule.addFunction<Bindings::jsDelete>("delete");

        quickjs::object::getGlobal(context).addObject("requests", requestModule);
    }

    Detail::RequestResult get(const std::string &url, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        auto result = Detail::request(curl, url, "", false, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }

    Detail::RequestResult post(const std::string &url, const std::string &data, bool isJson, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        auto result = Detail::request(curl, url, data, isJson, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }

    Detail::RequestResult put(const std::string &url, const std::string &data, bool isJson, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        auto result = Detail::request(curl, url, data, isJson, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }

    Detail::RequestResult delete_(const std::string &url, const Detail::headers_t &headers, const std::string &proxy, bool redirect, size_t timeout)
    {
        CURL *curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        auto result = Detail::request(curl, url, "", false, headers, proxy, redirect, timeout);

        curl_easy_cleanup(curl);

        return result;
    }
}