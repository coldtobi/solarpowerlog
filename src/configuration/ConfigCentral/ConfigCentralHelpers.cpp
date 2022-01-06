/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

 Copyright (C) 2015 Tobias Frost

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ----------------------------------------------------------------------------
 */

/*
 * ConfigCentral_helpers.cpp
 *
 *  Created on: 04.01.2015
 *      Author: tobi
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "IConfigCentralEntry.h"
#include "ConfigCentralHelpers.h"
#include <string>

std::string CConfigCentralHelpers::WrapForConfigSnippet(const std::string &description)
{

    std::string ret;
    std::string tmp = description;

    // the parsed snipped should look like
    // # <description wrapped at 80, manually wrap at \n
    // # manually remove double spaces and leading spaces.
    // # parameter = <your-value>

    // remove trailing white spaces (this ensures that the last char is not
    // a whitespace, so we can safely look ahead later.)
    // first remove trailing spaces / newlines.
    std::string whitespaces(" \t\f\v\n\r");
    std::size_t found = tmp.find_last_not_of(whitespaces);
    if (found != std::string::npos) tmp.erase(found + 1);
    else tmp.clear();            // is all whitespace?

    // iterate through string to remove double spaces
    size_t cursor = 0;
    std::string doublespace = "  ";
    while (1) {
        cursor = tmp.find(doublespace, cursor);
        if (std::string::npos == cursor) break;
        tmp.erase(cursor, 1);
    }

    cursor = 0;
    size_t column = 0;
    size_t len = tmp.length() - 1;
    // iterate through remaining string.
    while (len > cursor) {
        size_t s;
        // Add trailing "# "
        if (0 == column && len > cursor) {
            ret += "# ";
            column = 2;
        }

        if (tmp[cursor] == ' ') {
            cursor++;
            continue;
        }

        // look for newlines that shold break actual line
        s = tmp.find('\n', cursor);
        if (std::string::npos != s && (s - cursor) < (WRAP_AT - column)) {
            ret += tmp.substr(cursor, s - cursor + 1);
            column = 0;
            cursor = s + 1;
            continue;
        }

        // is the remaining line shorter than we would need to break anyway?
        s = len - cursor;
        if (s < (WRAP_AT - column)) {
            ret += tmp.substr(cursor);
            ret += '\n';
            break;
        }

        // find the spot to break
        // cursor = last character parsed
        // column = horizontal position

        // lets keep the followin debug aid for a moment:
        //std::cerr << "len=" << tmp.length() <<"column=" << column << " cursor=" << cursor << " " << std::endl;

        s = tmp.find_last_of(whitespaces, cursor + WRAP_AT - column);
        // s == npos means that we've got a monster string without spaces.
        // also if s is smaller than the current cursor position.
        if (std::string::npos == s || s < cursor) {
            s = tmp.find_first_not_of(whitespaces, cursor);
            if (s != std::string::npos) {
                if (s == cursor) {
                    // The next whitespace is at the cursor.
                    // we have to get the complete monster-word...
                    s = tmp.find_first_of(whitespaces,cursor+1);
                    if ( s == std::string::npos) s=tmp.length()-1;
                }
                ret += tmp.substr(cursor, s - cursor);
                cursor = s;
                continue;
            } else {
                ret += tmp.substr(cursor);
                ret += "\n";
                break;
            }
        } else {
            // s is > cursor and our wrap position
            ret += tmp.substr(cursor, s - cursor);
            cursor = s;
            ret += "\n";
            column = 0;
        }
    }
    return ret;
}
