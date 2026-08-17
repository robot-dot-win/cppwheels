//  Strings library extentions.
//
//  Copyright (C) 2024, Martin Young <martin_young@live.cn>
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
#include <algorithm>
#include <vector>
#include <type_traits>
#include <functional>
#include <concepts>
#include <charconv>
#include <optional>
#include <set>
#include <unordered_set>
#include <stdexcept>
#include <map>
#include <memory>
#include <ranges>
#include <cctype>
#include <version>  // __cpp_lib_* macro

#ifdef __cpp_lib_format
#include <format>
#else
#include <cstdio>
#endif

#include "strjoin.hpp"
#include "utf8_sv.hpp"

// About suffix of func names:
//    s  - return std::string type
//    sv - return std::string_view type
//    rf - return object referance

// About prefix of func names:
//    l  - Left or Lowercase
//    r  - Right
//    u  - Uppercase

// Since modern c++ 17:
// . Overloading std::string_view and const std::string& src is not recommended
// . string_view& is not recommended

using TStrVec = std::vector<std::string>;
using TSvVec  = std::vector<std::string_view>;

using TStrSet  = std::set<std::string>;
using TStrMSet = std::multiset<std::string>;

using TStrUoSet  = std::unordered_set<std::string>;
using TStrUoMSet = std::unordered_multiset<std::string>;

using TStrPair = std::pair<std::string, std::string>;
using TSvPair  = std::pair<std::string_view, std::string_view>;

using TStrMap  = std::map<std::string, std::string, std::less<>>;  // No need to construct a temp std::string for find("literal"sv)
using TSvMap   = std::map<std::string_view, std::string_view>;
using TStrMMap = std::multimap<std::string, std::string>;

//extern const std::string EMPTY_STR;
//extern const std::string SPACE_CHARS;

#define sqlquoted(s)   (std::quoted(s,'\'','\''))
#define nposs(pos)     ((pos)==std::string::npos)
#define npossv(pos)    ((pos)==std::string_view::npos)
#define str_found(pos) ((pos)!=std::string::npos)
#define  sv_found(pos) ((pos)!=std::string_view::npos)

//------------------------------------------------------------------------------------------------
constexpr inline std::string_view left_of (std::string_view src, const char mark,       bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view left_of (std::string_view src, std::string_view mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view right_of(std::string_view src, const char mark,       bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view right_of(std::string_view src, std::string_view mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;

constexpr inline std::string_view left_of_from_tail (std::string_view src,       const char mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view left_of_from_tail (std::string_view src, std::string_view mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view right_of_from_tail(std::string_view src,       const char mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;
constexpr inline std::string_view right_of_from_tail(std::string_view src, std::string_view mark, bool return_empty_if_not_found=true, bool including_mark=false) noexcept;

template <typename T2> inline std::string&  erase_left_at (std::string& src, T2 mark) noexcept;
template <typename T2> inline std::string&  erase_right_at(std::string& src, T2 mark) noexcept;

inline std::string_view rightsv(std::string_view sv, size_t n=std::string_view::npos) noexcept { n = std::min(n, sv.size()); return {sv.data() + sv.size() - n, n}; }

inline std::string_view ltrimsv(std::string_view sv) noexcept { return {std::find_if_not(sv.begin(), sv.end(), [](unsigned char c) {return std::isspace(c);}), sv.end()}; }
inline std::string      ltrims (std::string_view sv) noexcept { return std::string(ltrimsv(sv)); }
inline std::string&     ltrimrf(std::string& s)      noexcept { s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](unsigned char c) {return std::isspace(c);})); return s; }

inline std::string_view rtrimsv(std::string_view sv)  noexcept { return {sv.begin(), std::find_if_not(sv.rbegin(), sv.rend(), [](unsigned char c) {return std::isspace(c);}).base()}; }
inline std::string      rtrims (std::string_view sv)  noexcept { return std::string(rtrimsv(sv)); }
inline std::string&     rtrimrf(std::string& s)       noexcept { s.erase(std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {return std::isspace(c);}).base(), s.end()); return s; }

inline std::string      trims   (std::string_view sv) noexcept { return std::string(ltrimsv(rtrimsv(sv))); }
inline std::string_view trimsv  (std::string_view sv) noexcept { return ltrimsv(rtrimsv(sv)); }
inline std::string&     trimrf  (std::string& src)    noexcept;

inline std::string      lcases (std::string_view src) noexcept { std::string dst{}; dst.resize(src.size()); std::transform(src.cbegin(), src.cend(), dst.begin(), [](unsigned char c) noexcept -> char { return static_cast<char>(std::tolower(c)); }); return dst; }
inline std::string      ucases (std::string_view src) noexcept { std::string dst{}; dst.resize(src.size()); std::transform(src.cbegin(), src.cend(), dst.begin(), [](unsigned char c) noexcept -> char { return static_cast<char>(std::toupper(c)); }); return dst; }
inline std::string&     lcaserf(std::string& str) noexcept { std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) -> char {return static_cast<char>(std::tolower(c));}); return str; }
inline std::string&     ucaserf(std::string& str) noexcept { std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) -> char {return static_cast<char>(std::toupper(c));}); return str; }

