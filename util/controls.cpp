//
// open horizon -- undefined_darkness@outlook.com
//

#include "controls.h"
#include "config.h"
#include "xml.h"
#include <math.h>

#include "game/game.h"
#include "gui/menu.h"

//------------------------------------------------------------

void joystick_config::init(const char *name)
{
    pugi::xml_document doc;
    if (!name || !load_xml("joystick.xml", doc))
        return;

    std::string name_str(name);
    std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);

#ifdef _WIN32
    const std::string os("windows");
#elif defined __APPLE__
    const std::string os("osx");
#else
    const std::string os("linux");
#endif

    pugi::xml_node root = doc.first_child();
    for (pugi::xml_node j = root.child("joystick"); j; j = j.next_sibling("joystick"))
    {
        std::string name_part = j.attribute("name").as_string("");
        std::transform(name_part.begin(), name_part.end(), name_part.begin(), ::tolower);

        if (name_str.find(name_part) == std::string::npos)
            continue;

        for (pugi::xml_node c = j.first_child(); c; c = c.next_sibling())
        {
            auto aos = c.attribute("os");
            if (aos && os != aos.as_string())
                continue;

            std::string name(c.name());
            std::string type; int idx = 0;
            for (auto &n: name)
            {
                if (!isalpha(n))
                {
                    idx = atoi(&n);
                    break;
                }

                type.push_back(n);
            }

            auto cmd = convert_cmd(c.attribute("control").as_string(""));

            if (type == "axis")
                m_axes.push_back({idx, c.attribute("dead").as_float(), c.attribute("invert").as_bool(), cmd});
            else if (type == "btn")
                m_buttons.push_back({idx,cmd});
        }
    }

    update_config();
}

//------------------------------------------------------------

void joystick_config::update_config()
{
    bool clear = true;
    for (auto &v: config::get_vars())
    {
        if (v.first.compare(0, 4, "joy_") != 0)
            continue;

        if (clear)
        {
            m_buttons.clear();
            m_axes.clear();
            clear = false;
        }

        auto cmd = convert_cmd(v.first.substr(4));

        if (v.second.compare(0, 3, "btn") == 0)
        {
            btn b;
            b.idx = atoi(v.second.c_str() + 3);
            b.cmd = cmd;
            m_buttons.push_back(b);
            continue;
        }

        if (v.second.compare(1, 4, "axis") == 0)
        {
            axis a;
            a.idx = atoi(v.second.c_str() + 5);
            a.cmd = cmd;
            a.inverted = v.second.front() == '-';
            a.deadzone = v.second.front() == '-' || v.second.front() == '+' ? 0.05f : 0.5f;
            m_axes.push_back(a);
            continue;
        }
    }
}

//------------------------------------------------------------

void joystick_config::update(const float *axes, int axes_count, const unsigned char *btns, int btns_count)
{
    m_joy_axes.resize(axes_count);
    for (int i = 0; i < axes_count; ++i)
        m_joy_axes[i] = axes[i];

    m_joy_btns.resize(btns_count);
    for (int i = 0; i < btns_count; ++i)
        m_joy_btns[i] = btns[i] > 0;
}

//------------------------------------------------------------

void joystick_config::apply_controls(game::plane_controls &controls, bool &pause)
{
    for (int i = 0; i < (int)m_joy_axes.size(); ++i)
    {
        for (auto &a: m_axes)
        {
            if (a.idx != i)
                continue;

            float v = m_joy_axes[i];
            if (fabsf(v) < a.deadzone)
                continue;

            if (a.inverted)
                v = -v;

            switch (a.cmd)
            {
                case '+ppc': if (v > 0.0f) controls.rot.x = v; break;
                case '-ppc': if (v > 0.0f) controls.rot.x = -v; break;
                case '+prl': if (v > 0.0f) controls.rot.z = v; break;
                case '-prl': if (v > 0.0f) controls.rot.z = -v; break;
                case '+pyw': if (v > 0.0f) controls.rot.x = v; break;
                case '-pyw': if (v > 0.0f) controls.rot.x = -v; break;

                case 'pyaw': controls.rot.y = v; break;
                case 'ppch': controls.rot.x = v; break;
                case 'prll': controls.rot.z = v; break;
                case 'thrt': controls.throttle = v; break;

                case '+cpc': if (v > 0.0f) controls.cam_rot.x = v; break;
                case '-cpc': if (v > 0.0f) controls.cam_rot.x = -v; break;
                case '+cyw': if (v > 0.0f) controls.cam_rot.y = v; break;
                case '-cyw': if (v > 0.0f) controls.cam_rot.y = -v; break;

                default: break;
            }
        }
    }

    for (int i = 0; i < (int)m_joy_btns.size(); ++i)
    {
        if (!m_joy_btns[i])
            continue;

        for (auto &b: m_buttons)
        {
            if (b.idx != i)
                continue;

            switch (b.cmd)
            {
                case '+ppc': controls.rot.x = 1.0; break;
                case '-ppc': controls.rot.x = -1.0; break;
                case '+prl': controls.rot.z = 1.0; break;
                case '-prl': controls.rot.z = -1.0; break;
                case '+pyw': controls.rot.y = 1.0; break;
                case '-pyw': controls.rot.y = -1.0; break;

                case 'cchg': controls.change_camera = true; break;
                case 'wchg': controls.change_weapon = true; break;
                case 'tchg': controls.change_target = true; break;
                case 'rchg': controls.change_radar = true; break;
                case 'msle': controls.missile = true; break;
                case 'flrs': controls.flares = true; break;
                case 'mgun': controls.mgun = true; break;
                case 'brke': controls.brake = true; break;
                case 'thrt': controls.throttle = true; break;
                case 'paus': pause = true; break;

                case '+cpc': controls.cam_rot.x = 1.0; break;
                case '-cpc': controls.cam_rot.x = -1.0; break;
                case '+cyw': controls.cam_rot.y = 1.0; break;
                case '-cyw': controls.cam_rot.y = -1.0; break;

                default: break;
            }
        }
    }

    //if (fabsf(axis[2]) > joy_dead_zone) scene.camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
    //if (fabsf(axis[3]) > joy_dead_zone) scene.camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
}

