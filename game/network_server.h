//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "network_data.h"
#include "miso/server/server_tcp.h"

#include <map>
#include <set>

namespace game
{
//------------------------------------------------------------

class network_server: public noncopyable, public network_interface
{
public:
    bool open(unsigned short port, const char *name, const char *game_mode, const char *location, int max_players);
    void close();

    bool is_server() const override { return true; }
    bool is_up() const { return m_server.is_open(); }

    int get_players_count() const;

    ~network_server();

private:
    void update() override;
    void update_post(int dt) override;

private:
    struct client;
    void process_msg(const std::pair<miso::server_tcp::client_id, std::vector<uint8_t> > &msg);
    void process_msg(client &c, const std::vector<uint8_t> &msg);
    void remove_client(miso::server_tcp::client_id id);
    bool send_string(miso::server_tcp::client_id, const std::string &str);

private:
    miso::server_tcp m_server;
    std::string m_header;
    int m_max_players = 0;
    unsigned int m_last_send_time = 0;
    unsigned int m_last_client_range = 0;

    struct client
    {
        miso::server_tcp::client_id id;
        bool started = false;
    };

    std::map<miso::server_tcp::client_id, client> m_clients;
    std::set<miso::server_tcp::client_id> m_requests;

    void cache_net_game_data(const msg_game_data &data);
    std::vector<msg_game_data> m_game_data_cache;
};

//------------------------------------------------------------
}
