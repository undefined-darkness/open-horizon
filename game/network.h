//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#define ASIO_STANDALONE
#include "asio.hpp"

using asio::ip::tcp;

#include "util/util.h"

#include <thread>
#include <mutex>
#include <vector>
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

class network_server: public noncopyable
{
public:
    bool open(short port, const char *game_mode, int max_players);
    void close();

    ~network_server();

private:
    struct client;
    void new_client_wait();
    void handle_accept(client* c, asio::error_code err);
    void accept_thread(short port);
    void on_send_info(client* c, const asio::error_code & err, size_t bytes);

private:
    asio::io_service m_service;
    tcp::acceptor *m_acceptor = 0;

    struct client
    {
        tcp::socket m_socket;
        client(asio::io_service& io_service): m_socket(io_service) {}
        ~client() { m_socket.close(); }
    };

    std::vector<client *> m_clients;
    std::mutex m_handle_mutex;
    std::thread m_thread;
    std::string m_header;
    int m_max_players = 0;
};

//------------------------------------------------------------

class network_client: public noncopyable
{
public:
    bool connect(const char *address, short port);
    void disconnect();

    ~network_client();

private:
    asio::io_service m_service;
    tcp::socket *m_socket = 0;
    std::thread m_thread;
};

//------------------------------------------------------------
}
