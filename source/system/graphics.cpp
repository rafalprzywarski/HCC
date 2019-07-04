#include <cstdint>
#ifndef __APPLE__
#include <bcm_host.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
#include <OpenGL/gl.h>
#endif
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
#include <png.h>
#include <cstring>

namespace
{

constexpr int VA_BOTTOM = -1;
constexpr int VA_BASELINE = 0;
constexpr int VA_CENTER = 1;
constexpr int VA_BASELINE_CENTER = 2;
constexpr int VA_TOP = 3;

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

const std::string image_vertex_shader_source =
#ifndef __APPLE__
"#version 100\n"
#endif // __APPLE__
"uniform mat4 u_Projection;\n"
"attribute vec4 a_Position;\n"
"attribute vec2 a_TexCoord;\n"
"varying vec2 v_TexCoord;\n"
"void main()\n"
"{\n"
"    v_TexCoord = a_TexCoord;\n"
"    gl_Position = u_Projection * a_Position;\n"
"}\n";

const std::string image_fragment_shader_source =
#ifndef __APPLE__
"#version 100\n"
"precision mediump float;\n"
#endif // __APPLE__
"uniform sampler2D u_Texture;\n"
"uniform vec3 u_BackgroundColor;\n"
"varying vec2 v_TexCoord;\n"
HCC_GRAPHICS_TO_LINEAR
HCC_GRAPHICS_TO_SRGB
"void main()\n"
"{\n"
"    vec4 color = texture2D(u_Texture, v_TexCoord);\n"
"    if (color.a == 0.0)\n"
"        discard;\n"
"    gl_FragColor = vec4(tosRGB(mix(toLinear(u_BackgroundColor), toLinear(color.rgb), color.a)), 1);\n"
"}\n";

const std::string arc_vertex_shader_source =
#ifndef __APPLE__
"#version 100\n"
#endif // __APPLE__
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
#ifndef __APPLE__
"#version 100\n"
"precision mediump float;\n"
#endif // __APPLE__
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
"    float a = abs(v_Circle.z);\n"
"    float b = abs(v_Circle.w);\n"
"    float sample = smoothStep(a - 0.7071, a + 0.7071, length((v_Position - v_Circle.xy) * vec2(1, a / b)));\n"
"    return 0.5 - sign(v_Circle.z) * (sample - 0.5);\n"
"}\n"
"void main()\n"
"{\n"
"    float alpha = smoothSample();\n"
"    if (alpha == 0.0)\n"
"        discard;\n"
"    gl_FragColor = vec4(v_Color.rgb, v_Color.a * alpha);\n"
"}\n";

const std::string font_vertex_shader_source =
#ifndef __APPLE__
"#version 100\n"
#endif // __APPLE__
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

const std::string font_fragment_shader_source =
#ifndef __APPLE__
"#version 100\n"
"precision mediump float;\n"
#endif // __APPLE__
"uniform sampler2D u_Texture;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
"void main()\n"
"{\n"
"    float alpha = texture2D(u_Texture, v_TexCoord).a;\n"
"    if (alpha == 0.0)\n"
"        discard;\n"
"    gl_FragColor = vec4(v_Color.rgb, v_Color.a * alpha);\n"
"}\n";

const std::string combine_vertex_shader_source =
#ifndef __APPLE__
"#version 100\n"
#endif // __APPLE__
"uniform mat4 u_Projection;\n"
"uniform vec2 u_ScreenSize;\n"
"attribute vec4 a_Position;\n"
"varying vec2 v_TexCoord;\n"
"void main()\n"
"{\n"
"    v_TexCoord = a_Position.xy / u_ScreenSize;\n"
"    gl_Position = u_Projection * a_Position;\n"
"}\n";

const std::string combine_fragment_shader_source =
#ifndef __APPLE__
"#version 100\n"
"precision mediump float;\n"
#endif // __APPLE__
"uniform sampler2D u_ImageTexture;\n"
"uniform sampler2D u_ArcTexture;\n"
"uniform sampler2D u_FontTexture;\n"
"varying vec2 v_TexCoord;\n"
HCC_GRAPHICS_TO_LINEAR
HCC_GRAPHICS_TO_SRGB
"void main()\n"
"{\n"
"    vec3 imageColor = texture2D(u_ImageTexture, v_TexCoord).rgb;\n"
"    vec4 arcColor = texture2D(u_ArcTexture, v_TexCoord);\n"
"    vec4 fontColor = texture2D(u_FontTexture, v_TexCoord);\n"
"    gl_FragColor = vec4(tosRGB(mix(mix(toLinear(imageColor), toLinear(arcColor.rgb), arcColor.a), toLinear(fontColor.rgb), fontColor.a)), 1);\n"
"}\n";

#ifndef __APPLE__
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

#endif

struct FontDrawCall
{
    GLint offset{};
    GLsizei size{};
    GLuint texture{};
    FontDrawCall() = default;
    FontDrawCall(GLint offset, GLsizei size, GLuint texture) : offset(offset), size(size), texture(texture) { }
};

struct FontImage
{
    unsigned width{}, height{};
    std::vector<std::uint8_t> alpha;

