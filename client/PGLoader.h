
#pragma once

#ifndef PGLOADER_H
#define PGLOADER_H

#include "SettingsManager.h"

#ifdef PPA_INCLUDE_IPFILTER

#include "iplist.h"

class PGLoader
{
	public:
		PGLoader()
		{
		}
		
		~PGLoader()
		{
		}
		static bool check(uint32_t p_ip4);
		static void addLine(string& p_Line, CFlyLog& p_log);
		static void load(const string& p_data = Util::emptyString);
		static string getConfigFileName()
		{
			return Util::getConfigPath() + "IPTrust.ini";
		}
	private:
		static FastCriticalSection g_cs;
		static IPList  g_ipTrustListAllow;
		static IPList  g_ipTrustListBlock;
};
#endif // PPA_INCLUDE_IPFILTER

#endif // PGLOADER_H
