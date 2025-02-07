/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#pragma once

#ifndef DCPLUSPLUS_DCPP_NOEXCEPT_H
#define DCPLUSPLUS_DCPP_NOEXCEPT_H

// for compilers that don't support noexcept, use an exception specifier

#ifdef __GNUC__
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6) // GCC 4.6 is the first GCC to implement noexcept.

#ifndef noexcept
#define noexcept throw()
#endif

#endif
#elif defined(_MSC_VER)

#  if (_MSC_VER < 1900) // [+] FlylinkDC++ VC++2015
#ifndef noexcept
#define noexcept throw()
#endif

#endif

#endif

#endif