    FontImage() = default;
    FontImage(unsigned width, unsigned height)
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
    int capital_ascender{};
    FontImage image;
    std::unordered_map<char, std::unordered_map<char, int>> kerning;
    std::unordered_map<char, FontChar> chars;
};

struct Font
{
    int precision{};
    int ascender{}, descender{}, center{}, baseline_center{};
    int texture_width{}, texture_height{};
    GLuint texture;
    std::unordered_map<char, std::unordered_map<char, int>> kerning;
    std::unordered_map<char, FontChar> chars;
};

struct Image
{
    int texture_width{}, texture_height{};
    GLuint texture{};

    Image() = default;
    Image(int width, int height, GLuint texture) : texture_width(width), texture_height(height), texture(texture) {}
};

struct State
{
#ifndef __APPLE__
    EGLDisplay display{};
    EGLSurface surface{};
    EGL_DISPMANX_WINDOW_T window{};
#endif // __APPLE__
    int display_scale = 2;
    std::uint32_t display_width{}, display_height{};

    FT_Library freetype;

    std::vector<Font> fonts;
    std::vector<Image> images;

    std::vector<GLfloat> image_vertices;
    std::vector<GLfloat> image_coords;
    std::vector<GLfloat> arc_vertices;
    std::vector<GLfloat> arc_colors;
    std::vector<GLfloat> arc_circles;
    std::vector<GLfloat> font_vertices;
    std::vector<GLfloat> font_colors;
    std::vector<GLfloat> font_coords;
    GLuint image_vertex_buffer{};
    GLuint image_coord_buffer{};
    std::vector<FontDrawCall> image_draw_calls;
    GLuint arc_vertex_buffer{};
    GLuint arc_color_buffer{};
    GLuint arc_circle_buffer{};
    GLuint font_vertex_buffer{};
    GLuint font_color_buffer{};
    GLuint font_coord_buffer{};
    std::vector<FontDrawCall> font_draw_calls;
    GLuint combine_vertex_buffer{};

    GLuint image_vertex_shader{};
    GLuint image_fragment_shader{};
    GLuint image_program{};
    GLuint arc_vertex_shader{};
    GLuint arc_fragment_shader{};
    GLuint arc_program{};
    GLuint font_vertex_shader{};
    GLuint font_fragment_shader{};
    GLuint font_program{};
    GLuint combine_vertex_shader{};
    GLuint combine_fragment_shader{};
    GLuint combine_program{};

    std::array<GLfloat, 16> projection{};

    GLuint image_fbo;
    GLuint image_texture;
    GLuint arc_fbo;
    GLuint arc_texture;
    GLuint font_fbo;
    GLuint font_texture;

    std::array<GLfloat, 3> clear_color{};
};

State *state = nullptr;

void init_projection(int width, int height)
{
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
    glGenBuffers(1, &state->image_vertex_buffer);
    glGenBuffers(1, &state->image_coord_buffer);
    glGenBuffers(1, &state->arc_vertex_buffer);
    glGenBuffers(1, &state->arc_color_buffer);
    glGenBuffers(1, &state->arc_circle_buffer);
    glGenBuffers(1, &state->font_vertex_buffer);
    glGenBuffers(1, &state->font_color_buffer);
    glGenBuffers(1, &state->font_coord_buffer);
    glGenBuffers(1, &state->combine_vertex_buffer);
}

std::pair<GLuint, GLuint> create_framebuffer(GLenum format)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, format, state->display_width * state->display_scale, state->display_height * state->display_scale, 0, format, GL_UNSIGNED_BYTE, nullptr);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
    {
        std::cerr << "Framebuffer incomplete" << std::endl;
        std::abort();
    }
    return {fbo, texture};
}

