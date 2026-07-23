#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <inplace_vector>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include <lc/infra/cases/concepts.hpp>

namespace lc::infra::cases {

using namespace std::string_view_literals;

enum class error_kind {
    none,
    empty_file,
    unsupported_type,
    expected_value,
    expected_digit,
    expected_string,
    expected_left_bracket,
    expected_comma_or_right_bracket,
    expected_true_or_false,
    unterminated_string,
    invalid_escape,
    unescaped_control_character,
    invalid_unicode_surrogate,
    invalid_number,
    integer_out_of_range,
    floating_point_out_of_range,
    invalid_char_length,
};

struct case_error {
    error_kind kind = error_kind::none;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
    std::size_t test_case = 0;
    std::size_t argument = 0;
};

struct validation_summary {
    std::size_t test_count = 0;
};

using validation_result = std::expected<validation_summary, case_error>;

template<class T>
using parse_result = std::expected<std::remove_cvref_t<T>, case_error>;

using static_message = std::inplace_vector<char, 256>;

constexpr std::string_view error_text(error_kind error)
{
    switch (error) {
    using enum error_kind;
    case empty_file:                      return "cases.txt 为空";
    case unsupported_type:                return "尚不支持反射得到的 C++ 类型";
    case expected_value:                  return "此处需要一个值";
    case expected_digit:                  return "此处需要一个十进制数字";
    case expected_string:                 return "此处需要一个带双引号的 JSON 字符串";
    case expected_left_bracket:           return "此处需要 '['";
    case expected_comma_or_right_bracket: return "此处需要 ',' 或 ']'";
    case expected_true_or_false:          return "此处需要 true 或 false";
    case unterminated_string:             return "字符串缺少结束引号";
    case invalid_escape:                  return "JSON 字符串转义无效";
    case unescaped_control_character:     return "JSON 字符串包含未转义的控制字符";
    case invalid_unicode_surrogate:       return "JSON 字符串包含无效的 Unicode 代理项";
    case invalid_number:                  return "数字无效";
    case integer_out_of_range:            return "整数超出反射所得 C++ 类型的范围";
    case floating_point_out_of_range:     return "浮点数无效或超出范围";
    case invalid_char_length:             return "char 需要恰好包含一个字节的 JSON 字符串";
    case none:                            return "未知的 cases.txt 错误";
    }
    return "未知的 cases.txt 错误";
}

constexpr static_message make_message(const case_error& error)
{
    static_message message;
    auto output = std::back_inserter(message);
    std::format_to(output, "cases.txt：{}", error_text(error.kind));
    if (error.test_case != 0) {
        std::format_to(output, "；用例 {}", error.test_case);
    }
    if (error.argument != 0) {
        std::format_to(output, "，参数 {}", error.argument);
    }
    std::format_to(output, "，位于第 {} 行第 {} 列", error.line, error.column);
    return message;
}

consteval auto diagnostic(const validation_result& validation)
{
    return validation.has_value()
        ? static_message{}
        : make_message(validation.error());
}

inline std::string describe(const case_error& error)
{
    const auto message = make_message(error);
    return {message.data(), message.size()};
}

class cursor {
protected:
    struct number_token {
        std::size_t begin;
        std::size_t end;
    };

    std::string_view input_;
    std::size_t offset_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;

    static constexpr bool is_digit(char ch)
    {   // std::isdigit没有constexpr版本
        return ch >= '0' && ch <= '9';
    }

    static constexpr int hex_value(char ch)
    {
        if (is_digit(ch)) return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    }

    constexpr char peek() const
    {
        return offset_ < input_.size() ? input_[offset_] : '\0';
    }

    constexpr char take()
    {
        const char ch = peek();
        if (offset_ < input_.size()) {
            ++offset_;
            if (ch == '\n') {
                ++line_;
                column_ = 1;
            } else {
                ++column_;
            }
        }
        return ch;
    }

    constexpr bool take_if(char expected)
    {
        if (peek() != expected) {
            return false;
        }
        take();
        return true;
    }

