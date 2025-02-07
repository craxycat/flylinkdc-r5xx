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


#ifndef DCPLUSPLUS_DCPP_SEARCH_MANAGER_LISTENER_H
#define DCPLUSPLUS_DCPP_SEARCH_MANAGER_LISTENER_H

#include "SearchResult.h"

class SearchManagerListener
{
	public:
		virtual ~SearchManagerListener() { }
		template<int I> struct X
		{
			enum { TYPE = I };
		};
		
		typedef X<0> SR;
		virtual void on(SR, const SearchResult&) noexcept = 0;
};

#endif // !defined(SEARCH_MANAGER_LISTENER_H)

/**
 * @file
 * $Id: SearchManagerListener.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
