#include <cstdint>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <unordered_map>

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
"    float alpha = texture2D(u_Texture, v_TexCoord).a;\n"
"    if (alpha == 0.0)\n"
"        discard;\n"
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

struct FontDrawCall
{
    GLint offset{};
    GLsizei size{};
    GLuint texture{};
    FontDrawCall() = default;
    FontDrawCall(GLint offset, GLsizei size, GLuint texture) : offset(offset), size(size), texture(texture) { }
};

struct Image
{
    unsigned width{}, height{};
    std::vector<std::uint8_t> alpha;

    Image() = default;
    Image(unsigned width, unsigned height)
        : width(width), height(height), alpha(width * height, 0) { }
};

struct FontGlyph
{
    int img_x{}, img_y{}, img_width{}, img_height{};
};
struct FontChar
{
    int bearing_x{}, bearing_y{}, advance_x{};
    std::vector<FontGlyph> glyphs;
};


struct RasterizedFont
{
    int precision{};
    Image image;
    std::unordered_map<char, std::unordered_map<char, int>> kerning;
    std::unordered_map<char, FontChar> chars;
};

struct Font
{
    int precision{};
    int texture_width{}, texture_height{};
    GLuint texture;
    std::unordered_map<char, std::unordered_map<char, int>> kerning;
    std::unordered_map<char, FontChar> chars;
};

struct State
{
    EGLDisplay display{};
    std::uint32_t display_width{}, display_height{};
    EGLSurface surface{};
    EGL_DISPMANX_WINDOW_T window{};
    FT_Library freetype;

    std::vector<Font> fonts;

