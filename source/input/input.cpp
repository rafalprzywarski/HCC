#include <cstdint>
#include <deque>
#include <vector>
#include <array>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cassert>

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
    int fd = 0;
    std::int64_t touch_x{}, touch_y{};
    std::vector<input_event> raw_events;
    std::deque<Event> events;
};

State *state = nullptr;

Event create_event()
{
    Event event;
    event.type = TOUCH_MOVE;
    for (const auto& e : state->raw_events)
    {
        if (e.type == EV_KEY && e.code == BTN_TOUCH)
        {
            event.type = e.value ? TOUCH_DOWN : TOUCH_UP;
        }
        if (e.type == EV_ABS)
        {
            if (e.code == ABS_X)
                state->touch_x = e.value;
            if (e.code == ABS_Y)
                state->touch_y = e.value;
        }
    }
    event.x = state->touch_x;
    event.y = state->touch_y;
    return event;
}

void poll()
{
    std::array<input_event, 64> buffer;
    auto n = read(state->fd, buffer.data(), buffer.size() * sizeof(buffer[0]));
    if (n <= 0)
        return;
    assert(n % sizeof(buffer[0]) == 0);
    n /= sizeof(buffer[0]);

    for (auto e = begin(buffer); e != (begin(buffer) + n); ++e)
    {
        if (e->type != EV_SYN)
        {
            state->raw_events.insert(state->raw_events.end(), *e);
            continue;
        }
        state->events.push_back(create_event());
        state->raw_events.clear();
    }
}

}

extern "C"
{

std::int64_t initialize()
{
    state = new State;
    state->fd = open("/dev/input/event0", O_RDONLY);
    auto flags = fcntl(state->fd, F_GETFL, 0);
    fcntl(state->fd, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

std::int64_t shutdown()
{
    if (!state)
        return 0;
    close(state->fd);
    delete state;
    state = nullptr;
    return 0;
}

std::int64_t has_input()
{
    if (!state)
        return false;
    if (!state->events.empty())
        return true;
    poll();
    return !state->events.empty();
}

std::int64_t pop_event()
{
    if (has_input())
        state->events.pop_front();
    return 0;
}

std::int64_t get_event_type()
{
    if (!has_input())
        return 0;
    return state->events.front().type;
}

std::int64_t get_event_x()
{
    if (!has_input())
        return -1;
    return state->events.front().x;
}

std::int64_t get_event_y()
{
    if (!has_input())
        return -1;
    return state->events.front().y;
}

}
