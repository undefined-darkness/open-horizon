//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include "location_params.h"

#include "memory/memory_reader.h"
#include "shared.h"

namespace renderer
{

//------------------------------------------------------------

class particles_bcf
{
public:
    bool load(const char *name);

private:
};

//------------------------------------------------------------

class particles_bfx
{
public:
    bool load(const char *name);

private:
};

//------------------------------------------------------------
}
