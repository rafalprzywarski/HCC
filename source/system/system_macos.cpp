#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <memory>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

namespace
{

struct Event
{
    std::int64_t type{}, x{}, y{};
};

constexpr std::int64_t TOUCH_DOWN = 1;
constexpr std::int64_t TOUCH_UP = 2;
constexpr std::int64_t TOUCH_MOVE = 3;

struct State
{
    std::unique_ptr<sf::RenderWindow> window;
    int display_scale = 1;
    bool has_input = false;
    bool touch_down = false;
    Event event;
};

State *state = nullptr;

}

extern "C"
{

std::int64_t initialize_graphics(std::int64_t display_width, std::int64_t display_height, std::int64_t scale);
std::int64_t shutdown_graphics();

std::int64_t initialize(std::int64_t display_width, std::int64_t display_height, std::int64_t scale)
{
    std::unique_ptr<State> state{new State};
    state->display_scale = (scale < 1) ? 1 : ((scale > 3) ? 3 : scale);

    sf::ContextSettings settings;
    state->window.reset(new sf::RenderWindow(sf::VideoMode(display_width * state->display_scale, display_height * state->display_scale), "hcc", sf::Style::Titlebar | sf::Style::Close, settings));
    state->window->setVerticalSyncEnabled(true);

    ::state = state.release();

    ::state->window->setActive();
    initialize_graphics(display_width, display_height, ::state->display_scale);
    return 0;
}

std::int64_t shutdown()
{
    if (!state)
        return 0;
    shutdown_graphics();
    delete state;
    state = nullptr;
    return 0;
}

std::int64_t clear()
{
    if (!state)
        return 0;
    state->window->setActive();
    state->window->clear();
    return 0;
}

std::int64_t swap_buffers()
{
    state->window->display();
    return 0;
}

std::int64_t has_input()
{
    if (!state)
        return false;
    if (state->has_input)
        return true;
    sf::Event event;
    if (!state->window->pollEvent(event))
        return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == 0)
    {
        state->event.type = TOUCH_DOWN;
        state->event.x = event.mouseButton.x / state->display_scale;
        state->event.y = event.mouseButton.y / state->display_scale;
        state->has_input = true;
        state->touch_down = true;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == 0)
    {
        state->event.type = TOUCH_UP;
        state->event.x = event.mouseButton.x / state->display_scale;
        state->event.y = event.mouseButton.y / state->display_scale;
        state->has_input = true;
        state->touch_down = false;
    }
    else if (event.type == sf::Event::MouseMoved && state->touch_down)
    {
        state->event.type = TOUCH_MOVE;
        state->event.x = event.mouseMove.x / state->display_scale;
        state->event.y = event.mouseMove.y / state->display_scale;
        state->has_input = true;
    }

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
    if (!has_input())
        return 0;
    return state->event.type;
}

std::int64_t get_event_x()
{
    if (!has_input())
        return -1;
    return state->event.x;
}

std::int64_t get_event_y()
{
    if (!has_input())
        return -1;
    return state->event.y;
}

std::int64_t file_timestamp(const char *path)
{
    struct stat s;
    if (stat(path, &s))
        return 0;
    return s.st_mtime;
}

}
