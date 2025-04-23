//  Light UUID generator.
//  Solely on generating and manipulating UUID v4 conforming to RFC 4122.
//
//  Copyright (C) 2025, Martin Young <martin_young@live.cn>
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

#include <array>
#include <cstdint>
#include <string_view>
#include <random>
#include <thread>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <charconv>

#ifdef __glibcxx_format
#include <format>  // Available when __GNUC__ >= 13
#else
#include <sstream> // For std::stringstream
#include <iomanip> // For std::setw, std::setfill
#endif

namespace ltuuid {

struct uuid {
    uint64_t ab;
    uint64_t cd;

    auto operator<=>(const uuid&) const = default;

    std::string str() const;
    std::string pretty() const;

#ifdef __glibcxx_format
    template<typename FormatContext>
    auto format(FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", str());
    }
#else
    friend std::ostream& operator<<(std::ostream& os, const uuid& u) {
        return os << u.str();
    }
#endif
};

uuid uuid0() noexcept;
uuid uuid4() noexcept;

uuid rebuild(uint64_t ab, uint64_t cd) noexcept;
uuid rebuild(std::string_view uustr);
uuid rebuild_ne(std::string_view uustr) noexcept;

namespace impl {
    // Random number generation (C++20 uniform initialization)
    inline auto& random_engine() noexcept {
        thread_local std::mt19937_64 eng{std::random_device{}()};
        return eng;
    }
}

// UUID v4 implementation
uuid uuid4() noexcept {
    static thread_local std::uniform_int_distribution<uint64_t> dist;
    return {
        dist(impl::random_engine()) & 0xFFFFFFFFFFFF0FFULL | 0x0000000000004000ULL,
        dist(impl::random_engine()) & 0x3FFFFFFFFFFFFFFFULL | 0x8000000000000000ULL
    };
}

// String conversion implementation
std::string uuid::str() const
{
    constexpr auto hex = "0123456789abcdef";
    std::array<char, 36> buf{};

    auto fill = [&](auto val, size_t start, size_t len) {
        for (size_t i = 0; i < len; ++i)
            buf[start + i] = hex[(val >> (4 * (15 - i))) & 0xF];
    };

    fill(ab, 0, 8);        buf[8]   = '-';
    fill(ab >> 16, 9, 4);  buf[13] = '-';
    fill(ab >> 32, 14, 4); buf[18] = '-';
    fill(cd, 19, 4);       buf[23] = '-';
    fill(cd >> 16, 24, 12);

    return std::string(buf.data(), buf.size());
}

// Default zero UUID
uuid uuid0() noexcept { return {0, 0}; }

// Rebuild UUID from a pair of 64-bit integers
uuid rebuild(uint64_t ab, uint64_t cd) noexcept { return {ab, cd}; }

// Rebuild UUID from a std::string
uuid rebuild(std::string_view uustr)
{
    if (uustr.size() != 36)
        throw std::runtime_error("Invalid UUID std::string length");

    if (uustr[8] != '-' || uustr[13] != '-' || uustr[18] != '-' || uustr[23] != '-')
        throw std::runtime_error("Invalid UUID std::string format");

    uint64_t ab_val = 0;
    auto res = std::from_chars(uustr.data(), uustr.data() + 8, ab_val, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 1)");

    res = std::from_chars(uustr.data() + 9, uustr.data() + 13, ab_val, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 2)");

    res = std::from_chars(uustr.data() + 14, uustr.data() + 18, ab_val, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 3)");

    uint64_t cd_val = 0;
    res = std::from_chars(uustr.data() + 19, uustr.data() + 23, cd_val, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 4)");

    res = std::from_chars(uustr.data() + 24, uustr.data() + 36, cd_val, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 5)");

    // Need to combine the parsed parts correctly into ab_val and cd_val
    uint64_t ab_part1, ab_part2, ab_part3, cd_part1, cd_part2;

    res = std::from_chars(uustr.data(), uustr.data() + 8, ab_part1, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 1)");
    res = std::from_chars(uustr.data() + 9, uustr.data() + 13, ab_part2, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 2)");
    res = std::from_chars(uustr.data() + 14, uustr.data() + 18, ab_part3, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 3)");
    res = std::from_chars(uustr.data() + 19, uustr.data() + 23, cd_part1, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 4)");
    res = std::from_chars(uustr.data() + 24, uustr.data() + 36, cd_part2, 16);
    if (res.ec != std::errc{}) throw std::runtime_error("Invalid hexadecimal characters in UUID std::string (part 5)");

    ab_val = (ab_part1 << 32) | (ab_part2 << 16) | ab_part3; // Correct the bit shifts
    cd_val = (cd_part1 << 48) | cd_part2;                    // Correct the bit shifts

    return {ab_val, cd_val};
}

// Rebuild UUID from a std::string without exceptions
uuid rebuild_ne(std::string_view uustr) noexcept
{
    try{ return rebuild(uustr); } catch(...) { return {0, 0}; }
}

// The std::string format of UUID v4 is already considered "pretty"
std::string uuid::pretty() const { return str(); }

} // namespace ltuuid

// Formatter specialization for ltuuid::uuid in the std namespace
#ifdef __glibcxx_format
template <>
struct std::formatter<ltuuid::uuid> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const ltuuid::uuid& u, FormatContext& ctx) const {
        return std::formatter<std::string>::format(u.str(), ctx);
    }
};
#endif
