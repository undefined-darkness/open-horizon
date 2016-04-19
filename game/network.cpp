//
// open horizon -- undefined_darkness@outlook.com
//

#include "network.h"
#include "miso/socket/socket.h"
#include "miso/protocol/app_protocol_simple.h"
#include "miso/ipv4.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

#include <sstream>
#include <thread>
#include <chrono>

//ToDo: send plane data binary via udp and only important commands via tcp
//current inplementation is for testing purpose only

namespace game
{
//------------------------------------------------------------

static const int version = 1;
const char *server_header = "Open-Horizon server";

//------------------------------------------------------------

inline std::string resolve(const std::string &url)
{
#ifdef _WIN32
    static WSADATA wsaData;
    static bool once = true;
    if (once)
    {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        once = false;
    }
#endif

    const hostent *hp = gethostbyname(url.c_str());
    if (!hp || !hp->h_addr_list[0])
        return "";

    return inet_ntoa(*(in_addr*)(hp -> h_addr_list[0]));
}

//------------------------------------------------------------

bool http_get(const char *url, const char *arg, std::string &data)
{
    data.clear();
    if (!url || !arg)
        return false;

    auto ip = resolve("http://" + std::string(url));
    miso::socket_sync<miso::tcp> socket;
    if (!socket.connect(miso::ipv4_address(ip.c_str()), 80))
        return false;

    std::string request = "GET " + std::string(arg) + " HTTP/1.0\r\n";
    request +=  "Host: " + std::string(url) + "\r\n";
    request += "Accept: */*\r\n";
    request += "Connection: close\r\n\r\n";

    if (socket.send(request.c_str(), request.length()) < (int)request.length())
    {
        socket.close();
        return false;
    }

    std::string response;
    char buf[4096];
    int size;
    while ((size = socket.recv(buf, sizeof(buf))) > 0)
        response.append(buf, size);
    socket.close();

    std::istringstream response_stream(response);
    std::string http_version;
    response_stream >> http_version;
    if (http_version.substr(0, 5) != "HTTP/") //invalid response
        return false;

    unsigned int status_code = 0;
    response_stream >> status_code;

    if (status_code != 200)
    {
        std::string status_message;
        std::getline(response_stream, status_message);
        return false;
    }

    const char *data_end = "\r\n\r\n";
    const size_t data_pos = response.find(data_end);
    if (data_pos == std::string::npos)
        return false;

    data = response.substr(data_pos + strlen(data_end));
    return true;
}

//------------------------------------------------------------

static bool get_info(const std::string &s, server_info &info)
{
    if (s.compare(0, strlen(server_header), server_header) != 0)
        return false;

    std::istringstream ss(s.c_str() + strlen(server_header) + 1);
    std::string tmp;
    ss >> tmp;
    if (tmp != "version")
        return false;

    ss >> info.version;

    ss >> tmp;
    if (tmp != "game_mode")
        return false;

    ss >> info.game_mode;

    ss >> tmp;
    if (tmp != "location")
        return false;

    ss >> info.location;

    ss >> tmp;
    if (tmp != "players")
        return false;

    ss >> info.players;

    ss >> tmp;
    if (tmp != "max_players")
        return false;
    
    ss >> info.max_players;
    
    ss >> tmp;
    if (tmp != "end")
        return false;
    
    return true;
}

//------------------------------------------------------------

bool servers_list::request_update()
{
    if (!m_ready)
        return false;

    m_list.clear();
    std::string data;
    std::string params("/get?app=open-horizon");
    params.append("&version=").append(std::to_string(version));
    bool result = http_get("razgriz.pythonanywhere.com", params.c_str(), data);
    if (!result)
        return false;

    std::stringstream ss(data);
    std::string to;
    while (std::getline(ss,to,'\n'))
        m_list.push_back(to);

    for (auto &l: m_list)
    {
        //ToDo: check alive
    }

    return result;
}

//------------------------------------------------------------

bool servers_list::register_server(short port)
{
    std::string data;
    std::string params("/log?app=open-horizon");
    params.append("&version=").append(std::to_string(version));
    params.append("&port=").append(std::to_string(port));
    return http_get("razgriz.pythonanywhere.com", params.c_str(), data);
}

//------------------------------------------------------------

bool servers_list::is_ready() const
{
    return m_ready;
}

//------------------------------------------------------------

void servers_list::get_list(std::vector<std::string> &list) const
{
    list = m_list;
}

//------------------------------------------------------------

inline std::string to_string(const nya_math::vec3 &v)
{
    return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, nya_math::vec3 &v)
{
    is>>v.x, is>>v.y, is>>v.z;
}

//------------------------------------------------------------

inline std::string to_string(const nya_math::quat &q)
{
    return to_string(q.v) + " " + std::to_string(q.w);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, nya_math::quat &q)
{
    read(is, q.v), is>>q.w;
}

//------------------------------------------------------------

inline std::string to_string(const net_plane &n)
{
    return to_string(n.pos) + " " + to_string(n.vel) + " " + to_string(n.rot) + " "
           + to_string(n.ctrl_rot) + " " + std::to_string(n.ctrl_brake) + " " + std::to_string(n.hp);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, net_plane &n)
{
    read(is, n.pos), read(is, n.vel), read(is, n.rot),
    read(is, n.ctrl_rot), is>>n.ctrl_brake, is>>n.hp;
}

//------------------------------------------------------------

inline std::string to_string(const network_client::msg_add_plane &m)
{
    return std::to_string(m.client_id) + " " + std::to_string(m.plane_id) + " " + m.name + " " + std::to_string(m.color);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, network_client::msg_add_plane &m)
{
    is>>m.client_id, is>>m.plane_id, is>>m.name, is>>m.color;
}

//------------------------------------------------------------

bool network_server::open(short port, const char *game_mode, const char *location, int max_players)
{
    if (m_server.is_open())
        return false;

    if (!game_mode || !location)
        return false;

    m_header = server_header;
    m_header.append(" version ").append(std::to_string(version));
    m_header.append(" game_mode ").append(game_mode);
    m_header.append(" location ").append(location);
    m_header.append(" players ").append("1"); //ToDo
    m_header.append(" max_players ").append(std::to_string(max_players));
    m_header.append(" end\n");

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
        m_server.send_message(c.first, "disconnect");

    m_server.close();
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

        if (!m_server.send_message(id, m_header))
        {
            m_server.disconnect_user(id);
            continue;
        }

        if (m_max_players > 0 && get_players_count() >= m_max_players)
        {
            m_server.send_message(id, "max_players_limit");
            m_server.disconnect_user(id);
            continue;
        }

        m_requests.insert(id);
    }

