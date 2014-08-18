//----------------------------------------------------------------------------
/// \file  time_val.hpp
//----------------------------------------------------------------------------
/// \brief Derived implementation of ACE TimeVal to represent timeval
/// structure.
/// \author:  Serge Aleynikov
/// The original version is included in
/// (http://www.cs.wustl.edu/~schmidt/ACE.html).
//----------------------------------------------------------------------------
// ACE Library Copyright (c) 1993-2009 Douglas C. Schmidt
// Created: 2003-07-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2003 Serge Aleynikov <saleyn@gmail.com>

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


#ifndef _UTXX_TIME_VAL_HPP_
#define _UTXX_TIME_VAL_HPP_

#include <boost/static_assert.hpp>
#include <utxx/detail/gettimeofday.hpp>
#include <utxx/compiler_hints.hpp>
#include <cstddef>
#include <stdint.h>
#include <ctime>
#include <sys/time.h>
#include <math.h>

namespace utxx {
    class time_val;

    /// Indication of use of absolute time
    struct abs_time {
        long sec, usec;
        abs_time(long s=0, long us=0) : sec(s), usec(us) {}
    };
    /// Indication of use of relative time
    struct rel_time {
        long sec, usec;
        rel_time(long s=0, long us=0) : sec(s), usec(us) {}
    };

    /// A helper class for dealing with 'struct timeval' structure. This class adds ability
    /// to perform arithmetic with the structure leaving the same footprint.
    class time_val
    {
        static const long   N10e6 = 1000000;
        static const size_t N10e9 = 1000000000u;
        struct timeval m_tv;

        void normalize() {
            if (m_tv.tv_usec >= N10e6)
                do { ++m_tv.tv_sec; m_tv.tv_usec -= N10e6; }
                while (m_tv.tv_usec >= N10e6);
            else if (m_tv.tv_usec <= -N10e6)
                do { --m_tv.tv_sec; m_tv.tv_usec += N10e6; }
                while (m_tv.tv_usec <= -N10e6);

            if (m_tv.tv_sec >= 1 && m_tv.tv_usec < 0)
                { --m_tv.tv_sec; m_tv.tv_usec += N10e6; }
            else if (m_tv.tv_sec <  0 && m_tv.tv_usec > 0)
                { ++m_tv.tv_sec; m_tv.tv_usec -= N10e6; }
        }

        void setd(int64_t val, int64_t divisor) {
            int64_t n = val / divisor;
            m_tv.tv_sec  = n;
            m_tv.tv_usec = val - n*divisor;
        }

        void setd(double interval) {
            long n = long(round(interval*1e6));
            m_tv.tv_sec = n / N10e6; m_tv.tv_usec = n - m_tv.tv_sec*N10e6;
        }

    public:
        time_val()                   { m_tv.tv_sec=0; m_tv.tv_usec=0; }
        time_val(long _s, long _us)  { m_tv.tv_sec=_s; m_tv.tv_usec=_us; normalize(); }
        time_val(const time_val& tv, long _s, long _us=0) { set(tv, _s,  _us); }
        time_val(const time_val& tv, double interval)     { set(tv, interval); }
        time_val(const time_val& a) : m_tv(a.m_tv) {}

        explicit time_val(double interval)  { setd(interval); normalize(); }

        explicit time_val(const struct timeval& a) {
            m_tv.tv_sec=a.tv_sec; m_tv.tv_usec=a.tv_usec; normalize();
        }

        explicit time_val(struct tm& a_tm) {
            m_tv.tv_sec = mktime(&a_tm); m_tv.tv_usec = 0;
        }

        /// Set time to abosolute time given by \a a
        explicit time_val(const abs_time& a) {
            m_tv.tv_sec=a.sec; m_tv.tv_usec=a.usec; normalize();
        }

        /// Set time to relative offset from now given by \a a time.
        explicit time_val(const rel_time& a) { now(a.sec, a.usec); }

        #if __cplusplus >= 201103L
        time_val(time_val&& a) : m_tv(std::move(a.m_tv)) {}
        #endif

        const struct timeval&   timeval()  const { return m_tv; }
        struct timeval&         timeval()        { return m_tv; }
        struct timespec         timespec() const { struct timespec ts = {sec(),nanosec()};
                                                   return ts; }
        long                    sec()      const { return m_tv.tv_sec;  }
        long                    usec()     const { return m_tv.tv_usec; }
        time_t&                 sec()            { return m_tv.tv_sec;  }
        suseconds_t&            usec()           { return m_tv.tv_usec; }
        time_t                  msec()     const { return m_tv.tv_usec / 1000; }
        long                    nanosec()  const { return m_tv.tv_usec * 1000; }

        uint64_t microseconds() const {
            return (uint64_t)m_tv.tv_sec*N10e6 + (uint64_t)m_tv.tv_usec;
        }

        double seconds()          const { return  (double)m_tv.tv_sec
                                                + (double)m_tv.tv_usec/N10e6; }

