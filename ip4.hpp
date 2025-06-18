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

#pragma once

#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <compare>  // Since C++ 20
#include <bit>      // Since C++ 20

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
constexpr size_t ip4_byte_index(size_t n) { return 3 - n; }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
constexpr size_t ip4_byte_index(size_t n) { return n; }
#else
#error "Unsupported byte order"
#endif

//-------------------------------------------------------------------------------------------
class ip4 {
protected:
    union {
        uint32_t aa;
        uint8_t  ba_[4];
    };
public:
    ip4(uint32_t _aa = 0): aa(_aa) {}
    ip4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3): ba_{b0, b1, b2, b3} {
        if constexpr (ip4_byte_index(0) != 0) std::reverse(ba_, ba_ + 4);
    }

    ip4(std::string_view sv);
    ip4(const std::string& sa) { ip4(std::string_view(sa)); }
    ip4(const char* pa): ip4(pa ? std::string(pa) : throw std::invalid_argument("Null IPv4 string")) {}

          uint8_t& operator[](size_t n)       { return ba_[ip4_byte_index(n)]; }
    const uint8_t& operator[](size_t n) const { return ba_[ip4_byte_index(n)]; }

    [[nodiscard]] uint32_t get_aa() const noexcept { return aa; }

    [[nodiscard]] std::string sa() const {
        return std::to_string((*this)[0]) + "." +
               std::to_string((*this)[1]) + "." +
               std::to_string((*this)[2]) + "." +
               std::to_string((*this)[3]);
    }

    auto operator<=>(const ip4& other) const noexcept { return aa <=> other.aa; }   // Since C++ 20

    operator uint32_t() const noexcept { return aa; }

    ip4& operator ++()    noexcept { ++aa; return *this; }
    ip4& operator --()    noexcept { --aa; return *this; }
    ip4  operator ++(int) noexcept { return aa++; }
    ip4  operator --(int) noexcept { return aa--; }

    ip4& operator +=(int n) noexcept { aa+=n; return *this; }
    ip4& operator -=(int n) noexcept { aa-=n; return *this; }

    ip4 operator +(int n) const noexcept { return ip4(aa+n); }
    ip4 operator -(int n) const noexcept { return ip4(aa-n); }

    ip4  operator >>(uint8_t nbit) const noexcept { return (uint64_t)aa>>nbit; }
    ip4  operator <<(uint8_t nbit) const noexcept { return (uint64_t)aa<<nbit; }

    ip4& operator >>=(uint8_t nbit) noexcept { aa=(uint64_t)aa>>nbit; return *this; }
    ip4& operator <<=(uint8_t nbit) noexcept { aa=(uint64_t)aa<<nbit; return *this; }

    // Validate and assign ip4 value, ingoring exceptions
    [[nodiscard]] bool vali(const std::string& sa) noexcept {
        try { *this = ip4(sa); } catch(...) { return false; }
        return true;
    }
    [[nodiscard]] bool vali(std::string_view sv) noexcept {
        try { *this = ip4(sv); } catch(...) { return false; }
        return true;
    }
    [[nodiscard]] bool vali(const char* pa) noexcept {
        try { *this = pa; } catch(...) { return false; }
        return true;
    }
};

//-------------------------------------------------------------------------------------------
class ip4net {
protected:
    ip4 address_;
    uint8_t mask_;
public:
    ip4net(ip4 addr = {}, uint8_t mask = 32)
        : address_(addr), mask_(mask > 32 ? throw std::out_of_range("Invalid mask") : mask) {}

    // ips: "ip[/mask]", e.g. "192.168.0.6" "192.168.0.8/26" "192.168.0.8/255.255.255.192"
    //      When "/bits" is omitted, mask defaults to 32.
    explicit ip4net(std::string_view ipsv);
    explicit ip4net(const std::string& ips) { ip4net(std::string_view(ips)); }
    explicit ip4net(const char* ipc) { ipc? *this=ip4net(std::string(ipc)) : throw std::invalid_argument("NULL IPv4 net string."); }

    [[nodiscard]] uint8_t nmask() const { return mask_; }
    [[nodiscard]] ip4 addr()      const { return address_; }
    [[nodiscard]] ip4 imask()     const { return ip4(static_cast<uint32_t>((uint64_t)UINT32_MAX<<(32-mask_))); }  // shift count CANNOT >= width!
    [[nodiscard]] ip4 subnet()    const { return ip4(static_cast<uint32_t>((uint64_t)(address_.get_aa()) & ((uint64_t)UINT32_MAX<<(32-mask_)))); }
    [[nodiscard]] ip4 brdcast()   const { return ip4(static_cast<uint32_t>((uint64_t)(address_.get_aa()) | ((uint64_t)UINT32_MAX>>mask_))); }

    // Validate and assign ip4net value, ingoring exceptions
    [[nodiscard]] bool vali(ip4 ip, uint8_t msk) noexcept {
        try { *this = ip4net(ip,msk); } catch(...) { return false; }
        return true;
    }
    [[nodiscard]] bool vali(std::string_view ips) noexcept {
        try { *this = ip4net(ips); } catch(...) { return false; }
        return true;
    }
    [[nodiscard]] bool vali(const std::string& ips) noexcept {
        try { *this = ip4net(ips); } catch(...) { return false; }
        return true;
    }
};
//-------------------------------------------------------------------------------------------
