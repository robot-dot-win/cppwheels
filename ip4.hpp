//  Classes related to ipv4.
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

#ifndef QQ_IP4_HPP
#define QQ_IP4_HPP

#include <string>
#include <stdexcept>
#include <cstdint>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __IP4_BA(n) (ba_[3-n])
#elif __BYTE_ORDER__== __ORDER_BIG_ENDIAN__
#define __IP4_BA(n) (ba_[n])
#elif __BYTE_ORDER__== __ORDER_BIG_WORD_ENDIAN__
#define __IP4_BA(n) (ba_[n<2? n+2 : n-2])
#else   // __ORDER_LITTLE_WORD_ENDIAN__
#define __IP4_BA(n) (ba_[n==0||n==2? n+1 : n-1])
#endif

//-------------------------------------------------------------------------------------------
union ip4 {
    uint32_t aa;
    uint8_t  ba_[4];    // Don't access it directly. Use __IP4_BA(n) macro or ba(n) function
public:
    ip4(uint32_t _aa=0): aa(_aa) {}
    ip4(uint8_t ba0, uint8_t ba1, uint8_t ba2, uint8_t ba3) { __IP4_BA(0)=ba0,__IP4_BA(1)=ba1,__IP4_BA(2)=ba2,__IP4_BA(3)=ba3; }

    // Leading whitespace characters of every "byte" will be ignored, e.g. "  1. 33.0.  8"
    ip4(const std::string& _sa);
    ip4(const char* _pa): aa(_pa? ip4(std::string(_pa)).aa : throw std::invalid_argument("NULL IPv4 string.")) {}

    uint8_t& ba(int n) { return __IP4_BA(n); }

    std::string ia() const { return std::to_string(aa); }
    std::string sa() const { return std::to_string(__IP4_BA(0))+"."+std::to_string(__IP4_BA(1))+"."+std::to_string(__IP4_BA(2))+"."+std::to_string(__IP4_BA(3)); }

    bool operator==(const ip4& other) const { return aa==other.aa; }
    bool operator!=(const ip4& other) const { return aa!=other.aa; }
    bool operator>=(const ip4& other) const { return aa>=other.aa; }
    bool operator<=(const ip4& other) const { return aa<=other.aa; }
    bool operator >(const ip4& other) const { return aa >other.aa; }
    bool operator <(const ip4& other) const { return aa <other.aa; }

    // Validate and assign ip4 value, ingoring exceptions
    bool vali(const std::string& _sa) {
        try { *this = _sa; } catch(...) { return false; }
        return true;
    }
    bool vali(const char* _pa) {
        try { *this = _pa; } catch(...) { return false; }
        return true;
    }
};

//-------------------------------------------------------------------------------------------
class ip4net {
protected:
    ip4 taddr;
    uint8_t tmask;
public:
    ip4net(ip4 ip=(uint32_t)0, uint8_t msk=32): taddr(ip),tmask(msk>32? throw std::out_of_range("Invalid subnet mask range(0-32).") : msk) {}

    // ips: "ip[/mask]", e.g. "192.168.0.6" "192.168.0.8/26" "192.168.0.8/255.255.255.192"
    //      When "/bits" is omitted, mask defaults to 32.
    //      Whitespace characters between "/" and "mask" will be ignored, e.g. "192.168. 0.  8/      26"
    ip4net(const std::string& ips);
    ip4net(const char* ipc) { ipc? *this=ip4net(std::string(ipc)) : throw std::invalid_argument("NULL IPv4 net string."); }

    uint8_t nmask() const { return tmask; }
    ip4 addr()    const { return taddr; }
    ip4 imask()   const { return ip4((uint64_t)UINT32_MAX<<(32-tmask)); }  // shift count CANNOT >= width!
    ip4 subnet()  const { return ip4(taddr.aa & ((uint64_t)UINT32_MAX<<(32-tmask))); }
    ip4 brdcast() const { return ip4(taddr.aa | ((uint64_t)UINT32_MAX>>tmask)); }

    // Validate and assign ip4net value, ingoring exceptions
    bool vali(ip4 ip, uint8_t msk) {
        try { *this = ip4net(ip,msk); } catch(...) { return false; }
        return true;
    }
    bool vali(const std::string& ips) {
        try { *this = ip4net(ips); } catch(...) { return false; }
        return true;
    }
};

#endif // QQ_IP4_HPP
