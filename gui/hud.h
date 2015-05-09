//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"

namespace gui
{
//------------------------------------------------------------

class hud
{
public:
    void load(const char *aircraft_name);
    void draw(const render &r);
    void set_text(const char *id, const wchar_t *text);

private:
    bool load_widget(const char *name);

private:
};

//------------------------------------------------------------
}