enum SplitOption: uint8_t {TRIM=1<<0,NOEMPTY=1<<1};
using SplitOptions = uint8_t;
template <typename T> class  splitiv;
template <typename T> inline TStrVec& splits (TStrVec& dst, const std::string& src, T delimiters, SplitOptions opt=0) noexcept;
template <typename T> inline TStrVec  splits (const std::string& src, T delimiters, SplitOptions opt=0) noexcept { TStrVec dst; splits(dst,src,delimiters,opt); return dst; }
template <typename T> inline TSvVec   splitsv(std::string_view src, T delimiters, SplitOptions opt=0) noexcept;

template <typename T> inline TSvPair splitpairsv(std::string_view src, T separator, bool itrim=true) noexcept;

// find and replace all(not use <regex> library in small projects):
       std::string  replall(std::string_view src, std::string_view sfind, std::string_view swith);
inline std::string& replall(std::string& src, const char cfind, const char cwith) noexcept { std::replace(src.begin(),src.end(),cfind,cwith); return src; }

// Remove trailing comment started by the most right character mark, eg. '#' or ';'
inline std::string_view  rmcommsv  (std::string_view  srcv, const char mark='#', bool itrim=true) noexcept;

// string_view windows scanning marked by left and right marks:
template <typename T1, typename T2> inline std::string_view lrmarksv (std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos=0) noexcept;
template <typename T1, typename T2> inline TSvVec           strwinsvv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos=0) noexcept;
template <typename T1, typename T2> class strwinsv;
inline TSvVec multiwinsvv(std::string_view sv, const std::vector<TSvPair> lrmarks, size_t begin_pos=0) noexcept;

// Password generating and checking:
enum class PasswordSecurityLevel {LOW,MEDIUM,HIGH};
std::string genPassword(PasswordSecurityLevel level=PasswordSecurityLevel::MEDIUM, size_t length=8)  noexcept;
bool        chkPassword(std::string_view password, PasswordSecurityLevel level, size_t min_length=4) noexcept;

// Convert a string_view into an integer without error message and exception:
template<std::integral T = int> inline std::optional<T> str2int(std::string_view sv, std::optional<T> minvalue={}, std::optional<T> maxvalue={}, int base=10) noexcept;

template<std::integral T = int> inline bool from_c_succ(const char* first, const char* last, T& value, int base=10) noexcept
    { auto [ptr,ec] = std::from_chars(first, last, value, base); return ec==std::errc{} && ptr==last; }

// Format a character:
#ifdef __cpp_lib_format
#define fmt_char(dest, pattern1, pattern2, c) \
        dest += std::format(pattern1, static_cast<unsigned int>(static_cast<unsigned char>(c)))
#else
inline void fmt_char(std::string& dest, const char* pattern1, const char* pattern2, char c) {
    (void)pattern1;
    char buf[16];
    int n = std::snprintf(buf, sizeof(buf), pattern2, c);
    dest.append(buf, static_cast<std::size_t>(n));
}
#endif

//------------------------------------------------------------------------------------------------
// Deepseek version
template <typename T>
class splitiv {
private:
    T delimiters_;
    std::string_view sv_;
    mutable std::vector<size_t> delimiters_cache_;
    mutable bool cache_valid_{};

    // 惰性构建分隔符位置缓存
    void build_cache() const {
        if (cache_valid_) return;

        delimiters_cache_.clear();
        size_t pos{};
        while (true) {
            const size_t found = sv_.find_first_of(delimiters_, pos);
            if (nposs(found)) break;

            delimiters_cache_.push_back(found);
            pos = found + 1;
        }
        delimiters_cache_.push_back(sv_.size());
        cache_valid_ = true;
    }

public:
    // 构造函数优化
    explicit splitiv(T delimiters): delimiters_(delimiters), sv_("") {}
    splitiv(const std::string& src, T delimiters): delimiters_(delimiters), sv_(src) {}

