#include  <lc/prelude.hpp>
class Solution {
public:
    string smallestPalindrome(string s) {
        int cnt[26]{};
        for (char c : s) cnt[c - 'a']++;
        auto it = s.begin();
        for (char c = 'a'; int n : cnt)
            it = ranges::fill_n(it, n / 2, c++);
        ranges::copy(s.begin(), it, s.rbegin());
        return s;
    }
};