void init_framebuffers()
{
    std::tie(state->image_fbo, state->image_texture) = create_framebuffer(GL_RGB);
    std::tie(state->arc_fbo, state->arc_texture) = create_framebuffer(GL_RGBA);
    std::tie(state->font_fbo, state->font_texture) = create_framebuffer(GL_RGBA);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    state->image_vertex_shader = create_shader(GL_VERTEX_SHADER, image_vertex_shader_source);
    state->image_fragment_shader = create_shader(GL_FRAGMENT_SHADER, image_fragment_shader_source);
    state->image_program = create_program(state->image_vertex_shader, state->image_fragment_shader);

    state->arc_vertex_shader = create_shader(GL_VERTEX_SHADER, arc_vertex_shader_source);
    state->arc_fragment_shader = create_shader(GL_FRAGMENT_SHADER, arc_fragment_shader_source);
    state->arc_program = create_program(state->arc_vertex_shader, state->arc_fragment_shader);

    state->font_vertex_shader = create_shader(GL_VERTEX_SHADER, font_vertex_shader_source);
    state->font_fragment_shader = create_shader(GL_FRAGMENT_SHADER, font_fragment_shader_source);
    state->font_program = create_program(state->font_vertex_shader, state->font_fragment_shader);

    state->combine_vertex_shader = create_shader(GL_VERTEX_SHADER, combine_vertex_shader_source);
    state->combine_fragment_shader = create_shader(GL_FRAGMENT_SHADER, combine_fragment_shader_source);
    state->combine_program = create_program(state->combine_vertex_shader, state->combine_fragment_shader);
}

void set_vertex_attrib(GLuint program, const char *name, int size, GLuint buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    auto location = glGetAttribLocation(program, name);
    glVertexAttribPointer(location, size, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(location);
}

template <typename Container>
void set_buffer(GLuint buffer, const Container& data)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data[0]) * data.size(), data.data(), GL_DYNAMIC_DRAW);
}

GLuint create_texture(GLsizei width, GLsizei height, const void *data)
{
    GLuint texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
    return texture;
}


FontImage downscale(FT_Bitmap bitmap, unsigned n, unsigned offset, unsigned y_offset)
{
    FontImage g{(bitmap.width + offset + (n - 1)) / n, (bitmap.rows + y_offset + (n - 1)) / n};
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

void blit(FontImage& dst, unsigned dx, unsigned dy, const FontImage& src)
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
    FontImage img{2048 * unsigned(state->display_scale), 2048};
    unsigned dx = 0, dy = 0, row_height = 0;
    bool too_big = false;
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
        auto bearing_y = face->glyph->metrics.horiBearingY / 64;
        font.chars[c].bearing_y = (bearing_y + precision - 1) / precision * precision;
        font.chars[c].advance_x = face->glyph->metrics.horiAdvance / 64;

        if (c == 'T')
            font.capital_ascender = bearing_y;

        for (unsigned offset = 0; offset < precision; ++offset)
        {
            auto g = downscale(bg->bitmap, precision, offset, (font.precision - (bearing_y % font.precision)) % font.precision);
            if ((dx + g.width) >= img.width)
            {
                dx = 0;
                dy += row_height;
                row_height = 0;
            }
            row_height = std::max(row_height, g.height);
            blit(img, dx, dy, g);

            if (dy + g.height > img.height)
                too_big = true;

            FontGlyph fg;
            fg.img_x = dx;
            fg.img_y = dy;
            fg.img_width = g.width;
            fg.img_height = g.height;
            font.chars[c].glyphs.push_back(fg);

            dx += g.width;
        }
    }

    if (too_big)
        std::cerr << "error: font too big" << std::endl;

    while (dy + row_height <= img.height / 2)
        img.height /= 2;

    font.image = img;
    return font;
}