    constexpr bool take_if(std::string_view expected)
    {
        if (!input_.substr(offset_).starts_with(expected)) {
            return false;
        }
        for (std::size_t index = 0; index < expected.size(); ++index) {
            take();
        }
        return true;
    }

    constexpr void skip_space()
    {
        if (offset_ == 0 && input_.starts_with("\xEF\xBB\xBF"sv)) {
            offset_ += 3;   // 跳过 BOM
        }
        while (" \t\r\n"sv.contains(peek())) {
            take();
        }
    }

    constexpr std::expected<number_token, error_kind> consume_number(bool floating_point)
    {
        skip_space();
        const std::size_t begin = offset_;
        take_if('-');
        if (!is_digit(peek())) {
            return std::unexpected(error_kind::expected_digit);
        }
        if (take_if('0')) {
            if (is_digit(peek())) {
                return std::unexpected(error_kind::invalid_number);
            }
        } else {
            while (is_digit(peek())) {
                take();
            }
        }
        if (floating_point && take_if('.')) {
            if (!is_digit(peek())) {
                return std::unexpected(error_kind::invalid_number);
            }
            while (is_digit(peek())) {
                take();
            }
        }
        if (floating_point && (peek() == 'e' || peek() == 'E')) {
            take();
            if (peek() == '+' || peek() == '-') {
                take();
            }
            if (!is_digit(peek())) {
                return std::unexpected(error_kind::invalid_number);
            }
            while (is_digit(peek())) {
                take();
            }
        }
        return number_token{begin, offset_};
    }

    constexpr std::expected<std::uint32_t, error_kind> consume_hex_quad()
    {
        std::uint32_t codepoint = 0;
        for (int count = 0; count < 4; ++count) {
            const int digit = hex_value(peek());
            if (digit < 0) {
                return std::unexpected(error_kind::invalid_escape);
            }
            take();
            codepoint = codepoint * 16 + static_cast<std::uint32_t>(digit);
        }
        return codepoint;
    }

    template<class Sink>
    static constexpr std::size_t emit_utf8(std::uint32_t codepoint, Sink& sink)
        pre(codepoint <= 0x10FFFF)
        pre(codepoint < 0xD800 || codepoint > 0xDFFF)
    {
        const std::size_t size = codepoint <= 0x7F ? 1
            : codepoint <= 0x7FF ? 2
            : codepoint <= 0xFFFF ? 3
            : 4;
        if (size == 1) {
            sink(static_cast<char>(codepoint));
            return size;
        }
        sink(static_cast<char>(
            (0xF00 >> size) | (codepoint >> (6 * (size - 1)))));
        for (std::size_t index = size - 1; index-- > 0;) {
            sink(static_cast<char>(
                0x80 | ((codepoint >> (6 * index)) & 0x3F)));
        }
        return size;
    }

