//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#define ASIO_STANDALONE
#include "asio.hpp"

using asio::ip::tcp;

#include "util/util.h"

#include "network_data.h"

#include <thread>
#include <mutex>
#include <vector>
#include <deque>
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

class network_messages
{
public:
    void start(tcp::socket *socket);
    void end();
    void send_message(const std::string &msg);
    bool get_message(std::string &msg);

private:
    void start_read();
    void read_callback(const asio::error_code& error, std::size_t bytes_transferred);
    void write_callback(const asio::error_code& error);

private:
    asio::streambuf m_response;
    tcp::socket *m_socket = 0;
    std::mutex m_mutex;
    std::deque<std::string> m_msgs;
};

//------------------------------------------------------------

class network_server: public noncopyable, public network_interface
{
public:
    bool open(short port, const char *game_mode, const char *location, int max_players);
    void close();

    bool is_server() const override { return true; }

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
        bool started = false;
        tcp::socket socket;
        network_messages messages;

        client(asio::io_service& io_service): socket(io_service) {}
        ~client() { socket.close(); }
    };

    std::vector<client *> m_clients;
    std::mutex m_mutex;
    std::thread m_accept_thread;
    std::thread m_update_thread;
    std::string m_header;
    int m_max_players = 0;
};

//------------------------------------------------------------

class network_client: public noncopyable, public network_interface
{
public:
    bool connect(const char *address, short port);
    void disconnect();

    void start();

    const server_info &get_server_info() const;

    ~network_client();

private:
    asio::io_service m_service;
    tcp::socket *m_socket = 0;
    std::thread m_thread;
    std::thread m_update_thread;
    server_info m_server_info;
    network_messages m_messages;
    bool m_started = false;
};

//------------------------------------------------------------
}
