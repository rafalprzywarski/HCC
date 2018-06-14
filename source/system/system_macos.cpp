#include <cstdint>
#include <memory>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

namespace
{

constexpr int DISPLAY_WIDTH = 800;
constexpr int DISPLAY_HEIGHT = 480;

struct State
{
    std::unique_ptr<sf::RenderWindow> window;
    bool has_input = false;
    sf::Event event;
};

State *state = nullptr;

}

extern "C"
{

std::int64_t initialize()
{
    std::unique_ptr<State> state{new State};

    sf::ContextSettings settings;
    state->window.reset(new sf::RenderWindow(sf::VideoMode(DISPLAY_WIDTH, DISPLAY_HEIGHT), "hcc", sf::Style::Titlebar | sf::Style::Close, settings));
    state->window->setVerticalSyncEnabled(true);

    ::state = state.release();
    return 0;
}

std::int64_t shutdown()
{
    if (!state)
        return 0;
    delete state;
    state = nullptr;
    return 0;
}

std::int64_t clear()
{
    if (!state)
        return 0;
    state->window->clear();
    return 0;
}

std::int64_t arc(
    std::int64_t x0, std::int64_t y0,
    std::int64_t x1, std::int64_t y1,
    std::int64_t r, std::int64_t g, std::int64_t b,
    std::int64_t cx, std::int64_t cy, std::int64_t cr, std::int64_t corient)
{
    return 0;
}

std::int64_t rect(
    std::int64_t x0, std::int64_t y0,
    std::int64_t x1, std::int64_t y1,
    std::int64_t r, std::int64_t g, std::int64_t b)
{
    return 0;
}

std::int64_t load_font(const char *filename, std::int64_t size)
{
    return 0;
}

std::int64_t text(
    std::int64_t font_id, const char *text,
    std::int64_t x, std::int64_t y,
    std::int64_t anchor, std::int64_t vanchor,
    std::int64_t c_r, std::int64_t c_g, std::int64_t c_b, std::int64_t c_a,
    std::int64_t bg_r, std::int64_t bg_g, std::int64_t bg_b)
{
    return 0;
}

std::int64_t render()
{
    return 0;
}

std::int64_t swap_buffers()
{
    state->window->display();
    return 0;
}

std::int64_t get_display_width()
{
    return DISPLAY_WIDTH;
}

std::int64_t get_display_height()
{
    return DISPLAY_HEIGHT;
}

std::int64_t has_input()
{
    if (!state)
        return false;
    if (state->has_input)
        return true;
    state->has_input = state->window->pollEvent(state->event);
    return state->has_input;
}

std::int64_t pop_event()
{
    if (has_input())
        state->has_input = false;
    return 0;
}

std::int64_t get_event_type()
{
    return 0;
}

std::int64_t get_event_x()
{
    return -1;
}

std::int64_t get_event_y()
{
    return -1;
}

}
