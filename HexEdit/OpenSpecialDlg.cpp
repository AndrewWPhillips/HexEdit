// OpenSpecialDlg.cpp : implements dialog for opening special "files" -
//                      disks, volumes, perhaps processes in the future
//
// Copyright (c) 2005 by Andrew W. Phillips.
//
// No restrictions are placed on the noncommercial use of this code,
// as long as this text (from the above copyright notice to the
// disclaimer below) is preserved.
//
// This code may be redistributed as long as it remains unmodified
// and is not sold for profit without the author's written consent.
//
// This code, or any part of it, may not be used in any software that
// is sold for profit, without the author's written consent.
//
// DISCLAIMER: This file is provided "as is" with no expressed or
// implied warranty. The author accepts no liability for any damage
// or loss of business that this product may cause.
//

#include "stdafx.h"
#include <winioctl.h>
#include <Dbt.h>        // for DBT_DEVICEREMOVECOMPLETE etc
#include "hexedit.h"
#include "OpenSpecialDlg.h"
#include "ntapi.h"

#include "HelpID.hm"            // User defined help IDs
#include "resource.hm"    // help IDs

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenSpecialDlg dialog


COpenSpecialDlg::COpenSpecialDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenSpecialDlg::IDD, pParent)
	, m_readonly(TRUE)
{
	//{{AFX_DATA_INIT(COpenSpecialDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_selected = -1;
	m_do_rebuild = false;
	//m_next_time = true;

	GetNTAPIFuncs();
}

void COpenSpecialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COpenSpecialDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_OPEN_TREE, m_ctl_tree);
	DDX_Control(pDX, IDC_OPEN_INFO, m_ctl_info);
	DDX_Control(pDX, IDC_PROP_DESC, m_ctl_desc);
	DDX_Check  (pDX, IDC_OPEN_READONLY, m_readonly);
	DDX_Control(pDX, IDC_OPEN_READONLY, m_ctl_readonly);
	DDX_Control(pDX, IDOK, m_ctl_ok);
}

BEGIN_MESSAGE_MAP(COpenSpecialDlg, CDialog)
	//{{AFX_MSG_MAP(COpenSpecialDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_OPEN_TREE, OnSelchangedOpenTree)
	ON_NOTIFY(NM_DBLCLK, IDC_OPEN_TREE, OnDblclkOpenTree)
	//}}AFX_MSG_MAP
	ON_NOTIFY(TVN_SELCHANGING, IDC_OPEN_TREE, OnSelchangingOpenTree)
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_DEVICECHANGE()
	ON_WM_CLOSE()
    ON_WM_TIMER()
	ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_USER+1, OnWmUser1)
	ON_MESSAGE(WM_USER+2, OnWmUser2)
	ON_BN_CLICKED(IDC_ALGORITHM_HELP, OnBnClickedAlgorithmHelp)
END_MESSAGE_MAP()

