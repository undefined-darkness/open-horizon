//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "util/params.h"
#include "memory/shared_ptr.h"
#include "location.h"
#include "model.h"
#include "clouds.h"
#include <functional>
#include <vector>

namespace renderer
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef params::fvalue fvalue;

//------------------------------------------------------------

template<typename t> class ptr: public nya_memory::shared_ptr<t>
{
    friend class world;

public:
    ptr(): nya_memory::shared_ptr<t>() {}
    ptr(const ptr &p): nya_memory::shared_ptr<t>(p) {}

private:
    explicit ptr(bool): nya_memory::shared_ptr<t>(t()) {}
};

//------------------------------------------------------------

struct object
{
    model mdl;
};

typedef ptr<object> object_ptr;

//------------------------------------------------------------

typedef ptr<aircraft> aircraft_ptr;

//------------------------------------------------------------

struct missile: public object
{
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    virtual void set_location(const char *name);
    const location_params &get_location_params() { return m_location.get_params(); }
    virtual aircraft_ptr add_aircraft(const char *name, int color, bool player);
    virtual void update(int dt);
    aircraft_ptr get_player_aircraft() { return m_player_aircraft; }

protected:
    std::vector<aircraft_ptr> m_aircrafts;

protected:
    std::string m_location_name;
    location m_location;
    effect_clouds m_clouds;
    aircraft_ptr m_player_aircraft;
};

//------------------------------------------------------------
}
