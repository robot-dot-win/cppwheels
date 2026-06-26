//  Join strings from any standard container with a delimiter, zero-copy.
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
#include <set>
#include <unordered_set>
#include <ranges>
#include <concepts>
#include <type_traits>
#include <utility>

//------------------------------------------------------------------------
// 1. Core Concepts
//------------------------------------------------------------------------

// Constrains C to be a range whose elements are convertible to std::string_view
template <typename C>
concept StringContainer_Join = std::ranges::input_range<C> &&
    std::convertible_to<std::ranges::range_reference_t<C>, std::string_view>;

// Constrains T to be a valid delimiter (char or string-like)
template <typename T>
concept StrJoinDelimiter =
    std::same_as<std::remove_cvref_t<T>, char> ||
    std::convertible_to<T, std::string_view>;

//------------------------------------------------------------------------
// 2. Proxy for C++23 (Native Range Views)
//------------------------------------------------------------------------
template <std::ranges::range V>
struct lazy_str_proxy {
    V view;

    // Scenario A: Zero-copy direct stream output
    friend std::ostream& operator<<(std::ostream& os, const lazy_str_proxy& proxy) {
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
    operator std::string() const {
        auto mutable_view = view;
        std::string s;
        for (char c : mutable_view) s.push_back(c);
        return s;
    }

    // Scenario C: Support operator+ for concatenation
    friend std::string operator+(const lazy_str_proxy& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) + rhs; }
    friend std::string operator+(const std::string& lhs, const lazy_str_proxy& rhs) { return lhs + static_cast<std::string>(rhs); }
    friend std::string operator+(const lazy_str_proxy& lhs, const char* rhs) { return static_cast<std::string>(lhs) + rhs; }
    friend std::string operator+(const char* lhs, const lazy_str_proxy& rhs) { return lhs + static_cast<std::string>(rhs); }
};

template <typename V>
inline auto make_lazy_str_proxy(V&& view) {
    return lazy_str_proxy<std::remove_cvref_t<V>>{std::forward<V>(view)};
}

//------------------------------------------------------------------------
// 3. Proxy for C++20 Fallback (Forward Iteration on Containers)
//------------------------------------------------------------------------
template <typename C, typename D>
struct lazy_container_proxy {
    const C& container;
    D delim;

    friend std::ostream& operator<<(std::ostream& os, const lazy_container_proxy& proxy) {
        char buf[1024];
        size_t i = 0;

        auto append_char = [&](char ch) {
            buf[i++] = ch;
            if (i == sizeof(buf)) { os.write(buf, static_cast<std::streamsize>(i)); i = 0; }
        };
        auto append_sv = [&](std::string_view sv) {
            for (char ch : sv) append_char(ch);
        };

        bool first = true;
        for (const auto& item : proxy.container) {
            if (!first) {
                if constexpr (std::same_as<D, char>) append_char(proxy.delim);
                else append_sv(proxy.delim);
            }
            first = false;
            append_sv(item);
        }
        if (i > 0) os.write(buf, static_cast<std::streamsize>(i));
        return os;
    }

    operator std::string() const {
        std::string s;
        bool first = true;
        for (const auto& item : container) {
            if (!first) {
                if constexpr (std::same_as<D, char>) s.push_back(delim);
                else s.append(delim);
            }
            first = false;
            s.append(item);
        }
        return s;
    }

    friend std::string operator+(const lazy_container_proxy& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) + rhs; }
    friend std::string operator+(const std::string& lhs, const lazy_container_proxy& rhs) { return lhs + static_cast<std::string>(rhs); }
    friend std::string operator+(const lazy_container_proxy& lhs, const char* rhs) { return static_cast<std::string>(lhs) + rhs; }
    friend std::string operator+(const char* lhs, const lazy_container_proxy& rhs) { return lhs + static_cast<std::string>(rhs); }
};

//------------------------------------------------------------------------
// 4. strjoin: Join any string container using a delimiter lazily
//------------------------------------------------------------------------
#ifdef __cpp_lib_ranges_join_with

template <StringContainer_Join C, StrJoinDelimiter T = std::string_view>
inline auto strjoin(const C& container, const T& delimiter = " ") {
    if constexpr (std::same_as<std::remove_cvref_t<T>, char>) {
        return make_lazy_str_proxy(container | std::views::join_with(delimiter));
    } else {
        return make_lazy_str_proxy(container | std::views::join_with(std::string_view{delimiter}));
    }
}

#else

// C++20 Fallback seamlessly handles unordered_set, multiset, etc.
template <StringContainer_Join C, StrJoinDelimiter T = std::string_view>
inline auto strjoin(const C& container, const T& delimiter = " ") {
    if constexpr (std::same_as<std::remove_cvref_t<T>, char>) {
        return lazy_container_proxy<C, char>{container, delimiter};
    } else {
        return lazy_container_proxy<C, std::string_view>{container, std::string_view{delimiter}};
    }
}

#endif
