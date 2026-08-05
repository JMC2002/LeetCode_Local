#pragma once

#include <cstddef>
#include <experimental/scope>
#include <meta>
#include <utility>

#include <lc/infra/cases/parser.hpp>
#include <lc/infra/cases/serializer.hpp>
#include <lc/infra/diff/myers.hpp>
#include <lc/infra/meta/concepts.hpp>
#include <lc/infra/meta/reflection.hpp>
#include <lc/infra/runtime/debugger.hpp>
#include <lc/infra/runtime/timestamp.hpp>
#include <lc/infra/terminal/difference.hpp>
#include <lc/infra/terminal/style.hpp>
#include <lc/problem.hpp>

namespace lc {

// 替换此类型即可在编译期选择另一种 diff 核心算法。
using difference_algorithm = infra::diff::myers_trace<>;

template<std::size_t Id>
    requires infra::meta::runnable_problem<problem<Id>>
int run()
{
    using selected_problem = problem<Id>;
    using solution_type = typename selected_problem::solution_type;
    using arguments_type = infra::meta::arguments_t<solution_type>;
    using expected_type = infra::meta::output_t<solution_type>;
    using infra::terminal::println;
    using enum infra::terminal::style;

    auto pause_on_exit = std::experimental::scope_exit([] {
        infra::runtime::keep_console_open_for_debugger();
    });

    const auto report_error = [](const infra::cases::case_error& error) {
        println(failure, "[ERROR] {}", infra::cases::describe(error));
        return 2;
    };

    constexpr auto entry = infra::meta::entry_v<solution_type>;
    constexpr auto validation =
        infra::cases::validate<arguments_type, expected_type>(selected_problem::cases);
    static_assert(validation, infra::cases::diagnostic(validation));
    constexpr auto test_count = validation->test_count;

    infra::cases::parser parser{selected_problem::cases};
    std::size_t checked = 0, passed = 0;

    println(muted, "[{}]", infra::runtime::current_local_timestamp());
    println(heading, "LC {} :: Solution::{}", Id, std::meta::identifier_of(entry));

    for (std::size_t test_case = 1; test_case <= test_count; ++test_case) {
        auto parsed_case =
            parser.template parse_case<arguments_type, expected_type>(test_case);
        if (!parsed_case) {
            return report_error(parsed_case.error());
        }
        auto [arguments, expected, arguments_source] =
            std::move(*parsed_case);

        const auto actual = infra::meta::evaluate<solution_type>(arguments);

        const auto actual_text = infra::cases::serialize(actual);
        if (!expected) {
            println(running, "[RUN ] 用例 {:>2}：{}", test_case, actual_text);
            continue;
        }

        ++checked;
        if (actual == *expected) {
            ++passed;
            println(success, "[PASS] 用例 {:>2}：{}", test_case, actual_text);
        } else {
            println(failure, "[FAIL] 用例 {:>2}", test_case);
            println(muted, "  输入：{}", arguments_source);
            infra::terminal::print_difference<difference_algorithm>(
                actual, *expected);
        }
    }

    if (checked == 0) {
        println(warning, "已运行 {} 个用例；未提供任何期望值。", test_count);
        return 0;
    }

    println(passed == checked ? success : failure, "已检查的用例通过 {}/{}；共运行 {} 个用例。", passed, checked, test_count);
    return passed != checked;
}

} // namespace lc
