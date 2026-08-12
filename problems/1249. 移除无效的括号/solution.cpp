#include <lc/prelude.hpp>

class Solution {
public:
    string minRemoveToMakeValid(string s) {
        auto g = [](auto&& s, auto&& f) {
            auto rg = s | f;
            auto ch = "()"sv | f;
            auto tail = ranges::remove_if(rg, [&, cnt = 0](char c)mutable {
                return c == ch[1] && (!cnt || !cnt--)
                    || c == ch[0] && !++cnt;
            }).begin();
            return rg | views::take(tail - ranges::begin(rg)) | f;
        };
        auto r = g(g(s, views::all), views::reverse);
        ranges::copy(r, s.begin());
        s.resize(ranges::size(r));
        return s;
    }
};
