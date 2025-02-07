/*
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
 * Copyright (C) 2011      Big Muscle, http://strongdc.sf.net
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
#include "SharedFileStream.h"
#include "LogManager.h"
#include "ClientManager.h"
#include "../FlyFeatures/flyServer.h"

FastCriticalSection SharedFileStream::g_shares_file_cs;
#ifdef FLYLINKDC_USE_SHARED_FILE_STREAM_RW_POOL
SharedFileStream::SharedFileHandleMap SharedFileStream::g_readpool;
SharedFileStream::SharedFileHandleMap SharedFileStream::g_writepool;
#else
SharedFileStream::SharedFileHandleMap SharedFileStream::g_rwpool;
std::unordered_set<std::string> SharedFileStream::g_shared_stream_errors;
#endif

SharedFileStream::SharedFileStream(const string& aFileName, int aAccess, int aMode)
{
	dcassert(!aFileName.empty());
	CFlyFastLock(g_shares_file_cs);
#ifdef FLYLINKDC_USE_SHARED_FILE_STREAM_RW_POOL
	auto& pool = aAccess == File::READ ? g_readpool : g_writepool;
#else
	auto& pool = g_rwpool;
#endif
	auto p = pool.find(aFileName);
	if (p != pool.end())
	{
#ifdef _DEBUG
		// LogManager::message("Share SharedFileHandle aFileName = " + aFileName);
#endif
		m_sfh = p->second.get();
		m_sfh->m_ref_cnt++;
		dcassert(m_sfh->m_access == aAccess);
		dcassert(m_sfh->m_mode == aMode);
	}
	else
	{
#ifdef _DEBUG
		// LogManager::message("new SharedFileHandle aFileName = " + aFileName);
#endif
		m_sfh = new SharedFileHandle(aFileName, aAccess, aMode);
		try
		{
			m_sfh->init();
		}
		catch (FileException& e)
		{
			safe_delete(m_sfh);
			const auto l_error = "error r5xx SharedFileStream::SharedFileStream aFileName = "
			                     + aFileName + " Error = " + e.getError() + " Access = " + Util::toString(aAccess) + " Mode = " + Util::toString(aMode);
			const auto l_dup_filter = g_shared_stream_errors.insert(l_error);
			if (l_dup_filter.second == true)
			{
				CFlyServerJSON::pushError(9, l_error);
				const tstring l_email_message = Text::toT(string("\r\nError in SharedFileStream::SharedFileStream. aFileName = [") + aFileName + "]\r\n" +
				                                          "Error = " + e.getError() + "\r\nSend screenshot (or text - press ctrl+c for copy to clipboard) e-mail ppa74@ya.ru for diagnostic error!");
				::MessageBox(NULL, l_email_message.c_str(), T_APPNAME_WITH_VERSION, MB_OK | MB_ICONERROR);
			}
			else
			{
				LogManager::message(l_error);
			}
			throw;
		}
		pool[aFileName] = unique_ptr<SharedFileHandle>(m_sfh);
	}
}

void SharedFileStream::check_before_destoy()
{
	{
		CFlyFastLock(g_shares_file_cs);
#ifdef FLYLINKDC_USE_SHARED_FILE_STREAM_RW_POOL
		auto& pool = m_sfh->m_mode == File::READ ? g_readpool : g_writepool;
#else
		auto& pool = g_rwpool;
#endif
		dcassert(pool.empty());
	}
	cleanup();
}
// TODO - ������
void SharedFileStream::cleanup()
{
	CFlyFastLock(g_shares_file_cs);
#ifdef FLYLINKDC_USE_SHARED_FILE_STREAM_RW_POOL
	auto& pool = m_sfh->m_mode == File::READ ? g_readpool : g_writepool;
#else
	auto& pool = g_rwpool;
#endif
	for (auto i = pool.begin(); i != pool.end();)
	{
		if (i->second && i->second->m_ref_cnt == 0)
		{
			dcassert(0); // ��������� � SharedFileStream::~SharedFileStream()
#ifdef _DEBUG
			//  LogManager::message("[!] SharedFileStream::cleanup() aFileName = " + i->first);
#endif
			pool.erase(i);
			i = pool.begin();
		}
		else
		{
			++i;
		}
	}
}
SharedFileStream::~SharedFileStream()
{
	CFlyFastLock(g_shares_file_cs);
	
	m_sfh->m_ref_cnt--;
	if (m_sfh->m_ref_cnt == 0)
	{
#ifdef _DEBUG
		LogManager::message("m_ref_cnt = 0 ~SharedFileHandle aFileName = " + m_sfh->m_path);
#endif
#ifdef FLYLINKDC_USE_SHARED_FILE_STREAM_RW_POOL
		auto& pool = m_sfh->m_mode == File::READ ? g_readpool : g_writepool;
#else
		auto& pool = g_rwpool;
#endif
		dcassert(pool.find(m_sfh->m_path) != pool.end());
		pool.erase(m_sfh->m_path);
	}
}

size_t SharedFileStream::write(const void* buf, size_t len)
{
	CFlyFastLock(m_sfh->m_cs);
#ifdef _DEBUG
	// LogManager::message("SharedFileStream::write buf = " + Util::toString(int(buf)) + " len " + Util::toString(len));
#endif
	m_sfh->m_file.setPos(m_pos);
	m_sfh->m_file.write(buf, len); // https://crash-server.com/DumpGroup.aspx?ClientID=ppa&DumpGroupID=132490
	
	m_pos += len;
	return len;
}

size_t SharedFileStream::read(void* buf, size_t& len)
{
	CFlyFastLock(m_sfh->m_cs);
#ifdef _DEBUG
	// LogManager::message("SharedFileStream::read buf = " + Util::toString(int(buf)) + " len " + Util::toString(len));
#endif
	
	m_sfh->m_file.setPos(m_pos);
	len = m_sfh->m_file.read(buf, len);
	
	m_pos += len;
	return len;
}

int64_t SharedFileStream::getSize() const
{
	CFlyFastLock(m_sfh->m_cs);
#ifdef _DEBUG
	// LogManager::message("SharedFileStream::getSize size = " +  Util::toString(m_sfh->m_file.getSize()));
#endif
	return m_sfh->m_file.getSize();
}

void SharedFileStream::setSize(int64_t newSize)
{
	CFlyFastLock(m_sfh->m_cs);
#ifdef _DEBUG
	// LogManager::message("SharedFileStream::setSize size = " +  Util::toString(newSize));
#endif
	m_sfh->m_file.setSize(newSize);
}

size_t SharedFileStream::flush()
{
	if (!ClientManager::isShutdown()) // fix https://drdump.com/Problem.aspx?ProblemID=130529
		// ��� �������� ������ - ������ � ��� ����������� �� �����.
	{
		try
		{
			CFlyFastLock(m_sfh->m_cs);
			return m_sfh->m_file.flush();
		}
		catch (const Exception& e)
		{
			dcassert(0);
			LogManager::message("SharedFileStream::flush() = " + e.getError());
		}
	}
	return 0;
}

void SharedFileStream::setPos(int64_t aPos)
{
	CFlyFastLock(m_sfh->m_cs);
#ifdef _DEBUG
	// LogManager::message("SharedFileStream::setPos aPos = " +  Util::toString(aPos));
#endif
	m_pos = aPos;
}


