//
// open horizon -- undefined_darkness@outlook.com
//

#include "location.h"

//------------------------------------------------------------

bool is_native_location(std::string name)
{
    static std::string cache;
    if (cache == name)
        return true;

    if (nya_resources::get_resources_provider().has(("locations/" + name).c_str()))
    {
        cache = name;
        return true;
    }

    return false;
}

//------------------------------------------------------------

nya_resources::zip_resources_provider &get_native_location_provider(std::string name)
{
    static std::string cache;
    static nya_resources::zip_resources_provider zprov;
    if (cache == name)
        return zprov;

    zprov.close_archive();
    zprov.open_archive(("locations/" + name).c_str());
    return zprov;
}

//------------------------------------------------------------
