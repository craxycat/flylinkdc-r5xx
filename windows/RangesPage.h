#ifndef RANGES_PAGE_H
#define RANGES_PAGE_H


#pragma once


#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class RangesPage : public CPropertyPage<IDD_RANGES_PAGE>, public PropPage
{
	public:
		explicit RangesPage() : PropPage(TSTRING(IPGUARD)), m_isEnabledIPGuard(false)
		{
			SetTitle(m_title.c_str());
		}
		
		~RangesPage()
		{
			ctrlPolicy.Detach();
		}
		
		BEGIN_MSG_MAP_EX(RangesPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ENABLE_IPGUARD, onFixControls)
		CHAIN_MSG_MAP(PropPage)
		END_MSG_MAP()
		
		LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
		
		LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
		LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
		
		LRESULT onFixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			fixControls();
			return 0;
		}
		
		LRESULT onDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
		{
			NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
			
			if (item->iItem >= 0)
			{
				PostMessage(WM_COMMAND, IDC_CHANGE, 0);
			}
			else if (item->iItem == -1)
			{
				PostMessage(WM_COMMAND, IDC_ADD, 0);
			}
			
			return 0;
		}
		
		// Common PropPage interface
		PROPSHEETPAGE *getPSP()
		{
			return (PROPSHEETPAGE *) * this;
		}
		void write();
		void cancel()
		{
			cancel_check();
		}
	protected:
		CComboBox ctrlPolicy;
		
		static Item items[];
		static TextItem texts[];
		
		void fixControls();
	private:
		string m_IPGuard;
		string m_IPGuardPATH;
		string m_IPFilter;
		string m_ManualP2PGuard;
		string m_IPFilterPATH;
		bool m_isEnabledIPGuard;
};

#endif // RANGES_PAGE_H