    std::vector<GLfloat> arc_vertices;
    std::vector<GLfloat> arc_colors;
    std::vector<GLfloat> arc_circles;
    std::vector<GLfloat> font_vertices;
    std::vector<GLfloat> font_background_colors;
    std::vector<GLfloat> font_colors;
    std::vector<GLfloat> font_coords;
    GLuint arc_vertex_buffer{};
    GLuint arc_color_buffer{};
    GLuint arc_circle_buffer{};
    GLuint font_vertex_buffer{};
    GLuint font_background_color_buffer{};
    GLuint font_color_buffer{};
    GLuint font_coord_buffer{};
    std::vector<FontDrawCall> font_draw_calls;

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


Image downscale(FT_Bitmap bitmap, unsigned n, unsigned offset, unsigned y_offset)
{
    Image g{(bitmap.width + offset + (n - 1)) / n, (bitmap.rows + y_offset + (n - 1)) / n};
    auto pitch = bitmap.pitch;
    for (unsigned gy = 0; gy < g.height; ++gy)
        for (unsigned gx = 0; gx < g.width; ++gx)
        {
            unsigned s = 0;
            for (unsigned sy = (std::max(gy * n, y_offset) - y_offset) * pitch, msy = std::min((gy + 1) * n - y_offset, bitmap.rows) * pitch;
                sy < msy;
                sy += pitch)
                s += std::accumulate(
                    bitmap.buffer + std::max(gx * n, offset) - offset + sy,
                    bitmap.buffer + std::min((gx + 1) * n - offset, bitmap.width) + sy, 0u);
            g.alpha[gx + gy * g.width] = (s + (n * n) / 2) / (n * n);
        }
    return g;
}

void blit(Image& dst, unsigned dx, unsigned dy, const Image& src)
{
    if (dx >= dst.width)
        return;
    for (auto y = dy; y < std::min(dy + src.height, dst.height); ++y)
        std::copy_n(src.alpha.data() + (y - dy) * src.width, std::min(src.width, dst.width - dx), dst.alpha.begin() + dx + y * dst.width);
}

RasterizedFont rasterize_font(FT_Face face, const std::string& chars, unsigned precision)
{
    RasterizedFont font;
    font.precision = precision;
    Image img{1024, 1024};
    unsigned dx = 0, dy = 0, row_height = 0;
    for (auto a : chars)
        for (auto b : chars)
        {
            FT_Vector k{};
            FT_Get_Kerning(face, FT_Get_Char_Index(face, a), FT_Get_Char_Index(face, b), FT_KERNING_UNFITTED, &k);
            font.kerning[a][b] = (k.x + 31) / 64;
        }
    for (auto c : chars)
    {
        FT_Load_Char(face, c, FT_LOAD_RENDER);
        FT_Glyph glyph;
        FT_Get_Glyph(face->glyph, &glyph);
        auto bg = (FT_BitmapGlyph)glyph;

        font.chars[c].glyphs.reserve(precision);
        font.chars[c].bearing_x = face->glyph->metrics.horiBearingX / 64;
        font.chars[c].bearing_y = face->glyph->metrics.horiBearingY / 64;
        font.chars[c].advance_x = face->glyph->metrics.horiAdvance / 64;
        for (unsigned offset = 0; offset < precision; ++offset)
        {
            auto g = downscale(bg->bitmap, precision, offset, (font.precision - ((face->glyph->metrics.horiBearingY / 64) % font.precision)) % font.precision);
            if ((dx + g.width) >= img.width)
            {
                dx = 0;
                dy += row_height;
                row_height = 0;
            }
            row_height = std::max(row_height, g.height);
            blit(img, dx, dy, g);

            FontGlyph fg;
            fg.img_x = dx;
            fg.img_y = dy;
            fg.img_width = g.width;
            fg.img_height = g.height;
            font.chars[c].glyphs.push_back(fg);

            dx += g.width;
        }
    }
    font.image = img;
    return font;
}

Font generate_font(FT_Face face, const std::string& chars, unsigned precision)
{
    auto rf = rasterize_font(face, chars, precision);
    Font f;
    f.precision = rf.precision;
    f.texture_width = rf.image.width;
    f.texture_height = rf.image.height;
    f.texture = create_texture(rf.image.width, rf.image.height, rf.image.alpha.data());
    f.kerning = rf.kerning;
    f.chars = rf.chars;
    return f;
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

    FT_Init_FreeType(&::state->freetype);

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

std::int64_t rect(
    std::int64_t x0, std::int64_t y0,
    std::int64_t x1, std::int64_t y1,
    std::int64_t r, std::int64_t g, std::int64_t b)
{
    return arc(x0, y0, x1, y1, r, g, b, 0, 0, -16, -1);
}

std::int64_t load_font(const char *filename, std::int64_t size)
{
    const unsigned PRECISION = 16;
    FT_Face fontface;
    FT_New_Face(::state->freetype, filename, 0, &fontface);
    FT_Set_Char_Size(fontface, 0, size * PRECISION * 64, 131, 142);

    std::string charset;
    for (char c = 32; c < 127; ++c)
        charset += c;
    ::state->fonts.push_back(generate_font(fontface, charset, 16));

    FT_Done_Face(fontface);

    return ::state->fonts.size() - 1;
}

std::int64_t text(
    std::int64_t font_id, const char *text,
    std::int64_t x, std::int64_t y,
    std::int64_t c_r, std::int64_t c_g, std::int64_t c_b, std::int64_t c_a,
    std::int64_t bg_r, std::int64_t bg_g, std::int64_t bg_b)
{
    auto& font = ::state->fonts.at(font_id);
    auto array_offset = ::state->font_vertices.size() / 2;
    auto pen_x = x * font.precision;
    char prev_ch = 0;
    for (char ch : std::string(text))
    {
        if (prev_ch)
            pen_x += font.kerning.at(prev_ch).at(ch);

        auto& char_ = font.chars.at(ch);
        auto bearing_x = char_.bearing_x;
        auto glyph_x = pen_x + bearing_x;
        auto glyph_x_tr = glyph_x / font.precision;
        auto glyph = font.chars.at(ch).glyphs[glyph_x % font.precision];
        auto bearing_y = char_.bearing_y / font.precision;
        std::array<GLfloat, 12> vs{{
            GLfloat(glyph_x_tr), GLfloat(y + bearing_y - glyph.img_height),
            GLfloat(glyph_x_tr + glyph.img_width), GLfloat(y + bearing_y - glyph.img_height),
            GLfloat(glyph_x_tr + glyph.img_width), GLfloat(y + bearing_y),
            GLfloat(glyph_x_tr), GLfloat(y + bearing_y - glyph.img_height),
            GLfloat(glyph_x_tr + glyph.img_width), GLfloat(y + bearing_y),
            GLfloat(glyph_x_tr), GLfloat(y + bearing_y),
        }};
        std::array<GLfloat, 12> tc{{
            GLfloat(glyph.img_x) / font.texture_width, GLfloat(glyph.img_y + glyph.img_height) / font.texture_height,
            GLfloat(glyph.img_x + glyph.img_width) / font.texture_width, GLfloat(glyph.img_y + glyph.img_height) / font.texture_height,
            GLfloat(glyph.img_x + glyph.img_width) / font.texture_width, GLfloat(glyph.img_y) / font.texture_height,
            GLfloat(glyph.img_x) / font.texture_width, GLfloat(glyph.img_y + glyph.img_height) / font.texture_height,
            GLfloat(glyph.img_x + glyph.img_width) / font.texture_width, GLfloat(glyph.img_y) / font.texture_height,
            GLfloat(glyph.img_x) / font.texture_width, GLfloat(glyph.img_y) / font.texture_height,
        }};
        std::array<GLfloat, 4> color{{c_r / 255.0f, c_g / 255.0f, c_b / 255.0f, c_a / 255.0f}};
        std::array<GLfloat, 3> bg_color{{bg_r / 255.0f, bg_g / 255.0f, bg_b / 255.0f}};

        ::state->font_vertices.insert(::state->font_vertices.end(), vs.begin(), vs.end());
        ::state->font_coords.insert(::state->font_coords.end(), tc.begin(), tc.end());
        for (int i = 0; i < 6; ++i)
        {
            ::state->font_colors.insert(end(::state->font_colors), begin(color), end(color));
            ::state->font_background_colors.insert(end(::state->font_background_colors), begin(bg_color), end(bg_color));
        }

        pen_x += char_.advance_x;
        prev_ch = ch;
    }
    ::state->font_draw_calls.emplace_back(array_offset, ::state->font_vertices.size() / 2 - array_offset, font.texture);
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
    glUniform1i(glGetUniformLocation(state->font_program, "u_Texture"), 0);

    glActiveTexture(GL_TEXTURE0);
    for (auto& dc : state->font_draw_calls)
    {
        glBindTexture(GL_TEXTURE_2D, dc.texture);
        glDrawArrays(GL_TRIANGLES, dc.offset, dc.size);
    }
    state->font_vertices.clear();
    state->font_colors.clear();
    state->font_background_colors.clear();
    state->font_coords.clear();
    state->font_draw_calls.clear();

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
