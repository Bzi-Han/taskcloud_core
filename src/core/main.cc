#include "global.h"
#include "service.h"
#include "ModuleTools.h"

#include <yasio/yasio/obstream.hpp>

#include <iostream>

int main()
{
    std::system("chcp 65001");

    std::cout << "[=] Service running on: " << g_serviceAddress << ":" << g_servicePort << std::endl;

    ModuleTools::logger = [&](ModuleTools::log_t logType, const std::string_view &message, void *userData)
    {
        auto taskRunInfo = static_cast<Service::task_run_info_t *>(userData);

        yasio::obstream obs;
        auto packetSize = obs.push<uint32_t>();
        obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::log));
        obs.write<uint64_t>(taskRunInfo->userId);
        obs.write<uint64_t>(taskRunInfo->taskId);
        obs.write_byte(static_cast<uint8_t>(logType));
        obs.write_v32(taskRunInfo->taskName);
        obs.write_v32(message);
        obs.pop<uint32_t>(packetSize);

        g_service.backward(taskRunInfo->clientId, std::move(obs.buffer()));
    };

    g_service.addEventHandler(
        NetworkService::command_t::run,
        [&](uint32_t clientId, yasio::ibstream_view &ibs)
        {
            auto userId = ibs.read<uint64_t>();
            auto taskId = ibs.read<uint64_t>();
            auto languageType = ibs.read<Service::language_t>();
            auto name = ibs.read_v32();
            auto script = ibs.read_v32();
            auto passport = ibs.read_v32();
            auto callMethods = ibs.read_v32();
            auto runnerId = Service::run(
                clientId,
                userId,
                taskId,
                languageType,
                {name.data(), name.size()},
                {script.data(), script.size()},
                {passport.data(), passport.size()},
                {callMethods.data(), callMethods.size()});

            yasio::obstream obs;
            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::run));
            obs.write_byte(0 != runnerId);
            obs.write<uint64_t>(runnerId);
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));
        });

    g_service.addEventHandler(
        NetworkService::command_t::stop,
        [&](uint32_t clientId, yasio::ibstream_view &ibs)
        {
            auto runnerId = ibs.read<uint64_t>();

            yasio::obstream obs;
            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::stop));
            obs.write_byte(static_cast<uint8_t>(Service::stop(runnerId)));
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));
        });

    g_service.addEventHandler(
        NetworkService::command_t::status,
        [&](uint32_t clientId, yasio::ibstream_view &ibs)
        {
            auto runnerId = ibs.read<uint64_t>();

            yasio::obstream obs;
            auto packetSize = obs.push<uint32_t>();
            obs.write_byte(static_cast<uint8_t>(NetworkService::command_t::status));
            obs.write_byte(static_cast<uint8_t>(Service::status(runnerId)));
            obs.pop<uint32_t>(packetSize);

            g_service.backward(clientId, std::move(obs.buffer()));
        });

    std::string command;
    for (;;)
    {
        std::cin >> command;
        if ("exit" == command)
            break;
    }

    return 0;
}
