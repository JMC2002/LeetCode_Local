#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <mdspan>
#include <ranges>
#include <span>
#include <vector>

#include <lc/infra/diff/range.hpp>
#include <lc/infra/diff/script.hpp>

namespace lc::infra::diff {

template<std::size_t MaxMatrixCells = 1'000'000>
struct lcs_matrix {
    static_assert(MaxMatrixCells > 0);

    template<std::equality_comparable Token>
    edit_script operator()(std::span<const Token> actual,
                           std::span<const Token> expected) const
    {
        const std::size_t prefix = common_prefix_size(actual, expected);
        const std::size_t suffix = common_prefix_size(
            actual.subspan(prefix) | std::views::reverse,
            expected.subspan(prefix) | std::views::reverse);

        edit_script script;
        script.reserve(actual.size() + expected.size());
        script.insert(script.end(), prefix, operation::unchanged);

        const std::size_t actual_middle = actual.size() - prefix - suffix;
        const std::size_t expected_middle = expected.size() - prefix - suffix;
        const std::size_t rows = actual_middle + 1;
        const std::size_t columns = expected_middle + 1;

        if (rows <= MaxMatrixCells / columns) {
            std::vector<std::uint32_t> storage(rows * columns);
            std::mdspan lengths{storage.data(), rows, columns};
            for (std::size_t left = actual_middle; left-- > 0;) {
                for (std::size_t right = expected_middle; right-- > 0;) {
                    if (actual[prefix + left] == expected[prefix + right]) {
                        lengths[left, right] =
                            1 + lengths[left + 1, right + 1];
                    } else {
                        lengths[left, right] = std::max(
                            lengths[left + 1, right],
                            lengths[left, right + 1]);
                    }
                }
            }

            std::size_t left = 0, right = 0;
            while (left < actual_middle || right < expected_middle) {
                if (left < actual_middle
                    && right < expected_middle
                    && actual[prefix + left] == expected[prefix + right]) {
                    script.push_back(operation::unchanged);
                    ++left;
                    ++right;
                } else if (right < expected_middle
                           && (left == actual_middle
                               || lengths[left, right + 1]
                                   >= lengths[left + 1, right])) {
                    script.push_back(operation::added);
                    ++right;
                } else {
                    script.push_back(operation::removed);
                    ++left;
                }
            }
        } else {
            script.insert(script.end(), actual_middle, operation::removed);
            script.insert(script.end(), expected_middle, operation::added);
        }

        script.insert(script.end(), suffix, operation::unchanged);
        return script;
    }
};

} // namespace lc::infra::diff
