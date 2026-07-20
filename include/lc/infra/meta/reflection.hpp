#pragma once

#include <complex>
#include <meta>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

namespace lc::infra::meta {

template<class Solution>
consteval std::meta::info entry_of()
{
    for (auto member : std::meta::members_of(
             ^^Solution, std::meta::access_context::current())) {
        if (std::meta::is_public(member)                // 将头一个公有的
            && std::meta::has_identifier(member)        // 普通的
            && std::meta::is_user_provided(member)      // 用户提供的
            && !std::meta::is_deleted(member)           // 未被删除的
            && std::meta::is_function(member)) {        // 函数作为被调用的
            return member;
        }
    }
    return {};
}

template<class Solution>
inline constexpr std::meta::info entry_v = entry_of<Solution>();

consteval std::meta::info required_arguments_tuple_of(std::meta::info function)
{
    auto types = std::meta::parameters_of(function)
               | std::views::take_while(std::not_fn<&std::meta::has_default_argument>())
               | std::views::transform(std::meta::type_of)
               | std::views::transform(std::meta::remove_cvref)
               | std::ranges::to<std::vector>();
    return std::meta::substitute(^^std::tuple, types);
}

template<class Solution>
using arguments_t = [:required_arguments_tuple_of(entry_v<Solution>):];

template<class Solution>
using return_t = [:std::meta::remove_cvref(
    std::meta::return_type_of(entry_v<Solution>)):];

template<class Solution, class Tuple>
decltype(auto) invoke(Solution& solution, Tuple& arguments)
{
    auto& [...args] = arguments;
    return solution.[:entry_v<Solution>:](args...);
}

} // namespace lc::infra::meta