    for (int i = 0; i < m_server.get_lost_client_count(); ++i)
        remove_client(m_server.get_lost_client(i));

    for (size_t i = 0; i < m_server.get_message_count(); ++i)
    {
        auto &m = m_server.get_message(i);
        const auto id = m.first;

        auto c = m_clients.find(id);
        if (c != m_clients.end())
        {
            process_msg(c->second, m.second);
            continue;
        }

        auto r = m_requests.find(id);
        if (r == m_requests.end())
            continue;

        if (m.second == "connect")
        {
            if (m_max_players > 0 && get_players_count() >= m_max_players)
            {
                m_server.send_message(id, "max_players_limit");
                m_server.disconnect_user(id);
                m_requests.erase(id);
            }
            else
            {
                m_server.send_message(id, "connected " + std::to_string(id));
                m_requests.erase(id);
                client &c = m_clients[id];
                c.id = id;
            }
        }
    }
}

//------------------------------------------------------------

void network_server::process_msg(client &c, const std::string &msg)
{
    std::istringstream is(msg);
    std::string cmd;
    is>>cmd;

    if (cmd == "plane")
    {
        unsigned int time, plane_id;
        is >> time, is >> plane_id;

        auto k = std::make_pair(c.id, plane_id);
        auto remap_id = m_planes_remap.find(k);
        if (remap_id != m_planes_remap.end())
        {
            for (auto &p: m_planes)
            {
                if (p.r.plane_id == remap_id->second)
                {
                    if (p.last_time > time)
                        break;

                    //p.net->time_fix = m_time - time; //ToDo: synchronize time first
                    read(is, *p.net.get());
                    p.last_time = time;
                    break;
                }
            }
        }
    }
    else if (cmd == "add_plane")
    {
        msg_add_plane ap;
        read(is, ap);
        auto new_id = new_plane_id();
        m_planes_remap[std::make_pair(c.id, ap.plane_id)] = new_id;
        ap.client_id = c.id;
        ap.plane_id = new_id;
        m_add_plane_msgs.push_back(ap);

        for (auto &oc: m_clients)
        {
            if(oc.first == c.id)
                continue;

            m_server.send_message(oc.first, "add_plane " + to_string(ap));
        }
    }
    else if (cmd == "start")
    {
        for (auto &p: m_planes)
        {
            m_server.send_message(c.id, "add_plane " + to_string(p.r));
            c.started = true;
        }
    }
    else
        printf("server received: %s\n", msg.c_str());
}

