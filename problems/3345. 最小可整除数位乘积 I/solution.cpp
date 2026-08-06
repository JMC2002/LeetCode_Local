#include <lc/prelude.hpp>

class Solution {
public:
    int smallestNumber(int n, int t) {
        int b = n % 10;
        if (!b) return n;
        int k = t / gcd(max(1, n / 10), t);
        return n - b + min(10, (b + k - 1) / k * k);
    }
};