Font generate_font(FT_Face face, const std::string& chars, unsigned precision)
{
    auto rf = rasterize_font(face, chars, precision);
    Font f;
    f.precision = rf.precision;
    auto ascender = rf.capital_ascender ?
        rf.capital_ascender * 64 :
        FT_MulFix(face->ascender, face->size->metrics.y_scale);

    f.ascender = (ascender + (32 * rf.precision - 1)) / (64 * rf.precision);
    f.descender = -(FT_MulFix(-face->descender, face->size->metrics.y_scale) + (32 * rf.precision - 1)) / (64 * rf.precision);
    f.center = (ascender - FT_MulFix(-face->descender, face->size->metrics.y_scale) + (64 * rf.precision - 1)) / (2 * 64 * rf.precision);
    f.baseline_center = (ascender + (64 * rf.precision - 1)) / (2 * 64 * rf.precision);
    f.texture_width = rf.image.width;
    f.texture_height = rf.image.height;
    f.texture = create_texture(rf.image.width, rf.image.height, rf.image.alpha.data());
    f.kerning = rf.kerning;
    f.chars = rf.chars;
    return f;
}

std::int64_t text_width(const Font& font, const char *text)
{
    int width = 0;
    char prev_ch = 0;
    for (auto p = text; *p; ++p)
    {
        auto ch = *p;
        if (prev_ch)
            width += font.kerning.at(prev_ch).at(ch);
        width += font.chars.at(ch).advance_x;
        prev_ch = ch;
    }
    return width;
}

}

extern "C"
{

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

std::int64_t initialize_graphics(std::int64_t display_width, std::int64_t display_height, std::int64_t scale)
{
    std::unique_ptr<State> state{new State};
    state->display_scale = scale;

#ifndef __APPLE__
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
#else
    state->display_width = display_width;
    state->display_height = display_height;
#endif // __APPLE__
    ::state = state.release();

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    init_shaders();
    init_buffers();
    init_framebuffers();
    init_projection(::state->display_width * ::state->display_scale, ::state->display_height * ::state->display_scale);

    FT_Init_FreeType(&::state->freetype);

    return 0;
}

std::int64_t shutdown_graphics()
{
    if (!state)
        return 0;
#ifndef __APPLE__
    eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(state->display);
    eglReleaseThread();
#endif
    delete state;
    state = nullptr;
    return 0;
}


std::int64_t background_color(std::int64_t r, std::int64_t g, std::int64_t b)
{
    if (!state)
        return 0;
    state->clear_color = {r / 255.0f, g / 255.0f, b / 255.0f};
    return 0;
}

#ifndef __APPLE__
std::int64_t clear()
{
    if (!state)
        return 0;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}
#endif // __APPLE__

std::int64_t arc(
    std::int64_t x0, std::int64_t y0,
    std::int64_t x1, std::int64_t y1,
    std::int64_t r, std::int64_t g, std::int64_t b, std::int64_t a,
    std::int64_t cx, std::int64_t cy, std::int64_t ca, std::int64_t cb)
{
    if (!state)
        return 0;
    auto scale = state->display_scale;
    x0 *= scale; y0 *= scale;
    x1 *= scale; y1 *= scale;
    cx *= scale; cy *= scale; ca *= scale; cb *= scale;

    std::array<GLfloat, 12> vertices{{
        GLfloat(x0), GLfloat(y0), GLfloat(x1), GLfloat(y0), GLfloat(x1), GLfloat(y1),
        GLfloat(x0), GLfloat(y0), GLfloat(x1), GLfloat(y1), GLfloat(x0), GLfloat(y1)}};
    std::array<GLfloat, 4> color{{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}};
    std::array<GLfloat, 4> circle{{GLfloat(cx), GLfloat(cy), GLfloat(ca), GLfloat(cb)}};
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
    std::int64_t r, std::int64_t g, std::int64_t b, std::int64_t a)
{
    auto radius = std::max(std::abs(x1 - x0), std::abs(y1 - y0)) * 2;
    return arc(x0, y0, x1, y1, r, g, b, a, x0, y0, radius, radius);
}

std::int64_t load_font(const char *filename, std::int64_t size)
{
    const unsigned PRECISION = 16;
    FT_Face fontface;
    FT_New_Face(::state->freetype, filename, 0, &fontface);
    FT_Set_Char_Size(fontface, 0, size * PRECISION * state->display_scale * 64, 131, 142);

    std::string charset;
    for (char c = 32; c < 127; ++c)
        charset += c;
    ::state->fonts.push_back(generate_font(fontface, charset, PRECISION));

    FT_Done_Face(fontface);

    return ::state->fonts.size() - 1;
}

std::int64_t text(
    std::int64_t font_id, const char *text,
    std::int64_t x, std::int64_t y,
    std::int64_t anchor, std::int64_t vanchor,
    std::int64_t c_r, std::int64_t c_g, std::int64_t c_b, std::int64_t c_a)
{
    x *= state->display_scale; y *= state->display_scale;
    auto& font = ::state->fonts.at(font_id);
    auto array_offset = ::state->font_vertices.size() / 2;
    auto pen_x = x * font.precision;
    if (anchor > 0)
        pen_x -= text_width(font, text);
    else if (anchor == 0)
        pen_x -= text_width(font, text) / 2;

    switch (vanchor)
    {
    case VA_BOTTOM: y -= font.descender; break;
    case VA_CENTER: y -= font.center; break;
    case VA_BASELINE_CENTER: y -= font.baseline_center; break;
    case VA_TOP: y -= font.ascender; break;
    case VA_BASELINE:
    default:;
    }

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

        ::state->font_vertices.insert(::state->font_vertices.end(), vs.begin(), vs.end());
        ::state->font_coords.insert(::state->font_coords.end(), tc.begin(), tc.end());
        for (int i = 0; i < 6; ++i)
            ::state->font_colors.insert(end(::state->font_colors), begin(color), end(color));

        pen_x += char_.advance_x;
        prev_ch = ch;
    }
    ::state->font_draw_calls.emplace_back(array_offset, ::state->font_vertices.size() / 2 - array_offset, font.texture);
    return 0;
}

