#include <lc/prelude.hpp>

class Solution {
public:
    int maximumLengthSubstring(string s) {
        int cnt[26]{}, ans{};
        for (auto l = s.begin(), r = s.begin(); r != s.end(); r++) {
            if (cnt[*r - 'a']++ == 2) while (--cnt[*l++ - 'a'] != 2);
            ans = max<int>(ans, r - l + 1);
        }
        return ans;
    }
};
