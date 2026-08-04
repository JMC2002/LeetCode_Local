#pragma once

#include <chrono>
#include <format>
#include <string>

namespace lc::infra::runtime {

inline std::string current_local_timestamp()
{
    using namespace std::chrono;

    const zoned_time now{
        current_zone(),
        floor<seconds>(system_clock::now())
    };
    return std::format("{:%F %T}", now);
}

} // namespace lc::infra::runtime
