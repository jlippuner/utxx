//----------------------------------------------------------------------------
/// \file  get_option.hpp
//----------------------------------------------------------------------------
/// \brief Get command-line option values.
//----------------------------------------------------------------------------
// Author:  Serge Aleynikov
// Created: 2010-01-06
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

#include <stdlib.h>
#include <string.h>
#include <type_traits>
#include <boost/lexical_cast.hpp>

namespace utxx {

inline long env(const char* a_var, long a_default) {
    const char* p = getenv(a_var);
    return p ? atoi(p) : a_default;
}

namespace {
    template <typename T>
    typename std::enable_if<!std::is_same<T, bool>::value, T>::type
    convert(const char* a) { return boost::lexical_cast<T>(a); }

    template <typename T>
    typename std::enable_if<std::is_same<T, bool>::value, T>::type
    convert(const char* a) {
        return !(strcasecmp(a, "false") == 0 ||
                 strcasecmp(a, "no")    == 0 ||
                 strcasecmp(a, "off")   == 0 ||
                 strcmp    (a, "0")     == 0);
    }

    template <typename T, typename Char = const char>
    bool match_opt(int argc, Char** argv, T* a_value, const std::string& a, int& i) {
        if (a.empty() || argv[i][0] != '-') return false;
        if (a == argv[i]) {
            if (!a_value && i < argc-1 && argv[i+1][0] != '-')
                return false;
            else if (a_value)
                *a_value = convert<T>
                    ((i+1 >= argc || argv[i+1][0] == '-') ? "" : argv[++i]);
            return true;
        }
        size_t n = strlen(argv[i]);
        if (a.size()+1 <= n && strncmp(a.c_str(), argv[i], a.size()) == 0 &&
                               argv[i][a.size()] == '=') {
            if (a_value)
                *a_value = convert<T>(argv[i] + a.size()+1);
            return true;
        }
        return false;
    };
}

/// Get command line option given its short name \a a_opt and optional long name
///
/// @param argc         argument passed to main()
/// @param argv         argument passed to main()
/// @param a_value      option value to be set (can be NULL)
/// @param a_opt        option short name (e.g. "-o")
/// @param a_long_opt   option long name (e.g. "--output")
/// @return true if given option is found in the \a argv list, in which case the
///         \a a_value is set to the value of the option (e.g. "-o filename",
///         "--output=filename").
template <typename T, typename Char = const char>
bool get_opt(int argc, Char** argv, T* a_value,
             const std::string& a_opt, const std::string& a_long_opt = "")
{
    if (a_opt.empty() && a_long_opt.empty()) return false;

    for (int i=1; i < argc; i++) {
        if (match_opt(argc, argv, a_value, a_opt,      i)) return true;
        if (match_opt(argc, argv, a_value, a_long_opt, i)) return true;
    }

    return false;
}

class opts_parser {
    int          m_argc;
    const char** m_argv;
    int          m_idx;

    static void store(int&    a_val, const char* s) { a_val = std::stoi(s); }
    static void store(long&   a_val, const char* s) { a_val = std::stol(s); }
    static void store(double& a_val, const char* s) { a_val = std::stod(s); }
    static void store(std::string& a,const char* s) { a     = s;            }

public:
    template <typename Char = const char>
    opts_parser(int a_argc, Char** a_argv)
        : m_argc(a_argc), m_argv(const_cast<const char**>(a_argv)), m_idx(0)
    {}

    /// Checks if \a a_opt is present at i-th position in the m_argv list
    /// @param a_opt name of the option to match
    bool match(const char* a_opt) const {
        return strcmp(m_argv[m_idx], a_opt) == 0;
    }

    template <typename T>
    bool match(const std::string& a_opt, T* a_value) {
        return match_opt(m_argc, m_argv, a_value, a_opt, m_idx);
    }

    bool match(const std::string& a_short, const std::string& a_long) {
        return match_opt(m_argc, m_argv, (int*)nullptr, a_short, m_idx)
            || match_opt(m_argc, m_argv, (int*)nullptr, a_long,  m_idx);
    }

    /// Match current option against \a a_short name or \a a_long name
    ///
    /// This function is to be used in the loop:
    /// \code
    /// opts_parser opts(argc, argv);
    /// while (!opts.next()) {
    ///     if (opts.match("-a", "", &a)) continue;
    ///     ...
    /// }
    /// \endcode
    template <typename T>
    bool match(const std::string& a_short, const std::string& a_long, T* a_val) {
        return match_opt(m_argc, m_argv, a_val, a_short, m_idx)
            || match_opt(m_argc, m_argv, a_val, a_long,  m_idx);
    }

    /// Find an option identified either by \a a_short name or \a a_long name
    ///
    /// Current fundtion doesn't change internal state variables of this class
    template <typename T>
    bool find(const std::string& a_short, const std::string& a_long, T* a_val) const {
        for (int i=1; i < m_argc; ++i)
            if (match_opt(m_argc, m_argv, a_val, a_short, i)
            ||  match_opt(m_argc, m_argv, a_val, a_long,  i))
                return true;
        return false;
    }

    bool            is_help() { return match("-h", "--help", (bool*)nullptr); }

    void            reset()        { m_idx = 0;               }
    bool            next()         { return ++m_idx < m_argc; }
    bool            end()    const { return m_idx >= m_argc;  }

    int             argc()   const { return m_argc; }
    const char**    argv()   const { return m_argv; }

    const char* operator()() const { return m_idx < m_argc ? m_argv[m_idx]:""; }
};


} // namespace utxx
