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

#include <concepts>
#include <type_traits>
#include <string_view>
#include <cstddef>
#include <cstdint>
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    #define UTF_CONV_WINDOWS 1
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
#else
    #define UTF_CONV_WINDOWS 0
#endif

inline std::u16string utf8_to_16(std::string_view u8_str);
inline std::u32string utf8_to_32(std::string_view u8_str);
inline std::string    utf16_to_8(std::u16string_view u16_str);
inline std::string    utf32_to_8(std::u32string_view u32_str);

#if UTF_CONV_WINDOWS
//------------------------------------------------------------------------
// 将UTF-16字符串安全转换为Win32 API所需的 LPCWSTR；零拷贝、零开销
inline LPCWSTR as_lpcwstr(const std::u16string& str) noexcept
{
    return reinterpret_cast<LPCWSTR>(str.c_str());  // 保证零终止
}
//------------------------------------------------------------------------
// 将 Win32 宽字符串转为 u16string_view，零拷贝，用于后续转 UTF-8
inline std::u16string_view as_u16sv(const wchar_t* ptr, std::size_t len) noexcept
{
    return { reinterpret_cast<const char16_t*>(ptr), len };
}
inline std::u16string_view as_u16sv(LPCWSTR zstr) noexcept
{
    return { reinterpret_cast<const char16_t*>(zstr), std::wcslen(zstr) };
}
#endif

//------------------------------------------------------------------------
// UTF8的UNICODE字符视图
class utf8_sv {
private:
    std::string_view sv_;
    std::size_t length_;

    // 返回从 pos 开始的一个“字符单元”的下一个字节位置。
    // 合法 UTF-8 序列整体作为一个单元；非法时当前字节单独作为一个单元。
    static constexpr std::size_t next_unit(std::string_view sv, std::size_t pos) noexcept
    {
        const std::size_t sz = sv.size();
        const std::size_t remaining = sz - pos;
        const unsigned char c0 = static_cast<unsigned char>(sv[pos]);

        // ASCII
        if (c0 <= 0x7F)  return pos + 1;

        // 2-byte
        if ((c0 & 0xE0) == 0xC0)
            return pos + ((remaining >= 2 && is_continuation(sv[pos + 1]) && c0 >= 0xC2)? 2 : 1);

        // 3-byte
        if ((c0 & 0xF0) == 0xE0) {
            if (remaining >= 3 && is_continuation(sv[pos + 1]) && is_continuation(sv[pos + 2])) {
                const unsigned char c1 = static_cast<unsigned char>(sv[pos + 1]);
                // overlong 或 surrogate
                if (!(c0 == 0xE0 && c1 < 0xA0) && !(c0 == 0xED && c1 >= 0xA0)) return pos + 3;
            }
            return pos + 1;
        }

        // 4-byte
        if ((c0 & 0xF8) == 0xF0) {
            if (remaining >= 4 && is_continuation(sv[pos + 1]) && is_continuation(sv[pos + 2]) && is_continuation(sv[pos + 3])) {
                const unsigned char c1 = static_cast<unsigned char>(sv[pos + 1]);
                // overlong 或 > U+10FFFF
                if (!(c0 == 0xF0 && c1 < 0x90) && !(c0 > 0xF4) && !(c0 == 0xF4 && c1 > 0x8F)) return pos + 4;
            }
            return pos + 1;
        }

        // 非法首字节，或孤立的 continuation byte
        return pos + 1;
    }

    // 计算length
    static constexpr std::size_t count_units(std::string_view sv) noexcept
    {
        std::size_t count {};
        std::size_t pos {};
        const std::size_t sz {sv.size()};
        while (pos < sz) {
            pos = next_unit(sv, pos);
            ++count;
        }
        return count;
    }

    // 从 pos 前进 n 个字符单元，返回字节位置。
    // 若不足 n 个单元，返回 sv_.size()。
    constexpr std::size_t advance(std::size_t pos, std::size_t n) const noexcept
    {
        const std::size_t sz {sv_.size()};
        while (pos < sz && n != 0) {
            pos = next_unit(sv_, pos);
            --n;
        }
        return pos;
    }

public:
    static constexpr std::size_t npos = std::string_view::npos;
    static constexpr bool is_continuation(char c) noexcept {
        return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
    }

    constexpr utf8_sv() noexcept : sv_(), length_(0) {}

    // 阻止临时 std::string 悬空
    utf8_sv(std::string&&) = delete;

