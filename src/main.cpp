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
    auto now = std::chrono::system_clock::now();
    auto local_now = std::chrono::current_zone()->to_local(now);
    spdlog::info("Dandelion 3D Builder, started at {:%Y-%m-%d %H:%M:%S}", local_now);

    Platform platform;
    platform.eventloop();

    return 0;
}
