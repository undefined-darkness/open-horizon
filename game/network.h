//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "util/util.h"

#include "network_data.h"

#include "miso/server/server_tcp.h"
#include "miso/client/client_tcp.h"

#include <map>
#include <set>
#include <string>

namespace game
{
//------------------------------------------------------------

bool http_get(const char *url, const char *arg, std::string &data);

//------------------------------------------------------------

struct server_info
{
    int version = 0;
    std::string game_mode;
    std::string location;
    int players = 0;
    int max_players = 0;
};

//------------------------------------------------------------

class servers_list
{
public:
    bool request_update();
    bool is_ready() const;
    typedef std::vector<std::string> list;
    void get_list(list &l) const;

public:
    static bool register_server(short port);

private:
    list m_list;
    bool m_ready = true;
};

//------------------------------------------------------------

class network_server: public noncopyable, public network_interface
{
public:
    bool open(short port, const char *game_mode, const char *location, int max_players);
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
    void process_msg(const std::pair<miso::server_tcp::client_id, std::string> &msg);
    void process_msg(client &c, const std::string &msg);
    void remove_client(miso::server_tcp::client_id id);

private:
    miso::server_tcp m_server;
    std::string m_header;
    int m_max_players = 0;

    struct client
    {
        miso::server_tcp::client_id id;
        bool started = false;
    };

    std::map<miso::server_tcp::client_id, client> m_clients;
    std::set<miso::server_tcp::client_id> m_requests;

    std::map<std::pair<miso::server_tcp::client_id, unsigned int>, unsigned int> m_planes_remap;
};

//------------------------------------------------------------

class network_client: public noncopyable, public network_interface
{
public:
    bool connect(const char *address, short port);
    void disconnect();

    void start();

    bool is_up() const { return m_client.is_open(); }

    const server_info &get_server_info() const;

    ~network_client();

private:
    void update() override;
    void update_post(int dt) override;

private:
    miso::client_tcp m_client;
    server_info m_server_info;
};

//------------------------------------------------------------
}
