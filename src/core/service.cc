#include "service.h"

NetworkService g_service(g_serviceAddress, g_servicePort);

namespace Service::Detail
{
    bool lua(
        uint32_t clientId,
        uint64_t userId,
        uint64_t taskId,
        const std::string &name,
        const std::string &script,
        const std::string &passport,
        const std::string &callMethods,
        std::promise<uint64_t> *runnerId)
    {
        bool result = false;
        TaskRunInfo runInfo{clientId, userId, taskId, TaskRunStatus::waiting, name};

        // create lua vm
        lua_State *luaState = luaL_newstate();
        if (nullptr == luaState)
        {
            ModuleTools::Logger::failed({"create lua vm failed"}, &runInfo);
            runnerId->set_value(0);

            return result;
        }
        taskRunInfo.emplace(reinterpret_cast<uint64_t>(luaState), &runInfo);

        // construction finally block
        finally
        {
            yasio::obstream obs;

            runInfo.status = TaskRunStatus::finished;

            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::result));
            obs.write<uint64_t>(userId);
            obs.write<uint64_t>(taskId);
            obs.write_byte(result);
            obs.write<uint64_t>(reinterpret_cast<uint64_t>(luaState));
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));

            lua_close(luaState);
            taskRunInfo.erase(reinterpret_cast<uint64_t>(luaState));
        };

        // initialization environment
        luaL_openlibs(luaState);

        // register modules
        ModuleCrypto::bind(luaState);
        ModuleJson::bind(luaState);
        ModuleRequests::bind(luaState);
        ModuleTools::bind(luaState, &runInfo);
        ModuleSystem::bind(luaState);

        // notify runnerId
        runnerId->set_value(reinterpret_cast<uint64_t>(luaState));

        // load sciprt environment
        if (LUA_OK != luaL_dostring(luaState, script.c_str()))
        {
            ModuleTools::Logger::failed({lua_tostring(luaState, -1)}, &runInfo);

            return result;
        }

        runInfo.status = TaskRunStatus::running;
        auto methods = stringSplitAscii(callMethods, ",");
        for (size_t i = 0; i < methods.size() + 1; i++)
        {
            // get target method
            lua_getglobal(luaState, 0 == i ? "setTaskPassport" : methods[i - 1].c_str());
            // check target value if is not a function
            if (!lua_isfunction(luaState, -1))
            {
                ModuleTools::Logger::failed(
                    {
                        0 == i ? "setTaskPassport" : methods[i - 1],
                        "is not a function",
                    },
                    &runInfo);

                return result;
            }

            // call target method
            bool isCallSucceed = false;
            if (0 == i)
            {
                lua_pushstring(luaState, passport.c_str());
                isCallSucceed = LUA_OK == lua_pcall(luaState, 1, 1, 0);
            }
            else
                isCallSucceed = LUA_OK == lua_pcall(luaState, 0, 1, 0);

            // check call result
            if (!isCallSucceed)
            {
                ModuleTools::Logger::failed({lua_tostring(luaState, -1)}, &runInfo);

                return false;
            }
            else if (0 != i)
            {
                if (!lua_isboolean(luaState, -1))
                {
                    ModuleTools::Logger::failed(
                        {
                            methods[i - 1],
                            "return value is not a boolean",
                        },
                        &runInfo);

                    return result;
                }

                result = lua_toboolean(luaState, -1);
                lua_pop(luaState, 1);
            }
        }

        return result;
    }

    bool python(
        uint32_t clientId,
        uint64_t userId,
        uint64_t taskId,
        const std::string &name,
        const std::string &script,
        const std::string &passport,
        const std::string &callMethods,
        std::promise<uint64_t> *runnerId)
    {
        bool result = false;
        TaskRunInfo runInfo{clientId, userId, taskId, TaskRunStatus::waiting, name};

        // create python vm
        Py_Initialize();
        PyObject *mainModule, *globalDict;
        mainModule = PyImport_AddModule("__main__");
        if (nullptr == mainModule)
        {
            ModuleTools::Logger::failed({"create python vm failed"}, &runInfo);
            runnerId->set_value(0);

            return result;
        }
        globalDict = PyModule_GetDict(mainModule);
        if (nullptr == globalDict)
        {
            ModuleTools::Logger::failed({"get python structure failed"}, &runInfo);
            runnerId->set_value(0);

            return result;
        }
        taskRunInfo.emplace(reinterpret_cast<uint64_t>(mainModule), &runInfo);

        // construction finally block
        finally
        {
            yasio::obstream obs;

            runInfo.status = TaskRunStatus::finished;

            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::result));
            obs.write<uint64_t>(userId);
            obs.write<uint64_t>(taskId);
            obs.write_byte(result);
            obs.write<uint64_t>(reinterpret_cast<uint64_t>(mainModule));
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));

            Py_Finalize();
            taskRunInfo.erase(reinterpret_cast<uint64_t>(mainModule));
        };

        // register modules
        ModuleCrypto::bind(mainModule);
        ModuleJson::bind(mainModule);
        ModuleRequests::bind(mainModule);
        ModuleTools::bind(mainModule, &runInfo);
        ModuleSystem::bind(mainModule);

        // notify runnerId
        runnerId->set_value(reinterpret_cast<uint64_t>(mainModule));

        try
        {
            // load sciprt environment
            if (nullptr == PyRun_String(script.c_str(), Py_file_input, globalDict, globalDict))
            {
                ModuleTools::Logger::failed({pybind11::cast<std::string>(pybind11::error_scope().value)}, &runInfo);

                return result;
            }

            runInfo.status = TaskRunStatus::running;
            auto methods = stringSplitAscii(callMethods, ",");

            for (int i = 0; i < methods.size() + 1; i++)
            {
                if (0 == i)
                    pybind11::getattr(mainModule, "setTaskPassport")(passport);
                else
                {
                    auto callResult = pybind11::getattr(mainModule, methods[i - 1].c_str())();

                    result = pybind11::cast<bool>(callResult);
                }
            }
        }
        catch (const std::exception &e)
        {
            ModuleTools::Logger::failed({e.what()}, &runInfo);

            return (result = false);
        }

        return true;
    }

    bool javascript(
        uint32_t clientId,
        uint64_t userId,
        uint64_t taskId,
        const std::string &name,
        const std::string &script,
        const std::string &passport,
        const std::string &callMethods,
        std::promise<uint64_t> *runnerId)
    {
        bool result = false;
        TaskRunInfo runInfo{clientId, userId, taskId, TaskRunStatus::waiting, name};

        // create javascript vm
        auto runtime = JS_NewRuntime();
        if (nullptr == runtime)
        {
            ModuleTools::Logger::failed({"create javascript vm failed"}, &runInfo);
            runnerId->set_value(0);

            return result;
        }
        auto context = JS_NewContext(runtime);
        if (nullptr == context)
        {
            ModuleTools::Logger::failed({"create javascript context failed"}, &runInfo);
            runnerId->set_value(0);

            return result;
        }
        taskRunInfo.emplace(reinterpret_cast<uint64_t>(context), &runInfo);

        // construction finally block
        finally
        {
            yasio::obstream obs;

            runInfo.status = TaskRunStatus::finished;

            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::result));
            obs.write<uint64_t>(userId);
            obs.write<uint64_t>(taskId);
            obs.write_byte(result);
            obs.write<uint64_t>(reinterpret_cast<uint64_t>(context));
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));

            JS_FreeContext(context);
            JS_FreeRuntime(runtime);
            taskRunInfo.erase(reinterpret_cast<uint64_t>(context));
        };

        // register modules
        ModuleCrypto::bind(context);
        ModuleJson::bind(context);
        ModuleRequests::bind(context);
        ModuleTools::bind(context, &runInfo);
        ModuleSystem::bind(context);

        // notify runnerId
        runnerId->set_value(reinterpret_cast<uint64_t>(context));

        // load sciprt environment
        auto loadResult = JS_Eval(context, script.c_str(), script.size(), "<input>", JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(loadResult))
        {
            auto exception = JS_GetException(context);
            ModuleTools::Logger::failed({JS_ToCString(context, exception)}, &runInfo);
            JS_FreeValue(context, exception);

            return result;
        }

        runInfo.status = TaskRunStatus::running;
        auto methods = stringSplitAscii(callMethods, ",");
        auto globalThis = quickjs::object::getGlobal(context);
        for (size_t i = 0; i < methods.size() + 1; i++)
        {
            auto targetFunction = JS_GetPropertyStr(context, globalThis, 0 == i ? "setTaskPassport" : methods[i - 1].c_str());
            if (!JS_IsFunction(context, targetFunction))
            {
                ModuleTools::Logger::failed(
                    {
                        0 == i ? "setTaskPassport" : methods[i - 1],
                        "is not a function",
                    },
                    &runInfo);

                return result;
            }

            // call target function
            JSValue callResult = JS_UNDEFINED;
            if (0 == i)
            {
                auto args = JS_NewString(context, passport.c_str());
                callResult = JS_Call(context, targetFunction, globalThis, 1, &args);
            }
            else
                callResult = JS_Call(context, targetFunction, globalThis, 0, nullptr);
            JS_FreeValue(context, targetFunction);

            // check call result
            if (JS_IsException(callResult))
            {
                auto exception = JS_GetException(context);
                ModuleTools::Logger::failed({JS_ToCString(context, exception)}, &runInfo);
                JS_FreeValue(context, exception);

                return result;
            }
            else if (0 != i)
            {
                if (!JS_IsBool(callResult))
                {
                    ModuleTools::Logger::failed(
                        {
                            methods[i - 1],
                            "return value is not a boolean",
                        },
                        &runInfo);

                    return result;
                }

                result = JS_ToBool(context, callResult);
            }
        }

        // execute remaining jobs
        js_std_loop(context);

        return true;
    }

    std::vector<std::string> stringSplitAscii(const std::string_view &str, const std::string_view &delimiter)
    {
        std::vector<std::string> result;

        size_t pos = 0, lastPos = 0;
        while (std::string::npos != (pos = str.find(delimiter, lastPos)))
        {
            result.emplace_back(str.data() + lastPos, pos - lastPos);
            lastPos = pos + delimiter.length();
        }

        result.emplace_back(str.data() + lastPos, str.length() - lastPos);

        return result;
    }

    std::vector<std::string_view> stringSplitAsciiView(const std::string_view &str, const std::string_view &delimiter)
    {
        std::vector<std::string_view> result;

        size_t pos = 0, lastPos = 0;
        while (std::string::npos != (pos = str.find(delimiter, lastPos)))
        {
            result.emplace_back(str.data() + lastPos, pos - lastPos);
            lastPos = pos + delimiter.length();
        }

        result.emplace_back(str.data() + lastPos, str.length() - lastPos);

        return result;
    }

}

