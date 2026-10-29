//  FastCGI library extentions.
//
//  Require: fcgi-devel
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

#pragma once

#include <fcgio.h>
#include "httptools.hpp"
#include "strext.hpp"

namespace fcgiext {

using namespace std::string_literals;
using namespace std::string_view_literals;

//------------------------------------------------------------------------
// @note All string_view members are valid only within the lifetime of
//       the FCGX_Request passed to the constructor. Do NOT store rparser
//       beyond a single request cycle.
//
class rparser {		// Request parser and escape
private:
    constexpr static const char* QUERY_STRING   {"QUERY_STRING"};
    constexpr static const char* HTTP_COOKIE    {"HTTP_COOKIE"};
    constexpr static const char* REQUEST_SCHEME {"REQUEST_SCHEME"};
    constexpr static const char* SERVER_NAME    {"SERVER_NAME"};
    constexpr static const char* SERVER_PORT    {"SERVER_PORT"};
    constexpr static const char* REQUEST_URI    {"REQUEST_URI"};

    constexpr static std::string_view root_char{"/"sv};

    FCGX_Request& rq_;

protected:
    // REQUEST_URI解析出的包含最后一个'/'之前的部分；若为空，则返回"/"。
    // 例如：REQUEST_URI=/app-prj/fcgi/buy?uid=1008&nickname=%E5%B0%8F%E6%98%8E&goods=%5B%7B%22id%22%3A1%2C%22num%22%3A2%7D%5D
    // 返回：/app-prj/fcgi/
    // 第一个和最后一个字符一定是'/'。
    [[nodiscard]] std::string_view basepath_() const {
        auto u = left_of_from_tail(left_of(env(REQUEST_URI),'?',false),'/',true,true);
        return u.empty()? root_char : u;
    }

    // 剥离SERVER_NAME两端可能的IPv6的'['']'
    [[nodiscard]] std::string_view server_() const {
        auto s = env(SERVER_NAME);
        if (s.size() >= 2 && s.front() == '[' && s.back() == ']') {
            s.remove_prefix(1), s.remove_suffix(1);
        }
        return s;
    }

    // 由已初始化的4个部分组成本服务器标准URL，最后一个字符一定是'/'
    // 若前端使用了逆向代理配置了ProxyPassReverse，则302/301都可以被客户端浏览器正常访问
    [[nodiscard]] std::string baseurl_() const {
        bool is_def_port = (scheme == "http"sv && port == "80"sv) || (scheme == "https"sv && port == "443"sv);
        std::string t_server {server.find(':') == std::string::npos? std::string(server) : "["s+std::string(server)+"]"s};    // IPv6
#ifdef __cpp_lib_format
        std::string port_str {is_def_port? "" : std::format(":{}", port)};
        return std::format("{}://{}{}{}", scheme, t_server, port_str, basepath);
#else
        std::string bu(scheme); bu.append("://"); bu.append(t_server);
        if (!is_def_port) { bu.append(":"); bu.append(port); }
        bu.append(basepath);
        return bu;
#endif
    }

public:
    TStrMap query;       // 分解的QUERY_STRING，decode
    TStrMap cookie;      // 分解的HTTP_COOKIE, decode

    // These 4 declarations must be upper to "baseurl":
    const std::string_view scheme;
    const std::string_view server;
    const std::string_view port;
    const std::string_view basepath;

    const std::string_view docname;
    const std::string baseurl;

    [[nodiscard]] std::string_view env(const char* name) const {
        const char* v {FCGX_GetParam(name, rq_.envp)};
        return v? std::string_view(v) : std::string_view{};
    }

    explicit rparser(FCGX_Request& request):
                rq_(request),
                scheme(env(REQUEST_SCHEME)),
                server(server_()),
                port  (env(SERVER_PORT)),
                basepath(basepath_()),
                docname(right_of_from_tail(left_of(env(REQUEST_URI),'?',false), '/')),
                baseurl(baseurl_())
    {
        // Split QUERY_STRING safely:
        // QUERY_STRING=uid=1008&nickname=%E5%B0%8F%E6%98%8E&goods=%5B%7B%22id%22%3A1%2C%22num%22%3A2%7D%5D
        for( const auto& kv: splitsv(env(QUERY_STRING), '&', SplitOption::NOEMPTY) ) {  // 参数中可能会出现畸形两个连在一起的"&&"
            auto [k, v] = splitpairsv(kv, '=', false);
            query[urldecode(k)] = urldecode(v);
        }

        // Split HTTP_COOKIE:
        // 1. 分隔符：RFC 6265规定分隔符是"; "，但实际浏览器实现中也可能只发 ";"。
        // 2. 格式：key=value，无空格，不允许多余引号。
        // 3. 可能含有UNICODE或URL编码，此处做解码处理。
        // 4. 可能含有XSS攻击代码，此处不处理。
        // user_id=10086; token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9; sessionid=s987654321abcdef; theme=dark; remember_me=1; city=jinan; nickname=%E5%B0%8F%E6%98%8E; goods_cart=%5B%7B%22id%22%3A1%2C%22num%22%3A2%7D%5D; expires_time=1789023456
        for( const auto& kv : splitsv(env(HTTP_COOKIE), ';', SplitOption::TRIM | SplitOption::NOEMPTY) ) {
            auto [k, v] = splitpairsv(kv, '=', false);
            if( !k.empty() ) cookie[urldecode(k)] = urldecode(v);
        }
    }

    rparser()                          = delete;
    rparser(const rparser&)            = delete;
    rparser(rparser&&)                 = delete;
    rparser& operator=(const rparser&) = delete;
    rparser& operator=(rparser&&)      = delete;
};

//------------------------------------------------------------------------------------------------
} // namespace fcgiext
