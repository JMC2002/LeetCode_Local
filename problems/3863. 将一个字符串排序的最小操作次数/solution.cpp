#include <lc/prelude.hpp>

class Solution {
public:
    int minOperations(string s) {
        if (ranges::is_sorted(s)) return 0;
        if (s.size() == 2) return -1;
        auto [a, b] = ranges::minmax(s);
        if (s[0] != a && s.back() != b) {
            return (s[0] == b && s.back() == a && ranges::count(s, a) == 1 && ranges::count(s, b) == 1) ? 3 : 2;
        }
        return 1;
    }
};
