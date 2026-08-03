#include <chrono>
#include <format>

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
    spdlog::info(
        "Dandelion 3D Builder, started at {:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
        now->tm_year + 1'900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec
    );

    Platform platform;
    platform.eventloop();

    return 0;
}