//------------------------------------------------------------

void joystick_config::apply_controls(gui::menu_controls &controls)
{
    for (int i = 0; i < (int)m_joy_axes.size(); ++i)
    {
        for (auto &a: m_axes)
        {
            if (a.idx != i)
                continue;

            if (fabsf(m_joy_axes[i]) < a.deadzone)
                continue;

            float v = m_joy_axes[i];
            if (a.inverted)
                v = -v;

            if (a.cmd == 'm_ud')
            {
                if (v > 0.0f)
                    controls.up = true;
                else
                    controls.down = true;
            }
            else if (a.cmd == 'm_lr')
            {
                if (v > 0.0f)
                    controls.right = true;
                else
                    controls.left = true;
            }
        }
    }

    for (int i = 0; i < (int)m_joy_btns.size(); ++i)
    {
        if (!m_joy_btns[i])
            continue;

        //printf("btn %2d\n",i);

        for (auto &b: m_buttons)
        {
            if (b.idx != i)
                continue;

            switch (b.cmd)
            {
                case 'm_up': controls.up = true; break;
                case 'm_dn': controls.down = true; break;
                case 'm_lf': controls.left = true; break;
                case 'm_rt': controls.right = true; break;
                case 'm_pr': controls.prev = true; break;
                case 'm_nx': controls.next = true; break;
                default: break;
            }
        }
    }
}

//------------------------------------------------------------

unsigned int joystick_config::convert_cmd(const std::string &cmd)
{
    if (cmd == "yaw") return 'pyaw';
    if (cmd == "+yaw") return '+pyw';
    if (cmd == "-yaw") return '-pyw';
    if (cmd == "pitch") return 'ppch';
    if (cmd == "+pitch") return '+ppc';
    if (cmd == "-pitch") return '-ppc';
    if (cmd == "roll") return 'prll';
    if (cmd == "+roll") return '+prl';
    if (cmd == "-roll") return '-prl';
    if (cmd == "camera_yaw") return 'cyaw';
    if (cmd == "+camera_yaw") return '+cyw';
    if (cmd == "-camera_yaw") return '-cyw';
    if (cmd == "camera_pitch") return 'cpch';
    if (cmd == "+camera_pitch") return '+cpc';
    if (cmd == "-camera_pitch") return '-cpc';
    if (cmd == "change_camera") return 'cchg';
    if (cmd == "change_weapon") return 'wchg';
    if (cmd == "change_target") return 'tchg';
    if (cmd == "change_radar") return 'rchg';
    if (cmd == "pause") return 'paus';
    if (cmd == "brake") return 'brke';
    if (cmd == "throttle") return 'thrt';
    if (cmd == "missile") return 'msle';
    if (cmd == "flares") return 'flrs';
    if (cmd == "mgun") return 'mgun';

    if (cmd == "menu_up_down") return 'm_ud';
    if (cmd == "menu_up") return 'm_up';
    if (cmd == "menu_down") return 'm_dn';
    if (cmd == "menu_left") return 'm_lf';
    if (cmd == "menu_right") return 'm_rt';
    if (cmd == "menu_prev") return 'm_pr';
    if (cmd == "menu_next") return 'm_nx';

    return 0;
}

//------------------------------------------------------------
