//  Manipulate utf8 unicode charactars in a std::string_view converted
//  string.
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

class utf8_sv {
private:
    std::string_view sv_;

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
        size_t count = 0;
        for (char c : sv_) if ((c & 0xC0) != 0x80) count++;
        return count;
    }

    constexpr std::string_view left(size_t n = npos) const noexcept {
        if (n == 0) return {};
        if (n == npos) return sv_;

        size_t b_idx = 0;
        size_t chars_seen = 0;
        const size_t sz = sv_.size();

        while (b_idx < sz) {
            if ((sv_[b_idx] & 0xC0) != 0x80) {
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

        size_t b_idx = sv_.size();
        size_t chars_seen = 0;

        while (b_idx > 0) {
            b_idx--;
            if ((sv_[b_idx] & 0xC0) != 0x80) {
                chars_seen++;
                if (chars_seen == n) break;
            }
        }
        return sv_.substr(b_idx);
    }

    constexpr std::string_view substr(size_t pos = 0, size_t count = npos) const noexcept {
        if (pos == 0 && count == npos) return sv_;

        size_t b_idx = 0;
        size_t chars_seen = 0;
        const size_t sz = sv_.size();

        while (b_idx < sz) {
            if ((sv_[b_idx] & 0xC0) != 0x80) {
                if (chars_seen == pos) break;
                chars_seen++;
            }
            b_idx++;
        }

        if (b_idx == sz) return {};
        if (count == npos) return sv_.substr(b_idx);

        size_t end_idx = b_idx;
        chars_seen = 0;
        while (end_idx < sz) {
            if ((sv_[end_idx] & 0xC0) != 0x80) {
                if (chars_seen == count) break;
                chars_seen++;
            }
            end_idx++;
        }

        return sv_.substr(b_idx, end_idx - b_idx);
    }
};