std::int64_t load_image(const char *path)
{
    auto fp = std::fopen(path, "rb");
    if (!fp)
    {
        std::cerr << "error loading " << path << std::endl;
        std::abort();
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png)))
    {
        std::cerr << "error loading " << path << std::endl;
        std::abort();
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    auto width = png_get_image_width(png, info);
    auto height = png_get_image_height(png, info);
    auto color_type = png_get_color_type(png, info);
    auto bit_depth = png_get_bit_depth(png, info);

    if (bit_depth != 8 || (color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA))
    {
        std::cerr << "unsupported image format: " << path << std::endl;
        std::abort();
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);

    png_read_update_info(png, info);

    auto rows = static_cast<png_bytep *>(std::malloc(sizeof(png_bytep) * height));
    for (decltype(height) i = 0; i < height; ++i)
        rows[i] = static_cast<png_byte *>(std::malloc(png_get_rowbytes(png, info)));

    png_read_image(png, rows);

    std::fclose(fp);

    std::vector<std::uint8_t> data(width * height * 4);
    for (decltype(height) i = 0; i < height; ++i)
    {
        std::memcpy(&data[(height - i - 1) * width * 4], rows[i], width * 4);
        std::free(rows[i]);
    }
    std::free(rows);

    GLuint texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    state->images.emplace_back(width, height, texture);

    std::cout << "loaded " << path << std::endl;

    return state->images.size() - 1;
}

