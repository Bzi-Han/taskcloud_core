
#ifndef SERVICE_H // !SERVICE_H
#define SERVICE_H

#include <yasio/yasio/xxsocket.hpp>

#include "Finally.h"

#include "global.h"
#include "ModuleCrypto.h"
#include "ModuleJson.h"
#include "ModuleRequests.h"
#include "ModuleTools.h"
#include "ModuleSystem.h"
#include "NetworkService.h"

#include <yasio/yasio/obstream.hpp>
#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>
#include <quickjs-cmake/quickjs/quickjs-libc.h>

#include <string>
#include <string_view>
#include <sstream>
#include <future>
#include <unordered_map>

extern NetworkService g_service;

namespace Service
{
    namespace Detail
    {
        enum class LanguageType : uint8_t
        {
            lua,
            python,
            javascript,
        };

        enum class TaskRunStatus : uint8_t
        {
            waiting,
            running,
            finished,
            none,
        };

        struct TaskRunInfo
        {
            uint32_t clientId;
            uint64_t userId;
            uint64_t taskId;
            TaskRunStatus status;
            std::string taskName;
        };

        bool lua(
            uint32_t clientId,
            uint64_t userId,
            uint64_t taskId,
            const std::string &name,
            const std::string &script,
            const std::string &passport,
            const std::string &callMethods,
            std::promise<uint64_t> *runnerId);

        bool python(
            uint32_t clientId,
            uint64_t userId,
            uint64_t taskId,
            const std::string &name,
            const std::string &script,
            const std::string &passport,
            const std::string &callMethods,
            std::promise<uint64_t> *runnerId);

        bool javascript(
            uint32_t clientId,
            uint64_t userId,
            uint64_t taskId,
            const std::string &name,
            const std::string &script,
            const std::string &passport,
            const std::string &callMethods,
            std::promise<uint64_t> *runnerId);

        std::vector<std::string> stringSplitAscii(const std::string_view &str, const std::string_view &delimiter);

        std::vector<std::string_view> stringSplitAsciiView(const std::string_view &str, const std::string_view &delimiter);
    }

    using language_t = Detail::LanguageType;
    using task_run_status_t = Detail::TaskRunStatus;
    using task_run_info_t = Detail::TaskRunInfo;

    extern std::unordered_map<uint64_t, Detail::TaskRunInfo *> taskRunInfo;

    uint64_t run(
        uint32_t clientId,
        uint64_t userId,
        uint64_t taskId,
        language_t language,
        const std::string &name,
        const std::string &script,
        const std::string &passport,
        const std::string &callMethods);

    bool stop(uint64_t runnerId);

    task_run_status_t status(uint64_t runnerId);

    void join();
}

#endif // !SERVICE_H