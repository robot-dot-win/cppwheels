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
#include <stdexcept>     // not needed explicitly since c++20

extern const std::string EMPTY_STR;
extern const std::string SPACE_CHARS;

#define sqlquoted(s) (std::quoted(s,'\'','\''))
#define nposs(pos) ((pos)==std::string::npos)
#define lefts(s,n) ((s).substr(0,n))
#define leftsv(s,n) (std::string_view((s).data(),n))

inline std::string       rights (const std::string& src, size_t n=std::string::npos) { return n>=src.size()? src : src.substr(src.size()-n); }
inline std::string_view  rightsv(const std::string& src, size_t n=std::string::npos) { return n>=src.size()? std::string_view(src) : std::string_view(src.data()+src.size()-n,n); }

inline std::string       trims (const std::string& src);
inline std::string_view  trimsv(const std::string& src);
inline std::string&      trimr (      std::string& src);

inline std::string       ltrims (const std::string& src);
inline std::string_view  ltrimsv(const std::string& src);
//inline std::string&      ltrimr (      std::string& src);
#define ltrimr(src) (src.erase(0, src.find_first_not_of(SPACE_CHARS)))

inline std::string       rtrims (const std::string& src);
inline std::string_view  rtrimsv(const std::string& src);
//inline std::string&      rtrimr (      std::string& src);
#define rtrimr(src) (src.erase(src.find_last_not_of(SPACE_CHARS) + 1))

inline std::string  lcase (const std::string& src);
inline std::string  ucase (const std::string& src);

inline std::string& lcaser(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::tolower); return src; }
inline std::string& ucaser(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::toupper); return src; }

template <class T> class  spliti;
template <class T> class  splitiv;  // Deepseek version
template <class T> inline std::vector<std::string>&      splits (std::vector<std::string>& dst, const std::string& src, T delimiters);
template <class T> inline std::vector<std::string_view>  splitsv(const std::string& src, T delimiters);

// find and replace all(not use <regex> library in small projects):
inline std::string  replall(                  const std::string& src, const std::string& sfind, const std::string& swith);
inline std::string& replall(std::string& res, const std::string& src, const std::string& sfind, const std::string& swith);
inline std::string& replall(std::string& src, const char cfind, const char cwith) { std::replace(src.begin(),src.end(),cfind,cwith); return src; }

//------------------------------------------------------------------------------------------------
template <class T>
class spliti {
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
// This is Deepseek version.
template <class T>
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
    explicit splitiv(T delimiters)
        : delimiters_(delimiters), sv_("") {}

    splitiv(const std::string& src, T delimiters)
        : delimiters_(delimiters), sv_(src) {}

    // 设置新源字符串
    void reset(const std::string& src) {
        sv_ = src;
        cache_valid_ = false;  // 重置缓存状态
    }

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
std::vector<std::string>& splits(std::vector<std::string>& dst, const std::string& src, T delimiters)
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

    const char* const data = src.data();
    const size_t src_len = src.size();
    size_t start{};

    // 预分配优化：根据经验值预留空间
    if (dst.capacity() < 16) {
        dst.reserve(src_len / 8 + 2);  // 经验公式：每8字符一个分隔符
    }

