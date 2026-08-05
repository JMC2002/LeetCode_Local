#pragma once

#include <algorithm>
#include <cstddef>
#include <ranges>

namespace lc::infra::diff {

template<std::ranges::forward_range Left, std::ranges::forward_range Right>
constexpr std::size_t common_prefix_size(Left&& left, Right&& right)
{
    const auto result = std::ranges::mismatch(left, right);
    return static_cast<std::size_t>(
        std::ranges::distance(std::ranges::begin(left), result.in1));
}

} // namespace lc::infra::diff