    template<class Sink>
    constexpr std::expected<std::size_t, error_kind> consume_string(Sink sink)
    {
        skip_space();
        if (!take_if('"')) {
            return std::unexpected(error_kind::expected_string);
        }

        std::size_t size = 0;
        while (offset_ < input_.size()) {
            const auto byte = static_cast<unsigned char>(input_[offset_]);
            if (byte < 0x20) {
                return std::unexpected(error_kind::unescaped_control_character);
            }
            const char ch = take();
            if (ch == '"') {
                return size;
            }
            if (ch != '\\') {
                sink(ch);
                ++size;
                continue;
            }

            if (offset_ == input_.size()) {
                return std::unexpected(error_kind::unterminated_string);
            }
            constexpr auto escape_codes = "\"\\/bfnrt"sv;
            constexpr auto escape_values = "\"\\/\b\f\n\r\t"sv;
            const char escape = take();
            if (const auto index = escape_codes.find(escape);
                index != std::string_view::npos) {
                sink(escape_values[index]);
                ++size;
                continue;
            }
            if (escape != 'u') {
                return std::unexpected(error_kind::invalid_escape);
            }

            auto first = consume_hex_quad();
            if (!first) {
                return std::unexpected(first.error());
            }

            std::uint32_t codepoint = *first;
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                if (!take_if("\\u")) {
                    return std::unexpected(error_kind::invalid_unicode_surrogate);
                }
                auto second = consume_hex_quad();
                if (!second) {
                    return std::unexpected(second.error());
                }
                if (*second < 0xDC00 || *second > 0xDFFF) {
                    return std::unexpected(error_kind::invalid_unicode_surrogate);
                }
                codepoint = 0x10000
                    + ((codepoint - 0xD800) << 10)
                    + (*second - 0xDC00);
            } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                return std::unexpected(error_kind::invalid_unicode_surrogate);
            }
            size += emit_utf8(codepoint, sink);
        }
        return std::unexpected(error_kind::unterminated_string);
    }

    template<std::integral T>
    constexpr bool integer_fits(number_token token) const
    {
        T value{};
        const auto* first = input_.data() + token.begin;
        const auto* last  = input_.data() + token.end;
        const auto result = std::from_chars(first, last, value);
        return result && result.ptr == last;
    }

    // C++26的from_chars的浮点版本不是constexpr的，又不想引入fast_float，随便糊弄一下，当前实现只能大体正确，期待C++29
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3908r0.html
    template<std::floating_point T>
    constexpr bool floating_point_fits(number_token token) const
    {
        using comparison_type = long double;

        auto number = input_.substr(token.begin, token.end - token.begin);
        number.remove_prefix(number.front() == '-');
        const auto exponent_marker = number.find_first_of("eE");
        const auto significand = number.substr(0, exponent_marker);
        const auto point = significand.find('.');

        long long explicit_exponent = 0;
        if (exponent_marker != std::string_view::npos) {
            auto exponent = number.substr(exponent_marker + 1);
            const bool negative = exponent.front() == '-';
            exponent.remove_prefix(negative || exponent.front() == '+');
            int magnitude = 0;
            if (!std::from_chars(exponent.begin(), exponent.end(), magnitude)) {
                magnitude = 100'000;
            }
            explicit_exponent = negative ? -magnitude : magnitude;
        }

        std::size_t digit_index = 0;
        std::size_t first_nonzero = std::string_view::npos;
        comparison_type normalized = 0;
        comparison_type decimal_place = 1;
        int significant_digits = 0;
        constexpr int digits_to_keep =
            std::numeric_limits<comparison_type>::max_digits10 + 2;
        for (const char ch : significand) {
            if (ch == '.') {
                continue;
            }
            const int digit = ch - '0';
            if (first_nonzero == std::string_view::npos) {
                if (digit == 0) {
                    ++digit_index;
                    continue;
                }
                first_nonzero = digit_index;
            }
            if (significant_digits == 0) {
                normalized = static_cast<comparison_type>(digit);
            } else if (significant_digits < digits_to_keep) {
                decimal_place /= comparison_type{10};
                normalized += static_cast<comparison_type>(digit) * decimal_place;
            }
            ++significant_digits;
            ++digit_index;
        }
        if (first_nonzero == std::string_view::npos) {
            return true;
        }

        const auto integer_digits = point == std::string_view::npos
            ? significand.size()
            : point;
        const auto scientific_exponent = explicit_exponent
            + static_cast<long long>(integer_digits)
            - static_cast<long long>(first_nonzero) - 1;

        const auto normalize_decimal = [](comparison_type value) {
            long long exponent = 0;
            while (value >= comparison_type{10}) {
                value /= comparison_type{10};
                ++exponent;
            }
            while (value < comparison_type{1}) {
                value *= comparison_type{10};
                --exponent;
            }
            return std::pair{value, exponent};
        };

        const auto [maximum, maximum_exponent] = normalize_decimal(
            static_cast<comparison_type>(std::numeric_limits<T>::max()));
        const auto overflow_limit = maximum
            + maximum
                * static_cast<comparison_type>(std::numeric_limits<T>::epsilon())
                / comparison_type{4};
        if (scientific_exponent > maximum_exponent
            || (scientific_exponent == maximum_exponent
                && !(normalized < overflow_limit))) {
            return false;
        }

        comparison_type minimum =
            static_cast<comparison_type>(std::numeric_limits<T>::denorm_min());
        if (minimum == comparison_type{0}) {
            minimum = static_cast<comparison_type>(std::numeric_limits<T>::min());
        }
        auto [underflow_limit, minimum_exponent] = normalize_decimal(minimum);
        underflow_limit /= comparison_type{2};
        if (underflow_limit < comparison_type{1}) {
            underflow_limit *= comparison_type{10};
            --minimum_exponent;
        }
        return scientific_exponent > minimum_exponent
            || (scientific_exponent == minimum_exponent
                && normalized > underflow_limit);
    }

    constexpr case_error make_error(
        error_kind kind, std::size_t test_case, std::size_t argument) const
    {
        return {
            .kind = kind,
            .offset = offset_,
            .line = line_,
            .column = column_,
            .test_case = test_case,
            .argument = argument,
        };
    }

