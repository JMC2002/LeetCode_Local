#pragma once

#include <concepts>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace lc::infra::cases {

template<class T>
struct vector_traits {
    static constexpr bool value = false;
};

template<class T, class Allocator>
struct vector_traits<std::vector<T, Allocator>> {
    static constexpr bool value = true;
    using element_type = T;
};

template<class T>
concept vector_like = vector_traits<std::remove_cvref_t<T>>::value;

namespace detail {

template<class T>
consteval bool is_case_value()
{
    using value_type = std::remove_cvref_t<T>;
    if constexpr (std::same_as<value_type, std::string>
                  || std::same_as<value_type, char>
                  || std::same_as<value_type, bool>
                  || std::integral<value_type>
                  || std::floating_point<value_type>) {
        return true;
    } else if constexpr (vector_like<value_type>) {
        return is_case_value<typename vector_traits<value_type>::element_type>();
    } else {
        return false;
    }
}

} // namespace detail

// 校验、解析和序列化共同使用的递归值模型。
// 只有同时实现解析与序列化分支后，才能在这里加入新类型。
template<class T>
concept case_value = detail::is_case_value<T>();

template<class T>
concept parseable = case_value<T>
    && std::default_initializable<std::remove_cvref_t<T>>
    && std::movable<std::remove_cvref_t<T>>;

template<class T>
concept serializable = case_value<T>;

template<class Left, class Right = Left>
concept judge_comparable = requires(
    const std::remove_cvref_t<Left>& left,
    const std::remove_cvref_t<Right>& right) {
    { left == right } -> std::convertible_to<bool>;
};

namespace detail {

template<class T, class = void>
struct case_arguments : std::false_type {};

template<class T>
struct case_arguments<
    T,
    std::void_t<decltype(std::tuple_size<std::remove_cvref_t<T>>::value)>> {
private:
    using tuple_type = std::remove_cvref_t<T>;

    template<std::size_t... Indices>
    static consteval bool check(std::index_sequence<Indices...>)
    {
        return (parseable<std::tuple_element_t<Indices, tuple_type>> && ...);
    }

public:
    static constexpr bool value =
        check(std::make_index_sequence<std::tuple_size_v<tuple_type>>{});
};

} // namespace detail

template<class T>
concept case_arguments = detail::case_arguments<T>::value;

} // namespace lc::infra::cases
