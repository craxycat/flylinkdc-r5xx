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

#include "stdinc.h"
#include "Mapper_NATPMP.h"
#include "Util.h"
#include "LogManager.h"

extern "C" {
#ifndef STATICLIB
#define STATICLIB
#endif
#include "../natpmp/getgateway.h"
#include "../natpmp/natpmp.h"
}

const string Mapper_NATPMP::g_name = "NAT-PMP";

static natpmp_t g_nat;

bool Mapper_NATPMP::init()
{
	// the lib normally handles this but we call it manually to store the result (IP of the router).
	in_addr addr = {0};
	if (getdefaultgateway(reinterpret_cast<in_addr_t*>(&addr.s_addr)) < 0)
		return false;
	m_gateway = inet_ntoa(addr);
	const auto l_result = initnatpmp(&g_nat, 1, addr.s_addr);
	dcassert(l_result == 0);
	if (l_result)
	{
		LogManager::message("Mapper_NATPMP::init error = "
		                    + Util::toString(l_result) + " WSAGetLastError() = " + Util::toString(WSAGetLastError()));
	}
	
	return initnatpmp(&g_nat, 1, addr.s_addr) >= 0;
}

void Mapper_NATPMP::uninit()
{
	const auto l_result = closenatpmp(&g_nat);
	dcassert(l_result == 0);
	if (l_result)
	{
		LogManager::message("Mapper_NATPMP::uninit error = " + Util::toString(l_result));
	}
}

int reqType(const Mapper::Protocol protocol)
{
	if (protocol == Mapper::PROTOCOL_TCP)
	{
		return NATPMP_PROTOCOL_TCP;
	}
	else
	{
		dcassert(protocol == Mapper::PROTOCOL_UDP);
		return NATPMP_PROTOCOL_UDP;
	}
}

int respType(const Mapper::Protocol protocol)
{
	if (protocol == Mapper::PROTOCOL_TCP)
	{
		return NATPMP_RESPTYPE_TCPPORTMAPPING;
	}
	else
	{
		dcassert(protocol == Mapper::PROTOCOL_UDP);
		return NATPMP_RESPTYPE_UDPPORTMAPPING;
	}
}

bool sendRequest(const unsigned short port, const Mapper::Protocol protocol, uint32_t lifetime)
{
	return sendnewportmappingrequest(&g_nat, reqType(protocol), port, port, lifetime) >= 0;
}

bool read(natpmpresp_t& response)
{
	int res;
	do
	{
		// wait for the previous request to complete.
		timeval timeout;
		if (getnatpmprequesttimeout(&g_nat, &timeout) >= 0)
		{
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(g_nat.s, &fds);
			select(FD_SETSIZE, &fds, 0, 0, &timeout);
		}
		
		res = readnatpmpresponseorretry(&g_nat, &response);
	}
	while (res == NATPMP_TRYAGAIN && g_nat.try_number <= 5);  // don't wait for 9 failures as that takes too long.
	return res >= 0;
}

bool Mapper_NATPMP::add(const unsigned short port, const Protocol protocol, const string&)
{
	if (sendRequest(port, protocol, 3600))
	{
		natpmpresp_t response;
		if (read(response) && response.type == respType(protocol) && response.pnu.newportmapping.mappedpublicport == port)
		{
			lifetime = std::min(3600u, response.pnu.newportmapping.lifetime) / 60;
			return true;
		}
	}
	return false;
}

bool Mapper_NATPMP::remove(const unsigned short port, const Protocol protocol)
{
	if (sendRequest(port, protocol, 0))
	{
		natpmpresp_t response;
		return read(response) && response.type == respType(protocol) && response.pnu.newportmapping.mappedpublicport == port;
	}
	return false;
}


string Mapper_NATPMP::getExternalIP()
{
	if (sendpublicaddressrequest(&g_nat) >= 0)
	{
		natpmpresp_t response;
		if (read(response) && response.type == NATPMP_RESPTYPE_PUBLICADDRESS)
		{
			return inet_ntoa(response.pnu.publicaddress.addr);
		}
	}
	return Util::emptyString;
}
