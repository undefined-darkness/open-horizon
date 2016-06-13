//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <string>
#include <vector>

namespace game
{
//------------------------------------------------------------

typedef std::vector<std::pair<std::string, std::wstring> > locations_list;

static const locations_list &get_locations_list()
{
    static locations_list list;
    if (list.empty())
    {
#if _DEBUG || DEBUG
        list.push_back({"def", L"Test"}); //fast-loading location for testing
#endif
        list.push_back({"ms01", L"Miami"});
        list.push_back({"ms06", L"Dubai"});
        list.push_back({"ms30", L"Paris"});
        list.push_back({"ms50", L"Tokyo"});
        list.push_back({"ms51", L"Honolulu"});
        list.push_back({"ms08x", L"Beliy Base"});
        list.push_back({"ms09", L"Black Sea"});
        list.push_back({"ms12t", L"Florida"});
        list.push_back({"ms11b", L"Moscow"});
        list.push_back({"ms14", L"Washington"});
/*
         //wrong clouds height
        list.push_back({"ms02", L"Africa"}); //inferno, oil day
        list.push_back({"ms05", L"Africa (night)"}); //blue on blue, oil night

         //too dark
        list.push_back({"ms08", L"Derbent"});
        list.push_back({"ms13", L"Florida Coast"}); //hurricane

         //tex indices idx < size assert
        list.push_back({"ms03", L"Eastern Africa"}); //red moon
        list.push_back({"ms04", L"Mogadiyu"}); //spooky
        list.push_back({"ms07", L"Suez Canal"}); //lock n load
        list.push_back({"ms11a", L"Moscow 2"}); //motherland
        list.push_back({"msop", L"OP"});

         //type 8 chunk assert, kinda small
        list.push_back({"ms12", L"Miami"}); //homefront

         //phys::mesh::load assert
        list.push_back({"ms10", L"Caucasus Region"}); //launch
*/
    }

    return list;
}

//------------------------------------------------------------
}
