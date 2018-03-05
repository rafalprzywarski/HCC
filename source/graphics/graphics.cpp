#include <cstdint>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string>
#include <vector>
#include <memory>
#include <array>

namespace
{

const std::string arc_vertex_shader_source =
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

const std::string arc_fragment_shader_source =
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

const std::string textured_vertex_shader_source =
"#version 100\n"
"uniform mat4 u_Projection;\n"
"attribute vec4 a_Position;\n"
"attribute vec4 a_Color;\n"
"attribute vec2 a_TexCoord;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
"void main()\n"
"{\n"
"    v_Color = a_Color;\n"
"    v_TexCoord = a_TexCoord;\n"
"    gl_Position = u_Projection * a_Position;\n"
"}\n";

const std::string textured_fragment_shader_source =
"#version 100\n"
"precision mediump float;\n"
"uniform sampler2D u_Texture;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(u_Texture, v_TexCoord) * v_Color;\n"
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

const std::array<std::uint8_t, 4 * 2 * 4> texture_example{{
    255, 0, 0, 255, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0,
    0,   0, 0, 255, 0,   255, 0, 255, 255, 255, 0,   255, 255, 255, 0,   0,
}};

struct TexturedDrawCall
{
    GLint offset{};
    GLsizei size{};
    GLuint texture{};
    TexturedDrawCall() = default;
    TexturedDrawCall(GLint offset, GLsizei size, GLuint texture) : offset(offset), size(size), texture(texture) { }
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
    std::vector<GLfloat> textured_vertices{
        100, 100,
        200, 100,
        200, 200,
        100, 100,
        200, 200,
        100, 200};
    std::vector<GLfloat> textured_colors{
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f};
    std::vector<GLfloat> textured_coords{
        0, 0,
        5, 0,
        5, 10,
        0, 0,
        5, 10,
        0, 10};
    GLuint arc_vertex_buffer{};
    GLuint arc_color_buffer{};
    GLuint arc_circle_buffer{};
    GLuint textured_vertex_buffer{};
    GLuint textured_color_buffer{};
    GLuint textured_coord_buffer{};
    std::vector<TexturedDrawCall> textured_draw_calls;

    GLuint arc_vertex_shader{};
    GLuint arc_fragment_shader{};
    GLuint arc_program{};
    GLuint textured_vertex_shader{};
    GLuint textured_fragment_shader{};
    GLuint textured_program{};

    std::array<GLfloat, 16> projection{};
};

State *state = nullptr;

void init_projection()
{
    auto width = state->display_width;
    auto height = state->display_height;
    std::array<GLfloat,  16> m = {{
        2.0f / width, 0,             0, 0,
        0,            2.0f / height, 0, 0,
        0,            0,             1, 0,
        -1,           -1,            0, 1
    }};
    state->projection = m;
}

void init_buffers()
{
    glGenBuffers(1, &state->arc_vertex_buffer);
    glGenBuffers(1, &state->arc_color_buffer);
    glGenBuffers(1, &state->arc_circle_buffer);
    glGenBuffers(1, &state->textured_vertex_buffer);
    glGenBuffers(1, &state->textured_color_buffer);
    glGenBuffers(1, &state->textured_coord_buffer);
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
    state->arc_vertex_shader = create_shader(GL_VERTEX_SHADER, arc_vertex_shader_source);
    state->arc_fragment_shader = create_shader(GL_FRAGMENT_SHADER, arc_fragment_shader_source);
    state->arc_program = create_program(state->arc_vertex_shader, state->arc_fragment_shader);

    state->textured_vertex_shader = create_shader(GL_VERTEX_SHADER, textured_vertex_shader_source);
    state->textured_fragment_shader = create_shader(GL_FRAGMENT_SHADER, textured_fragment_shader_source);
    state->textured_program = create_program(state->textured_vertex_shader, state->textured_fragment_shader);
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

GLuint create_texture(GLsizei width, GLsizei height, const void *data)
{
    GLuint texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return texture;
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

    auto texture = create_texture(4, 2, texture_example.data());
    ::state->textured_draw_calls.emplace_back(0, 6, texture);

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
    std::array<GLfloat, 12> vertices{{
        GLfloat(x0), GLfloat(y0), GLfloat(x1), GLfloat(y0), GLfloat(x1), GLfloat(y1),
        GLfloat(x0), GLfloat(y0), GLfloat(x1), GLfloat(y1), GLfloat(x0), GLfloat(y1)}};
    std::array<GLfloat, 4> color{{r / 255.0f, g / 255.0f, b / 255.0f, 1.0f}};
    std::array<GLfloat, 4> circle{{GLfloat(cx), GLfloat(cy), GLfloat(cr), GLfloat(corient)}};
    state->arc_vertices.insert(end(state->arc_vertices), begin(vertices), end(vertices));
    for (int i = 0; i < 6; ++i)
    {
        state->arc_colors.insert(end(state->arc_colors), begin(color), end(color));
        state->arc_circles.insert(end(state->arc_circles), begin(circle), end(circle));
    }
    return 0;
}

std::int64_t render()
{
    if (!state)
        return 0;
    set_buffer(state->arc_vertex_buffer, state->arc_vertices);
    set_buffer(state->arc_color_buffer, state->arc_colors);
    set_buffer(state->arc_circle_buffer, state->arc_circles);

    glUseProgram(state->arc_program);
    glUniformMatrix4fv(glGetUniformLocation(state->arc_program, "u_Projection"), 1, false, state->projection.data());
    set_vertex_attrib(state->arc_program, "a_Position", 2, state->arc_vertex_buffer);
    set_vertex_attrib(state->arc_program, "a_Color", 4, state->arc_color_buffer);
    set_vertex_attrib(state->arc_program, "a_Circle", 4, state->arc_circle_buffer);

    glDrawArrays(GL_TRIANGLES, 0, state->arc_vertices.size() / 2);
    state->arc_vertices.clear();
    state->arc_colors.clear();
    state->arc_circles.clear();

    set_buffer(state->textured_vertex_buffer, state->textured_vertices);
    set_buffer(state->textured_color_buffer, state->textured_colors);
    set_buffer(state->textured_coord_buffer, state->textured_coords);

    glUseProgram(state->textured_program);
    glUniformMatrix4fv(glGetUniformLocation(state->textured_program, "u_Projection"), 1, false, state->projection.data());
    set_vertex_attrib(state->textured_program, "a_Position", 2, state->textured_vertex_buffer);
    set_vertex_attrib(state->textured_program, "a_Color", 4, state->textured_color_buffer);
    set_vertex_attrib(state->textured_program, "a_TexCoord", 2, state->textured_coord_buffer);

    for (auto& dc : state->textured_draw_calls)
    {
        glUniform1i(glGetUniformLocation(state->arc_program, "u_Texture"), dc.texture);
        glDrawArrays(GL_TRIANGLES, dc.offset, dc.size);
    }

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
