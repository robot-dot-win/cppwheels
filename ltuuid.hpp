//  Lightweight UUID generator.
//  Only support UUID v4 conforming to RFC 4122.
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

namespace impl {
    // Random number generation (C++20 uniform initialization)
    inline auto& random_engine() noexcept {
        thread_local std::mt19937_64 eng{std::random_device{}()};
        return eng;
    }
}

namespace ltuuid {

//------------------------------------------------------------------------
struct uuid {
    uint64_t ab;
    uint64_t cd;

    auto operator<=>(const uuid&) const = default;

    std::string str() const;

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

//------------------------------------------------------------------------
// Default zero UUID
inline uuid uuid0() noexcept { return {0, 0}; }

// UUID v4 implementation
inline uuid uuid4() noexcept {
    static thread_local std::uniform_int_distribution<uint64_t> dist;
    return {
        (dist(impl::random_engine()) & 0xFFFFFFFFFFFF0FFULL ) | 0x0000000000004000ULL,
        (dist(impl::random_engine()) & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL
    };
}

// Rebuild UUID from a pair of 64-bit integers
inline uuid rebuild(uint64_t ab, uint64_t cd) noexcept { return {ab, cd}; }

// Rebuild UUID from a std::string
uuid rebuild(std::string_view uustr);

// Rebuild UUID from a std::string without exceptions
inline uuid rebuild_ne(std::string_view uustr) noexcept
    { try{ return rebuild(uustr); } catch(...) { return {0, 0}; } }

} // namespace ltuuid

//------------------------------------------------------------------------
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
