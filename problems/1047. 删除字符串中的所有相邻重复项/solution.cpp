#include <lc/prelude.hpp>

class Solution {
public:
    string removeDuplicates(string s) {
        int pos{};
        for (char ch : s) {
            if (pos && s[pos - 1] == ch) pos--;
            else s[pos++] = ch;
        }
        s.resize(pos);
        return s;
    }
};