    template <typename T>
    requires std::convertible_to<T, std::string_view>
    constexpr utf8_sv(const T& str) noexcept(std::is_nothrow_constructible_v<std::string_view, const T&>)
        : sv_(str), length_(count_units(sv_)) {}

    constexpr operator std::string_view() const noexcept { return sv_; }
    constexpr std::string_view base() const noexcept { return sv_; }

    [[nodiscard]] constexpr bool empty() const noexcept { return sv_.empty(); }
    [[nodiscard]] constexpr std::size_t length() const noexcept { return length_; }

    [[nodiscard]] constexpr std::string_view left(std::size_t n = npos) const noexcept
    {
        if (n == 0) return {};
        if (n == npos || n >= length_) return sv_;
        return sv_.substr(0, advance(0, n));
    }

    [[nodiscard]] constexpr std::string_view right(std::size_t n = npos) const noexcept
    {
        if (n == 0) return {};
        if (n == npos || n >= length_) return sv_;
        return sv_.substr(advance(0, length_ - n));
    }

    // pos/count 按字符单元；pos 越界返回空；count 超出截到末尾。
    [[nodiscard]] constexpr std::string_view substr(std::size_t pos = 0, std::size_t count = npos) const noexcept
    {
        if (pos == 0 && count == npos) return sv_;
        if (count == 0 || pos >= length_) return {};

        const std::size_t start = advance(0, pos);
        if (start == sv_.size()) return {};

        if (count == npos || count >= length_ - pos)
            return sv_.substr(start);

        const std::size_t end = advance(start, count);
        return sv_.substr(start, end - start);
    }
};

