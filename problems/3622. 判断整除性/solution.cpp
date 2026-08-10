#include <lc/prelude.hpp>

class Solution {
public:
    bool checkDivisibility(int n) {
        int sum{}, prod = 1;
        for (int m = n; m; m /= 10) {
            sum += m % 10;
            prod *= m % 10;
        }
        return !(n % (sum + prod));
    }
};
