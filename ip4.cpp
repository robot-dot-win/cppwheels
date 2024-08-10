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

#include <sstream>
#include "ip4.hpp"

using namespace std;

//---------------------------------------------------------------------------------------
ip4::ip4(const string& _sa)
{
    const invalid_argument e_bad_format("Bad IPv4 format.");
    const out_of_range     e_range_error("Invalid IPv4 byte range(0-255).");

    if( _sa.empty() || _sa.size()<7 ) throw e_bad_format;

    stringstream ss(_sa);
    size_t n;
    unsigned long ul;
    int nc;
    string s;

    for( nc=0; nc<4 && getline(ss,s,'.'); nc++ ) {
        try {
            ul = stoul(s, &n);  // may throw exception invalid_argument or range_error，but what()="stoul"
        } catch(...) {
            throw e_bad_format;
        }
        if( ul>255 )      throw e_range_error;
        if( n!=s.size() ) throw e_bad_format;    // invalid character in string
        __IP4_BA(nc)=(uint8_t)ul;
    }
    if( nc!=4 || getline(ss,s,'.') ) throw e_bad_format;  // less bytes or extra characters
}

//---------------------------------------------------------------------------------------
ip4net::ip4net(const string& ips)
{
    const invalid_argument e_bad_format("Invalid subnet mask string.");
    const out_of_range     e_invalid_mask_range("Invalid subnet mask range(0-32).");
    const out_of_range     e_mask_byte_error("Invalid subnet mask byte range(0-255).");

    size_t n;
    unsigned long ul;

    if( ips.empty() || ips.size()<7 ) throw e_bad_format;

    size_t pos_slash = ips.find_first_of('/');
    if( pos_slash == string::npos ) {
        taddr = ip4(ips);   // may throw exception
        tmask=32;
        return;
    }

    taddr = ip4(ips.substr(0,pos_slash));   // may throw exception

    if( ips.find_first_of('.',pos_slash+1) == string::npos ) {   // mask is like "/26"
        try {
            ul = stoul(ips.substr(pos_slash+1), &n);
        } catch(...) {
            throw e_bad_format;
        }
        if( ul>32 ) throw e_invalid_mask_range;
        if( n!=ips.size()-pos_slash-1 ) throw e_bad_format;    // invalid character in string
        tmask = (uint8_t)ul;
        return;
    }

    // mask is like "/255.255.255.192"
    ip4 m;
    try {
        m = ips.substr(pos_slash+1);
    }
    catch( const out_of_range& e ) {
        throw e_mask_byte_error;
    }
    catch(...) {
        throw e_bad_format;
    }

    // bits of mask must be continuous '1' from left to right:
    bool zero_found = false;
    tmask=0;
    for( uint32_t u=m.aa; u; u<<=1,tmask++ ) {
        if( zero_found ) throw e_bad_format;
        zero_found = !(u&0x80000000);
    }
}
