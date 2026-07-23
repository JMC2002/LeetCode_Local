#pragma once

#include <concepts>
#include <meta>
#include <tuple>
#include <type_traits>

#include <lc/infra/cases/concepts.hpp>
#include <lc/infra/meta/reflection.hpp>

namespace lc::infra::meta {

template<class T>
concept complete = requires { sizeof(T); };

namespace detail {

template<class Solution>
consteval bool has_public_entry()
{
    return entry_v<Solution> != std::meta::info{};
}

template<class Solution>
consteval bool has_required_arguments()
{
    return std::tuple_size_v<arguments_t<Solution>> != 0;
}

} // 命名空间 detail

template<class Solution>
concept has_public_entry = complete<Solution>
                        && detail::has_public_entry<Solution>();

template<class Solution>
concept has_required_arguments = has_public_entry<Solution>
                              && detail::has_required_arguments<Solution>();

template<class Solution>
concept supported_arguments = has_required_arguments<Solution>
                           && cases::case_arguments<arguments_t<Solution>>;

template<class Solution>
concept supported_output = supported_arguments<Solution>
                        && cases::parseable<output_t<Solution>>
                        && cases::serializable<output_t<Solution>>;

template<class Solution>
concept comparable_output = supported_output<Solution>
                         && cases::judge_comparable<output_t<Solution>>;

template<class Solution>
concept runnable_solution = std::default_initializable<Solution>
                         && comparable_output<Solution>;

template<class Spec>
concept problem_spec = requires {
    typename Spec::solution_type;
    { Spec::cases.data() } -> std::convertible_to<const char*>;
    { Spec::cases.size() } -> std::convertible_to<std::size_t>;
};

template<class Spec>
concept runnable_problem = problem_spec<Spec>
                        && runnable_solution<typename Spec::solution_type>;

} // namespace lc::infra::meta