        long milliseconds()       const { return m_tv.tv_sec*1000u + m_tv.tv_usec/1000; }
        void sec (size_t s)             { m_tv.tv_sec  = s;  }
        void usec(size_t us)            { m_tv.tv_usec = us; normalize(); }
        void microseconds(uint64_t ms)  { setd(ms, N10e6); }
        void milliseconds(size_t   ms)  { setd(ms, 1000);  }
        void nanosec (uint64_t ms)      { setd(ms, N10e9); }
        bool empty()              const { return sec() == 0 && usec() == 0; }

        void clear()                    { m_tv.tv_sec = 0; m_tv.tv_usec = 0; }

        void set(const time_val& tv, long _s=0, long _us=0) {
            m_tv.tv_sec = tv.sec() + _s; m_tv.tv_usec = tv.usec() + _us; normalize();
        }

        void set(const struct timeval& tv, long _s=0, long _us=0) {
            m_tv.tv_sec = tv.tv_sec + _s; m_tv.tv_usec = tv.tv_usec + _us; normalize();
        }
        void set(const time_val& tv, double interval) {
            setd(interval); m_tv.tv_sec += tv.sec(); m_tv.tv_usec += tv.usec();
            normalize();
        }

        void copy_to(struct timeval& tv) const {
            tv.tv_sec = m_tv.tv_sec; tv.tv_usec = m_tv.tv_usec;
        }

        double diff(const time_val& t) const {
            time_val tv(this->timeval());
            tv -= t;
            return (double)tv.sec() + (double)tv.usec() / N10e6;
        }

        int64_t diff_usec(const time_val& t) const {
            time_val tv(this->timeval());
            tv -= t;
            return (int64_t)tv.sec() * N10e6 + tv.usec();
        }

        int64_t diff_msec(const time_val& t) const {
            time_val tv(this->timeval());
            tv -= t;
            return (int64_t)tv.sec() * 1000 + tv.usec() / 1000;
        }

        void add(long _sec, long _us) {
            m_tv.tv_sec += _sec; m_tv.tv_usec += _us;
            if (_sec || _us) normalize();
        }

        void add(double interval) {
            long n = long(round(interval*1e6));
            long s = n / N10e6, u = n - s*N10e6;
            add(s, u);
        }

        void now() { ::gettimeofday(&m_tv, 0); }

        static time_val universal_time() {
            time_val tv; ::gettimeofday(&tv, 0); return tv;
        }

        /// Construct a time_val from UTC "y/m/d-H:M:S"
        static time_val universal_time(int year, int month, int day,
                                       int hour, int min,   int sec, int usec) {
            static __thread int    s_y, s_m, s_d;
            static __thread time_t s_ymd;

            if (year != s_y || month != s_m || day != s_d) {
                struct tm tm = {0, 0, 0, day, month-1, year-1900};
                s_ymd = ::mktime(&tm);
                s_y   = year;
                s_m   = month;
                s_d   = day;
            }

            time_val tv;
            tv.m_tv.tv_sec  = s_ymd + hour*3600 + min*60 + sec;
            tv.m_tv.tv_usec = usec;
            return tv;
        };

        /// Construct a time_val from local "y/m/d-H:M:S"
        static time_val local_time(int year, int month, int day,
                                   int hour, int min,   int sec, int usec) {
            static __thread int    s_y, s_m, s_d, s_tzoffset;
            static __thread time_t s_ymd;

            if (year != s_y || month != s_m || day != s_d) {
                if (unlikely(!s_tzoffset))
                {
                    time_t now; time(&now);
                    struct tm ltm;
                    localtime_r(&now, &ltm);
                    s_tzoffset = ltm.tm_gmtoff;
                }
                struct tm tm = {0, 0, 0, day, month-1, year-1900};
                s_ymd = ::mktime(&tm) + s_tzoffset;
                s_y   = year;
                s_m   = month;
                s_d   = day;
            }

            time_val tv;
            tv.m_tv.tv_sec  = s_ymd + hour*3600 + min*60 + sec;
            tv.m_tv.tv_usec = usec;
            return tv;
        };

        time_val& now(long addS, long addUS=0) {
            ::gettimeofday(&m_tv, 0); add(addS, addUS);
            return *this;
        }

        const time_val* ptr() const { return reinterpret_cast<const time_val*>(&m_tv); }
        time_val*       ptr()       { return reinterpret_cast<time_val*>(&m_tv); }

        static double now_diff(const time_val& start) {
            time_val tv; tv.now(); tv -= start;
            return (double)tv.sec() + (double)tv.usec() / N10e6;
        }
        static int64_t now_diff_usec(const time_val& start) {
            time_val tv; tv.now(); tv -= start;
            return (int64_t)tv.sec() * N10e6 + tv.usec();
        }
        static int64_t now_diff_msec(const time_val& start) {
            time_val tv; tv.now(); tv -= start;
            return (int64_t)tv.sec() * 1000 + tv.usec() / 1000;
        }

