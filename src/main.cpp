#include <algorithm>
#include <iostream>
#include <meta>
#include <string>
#include <type_traits>

class Solution {    
public:
    std::string processStr(std::string s) {
        std::string out;
        for (char c : s) {
            if ('a' <= c && c <= 'z') {
                out += c;
            } else if (c == '*') {
                if (!out.empty()) {
                    out.pop_back();
                }
            } else if (c == '#') {
                out += out;
            } else if (c == '%') {
                std::ranges::reverse(out);
            }
        }
        return out;
    }
};

template <std::size_t I, class... Ts>
using nth_t = Ts...[I];

static_assert(std::is_same_v<nth_t<1, int, double, char>, double>);

constexpr std::meta::info solution_meta = ^^Solution;
static_assert(std::meta::is_type(solution_meta));

int main() {
    std::cout << "__cplusplus=" << __cplusplus << '\n';

#ifdef __cpp_impl_reflection
    std::cout << "__cpp_impl_reflection=" << __cpp_impl_reflection << '\n';
#endif

#ifdef __cpp_lib_reflection
    std::cout << "__cpp_lib_reflection=" << __cpp_lib_reflection << '\n';
#endif

#ifdef __cpp_pack_indexing
    std::cout << "__cpp_pack_indexing=" << __cpp_pack_indexing << '\n';
#endif

    std::cout << "Solution::processStr=" << Solution{}.processStr("a#b*%") << '\n';
}