    // 设置新源字符串
    void reset(const std::string& src) {
        sv_ = src;
        cache_valid_ = false;  // 重置缓存状态
    }

    // This line added by me:
    splitiv& operator=(const std::string& src) { reset(src); return *this; }

    // 常量访问方法
    size_t size() const {
        if (!cache_valid_) build_cache();
        return delimiters_cache_.size();
    }

    // 按需访问优化
    std::string_view operator[](size_t n) const {
        if (!cache_valid_) build_cache();
        if (n >= delimiters_cache_.size()) throw std::out_of_range("");

        const size_t start = (n == 0) ? 0 : delimiters_cache_[n-1] + 1;
        const size_t end = delimiters_cache_[n];
        return sv_.substr(start, end - start);
    }

    // 高效范围访问
    template <typename F>
    void for_each(F&& callback) const {
        size_t start{};
        sv_.find_first_of(delimiters_, 0, [&](size_t found) {
            callback(sv_.substr(start, found - start));
            start = found + 1;
            return false; // 继续查找
        });
    }
};

//------------------------------------------------------------------------------------------------
template <typename T>
TStrVec& splits(TStrVec& dst, const std::string& src, T delimiters, SplitOptions opt) noexcept
{
    dst.clear();
    if (src.empty()) return dst;

    const char* const data{src.data()};
    const size_t src_len{src.size()};
    size_t start{};

    // 预分配优化：根据经验值预留空间
    if (dst.capacity() < 16) {
        dst.reserve(src_len / 8 + 2);  // 经验公式：每8字符一个分隔符
    }

    // 类型分发优化
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        // 单字符分隔符优化路径
        while (true) {
            const size_t end {src.find(delimiters, start)};
            const bool is_end {nposs(end)};
            const size_t len {(is_end?src_len:end) - start};
            std::string s(data+start, len);
            if( opt & SplitOption::TRIM ) trimrf(s);
            if( !s.empty() || !(opt & SplitOption::NOEMPTY) ) dst.push_back(move(s));
            if( is_end ) break;
            ++(start=end);
        }
    } else {
        // 多字符分隔符处理
        const std::string_view delims{delimiters};
        const size_t delims_len{delims.length()};

        // SIMD优化预处理（示例伪代码）
        if (delims_len > 1 && (src_len > 256)) {
            // 此处可添加SIMD加速查找逻辑
        }

        // 常规处理
        while (true) {
            const size_t end {src.find_first_of(delims, start)};
            const bool is_end {nposs(end)};
            const size_t len {(is_end?src_len:end) - start};
            std::string s(data+start, len);
            if( opt & SplitOption::TRIM ) trimrf(s);
            if( !s.empty() || !(opt & SplitOption::NOEMPTY) ) dst.push_back(move(s));
            if( is_end ) break;
            ++(start=end);
        }
    }

    // 后置容量优化：避免过度预留
    if (dst.capacity() > dst.size() * 4) {
        dst.shrink_to_fit();
    }

    return dst;
}

//------------------------------------------------------------------------------------------------
template <typename T>
TSvVec splitsv(std::string_view src, T delimiters, SplitOptions opt) noexcept
{
    TSvVec dst;
    if (src.empty()) return dst;

    const char* const data{src.data()};
    const size_t src_len {src.size()};
    size_t start {};

    // 类型分发优化
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        // 单字符分隔符优化路径
        while (true) {
            const size_t end {src.find(delimiters, start)};
            const bool is_end  {npossv(end)};
            const size_t len {(is_end? src_len:end) - start};
            std::string_view sv(data+start, len);
            if( opt & SplitOption::TRIM ) sv = trimsv(sv);
            if( !sv.empty() || !(opt & SplitOption::NOEMPTY) ) dst.push_back(sv);
            if( is_end ) break;
            ++(start=end);
        }
    } else {
        // 通用分隔符处理
        const std::string_view delims(delimiters);
        while (true) {
            const size_t end {src.find_first_of(delims, start)};
            const bool is_end  {npossv(end)};
            const size_t len {(is_end? src_len:end) - start};
            std::string_view sv(data+start, len);
            if( opt & SplitOption::TRIM ) sv = trimsv(sv);
            if( !sv.empty() || !(opt & SplitOption::NOEMPTY) ) dst.push_back(sv);
            if( is_end ) break;
            ++(start=end);
        }
    }

    return dst;
}

