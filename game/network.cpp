//
// open horizon -- undefined_darkness@outlook.com
//

#include "network.h"
#include "network_helpers.h"
#include "miso/socket/socket.h"
#include "miso/ipv4.h"
#include "system/system.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

#include <thread>
#include <chrono>

namespace game
{
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
}
