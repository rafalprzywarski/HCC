#include <cstdint>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>

namespace
{

std::string arc_vertex_shader_source =
"#version 100\n"
"uniform mat4 u_Projection;\n"
"attribute vec4 a_Position;\n"
"attribute vec4 a_Color;\n"
"attribute vec4 a_Circle;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_Position;\n"
"varying vec4 v_Circle;\n"
"void main()\n"
"{\n"
"    v_Color = a_Color;\n"
"    v_Position = a_Position.xy;\n"
"    v_Circle = a_Circle;\n"
"    gl_Position = u_Projection * a_Position;\n"
"}\n";

std::string arc_fragment_shader_source =
"#version 100\n"
"precision mediump float;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_Position;\n"
"varying vec4 v_Circle;\n"
"float smoothStep(float edge0, float edge1, float x)\n"
"{\n"
"    float t = clamp((x - edge0) / (edge1  - edge0), 0.0, 1.0);\n"
"    return t * t * (3.0 - 2.0 * t);\n"
"}\n"
"float smoothSample()\n"
"{\n"
"    float sample = smoothStep(v_Circle.z - 0.7071, v_Circle.z + 0.7071, distance(v_Position, v_Circle.xy));\n"
"    return 0.5 - v_Circle.w * (sample - 0.5);\n"
"}\n"
"void main()\n"
"{\n"
"    float alpha = sqrt(smoothSample());\n"
"    gl_FragColor = vec4(v_Color.xyz, v_Color.w * alpha);\n"
"}\n";

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

struct State
{
    EGLDisplay display{};
    std::uint32_t display_width{}, display_height{};
    EGLSurface surface{};
    EGL_DISPMANX_WINDOW_T window{};
    std::vector<GLfloat> arc_vertices;
    std::vector<GLfloat> arc_colors;
    std::vector<GLfloat> arc_circles;
    GLuint vertex_buffer{};
    GLuint color_buffer{};
    GLuint circle_buffer{};

    GLuint vertex_shader{};
    GLuint fragment_shader{};
    GLuint program{};

    GLfloat projection[16]{};
};

State *state = nullptr;

void init_projection()
{
    auto width = state->display_width;
    auto height = state->display_height;
    GLfloat m[16] = {
        2.0f / width, 0,             0, 0,
        0,            2.0f / height, 0, 0,
        0,            0,             1, 0,
        -1,           -1,            0, 1
    };
    std::copy_n(m, 16, state->projection);
}

void init_buffers()
{
    glGenBuffers(1, &state->vertex_buffer);
    glGenBuffers(1, &state->color_buffer);
    glGenBuffers(1, &state->circle_buffer);
}

GLuint create_shader(GLenum type, const std::string& source)
{
    auto id = glCreateShader(type);
    auto source_ = source.c_str();
    glShaderSource(id, 1, &source_, nullptr);
    glCompileShader(id);
    return id;
}

GLuint create_program(GLuint vs, GLuint fs)
{
    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    return program;
}

void init_shaders()
{
    state->vertex_shader = create_shader(GL_VERTEX_SHADER, arc_vertex_shader_source);
    state->fragment_shader = create_shader(GL_FRAGMENT_SHADER, arc_fragment_shader_source);
    state->program = create_program(state->vertex_shader, state->fragment_shader);
}

void set_vertex_attrib(GLuint program, const char *name, int size, GLuint buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    auto location = glGetAttribLocation(program, name);
    glVertexAttribPointer(location, size, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(location);
}

void set_buffer(GLuint buffer, const std::vector<GLfloat>& data)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data[0]) * data.size(), data.data(), GL_DYNAMIC_DRAW);
}

}