//------------------------------------------------------------

void network_server::update_post(int dt)
{
    m_time += dt;

    for (auto &c: m_clients)
    {
        if (!c.second.started)
            continue;

        for (auto &p: m_planes)
        {
            if (c.first == p.r.client_id)
                continue;

            m_server.send_message(c.first, "plane " + std::to_string(m_time) + " " + std::to_string(p.r.plane_id) + " "+ to_string(*p.net.get()));
        }
    }

    if (!m_add_plane_requests.empty())
    {
        for (auto &c: m_clients)
        {
            if (!c.second.started)
                continue;

            for (auto &r: m_add_plane_requests)
            {
                if (c.first == r.client_id)
                    continue;
                
                m_server.send_message(c.first, "add_plane " + to_string(r));
            }
        }
        
        m_add_plane_requests.clear();
    }
}

//------------------------------------------------------------

void network_server::remove_client(miso::server_tcp::client_id id)
{
    //ToDo
}

//------------------------------------------------------------

bool network_client::connect(const char *address, short port)
{
    if (!address || m_client.is_open())
        return false;

    const int timeout = 2000;

    static miso::app_protocol_simple protocol;
    m_client.set_app_protocol(&protocol);
    if (!m_client.connect_wait(miso::ipv4_address(address), port, timeout))
        return false;

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
                    return true;
                }

                m_client.disconnect();
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }

    m_client.disconnect();
    return false;
}

//------------------------------------------------------------

void network_client::start()
{
    if (!m_client.is_open())
        return;

    m_client.send_message("start");
}

//------------------------------------------------------------

void network_client::disconnect()
{
    if (!m_client.is_open())
        return;

    m_client.send_message("disconnect");
    m_client.disconnect();
    m_planes.clear();
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
            unsigned short time, plane_id;
            is >> time, is >> plane_id;

            for (auto &p: m_planes)
            {
                if (p.r.plane_id == plane_id && p.r.client_id != m_id)
                {
                    if (p.last_time > time)
                        break;

                    //p.net->time_fix = m_time - time; //ToDo: synchronize time first
                    read(is, *p.net.get());
                    p.last_time = time;
                    break;
                }
            }
        }
        else if (cmd == "add_plane")
        {
            msg_add_plane ap;
            read(is, ap);
            m_add_plane_msgs.push_back(ap);
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

void network_client::update_post(int dt)
{
    m_time += dt;

    for (auto &p: m_planes)
    {
        if (!p.net->source)
            continue;

        m_client.send_message("plane " + std::to_string(m_time) + " " + std::to_string(p.r.plane_id) + " "+ to_string(*p.net.get()));
    }

    if (!m_add_plane_requests.empty())
    {
        for (auto &r: m_add_plane_requests)
        {
            if (r.client_id == m_id)
                m_client.send_message("add_plane " + to_string(r));
        }
        m_add_plane_requests.clear();
    }
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
