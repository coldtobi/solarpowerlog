/* ----------------------------------------------------------------------------
 solarpowerlog -- photovoltaic data logging

Copyright (C) 2009-2014 Tobias Frost

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "porting.h"
#endif

#ifdef  HAVE_FILTER_DBWRITER

#include "CDBWHSpecialTokens.h"

bool operator==(std::tm t1, std::tm t2) {
    if (t1.tm_sec != t2.tm_sec) return false;
    if (t1.tm_min != t2.tm_min) return false;
    if (t1.tm_hour != t2.tm_hour) return false;
    if (t1.tm_mday != t2.tm_mday) return false;
    if (t1.tm_mon != t2.tm_mon) return false;
    if (t1.tm_year != t2.tm_year) return false;
    return true;
}
bool operator!=(std::tm t1, std::tm t2) {
    if ((t1.tm_sec == t2.tm_sec) &&
       (t1.tm_min == t2.tm_min) &&
       (t1.tm_hour == t2.tm_hour) &&
       (t1.tm_mday == t2.tm_mday) &&
       (t1.tm_mon == t2.tm_mon) &&
       (t1.tm_year == t2.tm_year)) return false;
    return true;
}


bool CDBHST_Timestamp::Update(const std::tm &tm)
{
    bool ret;
    ret = (Get() != tm);
    Set(tm);
    return ret;
}

bool CDBHST_Year::Update(const std::tm &tm) {
    bool ret;
    long v;
    v = tm.tm_year;
    ret = (Get() != v);
    Set(v);
    return ret;
}

bool CDBHST_Month::Update(const std::tm &tm) {
    bool ret;
    long v;
    v = tm.tm_mon;
    ret = (Get() != v);
    Set(v);
    return ret;
}

bool CDBHST_Day::Update(const std::tm &tm) {
    bool ret;
    long v;
    v = tm.tm_mday;
    ret = (Get() != v);
    Set(v);
    return ret;
}

bool CDBHST_Hour::Update(const std::tm &tm) {
    bool ret;
    long v;
    v = tm.tm_hour;
    ret = (Get() != v);
    Set(v);
    return ret;
}

bool CDBHST_Minute::Update(const std::tm &tm) {
    bool ret;
    long v;
    v = tm.tm_min;
    ret = (Get() != v);
    Set(v);
    return ret;
}

#endif

