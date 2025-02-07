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


#ifndef DCPLUSPLUS_DCPP_BLOOM_FILTER_H
#define DCPLUSPLUS_DCPP_BLOOM_FILTER_H

#include "typedefs.h"

template<size_t N>
class BloomFilter
{
	public:
		explicit BloomFilter(size_t tableSize)
		{
			table.resize(tableSize);
		}
		~BloomFilter() { }
		
		void add(const string& s)
		{
			xadd(s, N);
		}
		bool match(const StringList& s) const
		{
			for (auto i = s.cbegin(); i != s.cend(); ++i)
			{
				if (!match(*i))
					return false;
			}
			return true;
		}
		bool match(const string& s) const
		{
			if (s.length() >= N)
			{
				string::size_type l = s.length() - N;
				for (string::size_type i = 0; i <= l; ++i)
				{
					if (!table[getPos(s, i, N)])
					{
						return false;
					}
				}
			}
			return true;
		}
		void clear()
		{
#ifdef _DEBUG
			std::vector<bool> l_test_vector = table;
			l_test_vector.clear();
			l_test_vector.resize(table.size());
#endif
			std::fill_n(table.begin(), table.size(), false);
			dcassert(l_test_vector == table);
		}
#ifdef TESTER
		void print_table_status()
		{
			int tot = 0;
			for (unsigned int i = 0; i < table.size(); ++i) if (table[i] == true) ++tot;
			
			std::cout << "table status: " << tot << " of " << table.size()
			          << " filled, for an occupancy percentage of " << (100.*tot) / table.size()
			          << '%' << std::endl;
		}
#endif
	private:
		void xadd(const string& s, size_t n)
		{
			if (s.length() >= n)
			{
				string::size_type l = s.length() - n;
				for (string::size_type i = 0; i <= l; ++i)
				{
					table[getPos(s, i, n)] = true;
				}
			}
		}
		
		/* This is roughly how boost::hash does it */
		size_t getPos(const string& s, size_t i, size_t l) const
		{
			size_t h = 0;
			const char* c = s.data() + i;
			const char* end = s.data() + i + l;
			for (; c < end; ++c)
			{
				h ^= *c + 0x9e3779b9 + (h << 6) + (h >> 2); //-V104
			}
			return (h % table.size());
		}
		
		vector<bool> table;
};

#endif // !defined(BLOOM_FILTER_H)

/**
 * @file
 * $Id: BloomFilter.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
