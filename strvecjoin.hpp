//  Join all lines in a string vector with a delimiter, zero-copy.
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

#include <version>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <concepts>
#include <type_traits>
#include <utility>

//------------------------------------------------------------------------
// Lazy evaluation proxy for range views.
// Bridges ranges with std::ostream and std::string seamlessly without
// unnecessary memory allocations.
//------------------------------------------------------------------------
template <std::ranges::range V>
struct lazy_str_proxy {
    V view;

    // Scenario A: Zero-copy direct stream output (e.g., std::cerr << proxy)
    // Uses a 1KB stack buffer to prevent system call overhead (write storms)
    // associated with unbuffered streams like std::cerr.
    friend std::ostream& operator<<(std::ostream& os, const lazy_str_proxy& proxy) {
        // By-value copy bypasses const-iterator limitations in C++20 views.
        // Copying a view is guaranteed to be an O(1) operation.
        auto mutable_view = proxy.view;

        char buf[1024];
        size_t i = 0;
        for (char c : mutable_view) {
            buf[i++] = c;
            if (i == sizeof(buf)) {
                os.write(buf, static_cast<std::streamsize>(i));
                i = 0;
            }
        }
        if (i > 0) os.write(buf, static_cast<std::streamsize>(i));
        return os;
    }

    // Scenario B: Implicit conversion to std::string
    // Entity generation happens only upon explicit demand.
    operator std::string() const {
        auto mutable_view = view;
        std::string s;
        for (char c : mutable_view) s.push_back(c);
        return s;
    }

    // Scenario C: Support operator+ for concatenation
    // Explicit friend overloads prevent template argument deduction failures.
    friend std::string operator+(const lazy_str_proxy& lhs, const std::string& rhs) {
        return static_cast<std::string>(lhs) + rhs;
    }

    friend std::string operator+(const std::string& lhs, const lazy_str_proxy& rhs) {
        return lhs + static_cast<std::string>(rhs);
    }

    friend std::string operator+(const lazy_str_proxy& lhs, const char* rhs) {
        return static_cast<std::string>(lhs) + rhs;
    }

    friend std::string operator+(const char* lhs, const lazy_str_proxy& rhs) {
        return lhs + static_cast<std::string>(rhs);
    }
};

// Helper function to deduce template types automatically
template <typename V>
inline auto make_lazy_str_proxy(V&& view) {
    return lazy_str_proxy<std::remove_cvref_t<V>>{std::forward<V>(view)};
}

//------------------------------------------------------------------------
// strvecjoin: Join vector of strings using a delimiter lazily
//------------------------------------------------------------------------
#ifdef __cpp_lib_ranges_join_with

template <typename T>
concept strvecjoin_Deli =
    std::same_as<std::remove_cvref_t<T>, char> ||
    std::convertible_to<T, std::string_view>;

template <strvecjoin_Deli T>
inline auto strvecjoin(const std::vector<std::string>& lines, const T& delimiter=" ") {
    if constexpr (std::same_as<std::remove_cvref_t<T>, char>) {
        return make_lazy_str_proxy(lines | std::views::join_with(delimiter));
    } else {
        return make_lazy_str_proxy(lines | std::views::join_with(std::string_view{delimiter}));
    }
}

#else

// C++20 fallback using compositional ranges
inline auto strvecjoin(const std::vector<std::string>& lines, std::string_view delim=" ") {
    const size_t n = lines.size();
    auto view = std::views::iota(size_t{0}, n > 0 ? 2 * n - 1 : size_t{0})
         | std::views::transform([&lines, delim](size_t i) -> std::string_view {
             return (i % 2 == 0) ? std::string_view(lines[i / 2]) : delim;
         })
         | std::views::join;
    return make_lazy_str_proxy(std::move(view));
}

#endif
