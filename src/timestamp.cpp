//----------------------------------------------------------------------------
/// \file  timestamp.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of timestamp class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/

#include <utxx/convert.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/string.hpp>
#include <utxx/time.hpp>
#include <utxx/error.hpp>
#include <stdio.h>

namespace utxx {

boost::mutex            timestamp::s_mutex;
__thread hrtime_t       timestamp::s_last_hrtime;
__thread struct timeval timestamp::s_last_time = {0,0};
__thread time_t         timestamp::s_next_local_midnight_seconds = 0;
__thread time_t         timestamp::s_next_utc_midnight_seconds   = 0;
__thread time_t         timestamp::s_utc_offset = 0;
__thread char           timestamp::s_local_timestamp[16];
__thread char           timestamp::s_utc_timestamp[16];

#ifdef DEBUG_TIMESTAMP
volatile long timestamp::s_hrcalls;
volatile long timestamp::s_syscalls;
#endif

namespace {
    static const char* s_values[] = {
        "none", "time",   "time-msec", "time-usec",
        "date-time", "date-time-msec", "date-time-usec"
    };
}

stamp_type parse_stamp_type(const std::string& a_line) {
    stamp_type t = find_index<stamp_type>(s_values, a_line, (stamp_type)-1, true);

    if (int(t) == -1)
        throw badarg_error("parse_stamp_tpe: invalid timestamp type: ", a_line);
    return t;
}

const char* to_string(stamp_type a_type) {
    assert(a_type < length(s_values));
    return s_values[a_type];
}

char* timestamp::write_time(
    char* a_buf,  const time_val& a_time, stamp_type a_type, bool a_utc,
    char  a_delim, char a_sep)
{
    auto tsec = a_utc ? a_time.sec() : a_time.sec() + s_utc_offset;
    unsigned long n  = tsec/86400;
                  n  = tsec - n*86400;
    int hour = n / 3600;    n -= hour*3600;
    int min  = n / 60;
    int sec  = n - min*60;  n = hour / 10;
    char*  p = a_buf;
    *p++ = '0' + n;     hour -= n*10;
    *p++ = '0' + hour;  n = min / 10;
    if (a_delim) *p++ = a_delim;
    *p++ = '0' + n;     min  -= n*10;
    *p++ = '0' + min;   n = sec / 10;
    if (a_delim) *p++ = a_delim;
    *p++ = '0' + n;     sec -= n*10;
    *p++ = '0' + sec;
    switch (a_type) {
        case TIME:
            break;
        case TIME_WITH_MSEC: {
            if (a_sep) *p++ = a_sep;
            int msec = a_time.msec();
            (void)itoa_right<int, 3>(p, msec, '0');
            p += 3;
            break;
        }
        case TIME_WITH_USEC: {
            if (a_sep) *p++ = a_sep;
            int usec = a_time.usec();
            (void)itoa_right<int, 6>(p, usec, '0');
            p += 6;
            break;
        }
        default:
            throw logic_error("timestamp::write_time: invalid a_type value: ", a_type);
    }
    return p;
}


void timestamp::internal_write_date(
    char* a_buf, time_t a_utc_seconds, bool a_utc, size_t eos_pos, char a_sep)
{
    assert(s_next_utc_midnight_seconds);

    if (!a_utc) a_utc_seconds += timestamp::s_utc_offset;
    int y; unsigned m,d;
    std::tie(y,m,d) = from_gregorian_time(a_utc_seconds);
    int   n = y / 1000;
    char* p = a_buf;
    *p++ = '0' + n; y -= n*1000; n = y/100;
    *p++ = '0' + n; y -= n*100;  n = y/10;
    *p++ = '0' + n; y -= n*10;
    *p++ = '0' + y; n  = m / 10;
    if (a_sep) *p++ = a_sep;
    *p++ = '0' + n; m -= n*10;
    *p++ = '0' + m; n  = d / 10;
    if (a_sep) *p++ = a_sep;
    *p++ = '0' + n; d -= n*10;
    *p++ = '0' + d;
    *p++ = '-';
    if (eos_pos) a_buf[eos_pos] = '\0';
}

void timestamp::write_date(
    char* a_buf, time_t a_utc_seconds, bool a_utc, size_t eos_pos, char a_sep)
{
    // If same day - use cached string value
    if (unlikely(a_utc_seconds >= s_next_utc_midnight_seconds))
        internal_write_date(a_buf, a_utc_seconds, a_utc, eos_pos, a_sep);
    else {
        strncpy(a_buf, a_utc ? s_utc_timestamp : s_local_timestamp, 9);
        if (eos_pos) a_buf[eos_pos] = '\0';
    }
}

void timestamp::update_midnight_seconds(const time_val& a_now)
{
    struct tm tm;
    localtime_r(&a_now.tv_sec(), &tm);
    s_utc_offset = tm.tm_gmtoff;
    s_next_utc_midnight_seconds   = (a_now.sec() - a_now.sec() % 86400) + 86400;
    s_next_local_midnight_seconds = s_next_utc_midnight_seconds  - s_utc_offset;

    // the mutex is not needed here at all - s_timestamp lives in TLS storage
    internal_write_date(s_local_timestamp, a_now.sec(), false, 9, '\0');
    internal_write_date(s_utc_timestamp, a_now.sec(), true, 9, '\0');
}

const time_val& timestamp::now() {
    // thread safe - we use TLV storage
    s_last_time   = time_val::universal_time().timeval();
    s_last_hrtime = high_res_timer::gettime();
    return last_time();
}

void timestamp::update_slow()
{
    now();
    #ifdef DEBUG_TIMESTAMP
    atomic::inc(&s_syscalls);
    #endif

    // FIXME: the method below will produce incorrect time stamps during
    // switch to/from daylight savings time because of the unaccounted
    // utc_offset change.
    if (unlikely(last_time().sec() >= s_next_utc_midnight_seconds))
        update_midnight_seconds(last_time());
}

size_t timestamp::format_size(stamp_type a_tp)
{
    switch (a_tp) {
        case NO_TIMESTAMP:          return 0;
        case TIME:                  return 8;
        case TIME_WITH_USEC:        return 15;
        case TIME_WITH_MSEC:        return 12;
        case DATE_TIME:             return 17;
        case DATE_TIME_WITH_USEC:   return 24;
        case DATE_TIME_WITH_MSEC:   return 21;
        default:
            assert(false);
    }
}

int timestamp::format(stamp_type a_tp,
    const struct timeval* tv, char* a_buf, size_t a_sz, bool a_utc)
{
    BOOST_ASSERT((a_tp < DATE_TIME && a_sz > 14) || a_sz > 25);

    if (unlikely(a_tp == NO_TIMESTAMP)) {
        a_buf[0] = '\0';
        return 0;
    }

    long midnight = a_utc ? s_next_utc_midnight_seconds : s_next_local_midnight_seconds;
    if (unlikely(tv->tv_sec >= midnight)) {
        timestamp ts; ts.update();
    }

    // If small time is given, it's a relative value.
    bool l_rel  = tv->tv_sec < 86400;
    time_t sec  = l_rel ? tv->tv_sec : tv->tv_sec + (a_utc ? 0 : s_utc_offset);
    long l_usec = tv->tv_usec;

    switch (a_tp) {
        case TIME:
            write_time(a_buf, sec, 8);
            return 8;
        case TIME_WITH_USEC:
            write_time(a_buf, sec, 15);
            a_buf[8] = '.';
            itoa_right(a_buf+9, 6, l_usec, '0');
            return 15;
        case TIME_WITH_MSEC:
            write_time(a_buf, sec, 12);
            a_buf[8] = '.';
            itoa_right(a_buf+9, 3, l_usec / 1000, '0');
            return 12;
        case DATE_TIME:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 8);
            return 17;
        case DATE_TIME_WITH_USEC:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 15);
            a_buf[17] = '.';
            itoa_right(a_buf+18, 6, l_usec, '0');
            return 24;
        case DATE_TIME_WITH_MSEC:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 12);
            a_buf[17] = '.';
            itoa_right(a_buf+18, 3, l_usec / 1000, '0');
            return 21;
        default:
            strcpy(a_buf, "UNDEFINED");
            return -1;
    }
}