// Rebuild the tree nodes.  Does not change nodes if they already exist for the unique device id,
// but will add/delete nodes when devices appear/disappear.
void COpenSpecialDlg::Rebuild()
{
	CSpecialList *psl = theApp.GetSpecialList();
	ASSERT(psl != NULL && psl->size() > 0);     // make sure there is at least one in the list

	ASSERT(m_hVol != 0);
	int idx;									// current index into special list
	HTREEITEM hcurr = m_ctl_tree.GetChildItem(m_hVol);
	for (idx = 0; idx < psl->size(); ++idx)
	{
		// If now in physical drives get first current child
		if (idx > 0 && psl->type(idx-1) < psl->type(idx))
			hcurr = m_ctl_tree.GetChildItem(m_hPhys);

		short id;                               // unique id of current existing tree item
		while (hcurr != 0 && (id = (short)m_ctl_tree.GetItemData(hcurr)) < psl->id(idx))
		{
			HTREEITEM hh = hcurr;
			hcurr = m_ctl_tree.GetNextSiblingItem(hcurr);

			// Remove devices no longer present
			m_ctl_tree.DeleteItem(hh);
		}

		if (hcurr == 0 || id > psl->id(idx))
		{
			HTREEITEM hnew;                     // handle to new tree item
			HTREEITEM hpos;                     // item to insert after (why the f*** is there no insert before)
			int img;                            // image number for new tree item in tree ctl image list
			CString ss;

			if (hcurr == 0)
				hpos = TVI_LAST;                                          // "insert" at end
			else if ((hpos = m_ctl_tree.GetPrevSiblingItem(hcurr)) == 0)  // insert before hcurr (ie after previous)
				hpos = TVI_FIRST;                                         // no previous so insert at start

			switch (psl->type(idx))
			{
			case 0:
				ss = psl->DriveType(idx);
				if (ss == "Floppy")
					img  = IMG_FLOPPY;
				else if (ss == "Removable")
					img  = IMG_REMOVE;
				else if (ss == "Optical")
					img  = IMG_OPT_DRIVE;
				else if (ss == "Network")
					img  = IMG_NET;
				else
				{
					ASSERT(ss == "Fixed");
					img  = IMG_DRIVE;
				}
				hnew = m_ctl_tree.InsertItem(psl->nicename(idx), img, img, m_hVol, hpos);
				break;

			case 1:
				ASSERT(theApp.is_nt_);              // physical device only handled under NT/2K/XP
				ss = psl->filename(idx);
                if (_tcsncmp(ss, _T("\\\\.\\Floppy"), 10) == 0)
                    img = IMG_PHYS_FLOPPY;
                else if (_tcsncmp(ss, _T("\\\\.\\CdRom"), 9) == 0)
                    img = IMG_PHYS_CD;
				else
				{
				    img = IMG_PHYS_DRIVE;

					// Check if busy before testing removeable (which may block) since we want to create
					// the basic tree quickly.  This is fixed later (see SetItemImage call below).
					if (!psl->busy() && psl->removeable(idx))
						img = IMG_PHYS_REMOVE;
				}

				hnew = m_ctl_tree.InsertItem(psl->nicename(idx), img, img, m_hPhys, hpos);
				break;

			default:
				ASSERT(0);
			}

			m_ctl_tree.SetItemData(hnew, (LPARAM)psl->id(idx));
		}
		else
			hcurr = m_ctl_tree.GetNextItem(hcurr, TVGN_NEXT);  // skip this one (assume nothing changed)

	}
	m_ctl_tree.Expand(m_hVol, TVE_EXPAND);
}

