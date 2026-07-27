//  Classes to manipulate configuration(.cfg/.conf/.ini/etc.) files.
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

//Configuration File Syntax
//=====================================
//
//Syntax Description
//---------------------------
//Configuration files are plain text files that consist of a number of sections, each
//containing zero or more options:
//
//# Comment ...
//
//name_in_empty_section = value 1
//
//[section 1]
//
//[section 2]
//name 1 = value 1   # Comment ...
//name 2 = value 2
//name 3 = Value_contains_#_character   # Must followed by a comment at the line
//name 4 =           # Empty value
//name 5             # Empty value
//       = Value_of_empty_key_name
//
//Ignored Matter
//---------------------------
//.Empty lines and lines that start with a '#' in the first non-space column are ignored.
//.From the most right character '#' to the end of the line are ignored.
//
//Sections
//---------------------------
//A section starts with a section header. The section header starts with a left bracket
//'[', any spaces before which are ignored. All charaters between the opening bracket and
// the first right bracket ']' in the line form the section name. Any text between that
// right bracket and the end of the line is ignored.
//
//Section names are case-sensitive. Empty-named sections are permitted.
//
//An invalid section header(without right bracket ']') is considered as an empty-named
//section(same as "[]").
//
//If the name contains '#' characters，you have to add a comment at the line.
//
//Reopening Sections
//---------------------------
//A section is implicitly closed when another section header is encountered. However,
//the section may be reopened by repeating its section header, and new options can be
//defined or existing options can be redefined.
//
//Options
//---------------------------
//An option is a mapping between a name and a value. Options before the first section
//are considered in the empty-named section. Options in different sections are considered
//separate even if they have the same name.
//
//The way to define an option:
//
//name = value
//
//. The name cannot start with '[' or '#', those characters start a section header or a
//  comment;
//. The name may not contain the separator '=' or the comment mark '#'.
//. Like section names, an option name may contain spaces within it, but all leading and
//  trailing spaces are removed.
//. If '=' is omitted, value is an empty string.
//. Option names are case-sensitive and can be empty(omitted).
//
//The options's value is all the text from the separator character until the end of the
//line or the most right character '#', which starts a comment at the line. Leading and
//trailing spaces around the option value are removed, but spaces within the value are
//preserved.
//
//So if the value contains '#' characters，you have to add a comment at the line.
//-------------------------------------------------------------------------------------------

#pragma once

#include "strext.hpp"

using TCfgMap = std::map<std::string, TStrMap, std::less<>>;

class cfgfile {
public:
    using section_iterator = TCfgMap::const_iterator;
    using key_iterator     = TStrMap::const_iterator;
    using section_range    = std::ranges::subrange<section_iterator>;
    using key_range        = std::ranges::subrange<key_iterator>;

    // Xstructors:
    cfgfile() = default;
    ~cfgfile() {
        if (modified_ && !readonly_) {
            try { save(); } catch (...) {}
        }
    }

    cfgfile(const cfgfile&)            = delete;
    cfgfile& operator=(const cfgfile&) = delete;
    cfgfile(cfgfile&&)                 = delete;
    cfgfile& operator=(cfgfile&&)      = delete;

    // File operations (exception thrown):
    void open(std::string path, bool readonly = true, char sep = '=');
    void save();

    // "read" operations:
    [[nodiscard]]
    std::string getStr(std::string_view section, std::string_view key, std::string_view def = {}) const
    {
        if (auto sec_it = cfg_.find(section); sec_it != cfg_.end())
            if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
                return key_it->second;

        return std::string(def);
    }

    template<std::integral T = int>
    [[nodiscard]]
    T getInt(std::string_view section, std::string_view key, T def = T{}) const
    {
        auto s = getStr(section, key);
        return s.empty()? def : str2int<T>(s).value_or(def);
    }

    [[nodiscard]]
    bool getBool(std::string_view section, std::string_view key, bool def = false) const
    {
        auto s = getStr(section, key);
        return s.empty()? def : (lcaserf(s) == "yes" || s == "true" || s == "on");
    }

    [[nodiscard]]
    std::vector<std::string> getStrList(std::string_view section, std::string_view key, char delimiter = ',', SplitOptions spo=SplitOption::TRIM) const
    {
        std::vector<std::string> result;
        auto s = getStr(section, key);
        if( !s.empty() ) splits(result, s, delimiter, spo);
        return result;
    }

    template<std::integral T = int>
    [[nodiscard]]
    std::vector<T> getIntList(std::string_view section, std::string_view key, T error_value={}, char delimiter = ',') const
    {
        std::vector<T> result;
        auto strlist = getStrList(section, key, delimiter);
        result.reserve(strlist.size());
        for (const auto& s : strlist)  result.push_back(str2int<T>(s).value_or(error_value));
        return result;
    }

    // "modify" operations (std::logic_error thrown when readonly)
    void setKey(std::string_view section,
                std::string_view key,
                std::string_view value)
    {
        check_writable();
        cfg_[std::string(section)][std::string(key)] = std::string(value);
        modified_ = true;
    }

    void delSec(std::string_view section)
    {
        check_writable();

#if defined(__cpp_lib_associative_heterogeneous_erasure) && __cpp_lib_associative_heterogeneous_erasure >= 202110L
        // Since C++23: erase(K&&) (P2077R3)
        if (cfg_.erase(section) > 0)  modified_ = true;
#else
        if (auto it = cfg_.find(section); it != cfg_.end()) {
            cfg_.erase(it);
            modified_ = true;
        }
#endif
    }

    void delKey(std::string_view section, std::string_view key)
    {
        check_writable();

        if (auto it = cfg_.find(section); it != cfg_.end()) {
#if defined(__cpp_lib_associative_heterogeneous_erasure) && __cpp_lib_associative_heterogeneous_erasure >= 202110L
            // Since C++23: erase(K&&) (P2077R3)
            if (it->second.erase(key) > 0)  modified_ = true;
#else
            if (auto kit = it->second.find(key); kit != it->second.end()) {
                it->second.erase(kit);
                modified_ = true;
            }
#endif
        }
    }

    // Query:
    [[nodiscard]]
    bool SecExists(std::string_view section) const
    { return cfg_.find(section) != cfg_.end(); }

    [[nodiscard]]
    bool KeyExists(std::string_view section, std::string_view key) const
    {
        if (auto sec_it = cfg_.find(section); sec_it != cfg_.end())
            return sec_it->second.find(key) != sec_it->second.end();
        return false;
    }

    // ranges (read-only):
    [[nodiscard]]
    section_range sections() const noexcept {
        return std::ranges::subrange(cfg_.cbegin(), cfg_.cend());
    }

    [[nodiscard]]
    key_range keys(std::string_view section) const noexcept {
        if (auto it = cfg_.find(section); it != cfg_.end()) {
            return std::ranges::subrange(it->second.cbegin(), it->second.cend());
        }
        static const TStrMap empty;
        return std::ranges::subrange(empty.cbegin(), empty.cend());
    }

private:
    static const char mark_{'#'};

    TCfgMap     cfg_;
    std::string srcfile_;
    bool        readonly_  = true;
    bool        modified_  = false;
    char        separator_ = '=';

    void parse_stream(std::istream& is);
    void write_stream(std::ostream& os) const;

    void check_writable() const
    { if (readonly_) throw std::logic_error("cfgfile: instance is read-only"); }
};
