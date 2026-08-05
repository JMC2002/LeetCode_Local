#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <lc/infra/cases/concepts.hpp>
#include <lc/infra/cases/serializer.hpp>
#include <lc/infra/diff/script.hpp>

namespace lc::infra::diff {

enum class alignment_operation {
    unchanged,
    replaced,
    removed,
    added,
};

inline constexpr std::size_t missing_index =
    std::numeric_limits<std::size_t>::max();

template<std::equality_comparable Token, class Visitor>
void visit_alignment(std::span<const Token> actual,
                     std::span<const Token> expected,
                     const edit_script& script,
                     Visitor&& visitor);

namespace detail {

[[noreturn]] inline void invalid_script()
{
    throw std::logic_error{"diff 算法返回了无效的编辑脚本"};
}

template<class Token>
void validate_script(std::span<const Token> actual,
                     std::span<const Token> expected,
                     const edit_script& script)
{
    std::size_t actual_index = 0;
    std::size_t expected_index = 0;

    for (const auto step : script) {
        if (step == operation::unchanged && (actual_index >= actual.size()
                                          || expected_index >= expected.size()
                                          || actual[actual_index] != expected[expected_index]))
            invalid_script();

        actual_index += step != operation::added;
        expected_index += step != operation::removed;
    }

    if (actual_index != actual.size() || expected_index != expected.size()) {
        invalid_script();
    }
}

struct change_run {
    std::size_t end;
    std::size_t removed;
    std::size_t added;
};

inline change_run scan_change_run(const edit_script& script,
                                  std::size_t begin)
{
    change_run result{begin, 0, 0};
    while (result.end < script.size()
           && script[result.end] != operation::unchanged) {
        if (script[result.end] == operation::removed) {
            ++result.removed;
        } else {
            ++result.added;
        }
        ++result.end;
    }
    return result;
}

template<class Visitor>
void emit_change_run(const change_run& run,
                     std::size_t actual_index,
                     std::size_t expected_index,
                     Visitor& visitor)
{
    const std::size_t paired = std::min(run.removed, run.added);
    for (std::size_t index = 0; index < paired; ++index) {
        visitor(alignment_operation::replaced,
                actual_index + index, expected_index + index);
    }
    for (std::size_t index = paired; index < run.removed; ++index) {
        visitor(alignment_operation::removed,
                actual_index + index, missing_index);
    }
    for (std::size_t index = paired; index < run.added; ++index) {
        visitor(alignment_operation::added,
                missing_index, expected_index + index);
    }
}

template<class Token>
bool equal_subranges(
    std::span<const Token> actual,
    std::span<const Token> expected,
    std::size_t actual_begin,
    std::size_t expected_begin,
    std::size_t count)
{
    return std::ranges::equal(
        actual.subspan(actual_begin, count),
        expected.subspan(expected_begin, count));
}

template<class Algorithm, class T>
consteval bool algorithm_supports_value()
{
    using value_type = std::remove_cvref_t<T>;

    if constexpr (std::same_as<value_type, std::string>) {
        return algorithm_for<Algorithm, std::string_view>;
    } else if constexpr (cases::vector_like<value_type>) {
        using element_type =
            typename cases::vector_traits<value_type>::element_type;
        using token_type = std::conditional_t<
            std::same_as<element_type, bool>, std::uint8_t, element_type>;
        return algorithm_for<Algorithm, token_type>
            && algorithm_supports_value<Algorithm, element_type>();
    } else {
        return true;
    }
}

inline bool is_utf8_continuation(unsigned char value)
{
    return (value & 0xc0) == 0x80;
}

inline std::vector<std::string_view> utf8_code_points(std::string_view text)
{
    std::vector<std::string_view> result;
    result.reserve(text.size());

    for (std::size_t offset = 0; offset < text.size();) {
        const auto lead = static_cast<unsigned char>(text[offset]);
        std::size_t width = 1;

        if (lead >= 0xc2 && lead <= 0xdf
            && offset + 1 < text.size()
            && is_utf8_continuation(
                static_cast<unsigned char>(text[offset + 1]))) {
            width = 2;
        } else if (lead >= 0xe0 && lead <= 0xef
                   && offset + 2 < text.size()) {
            const auto second = static_cast<unsigned char>(text[offset + 1]);
            const auto third = static_cast<unsigned char>(text[offset + 2]);
            const bool valid_second =
                is_utf8_continuation(second)
                && (lead != 0xe0 || second >= 0xa0)
                && (lead != 0xed || second <= 0x9f);
            if (valid_second && is_utf8_continuation(third)) width = 3;
        } else if (lead >= 0xf0 && lead <= 0xf4
                   && offset + 3 < text.size()) {
            const auto second = static_cast<unsigned char>(text[offset + 1]);
            const auto third = static_cast<unsigned char>(text[offset + 2]);
            const auto fourth = static_cast<unsigned char>(text[offset + 3]);
            const bool valid_second =
                is_utf8_continuation(second)
                && (lead != 0xf0 || second >= 0x90)
                && (lead != 0xf4 || second <= 0x8f);
            if (valid_second
                && is_utf8_continuation(third)
                && is_utf8_continuation(fourth)) {
                width = 4;
            }
        }

        result.push_back(text.substr(offset, width));
        offset += width;
    }
    return result;
}

template<class Element, class Allocator>
decltype(auto) element_at(const std::vector<Element, Allocator>& values,
                          std::size_t index)
{
    if constexpr (std::same_as<Element, bool>) {
        return static_cast<bool>(values[index]);
    } else {
        return (values[index]);
    }
}

template<class Algorithm, cases::serializable T, class Sink>
    requires (algorithm_supports_value<Algorithm, T>())
void visit_values(const T& actual,
                  const T& expected,
                  const Algorithm& algorithm,
                  Sink& sink);

template<class Algorithm, class Sink>
void visit_strings(const std::string& actual,
                   const std::string& expected,
                   const Algorithm& algorithm,
                   Sink& sink)
{
    const auto actual_units = utf8_code_points(actual);
    const auto expected_units = utf8_code_points(expected);
    const std::span actual_span{actual_units};
    const std::span expected_span{expected_units};
    const auto script = algorithm(actual_span, expected_span);
    validate_script(actual_span, expected_span, script);

    sink.append_both("\"");
    std::size_t actual_index = 0;
    std::size_t expected_index = 0;
    std::string escaped;

    for (const auto step : script) {
        escaped.clear();
        switch (step) {
        case operation::unchanged:
            cases::append_escaped_content(
                escaped, actual_units[actual_index++]);
            ++expected_index;
            sink.append_both(escaped);
            break;
        case operation::removed:
            cases::append_escaped_content(
                escaped, actual_units[actual_index++]);
            sink.append_actual(operation::removed, escaped);
            break;
        case operation::added:
            cases::append_escaped_content(
                escaped, expected_units[expected_index++]);
            sink.append_expected(operation::added, escaped);
            break;
        }
    }
    sink.append_both("\"");
}

template<class Algorithm, class Element, class Allocator, class Sink>
    requires (algorithm_supports_value<
        Algorithm, std::vector<Element, Allocator>>())
void visit_vectors(const std::vector<Element, Allocator>& actual,
                   const std::vector<Element, Allocator>& expected,
                   const Algorithm& algorithm,
                   Sink& sink)
{
    sink.append_both("[");
    bool first = true;

    const auto visitor = [&](alignment_operation kind,
                             std::size_t actual_index,
                             std::size_t expected_index) {
        if (!first) sink.append_both(",");
        first = false;
        sink.begin_cell();

        switch (kind) {
        case alignment_operation::unchanged: {
            const auto text = cases::serialize(
                element_at(actual, actual_index));
            sink.append_both(text);
            break;
        }
        case alignment_operation::replaced:
            visit_values(element_at(actual, actual_index),
                         element_at(expected, expected_index),
                         algorithm, sink);
            break;
        case alignment_operation::removed: {
            const auto text = cases::serialize(
                element_at(actual, actual_index));
            sink.append_actual(operation::removed, text);
            break;
        }
        case alignment_operation::added: {
            const auto text = cases::serialize(
                element_at(expected, expected_index));
            sink.append_expected(operation::added, text);
            break;
        }
        }

        sink.end_cell();
    };

    if constexpr (std::same_as<Element, bool>) {
        const auto actual_tokens =
            std::ranges::to<std::vector<std::uint8_t>>(actual);
        const auto expected_tokens =
            std::ranges::to<std::vector<std::uint8_t>>(expected);
        const std::span actual_span{actual_tokens};
        const std::span expected_span{expected_tokens};
        const auto script = algorithm(actual_span, expected_span);
        visit_alignment(actual_span, expected_span, script, visitor);
    } else {
        const std::span actual_span{actual};
        const std::span expected_span{expected};
        const auto script = algorithm(actual_span, expected_span);
        visit_alignment(actual_span, expected_span, script, visitor);
    }

    sink.append_both("]");
}

template<class Algorithm, cases::serializable T, class Sink>
    requires (algorithm_supports_value<Algorithm, T>())
void visit_values(const T& actual,
                  const T& expected,
                  const Algorithm& algorithm,
                  Sink& sink)
{
    using value_type = std::remove_cvref_t<T>;

    if (actual == expected) {
        const auto text = cases::serialize(actual);
        sink.append_both(text);
    } else if constexpr (std::same_as<value_type, std::string>) {
        visit_strings(actual, expected, algorithm, sink);
    } else if constexpr (cases::vector_like<value_type>) {
        visit_vectors(actual, expected, algorithm, sink);
    } else {
        sink.append_actual(operation::removed, cases::serialize(actual));
        sink.append_expected(operation::added, cases::serialize(expected));
    }
}

} // namespace detail

// 将标准 SES 惰性投影成展示列。这里会引入replaced状态，将类似删了又加变为replaced，视觉上做到更窄
template<std::equality_comparable Token, class Visitor>
void visit_alignment(std::span<const Token> actual,
                     std::span<const Token> expected,
                     const edit_script& script,
                     Visitor&& visitor)
{
    detail::validate_script(actual, expected, script);

    std::size_t script_index = 0;
    std::size_t actual_index = 0;
    std::size_t expected_index = 0;

    while (script_index < script.size()) {
        if (script[script_index] == operation::unchanged) {
            visitor(alignment_operation::unchanged,
                    actual_index++, expected_index++);
            ++script_index;
            continue;
        }

        const auto first = detail::scan_change_run(script, script_index);
        std::size_t unchanged_end = first.end;
        while (unchanged_end < script.size()
               && script[unchanged_end] == operation::unchanged) {
            ++unchanged_end;
        }
        const std::size_t unchanged = unchanged_end - first.end;

        if (unchanged != 0 && unchanged_end < script.size()) {
            const auto second =
                detail::scan_change_run(script, unchanged_end);

            if (first.removed != 0 && first.added == 0
                && second.removed == 0 && second.added != 0) {
                const std::size_t paired =
                    std::min(first.removed, second.added);
                if (detail::equal_subranges(
                        actual, expected,
                        actual_index + first.removed,
                        expected_index + paired,
                        unchanged)) {
                    for (std::size_t index = 0;
                         index < first.removed - paired;
                         ++index) {
                        visitor(alignment_operation::removed,
                                actual_index + index, missing_index);
                    }
                    for (std::size_t index = 0; index < paired; ++index) {
                        visitor(alignment_operation::replaced,
                                actual_index + first.removed - paired + index,
                                expected_index + index);
                    }
                    for (std::size_t index = 0; index < unchanged; ++index) {
                        visitor(alignment_operation::unchanged,
                                actual_index + first.removed + index,
                                expected_index + paired + index);
                    }
                    for (std::size_t index = paired;
                         index < second.added;
                         ++index) {
                        visitor(alignment_operation::added, missing_index,
                                expected_index + unchanged + index);
                    }
                    actual_index += first.removed + unchanged;
                    expected_index += unchanged + second.added;
                    script_index = second.end;
                    continue;
                }
            } else if (first.added != 0 && first.removed == 0
                       && second.added == 0 && second.removed != 0) {
                const std::size_t paired =
                    std::min(first.added, second.removed);
                if (detail::equal_subranges(
                        actual, expected,
                        actual_index + paired,
                        expected_index + first.added,
                        unchanged)) {
                    for (std::size_t index = 0;
                         index < first.added - paired;
                         ++index) {
                        visitor(alignment_operation::added, missing_index,
                                expected_index + index);
                    }
                    for (std::size_t index = 0; index < paired; ++index) {
                        visitor(alignment_operation::replaced,
                                actual_index + index,
                                expected_index + first.added - paired + index);
                    }
                    for (std::size_t index = 0; index < unchanged; ++index) {
                        visitor(alignment_operation::unchanged,
                                actual_index + paired + index,
                                expected_index + first.added + index);
                    }
                    for (std::size_t index = paired;
                         index < second.removed;
                         ++index) {
                        visitor(alignment_operation::removed,
                                actual_index + unchanged + index,
                                missing_index);
                    }
                    actual_index += unchanged + second.removed;
                    expected_index += first.added + unchanged;
                    script_index = second.end;
                    continue;
                }
            }
        }

        detail::emit_change_run(first, actual_index, expected_index, visitor);
        actual_index += first.removed;
        expected_index += first.added;
        script_index = first.end;
    }
}

template<class Algorithm, class T>
concept algorithm_for_value = cases::serializable<T>
    && detail::algorithm_supports_value<Algorithm, T>();

template<class Algorithm, cases::serializable T, class Sink>
    requires algorithm_for_value<Algorithm, T>
void visit_difference(const T& actual,
                      const T& expected,
                      const Algorithm& algorithm,
                      Sink& sink)
{
    detail::visit_values(actual, expected, algorithm, sink);
}

} // namespace lc::infra::diff
