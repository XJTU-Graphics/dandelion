#include <ctime>

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include "platform/platform.h"
#include "utils/logger.h"

int main()
{
    spdlog::set_pattern("[%n] [%^%l%$] %v");
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif
    spdlog::set_default_logger(get_logger("Default"));

    // Log the start time point.
    std::time_t current_timestamp = std::time(nullptr);
    std::tm*    now               = std::localtime(&current_timestamp);
    spdlog::info("Dandelion 3D, started at {:%Y-%m-%d %H:%M:%S}", *now);

    Platform platform;
    platform.eventloop();

    return 0;
}
