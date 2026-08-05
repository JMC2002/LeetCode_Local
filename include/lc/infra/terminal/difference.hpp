#pragma once

#include <print>
#include <string>
#include <string_view>
#include <utility>

#include <lc/infra/cases/concepts.hpp>
#include <lc/infra/diff/value.hpp>
#include <lc/infra/terminal/difference_sink.hpp>
#include <lc/infra/terminal/style.hpp>

namespace lc::infra::terminal {
namespace detail {

inline void append_label(std::string& output,
                         std::string_view label,
                         style value,
                         bool colors)
{
    if (colors) {
        append_styled(output, value, label);
    } else {
        output.append(label);
    }
}

} // namespace detail

template<class Algorithm, cases::serializable T>
    requires diff::algorithm_for_value<Algorithm, T>
std::string format_difference(const T& actual,
                              const T& expected,
                              bool colors,
                              const Algorithm& algorithm = {})
{
    detail::difference_sink sink{colors};
    diff::visit_difference(actual, expected, algorithm, sink);
    auto [actual_line, expected_line] = std::move(sink).finish();

    std::string output;
    detail::append_label(output, "  - 实际：", style::failure, colors);
    output.append(actual_line);
    output.push_back('\n');
    detail::append_label(output, "  + 期望：", style::success, colors);
    output.append(expected_line);
    output.push_back('\n');
    return output;
}

template<class Algorithm, cases::serializable T>
    requires diff::algorithm_for_value<Algorithm, T>
void print_difference(const T& actual,
                      const T& expected,
                      const Algorithm& algorithm = {})
{
    std::print(
        "{}", format_difference(actual, expected, colors_enabled(), algorithm));
}

} // namespace lc::infra::terminal
