//
// open horizon -- undefined_darkness@outlook.com
//

#include "network.h"
#include <iostream>

namespace game
{
//------------------------------------------------------------

static const int version = 1;
const char *server_header = "Open-Horizon server";

//------------------------------------------------------------

inline std::string as_string(asio::streambuf &b)
{
    std::ostringstream ss;
    ss << &b;
    std::string s = ss.str();
    return std::string(s.c_str());
}

//------------------------------------------------------------

bool http_get(const char *url, const char *arg, std::string &data)
{
    data.clear();
    if (!url || !arg)
        return false;

    asio::error_code error;
    asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(url, "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, error);
    if (error)
        return false;

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    asio::connect(socket, endpoint_iterator, error);
    if (error)
        return false;

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << arg << " HTTP/1.0\r\n";
    request_stream << "Host: " << url << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    asio::write(socket, request, error);
    if (error)
        return false;

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    asio::streambuf response;
    asio::read_until(socket, response, "\r\n", error);
    if (error)
        return false;

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/") //Invalid response
        return false;

    if (status_code != 200)
        return false;

    // Read the response headers, which are terminated by a blank line.
    asio::read_until(socket, response, "\r\n\r\n", error);
    if (error)
        return false;

    // Process the response headers.
    std::string header, h;
    while (std::getline(response_stream, h) && h != "\r")
        header.append(h);

    // Write whatever content we already have to output.
    if (response.size() > 0)
        response_stream >> data;

    // Read until EOF, writing data to output as we go.
    while (!error)
    {
        asio::read(socket, response, asio::transfer_at_least(1), error);
        data += as_string(response);
    }

    return error == asio::error::eof;
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
        asio::error_code error;
        asio::io_service service;
        tcp::socket socket(service);

        //asio::connect(socket, tcp::endpoint(asio::ip::address::from_string(l), port), error);
        if (error)
            continue;
    }

    //ToDo: check servers

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

bool network_server::open(short port, const char *game_mode, const char *location, int max_players)
{
    if (m_acceptor)
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

    try { m_acceptor = new tcp::acceptor(m_service, tcp::endpoint(tcp::v4(), port)); }
    catch (asio::system_error &e) { return false; }

    new_client_wait();
    m_thread = std::thread([&]() { try { m_service.run(); } catch (asio::system_error &e) {} });
    return true;
}

//------------------------------------------------------------

void network_server::new_client_wait()
{
    if (!m_acceptor)
        return;

    client *c = new client(m_service);
    m_acceptor->async_accept(c->m_socket, std::bind(&network_server::handle_accept, this, c, std::placeholders::_1));
}

//------------------------------------------------------------

void network_server::handle_accept(client* c, asio::error_code error)
{
    if (!error)
    {
        asio::write(c->m_socket, asio::buffer(m_header), error);
        if (!error)
        {
            asio::streambuf b;
            asio::read_until(c->m_socket, b, "\n", error);
            if (!error)
            {
                auto response = as_string(b);
                if (response == "connect\n")
                {
                    int players_count = 0;
                    m_handle_mutex.lock();
                    players_count = (int)m_clients.size() + 1; //server is also a player
                    m_handle_mutex.unlock();

                    if (m_max_players > 0 && players_count >= m_max_players)
                    {
                        asio::write(c->m_socket, asio::buffer("max_players_limit\n"), error);
                        delete c;
                        c = 0;
                    }
                    else
                    {
                        asio::write(c->m_socket, asio::buffer("connected\n"), error);
                        if (!error)
                        {
                            m_handle_mutex.lock();
                            m_clients.push_back(c);
                            m_handle_mutex.unlock();
                        }
                    }
                }
            }
        }
    }

    if (error && c)
        delete c;

    new_client_wait();
}

//------------------------------------------------------------

void network_server::close()
{
    if (!m_acceptor)
        return;

    m_service.stop();
    if (m_thread.joinable())
        m_thread.join();

    delete m_acceptor;
    m_acceptor = 0;
}

//------------------------------------------------------------

network_server::~network_server()
{
    close();
}

//------------------------------------------------------------

bool network_client::connect(const char *address, short port)
{
    if (!address || m_socket)
        return false;

    asio::error_code error;

    m_socket = new tcp::socket(m_service);

    tcp::endpoint ep(asio::ip::address::from_string(address), port);
    m_socket->connect(ep, error);
    if (error)
    {
        delete m_socket;
        m_socket = 0;
        return  false;
    }

    asio::streambuf b;
    asio::read_until(*m_socket, b, "\n", error);
    if (error && error != asio::error::eof)
    {
        m_socket->close();
        delete m_socket;
        m_socket = 0;
        return false;
    }

    if (!get_info(as_string(b), m_server_info))
    {
        m_socket->close();
        delete m_socket;
        m_socket = 0;
        return false;
    }

    asio::write(*m_socket, asio::buffer("connect\n"));
    asio::read_until(*m_socket, b, "\n", error);

    auto response = as_string(b);
    if (response != "connected\n")
    {
        m_socket->close();
        delete m_socket;
        m_socket = 0;
        return false;
    }

    return true;
}

//------------------------------------------------------------

void network_client::disconnect()
{
    if(!m_socket)
        return;

    asio::write(*m_socket, asio::buffer("disconnect\n"));

    delete m_socket;
    m_socket = 0;
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
