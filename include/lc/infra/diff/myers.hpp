#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

#include <lc/infra/diff/lcs.hpp>
#include <lc/infra/diff/range.hpp>
#include <lc/infra/diff/script.hpp>

namespace lc::infra::diff {

// Myers 最短编辑脚本。MaxTraceCells 限制回溯轨迹占用的整数个数；
// 超过限制时，仍在强类型 token 上使用有界 LCS，不会退回序列化文本比较。
template<std::size_t MaxTraceCells = 1'000'000,
         std::size_t MaxFallbackMatrixCells = 1'000'000>
struct myers_trace {
    static_assert(MaxTraceCells > 0);
    static_assert(MaxFallbackMatrixCells > 0);

    template<std::equality_comparable Token>
    edit_script operator()(std::span<const Token> actual,
                           std::span<const Token> expected) const
    {
        const std::size_t prefix = common_prefix_size(actual, expected);
        const std::size_t suffix = common_prefix_size(
            actual.subspan(prefix) | std::views::reverse,
            expected.subspan(prefix) | std::views::reverse);

        const auto actual_middle =
            actual.subspan(prefix, actual.size() - prefix - suffix);
        const auto expected_middle =
            expected.subspan(prefix, expected.size() - prefix - suffix);

        edit_script script;
        script.reserve(actual.size() + expected.size());
        script.insert(script.end(), prefix, operation::unchanged);

        if (actual_middle.empty()) {
            script.insert(
                script.end(), expected_middle.size(), operation::added);
        } else if (expected_middle.empty()) {
            script.insert(
                script.end(), actual_middle.size(), operation::removed);
        } else {
            auto middle = shortest_edit_script(actual_middle, expected_middle);
            script.append_range(middle);
        }

        script.insert(script.end(), suffix, operation::unchanged);
        return script;
    }

private:
    template<class Token>
    static edit_script fallback(std::span<const Token> actual,
                                std::span<const Token> expected)
    {
        return lcs_matrix<MaxFallbackMatrixCells>{}(actual, expected);
    }

    static std::ptrdiff_t diagonal(
        const std::vector<std::ptrdiff_t>& layer,
        std::size_t distance,
        std::ptrdiff_t k)
    {
        const auto index = static_cast<std::size_t>(
            (k + static_cast<std::ptrdiff_t>(distance)) / 2);
        return layer[index];
    }

    template<class Token>
    static edit_script shortest_edit_script(std::span<const Token> actual,
                                            std::span<const Token> expected)
    {
        using coordinate = std::ptrdiff_t;

        const auto actual_size = static_cast<coordinate>(actual.size());
        const auto expected_size = static_cast<coordinate>(expected.size());
        const std::size_t maximum_distance = actual.size() + expected.size();

        std::vector<std::vector<coordinate>> trace;
        trace.reserve(std::min(maximum_distance + 1, MaxTraceCells));
        std::size_t trace_cells = 0;

        for (std::size_t distance = 0;
             distance <= maximum_distance;
             ++distance) {
            if (distance + 1 > MaxTraceCells - trace_cells) {
                return fallback(actual, expected);
            }

            std::vector<coordinate> current(distance + 1);
            const auto signed_distance = static_cast<coordinate>(distance);

            for (std::size_t offset = 0; offset <= distance; ++offset) {
                const coordinate k =
                    -signed_distance + 2 * static_cast<coordinate>(offset);
                coordinate x = 0;

                if (distance != 0) {
                    const auto& previous = trace.back();
                    if (k == -signed_distance) {
                        x = diagonal(previous, distance - 1, k + 1);
                    } else if (k == signed_distance) {
                        x = diagonal(previous, distance - 1, k - 1) + 1;
                    } else {
                        const coordinate after_addition =
                            diagonal(previous, distance - 1, k + 1);
                        const coordinate after_removal =
                            diagonal(previous, distance - 1, k - 1) + 1;
                        x = after_addition > after_removal
                            ? after_addition
                            : after_removal;
                    }
                }

                coordinate y = x - k;
                while (x < actual_size
                       && y < expected_size
                       && actual[static_cast<std::size_t>(x)]
                           == expected[static_cast<std::size_t>(y)]) {
                    ++x;
                    ++y;
                }
                current[offset] = x;

                if (x == actual_size && y == expected_size) {
                    trace_cells += current.size();
                    trace.push_back(std::move(current));
                    return backtrack(trace, actual.size(), expected.size());
                }
            }

            trace_cells += current.size();
            trace.push_back(std::move(current));
        }

        // 编辑距离最多为 N + M，理论上不会走到这里。
        return fallback(actual, expected);
    }

    static edit_script backtrack(
        const std::vector<std::vector<std::ptrdiff_t>>& trace,
        std::size_t actual_size,
        std::size_t expected_size)
    {
        using coordinate = std::ptrdiff_t;

        coordinate x = static_cast<coordinate>(actual_size);
        coordinate y = static_cast<coordinate>(expected_size);
        edit_script reversed;
        reversed.reserve(actual_size + expected_size);

        for (std::size_t distance = trace.size() - 1;
             distance > 0;
             --distance) {
            const coordinate signed_distance =
                static_cast<coordinate>(distance);
            const coordinate k = x - y;
            const auto& previous = trace[distance - 1];

            coordinate previous_k = 0;
            if (k == -signed_distance) {
                previous_k = k + 1;
            } else if (k == signed_distance) {
                previous_k = k - 1;
            } else {
                const coordinate after_addition =
                    diagonal(previous, distance - 1, k + 1);
                const coordinate after_removal =
                    diagonal(previous, distance - 1, k - 1) + 1;
                previous_k = after_addition > after_removal
                    ? k + 1
                    : k - 1;
            }

            const coordinate previous_x =
                diagonal(previous, distance - 1, previous_k);
            const coordinate previous_y = previous_x - previous_k;

            while (x > previous_x && y > previous_y) {
                reversed.push_back(operation::unchanged);
                --x;
                --y;
            }

            if (x == previous_x) {
                reversed.push_back(operation::added);
                --y;
            } else {
                reversed.push_back(operation::removed);
                --x;
            }
        }

        while (x > 0 && y > 0) {
            reversed.push_back(operation::unchanged);
            --x;
            --y;
        }
        reversed.insert(reversed.end(), static_cast<std::size_t>(x),
                        operation::removed);
        reversed.insert(reversed.end(), static_cast<std::size_t>(y),
                        operation::added);
        std::ranges::reverse(reversed);
        return reversed;
    }
};

} // namespace lc::infra::diff
