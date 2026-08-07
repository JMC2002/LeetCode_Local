#include <lc/prelude.hpp>

class Solution {
    struct Need {
        // 分别是 2*v2、3*v3、6*v5、6*v7，使每个数位的最大势能都是 6。
        uint8_t p2{}, p3{}, p5{}, p7{};

        constexpr void consume(Need supplied) noexcept {
            p2 -= min(p2, supplied.p2);
            p3 -= min(p3, supplied.p3);
            p5 -= min(p5, supplied.p5);
            p7 -= min(p7, supplied.p7);
        }

        constexpr int potential() const noexcept { return p2 + p3 + p5 + p7; }
        constexpr bool empty() const noexcept { return !potential(); }
    };

    inline static constexpr array<Need, 10> suppliedByDigit{ {
        {0, 0, 0, 0},   // 0
        {0, 0, 0, 0},   // 1
        {2, 0, 0, 0},   // 2
        {0, 3, 0, 0},   // 3
        {4, 0, 0, 0},   // 4
        {0, 0, 6, 0},   // 5
        {2, 3, 0, 0},   // 6
        {0, 0, 0, 6},   // 7
        {6, 0, 0, 0},   // 8
        {0, 6, 0, 0},   // 9
    } };

    struct Core {
        array<char, 20> digits{};
        uint8_t size{};
    };

    static constexpr Core makeCore(Need need) noexcept {
        const int a = need.p2 / 2, b = need.p3 / 3;
        array<uint8_t, 10> cnt{};
        cnt[5] = need.p5 / 6;
        cnt[7] = need.p7 / 6;
        cnt[8] = a / 3;
        cnt[9] = b / 2;

        // (a mod 3, b mod 2)
        switch (2 * (a % 3) + b % 2) {
        case 1: ++cnt[3]; break;           // (0, 1) -> "3"
        case 2: ++cnt[2]; break;           // (1, 0) -> "2"
        case 3: ++cnt[6]; break;           // (1, 1) -> "6"
        case 4: ++cnt[4]; break;           // (2, 0) -> "4"
        case 5: ++cnt[2]; ++cnt[6]; break; // (2, 1) -> "26"
        default: break;
        }

        Core core;
        int pos = 0;
        for (int digit = 2; digit <= 9; ++digit) {
            for (int k = cnt[digit]; k > 0; --k) {
                core.digits[pos++] = '0' + digit;
            }
        }
        core.size = pos;
        return core;
    }

    static void writeGlobalMinimum(string& ans, size_t len, const Core& core) {
        ans.clear();
        ans.resize_and_overwrite(len, [len, &core](char* out, size_t) noexcept {
            const size_t padding = len - core.size;
            memset(out, '1', padding);
            memcpy(out + padding, core.digits.data(), core.size);
            return len;
        });
    }

    static void writeSmallestSuffix(char* out, size_t len, Need need) noexcept {
        const Core core = makeCore(need);
        const size_t padding = len - core.size;
        memset(out, '1', padding);
        memcpy(out + padding, core.digits.data(), core.size);
    }

    static void repairZeroSuffix(string& num, size_t from = 0) noexcept {
        if (const size_t zero = num.find('0', from); zero != string::npos) {
            memset(num.data() + zero, '1', num.size() - zero);
        }
    }

    static int nextDigit(Need need, int current, int suffixSlots) noexcept {
        if (current == 9) return -1;
        const int capacity = 6 * suffixSlots;
        if (need.potential() <= capacity) return current + 1;
        if (need.potential() > capacity + 6) return -1;

        for (int digit = current + 1; digit <= 9; ++digit) {
            Need next = need;
            next.consume(suppliedByDigit[digit]);
            if (next.potential() <= capacity) return digit;
        }
        return -1;
    }

    static int stripPrime(uint64_t& value, uint64_t prime) noexcept {
        int exponent = 0;
        while (value % prime == 0) {
            value /= prime;
            ++exponent;
        }
        return exponent;
    }

    static bool factorTarget(uint64_t t, Need& target) noexcept {
        target = {};
        int twos = countr_zero(t);
        target.p2 = 2 * twos;
        t >>= twos;
        target.p3 = 3 * stripPrime(t, 3);
        target.p5 = 6 * stripPrime(t, 5);
        target.p7 = 6 * stripPrime(t, 7);
        return t == 1;
    }

    struct Candidate {
        size_t position{ string::npos };
        char digit{};
        Need needBefore{};
    };

    static void materialize(string& ans, const Candidate& candidate) noexcept {
        Need suffixNeed = candidate.needBefore;
        suffixNeed.consume(suppliedByDigit[candidate.digit - '0']);
        ans[candidate.position] = candidate.digit;
        writeSmallestSuffix(ans.data() + candidate.position + 1,
                            ans.size() - candidate.position - 1, suffixNeed);
    }

public:
    string smallestNumber(string num, long long t) {
        Need target;
        if (!factorTarget(t, target)) return "-1";

        size_t n = num.size();
        if (target.empty()) {
            repairZeroSuffix(num);
            return num;
        }

        Core targetCore = makeCore(target);
        size_t minimumDigits = targetCore.size;
        if (minimumDigits > n) {
            writeGlobalMinimum(num, max(n + 1, minimumDigits), targetCore);
            return num;
        }

        size_t bulkEnd = n - minimumDigits;
        size_t start = 0;
        while (start < bulkEnd && num[start] == '1') ++start;

        if (start < bulkEnd) {
            if (num[start] == '0') {
                writeGlobalMinimum(num, n, targetCore);
                return num;
            }
        }
        else {
            int cmp = char_traits<char>::compare(targetCore.digits.data(), num.data() + bulkEnd, minimumDigits);
            if (cmp == 0) return num;
            if (cmp > 0) {
                writeGlobalMinimum(num, n, targetCore);
                return num;
            }
        }

        Candidate best;
        if (start > 0) best = { start - 1, '2', target };

        Need need = target;
        size_t i = start;

        for (; i < bulkEnd; ++i) {
            char currentChar = num[i];
            int current = currentChar - '0';
            if (currentChar != '9') {
                best = { i, static_cast<char>(currentChar + 1), need };
            }
            if (current == 0) {
                materialize(num, best);
                return num;
            }

            need.consume(suppliedByDigit[current]);
            if (need.empty()) {
                repairZeroSuffix(num, i + 1);
                return num;
            }
        }

        for (; i < n; ++i) {
            int cur = num[i] - '0';
            int digit = nextDigit(need, cur, n - i - 1);
            if (digit != -1) best = { i, static_cast<char>('0' + digit), need };
            if (cur == 0) break;

            need.consume(suppliedByDigit[cur]);
            if (need.empty()) {
                repairZeroSuffix(num, i + 1);
                return num;
            }
        }

        if (best.position != string::npos) {
            materialize(num, best);
            return num;
        }

        writeGlobalMinimum(num, max(n + 1, minimumDigits), targetCore);
        return num;
    }
};
