#include <lc/prelude.hpp>

bool dict[101];
class Solution {
public:
    vector<int> findMissingElements(vector<int>& nums) {
        memset(dict, 0, sizeof dict);
        for (int i : nums) dict[i] = true;
        return views::iota(ranges::min(nums), ranges::max(nums) + 1)
             | views::filter([](auto i) { return !dict[i]; })
             | ranges::to<vector<int>>();
    }
};
