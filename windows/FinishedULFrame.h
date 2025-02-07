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

#if !defined(FINISHED_UL_FRAME_H)
#define FINISHED_UL_FRAME_H

#pragma once

#include "FinishedFrameBase.h"

class FinishedULFrame : public FinishedFrameBase<FinishedULFrame, ResourceManager::FINISHED_UPLOADS, IDC_FINISHED_UL, IDR_FINISHED_UL>
{
	public:
		FinishedULFrame(): FinishedFrameBase(e_TransferUpload)
		{
			m_type = FinishedManager::e_Upload;
			boldFinished = SettingsManager::BOLD_FINISHED_UPLOADS;
			columnOrder = SettingsManager::FINISHED_UL_ORDER;
			columnWidth = SettingsManager::FINISHED_UL_WIDTHS;
			columnVisible = SettingsManager::FINISHED_UL_VISIBLE;
		}
		
		~FinishedULFrame() { }
		
		DECLARE_FRAME_WND_CLASS_EX(_T("FinishedULFrame"), IDR_FINISHED_UL, 0, COLOR_3DFACE);
		
	private:
	
		void on(AddedUl, const FinishedItemPtr& p_entry, bool p_is_sqlite) noexcept override
		{
			PostMessage(WM_SPEAKER, SPEAK_ADD_LINE, (WPARAM)new FinishedItemPtr(p_entry));
		}
		
		void on(RemovedUl, const FinishedItemPtr& p_entry) noexcept override// [+] IRainman http://code.google.com/p/flylinkdc/issues/detail?id=601
		{
			PostMessage(WM_SPEAKER, SPEAK_REMOVE_LINE, (WPARAM)new FinishedItemPtr(p_entry));
		}
};

#endif // !defined(FINISHED_UL_FRAME_H)

/**
 * @file
 * $Id: FinishedULFrame.h 310 2007-07-22 19:37:44Z bigmuscle $
 */