namespace Service
{
    std::unordered_map<uint64_t, Detail::TaskRunInfo *> taskRunInfo;

    uint64_t run(
        uint32_t clientId,
        uint64_t userId,
        uint64_t taskId,
        language_t language,
        const std::string &name,
        const std::string &script,
        const std::string &passport,
        const std::string &callMethods)
    {
        std::promise<uint64_t> runnerId;

        switch (language)
        {
        case language_t::lua:
            g_threadPool.addRunable(Detail::lua, clientId, userId, taskId, name, script, passport, callMethods, &runnerId);
            break;
        case language_t::python:
            g_threadPool.addRunable(Detail::python, clientId, userId, taskId, name, script, passport, callMethods, &runnerId);
            break;
        case language_t::javascript:
            g_threadPool.addRunable(Detail::javascript, clientId, userId, taskId, name, script, passport, callMethods, &runnerId);
            break;
        }

        return runnerId.get_future().get();
    }

    bool stop(uint64_t runnerId)
    {
        // !TODO: update thread-pool to support force stop a thread and
        //         auto-new a thread when the pool's size is not enough
        return false;
    }

    task_run_status_t status(uint64_t runnerId)
    {
        if (!taskRunInfo.contains(runnerId))
            return task_run_status_t::none;

        return taskRunInfo[runnerId]->status;
    }

    void join()
    {
        while (!taskRunInfo.empty())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

}