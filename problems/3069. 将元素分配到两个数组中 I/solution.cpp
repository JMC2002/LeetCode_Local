#include <lc/prelude.hpp>

class Solution {
public:
    vector<int> tmp[2];
    vector<int> resultArray(vector<int>& nums, int pre = INT_MAX) {
        if (tmp[0].size() + tmp[1].size() == nums.size()) {
            tmp[0].insert(tmp[0].end(), tmp[1].begin(), tmp[1].end());
            return exchange(tmp[0], tmp[1] = {});
        }
        int i = nums[tmp[0].size() + tmp[1].size()];
        tmp[pre < 0].push_back(i);
        return resultArray(nums, i < abs(pre) ? (pre > 0 ? -i : i) : pre);
    }
};
