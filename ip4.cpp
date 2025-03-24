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

#include <charconv>
#include <string_view>
#include "ip4.hpp"

//-------------------------------------------------------------------------------------------
ip4::ip4(const std::string& sa) {
    std::string_view sv(sa);
    size_t start{};
    int idx{};

    for (; idx < 4; ++idx) {
        size_t end {sv.find('.', start)};

        if (idx == 3)
            end = sv.size();
        else if (end == std::string_view::npos)
            throw std::invalid_argument("Bad IPv4 format");

        auto part {sv.substr(start, end - start)};
        uint32_t byte;
        auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), byte);

        if (ec != std::errc() || ptr != part.data() + part.size() || byte > 255)
            throw std::invalid_argument("Invalid IPv4 component");

        (*this)[idx] = static_cast<uint8_t>(byte);
        start = end + 1;
    }
    if (idx != 4 || start - 1 != sv.size())
        throw std::invalid_argument("Extra characters in IPv4");
}

//-------------------------------------------------------------------------------------------
ip4net::ip4net(const std::string& ips) {
    size_t slash {ips.find('/')};
    address_ = ip4(ips.substr(0, slash));  // may throw exception

    if (slash == std::string::npos) {
        mask_ = 32;
        return;
    }

    std::string_view mask_sv(ips.substr(slash + 1));
    if (mask_sv.find('.') != std::string_view::npos) {
        ip4 mask(std::string{mask_sv});  // may throw exception
       // bits of mask must be continuous '1' from left to right:
        auto continuous {std::countl_one(mask.get_aa())};   // type: int, range: [0,32]
        if (continuous != std::popcount(mask.get_aa()))
            throw std::invalid_argument("Invalid IPv4 mask pattern");
        mask_ = static_cast<uint8_t>(continuous);
    } else {
        uint32_t bits;
        auto [ptr, ec] = std::from_chars(mask_sv.data(), mask_sv.data() + mask_sv.size(), bits);
        if (ec != std::errc() || bits > 32)
            throw std::invalid_argument("Invalid IPv4 mask bits");
        mask_ = bits;
    }
}
//-------------------------------------------------------------------------------------------
