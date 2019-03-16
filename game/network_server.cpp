//
// open horizon -- undefined_darkness@outlook.com
//

#include "network_server.h"
#include "network_helpers.h"
#include "miso/protocol/app_protocol_simple.h"
#include "system/system.h"

namespace game
{
//------------------------------------------------------------

bool network_server::open(unsigned short port, const char *name, const char *game_mode, const char *location, int max_players)
{
    if (m_server.is_open())
        return false;

    if (!game_mode || !location)
        return false;

    m_header = server_header;
    m_header.append(" version ").append(std::to_string(version));
    m_header.append(" server_name ").append(name);
    m_header.append(" game_mode ").append(game_mode);
    m_header.append(" location ").append(location);
    m_header.append(" players ").append("1"); //ToDo
    m_header.append(" max_players ").append(std::to_string(max_players));
    m_header.append(" end");

    //self-check
    server_info i;
    if (!get_info(m_header, i))
        return false;

    m_max_players = max_players;

    static miso::app_protocol_simple protocol;
    m_server.set_app_protocol(&protocol);
    if (!m_server.open_ipv4(port))
        return false;

    return true;
}

//------------------------------------------------------------

void network_server::close()
{
    for (auto &c: m_clients)
        send_string(c.first, "disconnect");

    m_server.close(true);
    m_planes.clear();
}

//------------------------------------------------------------

network_server::~network_server()
{
    close();
}

//------------------------------------------------------------

int network_server::get_players_count() const
{
    return (int)m_clients.size() + 1; //server is also a player
}

//------------------------------------------------------------

void network_server::update()
{
    m_server.update();

    for (int i = 0; i < m_server.get_new_client_count(); ++i)
    {
        auto id = m_server.get_new_client(i);

        if (!send_string(id, m_header))
        {
            m_server.disconnect_user(id);
            continue;
        }

        if (m_max_players > 0 && get_players_count() >= m_max_players)
        {
            send_string(id, "max_players_limit");
            m_server.disconnect_user(id);
            continue;
        }

        m_requests.insert(id);
    }

    for (int i = 0; i < m_server.get_lost_client_count(); ++i)
        remove_client(m_server.get_lost_client(i));

    for (size_t i = 0; i < m_server.get_message_count(); ++i)
        process_msg(m_server.get_message(i));
}

//------------------------------------------------------------

void network_server::process_msg(const std::pair<miso::server_tcp::client_id, std::vector<uint8_t> > &msg)
{
    const auto id = msg.first;

    auto c = m_clients.find(id);
    if (c != m_clients.end())
    {
        process_msg(c->second, msg.second);
        return;
    }

    auto r = m_requests.find(id);
    if (r == m_requests.end())
        return;

    if (strcmp((char *)msg.second.data(), "connect") == 0)
    {
        if (m_max_players > 0 && get_players_count() >= m_max_players)
        {
            send_string(id, "max_players_limit");
            m_server.disconnect_user(id);
            m_requests.erase(id);
        }
        else
        {
            const unsigned int client_range = (unsigned int)(-1) / 1024;
            m_last_client_range += client_range;
            send_string(id, "connected " + std::to_string(id) + " " + std::to_string(m_last_client_range));
            m_requests.erase(id);
            client &c = m_clients[id];
            c.id = id;
        }
    }
}

//------------------------------------------------------------

void network_server::cache_net_game_data(const msg_game_data &data)
{
    for (auto &d: m_game_data_cache)
    {
        if (d.plane_id == data.plane_id)
        {
            d = data;
            return;
        }
    }

    m_game_data_cache.push_back(data);
}

//------------------------------------------------------------

void network_server::process_msg(client &c, const std::vector<uint8_t> &msg)
{
    if (msg.empty())
        return;

    std::string str((char *)msg.data(), msg.size());
    std::istringstream is(str);
    std::string cmd;
    is>>cmd;

    if (cmd == "plane")
    {
        unsigned int time, plane_id;
        is >> time, is >> plane_id;

        for (auto &p: m_planes.objects)
        {
            if (p.r.id == plane_id)
            {
                if (p.last_time > time)
                    break;

                read(is, p.net);
                const int time_fix = int(m_time - time);

                for (auto &oc: m_clients)
                {
                    if (oc.first == c.id)
                        continue;

                    send_string(oc.first, "plane " + std::to_string(time) + " " + std::to_string(p.r.id) + " "+ to_string(p.net));
                }

                p.net->pos += p.net->vel * (0.001f * time_fix);
                p.last_time = time;
                break;
            }
        }
    }
    else if (cmd == "missile")
    {
        unsigned int time, missile_id;
        is >> time, is >> missile_id;

        for (auto &m: m_missiles.objects)
        {
            if (m.r.id == missile_id)
            {
                if (m.last_time > time)
                    break;

                read(is, m.net);
                const int time_fix = int(m_time - time);

                for (auto &oc: m_clients)
                {
                    if (oc.first == c.id)
                        continue;

                    send_string(oc.first, "missile " + std::to_string(time) + " " + std::to_string(m.r.id) + " "+ to_string(m.net));
                }

                m.net->pos += m.net->vel * (0.001f * time_fix);
                m.last_time = time;
                break;
            }
        }
    }
    else if (cmd == "message")
    {
        std::string str;
        std::getline(is, str);
        m_general_msg.push_back(str);

        for (auto &oc: m_clients)
        {
            if (oc.first == c.id)
                continue;

            m_server.send_message(oc.first, msg);
        }
    }
    else if (cmd == "game_data")
    {
        msg_game_data mg;
        read(is, mg);
        m_game_data_msg.push_back(mg);
        cache_net_game_data(mg);

        for (auto &oc: m_clients)
        {
            if (oc.first == c.id)
                continue;

            m_server.send_message(oc.first, msg);
        }
    }
    else if (cmd == "add_plane")
    {
        msg_add_plane ap;
        read(is, ap);
        ap.client_id = c.id;
        m_planes.add_msgs.push_back(ap);

        for (auto &oc: m_clients)
        {
            if(oc.first == c.id)
                continue;

            send_string(oc.first, "add_plane " + to_string(ap));
        }
    }
    else if (cmd == "add_missile")
    {
        msg_add_missile am;
        read(is, am);

        am.client_id = c.id;
        m_missiles.add_msgs.push_back(am);

        for (auto &oc: m_clients)
        {
            if(oc.first == c.id)
                continue;

            send_string(oc.first, "add_missile " + to_string(am));
        }
    }
    else if (cmd == "remove_missile")
    {
        unsigned int missile_id;
        is >> missile_id;

        m_missiles.remove(missile_id);

        for (auto &oc: m_clients)
        {
            if(oc.first == c.id)
                continue;

            m_server.send_message(oc.first, msg);
        }
    }
    else if (cmd == "sync_time")
    {
        send_string(c.id, "ready");

        std::vector<std::pair<miso::server_interface::client_id, std::vector<uint8_t> > > other_messages;

        bool ready = false;
        while (m_server.is_open() && !ready)
        {
            m_server.update();
            for (int j = 0; j < m_server.get_message_count(); ++j)
            {
                auto &m = m_server.get_message(j);
                if(m.first == c.id && strcmp((char *)m.second.data(), "sync") == 0)
                    ready = true;
                else
                    other_messages.push_back(m);
            }
        }

        send_string(c.id, "time " + std::to_string(m_time));

        for (auto &o: other_messages)
            process_msg(o);
    }
    else if (cmd == "start")
    {
        for (auto &p: m_planes.objects)
            send_string(c.id, "add_plane " + to_string(p.r));

        for (auto &d: m_game_data_cache)
            send_string(c.id, "game_data " + to_string(d));

        c.started = true;
    }
    else
        printf("server received: %s\n", str.c_str());
}

//------------------------------------------------------------

inline bool _send_string(miso::server_tcp &server, miso::server_tcp::client_id id, const std::string &str)
{
    return server.send_message_cstr(id, str.c_str());
}

//------------------------------------------------------------

template<typename rs, typename cs> void send_requests(rs &requests, cs &clients, miso::server_tcp &server, const std::string &msg)
{
    if (requests.empty())
        return;

    for (auto &c: clients)
    {
        if (!c.second.started)
            continue;

        for (auto &r: requests)
            _send_string(server, c.first, msg + " " + to_string(r));
    }

    requests.clear();
}

//------------------------------------------------------------

template<typename rs, typename cs> void send_objects(rs &objs, cs &clients, miso::server_tcp &server, unsigned int time, const std::string &type)
{
    send_requests(objs.add_requests, clients, server, "add_" + type);

    for (auto &c: clients)
    {
        if (!c.second.started)
            continue;

        for (auto &o: objs.objects)
        {
            if (!o.net->source)
                continue;

            if (o.net.unique())
                _send_string(server, c.first, "remove_" + type + " " + std::to_string(o.r.id));
            else
                _send_string(server, c.first, type + " " + std::to_string(time) + " " + std::to_string(o.r.id) + " "+ to_string(o.net));
        }
    }

    objs.remove_src_unique();
}

//------------------------------------------------------------

void network_server::update_post(int dt)
{
    m_time += dt;
    if (m_time - m_last_send_time < 1000 / net_fps)
        return;
    m_last_send_time = m_time;

    for (auto &d: m_game_data_msg_requests)
        cache_net_game_data(d);

    send_objects(m_planes, m_clients, m_server, m_time, "plane");
    send_objects(m_missiles, m_clients, m_server, m_time, "missile");
    send_requests(m_general_msg_requests, m_clients, m_server, "message");
    send_requests(m_game_data_msg_requests, m_clients, m_server, "game_data");
}

//------------------------------------------------------------

void network_server::remove_client(miso::server_tcp::client_id id)
{
    for (auto &p: m_planes.objects)
    {
        if (p.r.client_id != id)
            continue;

        for (auto &c: m_clients)
            send_string(c.first, "remove_plane " + std::to_string(p.r.id));
    }

    m_planes.remove_by_client_id(id);

    for (auto &m: m_missiles.objects)
    {
        if (m.r.client_id != id)
            continue;

        for (auto &c: m_clients)
            send_string(c.first, "remove_missile " + std::to_string(m.r.id));
    }

    m_missiles.remove_by_client_id(id);
}

//------------------------------------------------------------

bool network_server::send_string(miso::server_tcp::client_id id, const std::string &str)
{
    return _send_string(m_server, id, str);
}

//------------------------------------------------------------
}
