#pragma once

#include <concepts>
#include <span>
#include <vector>

namespace lc::infra::diff {

enum class operation {
    unchanged, // 同时消费 actual 和 expected
    removed,   // 只消费 actual
    added,     // 只消费 expected
};

using edit_script = std::vector<operation>;

template<class Algorithm, class Token>
concept algorithm_for = requires(const Algorithm algorithm,
                                 std::span<const Token> actual,
                                 std::span<const Token> expected) {
    { algorithm(actual, expected) } -> std::same_as<edit_script>;
};

} // namespace lc::infra::diff
