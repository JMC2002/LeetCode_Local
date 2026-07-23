#include <lc/prelude.hpp>
#include <lc/selected_problem.hpp>
#include <lc/runner.hpp>

#include <contracts>
#include <cstdio>

void handle_contract_violation(
    const std::contracts::contract_violation& violation)
{
    const auto location = violation.location();
    std::fprintf(
        stderr, "[CONTRACT] %s，位于 %s:%u:%u\n",
        violation.comment(), location.file_name(),
        static_cast<unsigned>(location.line()),
        static_cast<unsigned>(location.column()));
}

int main()
{
    return lc::run<lc::generated::problem_id>();
}
