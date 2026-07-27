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
#include <istream>
#include <filesystem>
#include "cfgfile.hpp"

using namespace std;

//---------------------------------------------------------------------------------------
void cfgfile::open(string path, bool readonly, char sep)
{
    if (path.empty()) throw invalid_argument("cfgfile::open: path empty");

    if (!srcfile_.empty()) {    // re-use the instance
        if (modified_ && !readonly_) save();   // may throw exception
        cfg_.clear();
        modified_ = false;
    }

    srcfile_   = move(path);
    separator_ = sep;
    readonly_  = readonly;

    error_code ec;
    bool exists = filesystem::exists(srcfile_, ec);
    if (ec) throw filesystem::filesystem_error("cfgfile::open: cannot stat file", srcfile_, ec);

    if (exists) {
        ifstream ifs(srcfile_);
        if (!ifs.is_open()) throw runtime_error("cfgfile::open: cannot open file: " + srcfile_);
        parse_stream(ifs);
    } else {
        if (readonly) throw filesystem::filesystem_error( "cfgfile::open: file not found", srcfile_,
                make_error_code(errc::no_such_file_or_directory));

        // !readonly: try to create
        ofstream ofs(srcfile_);
        if (!ofs.is_open()) throw runtime_error("cfgfile::open: cannot create file: " + srcfile_);
    }
}

//---------------------------------------------------------------------------------------
void cfgfile::save()
{
    check_writable();

    ofstream ofs(srcfile_, ios::trunc);
    if (!ofs.is_open()) throw runtime_error("cfgfile::save: cannot open file: " + srcfile_);

    ofs.exceptions(ofstream::failbit | ofstream::badbit);

    try {
        write_stream(ofs);
        ofs.close();
    } catch (const ios_base::failure& e) {
        throw runtime_error( string("cfgfile::save: I/O error: ") + e.what());
    }

    modified_ = false;
}

//---------------------------------------------------------------------------------------
void cfgfile::parse_stream(istream& is)
{
    cfg_.clear();
    string line;
    string current_section{};

    while (getline(is, line)) {
        string_view linesv = rmcommsv(line, mark_);
        if (linesv.empty()) continue;
        if (linesv[0] == '[') {
            current_section = lrmarksv(linesv, '[', ']');   // may be empty if there's no ']'
            continue;
        }
        const auto& [key, value] {splitpairsv(linesv,separator_)};   // key or/and value may be empty
        cfg_[current_section][string(key)] = value;
    }
}

//---------------------------------------------------------------------------------------
void cfgfile::write_stream(ostream& os) const
{
    // If sec_name or k-v contains '#', add a comment at the end automatically
    auto add_comment = [&](){ os << "  " << mark_ << " must have a comment at tail"; };

    for (const auto& [sec_name, kvs] : cfg_) {
        os << '[' << sec_name << ']';
        if (sec_name.find(mark_) != string::npos) add_comment();
        os << '\n';
        for (const auto& [k, v] : kvs) {
            os << k << separator_ << v;
            if (k.find(mark_) != string::npos || v.find(mark_) != string::npos) add_comment();
            os << '\n';
        }
        os << '\n';
    }
}

//---------------------------------------------------------------------------------------