// Update the currently selected item in the tree
void COpenSpecialDlg::Update()
{
	// Disable these for case where nothing selected
	m_ctl_readonly.EnableWindow(FALSE);
	m_ctl_ok.EnableWindow(FALSE);

	HTREEITEM hh = m_ctl_tree.GetSelectedItem();
	short id;                       // unique id for device

	if (hh != NULL && (id = (short)m_ctl_tree.GetItemData(hh)) != -1)
	{
		CWaitCursor wc;

		CSpecialList *psl = theApp.GetSpecialList();
		CString strInfo, ss;

		int idx = theApp.GetSpecialList()->find(id);
		ASSERT(idx != -1);

		// Update image for removeable drives since we might not have known it was removeable in Rebuild() above
		if (psl->removeable(idx))
		{
			int img;
			m_ctl_tree.GetItemImage(hh, img, img);
			if (psl->type(idx) == 0 && img == IMG_DRIVE)
                m_ctl_tree.SetItemImage(hh, IMG_REMOVE, IMG_REMOVE);
			else if (psl->type(idx) == 1 && img == IMG_PHYS_DRIVE)
                m_ctl_tree.SetItemImage(hh, IMG_PHYS_REMOVE, IMG_PHYS_REMOVE);
		}

		if (psl->read_only(idx))
		{
			m_ctl_readonly.EnableWindow();
			m_ctl_readonly.SetCheck(1);
			m_ctl_readonly.EnableWindow(FALSE);
			m_readonly = TRUE;
		}
		else
		{
			// Enable control but don't change m_readonly
			m_ctl_readonly.EnableWindow();
		}

		// Display name of device in header (static text control)
		ss.Format("%s  Properties:", psl->name(idx));
		m_ctl_desc.SetWindowText(ss);

		// Display info that does not depend on disk being present
		if (psl->type(idx) == 0)
			strInfo += "Type: " + psl->DriveType(idx) + " Drive\r\n";
		if (psl->type(idx) == 1)
		{
			ss = psl->Vendor(idx);
			if (!ss.IsEmpty())
				strInfo += "Vendor:" + ss + "\r\n";
			ss = psl->Product(idx);
			if (!ss.IsEmpty())
				strInfo += "Model:" + ss + "\r\n";
			ss = psl->Revision(idx);
			if (!ss.IsEmpty())
				strInfo += "Revision:" + ss + "\r\n";
		}

		// Display medium info or reason why we couldn't (no medium or some sort of error)
		DWORD error = psl->error(idx);
		if (error != NO_ERROR)
		{
			LPVOID lpMsgBuf;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							  NULL,
							  error,
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							  (LPTSTR) &lpMsgBuf,
							  0,
							  NULL))
			{
				strInfo += "Error: " + CString((LPCTSTR)lpMsgBuf) + "\r\n";
			}
			else
				strInfo += "Error reading device\r\n";
		}
		else if (!psl->medium_present(idx))
		{
			strInfo += "Status: no medium present\r\n";
		}
		else
		{
			char buf[32];
			m_ctl_ok.EnableWindow();

			switch (psl->type(idx))
			{
			case 0:
				ss = psl->VolumeName(idx);
				if (!ss.IsEmpty())
					strInfo += "Volume: " + ss + "\r\n";
				ss = psl->FileSystem(idx);
				if (!ss.IsEmpty())
					strInfo += "File System: " + ss + "\r\n";

				// Next show cluster size (as it is filesystem related)
				ss.Format("Cluster size: %d sectors\r\n", psl->SectorsPerCluster(idx));
			    strInfo += ss;
				ss.Format("Sector size: %d bytes\r\n", psl->sector_size(idx));
			    strInfo += ss;

				ss.Format("Used space: %d%%\r\n", int(((psl->TotalClusters(idx) - __int64(psl->FreeClusters(idx)))*100)/psl->TotalClusters(idx)));
			    strInfo += ss;
				if (psl->FreeClusters(idx) > 0)
                {
					ss.Format("Free space: %sb\r\n", NumScale((double)psl->FreeClusters(idx) *
						                                      (double)psl->SectorsPerCluster(idx) *
															  (double)psl->sector_size(idx))); 
			        strInfo += ss;
					sprintf(buf, "%I64d", psl->FreeClusters(idx) * __int64(psl->SectorsPerCluster(idx)) * psl->sector_size(idx));
					ss = buf;
					AddCommas(ss);
					strInfo += "  = " + ss + " bytes\r\n";
					ss.Format("%d", psl->FreeClusters(idx));
					AddCommas(ss);
					strInfo += "  = " + ss + " clusters\r\n";
                }
                //ss.Format("Capacity: %sb\r\n", NumScale((double)psl->TotalClusters(idx) *
                //                                        (double)psl->SectorsPerCluster(idx) *
                //                                        (double)psl->sector_size(idx))); 
				ss.Format("Capacity: %sb\r\n", NumScale((double)psl->total_size(idx)));
			    strInfo += ss;
				sprintf(buf, "%I64d", __int64(psl->total_size(idx)));
				ss = buf;
				AddCommas(ss);
				strInfo += "  = " + ss + " bytes\r\n";
				ss.Format("%d", psl->TotalClusters(idx));
				AddCommas(ss);
				strInfo += "  = " + ss + " clusters\r\n";

				break;

			case 1:
				ss.Format("Sector size: %ld bytes\r\n", (long)psl->sector_size(idx));
				strInfo += ss;
				ss.Format("Sectors/track: %ld\r\n", (long)psl->SectorsPerTrack(idx));
				strInfo += ss;
				ss.Format("Tracks/cylinder: %ld\r\n", (long)psl->TracksPerCylinder(idx));
				strInfo += ss;
				ss.Format("Cylinders: %ld\r\n", (long)psl->Cylinders(idx));
				strInfo += ss;

				ss.Format("Capacity: %sb\r\n", NumScale((double)psl->total_size(idx))); 
				strInfo += ss;
				sprintf(buf, "%I64d", __int64(psl->total_size(idx)));
				ss = buf;
				AddCommas(ss);
				strInfo += "  = " + ss + " bytes\r\n";
				break;

			default:
				ASSERT(0);
			}
		}

		m_ctl_info.SetWindowText(strInfo);
	}
}

