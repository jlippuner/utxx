//----------------------------------------------------------------------------
/// \file   endian.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief A wrapper around boost endian-handling functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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
#pragma once

#if BOOST_VERSION >= 107100
#include <boost/endian/conversion.hpp>
#else
#include <boost/spirit/home/support/detail/endian.hpp>
#endif
#include <boost/static_assert.hpp>
#include <stdint.h>

namespace utxx {

namespace {
#if BOOST_VERSION >= 107100
    namespace bsd = boost::endian;
#elif BOOST_VERSION >= 104800
    namespace bsd = boost::spirit::detail;
#else
    namespace bsd = boost::detail;
#endif
}

template <typename T, typename Ch>
inline void put_be(Ch*& s, T n) {
#if BOOST_VERSION >= 107100
    bsd::endian_store<T, sizeof(T), bsd::order::big>(s, n);
#else
    bsd::store_big_endian<T, sizeof(T)>((void*)s, n);
#endif
    s += sizeof(T);
}

template <typename T, typename Ch>
inline void put_le(Ch*& s, T n) {
#if BOOST_VERSION >= 107100
    bsd::endian_store<T, sizeof(T), bsd::order::little>(s, n);
#else
    bsd::store_little_endian<T, sizeof(T)>((void*)s, n);
#endif
    s += sizeof(T);
}

template <typename T>
inline void store_be(char* s, T n) {
#if BOOST_VERSION >= 107100
    bsd::endian_store<T, sizeof(T), bsd::order::big>(reinterpret_cast<unsigned char*>(s), n);
#else
    bsd::store_big_endian<T, sizeof(T)>(static_cast<void*>(s), n);
#endif
}

template <typename T>
inline void store_le(char* s, T n) {
#if BOOST_VERSION >= 107100
    bsd::endian_store<T, sizeof(T), bsd::order::little>(reinterpret_cast<unsigned char*>(s), n);
#else
    bsd::store_little_endian<T, sizeof(T)>(static_cast<void*>(s), n);
#endif
}

inline void store_be(char* s, double n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union u { double f; uint64_t i; };
#if BOOST_VERSION >= 107100
    bsd::endian_store<uint64_t, sizeof(double), bsd::order::big>
        (reinterpret_cast<unsigned char*>(s), reinterpret_cast<u*>(&n)->i);
#else
    bsd::store_big_endian<uint64_t, sizeof(double)>(
        static_cast<void*>(s), reinterpret_cast<u*>(&n)->i);
#endif
}

inline void store_le(char* s, double n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union u { double f; uint64_t i; };
#if BOOST_VERSION >= 107100
    bsd::endian_store<uint64_t, sizeof(double), bsd::order::little>
        (reinterpret_cast<unsigned char*>(s), reinterpret_cast<u*>(&n)->i);
#else
    bsd::store_little_endian<uint64_t, sizeof(double)>(
        static_cast<void*>(s), reinterpret_cast<u*>(&n)->i);
#endif
}

template <typename T>
inline void cast_be(const char* s, T& n) {
#if BOOST_VERSION >= 107100
    n = bsd::endian_load<T, sizeof(T), bsd::order::big>
        (reinterpret_cast<const unsigned char*>(s));
#else
    n = bsd::load_big_endian<T, sizeof(T)>((const void*)s);
#endif
}

template <typename T>
inline void cast_le(const char* s, T& n) {
#if BOOST_VERSION >= 107100
    n = bsd::endian_load<T, sizeof(T), bsd::order::little>
        (reinterpret_cast<const unsigned char*>(s));
#else
    n = bsd::load_little_endian<T, sizeof(T)>((const void*)s);
#endif
}

inline void cast_be(const char* s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
#if BOOST_VERSION >= 107100
    u.i = bsd::endian_load<uint64_t, sizeof(double), bsd::order::big>
        (reinterpret_cast<const unsigned char*>(s));
#else
    u.i = bsd::load_big_endian<uint64_t, sizeof(double)>((const void*)s);
#endif
    n = u.f;
}

inline void cast_le(const char* s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
#if BOOST_VERSION >= 107100
    u.i = bsd::endian_load<uint64_t, sizeof(double), bsd::order::little>
        (reinterpret_cast<const unsigned char*>(s));
#else
    u.i = bsd::load_little_endian<uint64_t, sizeof(double)>((const void*)s);
#endif
    n = u.f;
}

template <typename T, typename Ch>
inline T cast_be(const Ch* s) { T n; cast_be(s, n); return n; }

template <typename T, typename Ch>
inline T cast_le(const Ch* s) { T n; cast_le(s, n); return n; }

template <typename T>
inline void get_be(const char*& s, T& n) {
    n = cast_be<T>(s);
    s += sizeof(T);
}

template <typename T, typename Ch>
inline void get_le(const Ch*& s, T& n) {
    n = cast_le<T>(s);
    s += sizeof(T);
}

template <typename T, typename Ch>
inline T get_be(const Ch*& s) { T n; get_be(s, n); return n; }

template <typename T, typename Ch>
inline T get_le(const Ch*& s) { T n; get_le(s, n); return n; }

inline void get_be(const char*& s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
    u.i = get_be<uint64_t, char>(s);
    n = u.f;
    s += sizeof(double);
}

inline void get_le(const char*& s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
    u.i = get_le<uint64_t, char>(s);
    n = u.f;
    s += sizeof(double);
}

template <class Ch>
inline void put8   (Ch*& s, uint8_t n ) { put_be(s, n); }
template <class Ch>
inline void put16be(Ch*& s, uint16_t n) { put_be(s, n); }
template <class Ch>
inline void put32be(Ch*& s, uint32_t n) { put_be(s, n); }
template <class Ch>
inline void put64be(Ch*& s, uint64_t n) { put_be(s, n); }

template <class Ch>
inline void put16le(Ch*& s, uint16_t n) { put_le(s, n); }
template <class Ch>
inline void put32le(Ch*& s, uint32_t n) { put_le(s, n); }
template <class Ch>
inline void put64le(Ch*& s, uint64_t n) { put_le(s, n); }

template <class Ch>
inline uint8_t  get8   (const Ch*& s) { uint8_t  n; get_be(s, n); return n; }
template <class Ch>
inline uint16_t get16be(const Ch*& s) { uint16_t n; get_be(s, n); return n; }
template <class Ch>
inline uint32_t get32be(const Ch*& s) { uint32_t n; get_be(s, n); return n; }
template <class Ch>
inline uint64_t get64be(const Ch*& s) { uint64_t n; get_be(s, n); return n; }

template <class Ch>
inline uint16_t get16le(const Ch*& s) { uint16_t n; get_le(s, n); return n; }
template <class Ch>
inline uint32_t get32le(const Ch*& s) { uint32_t n; get_le(s, n); return n; }
template <class Ch>
inline uint64_t get64le(const Ch*& s) { uint64_t n; get_le(s, n); return n; }

template <class Ch>
inline uint8_t  cast8   (const Ch* s) { return s[0]; }
template <class Ch>
inline uint16_t cast16be(const Ch* s) { uint16_t n; cast_be(s, n); return n; }
template <class Ch>
inline uint32_t cast32be(const Ch* s) { uint32_t n; cast_be(s, n); return n; }
template <class Ch>
inline uint64_t cast64be(const Ch* s) { uint64_t n; cast_be(s, n); return n; }
inline double   cast_double_be(const char* s) { double n; cast_be(s, n); return n; }

template <class Ch>
inline uint16_t cast16le(const Ch* s) { uint16_t n; cast_le(s, n); return n; }
template <class Ch>
inline uint32_t cast32le(const Ch* s) { uint32_t n; cast_le(s, n); return n; }
template <class Ch>
inline uint64_t cast64le(const Ch* s) { uint64_t n; cast_le(s, n); return n; }
inline double   cast_double_le(const char* s) { double n; cast_le(s, n); return n; }

} // namespace utxx
