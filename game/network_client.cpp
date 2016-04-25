//
// open horizon -- undefined_darkness@outlook.com
//

#include "network_client.h"
#include "network_helpers.h"
#include "system/system.h"
#include "miso/protocol/app_protocol_simple.h"
#include "miso/ipv4.h"

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
    if (!m_client.connect_wait(miso::ipv4_address(address), port, timeout))
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
            const std::string &m = m_client.get_message(j);
            if (!has_info)
            {
                if (!get_info(m, m_server_info))
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
                m_client.send_message("connect");
            }
            else
            {
                std::istringstream ss(m);
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

    m_client.send_message("sync_time");
    bool ready = false;
    while (m_client.is_connected() && !ready)
    {
        m_client.update();
        for (int j = 0; j < m_client.get_message_count(); ++j)
        {
            if(m_client.get_message(j)=="ready")
                ready = true;
        }
    }

    const auto time = nya_system::get_time();
    m_client.send_message("sync");

    ready = false;
    while (m_client.is_connected() && !ready)
    {
        m_client.update();
        for (int j = 0; j < m_client.get_message_count(); ++j)
        {
            std::istringstream ss(m_client.get_message(j));
            std::string command;
            ss >> command;
            if (command == "time")
                ss >> m_time, ready = true;
        }
    }

    auto ping = nya_system::get_time() - time;
    m_time += ping / 2; // client->server->client, assume symmetric lag

    m_client.send_message("start");
}

//------------------------------------------------------------

void network_client::disconnect()
{
    m_planes.clear();

    if (!m_client.is_open())
        return;

    m_client.send_message("disconnect");
    m_client.disconnect();
}

//------------------------------------------------------------

template<typename rs> bool receive_object(rs &objs, unsigned int client_id, std::istringstream &is, unsigned int time)
{
    unsigned int t, id;
    is >> t, is >> id;

    for (auto &o: objs.objects)
    {
        if (o.r.id == id && o.r.client_id != client_id)
        {
            if (o.last_time > time)
                return false;

            read(is, o.net);
            const int time_fix = int(time - t);
            o.net->pos += o.net->vel * (0.001f * time_fix);
            o.last_time = t;
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

        std::istringstream is(m);
        std::string cmd;
        is >> cmd;

        if (cmd == "plane")
        {
            receive_object(m_planes, m_id, is, m_time);
        }
        else if (cmd == "missile")
        {
            receive_object(m_missiles, m_id, is, m_time);
        }
        else if (cmd == "message")
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
            printf("client received: %s\n", m.c_str());
    }
}

//------------------------------------------------------------

template<typename rs> void send_requests(rs &requests, miso::client_tcp &client, const std::string &msg)
{
    if (requests.empty())
        return;

    for (auto &r: requests)
        client.send_message(msg + " " + to_string(r));
    requests.clear();
}

//------------------------------------------------------------

template<typename rs> void send_objects(rs &objs, miso::client_tcp &client, unsigned int time, const std::string &msg)
{
    send_requests(objs.add_requests, client, "add_" + msg);

    for (auto &o: objs.objects)
    {
        if (!o.net->source)
            continue;

        if (o.net.unique())
            client.send_message("remove_" + msg + " " + std::to_string(o.r.id));
        else
            client.send_message(msg + " " + std::to_string(time) + " " + std::to_string(o.r.id) + " "+ to_string(o.net));
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
