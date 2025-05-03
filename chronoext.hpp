//  Chrono library extentions.
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

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <mutex>
#include <stdexcept>

namespace chrono = std::chrono;

using StdClkTP  = chrono::time_point<chrono::steady_clock>;
using StdClkDur = chrono::steady_clock::duration;

enum class TimeUnit { Nano, Micro, Milli, Second, Minute, Hour, Day, Week, Month, Year };

inline StdClkDur time_dure(long long value, TimeUnit unit)
{
    switch (unit) {
    case TimeUnit::Nano:   return chrono::nanoseconds(value);
    case TimeUnit::Micro:  return chrono::microseconds(value);
    case TimeUnit::Milli:  return chrono::milliseconds(value);
    case TimeUnit::Second: return chrono::seconds(value);
    case TimeUnit::Minute: return chrono::minutes(value);
    case TimeUnit::Hour:   return chrono::hours(value);
    case TimeUnit::Day:    return chrono::days(value);      // Since C++ 20
    case TimeUnit::Week:   return chrono::weeks(value);     // Since C++ 20
    case TimeUnit::Month:  return chrono::months(value);    // Since C++ 20
    case TimeUnit::Year:   return chrono::years(value);     // Since C++ 20
    default:
        throw std::invalid_argument("Unsupported time unit");
    }
}

template <typename T> inline StdClkDur time_dure(long long value) { return T(value); }

inline auto stdnow() { return chrono::steady_clock::now(); }
inline auto sysnow() { return chrono::system_clock::now(); }

//------------------------------------------------------------------------------------------------
class TTimeout {
private:
    StdClkDur dur_{};
    StdClkTP  tp_{};

public:
    TTimeout(long long d, TimeUnit unit) { init(d,unit); }

    void init(long long d, TimeUnit unit) {
        dur_ = time_dure(d, unit);
        tp_ = stdnow()-dur_;  // to ensure that first time calling expires() returns true
    }

    bool expires() noexcept { return stdnow()-tp_ >= dur_; }
    void reset()   noexcept { tp_ = stdnow(); }

    TTimeout(const TTimeout&) = default;
    TTimeout& operator=(const TTimeout&) = default;
    TTimeout() = default;
    ~TTimeout() = default;
};

using TimeoutManager = std::map<std::string, TTimeout>;
//------------------------------------------------------------------------------------------------
