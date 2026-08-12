#include <lc/prelude.hpp>

class Solution {
public:
    string removeDuplicates(string s, int k) {
        vector<pair<char, int>> st{{'\0', 0}};
        for (char c : s) {
            auto& [ch, cnt] = st.back();
            if (ch == c) {
                cnt++;
                if (cnt == k) st.pop_back();
            }
            else st.emplace_back(c, 1);
        }
        string ans;
        for (auto&& [ch, cnt] : st) {
            ans += string(cnt, ch);
        }
        return ans;
    }
};
