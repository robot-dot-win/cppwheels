//  Chrono library extentions.
//
//  Header-only file.
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

#if HAS_ZONED_TIME
    #include <format>
#else
    #include <iomanip>
    #include <ctime>
    #include <sstream>
#endif

using namespace std::chrono_literals;
namespace chrono = std::chrono;

using StdClkTP  = chrono::time_point<chrono::steady_clock>;
using StdClkDur = chrono::steady_clock::duration;

using SysClkTP  = chrono::time_point<chrono::system_clock>;
using SysClkDur = chrono::system_clock::duration;

enum class TimeUnit { Nano, Micro, Milli, Second, Minute, Hour, Day, Week, Month, Year };

inline StdClkDur time_dura(time_t value, TimeUnit unit)
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

template <typename T> inline StdClkDur time_dura(time_t value) noexcept { return T(value); }

inline auto stdnow() noexcept { return chrono::steady_clock::now(); }
inline auto sysnow() noexcept { return chrono::system_clock::now(); }
#if HAS_ZONED_TIME
inline auto locnow() { return chrono::zoned_time{chrono::current_zone(), sysnow()}; }
#else
inline auto locnow() { return chrono::system_clock::to_time_t(sysnow()); }
#endif

//------------------------------------------------------------------------------------------------
namespace detail {

// Convert time_t to local calendar time using the standard (non-thread-safe)
// std::localtime. A later revision of this library may switch to a re-entrant
// alternative once a higher C++ standard is adopted.
// Returns a zero-initialized tm on failure.
inline std::tm safe_localtime(std::time_t tt) noexcept
{
    std::tm tm{};
    if (const std::tm* p = std::localtime(&tt))
        tm = *p;
    return tm;
}

// Build a chrono/strftime format string into a caller-provided buffer.
// date_deli / dt_deli / time_deli of '\0' means "no separator".
// Returns the number of characters written (excluding the terminating NUL).
// Buffer must be large enough (32 bytes is ample for any combination).
constexpr std::size_t make_datetime_fmt(char* buf, const char date_deli, const char dt_deli, const char time_deli) noexcept
{
    char* p = buf;
    *p++ = '{'; *p++ = ':';               // chrono format prefix (ignored by strftime)
    *p++ = '%'; *p++ = 'Y';
    if (date_deli != '\0') *p++ = date_deli;
    *p++ = '%'; *p++ = 'm';
    if (date_deli != '\0') *p++ = date_deli;
    *p++ = '%'; *p++ = 'd';
    if (dt_deli   != '\0') *p++ = dt_deli;
    *p++ = '%'; *p++ = 'H';
    if (time_deli != '\0') *p++ = time_deli;
    *p++ = '%'; *p++ = 'M';
    if (time_deli != '\0') *p++ = time_deli;
    *p++ = '%'; *p++ = 'S';
    *p++ = '}';
    *p   = '\0';
    return static_cast<std::size_t>(p - buf);
}

constexpr std::size_t make_date_fmt(char* buf, const char deli) noexcept
{
    char* p = buf;
    *p++ = '{'; *p++ = ':';
    *p++ = '%'; *p++ = 'Y';
    if (deli != '\0') *p++ = deli;
    *p++ = '%'; *p++ = 'm';
    if (deli != '\0') *p++ = deli;
    *p++ = '%'; *p++ = 'd';
    *p++ = '}';
    *p   = '\0';
    return static_cast<std::size_t>(p - buf);
}

constexpr std::size_t make_time_fmt(char* buf, const char deli) noexcept
{
    char* p = buf;
    *p++ = '{'; *p++ = ':';
    *p++ = '%'; *p++ = 'H';
    if (deli != '\0') *p++ = deli;
    *p++ = '%'; *p++ = 'M';
    if (deli != '\0') *p++ = deli;
    *p++ = '%'; *p++ = 'S';
    *p++ = '}';
    *p   = '\0';
    return static_cast<std::size_t>(p - buf);
}

// Convert the chrono-style format "{:...}" into a classic strftime format
// by skipping the leading "{:" and replacing the trailing '}' with NUL.
// The buffer is modified in-place; the returned pointer is valid for the
// lifetime of the original buffer.
inline const char* to_strftime(char* chrono_fmt) noexcept
{
    // chrono_fmt points to "{:....}"
    char* p = chrono_fmt;
    while (*p && *p != '}') ++p;
    *p = '\0';                 // drop the trailing '}'
    return chrono_fmt + 2;     // skip "{:"
}

} // namespace detail

