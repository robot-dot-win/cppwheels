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

enum class TimeUnit { Nano, Micro, Milli, Second, Minute, Hour, Day };

inline auto stdnow() { return chrono::steady_clock::now(); }
inline auto sysnow() { return chrono::system_clock::now(); }

//------------------------------------------------------------------------------------------------
// To manage named deadlines
class DeadlineManager {
private:
    struct Deadline {
        StdClkDur dur;
        StdClkTP  tp;
    };

    std::map<std::string, Deadline> deadlines_;
    mutable std::mutex mutex_;

    static StdClkDur convert_to_duration(long long value, TimeUnit unit)
    {
        switch (unit) {
        case TimeUnit::Nano:   return chrono::nanoseconds(value);
        case TimeUnit::Micro:  return chrono::microseconds(value);
        case TimeUnit::Milli:  return chrono::milliseconds(value);
        case TimeUnit::Second: return chrono::seconds(value);
        case TimeUnit::Minute: return chrono::minutes(value);
        case TimeUnit::Hour:   return chrono::hours(value);
        case TimeUnit::Day:    return chrono::days(value);      // Since C++ 20
        default: throw std::invalid_argument("Unsupported time unit");
        }
    }

public:
    void set(const std::string& name, long long dur, TimeUnit unit) {
        std::lock_guard lock(mutex_);
        const auto duration = convert_to_duration(dur, unit);
        deadlines_.insert_or_assign(
            name,
            Deadline{duration, stdnow()-duration}
        );
    }

    bool exists(const std::string& name) const {
        std::lock_guard lock(mutex_);
        return deadlines_.contains(name);
    }

    bool expires(const std::string& name) {
        std::lock_guard lock(mutex_);
        auto it = deadlines_.find(name);
        if (it == deadlines_.end())
            throw std::runtime_error("Deadline not found");

        auto& deadline = it->second;
        const auto now = stdnow();

        if ((now - deadline.tp) >= deadline.dur) {
            deadline.tp = now;
            return true;
        }
        return false;
    }

    void remove(const std::string& name) {
        std::lock_guard lock(mutex_);
        deadlines_.erase(name);
    }
};
//------------------------------------------------------------------------------------------------
