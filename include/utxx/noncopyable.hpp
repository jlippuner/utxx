//----------------------------------------------------------------------------
/// \file noncopyable.hpp
//----------------------------------------------------------------------------
/// This file contains a base class for noncopyable derived classes
//----------------------------------------------------------------------------
// Copyright (c) 2020 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-09-21
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

#include <boost/noncopyable.hpp>

namespace utxx {

using noncopyable = boost::noncopyable;

/*
class noncopyable {
public:
    noncopyable()  = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&)            = delete;
    noncopyable(noncopyable&&)                 = delete;
    noncopyable& operator=(const noncopyable&) = delete;
    noncopyable& operator=(noncopyable&&)      = delete;
};
*/

} // namespace utxx
