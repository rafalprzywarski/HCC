#include <cstdint>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

const EGLint DISPLAY_ATTRIBUTES[] =
{
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      8,
    EGL_NONE
};

const EGLint CONTEXT_ATTRIBUTES[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

EGLDisplay display;
std::uint32_t display_width, display_height;
EGLSurface surface;
EGL_DISPMANX_WINDOW_T window;

extern "C"
{

std::int64_t initialize()
{
    bcm_host_init();

    if (graphics_get_display_size(0, &display_width, &display_height) < 0)
        return -1;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint num_configs;
    EGLConfig config;
    if (!eglChooseConfig(display, DISPLAY_ATTRIBUTES, &config, 1, &num_configs) || num_configs == 0)
        return -2;

    auto context = eglCreateContext(display, config, EGL_NO_CONTEXT, CONTEXT_ATTRIBUTES);

    VC_RECT_T src_rect, dst_rect;
    vc_dispmanx_rect_set(&src_rect, 0, 0, display_width << 16, display_height << 16);
    vc_dispmanx_rect_set(&dst_rect, 0, 0, display_width, display_height);

    auto dispman_display = vc_dispmanx_display_open(0);
    auto dispman_update = vc_dispmanx_update_start(0);
    auto dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 1, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, 0, 0, DISPMANX_NO_ROTATE);

    window.element = dispman_element;
    window.width = display_width;
    window.height = display_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    surface = eglCreateWindowSurface(display, config, &window, NULL);
    if (eglGetError() != EGL_SUCCESS)
        return -3;
    eglMakeCurrent(display, surface, surface, context);
    if (eglGetError() != EGL_SUCCESS)
        return -4;
    eglSwapInterval(display, 1);
    return 0;
}

std::int64_t shutdown()
{
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(display);
    eglReleaseThread();
    return 0;
}

std::int64_t clear()
{
    glClearColor(0.773f, 0.102f, 0.290f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

std::int64_t swap_buffers()
{
    eglSwapBuffers(display, surface);
    return 0;
}

std::int64_t get_display_width() { return display_width; }
std::int64_t get_display_height() { return display_height; }

}
