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

using namespace std;

//---------------------------------------------------------------------------------------
bool cfgfile::reload()
{
    const char *space_chars{" \t\n\r\f\v"};

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
    string current_section;
    while (getline(ifs, line)) {
        // firstly, remove line leading spaces:
        line.erase(0, line.find_first_not_of(space_chars));

        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            size_t end_bracket = line.find(']');
            if (end_bracket == string::npos) {
                errmsg = "Invalid section header: "s + line;
                return false;
            }
            // section name including leading and trailing spaces,
            // means that one or more space characters can act as section name.
            current_section = line.substr(1, end_bracket - 1);
            if (current_section.empty()) {
                errmsg = "Section name is empty: "s + line;
                return false;
            }
            continue;
        }

        if (current_section.empty()) {
            errmsg = "Option outside of section: "s + line;
            return false;
        }

        // Now it's time to remove line trailing spaces:
        line.erase(line.find_last_not_of(space_chars) + 1);

        size_t sep_pos = line.find(separator);
        if (sep_pos == string::npos) {
            errmsg = "Invalid option format: "s + line;
            return false;
        }

        string key = line.substr(0, sep_pos);
        string value = line.substr(sep_pos + 1);

        key.erase(key.find_last_not_of(space_chars) + 1);       // leading spaces have been removed with line
        value.erase(0, value.find_first_not_of(space_chars));   // trailing spaces have been removed with line

        if (key.empty()) {
            errmsg = "Empty key in option: "s + line;
            return false;
        }

        cfg[current_section][key] = value;  // value can be empty
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

    for (const auto& section_pair : cfg) {
        ofs << '[' << section_pair.first << ']' << endl;
        for (const auto& option_pair : section_pair.second)
            ofs << option_pair.first << ' ' << separator << ' ' << option_pair.second << endl;

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

