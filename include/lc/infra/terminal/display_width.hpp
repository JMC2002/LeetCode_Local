#pragma once

#include <cstddef>
#include <string_view>
#include <utility>

namespace lc::infra::terminal::detail {

inline std::pair<char32_t, std::size_t> decode_utf8(
    std::string_view text, std::size_t offset)
{
    const auto lead = static_cast<unsigned char>(text[offset]);
    if (lead < 0x80) return {lead, 1};

    const auto continuation = [&](std::size_t index) {
        return index < text.size()
            && (static_cast<unsigned char>(text[index]) & 0xc0) == 0x80;
    };

    if (lead >= 0xc2 && lead <= 0xdf && continuation(offset + 1)) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        return {static_cast<char32_t>(((lead & 0x1f) << 6)
                                     | (second & 0x3f)), 2};
    }
    if (lead >= 0xe0 && lead <= 0xef
        && continuation(offset + 1) && continuation(offset + 2)) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        if ((lead != 0xe0 || second >= 0xa0)
            && (lead != 0xed || second <= 0x9f)) {
            return {static_cast<char32_t>(((lead & 0x0f) << 12)
                                         | ((second & 0x3f) << 6)
                                         | (third & 0x3f)), 3};
        }
    }
    if (lead >= 0xf0 && lead <= 0xf4
        && continuation(offset + 1) && continuation(offset + 2)
        && continuation(offset + 3)) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3]);
        if ((lead != 0xf0 || second >= 0x90)
            && (lead != 0xf4 || second <= 0x8f)) {
            return {static_cast<char32_t>(((lead & 0x07) << 18)
                                         | ((second & 0x3f) << 12)
                                         | ((third & 0x3f) << 6)
                                         | (fourth & 0x3f)), 4};
        }
    }
    return {lead, 1};
}

constexpr bool is_combining(char32_t point)
{
    return (point >= 0x0300 && point <= 0x036f)
        || (point >= 0x0483 && point <= 0x0489)
        || (point >= 0x0591 && point <= 0x05bd)
        || point == 0x05bf
        || (point >= 0x05c1 && point <= 0x05c2)
        || (point >= 0x0610 && point <= 0x061a)
        || (point >= 0x064b && point <= 0x065f)
        || point == 0x0670
        || (point >= 0x06d6 && point <= 0x06ed)
        || (point >= 0x0711 && point <= 0x0711)
        || (point >= 0x0730 && point <= 0x074a)
        || (point >= 0x07a6 && point <= 0x07b0)
        || (point >= 0x07eb && point <= 0x07f3)
        || (point >= 0x0816 && point <= 0x082d)
        || (point >= 0x0859 && point <= 0x085b)
        || (point >= 0x08d3 && point <= 0x0903)
        || (point >= 0x093a && point <= 0x094f)
        || (point >= 0x0951 && point <= 0x0957)
        || (point >= 0x0962 && point <= 0x0963)
        || (point >= 0x1ab0 && point <= 0x1aff)
        || (point >= 0x1dc0 && point <= 0x1dff)
        || (point >= 0x20d0 && point <= 0x20ff)
        || (point >= 0xfe00 && point <= 0xfe0f)
        || (point >= 0xfe20 && point <= 0xfe2f)
        || (point >= 0x1f3fb && point <= 0x1f3ff)
        || (point >= 0xe0100 && point <= 0xe01ef)
        || point == 0x200c || point == 0x200d;
}

constexpr bool is_wide(char32_t point)
{
    return point >= 0x1100
        && (point <= 0x115f
            || point == 0x2329 || point == 0x232a
            || (point >= 0x2e80 && point <= 0xa4cf && point != 0x303f)
            || (point >= 0xac00 && point <= 0xd7a3)
            || (point >= 0xf900 && point <= 0xfaff)
            || (point >= 0xfe10 && point <= 0xfe19)
            || (point >= 0xfe30 && point <= 0xfe6f)
            || (point >= 0xff00 && point <= 0xff60)
            || (point >= 0xffe0 && point <= 0xffe6)
            || (point >= 0x1f300 && point <= 0x1faff)
            || (point >= 0x20000 && point <= 0x3fffd));
}

inline std::size_t visible_width(std::string_view text)
{
    std::size_t width = 0;
    for (std::size_t offset = 0; offset < text.size();) {
        const auto [point, bytes] = decode_utf8(text, offset);
        offset += bytes;
        if (point == 0 || point < 0x20
            || (point >= 0x7f && point < 0xa0)
            || is_combining(point)) {
            continue;
        }
        width += is_wide(point) ? 2 : 1;
    }
    return width;
}

} // namespace lc::infra::terminal::detail
