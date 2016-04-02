//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <string>
#include <functional>
#include <map>

//------------------------------------------------------------

struct GLFWwindow;

class platform
{
public:
    bool init(int width, int height, const char *title);
    void terminate();
    bool should_terminate();
    void end_frame();
    bool was_pressed(int key);

    bool get_key(int key);
    bool get_mouse_lbtn();
    bool get_mouse_rbtn();
    int get_mouse_x();
    int get_mouse_y();
    int get_width();
    int get_height();

    typedef std::function<void(char c)> key_callback;
    void set_keyboard_callback(key_callback &k);

    static std::string open_folder_dialog();
    static bool show_msgbox(std::string message);

    platform(): m_window(0) {}

private:
    static void key_func(GLFWwindow *, int key, int scancode, int action, int mods);

private:
    GLFWwindow *m_window;
    int m_mouse_x, m_mouse_y;
    int m_screen_w, m_screen_h;
    static std::map<int, bool> m_buttons;
    std::map<int, bool> m_last_buttons;
    static key_callback m_key_callback;
};

//------------------------------------------------------------
