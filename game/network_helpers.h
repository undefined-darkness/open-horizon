//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "network_data.h"
#include <sstream>

//ToDo: send plane data binary via udp and only important commands via tcp
//current inplementation is for testing purpose only

namespace game
{
//------------------------------------------------------------

static const int version = 1;
static const char *server_header = "Open-Horizon server";
static const unsigned int net_fps = 25;

//------------------------------------------------------------

static bool get_info(const std::string &s, server_info &info)
{
    if (s.compare(0, strlen(server_header), server_header) != 0)
        return false;

    std::istringstream ss(s.c_str() + strlen(server_header) + 1);
    std::string tmp;

    ss >> tmp;
    if (tmp != "version") //obligatory
        return false;

    ss >> info.version;

    while (ss)
    {
        ss >> tmp;
        if (tmp == "server_name")
            ss >> info.server_name;
        else if (tmp == "game_mode")
            ss >> info.game_mode;
        else if (tmp == "location")
            ss >> info.location;
        else if (tmp == "players")
            ss >> info.players;
        else if (tmp == "max_players")
            ss >> info.max_players;
        else if (tmp == "end")
            return true;
        else
            ss >> tmp; //skip
    }
    
    return false;
}

//------------------------------------------------------------

inline std::string to_string(const nya_math::vec3 &v)
{
    std::ostringstream ss;
    ss << v.x << " " << v.y << " " << v.z;
    return ss.str();
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

inline std::string to_string(const net_plane_ptr &n)
{
    if (!n)
        return "";

    return to_string(n->pos) + " " + to_string(n->vel) + " " + to_string(n->rot) + " " + to_string(n->ctrl_rot) + " "
    + std::to_string(n->ctrl_throttle) + " " + std::to_string(n->ctrl_brake) + " "
    + (n->ctrl_mgun ? '1' : '0') + " " + (n->ctrl_mgp ? '1' : '0');
}

//------------------------------------------------------------

inline void read(std::istringstream &is, net_plane_ptr &n)
{
    if (!n)
        return;

    read(is, n->pos), read(is, n->vel), read(is, n->rot);
    read(is, n->ctrl_rot), is >> n->ctrl_throttle, is >> n->ctrl_brake;
    is >> n->ctrl_mgun, is >> n->ctrl_mgp;
}

//------------------------------------------------------------

inline std::string to_string(const network_interface::msg_add_plane &m)
{
    return std::to_string(m.client_id) + " " + std::to_string(m.id) + " " + m.preset + " " + m.player_name + " " + std::to_string(m.color);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, network_interface::msg_add_plane &m)
{
    is>>m.client_id, is>>m.id, is>>m.preset, is>>m.player_name, is>>m.color;
}

//------------------------------------------------------------

inline std::string to_string(const net_missile_ptr &n)
{
    if (!n)
        return "";

    return to_string(n->pos) + " " + to_string(n->vel) + " " + to_string(n->rot) + " " + to_string(n->target_dir) + " " + (n->engine_started ? '1' : '0');
}

//------------------------------------------------------------

inline void read(std::istringstream &is, net_missile_ptr &n)
{
    if (!n)
        return;

    read(is, n->pos), read(is, n->vel), read(is, n->rot), read(is, n->target_dir), is >> n->engine_started;
}

//------------------------------------------------------------

inline std::string to_string(const network_interface::msg_add_missile &m)
{
    return std::to_string(m.client_id) + " " + std::to_string(m.plane_id) + " " + std::to_string(m.id) + " " + std::to_string(m.special);
}

//------------------------------------------------------------

inline void read(std::istringstream &is, network_interface::msg_add_missile &m)
{
    is >> m.client_id, is >> m.plane_id, is >> m.id, is>>m.special;
}

//------------------------------------------------------------

inline std::string to_string(const network_interface::msg_game_data &m)
{
    std::string str = std::to_string(m.client_id) + " " + std::to_string(m.plane_id) + " ";
    for (auto &p: m.data.params)
        str.append(p.first + '{' + p.second + '}');

    return str;
}

//------------------------------------------------------------

inline void read(std::istringstream &is, network_interface::msg_game_data &m)
{
    is >> m.client_id, is >> m.plane_id;
    is.ignore(); //whitespace
    std::string name, value;
    while (std::getline(is, name, '{') && std::getline(is, value, '}'))
        m.data.params.push_back(std::make_pair(name, value));
}

//------------------------------------------------------------

inline std::string to_string(const std::string &s) { return s; }

//------------------------------------------------------------
}
