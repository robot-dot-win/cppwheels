//  HtmlEscaper — On-demand HTML entity escaping with named  variable
//  storage and zero-copy fast path.
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

// ============================================================
//  Design:
//    · 99.9999% of inputs are clean. The fast path performs a
//      single find_first_of scan → npos → stores the original
//      string_view directly. Zero allocation, zero copy.
//    · The rare dirty input goes through the slow path: escape
//      into a std::string held in storage_, then store a
//      string_view pointing into it.
//    · Transparent hashing allows all lookups to accept
//      string_view / const char* without constructing a
//      temporary std::string.
// ============================================================

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#ifdef __glibcxx_format
#include <format>   // Available when __GNUC__ >= 13
#else
#include <cstdio>   // Fallback: std::snprintf
#endif

class HtmlEscaper {
public:
    explicit HtmlEscaper(std::string_view special_chars = "&<>\"'")
        : special_chars_(special_chars) {}

    void clear() noexcept {
        vars_.clear();
        storage_.clear();
    }

    // --------------------------------------------------------
    //  add: Register a data item under the given name.
    //    · If the name already exists, the old entry (and its
    //      storage, if any) is removed first.
    //    · Fast path: no special character found → store the
    //      original string_view with zero allocation.
    //    · Slow path: escape into storage_, store a view into
    //      the owned string.
    //    · Returns: a safe string_view for immediate output.
    // --------------------------------------------------------
    [[nodiscard]]
    std::string_view add(std::string_view name, std::string_view data) {
        remove(name);

        // ---- Fast path: single SIMD-accelerated scan ----
        if (data.find_first_of(special_chars_) == std::string_view::npos) {
            auto [it, inserted] = vars_.emplace(std::string(name), data);
            return it->second;
        }

        // ---- Slow path: escape and own the memory ----
        std::string key(name);
        std::string escaped = do_escape(data);

        auto [sit, s_ins] = storage_.emplace(key, std::move(escaped));
        std::string_view  sv(sit->second);

        vars_.emplace(std::move(key), sv);
        return sv;
    }

    void remove(std::string_view name) {
        if (auto it = vars_.find(name); it != vars_.end()) vars_.erase(it);
        if (auto it = storage_.find(name); it != storage_.end()) storage_.erase(it);
    }

    // --------------------------------------------------------
    //  get: Retrieve the safe string_view by name.
    //  Returns an empty string_view if the name is not found.
    // --------------------------------------------------------
    [[nodiscard]]
    std::string_view get(std::string_view name) const noexcept {
        if (auto it = vars_.find(name); it != vars_.end()) return it->second;
        return {};
    }

    [[nodiscard]]
    bool contains(std::string_view name) const noexcept {
        return vars_.contains(name);
    }

private:
    // ========================================================
    //  Transparent hash: enables unordered_map lookups with
    //  string_view / const char* keys without constructing a
    //  temporary std::string.
    // ========================================================
    struct TransparentHash {
        using is_transparent = void;

        std::size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
        std::size_t operator()(const std::string& s) const noexcept {
            return std::hash<std::string_view>{}(std::string_view{s});
        }
        std::size_t operator()(const char* s) const noexcept {
            return std::hash<std::string_view>{}(std::string_view{s});
        }
    };

    // ========================================================
    //  do_escape: Slow path only. Iterates through the input
    //  with find_first_of, appending clean segments and entity
    //  replacements in a single pass.
    // ========================================================
    [[nodiscard]]
    std::string do_escape(std::string_view data) const {
        std::string result;
        result.reserve(data.size() + data.size() / 8);  // Heuristic pre-alloc.

        std::string_view::size_type pos = 0;
        while (pos < data.size()) {
            auto found = data.find_first_of(special_chars_, pos);
            if (found == std::string_view::npos) {
                result.append(data.data() + pos, data.size() - pos);
                break;
            }
            result.append(data.data() + pos, found - pos);
            append_entity(result, data[found]);
            pos = found + 1;
        }
        return result;
    }

    // ========================================================
    //  append_entity: Map a single character to its HTML
    //  entity. The five well-known characters use named
    //  entities; any custom character uses a numeric hex
    //  entity (&#xHH;).
    // ========================================================
    static void append_entity(std::string& out, char c) {
        switch (c) {
            case '&':  out += "&amp;";  return;
            case '<':  out += "&lt;";   return;
            case '>':  out += "&gt;";   return;
            case '"':  out += "&quot;"; return;
            case '\'': out += "&#39;";  return;
            default: {
#ifdef __glibcxx_format
                out += std::format("&#x{:02X};", static_cast<unsigned char>(c));
#else
                char buf[16];
                int n = std::snprintf(buf, sizeof(buf), "&#x%02X;", static_cast<unsigned char>(c));
                out.append(buf, static_cast<std::size_t>(n));
#endif
            }
        }
    }

    // ========================================================
    //  Data members
    // ========================================================

    // The set of characters that require escaping.
    std::string special_chars_;

    // Variable container: name → safe string_view.
    //   Fast path: view points into the caller's original data.
    //   Slow path: view points into storage_ below.
    std::unordered_map<std::string, std::string_view, TransparentHash, std::equal_to<>> vars_;

    // Storage container: owns the heap memory for escaped
    // strings only. erase() destroys the string immediately.
    std::unordered_map<std::string, std::string, TransparentHash, std::equal_to<>> storage_;
};
