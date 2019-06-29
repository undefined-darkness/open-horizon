//
// open horizon -- undefined_darkness@outlook.com
//

#include "platform.h"
#include "GLFW/glfw3.h"
#include "render/render.h"

//------------------------------------------------------------

std::map<int, bool> platform::m_buttons;
platform::key_callback platform::m_key_callback;

//------------------------------------------------------------

bool platform::init(int width, int height, const char *title, bool fullscreen)
{
    if (!glfwInit())
        return false;
/*
    if (nya_render::get_render_api() == nya_render::render_api_opengl3)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
*/
    glfwSwapInterval(1);

    /*
#ifdef _WIN32
        typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
            wglSwapIntervalEXT(1);
#endif
    */

#if GLFW_VERSION_MAJOR > 3 || GLFW_VERSION_MINOR >= 3
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

    if (fullscreen)
    {
        auto monitor = glfwGetPrimaryMonitor();
        if (!monitor)
            return init(width, height, title, false);

        auto mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        m_window = glfwCreateWindow(mode->width, mode->height, title, monitor, NULL);
    }
    else
        m_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(m_window, key_func);
    glfwMakeContextCurrent(m_window);
    UpdateSize();

    return true;
}

//------------------------------------------------------------

void platform::terminate()
{
    if (!m_window)
        return;

    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = 0;
}

//------------------------------------------------------------

bool platform::should_terminate()
{
    return m_window ? glfwWindowShouldClose(m_window) > 0 : true;
}

//------------------------------------------------------------

void platform::end_frame()
{
    if (!m_window)
        return;

    m_last_buttons = m_buttons;
    glfwPollEvents();

    double mx,my;
    glfwGetCursorPos(m_window, &mx, &my);
    m_mouse_x = int(mx), m_mouse_y = int(my);
    UpdateSize();

    glfwSwapBuffers(m_window);
}

//------------------------------------------------------------

bool platform::was_pressed(int key)
{
    auto k = m_buttons.find(key);
    if (k == m_buttons.end() || !k->second)
        return false;

    auto l = m_last_buttons.find(key);
    return l == m_last_buttons.end() || !l->second;
}

//------------------------------------------------------------

bool platform::get_key(int key) { auto k = m_buttons.find(key); return k == m_buttons.end() ? false : k->second; }
bool platform::get_mouse_lbtn() { return m_window ? glfwGetMouseButton(m_window, 0) > 0 : false; }
bool platform::get_mouse_rbtn() { return m_window ? glfwGetMouseButton(m_window, 1) > 0 : false; }
int platform::get_mouse_x() { return m_mouse_x; }
int platform::get_mouse_y() { return m_mouse_y; }
int platform::get_frame_width() { return m_frame_w; }
int platform::get_frame_height() { return m_frame_h; }
int platform::get_window_width() { return m_window_w; }
int platform::get_window_height() { return m_window_h; }

//------------------------------------------------------------

void platform::set_fullscreen(bool value, int windowed_width, int windowed_height)
{
    if (!m_window)
        return;

    auto monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return;

#if GLFW_VERSION_MAJOR > 3 || GLFW_VERSION_MINOR >= 2
    auto mode = glfwGetVideoMode(monitor);
    const int desktop_width = mode->width, desktop_height = mode->height;

    if (value)
    {
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        const int x = (desktop_width - windowed_width) / 2, y = (desktop_height - windowed_height) / 2;
        glfwSetWindowMonitor(m_window, 0, x, y, windowed_width, windowed_height, 0);
    }
    UpdateSize();
#endif
}

//------------------------------------------------------------

void platform::set_keyboard_callback(key_callback &k) { m_key_callback = k; }

//------------------------------------------------------------

void platform::key_func(GLFWwindow *, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        m_buttons[key] = true;
        if (m_key_callback)
        {
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
                m_key_callback('0' + key - GLFW_KEY_0);
            else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
                m_key_callback('A' + key - GLFW_KEY_A);
            else if (key == GLFW_KEY_PERIOD)
                m_key_callback('.');
            else if (key == GLFW_KEY_SEMICOLON)
                m_key_callback(':');
            else if (key == GLFW_KEY_MINUS)
                m_key_callback('-');
            else if (key == GLFW_KEY_SLASH)
                m_key_callback('/');
            else if (key == GLFW_KEY_SPACE)
                m_key_callback(' ');
            else if (key == GLFW_KEY_BACKSPACE)
                m_key_callback(8);
        }
    }
    else if (action == GLFW_RELEASE)
        m_buttons[key] = false;

#ifdef __APPLE__
    m_buttons[GLFW_KEY_LEFT_CONTROL] = mods & GLFW_MOD_CONTROL;
    m_buttons[GLFW_KEY_LEFT_SHIFT] = mods & GLFW_MOD_SHIFT;
#endif
}

//------------------------------------------------------------

void platform::UpdateSize()
{
    glfwGetWindowSize(m_window, &m_window_w, &m_window_h);
    glfwGetFramebufferSize(m_window, &m_frame_w, &m_frame_h);
}

//------------------------------------------------------------