        time_val operator- (const time_val& tv) const {
            return time_val(m_tv.tv_sec - tv.sec(), m_tv.tv_usec - tv.usec());
        }
        time_val operator+ (const time_val& tv) const {
            return time_val(m_tv.tv_sec + tv.sec(), m_tv.tv_usec + tv.usec());
        }

        time_val operator- (double interval) const { return operator-(time_val(interval)); }
        time_val operator+ (double interval) const { return operator+(time_val(interval)); }

        time_val operator- (const struct timeval& tv) const {
            return time_val(m_tv.tv_sec - tv.tv_sec, m_tv.tv_usec - tv.tv_usec);
        }
        time_val operator+ (const struct timeval& tv) const {
            return time_val(m_tv.tv_sec + tv.tv_sec, m_tv.tv_usec + tv.tv_usec);
        }

        void operator-= (const struct timeval& tv) {
            m_tv.tv_sec -= tv.tv_sec; m_tv.tv_usec -= tv.tv_usec; normalize();
        }
        void operator-= (const time_val& tv) {
            m_tv.tv_sec -= tv.sec(); m_tv.tv_usec -= tv.usec(); normalize();
        }
        void operator+= (const time_val& tv) {
            m_tv.tv_sec += tv.sec(); m_tv.tv_usec += tv.usec(); normalize();
        }
        void operator+= (double  _interval) { add(_interval); }
        void operator+= (int32_t _sec)      { m_tv.tv_sec += _sec; }
        void operator+= (int64_t _microsec) {
            int64_t n = _microsec / N10e6;
            m_tv.tv_sec  += n;
            m_tv.tv_usec += (_microsec - (n * N10e6));
            normalize();
        }

        #if __cplusplus >= 201103L
        time_val& operator=(time_val&& a_rhs) {
            m_tv = a_rhs.m_tv;
            return *this;
        }
        #endif

        void operator= (const time_val& t) {
            m_tv.tv_sec = t.sec(); m_tv.tv_usec = t.usec();
        }
        void operator= (const struct timeval& t) {
            m_tv.tv_sec = t.tv_sec; m_tv.tv_usec = t.tv_usec;
        }
        struct timeval* operator& ()               { return &m_tv; }
        bool operator== (const time_val& tv) const {
            return sec() == tv.sec() && usec() == tv.usec();
        }
        bool operator!= (const time_val& tv) const { return !operator== (tv); }
        bool operator>  (const time_val& tv) const {
            return sec() > tv.sec() || (sec() == tv.sec() && usec() > tv.usec());
        }
        bool operator>= (const time_val& tv) const {
            return sec() > tv.sec() || (sec() == tv.sec() && usec() >= tv.usec());
        }
        bool operator<  (const time_val& tv) const {
            return sec() < tv.sec() || (sec() == tv.sec() && usec() < tv.usec());
        }
        bool operator<= (const time_val& tv) const {
            return sec() < tv.sec() || (sec() == tv.sec() && usec() <= tv.usec());
        }

        operator bool() const { return sec() == 0 && usec() == 0; }
    };

    template <typename T>
    time_val& time_val_cast(T& tv) {
        BOOST_STATIC_ASSERT(sizeof(T) == sizeof(struct timeval));
        return reinterpret_cast<time_val&>(tv);
    }

    template <typename T>
    const time_val& time_val_cast(const T& tv) {
        BOOST_STATIC_ASSERT(sizeof(T) == sizeof(struct timeval));
        return reinterpret_cast<const time_val&>(tv);
    }

    /// Same as gettimeofday() call
    inline time_val now_utc() { return time_val::universal_time(); }

    /// Simple timer for measuring interval of time
    ///
    /// Example usage1:
    /// \code
    ///     timer t; do_something(); double elapsed = t.elapsed();
    /// \endcode
    /// Example usage2:
    /// \code
    ///     time_val time;
    ///     {
    ///         timer t(time);
    ///         do_something();
    ///     }
    ///     double elapsed = time.seconds();
    /// \endcode
    class timer {
        time_val* m_result;
        time_val  m_started;
    public:
        timer() : m_result(NULL), m_started(time_val::universal_time()) {}
        timer(time_val& tv) : m_result(tv.ptr()), m_started(time_val::universal_time()) {}

        ~timer() { if (m_result) *m_result = time_val::universal_time() - m_started; }

        void   reset() { m_started = time_val::universal_time(); }

        double elapsed()      const { return time_val::now_diff(m_started); }
        double elapsed_msec() const { return elapsed() * 1000; }
        double elapsed_usec() const { return elapsed() * 1000000; }

        double latency_usec(size_t a_count) const { return elapsed_usec() / a_count; }
        double latency_msec(size_t a_count) const { return elapsed()*1000 / a_count; }
        double latency_sec (size_t a_count) const { return elapsed()      / a_count; }
    };

} // namespace utxx

#endif // _UTXX_TIME_VAL_HPP_
