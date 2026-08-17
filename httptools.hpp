//  Self-made tools for http related programing.
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

#include <string>
#include <string_view>

inline std::string urlencode(std::string_view src);
inline std::string urldecode(std::string_view src, bool plus_as_space = true);

#include "strext.hpp"
#include "httpurl.hpp"
#include "htmlescape.hpp"

//------------------------------------------------------------------------
// Alternatives to urldecode and urlencode:
//
//    std::string raw_url = "https://example.com/search?q=C++ FastCGI";
//
// https://github.com/yhirose/cpp-httplib
// =========================================
//    // 1. URL 编码
//    std::string encoded = httplib::detail::encode_url(raw_url);
//    // 输出类似: https%3A%2F%2Fexample.com%2Fsearch%3Fq%3DC%2B%2B%20FastCGI
//
//    // 2. URL 解码
//    // 注意第二个参数：处理标准 URL 编码时通常设为 true
//    std::string decoded = httplib::detail::decode_url(encoded, true);
//
// libcurl:
// =========================================
//    // 1. URL 编码
//    std::string encoded;
//    char* output = curl_easy_escape(nullptr, raw_url.c_str(), 0);
//    if (output) {
//        encoded = output;
//        curl_free(output);
//    }
//
//    // 2. URL 解码
//    std::string decoded;
//
//    // 关键陷阱处理：libcurl 严格遵守 RFC 3986，它不会主动把 '+' 变为空格。
//    // 但前端的 application/x-www-form-urlencoded 表单确实会将空格编码为 '+'。
//    // 所以在使用 curl 解码前，必须手动把 '+' 换成空格。
//    std::replace(encoded.begin(),encoded.end(),'+',' ');
//
//    char* output = curl_easy_unescape(nullptr, encoded.c_str(), 0, nullptr);
//    if (output) {
//        decoded = output;
//        curl_free(output);
//    }
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// urlencode: Strict RFC 3986 encoding
//
std::string urlencode(std::string_view src)
{
    std::string result;
    // Heuristic pre-allocation: Assume ~20% of the string might need encoding
    // result.reserve(src.size() + (src.size() / 5));
    result.reserve(src.size() * 3);  // sufficient for UTF8 encoded unicode

    // Extremely fast lookup table for hex conversions
    constexpr char hex_chars[] = "0123456789ABCDEF";

    for (char c : src) {
        auto uc = static_cast<unsigned char>(c);

        // Unreserved characters per RFC 3986 don't need encoding
        if (std::isalnum(uc) || uc == '-' || uc == '_' || uc == '.' || uc == '~') {
            result.push_back(c);
        } else {
            // Encode all other characters as %HH
            result.push_back('%');
            result.push_back(hex_chars[(uc >> 4) & 0xF]);
            result.push_back(hex_chars[uc & 0xF]);
        }
    }
    return result;
}

//------------------------------------------------------------------------
// urldecode: High-performance URL decoding
//
// plus_as_space  true:  for application/x-www-form-urlencoded
//                false: for path segment
//
std::string urldecode(std::string_view src, bool plus_as_space)
{
    std::string result;
    // Pre-allocate memory. The decoded string will never be longer than the source.
    result.reserve(src.size());

    const char* p = src.data();
    const char* end = p + src.size();

    while (p < end) {
        if (*p == '%') {
            if (p + 2 < end) {
                unsigned int value{};
                auto [ptr, ec] = std::from_chars(p + 1, p + 3, value, 16);
                if (ec == std::errc{}) {
                    result.push_back(static_cast<char>(value));
                    p += 3;
                    continue;
                }
            }
            // If the % sequence is malformed, treat it as a literal '%' (Resilience)
            result.push_back(*p++);
        } else if (plus_as_space && *p == '+') {
            result.push_back(' ');
            p++;
        } else result.push_back(*p++);
    }
    return result;
}

//------------------------------------------------------------------------
