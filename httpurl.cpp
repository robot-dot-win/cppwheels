//  Manipulate HTTP URL string.
//
//  Copyright (C) 2026, Martin Young <martin_young@live.cn>
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

#include <limits>
#include "httpurl.hpp"

using namespace std;

//------------------------------------------------------------------------
void THttpUrl::set(std::string_view url)
{
    // 初始化清理状态
    scheme = HttpSchm::NA;
    user.clear(); password.clear(); server.clear(); port = 0;
    basepath.clear(); docname.clear(); squery.clear(); frag.clear();

    if (url.empty()) return;

    // 提取并剥离 Fragment
    std::tie(url,frag) = splitpairsv(url,'#',false);

    // 提取并剥离 Query
    {
        std::string_view qr;
        std::tie(url,qr) = splitpairsv(url,'?',false);
        squery = string(qr);
    }

    // 提取 Scheme 和 Authority
    if (auto f = url.find("://"); f != std::string_view::npos) {
        auto schm = lcases(url.substr(0,f));
        if( schm == "https") {
            scheme = HttpSchm::https;
            port = 443;        // 先设置默认端口，后面解析不到端口就是这个值
        } else {    // 默认http
            scheme = HttpSchm::http;
            port = 80;        // 先设置默认端口，后面解析不到端口就是这个值
        }
        url = url.substr(f+3);   // 跳过 "://"

        // 剥离 Auth
        std::string_view auth;
        if (auto ff = url.find('/'); ff != std::string_view::npos) {
            auth = url.substr(0, ff);
            url  = url.substr(ff);
        } else {
            auth = url;
            url = "/";
        }

        // 解析 Auth: [user[:password]@]server[:port]
        auto at_pos = auth.rfind('@');  // in case of '@' in password
        if (at_pos != std::string_view::npos) {
            auto u_p = auth.substr(0, at_pos);
            auto [u,p] = splitpairsv(u_p,':',false);
            user = string(u); password = string(p);

            auth = auth.substr(at_pos + 1);
        }

        // 解析 server[:port] 注意：server可能是IPv6
        auto colon_pos = auth.rfind(':');
        auto bracket_pos = auth.rfind(']');
        // 判断条件：只有当 ':' 存在，且它位于 ']' 之后（或者完全没有 ']' 时），它才是端口分隔符
        if (colon_pos != std::string_view::npos && (bracket_pos == std::string_view::npos || colon_pos > bracket_pos)) {
            server = auth.substr(0, colon_pos);
            port = str2int<decltype(port)>(auth.substr(colon_pos+1), 1, numeric_limits<decltype(port)>::max()).value_or(scheme==HttpSchm::http? 80:443);
        } else  // 无端口，整个 auth 都是 server
            server = auth;

        // 剥离IPv6两端的方括号字符
        if (server.size() >= 2 && server.front() == '[' && server.back() == ']') {
            server.pop_back();
            server.erase(0, 1);
        }

        // server小写
        lcaserf(server);
    }

    // 切割 basepath 与 docname
    // 如果指定了 Scheme (合法绝对URL)，且路径为空，强制修正为 HTTP 默认的 "/"
    if (scheme != HttpSchm::NA && url.empty()) url = "/";

    if( auto slash_pos = url.rfind('/'); slash_pos != std::string_view::npos) {
        basepath = url.substr(0, slash_pos + 1);
        docname  = url.substr(slash_pos + 1);
    } else {    // 相对URL，从docname开始
        basepath = {};
        docname = url;
    }
}

//------------------------------------------------------------------------
