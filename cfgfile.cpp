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

#include <iostream>
#include <fstream>
#include <filesystem>
#include "cfgfile.hpp"
#include "strext.hpp"

using namespace std;

//---------------------------------------------------------------------------------------
bool cfgfile::reload()
{
    cfg.clear();
    errmsg.clear();

    if (srcfile.empty()) {
        if (readonly) return true;
        errmsg = "No file name. Changes will not be saved"s;
        return false;
    }

    if( !filesystem::exists(srcfile) ) {
        if (readonly) {
            errmsg = "File does not exist: "s + srcfile;
            return false;
        }
        return true;    // a new file for writing that may not exist
    }

    ifstream ifs(srcfile);
    if (!ifs.is_open()) {
        errmsg = "Failed to open file: "s + srcfile;
        return false;
    }

    string line;
    string current_section{};
    while (getline(ifs, line)) {
        string_view linesv = rmcommsv(line);
        if (linesv.empty()) continue;

        if (linesv[0] == '[') {
            // Section name including leading and trailing spaces means that one or more space characters can act as section name.
            // Characters after ']' are ignored.
            current_section = lrmarksv(linesv, '[', ']');
            if (current_section.empty()) {
                errmsg = "Invalid section header: "s + line;
                return false;
            }
            continue;
        }

        if (current_section.empty()) {
            errmsg = "Option outside of section: "s + line;
            return false;
        }

        const pair key_value {splitpairsv(linesv,separator)};

        if( key_value.first.empty() ) {
            errmsg = "Empty key in option: "s + line;
            return false;
        }

        cfg[current_section][std::string(key_value.first)] = key_value.second;
    }

    return true;
}

//---------------------------------------------------------------------------------------
bool cfgfile::save()
{
    if (srcfile.empty()) {
        errmsg = "No file to write."s;
        return false;
    }

    if (readonly) {
        errmsg = "Read-only is set: "s + srcfile;
        return false;
    }

    ofstream ofs(srcfile);
    if (!ofs.is_open()) {
        errmsg = "Failed to open file for writing: "s + srcfile;
        return false;
    }

    // If value contains '#', add a comment at the end automatically
    constexpr std::string_view auto_comment = "    # value containing number sign(s) must have a comment at the line"sv;

    for (const auto& section_pair : cfg) {
        ofs << '[' << section_pair.first << ']' << endl;
        for (const auto& option_pair : section_pair.second) {
            ofs << option_pair.first << separator << option_pair.second;
            if( option_pair.second.find('#') != std::string::npos ) ofs << auto_comment;
            ofs << endl;
        }
        ofs << endl;
    }

    ofs.exceptions ( ofstream::failbit | ofstream::badbit );
    try {
        ofs.close();
    }
    catch (ofstream::failure &e) {
        errmsg = "Error writing file: "s + e.what();
        return false;
    }

    errmsg.clear();
    return true;
}
