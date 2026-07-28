#pragma once

#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include <lc/infra/terminal/style.hpp>

namespace lc::infra::runtime {

inline bool debugger_attached()
{
#ifdef __linux__
    std::ifstream status{"/proc/self/status"};

    for (std::string field; status >> field;) {
        if (field != "TracerPid:") continue;
        int process_id = 0;
        return status >> process_id && process_id != 0;
    }
#endif
    return false;
}

inline void keep_console_open_for_debugger()
{
    if (!debugger_attached()) {
        return;
    }
    // terminal::println(terminal::style::muted, "\n按 Enter 键关闭...");
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

} // namespace lc::infra::runtime