/////////////////////////////////////////////////////////////////////////////
// COpenSpecialDlg message handlers

BOOL COpenSpecialDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

    // Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(TRUE);
    resizer_.Add(IDC_OPEN_TREE,        0,   0, 100, 100);
    resizer_.Add(IDC_PROP_DESC,      100,   0,   0,   0);
    resizer_.Add(IDC_OPEN_INFO,      100,   0,   0, 100);
    resizer_.Add(IDC_OPEN_READONLY,  100, 100,   0,   0);
    resizer_.Add(IDC_ALGORITHM_HELP, 100, 100,   0,   0);
    resizer_.Add(IDOK,               100, 100,   0,   0);
    resizer_.Add(IDCANCEL,           100, 100,   0,   0);

	// Set up tree control
	m_ctl_tree.ModifyStyle(0, TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);
    m_imagelist.Create(MAKEINTRESOURCE(IDB_DEVIMAGES), 16, 1, RGB(0xFF, 0xFF, 0xFF));
    m_ctl_tree.SetImageList(&m_imagelist, TVSIL_NORMAL);

	// Add root level nodes to tree
	m_hVol  = m_ctl_tree.InsertItem("Logical Volumes", IMG_DRIVES, IMG_DRIVES);
	m_ctl_tree.SetItemData(m_hVol, (LPARAM)-1);
	m_hPhys = 0;
	if (theApp.is_nt_)
	{
		// Only allow access to physical devices under NT/2K/XP
		m_hPhys = m_ctl_tree.InsertItem("Physical Drives", IMG_PHYS_DRIVES, IMG_PHYS_DRIVES);
		m_ctl_tree.SetItemData(m_hPhys, (LPARAM)-1);
	}

	Rebuild();                          // Add nodes for all devices
    SetTimer(1, 5000, NULL);           // update removeable device every 5 seconds

	return TRUE;
}

void COpenSpecialDlg::OnClose()
{
	CDialog::OnClose();
}

void COpenSpecialDlg::OnOK()
{
	HTREEITEM hh = m_ctl_tree.GetSelectedItem();
	if (hh == 0)
		return;                         // Don't exit dlg if nothing selected

    // Get tree item selected
	m_selected = (short)m_ctl_tree.GetItemData(hh);
    if (m_selected == -1)
        return;                         // Don't exit dlg if leaf not selected

	int idx;
	if ((idx = theApp.GetSpecialList()->find(m_selected)) == -1)
	{
		ASSERT(0);
		AfxMessageBox("Device is no longer present");
		return;
	}

	if (theApp.GetSpecialList()->error(idx) != NO_ERROR)
	{
		AfxMessageBox("No medium present");
		return;
	}
	else if (!theApp.GetSpecialList()->medium_present(idx))
	{
		AfxMessageBox("No medium present");
		return;
	}

	CDialog::OnOK();
}

static DWORD id_pairs[] = { 
    IDC_OPEN_TREE, HIDC_OPEN_TREE,
    IDC_OPEN_READONLY, HIDC_OPEN_READONLY,
    IDC_PROP_DESC, HIDC_OPEN_INFO,
    IDC_OPEN_INFO, HIDC_OPEN_INFO,
    0,0
};