extern "C"
{

std::int64_t initialize()
{
    std::unique_ptr<State> state{new State};
    bcm_host_init();

    graphics_get_display_size(0, &state->display_width, &state->display_height);

    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(state->display, nullptr, nullptr);

    EGLint num_configs;
    EGLConfig config;
    eglChooseConfig(state->display, DISPLAY_ATTRIBUTES, &config, 1, &num_configs);

    auto context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, CONTEXT_ATTRIBUTES);

    VC_RECT_T src_rect, dst_rect;
    vc_dispmanx_rect_set(&src_rect, 0, 0, state->display_width << 16, state->display_height << 16);
    vc_dispmanx_rect_set(&dst_rect, 0, 0, state->display_width, state->display_height);

    auto dispman_display = vc_dispmanx_display_open(0);
    auto dispman_update = vc_dispmanx_update_start(0);
    auto dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 1, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, 0, 0, DISPMANX_NO_ROTATE);

    state->window.element = dispman_element;
    state->window.width = state->display_width;
    state->window.height = state->display_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    state->surface = eglCreateWindowSurface(state->display, config, &state->window, NULL);
    eglMakeCurrent(state->display, state->surface, state->surface, context);
    eglSwapInterval(state->display, 1);

    ::state = state.release();

    init_shaders();
    init_buffers();
    init_projection();

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_DST_ALPHA);

    return 0;
}

std::int64_t shutdown()
{
    if (!state)
        return 0;
    eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(state->display);
    eglReleaseThread();
    delete state;
    state = nullptr;
    return 0;
}

std::int64_t clear()
{
    if (!state)
        return 0;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

std::int64_t arc(
    std::int64_t x0, std::int64_t y0,
    std::int64_t x1, std::int64_t y1,
    std::int64_t r, std::int64_t g, std::int64_t b,
    std::int64_t cx, std::int64_t cy, std::int64_t cr, std::int64_t corient)
{
    if (!state)
        return 0;
    state->arc_vertices.push_back(x0); state->arc_vertices.push_back(y0);
    state->arc_vertices.push_back(x1); state->arc_vertices.push_back(y0);
    state->arc_vertices.push_back(x1); state->arc_vertices.push_back(y1);
    state->arc_vertices.push_back(x0); state->arc_vertices.push_back(y0);
    state->arc_vertices.push_back(x1); state->arc_vertices.push_back(y1);
    state->arc_vertices.push_back(x0); state->arc_vertices.push_back(y1);
    for (int i = 0; i < 6; ++i)
    {
        state->arc_colors.push_back(r / 255.0f);
        state->arc_colors.push_back(g / 255.0f);
        state->arc_colors.push_back(b / 255.0f);
        state->arc_colors.push_back(1);
        state->arc_circles.push_back(cx);
        state->arc_circles.push_back(cy);
        state->arc_circles.push_back(cr);
        state->arc_circles.push_back(corient);
    }
    return 0;
}

std::int64_t render()
{
    if (!state)
        return 0;
    set_buffer(state->vertex_buffer, state->arc_vertices);
    set_buffer(state->color_buffer, state->arc_colors);
    set_buffer(state->circle_buffer, state->arc_circles);

    glUseProgram(state->program);
    glUniformMatrix4fv(glGetUniformLocation(state->program, "u_Projection"), 1, false, state->projection);
    set_vertex_attrib(state->program, "a_Position", 2, state->vertex_buffer);
    set_vertex_attrib(state->program, "a_Color", 4, state->color_buffer);
    set_vertex_attrib(state->program, "a_Circle", 4, state->circle_buffer);

    glDrawArrays(GL_TRIANGLES, 0, state->arc_vertices.size() / 2);
    state->arc_vertices.clear();
    state->arc_colors.clear();
    state->arc_circles.clear();

    return 0;
}

std::int64_t swap_buffers()
{
    if (!state)
        return 0;
    eglSwapBuffers(state->display, state->surface);
    return 0;
}

std::int64_t get_display_width()
{
    if (!state)
        return 0;
    return state->display_width;
}
std::int64_t get_display_height()
{
    if (!state)
        return 0;
    return state->display_height;
}

}
