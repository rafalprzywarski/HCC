#include <cstdint>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <iostream>

namespace
{
#define HCC_GRAPHICS_TO_LINEAR \
"vec3 toLinear(vec3 color)\n" \
"{\n" \
"    return mix(\n" \
"        color / vec3(12.92),\n" \
"        pow((color + vec3(0.055)) / vec3(1.055), vec3(2.4)),\n" \
"        vec3(greaterThan(color, vec3(0.04045))));\n" \
"}\n"
#define HCC_GRAPHICS_TO_SRGB \
"vec3 tosRGB(vec3 color)\n" \
"{\n" \
"    return mix(\n" \
"        color * vec3(12.92),\n" \
"        vec3(1.055) * pow(color, vec3(1.0 / 2.4)) - vec3(0.055),\n" \
"        vec3(greaterThan(color, vec3(0.0031308))));\n" \
"}\n"

const std::string arc_vertex_shader_source =
"#version 100\n"
"uniform mat4 u_Projection;\n"
"attribute vec4 a_Position;\n"
"attribute vec4 a_Color;\n"
"attribute vec4 a_Circle;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_Position;\n"
"varying vec4 v_Circle;\n"
HCC_GRAPHICS_TO_LINEAR
"void main()\n"
"{\n"
"    v_Color = vec4(toLinear(a_Color.rgb), a_Color.a);\n"
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
HCC_GRAPHICS_TO_SRGB
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
"    float alpha = smoothSample();\n"
"    if (alpha == 0.0)\n"
"        discard;\n"
"    gl_FragColor = vec4(tosRGB(v_Color.rgb * alpha), 1);\n"
"}\n";

const std::string font_vertex_shader_source =
"#version 100\n"
"uniform mat4 u_Projection;\n"
"attribute vec4 a_Position;\n"
"attribute vec3 a_BackgroundColor;\n"
"attribute vec4 a_Color;\n"
"attribute vec2 a_TexCoord;\n"
"varying vec3 v_BackgroundColor;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
HCC_GRAPHICS_TO_LINEAR
"void main()\n"
"{\n"
"    v_BackgroundColor = toLinear(a_BackgroundColor);\n"
"    v_Color = vec4(toLinear(a_Color.rgb), a_Color.a);\n"
"    v_TexCoord = a_TexCoord;\n"
"    gl_Position = u_Projection * a_Position;\n"
"}\n";

const std::string font_fragment_shader_source =
"#version 100\n"
"precision mediump float;\n"
"uniform sampler2D u_Texture;\n"
"varying vec3 v_BackgroundColor;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
HCC_GRAPHICS_TO_SRGB
"void main()\n"
"{\n"
"    float alpha = texture2D(u_Texture, v_TexCoord).a;"
"    gl_FragColor = vec4(tosRGB(mix(v_BackgroundColor, v_Color.rgb, v_Color.a * alpha)), 1);\n"
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

const std::array<std::uint8_t, 4 * 4> texture_example{{
    255, 204, 162, 127,
    204, 162, 127, 84,
    162, 127, 84,  42,
    127, 84,  42,  0,
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
    std::vector<GLfloat> font_vertices{
        100, 100,
        200, 100,
        200, 200,
        100, 100,
        200, 200,
        100, 200};
    std::vector<GLfloat> font_background_colors{
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f,
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f,
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f,
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f,
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f,
        197 / 255.0f, 26 / 255.0f, 74 / 255.0f};
    std::vector<GLfloat> font_colors{
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f,
        0.5f, 0.75f, 0.25f, 0.75f};
    std::vector<GLfloat> font_coords{
        0, 0,
        5, 0,
        5, 5,
        0, 0,
        5, 5,
        0, 5};
    GLuint arc_vertex_buffer{};
    GLuint arc_color_buffer{};
    GLuint arc_circle_buffer{};
    GLuint font_vertex_buffer{};
    GLuint font_background_color_buffer{};
    GLuint font_color_buffer{};
    GLuint font_coord_buffer{};
    std::vector<TexturedDrawCall> font_draw_calls;

    GLuint arc_vertex_shader{};
    GLuint arc_fragment_shader{};
    GLuint arc_program{};
    GLuint font_vertex_shader{};
    GLuint font_fragment_shader{};
    GLuint font_program{};

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
    glGenBuffers(1, &state->font_vertex_buffer);
    glGenBuffers(1, &state->font_background_color_buffer);
    glGenBuffers(1, &state->font_color_buffer);
    glGenBuffers(1, &state->font_coord_buffer);
}

GLuint create_shader(GLenum type, const std::string& source)
{
    auto id = glCreateShader(type);
    auto source_ = source.c_str();
    glShaderSource(id, 1, &source_, nullptr);
    glCompileShader(id);
    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
        GLint log_length{};
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
		std::vector<GLchar> log(log_length);
		glGetShaderInfoLog(id, log_length, nullptr, log.data());
		std::cerr << "Shader " << type << " compilation error: " << log.data() << std::endl;
        std::abort();
	}
    return id;
}

GLuint create_program(GLuint vs, GLuint fs)
{
    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
        GLint log_length{};
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
		std::vector<GLchar> log(log_length);
		glGetProgramInfoLog(program, log_length, nullptr, log.data());
		std::cerr << "Program link error: " << log.data() << std::endl;
        std::abort();
	}
    return program;
}

void init_shaders()
{
    state->arc_vertex_shader = create_shader(GL_VERTEX_SHADER, arc_vertex_shader_source);
    state->arc_fragment_shader = create_shader(GL_FRAGMENT_SHADER, arc_fragment_shader_source);
    state->arc_program = create_program(state->arc_vertex_shader, state->arc_fragment_shader);

    state->font_vertex_shader = create_shader(GL_VERTEX_SHADER, font_vertex_shader_source);
    state->font_fragment_shader = create_shader(GL_FRAGMENT_SHADER, font_fragment_shader_source);
    state->font_program = create_program(state->font_vertex_shader, state->font_fragment_shader);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
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

    auto texture = create_texture(4, 4, texture_example.data());
    ::state->font_draw_calls.emplace_back(0, 6, texture);

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

    set_buffer(state->font_vertex_buffer, state->font_vertices);
    set_buffer(state->font_background_color_buffer, state->font_background_colors);
    set_buffer(state->font_color_buffer, state->font_colors);
    set_buffer(state->font_coord_buffer, state->font_coords);

    glUseProgram(state->font_program);
    glUniformMatrix4fv(glGetUniformLocation(state->font_program, "u_Projection"), 1, false, state->projection.data());
    set_vertex_attrib(state->font_program, "a_Position", 2, state->font_vertex_buffer);
    set_vertex_attrib(state->font_program, "a_BackgroundColor", 3, state->font_background_color_buffer);
    set_vertex_attrib(state->font_program, "a_Color", 4, state->font_color_buffer);
    set_vertex_attrib(state->font_program, "a_TexCoord", 2, state->font_coord_buffer);

    for (auto& dc : state->font_draw_calls)
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
