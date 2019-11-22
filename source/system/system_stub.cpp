#include <cstdint>

extern "C"
{

std::int64_t initialize(std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

std::int64_t shutdown()
{
    return 0;
}

std::int64_t background_color(std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

 std::int64_t clear()
{
    return 0;
}

std::int64_t render()
{
    return 0;
}

std::int64_t arc(std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

std::int64_t rect(std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

std::int64_t load_font(const char *, std::int64_t)
{
    return 0;
}

std::int64_t load_image(const char *)
{
    return 0;
}

std::int64_t text(std::int64_t, const char *, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

std::int64_t image(std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)
{
    return 0;
}

std::int64_t swap_buffers()
{
    return 0;
}

std::int64_t get_display_width()
{
    return 800;
}

std::int64_t get_display_height()
{
    return 480;
}

std::int64_t has_input()
{
    return 0;
}

std::int64_t pop_event()
{
    return 0;
}

std::int64_t get_event_type()
{
    return 0;
}

std::int64_t get_event_x()
{
    return 0;
}

std::int64_t get_event_y()
{
    return 0;
}

std::int64_t file_timestamp(const char*)
{
    return 0;
}

}
