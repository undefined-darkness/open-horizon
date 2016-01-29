//
// open horizon -- undefined_darkness@outlook.com
//

#include "network.h"

#include "asio.hpp"
#include <iostream>

using asio::ip::tcp;

namespace game
{
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
        std::ostringstream ss;
        ss << &response;
        data.append(ss.str());
    }

    return error == asio::error::eof;
}

//------------------------------------------------------------

bool servers_list::request_update()
{
    if (!m_ready)
        return false;

    m_list.clear();
    std::string data;
    bool result = http_get("razgriz.pythonanywhere.com", "/get?app=open-horizon&version=1", data);
    if (!result)
        return false;

    std::stringstream ss(data);
    std::string to;
    while(std::getline(ss,to,'\n'))
        m_list.push_back(to);

    //ToDo: check servers

    return result;
}

//------------------------------------------------------------

bool servers_list::register_server()
{
    std::string data;
    return http_get("razgriz.pythonanywhere.com", "/log?app=open-horizon&version=1", data);
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
}
