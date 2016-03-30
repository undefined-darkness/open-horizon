//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <vector>

//------------------------------------------------------------

//ToDo - dependency inversion?

namespace gui { struct menu_controls; }
namespace game { struct plane_controls; }

//------------------------------------------------------------

class joystick_config
{
public:
    void init(const char *name);
    void update(const float *axes, int axes_count, const unsigned char *btns, int btns_count);
    void apply_controls(game::plane_controls &controls, bool &pause);
    void apply_controls(gui::menu_controls &controls);

private:
    static unsigned int convert_cmd(const std::string &cmd);

private:
    struct axis
    {
        int idx;
        float deadzone;
        bool inverted;
        unsigned int cmd;
    };
    
    std::vector<axis> m_axes;
    
    struct btn
    {
        int idx;
        unsigned int cmd;
    };
    
    std::vector<btn> m_buttons;
    
private:
    std::vector<float> m_joy_axes;
    std::vector<bool> m_joy_btns;
};

//------------------------------------------------------------
