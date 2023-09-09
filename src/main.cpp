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
    // Here we do not use std::chrono::system_clock, because the formatter
    // of std::chrono::time_point in fmtlib 9.1.0 formats it to local time,
    // but the formatter in C++20's std::format formats it to UTC time.
    // In the future we may migrate the whole project to C++20/23, so we do
    // not want to use an inconsistent feature.
    std::tm now = fmt::localtime(std::time(nullptr));
    spdlog::info("Dandelion 3D, started at {:%Y-%m-%d %H:%M:%S%z}", now);

    Platform platform;
    platform.eventloop();

    return 0;
}
