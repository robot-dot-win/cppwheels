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

#ifndef QQ_STREXT_HPP
#define QQ_STREXT_HPP

#include <string>
#include <string_view>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <functional>    // For Boyer-Moore searcher since c++17
#include <stdexcept>     // not needed explicitly since c++20

// About suffix of func names:
//    s  - return std::string type
//    sv - return std::string_view type
//    rf - return object referance

// About prefix of func names:
//    l  - Left or Lowercase
//    r  - Right
//    u  - Uppercase

extern const std::string EMPTY_STR;
//extern const std::string SPACE_CHARS;

#define sqlquoted(s) (std::quoted(s,'\'','\''))
#define nposs(pos)   ((pos)==std::string::npos)
#define npossv(pos)  ((pos)==std::string_view::npos)
#define lefts(s,n)   ((s).substr(0,n))
#define leftsv(s,n)  ((s).substr(0,n))   // Since C++ 17

enum class PasswordSecurityLevel {
    LOW,
    MEDIUM,
    HIGH
};

inline std::string       rights (const std::string& src, size_t n=std::string::npos) noexcept { const size_t src_len{src.size()}; return n>=src_len? src : src.substr(src_len-n); }
inline std::string_view  rightsv(const std::string& src, size_t n=std::string::npos) noexcept { const size_t src_len{src.size()}; return n>=src_len? std::string_view(src) : std::string_view(src.data()+src_len-n,n); }

inline std::string       ltrims (std::string_view sv)    noexcept;
inline std::string       ltrims (const std::string& s)   noexcept { return ltrims(std::string_view{s}); }
inline std::string_view  ltrimsv(std::string_view sv)    noexcept;
inline std::string_view  ltrimsv(const std::string& s)   noexcept { return ltrimsv(std::string_view{s}); }
inline std::string&      ltrimrf(std::string& s)         noexcept { s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](unsigned char c) {return std::isspace(c);})); return s; }
inline std::string_view& ltrimsvrf(std::string_view& sv) noexcept;
//#define ltrimrf(src) ((src).erase(0, (src).find_first_not_of(SPACE_CHARS))) // My version

inline std::string       rtrims (std::string_view sv)    noexcept;
inline std::string       rtrims (const std::string& s)   noexcept { return rtrims(std::string_view{s}); }
inline std::string_view  rtrimsv(std::string_view sv)    noexcept;
inline std::string_view  rtrimsv(const std::string& s)   noexcept { return rtrimsv(std::string_view{s}); }
inline std::string&      rtrimrf(std::string& s)         noexcept { s.erase(std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {return std::isspace(c);}).base(), s.end()); return s; }
inline std::string_view& rtrimsvrf(std::string_view& sv) noexcept;
//#define rtrimrf(src) (src.erase(src.find_last_not_of(SPACE_CHARS) + 1))    // My version

inline std::string       trims   (std::string_view sv)    noexcept;   // Deepseek recommended
inline std::string       trims   (const std::string& src) noexcept { return trims(std::string_view{src}); }   // not needed for C++ 20
inline std::string_view  trimsv  (std::string_view sv)    noexcept;   // Deepseek recommended
inline std::string_view  trimsv  (const std::string& src) noexcept { return trimsv(std::string_view{src}); }
inline std::string&      trimrf  (std::string& src)       noexcept;
inline std::string_view& trimsvrf(std::string_view& sv)   noexcept { return ltrimsvrf(rtrimsvrf(sv)); }

inline std::string lcases (std::string_view   src) noexcept { std::string dst{}; dst.resize(src.size()); std::transform(src.cbegin(), src.cend(), dst.begin(), [](unsigned char c) noexcept -> char { return static_cast<char>(std::tolower(c)); }); return dst; }
inline std::string lcases (const std::string& src) noexcept { return lcases(std::string_view{src}); }
inline std::string ucases (std::string_view   src) noexcept { std::string dst{}; dst.resize(src.size()); std::transform(src.cbegin(), src.cend(), dst.begin(), [](unsigned char c) noexcept -> char { return static_cast<char>(std::toupper(c)); }); return dst; }
inline std::string ucases (const std::string& src) noexcept { return ucases(std::string_view{src}); }
// My version:
//inline std::string& lcaserf(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::tolower); return src; }
//inline std::string& ucaserf(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::toupper); return src; }
// Deepseek version:
inline std::string& lcaserf(std::string& str) noexcept { std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) -> char {return static_cast<char>(std::tolower(c));}); return str; }
inline std::string& ucaserf(std::string& str) noexcept { std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) -> char {return static_cast<char>(std::toupper(c));}); return str; }

