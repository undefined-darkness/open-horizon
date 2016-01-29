//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <vector>
#include <string>

namespace game
{
//------------------------------------------------------------

bool http_get(const char *url, const char *arg, std::string &data);

//------------------------------------------------------------

class servers_list
{
public:
    bool request_update();
    bool is_ready() const;
    typedef std::vector<std::string> list;
    void get_list(list &l) const;

public:
    bool register_server();

private:
    list m_list;
    bool m_ready = true;
};

//------------------------------------------------------------
}