//------------------------------------------------------------------------
// UTF-8 转 UTF-32
// 错误策略：每个无效字节输出一个U+FFFD，逐字节前进；恢复粒度为单字节，
//           可能产生多个U+FFFD
// 不自动剥离 BOM，U+FEFF 作为普通码点保留
// 性能敏感场景：若为高频调用，可考虑查表法加速续字节判断，或使用 SIMD 优化；
//               普通业务场景当前实现已足够。
std::u32string utf8_to_32(std::string_view u8)
{
    constexpr char32_t REPLACEMENT = U'\uFFFD';

    std::u32string result;
    result.reserve(u8.size());

    // 空 string_view 的 data() 可能为 nullptr；当 u8.empty() 且 data()==nullptr 时，nullptr + 0=nullprt
    // C++ 标准 [expr.add]/4 明确写到：
    //      If P evaluates to a null pointer value and J evaluates to 0, the result is a null pointer value.
    // 也就是说，nullptr+0行为可期，因此不必做u8.empty()判断
    const unsigned char* p   = reinterpret_cast<const unsigned char*>(u8.data());
    const unsigned char* end = p + u8.size();

    while (p < end) {
        const unsigned char c0 = *p;
        const std::size_t remaining = static_cast<std::size_t>(end - p);

        // 1-byte (ASCII)
        if (c0 <= 0x7F) {
            result.push_back(static_cast<char32_t>(c0));
            ++p;
            continue;
        }

        // 2-byte
        if ((c0 & 0xE0) == 0xC0) {
            if (remaining < 2 || !utf8_sv::is_continuation(p[1]) || c0 < 0xC2) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x1F) << 6) |  static_cast<char32_t>(p[1] & 0x3F);
            result.push_back(cp);
            p += 2;
            continue;
        }

        // 3-byte
        if ((c0 & 0xF0) == 0xE0) {
            if (remaining < 3 || !utf8_sv::is_continuation(p[1]) || !utf8_sv::is_continuation(p[2])) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            // 拦截过长编码与代理对区间
            if ((c0 == 0xE0 && p[1] < 0xA0) || (c0 == 0xED && p[1] >= 0xA0)) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x0F) << 12)
                              | (static_cast<char32_t>(p[1] & 0x3F) <<  6)
                              |  static_cast<char32_t>(p[2] & 0x3F);
            result.push_back(cp);
            p += 3;
            continue;
        }

        // 4-byte
        if ((c0 & 0xF8) == 0xF0) {
            if (remaining < 4
                || !utf8_sv::is_continuation(p[1])
                || !utf8_sv::is_continuation(p[2])
                || !utf8_sv::is_continuation(p[3]))
            {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            // 拦截过长编码与超出 Unicode 范围
            if ((c0 == 0xF0 && p[1] < 0x90)
                || c0 > 0xF4
                || (c0 == 0xF4 && p[1] > 0x8F))
            {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x07) << 18)
                              | (static_cast<char32_t>(p[1] & 0x3F) << 12)
                              | (static_cast<char32_t>(p[2] & 0x3F) <<  6)
                              |  static_cast<char32_t>(p[3] & 0x3F);
            result.push_back(cp);
            p += 4;
            continue;
        }

        // 无效起始字节（0x80~0xBF、0xF8~0xFF）
        result.push_back(REPLACEMENT);
        ++p;
    }

    return result;
}
//------------------------------------------------------------------------
// UTF-32 转 UTF-8
// 错误策略：非法码点（代理对、超范围）替换为 U+FFFD
std::string utf32_to_8(std::u32string_view u32)
{
    constexpr char32_t MAX_CODEPOINT = 0x10FFFF;

    std::string result;
    result.reserve(u32.size() * 3); // 覆盖绝大多数情况；特殊情况下全是4字节的串也不会很长

    for (char32_t cp : u32) {
        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 |  (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            if (cp >= 0xD800 && cp <= 0xDFFF)   // surrogate不是合法Unicode标量值，替换
                result.append("\xEF\xBF\xBD", 3);
            else {
                result.push_back(static_cast<char>(0xE0 |  (cp >> 12)));
                result.push_back(static_cast<char>(0x80 | ((cp >>  6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 |  (cp        & 0x3F)));
            }
        } else if (cp <= MAX_CODEPOINT) {
            result.push_back(static_cast<char>(0xF0 |  (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >>  6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 |  (cp        & 0x3F)));
        } else  // 超出 Unicode 码点范围
            result.append("\xEF\xBF\xBD", 3);
    }

    return result;
}

//------------------------------------------------------------------------
// UTF-8 转 UTF-16
// 错误策略：无效字节逐字符替换为 U+FFFD
// Windows 版依赖系统 API，替换粒度为「单无效序列一个 U+FFFD」
// 非 Windows 版为自主实现，替换粒度为「单无效字节一个 U+FFFD」
// 二者输出均为合法 UTF-16，仅极端非法输入下替换字符数量略有差异
//------------------------------------------------------------------------

#if UTF_CONV_WINDOWS    //  Windows native implementation

std::u16string utf8_to_16(std::string_view u8)
{
    if (u8.empty()) return {};

    assert(u8.size() <= static_cast<size_t>(INT_MAX) && "Input too large for Win32 API");

    // 第一遍：计算所需缓冲区长度
    int len = ::MultiByteToWideChar(
        CP_UTF8,
        0,                              // 无效字符自动替换为 U+FFFD
        u8.data(),
        static_cast<int>(u8.size()),
        nullptr,
        0);

    // 极端参数错误兜底，正常合法/非法输入均不会触发
    if (len <= 0) return {u'\uFFFD'};

    std::u16string result(static_cast<size_t>(len), u'\0');
    ::MultiByteToWideChar(
        CP_UTF8,
        0,
        u8.data(),
        static_cast<int>(u8.size()),
        reinterpret_cast<LPWSTR>(result.data()),
        len);
    return result;
}

//------------------------------------------------------------------------
#else  // Non-Windows 自主实现

std::u16string utf8_to_16(std::string_view u8)
{
    constexpr char16_t REPLACEMENT = u'\uFFFD';
    std::u16string result;
    result.reserve(u8.size());

    const auto* p   = reinterpret_cast<const unsigned char*>(u8.data());
    const auto* end = p + u8.size();

    while (p < end) {
        const unsigned char c0 = *p;
        const std::size_t remaining = static_cast<std::size_t>(end - p);

        // 1-byte ASCII
        if (c0 <= 0x7F) {
            result.push_back(static_cast<char16_t>(c0));
            ++p;
            continue;
        }

        // 2-byte 序列
        if ((c0 & 0xE0) == 0xC0) {
            if (remaining < 2 || !utf8_sv::is_continuation(static_cast<char>(p[1])) || c0 < 0xC2) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x1F) << 6) |  static_cast<char32_t>(p[1] & 0x3F);
            result.push_back(static_cast<char16_t>(cp));
            p += 2;
            continue;
        }

        // 3-byte 序列
        if ((c0 & 0xF0) == 0xE0) {
            if (remaining < 3
                || !utf8_sv::is_continuation(static_cast<char>(p[1]))
                || !utf8_sv::is_continuation(static_cast<char>(p[2])))
            {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            // 拦截过长编码与代理对区间
            if ((c0 == 0xE0 && p[1] < 0xA0) || (c0 == 0xED && p[1] >= 0xA0)) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x0F) << 12)
                              | (static_cast<char32_t>(p[1] & 0x3F) <<  6)
                              |  static_cast<char32_t>(p[2] & 0x3F);
            result.push_back(static_cast<char16_t>(cp));
            p += 3;
            continue;
        }

        // 4-byte 序列（转代理对）
        if ((c0 & 0xF8) == 0xF0) {
            if (remaining < 4
                || !utf8_sv::is_continuation(static_cast<char>(p[1]))
                || !utf8_sv::is_continuation(static_cast<char>(p[2]))
                || !utf8_sv::is_continuation(static_cast<char>(p[3])))
            {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            // 拦截过长编码与超出 Unicode 范围
            if ((c0 == 0xF0 && p[1] < 0x90) || c0 > 0xF4 || (c0 == 0xF4 && p[1] > 0x8F)) {
                result.push_back(REPLACEMENT);
                ++p;
                continue;
            }
            char32_t cp = (static_cast<char32_t>(c0 & 0x07) << 18)
                        | (static_cast<char32_t>(p[1] & 0x3F) << 12)
                        | (static_cast<char32_t>(p[2] & 0x3F) <<  6)
                        |  static_cast<char32_t>(p[3] & 0x3F);

            // 转为 UTF-16 代理对
            cp -= 0x10000;
            result.push_back(static_cast<char16_t>(0xD800 | (cp >> 10)));
            result.push_back(static_cast<char16_t>(0xDC00 | (cp & 0x3FF)));
            p += 4;
            continue;
        }

        // 无效首字节（孤立续字节、0xF8~0xFF）
        result.push_back(REPLACEMENT);
        ++p;
    }
    return result;
}
#endif // UTF_CONV_WINDOWS

//------------------------------------------------------------------------
// UTF-16 转 UTF-8
// 错误策略：无效代理对替换为 U+FFFD
// Windows 版依赖系统 API，非 Windows 版自主实现，行为基本一致
//------------------------------------------------------------------------

#if UTF_CONV_WINDOWS

std::string utf16_to_8(std::u16string_view u16)
{
    if (u16.empty()) return {};

    assert(u16.size() <= static_cast<size_t>(INT_MAX) && "Input too large for Win32 API");

    // 第一遍：计算所需缓冲区长度
    int len = ::WideCharToMultiByte(
        CP_UTF8,
        0,                              // 无效代理自动替换为 U+FFFD
        reinterpret_cast<LPCWCH>(u16.data()),
        static_cast<int>(u16.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    // 极端参数错误兜底
    if (len <= 0) return {"\xEF\xBF\xBD", 3};

    std::string result(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        reinterpret_cast<LPCWCH>(u16.data()),
        static_cast<int>(u16.size()),
        result.data(),
        len,
        nullptr,
        nullptr);
    return result;
}

//------------------------------------------------------------------------
#else  // Non-Windows 自主实现

std::string utf16_to_8(std::u16string_view u16)
{
    constexpr const char* REPLACEMENT = "\xEF\xBF\xBD";
    constexpr std::size_t REPLACEMENT_LEN = 3;

    std::string result;
    result.reserve(u16.size() * 3);

    const char16_t* p   = u16.data();
    const char16_t* end = p + u16.size();

    while (p < end) {
        const char16_t c1 = *p;

        // 高代理位
        if (c1 >= 0xD800 && c1 <= 0xDBFF) {
            if (p + 1 < end) {
                const char16_t c2 = *(p + 1);
                if (c2 >= 0xDC00 && c2 <= 0xDFFF) {
                    // 合法代理对，计算码点后转 4 字节 UTF-8
                    const char32_t cp = 0x10000u + (((static_cast<char32_t>(c1) & 0x3FFu) << 10) | (static_cast<char32_t>(c2) & 0x3FFu));
                    result.push_back(static_cast<char>(0xF0 |  (cp >> 18)));
                    result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | ((cp >>  6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 |  (cp        & 0x3F)));
                    p += 2;
                    continue;
                }
            }
            // 孤立高代理或截断
            result.append(REPLACEMENT, REPLACEMENT_LEN);
            ++p;
            continue;
        }

        // 孤立低代理
        if (c1 >= 0xDC00 && c1 <= 0xDFFF) {
            result.append(REPLACEMENT, REPLACEMENT_LEN);
            ++p;
            continue;
        }

        // BMP 普通码点
        const char32_t cp = c1;
        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 |  (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xE0 |  (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >>  6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 |  (cp        & 0x3F)));
        }
        ++p;
    }
    return result;
}
#endif // UTF_CONV_WINDOWS
//------------------------------------------------------------------------
