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
// Rebuild UUID from a std::string_view
uuid rebuild(std::string_view uustr)
{
    if (uustr.size() != 36)
        throw std::runtime_error("Invalid UUID string length");

    if (uustr[8] != '-' || uustr[13] != '-' || uustr[18] != '-' || uustr[23] != '-')
        throw std::runtime_error("Invalid UUID string format");

    uint64_t five_parts[5] {};
    size_t len[] { 8,4,4,4,12 };

    const char *p = uustr.data();
    for( int i=0; i<5; ++i, ++p ) {
        if( auto [ptr,ec] = std::from_chars(p, p+len[i], five_parts[i], 16); ec!=std::errc{} || ptr!=p+len[i] )
            throw std::runtime_error("Invalid hexadecimal characters in UUID string (part "+std::to_string(i+1)+")");
        p += len[i];
    }

    return { (five_parts[0] << 32) | (five_parts[1] << 16) | five_parts[2],
             (five_parts[3] << 48) |  five_parts[4] };
}
//------------------------------------------------------------------------
} // namespace ltuuid