    // 类型分发优化
    if constexpr (std::is_same_v<T, char>) {
        // 单字符分隔符优化路径
        while (true) {
            const size_t end = src.find(delimiters, start);
            dst.emplace_back(data + start, (end == std::string::npos) ? src_len - start : end - start);

            if (end == std::string::npos) break;
            start = end + 1;
        }
    } else {
        // 多字符分隔符处理
        const std::string_view delims(delimiters);
        const size_t delims_len = delims.length();

        // SIMD优化预处理（示例伪代码）
        if (delims_len > 1 && (src_len > 256)) {
            // 此处可添加SIMD加速查找逻辑
        }

        // 常规处理
        while (true) {
            const size_t end = src.find_first_of(delims, start);
            dst.emplace_back(data + start, (end == std::string::npos) ? src_len - start : end - start);

            if (end == std::string::npos) break;
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
std::vector<std::string_view> splitsv(const std::string& src, T delimiters)
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

    const size_t src_len = src.size();
    size_t start_pos{};

    // 类型分发优化
    if constexpr (std::is_same_v<T, char>) {
        // 针对单个字符的优化路径
        while (true) {
            const size_t found = src.find(delimiters, start_pos);
            const bool is_end = (found == std::string::npos);
            const size_t substr_len = is_end ? src_len - start_pos : found - start_pos;

            dst.emplace_back(&src[start_pos], substr_len);

            if (is_end) break;
            start_pos = found + 1;
        }
    } else {
        const std::string_view delims(delimiters);
        // 通用分隔符处理
        while (true) {
            const size_t found = src.find_first_of(delims, start_pos);
            const bool is_end = (found == std::string::npos);
            const size_t substr_len = is_end ? src_len - start_pos : found - start_pos;

            dst.emplace_back(&src[start_pos], substr_len);

            if (is_end) break;
            start_pos = found + 1;
        }
    }

    return dst;
}

//------------------------------------------------------------------------------------------------
std::string  trims (const std::string& src)
{
    if( src.empty() || (!std::isspace(*src.begin()) && !std::isspace(*src.rbegin())) ) return src;

    size_t i{};
    size_t j = src.size()-1;

    while( i<=j && std::isspace(src[i]) ) i++;
    if( i>j ) return {};

    while( j>i && std::isspace(src[j]) ) j--;
    return src.substr(i, j-i+1);
}

std::string_view  trimsv(const std::string& src)
{
    if( src.empty() || (!std::isspace(*src.begin()) && !std::isspace(*src.rbegin())) ) return std::string_view(src);

    size_t i{};
    size_t j = src.size()-1;

    while( i<=j && std::isspace(src[i]) ) i++;
    if( i>j ) return {};

    while( j>i && std::isspace(src[j]) ) j--;
    return std::string_view(src.data()+i, j-i+1);
}

std::string& trimr(std::string& src)
{
    src.erase(0, src.find_first_not_of(SPACE_CHARS));
    src.erase(src.find_last_not_of(SPACE_CHARS) + 1);

    return src;
}

std::string ltrims(const std::string& src)
{
    if( src.empty() || !std::isspace(*src.begin()) ) return src;

    size_t i{1};
    size_t j = src.size()-1;

    while( i<=j && std::isspace(src[i]) ) i++;

    return i>j? std::string() : src.substr(i);
}

std::string_view ltrimsv(const std::string& src)
{
    if( src.empty() || !std::isspace(*src.begin()) ) return std::string_view(src);

    size_t i{1};
    size_t j = src.size()-1;

    while( i<=j && std::isspace(src[i]) ) i++;

    return i>j? std::string_view() : std::string_view(src.data()+i, src.size()-i);
}

std::string rtrims (const std::string& src)
{
    if( src.empty() || !std::isspace(*src.rbegin()) ) return src;

    auto r = src.rbegin(); r++;
    while( r!=src.rend() && std::isspace(*r) ) r++;
    return r==src.rend()? std::string() : src.substr(0, src.size()-(r-src.rbegin()));
}

std::string_view  rtrimsv(const std::string& src)
{
    if( src.empty() || !std::isspace(*src.rbegin()) ) return std::string_view(src);

    auto r = src.rbegin(); r++;
    while( r!=src.rend() && std::isspace(*r) ) r++;
    return r==src.rend()? std::string_view() : std::string_view(src.data(), src.size()-(r-src.rbegin()));
}

std::string  lcase (const std::string& src)
{
    std::string dst;
    if( !src.empty() ) {
        dst.resize(src.size());
        std::transform(src.begin(), src.end(), dst.begin(), ::tolower);
    }
    return dst;
}

std::string  ucase (const std::string& src)
{
    std::string dst;
    if( !src.empty() ) {
        dst.resize(src.size());
        std::transform(src.begin(), src.end(), dst.begin(), ::toupper);
    }
    return dst;
}

std::string replall(const std::string& src, const std::string& sfind, const std::string& swith)
{
    if( src.empty() || sfind.empty() ) return src;

    size_t pos = src.find(sfind);
    if (nposs(pos)) return src;

    size_t nfrom{};
    std::string res;
    do {
        res += src.substr(nfrom, pos-nfrom)+swith;
        nfrom = pos+sfind.length();
        pos = src.find(sfind, nfrom);
    } while( !nposs(pos) );
    if( nfrom < src.size() ) res += src.substr(nfrom);
    return res;
}

std::string& replall(std::string& res, const std::string& src, const std::string& sfind, const std::string& swith)
{
    if( src.empty() || sfind.empty() ) {
        res.clear();
        return res;
    }

    size_t pos = src.find(sfind);
    if (nposs(pos)) {
        res = src;
        return res;
    }

    size_t nfrom{};
    res.clear();
    do {
        res += src.substr(nfrom, pos-nfrom)+swith;
        nfrom = pos+sfind.length();
        pos = src.find(sfind, nfrom);
    } while( !nposs(pos) );
    if( nfrom < src.size() ) res += src.substr(nfrom);
    return res;
}

//------------------------------------------------------------------------------------------------
#endif // QQ_STREXT_HPP