//------------------------------------------------------------------------------------------------
// Format a time_point as "YYYY-MM-DD HH:MM:SS" (or with custom separators).
// A delimiter of '\0' means the corresponding separator is omitted.
// When the delimiters are compile-time constants the format-string construction
// is trivial and is expected to be fully optimised by the compiler.
template <typename Clock, typename Duration>
inline std::string str_datetime(
    const std::chrono::time_point<Clock, Duration>& tp,
    const char date_deli = '-',
    const char dt_deli   = ' ',
    const char time_deli = ':') noexcept
{
    char fmt[32];
    detail::make_datetime_fmt(fmt, date_deli, dt_deli, time_deli);

#if HAS_ZONED_TIME
    const auto local = chrono::zoned_time{
        chrono::current_zone(),
        chrono::floor<chrono::seconds>(chrono::clock_cast<chrono::system_clock>(tp))
    }.get_local_time();
    return std::vformat(fmt, std::make_format_args(local));
#else
    static_assert(std::is_same_v<Clock, std::chrono::system_clock>,
                  "Only system_clock::time_point is supported when zoned_time is unavailable");
    const auto tt = Clock::to_time_t(tp);
    const auto tm = detail::safe_localtime(tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, detail::to_strftime(fmt));
    return oss.str();
#endif
}
//------------------------------------------------------------------------------------------------
// Format a time_point as "YYYY-MM-DD" (or with a custom separator).
// A delimiter of '\0' means no separator (e.g. "20251026").
template <typename Clock, typename Duration>
inline std::string str_date(
    const std::chrono::time_point<Clock, Duration>& tp,
    const char deli = '-') noexcept
{
    char fmt[16];
    detail::make_date_fmt(fmt, deli);

#if HAS_ZONED_TIME
    const auto local = chrono::zoned_time{
        chrono::current_zone(),
        chrono::floor<chrono::days>(chrono::clock_cast<chrono::system_clock>(tp))
    }.get_local_time();
    return std::vformat(fmt, std::make_format_args(local));
#else
    static_assert(std::is_same_v<Clock, std::chrono::system_clock>,
                  "Only system_clock::time_point is supported when zoned_time is unavailable");
    const auto tt = Clock::to_time_t(tp);
    const auto tm = detail::safe_localtime(tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, detail::to_strftime(fmt));
    return oss.str();
#endif
}
//------------------------------------------------------------------------------------------------
// Format a time_point as "HH:MM:SS" (or with a custom separator).
// A delimiter of '\0' means no separator (e.g. "143052").
template <typename Clock, typename Duration>
inline std::string str_time(
    const std::chrono::time_point<Clock, Duration>& tp,
    const char deli = ':') noexcept
{
    char fmt[16];
    detail::make_time_fmt(fmt, deli);

#if HAS_ZONED_TIME
    const auto local = chrono::zoned_time{
        chrono::current_zone(),
        chrono::floor<chrono::seconds>(chrono::clock_cast<chrono::system_clock>(tp))
    }.get_local_time();
    return std::vformat(fmt, std::make_format_args(local));
#else
    static_assert(std::is_same_v<Clock, std::chrono::system_clock>,
                  "Only system_clock::time_point is supported when zoned_time is unavailable");
    const auto tt = Clock::to_time_t(tp);
    const auto tm = detail::safe_localtime(tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, detail::to_strftime(fmt));
    return oss.str();
#endif
}

//------------------------------------------------------------------------------------------------
class TTimeout {
private:
    StdClkDur dur_{}; // declaration must before tp_ to ensure init sequence
    StdClkTP  tp_{};  // init value set to stdnow()-dur_, to ensure that first time calling expires() returns true

public:
    TTimeout(StdClkDur dur)
        : dur_(dur), tp_(stdnow()-dur_) {}

    TTimeout(time_t d, TimeUnit unit)
        : dur_(time_dura(d, unit)), tp_(stdnow()-dur_) {}

    template <typename T> TTimeout(time_t d)
        : dur_(time_dura<T>(d)), tp_(stdnow()-dur_) {}

    TTimeout& operator=(StdClkDur dur) {
        dur_ = dur;
        tp_  = stdnow() - dur_;
        return *this;
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
