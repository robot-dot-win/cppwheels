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

#pragma once

#include <string>
#include <map>

enum class cfgopenmode {r, rw};

//-------------------------------------------------------------------------------------------
class cfgfile {     // Only for single thread usage
protected:
    const std::string srcfile;
    const bool readonly;
    const char separator;
public:
    std::string errmsg;
    std::map<std::string, std::map<std::string, std::string>> cfg;

    // After initializing an instance, check errmsg to judge if error occurs.
    cfgfile(const std::string sfile={}, cfgopenmode mode=cfgopenmode::r, const char sep='=')
        : srcfile(sfile), readonly(mode==cfgopenmode::r), separator(sep)
        { reload(); }

    ~cfgfile() { if(!readonly) save(); }

    bool reload();
    bool save();
};

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
//[section 1]
//
//[section 2]
//name 1 = value 1   # Comment ...
//name 2 = value 2
//name 3 = Value_contains_#_character   # Must followed by a comment at the line
//name 4 =           # Empty value
//name 5             # Empty value
//
//Ignored Matter
//---------------------------
//.Empty lines and lines that start with a '#' in the first non-space column are ignored.
//.From the most right character '#' to the end of the line are ignored.
//
//Sections
//---------------------------
//A section starts with a section header. The section header must be the first line
//in the configuration file, except for empty lines or comments.
//
//The section header starts with a left bracket '[', any space before which is
//ignored. Everything(including any space) between the opening bracket and the first
//right bracket ']' in the line is the section name. Any text between that right
//bracket and the end of the line is ignored.
//
//Section names are case-sensitive and not empty.
//
//Reopening Sections
//---------------------------
//A section is implicitly closed when another section header is encountered. However,
//the section may be reopened by repeating its section header, and new options can be
//defined or existing options can be redefined.
//
//Options
//---------------------------
//An option is a mapping between a name and a value. Options cannot appear outside of
//a section. Options in different sections are considered separate even if they have
//the same name.
//
//The way to define an option:
//
//name = value
//
//Option names, like section names, are case-sensitive and not empty.
//
//The option name must start in the first column of the line and stops at the separator
//'=', but the leading spaces and the spaces around the separator are optional and are
//ignored. This means that:
//
//. The name cannot start with '[' or '#', those characters start a section header or a
//  comment;
//. The name may not contain the separator '=' or the comment mark '#'.
//. Like section names, an option name may contain spaces within it, but all leading and
//  trailing spaces are removed.
//. If '=' is omitted, value is an empty string.
//
//The options's value is all the text from the separator character until the end of the
//line or the most right character '#', which starts a comment at the line. Leading and
//trailing spaces around the option value are removed, but spaces within the value are
//preserved.
//
//So if the value contains '#' characters，you have to add a comment at the line.
