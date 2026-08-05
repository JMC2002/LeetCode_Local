#include <lc/infra/diff/lcs.hpp>
#include <lc/infra/diff/myers.hpp>
#include <lc/infra/diff/range.hpp>
#include <lc/infra/diff/value.hpp>
#include <lc/infra/terminal/difference.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

struct replace_all {
    template<class Token>
    lc::infra::diff::edit_script operator()(
        std::span<const Token> actual,
        std::span<const Token> expected) const
    {
        lc::infra::diff::edit_script result;
        result.insert(
            result.end(), actual.size(), lc::infra::diff::operation::removed);
        result.insert(
            result.end(), expected.size(), lc::infra::diff::operation::added);
        return result;
    }
};

void expect_equal(std::string_view name,
                  std::string_view actual,
                  std::string_view expected)
{
    if (actual == expected) return;

    ++failures;
    std::println(stderr, "[FAIL] {}\n实际：{}\n期望：{}", name, actual, expected);
}

void expect_size(std::string_view name,
                 std::size_t actual,
                 std::size_t expected)
{
    if (actual == expected) return;

    ++failures;
    std::println(stderr, "[FAIL] {}：{} != {}", name, actual, expected);
}

template<class Token>
std::size_t validate_script(std::string_view name,
                            std::span<const Token> actual,
                            std::span<const Token> expected,
                            const lc::infra::diff::edit_script& script)
{
    using enum lc::infra::diff::operation;
    std::size_t actual_index = 0;
    std::size_t expected_index = 0;
    std::size_t edits = 0;

    for (const auto step : script) {
        switch (step) {
        case unchanged:
            if (actual_index == actual.size()
                || expected_index == expected.size()
                || actual[actual_index] != expected[expected_index]) {
                ++failures;
                std::println(stderr, "[FAIL] {}：无效的 unchanged", name);
                return static_cast<std::size_t>(-1);
            }
            ++actual_index;
            ++expected_index;
            break;
        case removed:
            if (actual_index == actual.size()) {
                ++failures;
                std::println(stderr, "[FAIL] {}：removed 越界", name);
                return static_cast<std::size_t>(-1);
            }
            ++actual_index;
            ++edits;
            break;
        case added:
            if (expected_index == expected.size()) {
                ++failures;
                std::println(stderr, "[FAIL] {}：added 越界", name);
                return static_cast<std::size_t>(-1);
            }
            ++expected_index;
            ++edits;
            break;
        }
    }

    if (actual_index != actual.size() || expected_index != expected.size()) {
        ++failures;
        std::println(stderr, "[FAIL] {}：编辑脚本未消费完整序列", name);
        return static_cast<std::size_t>(-1);
    }
    return edits;
}

std::vector<int> bit_sequence(std::uint32_t bits, std::size_t length)
{
    std::vector<int> result(length);
    for (std::size_t index = 0; index < length; ++index) {
        result[index] = (bits >> index) & 1u;
    }
    return result;
}

void test_myers_against_lcs()
{
    const lc::infra::diff::myers_trace<> myers;
    const lc::infra::diff::lcs_matrix<> lcs;

    for (std::size_t actual_length = 0; actual_length <= 5; ++actual_length) {
        for (std::size_t expected_length = 0;
             expected_length <= 5;
             ++expected_length) {
            const auto actual_count = 1u << actual_length;
            const auto expected_count = 1u << expected_length;
            for (std::uint32_t actual_bits = 0;
                 actual_bits < actual_count;
                 ++actual_bits) {
                for (std::uint32_t expected_bits = 0;
                     expected_bits < expected_count;
                     ++expected_bits) {
                    const auto actual =
                        bit_sequence(actual_bits, actual_length);
                    const auto expected =
                        bit_sequence(expected_bits, expected_length);
                    const std::span<const int> actual_span{actual};
                    const std::span<const int> expected_span{expected};
                    const auto myers_script = myers(actual_span, expected_span);
                    const auto lcs_script = lcs(actual_span, expected_span);
                    const auto myers_edits = validate_script(
                        "Myers 穷举", actual_span, expected_span, myers_script);
                    const auto lcs_edits = validate_script(
                        "LCS 穷举", actual_span, expected_span, lcs_script);
                    if (myers_edits != lcs_edits) {
                        ++failures;
                        std::println(
                            stderr,
                            "[FAIL] Myers 非最短：长度 {} -> {}，{} != {}",
                            actual_length, expected_length,
                            myers_edits, lcs_edits);
                        return;
                    }
                }
            }
        }
    }
}

