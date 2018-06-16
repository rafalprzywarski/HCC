#include <cstdint>
#include <memory>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

namespace
{

constexpr int DISPLAY_WIDTH = 800;
constexpr int DISPLAY_HEIGHT = 480;

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

std::int64_t initialize_graphics(std::int64_t scale);
std::int64_t shutdown_graphics();

std::int64_t initialize(std::int64_t scale)
{
    std::unique_ptr<State> state{new State};
    state->display_scale = (scale < 1) ? 1 : ((scale > 3) ? 3 : scale);

    sf::ContextSettings settings;
    state->window.reset(new sf::RenderWindow(sf::VideoMode(DISPLAY_WIDTH * state->display_scale, DISPLAY_HEIGHT * state->display_scale), "hcc", sf::Style::Titlebar | sf::Style::Close, settings));
    state->window->setVerticalSyncEnabled(true);

    ::state = state.release();

    ::state->window->setActive();
    initialize_graphics(::state->display_scale);
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
    state->window->setActive();
    state->window->clear();
    if (!state)
        return 0;
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

}
