#include "NetworkService.h"

using self = NetworkService;

self::NetworkService(const char *host, uint16_t port)
    : m_service({host, port}),
      m_eventHandlers()
{
    m_service.set_option(yasio::inet::YOPT_C_UNPACK_PARAMS, 10 * 1024 * 1024, -1, 4, 0);
    m_service.set_option(yasio::inet::YOPT_S_NO_DISPATCH, 0);
    m_service.open(0, yasio::YCK_TCP_SERVER);

    m_service.start(
        [this](yasio::event_ptr &&ev)
        {
            switch (ev->kind())
            {
            case yasio::YEK_ON_OPEN:
                break;
            case yasio::YEK_ON_CLOSE:
                if (m_clientInfos.contains(ev->source_id()))
                    m_clientInfos.erase(ev->source_id());
                break;
            case yasio::YEK_ON_PACKET:
                if (!isNormalPacket(ev))
                    break; // not a normal packet, ignore it

                g_threadPool.addRunable(&NetworkService::dataHandler, this, ev->source_id(), ev->transport(), ev->packet());
                break;
            };
        });
}

self::~NetworkService()
{
    m_service.stop();
    m_service.close(0);
}

int self::backward(uint32_t clientId, yasio::packet_t &&packet)
{
    if (!m_clientInfos.contains(clientId))
        return -1;

    auto &client = m_clientInfos[clientId];
    if (!client.handshaked)
        return -1;

    return m_service.write(client.transportHandle, packet);
}

bool self::isNormalPacket(const yasio::event_ptr &ev)
{
    if (m_clientInfos.contains(ev->source_id()))
        return m_clientInfos[ev->source_id()].handshaked; // permit only handshaked clients

    if (5 > ev->packet().size())
        return false; // too short

    yasio::ibstream_view ibs(ev->packet());

    auto packetSize = ibs.read<uint32_t>();
    if (packetSize != ev->packet().size())
        return false; // packet size is not correct

    // check if the packet is a handshake packet
    return command_t::handshake == ibs.read<command_t>();
}

void self::dataHandler(uint32_t transportId, yasio::transport_handle_t transportHandle, yasio::packet_t &packet)
{
    yasio::ibstream_view ibs(packet);

    ibs.seek(4, SEEK_SET);
    auto command = ibs.read<command_t>();
    // check if the command is valid
    if (static_cast<uint32_t>(command) >= static_cast<uint32_t>(command_t::__max))
        return;

    // internal processing
    switch (command)
    {
    case command_t::handshake:
        yasio::obstream obs;

        auto packetSize = obs.push<uint32_t>();
        obs.write_byte(static_cast<uint8_t>(command_t::handshake));

        if (g_serviceKey == ibs.read_v32())
        {
            m_clientInfos[transportId].handshaked = true;
            m_clientInfos[transportId].transportHandle = transportHandle;

            obs.write_byte(true);
        }
        else
            obs.write_byte(false);

        obs.pop<uint32_t>(packetSize);

        m_service.write(transportHandle, std::move(obs.buffer()));
        return;
    }

    // check if the client is not handshaked
    if (!m_clientInfos.contains(transportId) || !m_clientInfos[transportId].handshaked)
        return;

    // event callback
    for (auto it = m_eventCallbacks.equal_range(command); it.first != it.second; it.first++)
    {
        ibs.seek(5, SEEK_SET);
        it.first->second(transportId, ibs);
    }
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        m_eventCallbacks.erase(command);
    }

    // event handler
    for (auto it = m_eventHandlers.equal_range(command); it.first != it.second; it.first++)
    {
        ibs.seek(5, SEEK_SET);
        it.first->second(transportId, ibs);
    }
}