void test_common_prefix_size()
{
    using lc::infra::diff::common_prefix_size;

    const std::vector<int> empty;
    const std::vector<int> values{1, 2, 3};
    const std::vector<int> prefix{1, 2};
    const std::vector<int> different{1, 9, 3};

    expect_size("空序列前缀", common_prefix_size(empty, empty), 0);
    expect_size("完全相等前缀", common_prefix_size(values, values), 3);
    expect_size("一边为前缀", common_prefix_size(values, prefix), 2);
    expect_size("中途不同前缀", common_prefix_size(values, different), 1);

    const auto suffix = common_prefix_size(
        std::span<const int>{values}.subspan(1) | std::views::reverse,
        std::span<const int>{different}.subspan(1) | std::views::reverse);
    expect_size("反向 view 后缀", suffix, 1);

    const std::array left{1, 2};
    const std::array right{1, 2, 3};
    expect_size("公共前后缀相接", common_prefix_size(left, right), 2);
}

std::string alignment_signature(std::span<const int> actual,
                                std::span<const int> expected,
                                const lc::infra::diff::edit_script& script)
{
    std::string result;
    lc::infra::diff::visit_alignment(
        actual, expected, script,
        [&](lc::infra::diff::alignment_operation kind,
            std::size_t,
            std::size_t) {
            using enum lc::infra::diff::alignment_operation;
            switch (kind) {
            case unchanged: result.push_back('U'); break;
            case replaced: result.push_back('X'); break;
            case removed: result.push_back('R'); break;
            case added: result.push_back('A'); break;
            }
        });
    return result;
}

void test_alignment_normalization()
{
    using enum lc::infra::diff::operation;

    const std::vector<int> removal_first_actual{2, 3};
    const std::vector<int> removal_first_expected{3, 3};
    expect_equal(
        "删除-相同-新增规范化",
        alignment_signature(removal_first_actual, removal_first_expected,
                            {removed, unchanged, added}),
        "XU");

    const std::vector<int> addition_first_actual{3, 3};
    const std::vector<int> addition_first_expected{2, 3};
    expect_equal(
        "新增-相同-删除规范化",
        alignment_signature(addition_first_actual, addition_first_expected,
                            {added, unchanged, removed}),
        "XU");

    const std::vector<int> unsafe_actual{1, 2};
    const std::vector<int> unsafe_expected{2, 3};
    expect_equal(
        "不能安全移动边界",
        alignment_signature(unsafe_actual, unsafe_expected,
                            {removed, unchanged, added}),
        "RUA");
}

} // namespace

