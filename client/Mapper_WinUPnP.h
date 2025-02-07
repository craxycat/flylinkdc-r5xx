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


#ifndef DCPLUSPLUS_DCPP_MAPPER_WINUPNP_H
#define DCPLUSPLUS_DCPP_MAPPER_WINUPNP_H

#include "Mapper.h"

struct IUPnPNAT;
struct IStaticPortMappingCollection;

/// @todo this class is far from complete (should register callbacks, etc)
class Mapper_WinUPnP : public Mapper
{
	public:
		Mapper_WinUPnP() : Mapper(), pUN(0), lastPort(0), lastProtocol(PROTOCOL_TCP) {}
		
		static const string g_name;
		
	private:
		bool init();
		void uninit();
		
		bool add(const unsigned short port, const Protocol protocol, const string& description);
		bool remove(const unsigned short port, const Protocol protocol);
		
		uint32_t renewal() const
		{
			return 0;
		}
		
		string getDeviceName() const
		{
			/// @todo use IUPnPDevice::ModelName <http://msdn.microsoft.com/en-us/library/aa381670(VS.85).aspx>?
			return "";
		}
		string getExternalIP();
		string getModelDescription() const
		{
			return "WinUPnP";
		}
		
		const string& getMapperName() const
		{
			return g_name;
		}
		IUPnPNAT* pUN;
		// this one can become invalid so we can't cache it
		IStaticPortMappingCollection* getStaticPortMappingCollection();
		
		// need to save these to get the external IP...
		unsigned short lastPort;
		Protocol lastProtocol;
};

#endif
