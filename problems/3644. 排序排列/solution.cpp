#include <lc/prelude.hpp>

class Solution {
public:
    int sortPermutation(vector<int>& nums) {
       return *ranges::fold_left_first(nums | views::enumerate
                                             | views::filter([](auto&& p) { return get<0>(p) != get<1>(p); })
                                             | views::values, bit_and{});
    }
};