time_val timestamp::from_string(const char* a_datetime, size_t n, bool a_utc) {
    if (unlikely(a_datetime[8] != '-' ||
                 a_datetime[11] != ':' || a_datetime[14] != ':' ||
                 n < 17))
        throw badarg_error("Invalid time format: ", std::string(a_datetime, n));

    const char* p = a_datetime;

    auto parse = [&p](int digits) {
        int i = 0; const char* end = p + digits;
        for (; p != end; ++p) i = (10*i) + (*p - '0');
        return i;
    };

    int      year = parse(4);
    unsigned mon  = parse(2);
    unsigned day  = parse(2); p++;
    unsigned hour = parse(2); p++;
    unsigned min  = parse(2); p++;
    unsigned sec  = parse(2);
    unsigned usec = 0;

    const char* dot = p++;

    if (*dot == '.' && n > 17)
    {
        const char* end = p + std::min<size_t>(6, n - (p - a_datetime));
        while (*p >= '0' && *p <= '9' && p != end)
            usec = 10*usec + (*p++ - '0');

        int len = p - dot - 1;
        switch (len)
        {
            case 3:  usec *= 1000; break;
            case 6:  break;
            default: throw badarg_error("Invalid millisecond format: ",
                                        std::string(a_datetime, end - a_datetime));
        }
    }

    return a_utc
         ? time_val::universal_time(year, mon, day, hour, min, sec, usec)
         : time_val::local_time(year, mon, day, hour, min, sec, usec);
}

} // namespace utxx