public:
    constexpr explicit cursor(std::string_view input) : input_(input) {}

    constexpr bool at_end()
    {
        skip_space();
        return offset_ == input_.size();
    }

    constexpr bool take_arrow()
    {
        skip_space();
        return take_if("=>");
    }
};

class validator : public cursor {
    case_error error_{};
    std::size_t test_case_ = 0;
    std::size_t argument_ = 0;
    std::size_t test_count_ = 0;

    constexpr bool fail(error_kind kind)
    {
        error_ = make_error(kind, test_case_, argument_);
        return false;
    }

    template<case_value T>
    constexpr bool validate_value()
    {
        using value_type = std::remove_cvref_t<T>;
        skip_space();
        if constexpr (std::same_as<value_type, std::string>
                   || std::same_as<value_type, char>) {
            auto size = consume_string([](char) {});
            if (!size) {
                return fail(size.error());
            }
            if constexpr (std::same_as<value_type, char>) {
                if (*size != 1) {
                    return fail(error_kind::invalid_char_length);
                }
            }
            return true;
        } else if constexpr (std::same_as<value_type, bool>) {
            return take_if("true") || take_if("false")
                || fail(error_kind::expected_true_or_false);
        } else if constexpr (std::integral<value_type>) {
            auto token = consume_number(false);
            if (!token) {
                return fail(token.error());
            }
            return integer_fits<value_type>(*token)
                || fail(error_kind::integer_out_of_range);
        } else if constexpr (std::floating_point<value_type>) {
            auto token = consume_number(true);
            if (!token) {
                return fail(token.error());
            }
            return floating_point_fits<value_type>(*token)
                || fail(error_kind::floating_point_out_of_range);
        } else if constexpr (vector_traits<value_type>::value) {
            if (!take_if('[')) {
                return fail(error_kind::expected_left_bracket);
            }
            skip_space();
            if (take_if(']')) {
                return true;
            }
            while (true) {
                if (!validate_value<typename vector_traits<value_type>::element_type>()) {
                    return false;
                }
                skip_space();
                if (take_if(']')) {
                    return true;
                }
                if (!take_if(',')) {
                    return fail(error_kind::expected_comma_or_right_bracket);
                }
            }
        }
        else {
            static_assert(!case_value<value_type>, "case_value 新增了尚未实现的解析分支");
            std::unreachable();
        }
    }

    template<case_arguments Tuple>
    constexpr bool validate_tuple()
    {
        auto&& [...args] = Tuple{};     // 这里会在编译期造成一些开销，不过这么写太简洁了
        return ((++argument_, validate_value<decltype(args)>()) && ...);
    }

public:
    constexpr explicit validator(std::string_view input) : cursor(input) {}

