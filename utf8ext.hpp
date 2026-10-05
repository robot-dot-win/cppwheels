//  Manipulate utf8 unicode strings.
//
//  Header-only file.
//
//  Copyright (C) 2026, Martin Young <martin_young@live.cn>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

#pragma once

#include <string_view>
#include <concepts>
#include <cstddef>
#include <cstdint>

inline std::u32string utf8_to_utf32(std::string_view u8_str);
inline std::string    utf32_to_utf8(std::u32string_view u32_str);

//------------------------------------------------------------------------
class utf8_sv {
private:
    std::string_view sv_;

    // Evaluates to true if the byte is an ASCII character or a UTF-8 leading byte.
    // Evaluates to false if the byte is a UTF-8 continuation byte (10xxxxxx).
    static constexpr bool is_char_boundary(char c) noexcept {
        return (c & 0xC0) != 0x80;
    }

public:
    static constexpr size_t npos = std::string_view::npos;

    constexpr utf8_sv() noexcept = default;

    template <typename T>
    requires std::convertible_to<T, std::string_view>
    constexpr utf8_sv(const T& str) noexcept : sv_(str) {}

    constexpr operator std::string_view() const noexcept { return sv_; }
    constexpr std::string_view base() const noexcept { return sv_; }
    constexpr bool empty() const noexcept { return sv_.empty(); }

    constexpr size_t length() const noexcept {
        size_t count{};
        for (char c : sv_) if (is_char_boundary(c)) count++;
        return count;
    }

    constexpr std::string_view left(size_t n = npos) const noexcept {
        if (n == 0) return {};
        if (n == npos) return sv_;

        size_t b_idx{};
        size_t chars_seen{};
        const size_t sz{sv_.size()};

        while (b_idx < sz) {
            if (is_char_boundary(sv_[b_idx])) {
                if (chars_seen == n) break;
                chars_seen++;
            }
            b_idx++;
        }
        return sv_.substr(0, b_idx);
    }

    constexpr std::string_view right(size_t n = npos) const noexcept {
        if (n == 0) return {};
        if (n == npos || sv_.empty()) return sv_;

        size_t b_idx{sv_.size()};
        size_t chars_seen{};

        while (b_idx > 0) {
            b_idx--;
            if (is_char_boundary(sv_[b_idx])) {
                chars_seen++;
                if (chars_seen == n) break;
            }
        }
        return sv_.substr(b_idx);
    }

    constexpr std::string_view substr(size_t pos = 0, size_t count = npos) const noexcept {
        if (pos == 0 && count == npos) return sv_;

        size_t b_idx{};
        size_t chars_seen{};
        const size_t sz{sv_.size()};

        while (b_idx < sz) {
            if (is_char_boundary(sv_[b_idx])) {
                if (chars_seen == pos) break;
                chars_seen++;
            }
            b_idx++;
        }

        if (b_idx == sz) return {};
        if (count == npos) return sv_.substr(b_idx);

        size_t end_idx{b_idx};
        chars_seen = 0;

        while (end_idx < sz) {
            if (is_char_boundary(sv_[end_idx])) {
                if (chars_seen == count) break;
                chars_seen++;
            }
            end_idx++;
        }

        return sv_.substr(b_idx, end_idx - b_idx);
    }
};
//------------------------------------------------------------------------
// Converts a UTF-8 encoded std::string_view to a UTF-32 std::u32string
//
std::u32string utf8_to_utf32(std::string_view u8_str)
{
    std::u32string result{};

    const size_t sz{u8_str.size()};
    result.reserve(sz);     // Pre-allocate assuming mostly ASCII for efficiency; prevents excessive reallocation
    for (size_t i{}; i < sz;) {
        unsigned char c {static_cast<unsigned char>(u8_str[i])};
        if (c <= 0x7F) {    // 1-byte sequence (ASCII)// 1-byte sequence (ASCII)
            result.push_back(c);
            i++;
        } else if ((c & 0xE0) == 0xC0) {    // 2-byte sequence
            if (i + 1 >= sz || (u8_str[i+1] & 0xC0) != 0x80) {
                result.push_back(0xFFFD); i++; continue;
            }
            char32_t cp = ((c & 0x1F) << 6) | (u8_str[i+1] & 0x3F);
            result.push_back(cp);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {    // 3-byte sequence
            if (i + 2 >= sz || (u8_str[i+1] & 0xC0) != 0x80 || (u8_str[i+2] & 0xC0) != 0x80) {
                result.push_back(0xFFFD); i++; continue;
            }
            char32_t cp = ((c & 0x0F) << 12) | ((u8_str[i+1] & 0x3F) << 6) | (u8_str[i+2] & 0x3F);
            result.push_back(cp);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {    // // 4-byte sequence
            if (i + 3 >= sz || (u8_str[i+1] & 0xC0) != 0x80 || (u8_str[i+2] & 0xC0) != 0x80 || (u8_str[i+3] & 0xC0) != 0x80) {
                result.push_back(0xFFFD); i++; continue;
            }
            char32_t cp = ((c & 0x07) << 18) | ((u8_str[i+1] & 0x3F) << 12) | ((u8_str[i+2] & 0x3F) << 6) | (u8_str[i+3] & 0x3F);
            result.push_back(cp);
            i += 4;
        } else {    // Invalid leading byte
            result.push_back(0xFFFD); // Insert standard Unicode replacement character
            i++;
        }
    }
    return result;
}
//------------------------------------------------------------------------
// Converts a UTF-32 encoded std::u32string_view back to a UTF-8 std::string
//
std::string utf32_to_utf8(std::u32string_view u32_str)
{
    std::string result{};
    result.reserve(u32_str.size() * 2);  // Heuristic reserve: assume an average of 2 bytes per codepoint to minimize reallocations
    for (char32_t cp : u32_str) {
        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.append("\xEF\xBF\xBD");  // Invalid codepoint fallback (U+FFFD in UTF-8 is EF BF BD)
        }
    }
    return result;
}
//------------------------------------------------------------------------
