#include <cstdint>

extern "C"
{

std::int64_t initialize_graphics();
std::int64_t shutdown_graphics();
std::int64_t initialize_input();
std::int64_t shutdown_input();

std::int64_t initialize()
{
    initialize_graphics();
    initialize_input();
    return 0;
}

std::int64_t shutdown()
{
    shutdown_input();
    shutdown_graphics();
    return 0;
}

}
