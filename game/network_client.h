//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "network_data.h"
#include "miso/client/client_tcp.h"

namespace game
{
//------------------------------------------------------------

class network_client: public noncopyable, public network_interface
{
public:
    bool connect(const char *address, short port);
    void disconnect();

    void start();

    bool is_up() const { return m_client.is_open(); }

    const server_info &get_server_info() const;

    std::string get_error() const { return m_error; }

    ~network_client();

private:
    void update() override;
    void update_post(int dt) override;

private:
    miso::client_tcp m_client;
    server_info m_server_info;
    unsigned int m_last_send_time = 0;
    std::string m_error;
};

//------------------------------------------------------------
}