    template<case_arguments Arguments, parseable Expected>
    constexpr validation_result validate()
    {
        if (at_end()) {
            fail(error_kind::empty_file);
            return std::unexpected(error_);
        }

        while (!at_end()) {
            const std::size_t begin = offset_;
            ++test_case_;
            argument_ = 0;
            if (!validate_tuple<Arguments>()) {
                return std::unexpected(error_);
            }
            if (take_arrow()) {
                argument_ = std::tuple_size_v<Arguments> + 1;
                if (!validate_value<Expected>()) {
                    return std::unexpected(error_);
                }
            }
            if (offset_ == begin) {
                fail(error_kind::expected_value);
                return std::unexpected(error_);
            }
            ++test_count_;
        }
        return validation_summary{.test_count = test_count_};
    }
};

template<case_arguments Arguments, parseable Expected>
consteval validation_result validate(std::string_view input)
{
    return validator{input}.template validate<Arguments, Expected>();
}

class parser : public cursor {
    std::size_t test_case_ = 0;
    std::size_t argument_ = 0;

    std::unexpected<case_error> fail(error_kind kind) const
    {
        return std::unexpected(make_error(kind, test_case_, argument_));
    }

    parse_result<std::string> parse_string()
    {
        std::string result;
        auto size = consume_string([&result](char ch) { result.push_back(ch); });
        if (!size) {
            return fail(size.error());
        }
        return result;
    }

    template<class T>
        requires std::integral<T> || std::floating_point<T>
    parse_result<T> parse_number()
    {
        auto token = consume_number(std::floating_point<T>);
        if (!token) {
            return fail(token.error());
        }

        T value{};
        const auto* first = input_.data() + token->begin;
        const auto* last  = input_.data() + token->end;
        const auto result = std::from_chars(first, last, value);
        if (!result || result.ptr != last) {
            constexpr auto error = std::integral<T>
                ? error_kind::integer_out_of_range
                : error_kind::floating_point_out_of_range;
            return fail(error);
        }
        return value;
    }

    template<case_arguments Tuple>
    parse_result<Tuple> parse_tuple()
    {
        Tuple result{};
        template for (auto& element : result) {
            ++argument_;
            using element_type = std::remove_cvref_t<decltype(element)>;
            auto value = parse_value<element_type>();
            if (!value) {
                return std::unexpected(value.error());
            }
            element = std::move(*value);
        }
        return result;
    }

    template<parseable T>
    parse_result<T> parse_value()
    {
        using value_type = std::remove_cvref_t<T>;
        skip_space();
        if constexpr (std::same_as<value_type, std::string>) {
            return parse_string();
        } else if constexpr (std::same_as<value_type, char>) {
            auto value = parse_string();
            if (!value) {
                return std::unexpected(value.error());
            }
            if (value->size() != 1) {
                return fail(error_kind::invalid_char_length);
            }
            return value->front();
        } else if constexpr (std::same_as<value_type, bool>) {
            if (take_if("true")) return true;
            if (take_if("false")) return false;
            return fail(error_kind::expected_true_or_false);
        } else if constexpr (std::integral<value_type> || std::floating_point<value_type>) {
            return parse_number<value_type>();
        } else if constexpr (vector_traits<value_type>::value) {
            if (!take_if('[')) {
                return fail(error_kind::expected_left_bracket);
            }
            value_type result;
            skip_space();
            if (take_if(']')) {
                return result;
            }
            while (true) {
                auto element = parse_value<typename vector_traits<value_type>::element_type>();
                if (!element) {
                    return std::unexpected(element.error());
                }
                result.push_back(std::move(*element));
                skip_space();
                if (take_if(']')) {
                    return result;
                }
                if (!take_if(',')) {
                    return fail(error_kind::expected_comma_or_right_bracket);
                }
            }
        }
        else {
            static_assert(!case_value<value_type>, "case_value 新增了尚未实现的解析分支");
            std::unreachable();
        }
    }

public:
    using cursor::cursor;

    template<parseable T>
    parse_result<T> parse(std::size_t test_case, std::size_t argument)
    {
        test_case_ = test_case;
        argument_  = argument;
        return parse_value<T>();
    }

    template<case_arguments Tuple>
    parse_result<Tuple> parse_arguments(std::size_t test_case)
    {
        test_case_ = test_case;
        argument_ = 0;
        return parse_tuple<Tuple>();
    }
};

} // namespace lc::infra::cases
