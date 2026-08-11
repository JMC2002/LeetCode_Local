#include <lc/prelude.hpp>

class Solution {
public:
    int largestInteger(vector<int>& nums, int k) {
        if (nums.size() == k) return ranges::max(nums);
        int cnt[51]{};
        for (int i : nums) cnt[i]++;
        auto f = [&](auto r) { return ranges::max(r | views::filter([&](int i) { return i < 0 || cnt[i] == 1; })); };
        return k == 1 ? f(views::iota(-1, 51)) : f(array{ -1, nums.front(), nums.back() });
    }
};
