//
// open horizon -- undefined_darkness@outlook.com
//

#include "hud.h"
#include "containers/fhm.h"
#include "renderer/shared.h"

namespace gui
{
//------------------------------------------------------------

void hud::load(const char *aircraft_name)
{
    *this = hud(); //release

    if (!aircraft_name)
        return;

    if (!m_common_loaded)
    {
        m_fonts.load("Hud/hudCommon.fhm");
        m_common.load("Hud/hudCommon.fhm");
        m_common_loaded = true;
    }

    m_aircraft.load(("Hud/aircraft/hudAircraft_" + std::string(aircraft_name) + ".fhm").c_str());
}

//------------------------------------------------------------

void hud::draw(const render &r)
{
    const auto green = nya_math::vec4(103,223,144,255)/255.0;

    wchar_t buf[255];
    swprintf(buf, sizeof(buf), L"%d", m_speed);
    m_fonts.draw_text(r, L"SPEED", "NowGE20", r.get_width() * 0.35, r.get_height() * 0.5 - 20, green);
    m_fonts.draw_text(r, buf, "NowGE20", r.get_width() * 0.35, r.get_height() * 0.5, green);
    swprintf(buf, sizeof(buf), L"%d", m_alt);
    m_fonts.draw_text(r, L"ALT", "NowGE20", r.get_width() * 0.6, r.get_height() * 0.5 - 20, green);
    m_fonts.draw_text(r, buf, "NowGE20", r.get_width() * 0.6, r.get_height() * 0.5, green);

    //m_common.debug_draw_tx(r);
    //m_aircraft.debug_draw_tx(r);
    //m_common.debug_draw(r, debug_variable::get());
    //m_aircraft.debug_draw(r, debug_variable::get());

    //m_common.draw(r, 3, green);
    //m_common.draw(r, 158, green);
    //m_common.draw(r, 159, green);
    //m_common.draw(r, 214, green);
}

//------------------------------------------------------------
}
