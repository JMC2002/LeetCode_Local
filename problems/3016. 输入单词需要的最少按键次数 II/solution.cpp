#include  <lc/prelude.hpp>

class Solution {
public:
    int minimumPushes(string word) {
        int cnt[26]{};
        for (char ch : word) cnt[ch - 'a']++;
        ranges::sort(cnt, greater{});
        return ranges::fold_left(cnt, 0, [n = 0](int a, int i)mutable {
            return a + (n++ / 8 + 1) * i;
        });
    }
};