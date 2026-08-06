#pragma once

// 不知道为啥Resharper的__cpp_concepts值很低，导致expected之类的会报错，手动指定一下
#if defined(__RESHARPER__) && __cpp_concepts < 202002L
#  undef __cpp_concepts
#  define __cpp_concepts 202002L
#endif

#if defined(__RESHARPER__) && !defined(__cpp_lib_contracts)
#  define __cpp_lib_contracts
#endif

#include <iostream>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <debugging>
#include <expected>
#include <format>
#include <functional>
#include <limits>
#include <map>
#include <meta>
#include <memory>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

