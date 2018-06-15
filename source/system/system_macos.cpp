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

std::int64_t initialize_graphics();
std::int64_t shutdown_graphics();

std::int64_t initialize()
{
    std::unique_ptr<State> state{new State};

    sf::ContextSettings settings;
    state->window.reset(new sf::RenderWindow(sf::VideoMode(DISPLAY_WIDTH, DISPLAY_HEIGHT), "hcc", sf::Style::Titlebar | sf::Style::Close, settings));
    state->window->setVerticalSyncEnabled(true);

    ::state = state.release();

    ::state->window->setActive();
    initialize_graphics();
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
