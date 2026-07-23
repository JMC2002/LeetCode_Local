#pragma once

#include <cstdlib>
#include <format>
#include <print>
#include <string_view>
#include <utility>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace lc::infra::terminal {

enum class style {
    heading,
    running,
    success,
    failure,
    warning,
    muted,
};

constexpr std::string_view escape(style value)
{
    using enum style;
    switch (value) {
    case heading: return "\x1b[1;96m"; // 加粗亮青色
    case running: return "\x1b[96m";   // 亮青色
    case success: return "\x1b[1;92m"; // 加粗亮绿色
    case failure: return "\x1b[1;91m"; // 加粗亮红色
    case warning: return "\x1b[1;93m"; // 加粗亮黄色
    case muted: return "\x1b[2;37m";   // 暗白色
    }
    std::unreachable();
}

inline bool colors_enabled()
{
    static const bool enabled = [] {
        if (const char* mode = std::getenv("LC_COLOR")) {
            const std::string_view value{mode};
            if (value == "always") return true;
            if (value == "never") return false;
        }
        if (std::getenv("NO_COLOR") != nullptr) {
            return false;
        }
#if defined(__linux__)
        return ::isatty(STDOUT_FILENO) != 0;
#else
        return false;
#endif
    }();
    return enabled;
}

template<class... Arguments>
void println(style value,
             std::format_string<Arguments...> fmt,
             Arguments&&... args)
{
    const auto text = std::format(fmt, std::forward<Arguments>(args)...);
    if (colors_enabled()) {
        std::println("{}{}\x1b[0m", escape(value), text);
    } else {
        std::println("{}", text);
    }
}

} // namespace lc::infra::terminal
