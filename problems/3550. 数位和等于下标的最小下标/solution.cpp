#include <lc/prelude.hpp>

class Solution {
public:
    int smallestIndex(vector<int>& nums) {
        auto f = [](this auto f, int i, int sum = 0)->int { return i ? f(i / 10, sum + i % 10) : sum; };
        for (auto&& [i, x] : nums | views::enumerate)
            if (f(x) == i) return i;
        return -1;
    }
};
