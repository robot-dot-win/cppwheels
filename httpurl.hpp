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

#pragma once

#include "strext.hpp"
#include "httptools.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

//------------------------------------------------------------------------
// https://User:Pw@Example.COM:8443/a%20b/%E6%96%87%E4%BB%B6/page.php?name=%E4%BD%A0%E5%A5%BD&flag&r=%2F#frag
// {scheme}://{user}:{password}@{server}[:{port}]{basepath}{docname}?{query}#{frag}
//
enum class HttpSchm { NA, http, https };    // NA表示无"://"，从path开始

class THttpUrl {
public:
    HttpSchm scheme{HttpSchm::NA};
    std::string user;
    std::string password;
    std::string server;     // IPv6则剥离两端'['和']'
    uint16_t port{0};
    std::string basepath;
    std::string docname;
    std::string squery;
    std::string frag;

    void set(std::string_view url);     // 若无 "://"，认为从path开始

    THttpUrl(std::string_view url) { set(url); }

    THttpUrl& operator =(std::string_view url) { this->set(url); return *this; }    //  不抑制移动构造，但抑制了移动赋值

    THttpUrl(const THttpUrl&) = default;
    THttpUrl& operator=(const THttpUrl&) = default;
    THttpUrl(THttpUrl&&) noexcept = default;
    THttpUrl& operator=(THttpUrl&&) noexcept = default;

    //-------------------------------------------------------------------
    std::string baseurl() const   // docname之前的部分
    {
        if (scheme == HttpSchm::NA) return basepath;

        // 判断是否需要隐藏默认端口
        bool is_def_port = (scheme == HttpSchm::http && port == 80)   ||
                           (scheme == HttpSchm::https && port == 443) ||
                           (port == 0);

        std::string_view schm_sv = (scheme == HttpSchm::https) ? "https" : "http";
        std::string t_server {server.find(':') == std::string::npos? server : "["s+server+"]"s};

#ifdef __cpp_lib_format
        std::string auth = user;
        if (!password.empty()) auth += ":" + password;
        if (!auth.empty()) auth += "@";

        std::string port_str {is_def_port? "" : std::format(":{}", port)};
        return std::format("{}://{}{}{}{}", schm_sv, auth, t_server, port_str, basepath);
#else
        std::string res;
        res.reserve(32 + user.length() + password.length() + t_server.length() + basepath.length());

        res.append(schm_sv).append("://");
        if (!user.empty() || !password.empty()) {
            res.append(user);
            if (!password.empty()) {
                res.push_back(':');
                res.append(password);
            }
            res.push_back('@');
        }
        res.append(t_server);

        if (!is_def_port) {
            char buf[16];
            int n = std::snprintf(buf, sizeof(buf), ":%u", port);
            if (n > 0) res.append(buf, static_cast<std::size_t>(n));
        }
        res.append(basepath);

        return res;
#endif
    }

    //-------------------------------------------------------------------
    std::string fullurl() const
    {
        std::string res = baseurl();

#ifdef __cpp_lib_format
        std::string qs = squery.empty() ? "" : std::format("?{}", squery);
        std::string fs = frag.empty() ? "" : std::format("#{}", frag);
        return std::format("{}{}{}{}", res, docname, qs, fs);
#else
        res.reserve(res.length() + docname.length() + squery.length() + frag.length() + 2);

        res.append(docname);
        if (!squery.empty()) {
            res.push_back('?');
            res.append(squery);
        }
        if (!frag.empty()) {
            res.push_back('#');
            res.append(frag);
        }
        return res;
#endif
    }

    //-------------------------------------------------------------------
    // Although duplicate keys permitted by standard, we do NOT accept here.
    TStrMap query(bool decode) const
    {
        TStrMap res;
        for( const auto& kv: splitsv(squery, '&', SplitOption::NOEMPTY) ) {  // SplitOption::NOEMPTY in case of "&&"
            auto [k, v] = splitpairsv(kv, '=', false);
            if (decode) res[urldecode(k)]   = urldecode(v);
            else        res[std::string(k)] = std::string(v);
        }

        return res;
    }
};

//------------------------------------------------------------------------
