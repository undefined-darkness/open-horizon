//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <string>
#include <vector>
#include <set>
#include "util/util.h"
#include "util/xml.h"
#include "extensions/zip_resources_provider.h"

namespace game
{
//------------------------------------------------------------

typedef std::vector<std::pair<std::string, std::string> > locations_list;

static const locations_list &get_locations_list()
{
    static locations_list list;
    if (list.empty())
    {
#if _DEBUG || DEBUG
        list.push_back({"def", "Test"}); //fast-loading location for testing
#endif
        list.push_back({"ms01", "Miami"});
        list.push_back({"ms06", "Dubai"});
        list.push_back({"ms30", "Paris"});
        list.push_back({"ms50", "Tokyo"});
        list.push_back({"ms51", "Honolulu"});
        list.push_back({"ms08x", "Beliy Base"});
        list.push_back({"ms09", "Black Sea"});
        list.push_back({"ms12t", "Florida"});
        list.push_back({"ms11b", "Moscow"});
        list.push_back({"ms14", "Washington"});
/*
         //wrong clouds height
        list.push_back({"ms02", "Africa"}); //inferno, oil day
        list.push_back({"ms05", "Africa (night)"}); //blue on blue, oil night

         //too dark
        list.push_back({"ms08", "Derbent"});
        list.push_back({"ms13", "Florida Coast"}); //hurricane

         //tex indices idx < size assert
        list.push_back({"ms03", "Eastern Africa"}); //red moon
        list.push_back({"ms04", "Mogadiyu"}); //spooky
        list.push_back({"ms07", "Suez Canal"}); //lock n load
        list.push_back({"ms11a", "Moscow 2"}); //motherland
        list.push_back({"msop", "OP"});

         //type 8 chunk assert, kinda small
        list.push_back({"ms12", "Miami"}); //homefront

         //phys::mesh::load assert
        list.push_back({"ms10", "Caucasus Region"}); //launch
*/

        const std::string folder = "locations/";
        auto custom_locations = list_files(folder);
        for (auto &id: custom_locations)
        {
            nya_resources::zip_resources_provider zprov;
            if (!zprov.open_archive(id.c_str()))
                continue;

            pugi::xml_document doc;
            if (!load_xml(zprov.access("info.xml"), doc))
                continue;

            auto root = doc.first_child();
            std::string name = root.attribute("name").as_string();
            if (name.empty())
                continue;

            list.push_back({id.substr(folder.length()), name});
        }
    }

    return list;
}

//------------------------------------------------------------
}