BOOL COpenSpecialDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs);
    return TRUE;
}

void COpenSpecialDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu((HWND)pWnd->GetSafeHwnd(), id_pairs);
}

LRESULT COpenSpecialDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
    return FALSE;
}

void COpenSpecialDlg::OnTimer(UINT nIDEvent)
{
    ASSERT(nIDEvent == 1);
	// This has been obviated by new system and checking if theApp.GetSpecialList()->busy()
	//if (m_do_rebuild && m_next_time)
	//{
	//	// This avoids doing rebuilds on 2 consecutive timer events - this happens often when USB device
	//	// is removed as we seem to get several DEVNODES_CHANGED events over several seconds.
	//	m_next_time = false;
	//}
	//else
	if (m_do_rebuild)
	{
		if (!theApp.GetSpecialList()->busy())
		{
			theApp.GetSpecialList()->refresh(m_hWnd, -1);       // rebuild whole tree (WM_USER + 1)

			// Only clear m_do_rebuild if we are *sure* we have started a full rebuild (-1 passed
			// to refresh), since the special list could be locked just to update a single item.
			m_do_rebuild = false;
		}
		//m_next_time = true;
	}
	else
    {
		// Refresh currently selected item if medium could change (ie, removeable drive)
	    short id;                       // unique id for device
		int idx;                        // index into special list
	    HTREEITEM hh = m_ctl_tree.GetSelectedItem();
	    if (hh != NULL &&
            (id = (short)m_ctl_tree.GetItemData(hh)) != -1 &&
			(idx = theApp.GetSpecialList()->find(id)) != -1 &&
			!theApp.GetSpecialList()->busy() &&
			theApp.GetSpecialList()->removeable(idx))
		{
			theApp.GetSpecialList()->refresh(m_hWnd, id);       // refresh this element (WM_USER + 2)
		}
    }
}

BOOL COpenSpecialDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
    if (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE)
	{
		// Media has been removed/inserted so update info in case it was the device we are looking at
	    short id;                       // unique id for device
		int idx;                        // index into special list
	    HTREEITEM hh = m_ctl_tree.GetSelectedItem();
	    if (hh != NULL &&
            (id = (short)m_ctl_tree.GetItemData(hh)) != -1 &&
			(idx = theApp.GetSpecialList()->find(id)) != -1 &&
			!theApp.GetSpecialList()->busy() &&
			theApp.GetSpecialList()->removeable(idx))
		{
			theApp.GetSpecialList()->refresh(m_hWnd, id);       // refresh this element (WM_USER + 2)
		}
	}
    else if (nEventType == DBT_DEVNODES_CHANGED || nEventType == DBT_CONFIGCHANGED)
	{
		// A volume has been added or removed so we need to completely rebuild the list of devices
		// Note: We can get several of these messages related to the one "event" so we just make a note
		// of this and then rebuild the whole tree later (in OnTimer).
		m_do_rebuild = true;
	}

    return TRUE;
}

LRESULT COpenSpecialDlg::OnWmUser1(WPARAM, LPARAM)
{
	Rebuild();

    return FALSE;
}

LRESULT COpenSpecialDlg::OnWmUser2(WPARAM, LPARAM)
{
	Update();

    return FALSE;
}

void COpenSpecialDlg::OnSelchangedOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	Update();

	*pResult = 0;
}

void COpenSpecialDlg::OnSelchangingOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	m_ctl_info.SetWindowText(_T(""));
	m_ctl_desc.SetWindowText(_T(""));

	*pResult = 0;
}

void COpenSpecialDlg::OnDblclkOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Check if there is a valid selection and close the dialog
	OnOK();

	*pResult = 0;
}

void COpenSpecialDlg::OnBnClickedAlgorithmHelp()
{
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_OPEN_SPECIAL_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}
