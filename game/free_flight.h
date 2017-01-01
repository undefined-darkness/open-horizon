//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class free_flight: public game_mode
{
public:
    virtual void start(const char *plane, int color, const char *location);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end() override;

    void set_location(const char *location);
    void set_plane(const char *plane, int color);

    free_flight(world &w): game_mode(w) {}

protected:
    static bool is_ally(const plane_ptr &a, const plane_ptr &b) { return true; }

    void update_spawn_pos();

    plane_ptr m_player;
    ivalue m_respawn_time;
    nya_math::vec3 m_spawn_pos;
};

//------------------------------------------------------------
}
