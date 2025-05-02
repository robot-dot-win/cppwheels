//  Lightweight UUID generator.
//  Only support UUID v4 conforming to RFC 4122.
//
//  Copyright (C) 2025, Martin Young <martin_young@live.cn>
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

#include "ltuuid.hpp"

namespace ltuuid {

//------------------------------------------------------------------------
// String conversion implementation
std::string uuid::str() const
{
    constexpr auto hex = "0123456789abcdef";
    std::array<char, 36> buf{};

    auto fill = [&](auto val, size_t start, size_t len) {
        for (size_t i = 0; i < len; ++i)
            buf[start + i] = hex[(val >> (4 * (len - 1 - i))) & 0xF];
    };

    fill(ab >> 32, 0, 8);            buf[8]  = '-';
    fill((ab >> 16) & 0xFFFF, 9, 4); buf[13] = '-';
    fill(ab & 0xFFFF, 14, 4);        buf[18] = '-';
    fill(cd >> 48, 19, 4);           buf[23] = '-';
    fill(cd & 0xFFFFFFFFFFFF, 24, 12);

    return std::string(buf.data(), buf.size());
}
//------------------------------------------------------------------------
// Rebuild UUID from a std::string
uuid rebuild(std::string_view uustr)
{
    if (uustr.size() != 36)
        throw std::runtime_error("Invalid UUID std::string length");

    if (uustr[8] != '-' || uustr[13] != '-' || uustr[18] != '-' || uustr[23] != '-')
        throw std::runtime_error("Invalid UUID std::string format");

    uint64_t parts[][2] { {8,0},{4,0},{4,0},{4,0},{12,0} };   // [length][value]

    const char *p = uustr.data();
    for( int i=0; i<5; ++i, ++p ) {
        if( auto res = std::from_chars(p, p+parts[i][0], parts[i][1], 16); res.ec!=std::errc{} || res.ptr!=p+parts[i][0] )
            throw std::runtime_error("Invalid hexadecimal characters in UUID string (part "+std::to_string(i+1)+")");
        p += parts[i][0];
    }

    return { (parts[0][1] << 32) | (parts[1][1] << 16) | parts[2][1],
             (parts[3][1] << 48) | parts[4][1] };
}
//------------------------------------------------------------------------
} // namespace ltuuid