template <class T> class  spliti;
template <class T> class  splitiv;  // Deepseek version
template <class T> inline std::vector<std::string>&     splits (std::vector<std::string>& dst, const std::string& src, T delimiters) noexcept;
template <class T> inline std::vector<std::string_view> splitsv(std::string_view   src, T delimiters) noexcept;
template <class T> inline std::vector<std::string_view> splitsv(const std::string& src, T delimiters) noexcept { return splitsv(std::string_view{src},delimiters); }

template <class T> inline std::pair<std::string_view, std::string_view> splitpairsv(std::string_view   src, T separator, bool itrim=true) noexcept;
template <class T> inline std::pair<std::string_view, std::string_view> splitpairsv(const std::string& src, T separator, bool itrim=true) noexcept { return splitpairsv(std::string_view{src},separator,itrim); }

// find and replace all(not use <regex> library in small projects):
       std::string  replall(                  std::string_view   src, std::string_view    sfind, std::string_view   swith) noexcept;
inline std::string  replall(                  const std::string& src, const std::string& sfind, const std::string& swith) noexcept { return replall(std::string_view(src), std::string_view(sfind), std::string_view(swith)); }  // not needed for C++ 20
       std::string& replall(std::string& res, std::string_view   src, std::string_view    sfind, std::string_view   swith) noexcept;
inline std::string& replall(std::string& res, const std::string& src, const std::string& sfind, const std::string& swith) noexcept { return replall(res, std::string_view(src), std::string_view(sfind), std::string_view(swith)); }  // not needed for C++ 20
inline std::string& replall(std::string& src, const char cfind, const char cwith) noexcept { std::replace(src.begin(),src.end(),cfind,cwith); return src; }

// Remove trailing comment started by the most right character mark, eg. '#' or ';'
inline std::string_view  rmcommsv  (std::string_view  srcv, const char mark='#', bool itrim=true) noexcept;
inline std::string_view& rmcommsvrf(std::string_view& srcv, const char mark='#', bool itrim=true) noexcept;

// string_view windows scanning marked by left and right marks:
template <class T1, class T2> inline std::string_view              lrmarksv (std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos=0) noexcept;
template <class T1, class T2> inline std::vector<std::string_view> strwinsvv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos=0) noexcept;
template <class T1, class T2> class strwinsv;

std::string genPassword(PasswordSecurityLevel level=PasswordSecurityLevel::MEDIUM, size_t length=8);
bool        chkPassword(std::string_view password, PasswordSecurityLevel level);

//------------------------------------------------------------------------------------------------
// My version (for compatibility)
template <class T> class spliti {
protected:
    T d_;                       // delimiter(s)
    const std::string *ps_;     // hold the string to split
    std::vector<size_t> idx;    // Position of every delimiter. The last one is size()
public:
    spliti(T delimiters): d_(delimiters), ps_(&EMPTY_STR), idx(0) {}
    spliti(const std::string *psrc, T delimiters): d_(delimiters) { *this=psrc; }

    spliti& operator=(const std::string *psrc) {
        ps_ = psrc? psrc : &EMPTY_STR;
        size_t nfrom{}, nfind;
        idx.clear();
        do {
            idx.push_back(nfrom = nfind = (*ps_).find_first_of(d_, nfrom));
            nfrom++;
        } while(!nposs(nfind));
        *idx.rbegin() = (*ps_).size();
        return *this;
    }

    size_t size() const { return idx.size(); }

    // may throw exeption if ps_ or n is invalid:
    std::string      s (size_t n) const { return n==0? (*ps_).substr(0,idx[0]) : (*ps_).substr(idx[n-1]+1,idx[n]-idx[n-1]-1); }
    std::string_view sv(size_t n) const { return n==0? std::string_view((*ps_).data(),idx[0]) : std::string_view((*ps_).data()+idx[n-1]+1,idx[n]-idx[n-1]-1); }
};

//------------------------------------------------------------------------------------------------
// Deepseek version
template <class T> class splitiv {
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
    template <class F>
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
template <class T>
std::vector<std::string>& splits(std::vector<std::string>& dst, const std::string& src, T delimiters) noexcept
{
    // My version:
    // size_t nfrom, nfind;
    // dst.clear();
    // if( !src.empty() ) {
    //     for( nfrom=0; !nposs(nfind = src.find_first_of(delimiters, nfrom)); nfrom=nfind+1 )
    //         dst.emplace_back(src.substr(nfrom, nfind-nfrom));
    //     dst.emplace_back(src.substr(nfrom, src.size()-nfrom));
    // }
    // return dst;

    // Deepseek optimized version:
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
            const size_t end{src.find(delimiters, start)};
            dst.emplace_back(data + start, (nposs(end)?src_len:end) - start);

            if (nposs(end)) break;
            start = end + 1;
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
            const size_t end{src.find_first_of(delims, start)};
            dst.emplace_back(data + start, (nposs(end)? src_len:end) - start);

            if (nposs(end)) break;
            start = end + 1;
        }
    }

    // 后置容量优化：避免过度预留
    if (dst.capacity() > dst.size() * 4) {
        dst.shrink_to_fit();
    }

    return dst;
}