int main()
{
    using difference_algorithm = lc::infra::diff::myers_trace<>;
    using lc::infra::terminal::format_difference;

    static_assert(lc::infra::diff::algorithm_for<difference_algorithm, int>);
    static_assert(lc::infra::diff::algorithm_for_value<
                  difference_algorithm, std::vector<std::vector<int>>>);
    static_assert(lc::infra::diff::algorithm_for_value<
                  difference_algorithm, std::vector<bool>>);
    static_assert(lc::infra::diff::algorithm_for_value<
                  lc::infra::diff::lcs_matrix<>, std::vector<std::string>>);

    test_myers_against_lcs();
    test_common_prefix_size();
    test_alignment_normalization();

    expect_equal(
        "手动选择算法",
        format_difference<replace_all>(
            std::string{"ab"}, std::string{"ac"}, false),
        "  - 实际：\"[-ab-]\"\n"
        "  + 期望：\"{+ac+}\"\n");

    expect_equal(
        "切换为 LCS 核心",
        format_difference<lc::infra::diff::lcs_matrix<>>(
            std::vector{1, 12, 3}, std::vector{1, 13, 3}, false),
        "  - 实际：[1,[-12-],3]\n"
        "  + 期望：[1,{+13+},3]\n");

    expect_equal(
        "尾部新增",
        format_difference<difference_algorithm>(
            std::vector{2, 3, 4}, std::vector{2, 3, 4, 5}, false),
        "  - 实际：[2,3,4,     ]\n"
        "  + 期望：[2,3,4,{+5+}]\n");

    expect_equal(
        "尾部删除",
        format_difference<difference_algorithm>(
            std::vector{2, 3, 4, 5}, std::vector{2, 3, 4}, false),
        "  - 实际：[2,3,4,[-5-]]\n"
        "  + 期望：[2,3,4,     ]\n");

    expect_equal(
        "重复值替换优先对齐",
        format_difference<difference_algorithm>(
            std::vector{2, 3, 4}, std::vector{3, 3, 4, 5}, false),
        "  - 实际：[[-2-],3,4,     ]\n"
        "  + 期望：[{+3+},3,4,{+5+}]\n");

    expect_equal(
        "对称重复值替换优先对齐",
        format_difference<difference_algorithm>(
            std::vector{3, 3, 4, 5}, std::vector{2, 3, 4}, false),
        "  - 实际：[[-3-],3,4,[-5-]]\n"
        "  + 期望：[{+2+},3,4,     ]\n");

    expect_equal(
        "不安全边界保留原对齐",
        format_difference<difference_algorithm>(
            std::vector{1, 2}, std::vector{2, 3}, false),
        "  - 实际：[[-1-],2,     ]\n"
        "  + 期望：[     ,2,{+3+}]\n");

    expect_equal(
        "标量原子替换",
        format_difference<difference_algorithm>(12, 13, false),
        "  - 实际：[-12-]\n"
        "  + 期望：{+13+}\n");

    expect_equal(
        "元素原子替换",
        format_difference<difference_algorithm>(
            std::vector{1, 12, 3}, std::vector{1, 13, 3}, false),
        "  - 实际：[1,[-12-],3]\n"
        "  + 期望：[1,{+13+},3]\n");

    expect_equal(
        "多处差异",
        format_difference<difference_algorithm>(
            std::vector{1, 2, 3, 4}, std::vector{1, 9, 3, 8}, false),
        "  - 实际：[1,[-2-],3,[-4-]]\n"
        "  + 期望：[1,{+9+},3,{+8+}]\n");

    expect_equal(
        "嵌套数组",
        format_difference<difference_algorithm>(
            std::vector<std::vector<int>>{{1, 2}, {3, 4}},
            std::vector<std::vector<int>>{{1, 9}, {3, 5}}, false),
        "  - 实际：[[1,[-2-]],[3,[-4-]]]\n"
        "  + 期望：[[1,{+9+}],[3,{+5+}]]\n");

    expect_equal(
        "UTF-8 字符串",
        format_difference<difference_algorithm>(
            std::string{"你好"}, std::string{"你坏"}, false),
        "  - 实际：\"你[-好-]\"\n"
        "  + 期望：\"你{+坏+}\"\n");

    expect_equal(
        "容器中的字符串递归",
        format_difference<difference_algorithm>(
            std::vector<std::string>{"你好"},
            std::vector<std::string>{"你坏"}, false),
        "  - 实际：[\"你[-好-]\"]\n"
        "  + 期望：[\"你{+坏+}\"]\n");

    expect_equal(
        "vector<bool>",
        format_difference<difference_algorithm>(
            std::vector<bool>{true, false},
            std::vector<bool>{true, true}, false),
        "  - 实际：[true,[-false-]]\n"
        "  + 期望：[true,{+true+} ]\n");

    expect_equal(
        "数量不等的差异块",
        format_difference<difference_algorithm>(
            std::vector{1, 2, 3}, std::vector{7}, false),
        "  - 实际：[[-1-],[-2-],[-3-]]\n"
        "  + 期望：[{+7+},     ,     ]\n");

    expect_equal(
        "UTF-8 容器列宽",
        format_difference<difference_algorithm>(
            std::vector<std::string>{"a"},
            std::vector<std::string>{"你好"}, false),
        "  - 实际：[\"[-a-]\"   ]\n"
        "  + 期望：[\"{+你好+}\"]\n");

    expect_equal(
        "ANSI 高亮",
        format_difference<difference_algorithm>(
            std::vector{2, 3, 4}, std::vector{2, 3, 5}, true),
        "\x1b[1;91m  - 实际：\x1b[0m[2,3,\x1b[1;91m4\x1b[0m]\n"
        "\x1b[1;92m  + 期望：\x1b[0m[2,3,\x1b[1;92m5\x1b[0m]\n");

    expect_equal(
        "ANSI 缺失单元格与重复值对齐",
        format_difference<difference_algorithm>(
            std::vector{2, 3, 4}, std::vector{3, 3, 4, 5}, true),
        "\x1b[1;91m  - 实际：\x1b[0m[\x1b[1;91m2\x1b[0m,3,4, ]\n"
        "\x1b[1;92m  + 期望：\x1b[0m[\x1b[1;92m3\x1b[0m,3,4,\x1b[1;92m5\x1b[0m]\n");

    expect_size("组合字符宽度",
                lc::infra::terminal::detail::visible_width("\u0301"), 0);
    expect_size("汉字宽度",
                lc::infra::terminal::detail::visible_width("你"), 2);
    expect_size("emoji 宽度",
                lc::infra::terminal::detail::visible_width("😀"), 2);

    using bounded_algorithm = lc::infra::diff::myers_trace<1, 1>;
    const std::vector<int> long_actual(1'001, 1);
    const std::vector<int> long_expected(1'001, 2);
    const auto bounded_script = bounded_algorithm{}(
        std::span<const int>{long_actual},
        std::span<const int>{long_expected});
    const auto bounded_edits = validate_script(
        "有界降级", std::span<const int>{long_actual},
        std::span<const int>{long_expected}, bounded_script);
    if (bounded_edits != long_actual.size() + long_expected.size()) {
        ++failures;
        std::println(stderr, "[FAIL] 有界降级没有产生完整替换脚本");
    }

    if (failures != 0) {
        std::println(stderr, "{} 个测试失败。", failures);
        return 1;
    }
    std::println("所有强类型 diff 测试均已通过。");
    return 0;
}