//------------------------------------------------------------------------------------------------
std::string& trimrf(std::string& src) noexcept
{
    src.erase(std::find_if_not(src.rbegin(), src.rend(), [](unsigned char c) {return std::isspace(c);}).base(), src.end());
    src.erase(src.begin(), std::find_if_not(src.begin(), src.end(), [](unsigned char c) {return std::isspace(c);}));
    return src;
}

//------------------------------------------------------------------------------------------------
template <typename T1, typename T2>
class strwinsv {
private:
    std::string_view src;
    T1 leftmark;
    T2 rightmark;
    size_t current_pos{};

    template <typename T>
    size_t find_mark(T mark, size_t pos) const {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, char>)
            return src.find(mark, pos);
        else
            return src.find(std::string_view(mark), pos);
    }

public:
    std::string_view winsv;

    strwinsv(std::string_view s, T1 lm, T2 rm, size_t begin_pos = 0)
        : src(s), leftmark(lm), rightmark(rm), current_pos(begin_pos) {}

    bool next() {
        const auto lpos {find_mark(leftmark, current_pos)};
        if (npossv(lpos)) return false;

        size_t start {lpos};
        if constexpr (std::is_same_v<std::remove_cv_t<T1>, char>)
            start += 1;
        else
            start += leftmark.size();

        const auto rpos = find_mark(rightmark, start);
        if (npossv(rpos)) return false;

        winsv = src.substr(start, rpos - start);

        if constexpr (std::is_same_v<std::remove_cv_t<T2>, char>)
            current_pos = rpos + 1;
        else
            current_pos = rpos + rightmark.size();

        return true;
    }
};

//------------------------------------------------------------------------------------------------
template <typename T1, typename T2>
std::string_view lrmarksv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos) noexcept
{
    auto findmark = [&sv](auto mark, size_t pos) {
        if constexpr (std::is_same_v<decltype(mark), char>)
            return sv.find(mark, pos);
        else
            return sv.find(std::string_view(mark), pos);
    };

    const size_t lpos {findmark(leftmark, begin_pos)};
    if (npossv(lpos)) return {};

    size_t start {lpos};
    if constexpr (std::is_same_v<std::remove_cv_t<T1>, char>)
        start += 1;
    else
        start += std::string_view(leftmark).size();

    const size_t rpos {findmark(rightmark, start)};
    if (npossv(rpos)) return {};

    return sv.substr(start, rpos - start);
}

//------------------------------------------------------------------------------------------------
// By Deepseek
template <typename T1, typename T2>
TSvVec strwinsvv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos) noexcept
{
    TSvVec results;
    strwinsv<T1, T2> scanner(sv, leftmark, rightmark, begin_pos);
    if constexpr (std::is_same_v<std::remove_cv_t<T1>, char> && std::is_same_v<std::remove_cv_t<T2>, char>)
        results.reserve(sv.size() / 8);  // 预分配内存优化（按经验值）：每8字符一个匹配
    while (scanner.next()) results.emplace_back(scanner.winsv);
    results.shrink_to_fit();
    return results;
}

//------------------------------------------------------------------------------------------------
// Caution: The separator is either a character or a STRING. Eg.
//      splitpairsv("key=value", '=');        // {"key", "value"}
//      splitpairsv("key=>value", "=>");      // {"key", "value"}
template <typename T>
TSvPair splitpairsv(std::string_view src, T separator, bool itrim) noexcept
{
    size_t pos {std::string_view::npos};
    size_t delimiter_length {};

    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        pos = src.find(separator);
        delimiter_length = 1;
    } else {
        const std::string_view sv_delimiter(separator);
        pos = src.find(sv_delimiter);
        delimiter_length = sv_delimiter.size();
    }

    if (npossv(pos) || delimiter_length == 0) return {itrim?trimsv(src):src, {}};

    const size_t second_start {pos+delimiter_length};
    return {
        itrim? trimsv(src.substr(0, pos)) : src.substr(0, pos),
        second_start<src.size()? (itrim? trimsv(src.substr(second_start)) : src.substr(second_start)) : std::string_view{}
    };
}

//------------------------------------------------------------------------------------------------
// If the first char is mark, the whole string is comment, otherwise the comment is from the most
// right mark char to the end of the string.
std::string_view rmcommsv(std::string_view srcv, const char mark, bool itrim) noexcept
{
    if( itrim ) srcv = trimsv(srcv);
    if( srcv.empty() || srcv[0]==mark ) return {};

    const size_t comment_pos = srcv.rfind(mark);
    return npossv(comment_pos)? srcv : (itrim? rtrimsv(srcv.substr(0, comment_pos)) : srcv.substr(0, comment_pos));
}

