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

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <charconv>
#include <cctype>
#include <variant>

#ifdef __cpp_lib_format
#include <format>   // Available when __GNUC__ >= 13
#else
#include <cstdio>   // Fallback: std::snprintf
#endif

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

// --------------------------------------------------------
// urldecode: High-performance URL decoding
// --------------------------------------------------------
inline std::string urldecode(std::string_view src)
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
        } else if (*p == '+') {
            // 'application/x-www-form-urlencoded' standardizes '+' as a space
            result.push_back(' ');
            p++;
        } else result.push_back(*p++);
    }
    return result;
}

// --------------------------------------------------------
// urlencode: Strict RFC 3986 encoding
// --------------------------------------------------------
inline std::string urlencode(std::string_view src)
{
    std::string result;
    // Heuristic pre-allocation: Assume ~20% of the string might need encoding
    result.reserve(src.size() + (src.size() / 5));

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
// HTML转义函数智能返回类型：根据是否转义，自动选择零拷贝 (View) 或独占内存 (String)
//
class HtmlEscapedStr {
private:
    std::variant<std::string_view, std::string> data_;

public:
    explicit HtmlEscapedStr(std::string_view sv) : data_(sv) {}
    explicit HtmlEscapedStr(std::string&& s) : data_(std::move(s)) {}

    operator std::string_view() const noexcept {
        if (auto* sv = std::get_if<std::string_view>(&data_)) return *sv;
        if (auto* s = std::get_if<std::string>(&data_)) return *s;
        return {};
    }

    std::string to_string() const { return std::string(operator std::string_view()); }
};

//------------------------------------------------------------------------
// 高性能 HTML 转义函数
//
// 用法 1：直接输出 (触发隐式转换 operator std::string_view)
//        std::cout << htmlesc(textsv);
// 用法 2：强制获取 std::string
//        std::string final_str = htmlesc(textsv).to_string();
//
inline HtmlEscapedStr htmlesc(std::string_view src, std::string_view special_chars = "&<>\"'")
{
    if (src.empty()) return HtmlEscapedStr(src);

    // 1. 第一阶段：极速扫描 (SIMD 加速)
    // 如果没有找到任何特殊字符，直接返回原 View，真正做到 0 分配 0 拷贝！
    size_t first_pos = src.find_first_of(special_chars);
    if (first_pos == std::string_view::npos) return HtmlEscapedStr(src);

    // 2. 第二阶段：不可避免地发生转义，进入内存分配模式
    std::string result;
    // 经验预分配：假设只有少量字符被转义，减少动态扩容开销
    result.reserve(src.size() + (src.size() / 8));

    // 将扫描到的第一段安全字符直接塞进去
    result.append(src.data(), first_pos);

    size_t pos = first_pos;
    while (pos < src.size()) {
        char c = src[pos];
        switch (c) {
        case '&':  result += "&amp;";  break;
        case '<':  result += "&lt;";   break;
        case '>':  result += "&gt;";   break;
        case '"':  result += "&quot;"; break;
        case '\'': result += "&#39;";  break;
        default:
#ifdef __cpp_lib_format
            result += std::format("&#x{:02X};", static_cast<unsigned char>(c));
#else
            char buf[16];
            int n = std::snprintf(buf, sizeof(buf), "&#x%02X;", static_cast<unsigned char>(c));
            result.append(buf, static_cast<std::size_t>(n));
#endif
        }

        pos++;

        // 查找下一段安全文本
        size_t next_pos = src.find_first_of(special_chars, pos);
        if (next_pos == std::string_view::npos) {
            result.append(src.data() + pos, src.size() - pos);
            break;
        }
        result.append(src.data() + pos, next_pos - pos);
        pos = next_pos;
    }

    return HtmlEscapedStr(std::move(result));
}
//------------------------------------------------------------------------
