#ifndef NETWORK_SERVICE_H // !NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include "global.h"

#include <yasio/yasio/yasio.hpp>
#include <yasio/yasio/ibstream.hpp>
#include <yasio/yasio/obstream.hpp>

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class NetworkService
{
private:
    enum class Command : uint8_t
    {
        handshake,
        run,
        stop,
        status,
        log,
        result,
        __max,
    };

    struct ClientInfo
    {
        bool handshaked;
        yasio::transport_handle_t transportHandle;
    };

public:
    using client_t = ClientInfo;
    using command_t = Command;

    using event_callback_t = std::function<void(uint32_t clientId, yasio::ibstream_view &)>;
    using event_handler_t = event_callback_t;

public:
    NetworkService(const char *host, uint16_t port);
    ~NetworkService();

    int backward(uint32_t clientId, yasio::packet_t &&packet);

    auto addEventCallback(command_t command, event_callback_t &&callback)
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        return m_eventCallbacks.emplace(command, std::move(callback));
    }

    template <typename iterator_t>
    void removeEventCallback(iterator_t callbackIt)
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        m_eventCallbacks.erase(callbackIt);
    }

    auto addEventHandler(command_t command, event_handler_t &&handler)
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        return m_eventHandlers.emplace(command, std::move(handler));
    }

    template <typename iterator_t>
    void removeEventHandler(iterator_t handlerIt)
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        m_eventHandlers.erase(handlerIt);
    }

private:
    bool isNormalPacket(const yasio::event_ptr &ev);

    void dataHandler(uint32_t transportId, yasio::transport_handle_t transportHandle, yasio::packet_t &packet);

private:
    std::mutex m_mutex;
    yasio::io_service m_service;

    std::unordered_map<uint32_t, client_t> m_clientInfos;
    std::unordered_multimap<command_t, event_callback_t> m_eventCallbacks;
    std::unordered_multimap<command_t, event_handler_t> m_eventHandlers;
};

#endif // !NETWORK_SERVICE_H