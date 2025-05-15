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

#include <concepts>
#include <chrono>
#include <map>
#include <string>
#include <stdexcept>

#if defined(__cpp_lib_chrono) && (__cpp_lib_chrono >= 201907L)
#define HAS_ZONED_TIME 1
#else
#define HAS_ZONED_TIME 0
#endif

#if !HAS_ZONED_TIME
#include <iomanip>
#include <ctime>
#endif

using namespace std::chrono_literals;
namespace chrono = std::chrono;

using StdClkTP  = chrono::time_point<chrono::steady_clock>;
using StdClkDur = chrono::steady_clock::duration;

using SysClkTP  = chrono::time_point<chrono::system_clock>;
using SysClkDur = chrono::system_clock::duration;

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

template <typename T> inline StdClkDur time_dure(long long value) noexcept { return T(value); }

inline auto stdnow() noexcept { return chrono::steady_clock::now(); }
inline auto sysnow() noexcept { return chrono::system_clock::now(); }
#if HAS_ZONED_TIME
inline auto locnow() { return chrono::zoned_time{chrono::current_zone(), sysnow()}; }
#else
inline auto locnow() { return chrono::system_clock::to_time_t(sysnow()); }
#endif

enum class DateChar { dash, dot };

//------------------------------------------------------------------------------------------------
template <typename Clock, typename Duration>
inline std::string str_datetime(const std::chrono::time_point<Clock, Duration>& tp, DateChar deli=DateChar::dash) noexcept
{
    #if HAS_ZONED_TIME
        const auto local_time = chrono::zoned_time{chrono::current_zone(), chrono::floor<chrono::seconds>(chrono::clock_cast<chrono::system_clock>(tp))}.get_local_time();
        return deli==DateChar::dash? std::format("{:%Y-%m-%d %H:%M:%S}",local_time) : std::format("{:%Y.%m.%d %H:%M:%S}",local_time);
    #else
        static_assert(
            std::is_same_v<Clock, std::chrono::system_clock>,
            "Only support system_clock::time_point parameter"
        );
        const auto tt = Clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&tt), deli==DateChar::dash? "%Y-%m-%d %H:%M:%S" : "%Y.%m.%d %H:%M:%S");
        return oss.str();
    #endif
}

//------------------------------------------------------------------------------------------------
template <typename Clock, typename Duration>
inline std::string str_date(const std::chrono::time_point<Clock, Duration>& tp, DateChar deli=DateChar::dash) noexcept
{
    #if HAS_ZONED_TIME
        const auto local_time = chrono::zoned_time{chrono::current_zone(), chrono::floor<chrono::days>(chrono::clock_cast<chrono::system_clock>(tp))}.get_local_time();
        return deli==DateChar::dash? std::format("{:%Y-%m-%d}",local_time) : std::format("{:%Y.%m.%d}",local_time);
    #else
        static_assert(
            std::is_same_v<Clock, std::chrono::system_clock>,
            "Only support system_clock::time_point parameter"
        );
        const auto tt = Clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&tt), deli==DateChar::dash? "%Y-%m-%d" : "%Y.%m.%d");
        return oss.str();
    #endif
}

//------------------------------------------------------------------------------------------------
class TTimeout {
private:
    StdClkDur dur_{};
    StdClkTP  tp_{};

public:
    TTimeout(long long d, TimeUnit unit) { init(d,unit); }
    template <typename T> TTimeout(long long d) { init<T>(d); }

    void init(long long d, TimeUnit unit) {
        dur_ = time_dure(d, unit);
        tp_ = stdnow()-dur_;  // to ensure that first time calling expires() returns true
    }

    template <typename T> void init(long long d) {
        dur_= time_dure<T>(d);
        tp_ = stdnow()-dur_;
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