std::int64_t image(
    std::int64_t image_id,
    std::int64_t x, std::int64_t y,
    std::int64_t anchor, std::int64_t vanchor)
{
    const auto& img = ::state->images[image_id];
    if (anchor > 0)
        x -= img.texture_width;
    else if (anchor == 0)
        x -= img.texture_width / 2;
    if (vanchor > 0)
        y -= img.texture_height;
    else if (vanchor == 0)
        y -= img.texture_height / 2;
    x *= state->display_scale;
    y *= state->display_scale;
    auto width = img.texture_width * state->display_scale;
    auto height = img.texture_height * state->display_scale;
    std::array<GLfloat, 12> vs{{
        GLfloat(x), GLfloat(y),
        GLfloat(x) + width, GLfloat(y),
        GLfloat(x) + width, GLfloat(y) + height,
        GLfloat(x), GLfloat(y),
        GLfloat(x) + width, GLfloat(y) + height,
        GLfloat(x), GLfloat(y) + height}};
    std::array<GLfloat, 12> cs{{0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}};

    ::state->image_vertices.insert(::state->image_vertices.end(), vs.begin(), vs.end());
    ::state->image_coords.insert(::state->image_coords.end(), cs.begin(), cs.end());
    ::state->image_draw_calls.emplace_back((::state->image_vertices.size() - vs.size()) / 2, vs.size() / 2, img.texture);

    return 0;
}

std::int64_t render()
{
    if (!state)
        return 0;

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, state->image_fbo);
    glClearColor(state->clear_color[0], state->clear_color[1], state->clear_color[2], 1);
    glClear(GL_COLOR_BUFFER_BIT);

    set_buffer(state->image_vertex_buffer, state->image_vertices);
    set_buffer(state->image_coord_buffer, state->image_coords);
    glUseProgram(state->image_program);
    glUniformMatrix4fv(glGetUniformLocation(state->image_program, "u_Projection"), 1, false, state->projection.data());
    glUniform3fv(glGetUniformLocation(state->image_program, "u_BackgroundColor"), 1, state->clear_color.data());
    set_vertex_attrib(state->image_program, "a_Position", 2, state->image_vertex_buffer);
    set_vertex_attrib(state->image_program, "a_TexCoord", 2, state->image_coord_buffer);
    glUniform1i(glGetUniformLocation(state->image_program, "u_Texture"), 0);

    glActiveTexture(GL_TEXTURE0);
    for (auto& dc : state->image_draw_calls)
    {
        glBindTexture(GL_TEXTURE_2D, dc.texture);
        glDrawArrays(GL_TRIANGLES, dc.offset, dc.size);
    }
    state->image_vertices.clear();
    state->image_coords.clear();
    state->image_draw_calls.clear();

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);

    glBindFramebuffer(GL_FRAMEBUFFER, state->arc_fbo);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

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

    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ONE);

    glBindFramebuffer(GL_FRAMEBUFFER, state->font_fbo);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    set_buffer(state->font_vertex_buffer, state->font_vertices);
    set_buffer(state->font_color_buffer, state->font_colors);
    set_buffer(state->font_coord_buffer, state->font_coords);

    glUseProgram(state->font_program);
    glUniformMatrix4fv(glGetUniformLocation(state->font_program, "u_Projection"), 1, false, state->projection.data());
    set_vertex_attrib(state->font_program, "a_Position", 2, state->font_vertex_buffer);
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
    state->font_coords.clear();
    state->font_draw_calls.clear();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);

    std::array<GLfloat, 2> screen_size{{GLfloat(state->display_width * state->display_scale), GLfloat(state->display_height * state->display_scale)}};
    std::array<GLfloat, 12> screen_vertices{{0, 0, screen_size[0], 0, screen_size[0], screen_size[1], 0, 0, screen_size[0], screen_size[1], 0, screen_size[1]}};
    set_buffer(state->combine_vertex_buffer, screen_vertices);
    glUseProgram(state->combine_program);
    glUniformMatrix4fv(glGetUniformLocation(state->combine_program, "u_Projection"), 1, false, state->projection.data());
    glUniform2fv(glGetUniformLocation(state->combine_program, "u_ScreenSize"), 1, screen_size.data());
    set_vertex_attrib(state->combine_program, "a_Position", 2, state->combine_vertex_buffer);
    glUniform1i(glGetUniformLocation(state->combine_program, "u_ImageTexture"), 0);
    glUniform1i(glGetUniformLocation(state->combine_program, "u_ArcTexture"), 1);
    glUniform1i(glGetUniformLocation(state->combine_program, "u_FontTexture"), 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state->image_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, state->arc_texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, state->font_texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    return 0;
}

#ifndef __APPLE__
std::int64_t swap_buffers()
{
    if (!state)
        return 0;
    eglSwapBuffers(state->display, state->surface);
    return 0;
}
#endif

}