//------------------------------------------------------------------------------------------------
// Caution: Definition of template function is NOT the same as declaration in format
template<std::integral T>
std::optional<T> str2int(std::string_view sv, std::optional<T> minvalue, std::optional<T> maxvalue, int base) noexcept
{
    if (sv.empty()) return std::nullopt;

    T value{};
    if ( !from_c_succ(sv.data(), sv.data()+sv.size(), value, base)
        || (minvalue.has_value() && value < minvalue.value())
        || (maxvalue.has_value() && value > maxvalue.value())
    ) return std::nullopt;

    return value;
}

// -----------------------------------------------------------------------------
constexpr inline std::string_view left_of(std::string_view src, const char mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.find(mark); pos != std::string_view::npos) {
        return src.substr(0, including_mark ? pos + 1 : pos);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}
constexpr inline std::string_view left_of(std::string_view src, std::string_view mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.find(mark); pos != std::string_view::npos) {
        return src.substr(0, including_mark ? pos + mark.size() : pos);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}

// -----------------------------------------------------------------------------
constexpr inline std::string_view right_of(std::string_view src, const char mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.find(mark); pos != std::string_view::npos) {
        return src.substr(including_mark ? pos : pos + 1);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}
constexpr inline std::string_view right_of(std::string_view src, std::string_view mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.find(mark); pos != std::string_view::npos) {
        return src.substr(including_mark ? pos : pos + mark.size());
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}
// -----------------------------------------------------------------------------
constexpr inline std::string_view left_of_from_tail(std::string_view src, const char mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.rfind(mark); pos != std::string_view::npos) {
        return src.substr(0, including_mark ? pos + 1 : pos);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}
constexpr inline std::string_view left_of_from_tail(std::string_view src, std::string_view mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.rfind(mark); pos != std::string_view::npos) {
        return src.substr(0, including_mark ? pos + mark.size() : pos);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}

// -----------------------------------------------------------------------------
constexpr inline std::string_view right_of_from_tail(std::string_view src, const char mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.rfind(mark); pos != std::string_view::npos) {
        return src.substr(including_mark ? pos : pos + 1);
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}
constexpr inline std::string_view right_of_from_tail(std::string_view src, std::string_view mark, bool return_empty_if_not_found, bool including_mark) noexcept
{
    if (const size_t pos = src.rfind(mark); pos != std::string_view::npos) {
        return src.substr(including_mark ? pos : pos + mark.size());
    }
    return return_empty_if_not_found ? std::string_view{} : src;
}

//------------------------------------------------------------------------------------------------
template <typename T2>
std::string& erase_left_at (std::string& src, T2 mark) noexcept
{
    const size_t lpos {src.find(mark)};
    if( nposs(lpos) ) return src;

    size_t mark_size{1};
    if constexpr (!std::is_same_v<std::remove_cv_t<T2>, char>)
        mark_size = std::string_view(mark).size();

    if( lpos+mark_size==src.size() ) src.clear();
    else src.erase(0, lpos+mark_size);

    return src;
}
//------------------------------------------------------------------------------------------------
template <typename T2>
std::string&  erase_right_at(std::string& src, T2 mark) noexcept
{
    const size_t lpos {src.find(mark)};
    return str_found(lpos)? src.erase(lpos) : src;
}
//------------------------------------------------------------------------------------------------
TSvVec multiwinsvv(std::string_view sv, const std::vector<TSvPair> lrmarks, size_t begin_pos) noexcept
{
    TSvVec results;
    results.reserve(lrmarks.size());
    size_t current_search_pos {begin_pos};

    for (const auto& [left_mark,right_mark] : lrmarks) {
        const size_t lpos {sv.find(left_mark, current_search_pos)};
        if (npossv(lpos)) {
            results.emplace_back();
            continue;
        }
        const size_t start_content_pos {lpos + left_mark.length()};
        const size_t rpos {sv.find(right_mark, start_content_pos)};
        if (npossv(rpos)) {
            results.emplace_back();
            continue;
        }
        results.emplace_back(sv.substr(start_content_pos, rpos - start_content_pos));
        current_search_pos = rpos + right_mark.length();
    }
    return results;
}
//------------------------------------------------------------------------------------------------
