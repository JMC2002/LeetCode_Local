#include <lc/prelude.hpp>

class Solution {
public:
    int missingInteger(vector<int>& nums) {
        auto n = ranges::adjacent_find(nums, [](int a, int b) { return b != a + 1; }) - nums.begin();
        int s = ranges::fold_left(nums | views::take(n + 1), 0, plus{});
        return *ranges::find_if(views::iota(s), [&](int x) { return !ranges::contains(nums, x); });
    }
};