//------------------------------------------------------------------------------------------------
template <class T>
std::vector<std::string_view> splitsv(std::string_view src, T delimiters) noexcept
{
    // My version:
    // std::vector<std::string_view> dst;
    // size_t nfrom, nfind;
    // if( !src.empty() ) {
    //     for( nfrom=0; !nposs(nfind = src.find_first_of(delimiters, nfrom)); nfrom=nfind+1 )
    //         dst.emplace_back(src.data()+nfrom, nfind-nfrom);
    //     dst.emplace_back(src.data()+nfrom, src.size()-nfrom);
    // }
    // return dst;

    // Deepseek optimized version:
    std::vector<std::string_view> dst;
    if (src.empty()) return dst;

    const size_t src_len {src.size()};
    size_t start_pos {};

    // 类型分发优化
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>) {
        // 单字符分隔符优化路径
        while (true) {
            const size_t found {src.find(delimiters, start_pos)};
            const bool is_end  {npossv(found)};
            const size_t substr_len {(is_end? src_len:found) - start_pos};

            dst.emplace_back(src.data() + start_pos, substr_len);

            if (is_end) break;
            start_pos = found + 1;
        }
    } else {
        // 通用分隔符处理
        const std::string_view delims(delimiters);
        while (true) {
            const size_t found {src.find_first_of(delims, start_pos)};
            const bool is_end  {npossv(found)};
            const size_t substr_len {(is_end? src_len:found) - start_pos};

            dst.emplace_back(src.data() + start_pos, substr_len);

            if (is_end) break;
            start_pos = found + 1;
        }
    }

    return dst;
}

//------------------------------------------------------------------------------------------------
// My version
// std::string  trims (const std::string& src)
// {
//     if( src.empty() || (!std::isspace(*src.begin()) && !std::isspace(*src.rbegin())) ) return src;
//
//     size_t i{};
//     size_t j = src.size()-1;
//
//     while( i<=j && std::isspace(src[i]) ) i++;
//     if( i>j ) return {};
//
//     while( j>i && std::isspace(src[j]) ) j--;
//     return src.substr(i, j-i+1);
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string trims(std::string_view sv) noexcept
{
    auto first = std::find_if_not(sv.begin(), sv.end(),
                [](unsigned char c) {return std::isspace(c);});

    if (first == sv.end()) return {};

    auto last = std::find_if_not(sv.rbegin(), sv.rend(),
                [](unsigned char c) {return std::isspace(c);}).base();

    return (first > last) ? std::string{} : std::string{first, last};
}

//------------------------------------------------------------------------------------------------
// My version
// std::string_view  trimsv(const std::string& src)
// {
//     if( src.empty() || (!std::isspace(*src.begin()) && !std::isspace(*src.rbegin())) ) return std::string_view(src);
//
//     size_t i{};
//     size_t j = src.size()-1;
//
//     while( i<=j && std::isspace(src[i]) ) i++;
//     if( i>j ) return {};
//
//     while( j>i && std::isspace(src[j]) ) j--;
//     return std::string_view(src.data()+i, j-i+1);
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string_view trimsv(std::string_view sv) noexcept
{
    if (sv.empty()) return sv;

    auto first = std::find_if_not(sv.begin(), sv.end(), [](unsigned char c) {return std::isspace(c);});

    if (first == sv.end()) return {};

    auto last = std::find_if_not(sv.rbegin(), sv.rend(), [](unsigned char c) {return std::isspace(c);}).base();

    return (first < last)
        ? std::string_view{&*first, static_cast<size_t>(last - first)}
        : std::string_view{};
}

//------------------------------------------------------------------------------------------------
// My version
// std::string& trimrf(std::string& src)
// {
//     src.erase(0, src.find_first_not_of(SPACE_CHARS));
//     src.erase(src.find_last_not_of(SPACE_CHARS) + 1);
//
//     return src;
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string& trimrf(std::string& src) noexcept
{
    auto first = std::find_if_not(src.begin(),  src.end(),  [](unsigned char c) { return std::isspace(c); });
    auto last  = std::find_if_not(src.rbegin(), src.rend(), [](unsigned char c) { return std::isspace(c); }).base();

    // 触发移动语义优化
    if (first < last) src = std::string(first, last); else src.clear();

    return src;
}

