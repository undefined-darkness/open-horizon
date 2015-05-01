//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_mesh.h"
#include "location_params.h"

//------------------------------------------------------------

class model
{
public:
    bool load(const char *name, const location_params &params);
    void draw() { m_mesh.draw(0); }

    void set_pos(const nya_math::vec3 &pos) { m_mesh.set_pos(pos); }

public:
    model() {}
    model(const char *name, const location_params &params) { load(name, params); }

private:
    void load_tdp(const std::string &name);

private:
    fhm_mesh m_mesh;
};

//------------------------------------------------------------
