//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    #define LEGACY_QGL
#endif

#ifdef LEGACY_QGL
    #include <QGLWidget>
    #define QOpenGLWidget QGLWidget
#else
    #include <QOpenGLWidget>
#endif

#include "renderer/location.h"
#include "renderer/model.h"
#include "phys/physics.h"
#include "render/debug_draw.h"
#include "game/objects.h"
#include <functional>
#include <set>

//------------------------------------------------------------

class scene_view : public QOpenGLWidget
{
    Q_OBJECT

public:
    scene_view(QWidget *parent = NULL);
    std::function<void()> update_objects_tree;
    std::function<void(std::string, int)> set_selection;

public:
    void load_location(std::string name);

    enum mode
    {
        mode_add,
        mode_edit,
        mode_path,
        mode_zone,
        mode_other
    };

    void set_mode(mode m) { m_mode = m; }
    void set_selected_add(std::string str);

    void clear_selection();
    void select(std::string group, int idx);
    void set_focus(std::string group, int idx);
    void delete_selected();

    struct base
    {
        std::string name;
        bool active = true;
        std::map<std::string, std::string> attributes;
    };

    base &get_selected();

    struct object: public base
    {
        std::string id;
        nya_math::vec3 pos;
        nya_math::angle_deg yaw;
        float y = 0.0f, dy = 0.0f;

        object() { attributes["align"] = "enemy"; }
    };

    const std::vector<object> get_objects() const { return m_objects; }
    void clear_objects() { m_objects.clear(); }
    void add_object(const object &o);

    void reorder_objects(std::string group, std::vector<int> from, int to);

    const object &get_player() const { return m_player; }
    void set_player(const object &p) { m_player = p; }

    struct path: public base
    {
        std::vector<nya_math::vec4> points; //point, height
    };

    std::vector<path> &get_paths() { return m_paths; }

    struct zone: public base
    {
        nya_math::vec3 pos;
        float radius = 50.0f;

        zone() { attributes["display"] = "none"; }
    };

    const std::vector<zone> get_zones() const { return m_zones; }
    void clear_zones() { m_zones.clear(), m_zones_internal.clear(); }
    void add_zone(const zone &z);

    float get_height(float x, float z) const { return m_location_phys.get_height(x, z, true); }

private:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

private:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    QSize minimumSizeHint() const { return QSize(50, 50); }

private:
    nya_math::vec3 world_cursor_pos() const;
    void draw(const object &o);
    void cache_mesh(std::string name);
    struct zone_internal;
    void update_zone(const zone &z, zone_internal &zi);
    void add_to_draw(const zone_internal &zi, nya_math::vec4 color);

private:
    renderer::location m_location;
    phys::world m_location_phys;
    nya_render::debug_draw m_dd;
    nya_math::angle_deg m_camera_yaw, m_camera_pitch;
    nya_math::vec3 m_camera_pos;
    int m_mouse_x = 0, m_mouse_y = 0;
    nya_math::vec3 m_cursor_pos;
    mode m_mode;
    std::map<std::string, renderer::model> m_models;
    object m_selected_add;
    object m_player;
    std::vector<object> m_objects;
    std::vector<path> m_paths;
    struct zone_internal { std::vector<nya_math::vec3> verts; };
    std::vector<zone_internal> m_zones_internal;
    std::vector<zone> m_zones;
    zone m_zone_add;

    std::map<std::string, std::set<int> > m_selection;
};

//------------------------------------------------------------

inline nya_math::vec3 pos_h(const nya_math::vec3 &pos, float height)
{
    nya_math::vec3 r = pos;
    r.y += height;
    return r;
}

//------------------------------------------------------------
