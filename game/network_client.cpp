//
// open horizon -- undefined_darkness@outlook.com
//

#include "network_client.h"
#include "network_helpers.h"
#include "system/system.h"
#include "miso/protocol/app_protocol_simple.h"
#include "miso/socket/ipv4.h"

#include <thread>
#include <chrono>

namespace game
{
//------------------------------------------------------------

bool network_client::connect(const char *address, short port)
{
    if (!address || m_client.is_open())
        return false;

    m_error.clear();

    const int timeout = 2000;

    static miso::app_protocol_simple protocol;
    m_client.set_app_protocol(&protocol);
    if (!m_client.connect_wait(miso::ipv4_address::create(address), port, timeout))
    {
        m_error = "Unable to connect";
        return false;
    }

    bool has_info = false;
    const int interval = 100;
    for (int i = 0; i < timeout / interval; ++i)
    {
        m_client.update();
        for (size_t j = 0; j < m_client.get_message_count(); ++j)
        {
            const std::vector<uint8_t> &m = m_client.get_message(j);
            if (!has_info)
            {
                if (!get_info((char *)m.data(), m_server_info))
                {
                    m_client.disconnect();
                    m_error = "Invalid server response";
                    return false;
                }

                if (m_server_info.version != version)
                {
                    m_client.disconnect();
                    m_error = "Incompatible server version";
                    if (m_server_info.version > version)
                        m_error += ", please update";
                    return false;
                }

                has_info = true;
                send_string("connect");
            }
            else
            {
                const std::string str((char *)m.data(), m.size());
                std::istringstream ss(str);
                std::string command;
                ss >> command;

                if (command == "connected")
                {
                    ss >> m_id;
                    ss >> m_last_obj_id;
                    return true;
                }

                m_client.disconnect();
                m_error = "Invalid server response";
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }

    m_client.disconnect();
    m_error = "Connection timed out";
    return false;
}

//------------------------------------------------------------

void network_client::start()
{
    if (!m_client.is_open())
        return;

    send_string("sync_time");
    bool ready = false;
    while (m_client.is_connected() && !ready)
    {
        m_client.update();
        for (int j = 0; j < m_client.get_message_count(); ++j)
        {
            if (strcmp((char *)m_client.get_message(j).data(), "ready") == 0)
                ready = true;
        }
    }

    const auto time = nya_system::get_time();
    send_string("sync");

    ready = false;
    while (m_client.is_connected() && !ready)
    {
        m_client.update();
        for (int j = 0; j < m_client.get_message_count(); ++j)
        {
            const auto &m = m_client.get_message(j);
            const std::string str((char *)m.data(), m.size());
            std::istringstream ss(str);
            std::string command;
            ss >> command;
            if (command == "time")
                ss >> m_time, ready = true;
        }
    }

    auto ping = nya_system::get_time() - time;
    m_time += ping / 2; // client->server->client, assume symmetric lag

    send_string("start");
}

//------------------------------------------------------------

void network_client::disconnect()
{
    m_planes.clear();

    if (!m_client.is_open())
        return;

    send_string("disconnect");
    m_client.disconnect();
}

//------------------------------------------------------------

template<typename rs> bool receive_object(rs &objs, unsigned int client_id, const std::vector<uint8_t> &msg, unsigned int time)
{
    net_msg_header h;
    h.deserialize(msg);

    for (auto &o: objs.objects)
    {
        if (o.r.id == h.id && o.r.client_id != client_id)
        {
            if (o.last_time > time)
                return false;

            o.net->deserialize(msg);
            o.net->fix_time(int(time - h.time));
            o.last_time = h.time;
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------

void network_client::update()
{
    m_client.update();

    for (size_t i = 0; i < m_client.get_message_count(); ++i)
    {
        auto &m = m_client.get_message(i);
        if (m.empty())
            continue;

        switch (m[0])
        {
            case net_msg_plane: receive_object(m_planes, m_id, m, m_time); continue;
            case net_msg_missile: receive_object(m_missiles, m_id, m, m_time); continue;
            default: break;
        }

        const std::string str((char *)m.data(), m.size());
        std::istringstream is(str);
        std::string cmd;
        is >> cmd;

        if (cmd == "message")
        {
            std::string str;
            std::getline(is, str);
            m_general_msg.push_back(str);
        }
        else if (cmd == "game_data")
        {
            msg_game_data mg;
            read(is, mg);
            m_game_data_msg.push_back(mg);
        }
        else if (cmd == "add_plane")
        {
            msg_add_plane ap;
            read(is, ap);
            m_planes.add_msgs.push_back(ap);
        }
        else if (cmd == "remove_plane")
        {
            unsigned int plane_id;
            is >> plane_id;
            m_planes.remove(plane_id);
        }
        else if (cmd == "add_missile")
        {
            msg_add_missile am;
            read(is, am);
            m_missiles.add_msgs.push_back(am);
        }
        else if (cmd == "remove_missile")
        {
            unsigned int missile_id;
            is >> missile_id;
            m_missiles.remove(missile_id);
        }
        else if (cmd == "disconnect")
        {
            m_client.disconnect();
        }
        else
            printf("client received: %s\n", str.c_str());
    }
}

//------------------------------------------------------------

inline bool _send_string(miso::client_tcp &client, const std::string &str)
{
    return client.send_message_cstr(str.c_str());
}

//------------------------------------------------------------

template<typename rs> void send_requests(rs &requests, miso::client_tcp &client, const std::string &msg)
{
    if (requests.empty())
        return;

    for (auto &r: requests)
        _send_string(client, msg + " " + to_string(r));
    requests.clear();
}

//------------------------------------------------------------

template<typename rs> void send_objects(rs &objs, miso::client_tcp &client, unsigned int time, const std::string &type)
{
    send_requests(objs.add_requests, client, "add_" + type);

    for (auto &o: objs.objects)
    {
        if (!o.net->source)
            continue;

        if (o.net.unique())
            _send_string(client, "remove_" + type + " " + std::to_string(o.r.id));
        else
        {
            std::vector<uint8_t> msg;
            net_msg_header h;
            h.time = time;
            h.id = o.r.id;
            h.serialize(msg);
            o.net->serialize(msg);
            client.send_message(msg);
        }
    }

    objs.remove_src_unique();
}

//------------------------------------------------------------

void network_client::update_post(int dt)
{
    m_time += dt;
    if (m_time - m_last_send_time < 1000 / net_fps)
        return;
    m_last_send_time = m_time;

    send_objects(m_planes, m_client, m_time, "plane");
    send_objects(m_missiles, m_client, m_time, "missile");
    send_requests(m_general_msg_requests, m_client, "message");
    send_requests(m_game_data_msg_requests, m_client, "game_data");
}

//------------------------------------------------------------

bool network_client::send_string(const std::string &str)
{
    return _send_string(m_client, str);
}

//------------------------------------------------------------

const server_info &network_client::get_server_info() const
{
    return m_server_info;
}

//------------------------------------------------------------

network_client::~network_client()
{
    disconnect();
}

//------------------------------------------------------------
}
