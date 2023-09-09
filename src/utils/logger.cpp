#include "logger.h"

#include <array>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using std::array;
using std::make_shared;
using std::shared_ptr;
using std::string;

shared_ptr<spdlog::logger> get_logger(const string& name)
{
    // All loggers must share the same file sink.
    static auto file_sink = make_shared<spdlog::sinks::basic_file_sink_st>("dandelion.log", true);
    shared_ptr<spdlog::logger> logger = spdlog::get(name);
    if (logger != nullptr) {
        return logger;
    }
    auto console_sink = make_shared<spdlog::sinks::stdout_color_sink_st>();
    array<spdlog::sink_ptr, 2> sinks{console_sink, file_sink};
    logger = make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    spdlog::initialize_logger(logger);
    return logger;
}