//------------------------------------------------------------------------------------------------
// My version
// std::string ltrims(const std::string& src)
// {
//     if( src.empty() || !std::isspace(*src.begin()) ) return src;
//
//     size_t i{1};
//     size_t j = src.size()-1;
//
//     while( i<=j && std::isspace(src[i]) ) i++;
//
//     return i>j? std::string() : src.substr(i);
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string ltrims(std::string_view sv) noexcept
{
    auto first = std::find_if_not(sv.begin(), sv.end(), [](unsigned char c) { return std::isspace(c); });
    return (first != sv.end())? std::string(first, sv.end()) : std::string{};
}

//------------------------------------------------------------------------------------------------
// My version
// std::string_view ltrimsv(const std::string& src)
// {
//     if( src.empty() || !std::isspace(*src.begin()) ) return std::string_view(src);
//
//     size_t i{1};
//     size_t j = src.size()-1;
//
//     while( i<=j && std::isspace(src[i]) ) i++;
//
//     return i>j? std::string_view() : std::string_view(src.data()+i, src.size()-i);
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string_view ltrimsv(std::string_view sv) noexcept
{
    auto first = std::find_if_not(sv.begin(), sv.end(), [](unsigned char c) { return std::isspace(c); });
    return (first != sv.end())? sv.substr(static_cast<size_t>(first - sv.begin())) : std::string_view{};
}
//------------------------------------------------------------------------------------------------
std::string_view& ltrimsvrf(std::string_view& sv) noexcept
{
    auto it = std::find_if_not(sv.begin(), sv.end(), [](unsigned char c) {return std::isspace(c);});
    sv.remove_prefix(static_cast<size_t>(it - sv.begin()));
    return sv;
}

//------------------------------------------------------------------------------------------------
// My version
// std::string rtrims (const std::string& src)
// {
//     if( src.empty() || !std::isspace(*src.rbegin()) ) return src;
//
//     auto r = src.rbegin(); r++;
//     while( r!=src.rend() && std::isspace(*r) ) r++;
//     return r==src.rend()? std::string() : src.substr(0, src.size()-(r-src.rbegin()));
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string rtrims(std::string_view sv) noexcept
{
    auto last = std::find_if_not(sv.rbegin(), sv.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return (last > sv.begin())? std::string(sv.begin(), last) : std::string{};
}

//------------------------------------------------------------------------------------------------
// My version
// std::string_view  rtrimsv(const std::string& src)
// {
//     if( src.empty() || !std::isspace(*src.rbegin()) ) return std::string_view(src);
//
//     auto r = src.rbegin(); r++;
//     while( r!=src.rend() && std::isspace(*r) ) r++;
//     return r==src.rend()? std::string_view() : std::string_view(src.data(), src.size()-(r-src.rbegin()));
// }
//------------------------------------------------------------------------------------------------
// Deepseek version
std::string_view rtrimsv(std::string_view sv) noexcept
{
    auto last = std::find_if_not(sv.rbegin(), sv.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return (last > sv.begin()) ? sv.substr(0, static_cast<size_t>(last - sv.begin())) : std::string_view{};
}

//------------------------------------------------------------------------------------------------
std::string_view& rtrimsvrf(std::string_view& sv) noexcept
{
    auto rit = std::find_if_not(sv.rbegin(), sv.rend(), [](unsigned char c) { return std::isspace(c); });
    sv.remove_suffix(static_cast<size_t>(sv.end() - rit.base()));
    return sv;
}

//------------------------------------------------------------------------------------------------
template <class T1, class T2> class strwinsv {
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
template <class T1, class T2> std::string_view
lrmarksv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos) noexcept
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
template <class T1, class T2> inline std::vector<std::string_view>
strwinsvv(std::string_view sv, T1 leftmark, T2 rightmark, size_t begin_pos) noexcept
{
    std::vector<std::string_view> results;
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
template <class T> inline std::pair<std::string_view, std::string_view>
splitpairsv(std::string_view src, T separator, bool itrim) noexcept
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
inline std::string_view rmcommsv(std::string_view srcv, const char mark, bool itrim) noexcept
{
    std::string_view sv = itrim? trimsv(srcv) : srcv;
    if( sv.empty() || sv[0]==mark ) return {};

    const size_t comment_pos = sv.rfind(mark);
    return npossv(comment_pos)? sv : (itrim? rtrimsv(sv.substr(0, comment_pos)) : sv.substr(0, comment_pos));
}

//------------------------------------------------------------------------------------------------
inline std::string_view& rmcommsvrf(std::string_view& srcv, const char mark, bool itrim) noexcept
{
    if( itrim ) trimsvrf(srcv);
    if( srcv.empty() || srcv[0]==mark ) return srcv={};

    const size_t comment_pos = srcv.rfind(mark);
    if( npossv(comment_pos) ) return srcv;
    srcv.remove_suffix(srcv.size() - comment_pos);
    if( itrim ) rtrimsvrf(srcv); // remove spaces between contents and mark
    return srcv;
}

//------------------------------------------------------------------------------------------------
#endif // QQ_STREXT_HPP
