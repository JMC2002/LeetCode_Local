#pragma once

#include <charconv>
#include <concepts>
#include <format>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <lc/infra/cases/concepts.hpp>

namespace lc::infra::cases {

inline void append_escaped_content(std::string& output, std::string_view value)
{
    for (const char ch : value) {
        switch (ch) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default: output.push_back(ch); break;
        }
    }
}

inline void append_escaped(std::string& output, std::string_view value)
{
    output.push_back('"');
    append_escaped_content(output, value);
    output.push_back('"');
}

template<serializable T>
void append(std::string& output, const T& value)
{
    using V = std::remove_cvref_t<T>;

    if constexpr (std::same_as<V, std::string>)
        append_escaped(output, value);
    else if constexpr (std::same_as<V, char>)
        append_escaped(output, { &value, 1 });
    else if constexpr (std::integral<V> || std::floating_point<V>)
        std::format_to(std::back_inserter(output), "{}", value);
    else if constexpr (vector_like<V>) {
        output += '[';
        for (std::string_view sep; const auto& element : value) {
            output += std::exchange(sep, ",");
            append(output, element);
        }
        output += ']';
    }
    else {
        static_assert(!case_value<V>,
                      "case_value 新增了尚未实现的序列化分支");
        std::unreachable();
    }
}

template<serializable T>
std::string serialize(const T& value)
{
    std::string output;
    append(output, value);
    return output;
}

} // namespace lc::infra::cases
