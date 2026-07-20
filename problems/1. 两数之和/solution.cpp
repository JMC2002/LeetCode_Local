#include "../../include/lc/header.h"

class Solution {
    unordered_map<int, int> dict;
public:
    vector<int> twoSum(vector<int>& nums, int target, int pos = 0) {
        return pos == nums.size() ? vector{ 0,0 } : (dict.count(nums[pos]) ? vector{ pos, dict[nums[pos]] } : (dict[target - nums[pos]] = pos, twoSum(nums, target, pos + 1)));
    }
};
