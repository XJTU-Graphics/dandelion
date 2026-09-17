#include <ctime>

#include <catch2/catch_amalgamated.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%n] [%^%l%$] %v");
    spdlog::set_default_logger(spdlog::stdout_color_mt("Test"));
    std::time_t current_timestamp = std::time(nullptr);
    std::tm*    now               = std::localtime(&current_timestamp);
    spdlog::info(
        "Dandelion 3D Builder, started at {:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
        now->tm_year + 1'900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec
    );

    int result = Catch::Session().run(argc, argv);

    return result;
}
