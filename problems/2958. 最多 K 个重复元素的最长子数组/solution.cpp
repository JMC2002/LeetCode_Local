#include <lc/prelude.hpp>

class Solution {
public:
    int maxSubarrayLength(vector<int>& nums, int k) {
        unordered_map<int, int> cnt;
        int ans{};
        for (auto l = nums.begin(), r = nums.begin(); r != nums.end(); r++) {
            if (cnt[*r]++ == k) while (--cnt[*l++] != k);
            ans = max<int>(ans, r - l + 1);
        }
        return ans;
    }
};
