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

extern const std::string EMPTY_STR;

#define sqlquoted(s) (std::quoted(s,'\'','\''))

#define lefts(s,n) ((s).substr(0,n))
#define leftsv(s,n) (std::string_view((s).data(),n))

inline std::string       rights (const std::string& src, size_t n=std::string::npos) { return n>=src.size()? src : src.substr(src.size()-n); }
inline std::string_view  rightsv(const std::string& src, size_t n=std::string::npos) { return n>=src.size()? std::string_view(src) : std::string_view(src.data()+src.size()-n,n); }

inline std::string       trims (const std::string& src);
inline std::string_view  trimsv(const std::string& src);
inline std::string&      trimr (      std::string& src);

inline std::string       ltrims (const std::string& src);
inline std::string_view  ltrimsv(const std::string& src);
inline std::string&      ltrimr (      std::string& src);

inline std::string       rtrims (const std::string& src);
inline std::string_view  rtrimsv(const std::string& src);
inline std::string&      rtrimr (      std::string& src);

inline std::string  lcase (const std::string& src);
inline std::string  ucase (const std::string& src);

inline std::string& lcaser(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::tolower); return src; }
inline std::string& ucaser(std::string& src) { std::transform(src.begin(), src.end(), src.begin(), ::toupper); return src; }

template <class T> class  spliti;
template <class T> inline std::vector<std::string>&      splits (std::vector<std::string>&      dst, const std::string& src, T delimiters);
template <class T> inline std::vector<std::string_view>& splitsv(std::vector<std::string_view>& dst, const std::string& src, T delimiters);

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
        } while(nfind != std::string::npos);
        *idx.rbegin() = (*ps_).size();
        return *this;
    }

    size_t size() const { return idx.size(); }

    // may throw exeption if ps_ or n is invalid:
    std::string      s (size_t n) const { return n==0? (*ps_).substr(0,idx[0]) : (*ps_).substr(idx[n-1]+1,idx[n]-idx[n-1]-1); }
    std::string_view sv(size_t n) const { return n==0? std::string_view((*ps_).data(),idx[0]) : std::string_view((*ps_).data()+idx[n-1]+1,idx[n]-idx[n-1]-1); }
};

//------------------------------------------------------------------------------------------------
template <class T>
std::vector<std::string>& splits(std::vector<std::string>& dst, const std::string& src, T delimiters)
{
	size_t nfrom, nfind;
	dst.clear();
	if( !src.empty() ) {
        for( nfrom=0; (nfind = src.find_first_of(delimiters, nfrom)) != std::string::npos; nfrom=nfind+1 )
            dst.emplace_back(src.substr(nfrom, nfind-nfrom));
        dst.emplace_back(src.substr(nfrom, src.size()-nfrom));
    }
    return dst;
}

//------------------------------------------------------------------------------------------------
template <class T>
std::vector<std::string_view>& splitsv(std::vector<std::string_view>& dst, const std::string& src, T delimiters)
{
	size_t nfrom, nfind;
	dst.clear();
	if( !src.empty() ) {
        for( nfrom=0; (nfind = src.find_first_of(delimiters, nfrom)) != std::string::npos; nfrom=nfind+1 )
            dst.emplace_back(src.data()+nfrom, nfind-nfrom);
        dst.emplace_back(src.data()+nfrom, src.size()-nfrom);
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
    if( src.empty() || (!std::isspace(*src.begin()) && !std::isspace(*src.rbegin())) ) return src;

    // Erase tail spaces firstly:
    auto r = src.rbegin();
    while( r!=src.rend() && std::isspace(*r) ) r++;
    if( r==src.rend() ) { src.clear(); return src; }
    if( r!=src.rbegin() ) src.erase(src.size()-(r-src.rbegin()));

    // Erase leading spaces then:
    size_t i{};
    size_t j = src.size()-1;
    while( i<j && std::isspace(src[i]) ) i++;
    if( i>0 ) src.erase(0, i);

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

std::string& ltrimr(std::string& src)
{
    if( src.empty() || !std::isspace(*src.begin()) ) return src;

    size_t i{1};
    size_t j = src.size()-1;

    while( i<=j && std::isspace(src[i]) ) i++;

    if( i>j ) src.clear(); else src.erase(0,i);
    return src;
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

std::string& rtrimr (std::string& src)
{
    if( src.empty() || !std::isspace(*src.rbegin()) ) return src;

    auto r = src.rbegin(); r++;
    while( r!=src.rend() && std::isspace(*r) ) r++;
    if( r==src.rend() ) src.clear(); else src.erase(src.size()-(r-src.rbegin()));
    return src;
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
    if (pos == std::string::npos) return src;

    size_t nfrom=0;
    std::string res;
    do {
        res += src.substr(nfrom, pos-nfrom)+swith;
        nfrom = pos+sfind.length();
        pos = src.find(sfind, nfrom);
    } while( pos != std::string::npos );
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
    if (pos == std::string::npos) {
        res = src;
        return res;
    }

    size_t nfrom=0;
    res.clear();
    do {
        res += src.substr(nfrom, pos-nfrom)+swith;
        nfrom = pos+sfind.length();
        pos = src.find(sfind, nfrom);
    } while( pos != std::string::npos );
    if( nfrom < src.size() ) res += src.substr(nfrom);
    return res;
}

//------------------------------------------------------------------------------------------------
#endif // QQ_STREXT_HPP
