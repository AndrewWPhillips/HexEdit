// Dialog.cpp : implementation file
//
// Copyright (c) 1999-2010 by Andrew W. Phillips.
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
#include <sys/stat.h>
#include <shlwapi.h>    // for PathRemoveFileSpec

#include "HexEdit.h"
#include "MainFrm.h"
#include "Dialog.h"
#include "HexFileList.h"
#include "CFile64.h"
#include <Dlgs.h>               // For file dialog control IDs
#include "resource.hm"
#include "HelpID.hm"            // User defined help IDs

#include <HtmlHelp.h>

extern CHexEditApp theApp;
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

#ifndef  SFGAO_ENCRYPTED
#define  SFGAO_ENCRYPTED  0x00002000L
#define  SFGAO_STREAM     0x00400000L
#endif

#ifndef SHGFI_ATTR_SPECIFIED
#define SHGFI_ATTR_SPECIFIED    0x000020000     // get only specified attributes
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHexDialogBar - dialog bar that supports DDX and OnInitDialog

IMPLEMENT_DYNAMIC(CHexDialogBar, CDialogBar)

BEGIN_MESSAGE_MAP(CHexDialogBar, CDialogBar)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	//ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	ON_MESSAGE(WM_USER+1, InitDialogBarHandler)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexDialogBar message handlers

int CHexDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	PostMessage(WM_USER+1, 0, 0);
	return 0;
}

BOOL CHexDialogBar::OnEraseBkgnd(CDC *pDC)
{
	CRect rct;
	GetClientRect(&rct);

	// Fill the background with a colour that matches the current BCG theme (and hence sometimes with the Windows Theme)
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	return TRUE;
}

HBRUSH CHexDialogBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		// Draw static controls with transparent background else they look
		// aweful against the background colour (see OnEraseBkgnd above).
		pDC->SetBkMode(TRANSPARENT);
		return(HBRUSH) ::GetStockObject(HOLLOW_BRUSH);
	}
	else
		return CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

/////////////////////////////////////////////////////////////////////////////
// CHexDialogBar public methods

LRESULT CHexDialogBar::InitDialogBarHandler(WPARAM, LPARAM)
{
	UpdateData(FALSE);
	OnInitDialog();
    return FALSE;
}

#if 0 // xxx
void CHexDialogBar::Unroll()
{
    if (IsFloating())
    {
	    // To programatically unrol we need to turn on dynamic unroll, which does
	    // not work unless m_pDockContext is NULL.
        CDockContext *tmp;
        tmp = m_pDockContext;
        m_pDockContext = NULL;
        EnableRollUp(BCG_ROLLUP_DYNAMIC_ON);
        SetRollupState(BCG_ROLLUP_STATE_UNROLL);
        EnableRollUp(BCG_ROLLUP_NORMAL);
        m_pDockContext = tmp;
    }
}

void CHexDialogBar::FixAndFloat(BOOL show /*=FALSE*/)
{
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    ASSERT(mm != NULL);
    CPoint pt(0,0);                     // Where the dialogs are positioned in screen coords
    mm->ClientToScreen(&pt);

    mm->ShowControlBar(this, show, FALSE);   // Hide or show
    mm->FloatControlBar(this, pt);           // Undock in case it is docked weirdly or in wrong place
    Unroll();                                // Make sure it is not rolled up
}
#endif

/* Here we have CExplorerWnd and related classes */

/////////////////////////////////////////////////////////////////////////////
// CHistoryShellList - keeps history of folders that have been shown

// This is a virtual function that is called whenever the current folder is to change
HRESULT CHistoryShellList::DisplayFolder(LPAFX_SHELLITEMINFO lpItemInfo)
{
    HRESULT retval = CMFCShellListCtrl::DisplayFolder(lpItemInfo);

    if (retval == S_OK && pExpl_ != NULL && !in_move_)
	{
		CString curr;
		if (GetCurrentFolder(curr) &&
            (pos_ == -1 || curr.CompareNoCase(name_[pos_]) != 0) )
		{
			// Truncate the list past the current pos
			ASSERT(pos_ >= -1 && pos_ < int(name_.size()));
			name_.erase(name_.begin() + (pos_ + 1), name_.end());
    		pos_ = name_.size();

			name_.push_back(curr);
            if (add_to_hist_)
                pExpl_->AddFolder();
		}
		pExpl_->UpdateFolderInfo(curr);
    }

    return retval;
}

void CHistoryShellList::Back(int count)
{
    do_move(pos_ - count);
}

void CHistoryShellList::Forw(int count)
{
    do_move(pos_ + count);
}

void CHistoryShellList::AdjustMenu(HMENU hm)
{
    int ii = 0;                 // Current menu item number

    MENUITEMINFO mii;
    memset(&mii, '\0', sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
    mii.wID = ID_EXPLORER_OPEN;
    mii.fState = MFS_ENABLED;
    mii.fType = MFT_STRING;
    mii.dwTypeData = _T("Open in HexEdit");
    ::InsertMenuItem(hm, ii++, TRUE, &mii);

    mii.wID = ID_EXPLORER_OPEN_RO;
    mii.dwTypeData = _T("Open Read-Only");
    ::InsertMenuItem(hm, ii++, TRUE, &mii);

    mii.fType = MFT_SEPARATOR;
    ::InsertMenuItem(hm, ii++, TRUE, &mii);
}

void CHistoryShellList::MenuCommand(HMENU hm, UINT id, LPCTSTR file_name)
{
    ASSERT(id == ID_EXPLORER_OPEN || id == ID_EXPLORER_OPEN_RO);
    CString full_name;
    struct _stat stat;

    if (!GetCurrentFolder(full_name))
        return;

    full_name += CString("\\") + file_name;
    if (::_stat(full_name, &stat) == -1)
        return;

    // Open the file if it is not a directory
    if ((stat.st_mode & _S_IFDIR) == 0)
    {
	    ASSERT(theApp.open_current_readonly_ == -1);
        theApp.open_current_readonly_ = (id == ID_EXPLORER_OPEN_RO);
        theApp.OpenDocumentFile(full_name);
    }
    // else open all files in directory??
}

// moves to a folder in the undo/redo stack
void CHistoryShellList::do_move(int ii)
{
	// Detect problems but handle them in release too (defensive programming)
	ASSERT(ii >= 0 && ii < int(name_.size()));
	if (ii < 0)
		ii = 0;
	else if (ii >= int(name_.size()))
		ii = name_.size() - 1;

	// Now move to the folder
	in_move_ = true;
    CMFCShellListCtrl::DisplayFolder(name_[ii]);
	in_move_ = false;

	ASSERT(pExpl_ != NULL);
	pExpl_->UpdateFolderInfo(name_[ii]);

	pos_ = ii;
}

// Column names that appear in the column header
static const TCHAR * colnames[] = 
{
	_T("Name"),             // COLNAME
	_T("Size"),             // COLSIZE
	_T("Type"),             // COLTYPE
	_T("Modified"),         // COLMOD
	_T("Attributes"),       // COLATTR
	_T("Last Opened"),      // COLOPENED
    _T("Category"),         // COLCATEGORY
    _T("Keywords"),         // COLKEYWORDS
    _T("Comments"),         // COLCOMMENTS
    // always add to the end of this list as existing registry strings may rely on this order
    NULL
};

char * CHistoryShellList::defaultWidths = "150|60|150|150|60|150|60|100|200";  // name,size,type,mod-time,attr,last-opened-timecat,kw,comments

void CHistoryShellList::OnSetColumns()
{
    int col;            // loops through columns
    CString ss;         // temp string

	ModifyStyle(LVS_TYPEMASK, LVS_REPORT);      // Need to set details mode before setting up columns
    SetExtendedStyle(LVS_EX_HEADERDRAGDROP);    // Allow column reordering

    // As we still rely on the BCG base class to handle a few things we just need
    // to check that BCG has not pulled the rug by reordering the columns.
    ASSERT(AFX_ShellList_ColumnName     == COLNAME &&
           AFX_ShellList_ColumnSize     == COLSIZE &&
           AFX_ShellList_ColumnType     == COLTYPE &&
           AFX_ShellList_ColumnModified == COLMOD);

    ASSERT(colnames[COLLAST] == NULL);      // check numbering consistency

    CString widths = theApp.GetProfileString("File-Settings", "ExplorerColWidths", defaultWidths);

    // First work out the order and widths of the columns
	for (col = 0; col < COLLAST; ++col)
    {
        // Work out format, width etc of this column
    	int fmt = col == COLSIZE ? LVCFMT_RIGHT : LVCFMT_LEFT;
        AfxExtractSubString(ss, widths, col, '|');
        int width = atoi(ss);

		InsertColumn(col, colnames[col], fmt, width, col);
    }

    // This (in combination with LVS_EX_HEADERDRAGDROP style) allows the
    // columns to be reordered by dragging, and the order saved and restored.
    CString orders = theApp.GetProfileString("File-Settings", "ExplorerColOrder");
    int order[COLLAST];
	for (col = 0; col < COLLAST; ++col)
    {
        AfxExtractSubString(ss, orders, col, '|');
        if (ss.IsEmpty())
            order[col] = col;           // this means that an empty string will just give display order = col order
        else
            order[col] = atoi(ss);
    }
    SetColumnOrderArray(COLLAST, order);
}

void CHistoryShellList::SaveLayout()
{
	// Save current folder view mode
	DWORD lvs = (GetStyle () & LVS_TYPEMASK);
	theApp.WriteProfileInt("File-Settings", "ExplorerView", lvs);
    if (lvs == LVS_REPORT)
    {
        CString widths, orders;         // Strings written to registry with column order and widths
        CString ss;
        int order[COLLAST];
        GetColumnOrderArray(order, COLLAST);

        // Save widths and order of columns
        for (int col = 0; col < COLLAST; ++col)
        {
            ss.Format("%d|", GetColumnWidth(col));
            widths += ss;
            ss.Format("%d|", order[col]);
            orders += ss;
        }
	    theApp.WriteProfileString("File-Settings", "ExplorerColWidths", widths);
	    theApp.WriteProfileString("File-Settings", "ExplorerColOrder", orders);
    }
}

CString CHistoryShellList::OnGetItemText(int iItem, int iColumn, 
									     LPAFX_SHELLITEMINFO pItem)
{
    // We use fl_idx_ to store the index into recent file list (pfl) of the current file,
    // which saves time by avoiding calls to SHGetPathFromIDList for each column.
    // This is obtained when getting the first of our columns (COLATTR) but this may need
    // fixing when the user can reorder columns (not sure COLATTR will still be done first).
    // (A better soln might be to save pItem to compare in the next call.)
	if (iColumn == COLNAME)
		fl_idx_ = -2;  // Reset recent file list index at the start of a new row to help detect bugs/problems

	TCHAR path[MAX_PATH];

	// Handle size column ourselves since BCG base class can't handle file sizes > 2Gbytes
	if (iColumn == COLSIZE)
	{
        CFileStatus fs;
		if (SHGetPathFromIDList(pItem->pidlFQ, path) &&
			CFile64::GetStatus(path, fs))
		{
			if ((fs.m_attribute & (CFile::directory | CFile ::volume)) != 0)
				return _T("");
			CString ss = NumScale(double(fs.m_size));
			if (ss.Right(1) == _T(" "))
				return ss;
			else
				return ss + _T("B");
		}
	}

	// Call BCG base class for columns it can handle
    if (iColumn < COLATTR)
        return CMFCShellListCtrl::OnGetItemText(iItem, iColumn, pItem);

    CString retval;
    CHexFileList *pfl = theApp.GetFileList();

	// These are used to get file attributes
	SHFILEINFO sfi;
	CFileStatus fs;

	switch (iColumn)
	{
	case COLATTR:  // Attributes
		sfi.dwAttributes = SFGAO_READONLY | SFGAO_HIDDEN | SFGAO_COMPRESSED | SFGAO_ENCRYPTED;
		// Get file name then get attributes
		if (SHGetPathFromIDList(pItem->pidlFQ, path))
		{
			// We get the index for this file from the recent file list here to save time repeating the
			// procedure (get path and then look up index) for the other columns.
			// IMPORTANT: This assumes that CMFCShellListCtrl::OnGetItemText calls process the list box items
			// a row at a time, from left to right - if this assumption changes this will stuff up.
			ASSERT(fl_idx_ == -2);
			if (pfl != NULL)
			    fl_idx_ = pfl->GetIndex(path);

			if (CFile::GetStatus(path, fs) &&
		        SHGetFileInfo((LPCTSTR)pItem->pidlFQ, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED))
			{
	//			if ((fs.m_attribute & CFile::readOnly) != 0)
				if ((sfi.dwAttributes & SFGAO_READONLY) != 0)
					retval += _T("R");
	//			if ((fs.m_attribute & CFile::hidden) != 0)
				if ((sfi.dwAttributes & SFGAO_HIDDEN) != 0)
					retval += _T("H");

				if ((fs.m_attribute & CFile::system) != 0)
					retval += _T("S");
				if ((fs.m_attribute & CFile::archive) != 0)
					retval += _T("A");

				if ((sfi.dwAttributes & SFGAO_COMPRESSED) != 0)
					retval += _T("C");
				if ((sfi.dwAttributes & SFGAO_ENCRYPTED) != 0)
					retval += _T("E");
				return retval;
			}
		}
		break;

	case COLOPENED:
		{
			if (pfl != NULL && fl_idx_ > -1)
				OnFormatFileDate (CTime(pfl->GetOpened(fl_idx_)), retval);
			return retval;
		}
		break;

    case COLCATEGORY:
		if (pfl != NULL && fl_idx_ > -1)
            return pfl->GetData(fl_idx_, CHexFileList::CATEGORY);
        break;

    case COLKEYWORDS:
		if (pfl != NULL && fl_idx_ > -1)
            return pfl->GetData(fl_idx_, CHexFileList::KEYWORDS);
        break;

    case COLCOMMENTS:
		if (pfl != NULL && fl_idx_ > -1)
            return pfl->GetData(fl_idx_, CHexFileList::COMMENTS);
        break;

	default:
		ASSERT(0);
		break;
	}

	return _T("");
}

int CHistoryShellList::OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn)
{
	LPAFX_SHELLITEMINFO pItem1 = (LPAFX_SHELLITEMINFO)lParam1;
	LPAFX_SHELLITEMINFO	pItem2 = (LPAFX_SHELLITEMINFO)lParam2;
	ASSERT(pItem1 != NULL);
	ASSERT(pItem2 != NULL);

    CHexFileList * pfl;
	TCHAR path1[MAX_PATH];
	TCHAR path2[MAX_PATH];

	// Handle size column ourselves since BCG base class can't handle file sizes > 2Gbytes
	switch (iColumn)
	{
	case COLSIZE:
		{
			SHFILEINFO sfi1, sfi2;
			sfi1.dwAttributes = sfi2.dwAttributes = SFGAO_FOLDER|SFGAO_STREAM;

			CFileStatus fs1, fs2;

			if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
				CFile64::GetStatus(path1, fs1) &&
				SHGetFileInfo((LPCTSTR)pItem1->pidlFQ, 0, &sfi1, sizeof(sfi1), SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED) &&
				SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
				CFile64::GetStatus(path2, fs2) &&
				SHGetFileInfo((LPCTSTR)pItem2->pidlFQ, 0, &sfi2, sizeof(sfi2), SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED))
			{
				// Make sure folders are sorted away from files
				// Note: under Xp zip files have both SFGAO_FOLDER and SFGAO_STREAM flags which is why the check is done like this.
				// Note2: using SHGFI_ATTR_SPECIFIED still seems to return all attrs but is a lot faster!?!?
				if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER ||
					(sfi2.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
				{
					if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER &&
						(sfi2.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
						return CString(path1).CompareNoCase(path2);   // Both are folders so sort on name
					else if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
						return -1;
					else
						return 1;

				}
				return fs1.m_size < fs2.m_size ? -1 : fs1.m_size > fs2.m_size ? 1 : 0;
			}
		}
		break;

	case COLATTR:
		{
			SHFILEINFO sfi1, sfi2;
			sfi1.dwAttributes = sfi2.dwAttributes = SFGAO_FOLDER|SFGAO_STREAM|SFGAO_READONLY|SFGAO_HIDDEN|SFGAO_COMPRESSED|SFGAO_ENCRYPTED;

			CFileStatus fs1, fs2;

			if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
				CFile64::GetStatus(path1, fs1) &&
				SHGetFileInfo((LPCTSTR)pItem1->pidlFQ, 0, &sfi1, sizeof(sfi1), SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED) &&
				SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
				CFile64::GetStatus(path2, fs2) &&
				SHGetFileInfo((LPCTSTR)pItem2->pidlFQ, 0, &sfi2, sizeof(sfi2), SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED))
			{
				// If one or both are folders keep them separate
				if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER ||
					(sfi2.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
				{
					if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER &&
						(sfi2.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
						return CString(path1).CompareNoCase(path2);
					else if ((sfi1.dwAttributes & (SFGAO_FOLDER|SFGAO_STREAM)) == SFGAO_FOLDER)
						return -1;
					else
						return 1;

				}

				CString ss1, ss2;

				if ((sfi1.dwAttributes & SFGAO_READONLY) != 0)
					ss1 += _T("R");
				if ((sfi1.dwAttributes & SFGAO_HIDDEN) != 0)
					ss1 += _T("H");
				if ((fs1.m_attribute & CFile::system) != 0)
					ss1 += _T("S");
				if ((fs1.m_attribute & CFile::archive) != 0)
					ss1 += _T("A");
				if ((sfi1.dwAttributes & SFGAO_COMPRESSED) != 0)
					ss1 += _T("C");
				if ((sfi1.dwAttributes & SFGAO_ENCRYPTED) != 0)
					ss1 += _T("E");

				if ((sfi2.dwAttributes & SFGAO_READONLY) != 0)
					ss2 += _T("R");
				if ((sfi2.dwAttributes & SFGAO_HIDDEN) != 0)
					ss2 += _T("H");
				if ((fs2.m_attribute & CFile::system) != 0)
					ss2 += _T("S");
				if ((fs2.m_attribute & CFile::archive) != 0)
					ss2 += _T("A");
				if ((sfi2.dwAttributes & SFGAO_COMPRESSED) != 0)
					ss2 += _T("C");
				if ((sfi2.dwAttributes & SFGAO_ENCRYPTED) != 0)
					ss2 += _T("E");

				return ss1 < ss2 ? -1 : ss1 > ss2 ? 1 : 0;
			}
		}
		break;
		
	case COLOPENED:
		{
			if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
				SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
				(pfl = theApp.GetFileList()) != NULL)
			{
				int idx1 = pfl->GetIndex(path1);
				int idx2 = pfl->GetIndex(path2);
				if (idx1 == -1 || idx2 == -1)
				{
					// One or both have not been opened in HexEdit
					if (idx1 == -1 && idx2 == -1)
						return CString(path1).CompareNoCase(path2);   // Both never opened so sort on name
					else if (idx1 == -1)
						return -1;
					else
						return 1;
				}

				time_t t1 = pfl->GetOpened(idx1);
				time_t t2 = pfl->GetOpened(idx2);
				return t1 < t2 ? -1 : t1 > t2 ? 1 : 0;
			}
		}
		break;

    case COLCATEGORY:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			(pfl = theApp.GetFileList()) != NULL)
		{
			int idx1 = pfl->GetIndex(path1);
			int idx2 = pfl->GetIndex(path2);

            CString ss1, ss2;
            if (idx1 != -1) ss1 = pfl->GetData(idx1, CHexFileList::CATEGORY);
            if (idx2 != -1) ss2 = pfl->GetData(idx2, CHexFileList::CATEGORY);
            int retval = ss1.CompareNoCase(ss2);
            if (retval == 0)
                return CString(path1).CompareNoCase(path2);  // sort by file name within category
            else
                return retval;
        }
        break;

    case COLKEYWORDS:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			(pfl = theApp.GetFileList()) != NULL)
		{
			int idx1 = pfl->GetIndex(path1);
			int idx2 = pfl->GetIndex(path2);

            CString ss1, ss2;
            if (idx1 != -1) ss1 = pfl->GetData(idx1, CHexFileList::KEYWORDS);
            if (idx2 != -1) ss2 = pfl->GetData(idx2, CHexFileList::KEYWORDS);
            return ss1.CompareNoCase(ss2);
        }
        break;

    case COLCOMMENTS:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			(pfl = theApp.GetFileList()) != NULL)
		{
			int idx1 = pfl->GetIndex(path1);
			int idx2 = pfl->GetIndex(path2);

            CString ss1, ss2;
            if (idx1 != -1) ss1 = pfl->GetData(idx1, CHexFileList::COMMENTS);
            if (idx2 != -1) ss2 = pfl->GetData(idx2, CHexFileList::COMMENTS);
            return ss1.CompareNoCase(ss2);
        }
        break;
	}

	return CMFCShellListCtrl::OnCompareItems(lParam1, lParam2, iColumn);
}

BEGIN_MESSAGE_MAP(CHistoryShellList, CMFCShellListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CHistoryShellList::OnContextMenu(CWnd * pWnd, CPoint point) 
{
    CPoint pt(point);
    ScreenToClient(&pt);
    CRect rct;
    GetHeaderCtrl().GetWindowRect(&rct);
    if (pt.y < rct.Height())
    {
        // Right click on header - display menu of available columns
        CMenu mm;
        mm.CreatePopupMenu();

        for (int col = 0; col < COLLAST; ++col)
            mm.AppendMenu(MF_ENABLED|(GetColumnWidth(col)>0?MF_CHECKED:0), col+1, colnames[col]);

        int item = mm.TrackPopupMenu(
                TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                point.x, point.y, this);
        if (item != 0)
        {
            if (GetColumnWidth(item-1) == 0)
            {
                // Set to default column width
                CString ss;
                AfxExtractSubString(ss, defaultWidths, item-1, '|');
                int width = max(strtol(ss, NULL, 10), 20);
                SetColumnWidth(item-1, width);
            }
            else
                SetColumnWidth(item-1, 0);      // set to width to zero to hide it
        }
        return;
    }

    CMFCShellListCtrl::OnContextMenu(pWnd, point);
}

void CHistoryShellList::OnDblClk(NMHDR * pNMHDR, LRESULT * pResult) 
{
	*pResult = 0;

	LVITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_PARAM;

    // Get info on the focused item (ie, the one d-clicked)
	if ((lvItem.iItem = GetNextItem(-1, LVNI_FOCUSED)) == -1 ||
        !GetItem (&lvItem))
    {
        return;
    }

	LPAFX_SHELLITEMINFO	pInfo = (LPAFX_SHELLITEMINFO) lvItem.lParam;
	if (pInfo == NULL || pInfo->pParentFolder == NULL || pInfo->pidlRel == NULL)
	{
		ASSERT (FALSE);
		return;
	}

    // Don't do anything with folders (ie, call base class)
    // Note pInfo->pParentFolder is of type IShellFolder *
	ULONG ulAttrs = SFGAO_FOLDER;
	pInfo->pParentFolder->GetAttributesOf (1, 
		(const struct _ITEMIDLIST **) &pInfo->pidlRel, &ulAttrs);

	if (ulAttrs & SFGAO_FOLDER)
    {
		CMFCShellListCtrl::OnDblClk(pNMHDR, pResult);
        return;
    }

    // Get the full path name of the file
    CString full_name;
    STRRET str;
	if (!GetCurrentFolder(full_name) ||
        pInfo->pParentFolder->GetDisplayNameOf(pInfo->pidlRel, SHGDN_INFOLDER, &str) == S_FALSE)
    {
        CMFCShellListCtrl::OnDblClk(pNMHDR, pResult);
        return;
    }
    full_name += CString("\\") + CString((LPCWSTR)str.pOleStr);

    // Check if it is a shortcut (.LNK) file
    // (See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/programmersguide/shell_int/shell_int_programming/shortcuts/shortcut.asp)
    IShellLink * psl = NULL;
    IPersistFile * ppf = NULL;
    WCHAR wide_name[MAX_PATH];  // Shortcut file name as a wide string
    char dest_name[MAX_PATH];   // Shortcut destination - ie the file/folder we want
    WIN32_FIND_DATA wfd;        // Info about shortcut destination file/folder

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl)) &&
        SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) &&
        SUCCEEDED(MultiByteToWideChar(CP_ACP, 0, full_name, -1, wide_name, MAX_PATH)) &&
        wcscat(wide_name, L".LNK") != NULL &&
        SUCCEEDED(ppf->Load(wide_name, STGM_READ)) &&
        SUCCEEDED(psl->GetPath(dest_name, MAX_PATH, &wfd, 0)) )
    {
        // We now have the path and info about the shortcut dest.
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            // If its a shortcut to a directory then change to that directory
            DisplayFolder(dest_name);
            ppf->Release();
            psl->Release();
            return;
        }

        // Make the name of the file to be opened = destination of shortcut
        full_name = dest_name;
    }
    if (ppf != NULL) ppf->Release();
    if (psl != NULL) psl->Release();

	ASSERT(theApp.open_current_readonly_ == -1);
    theApp.OpenDocumentFile(full_name);
}

/////////////////////////////////////////////////////////////////////////////
// CFilterEdit - edit control which uses entered filter when CR hit
BEGIN_MESSAGE_MAP(CFilterEdit, CEdit)
    ON_WM_CHAR()
    ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

void CFilterEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    CEdit::OnChar(nChar, nRepCnt, nFlags);
    if (nChar == '\r' || nChar == '\n')
    {
		ASSERT(pExpl_ != NULL);
		pExpl_->NewFilter();
		SetSel(0, -1);
    }
    else if (nChar == '\033')
    {
		ASSERT(pExpl_ != NULL);
		pExpl_->OldFilter();
	}
}

UINT CFilterEdit::OnGetDlgCode() 
{
    // Get all keys so that we see CR
    return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CFolderEdit - edit control which uses entered folder name when CR hit
BEGIN_MESSAGE_MAP(CFolderEdit, CEdit)
    ON_WM_CHAR()
    ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

void CFolderEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    CEdit::OnChar(nChar, nRepCnt, nFlags);
    if (nChar == '\r' || nChar == '\n')
    {
		ASSERT(pExpl_ != NULL);
		pExpl_->NewFolder();
		SetSel(0, -1);
    }
    else if (nChar == '\033')
    {
		ASSERT(pExpl_ != NULL);
		pExpl_->OldFolder();
	}
}

UINT CFolderEdit::OnGetDlgCode() 
{
    // Get all keys so that we see CR
    return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CExplorerWnd - modeless dialog that looks like Windows Explorer
BOOL CExplorerWnd::Create(CWnd* pParentWnd) 
{
	if (!CHexPaneDialog::Create(pParentWnd, IDD, CBRS_LEFT | CBRS_SIZE_DYNAMIC, IDD))
        return FALSE;

    // Create contained splitter window and shell windows in the 2 panes
    // First get the rectangle for the splitter window from the place holder group box
    CRect rct;
    CWnd *pw = GetDlgItem(IDC_EXPLORER);
    ASSERT(pw != NULL);
    pw->GetWindowRect(&rct);
    VERIFY(pw->DestroyWindow());  // destroy place holder window since we don't want 2 with the same ID
    // Create the splitter
    ScreenToClient(&rct);
    rct.InflateRect(0, 0, 3, 30);
	splitter_.Create(this, rct, IDC_EXPLORER);
    // Create the 2 pane windows and add them to the splitter
	tree_.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
			              CRect(0, 0, 0, 0), &splitter_, IDC_EXPLORER_TREE);
	splitter_.SetPane(0, &tree_);
	list_.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT,
			              CRect(0, 0, 0, 0), &splitter_, IDC_EXPLORER_LIST);
	splitter_.SetPane(1, &list_);

    // Set options for the tree and list
	tree_.SetFlags(SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN);
	tree_.SetRelatedList(&list_);
	tree_.EnableShellContextMenu();
    list_.EnableShellContextMenu();

 //   // Create the resizer that move the controls when dialog is resized
	//resizer_.Create(GetSafeHwnd(), TRUE, 100, TRUE);
	//resizer_.SetInitialSize(m_sizeDefault);
	//resizer_.SetMinimumTrackingSize(m_sizeDefault);
	//resizer_.SetGripEnabled(FALSE);

 //   resizer_.Add(IDC_FOLDER_FILTER,    0, 0, 100,   0);  // move right edge
 //   resizer_.Add(IDC_FOLDER_REFRESH, 100, 0,   0,   0);  // stick to right side
 //   resizer_.Add(IDC_FILTER_OPTS,    100, 0,   0,   0);
 //   resizer_.Add(IDC_FOLDER_VIEW,    100, 0,   0,   0);
 //   resizer_.Add(IDC_FOLDER_NAME,      0, 0, 100,   0);
 //   resizer_.Add(IDC_EXPLORER,         0, 0, 100, 100);  // move right & bottom edges

    //SetMinSize(m_sizeDefault);  // xxx fix for MFC9

    return TRUE;
}

void CExplorerWnd::DoDataExchange(CDataExchange* pDX)
{
	CHexPaneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FOLDER_BACK,    ctl_back_);
	DDX_Control(pDX, IDC_FOLDER_FORW,    ctl_forw_);
	DDX_Control(pDX, IDC_FOLDER_PARENT,  ctl_up_);
    DDX_Control(pDX, IDC_FILTER_OPTS,    ctl_filter_opts_);
	DDX_Control(pDX, IDC_FOLDER_REFRESH, ctl_refresh_);
	DDX_Control(pDX, IDC_FOLDER_VIEW,    ctl_view_);
	DDX_Control(pDX, IDC_FOLDER_FLIP,    ctl_flip_);

	DDX_Control(pDX, IDC_FOLDER_FILTER,  ctl_filter_);
	DDX_Control(pDX, IDC_FOLDER_NAME,    ctl_name_);

	DDX_CBString(pDX, IDC_FOLDER_FILTER, curr_filter_);
	DDX_CBString(pDX, IDC_FOLDER_NAME,   curr_name_);
}

BOOL CExplorerWnd::OnInitDialog()
{
    int pane_size[2];
    splitter_.SetOrientation(theApp.GetProfileInt("File-Settings", "ExplorerOrientation", SSP_HORZ));
    pane_size[0] = theApp.GetProfileInt("File-Settings", "ExplorerTreeSize", 0);
    if (pane_size[0] > 0)
    {
        pane_size[1] = theApp.GetProfileInt("File-Settings", "ExplorerListSize", 1);
        splitter_.SetPaneSizes(pane_size);
    }

    CWnd *pwnd = ctl_filter_.GetWindow(GW_CHILD);
	ASSERT(pwnd != NULL);
    VERIFY(ctl_filter_edit_.SubclassWindow(pwnd->m_hWnd));
	ctl_filter_edit_.Start(this);

    pwnd = ctl_name_.GetWindow(GW_CHILD);
	ASSERT(pwnd != NULL);
    VERIFY(ctl_name_edit_  .SubclassWindow(pwnd->m_hWnd));
	ctl_name_edit_.Start(this);

	// Restore last used view type
	DWORD lvs = theApp.GetProfileInt("File-Settings", "ExplorerView", LVS_REPORT);
	list_.ModifyStyle(LVS_TYPEMASK, lvs);

    if (theApp.GetProfileInt("File-Settings", "ExplorerShowHidden", 1) == 1)
        list_.SetItemTypes(SHCONTF(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN));
    else
        list_.SetItemTypes(SHCONTF(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS));

    int ii;

	// Restore filter history to the drop down list
    std::vector<CString> sv;
    ::LoadHist(sv,  "ExplorerFilters", theApp.max_expl_filt_hist_);
	for (ii = 0; ii < sv.size(); ++ii)
		if (!sv[ii].IsEmpty())
			ctl_filter_.AddString(sv[ii]);
	//for (ii = 0; AfxExtractSubString(ss, hist, ii, '|'); ++ii)
	//	if (!ss.IsEmpty())
	//		ctl_filter_.AddString(ss);

	// Restore last used filter
    CString filter = theApp.GetProfileString("File-Settings", "ExplorerFilter", "*.*");
	ctl_filter_.SetWindowText(filter);
	list_.SetFilter(filter);

	// Restore folder history to the drop down list
	//for (ii = 0; AfxExtractSubString(ss, hist, ii, '|'); ++ii)
	//	if (!ss.IsEmpty())
	//		ctl_name_.AddString(ss);
    ::LoadHist(sv,  "ExplorerFolders", theApp.max_expl_dir_hist_);
	for (ii = 0; ii < sv.size(); ++ii)
		if (!sv[ii].IsEmpty())
			ctl_name_.AddString(sv[ii]);

	// Restore last used folder
    if (IsVisible())
    {
        CString dir = theApp.GetProfileString("File-Settings", "ExplorerDir", "C:\\");
        ctl_name_.SetWindowText(dir);
        tree_.SelectPath(dir);
	    list_.Start(this);
	    list_.DisplayFolder((const char *)dir);
    }
    else
	    list_.Start(this);                      // Signal that everything is set up

    ctl_back_.SetImage(IDB_BACK, IDB_BACK_HOT);
	ctl_back_.m_bTransparent = TRUE;
    ctl_back_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    ctl_back_.SetTooltip(_T("Back To Previous Folder"));
	ctl_back_.Invalidate();

    ctl_forw_.SetImage(IDB_FORW, IDB_FORW_HOT);
	ctl_forw_.m_bTransparent = TRUE;
    ctl_forw_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    ctl_forw_.SetTooltip(_T("Forward"));
	ctl_forw_.Invalidate();

    ctl_up_.SetImage(IDB_PARENT, IDB_PARENT_HOT);
	ctl_up_.m_bTransparent = TRUE;
    ctl_up_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    ctl_up_.SetTooltip(_T("Move Up One Level"));
    ctl_up_.Invalidate();

    VERIFY(arrow_icon_ = AfxGetApp()->LoadIcon(IDI_ARROW));
    ctl_filter_opts_.SetIcon(arrow_icon_);
    ctl_filter_opts_.m_bRightArrow = TRUE;

    // Build menu
    build_filter_menu();

    ctl_refresh_.SetImage(IDB_REFRESH, IDB_REFRESH_HOT);
	ctl_refresh_.m_bTransparent = TRUE;
    ctl_refresh_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    ctl_refresh_.SetTooltip(_T("Refresh"));
	ctl_refresh_.Invalidate();

    if (splitter_.Orientation() == SSP_VERT)
        ctl_flip_.SetImage(IDB_VERT, IDB_VERT_HOT);
    else
        ctl_flip_.SetImage(IDB_HORZ, IDB_HORZ_HOT);

	ctl_flip_.m_bTransparent = TRUE;
    ctl_flip_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    ctl_flip_.SetTooltip(_T("Flip vertical/horizontal"));
	ctl_flip_.Invalidate();

	m_menu_.LoadMenu(IDR_EXPL);
    // Views menu
	ctl_view_.m_hMenu = m_menu_.GetSubMenu(0)->GetSafeHmenu();
	ctl_view_.m_bOSMenu = FALSE;
    ctl_view_.SetImage(IDB_VIEW, IDB_VIEW_HOT);
	ctl_view_.m_bTransparent = TRUE;
    ctl_view_.m_nFlatStyle = CMFCButton::BUTTONSTYLE_FLAT;
    //ctl_view_.SizeToContent();
    ctl_view_.SetTooltip(_T("Change Folder View"));
	ctl_view_.Invalidate();

	init_ = true;

    return TRUE;
}

// Rebuild the filter menu when the filter list has changed
void CExplorerWnd::build_filter_menu()
{
    CString filters = theApp.GetCurrentFilters();
    if (filters == filters_)
        return;

    CMenu mm;
    mm.CreatePopupMenu();
    CString filter_name;
    for (int filter_num = 0;
         AfxExtractSubString(filter_name, filters, filter_num*2, '|') && !filter_name.IsEmpty();
         ++filter_num)
    {
        // Store filter and use index as menu item ID (but add 1 since 0 means no ID used).
        mm.AppendMenu(MF_STRING, filter_num+1, filter_name);
    }
    mm.AppendMenu(MF_SEPARATOR);
    mm.AppendMenu(MF_STRING, 0x7FFF, _T("&Edit Filter List..."));
    ctl_filter_opts_.m_hMenu = mm.GetSafeHmenu();
    mm.Detach();

    filters_ = filters;
}

// Called after the currently displayed folder has changed
void CExplorerWnd::UpdateFolderInfo(CString folder) 
{
	ctl_name_.SetWindowText(folder);

	if (hh_ != 0)
	{
		::FindCloseChangeNotification(hh_);
		hh_ = 0;
	}
	hh_ = ::FindFirstChangeNotification(folder, FALSE,
		                                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
										FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
										FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION);
}

void CExplorerWnd::Refresh() 
{
    list_.Refresh();
}

void CExplorerWnd::NewFilter() 
{
	VERIFY(UpdateData());

	if (curr_filter_.IsEmpty())
		return;

	// Change current filter
	if (curr_filter_ != list_.GetFilter())
		list_.SetFilter(curr_filter_);

	// Add string entered into drop down list if not there
    AddFilter();

	Refresh();
}

// Add current filter to the drop down list or move to top if already there
void CExplorerWnd::AddFilter() 
{
	int ii, count = ctl_filter_.GetCount();
    CString tmp = curr_filter_;
    tmp.MakeUpper();  // Get uppercase version for case-insensitive compare

    // See if it is already there
    for (ii = 0; ii < count; ++ii)
	{
		CString ss;
		ctl_filter_.GetLBText(ii, ss);
        ss.MakeUpper();
		if (tmp == ss)
        {
            // Delete it from the list so we can add it at the top (below)
            ctl_filter_.DeleteString(ii);
			break;
        }
	}
	ctl_filter_.InsertString(0, curr_filter_);
}

void CExplorerWnd::OldFilter() 
{
	VERIFY(UpdateData(FALSE));
}

// User typed in a folder name
void CExplorerWnd::NewFolder()
{
	VERIFY(UpdateData());

	if (curr_name_.IsEmpty())
		return;

	// Change current folder
    CString dir;
    if (!list_.GetCurrentFolder(dir) || curr_name_ != dir)
        list_.DisplayFolder(curr_name_);

	Refresh();
}

// Add current folder to the drop down list or move to top if already there
void CExplorerWnd::AddFolder()
{
	int ii, count = ctl_name_.GetCount();
    CString tmp = list_.Folder();
    tmp.MakeUpper();  // Get uppercase version for case-insensitive compare

    // See if it is already there
	for (ii = 0; ii < count; ++ii)
	{
		CString ss;
		ctl_name_.GetLBText(ii, ss);
        ss.MakeUpper();
		if (tmp == ss)
        {
            // Delete it from the list so we can add it to the top (below)
            ctl_name_.DeleteString(ii);
			break;
        }
	}
	ctl_name_.InsertString(0, list_.Folder());  // Add original string (with case)
}

void CExplorerWnd::OldFolder() 
{
	VERIFY(UpdateData(FALSE));
}

BEGIN_MESSAGE_MAP(CExplorerWnd, CHexPaneDialog)
	//ON_WM_SIZE()
	ON_WM_DESTROY()
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_FOLDER_BACK, OnFolderBack)
	ON_BN_CLICKED(IDC_FOLDER_FORW, OnFolderForw)
	ON_BN_CLICKED(IDC_FOLDER_PARENT, OnFolderParent)
	ON_BN_CLICKED(IDC_FILTER_OPTS, OnFilterOpts)
	ON_BN_CLICKED(IDC_FOLDER_REFRESH, OnFolderRefresh)
	ON_BN_CLICKED(IDC_FOLDER_VIEW, OnFolderView)
	ON_BN_CLICKED(IDC_FOLDER_FLIP, OnFolderFlip)
	ON_CBN_SELCHANGE(IDC_FOLDER_NAME, OnSelchangeFolderName)
	ON_CBN_SELCHANGE(IDC_FOLDER_FILTER, OnSelchangeFilter)
END_MESSAGE_MAP()

void CExplorerWnd::OnDestroy() 
{
    theApp.WriteProfileInt("File-Settings", "ExplorerOrientation", splitter_.Orientation());
    theApp.WriteProfileInt("File-Settings", "ExplorerTreeSize", splitter_.GetPaneSize(0));
    theApp.WriteProfileInt("File-Settings", "ExplorerListSize", splitter_.GetPaneSize(1));

    list_.SaveLayout();         // save columns etc for list

	theApp.WriteProfileInt("File-Settings", "ExplorerShowHidden",
                           (list_.GetItemTypes() & SHCONTF_INCLUDEHIDDEN) != 0);

	// Save current folder
    CString dir;
    if (list_.GetCurrentFolder(dir))
        theApp.WriteProfileString("File-Settings", "ExplorerDir", dir);

	// Save folder history
	//int count = ctl_name_.GetCount();
	//for (int ii = 0; ii < count && ii < 32; ++ii)
	//{
	//	if (!hist.IsEmpty())
	//		hist += CString("|");
	//	CString ss;
	//	ctl_name_.GetLBText(ii, ss);
	//	hist += ss;
	//}
    //theApp.WriteProfileString("File-Settings", "ExplorerHistory", hist);
    std::vector<CString> sv;
	int count = min(ctl_name_.GetCount(), theApp.max_expl_dir_hist_);
	for (int ii = 0; ii < count; ++ii)
    {
		CString ss;
		ctl_name_.GetLBText(ii, ss);
        sv.push_back(ss);
    }
    ::SaveHist(sv,  "ExplorerFolders", theApp.max_expl_dir_hist_);

	// Save current filter to restore later
	CString filter;
    ctl_filter_.GetWindowText(filter);
	theApp.WriteProfileString("File-Settings", "ExplorerFilter", filter);

	// Save filter history
	//hist.Empty();
	//count = ctl_filter_.GetCount();
	//for (int jj = 0; jj < count && jj < 16; ++jj)
	//{
	//	if (jj > 0)
	//		hist += CString("|");
	//	CString ss;
	//	ctl_filter_.GetLBText(jj, ss);
	//	hist += ss;
	//}
    //theApp.WriteProfileString("File-Settings", "ExplorerFilterHistory", hist);

    sv.clear();
	count = min(ctl_filter_.GetCount(), theApp.max_expl_filt_hist_);
	for (int ii = 0; ii < count; ++ii)
    {
		CString ss;
		ctl_filter_.GetLBText(ii, ss);
        sv.push_back(ss);
    }
    ::SaveHist(sv,  "ExplorerFilters", theApp.max_expl_filt_hist_);

	if (hh_ != 0)
	{
		::FindCloseChangeNotification(hh_);
		hh_ = 0;
	}

    CHexPaneDialog::OnDestroy();
}

// When something happens to change a file we call this function so that if the file is
// in the active folder we can update the folder.  We only keep track of the fact that an
// update is needed and do the update later in OnKickIdle - this means that lots of
// changes to the folder will not cause lots of calls to Refresh().
void CExplorerWnd::Update(LPCTSTR file_name /* = NULL */)
{
    if (update_required_)
        return;                 // Update already flagged

	if (file_name == NULL)
		update_required_ = true;
	else
	{
		// Get the name of the folder containing the file
		CString ss(file_name);
		::PathRemoveFileSpec(ss.GetBuffer(1));
		ss.ReleaseBuffer();

		// If the file's folder is the folder we are displaying then remember that we need to refresh it
		if (AfxComparePath(ss, list_.Folder()))
			update_required_ = true;
	}
}

static DWORD id_pairs_explorer[] = {
	IDC_FOLDER_BACK, HIDC_FOLDER_BACK,
	IDC_FOLDER_FORW, HIDC_FOLDER_FORW,
	IDC_FOLDER_PARENT, HIDC_FOLDER_PARENT,
	IDC_FOLDER_FILTER, HIDC_FOLDER_FILTER,
	IDC_FILTER_OPTS, HIDC_FILTER_OPTS,
	IDC_FOLDER_VIEW, HIDC_FOLDER_VIEW,
	IDC_FOLDER_FLIP, HIDC_FOLDER_FLIP,
	IDC_FOLDER_NAME, HIDC_FOLDER_NAME,
	IDC_FOLDER_REFRESH, HIDC_FOLDER_REFRESH,
	IDC_EXPLORER, HIDC_EXPLORER,
	IDC_EXPLORER_TREE, HIDC_EXPLORER,
	IDC_EXPLORER_LIST, HIDC_EXPLORER,
    0,0 
}; 

BOOL CExplorerWnd::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// Note calling theApp.HtmlHelpWmHelp here seems to make the window go behind 
	// and then disappear when mouse up evenet is seen.  The only soln I could
	// find after a lot of experimenetation is to do it later (in OnKickIdle).
	help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
    return TRUE;
}

LRESULT CExplorerWnd::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (!init_)
		return FALSE;

	ctl_back_.EnableWindow(list_.BackAllowed());
	ctl_forw_.EnableWindow(list_.ForwAllowed());
	ctl_up_.EnableWindow(!list_.IsDesktop());

	// Update view check of menu items to reflect current view type
	DWORD lvs = (list_.GetStyle () & LVS_TYPEMASK);
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_VIEW_LARGEICON, (lvs == LVS_ICON      ? MF_CHECKED : MF_UNCHECKED));
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_VIEW_SMALLICON, (lvs == LVS_SMALLICON ? MF_CHECKED : MF_UNCHECKED));
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_VIEW_LIST,      (lvs == LVS_LIST      ? MF_CHECKED : MF_UNCHECKED));
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_VIEW_DETAILS,   (lvs == LVS_REPORT    ? MF_CHECKED : MF_UNCHECKED));

    bool show = (list_.GetItemTypes() & SHCONTF_INCLUDEHIDDEN) != 0;
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_HIDDEN_HIDE, !show ? MF_CHECKED : MF_UNCHECKED);
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_HIDDEN_SHOW, show ? MF_CHECKED : MF_UNCHECKED);

    build_filter_menu();

	if (hh_ != 0 && ::WaitForMultipleObjects(1, &hh_, FALSE, 0) == WAIT_OBJECT_0)
	{
		VERIFY(::FindNextChangeNotification(hh_));
		update_required_ = true;
	}
	if (update_required_)
	{
		Refresh();
		update_required_ = false;
	}

	// Display context help for ctrl set up in call to a page's OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs_explorer);
		help_hwnd_ = (HWND)0;
	}

	return FALSE;
}

void CExplorerWnd::OnFolderBack()
{
	ASSERT(list_.BackAllowed());
    if (list_.BackAllowed())
	    list_.Back();
}

void CExplorerWnd::OnFolderForw()
{
	ASSERT(list_.ForwAllowed());
    if (list_.ForwAllowed())
        list_.Forw();
}

// User clicked the parent folder (up) button
void CExplorerWnd::OnFolderParent()
{
	ASSERT(!list_.IsDesktop());
	list_.DisplayParentFolder();
}

// Called when an item is selected from the filters menu
void CExplorerWnd::OnFilterOpts()
{
    if (ctl_filter_opts_.m_nMenuResult == 0x7FFF)
    {
		// Edit filters option - fire up options dlg at filters page
        theApp.display_options(FILTER_OPTIONS_PAGE, TRUE);
    }
    else if (ctl_filter_opts_.m_nMenuResult != 0)
    {
		// Get the filter string corresp. to the menu item selected
        CString filters = theApp.GetCurrentFilters();
        CString ss;
        AfxExtractSubString(ss, filters, (ctl_filter_opts_.m_nMenuResult-1)*2 + 1, '|');

		// Use the new filter
	    ctl_filter_.SetWindowText(ss);
	    VERIFY(UpdateData());
	    list_.SetFilter(curr_filter_);

        AddFilter();

	    Refresh();
    }
}

// Called when refresh button clicked
void CExplorerWnd::OnFolderRefresh()
{
	VERIFY(UpdateData());               // get any new filter or folder name string
	list_.SetFilter(curr_filter_);
    list_.add_to_hist_ = false;
    list_.DisplayFolder(curr_name_);
    list_.add_to_hist_ = true;
	Refresh();
}

void CExplorerWnd::OnFolderFlip()
{
    splitter_.Flip();
    if (splitter_.Orientation() == SSP_VERT)
        ctl_flip_.SetImage(IDB_VERT, IDB_VERT_HOT);
    else
        ctl_flip_.SetImage(IDB_HORZ, IDB_HORZ_HOT);

    // This fixes icon positions for resized window
    UINT style = list_.GetStyle () & LVS_TYPEMASK;
	list_.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
    list_.ModifyStyle(LVS_TYPEMASK, style);
}

void CExplorerWnd::OnFolderView() 
{
	// Change folder list view depending on menu item selected
	switch (ctl_view_.m_nMenuResult)
	{
	case ID_VIEW_LARGEICON:
		list_.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
		break;

	case ID_VIEW_SMALLICON:
		list_.ModifyStyle(LVS_TYPEMASK, LVS_SMALLICON);
		break;

	case ID_VIEW_LIST:
		list_.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
		break;

	case ID_VIEW_DETAILS:
		list_.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
		break;

    // Show/hide hidden files
	case ID_HIDDEN_HIDE:
        list_.SetItemTypes(SHCONTF(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS));
		break;

    case ID_HIDDEN_SHOW:
        list_.SetItemTypes(SHCONTF(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN));
		break;

	default:
		ASSERT(0);
		return;
	}
}

// Called when a name selected from the folder drop down list (history)
void CExplorerWnd::OnSelchangeFolderName()
{
	ASSERT(ctl_name_.GetCurSel() > -1);
	CString ss;
	// Get the folder name that was selected and put it into the control
	// [For some reason when a new item is selected from the drop down list the
	//  previous string is still in the control when we get this message.]
	ctl_name_.GetLBText(ctl_name_.GetCurSel(), ss);
	ctl_name_.SetWindowText(ss);

	VERIFY(UpdateData());
    list_.add_to_hist_ = false;
    list_.DisplayFolder(curr_name_);
    list_.add_to_hist_ = true;

	Refresh();
}

// Called when a name selected from the filters drop down list (history)
void CExplorerWnd::OnSelchangeFilter()
{
	ASSERT(ctl_filter_.GetCurSel() > -1);
	CString ss;
	// Get the filter that was selected and put it into the control
	// [For some reason when a new item is selected from the drop down list the
	//  previous string is still in the control when we get this message.]
	ctl_filter_.GetLBText(ctl_filter_.GetCurSel(), ss);
	ctl_filter_.SetWindowText(ss);

	VERIFY(UpdateData());
	list_.SetFilter(curr_filter_);

	Refresh();
}

/////////////////////////////////////////////////////////////////////////////
// CImportDialog - derived from CFileDialog (via CHexFileDialog) for handling extra controls during import

void CImportDialog::OnInitDone()
{
    CRect rct;                          // Used to move/resize controls
    CWnd *pp;                           // Parent = the dialog window itself
    VERIFY(pp = GetParent());

    ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(IDOK) != NULL);

    // Create a new button below the "Type" drop down list
    pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get button rectangle
    pp->ScreenToClient(rct);
    rct.InflateRect(0, 4, -rct.Width()/3, 4);   // Make 2/3 width, + make higher for 2 lines of text
    rct.OffsetRect(0, 30);

    m_discon.Create(_T("Use import addresses\r\n(for non-adjoining records)"),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_DISCON);
    m_discon.SetFont(pp->GetDlgItem(IDOK)->GetFont());
    m_discon.SetCheck(theApp.import_discon_);

    rct.OffsetRect(rct.Width()+4, 0);
	rct.InflateRect(0, 0, -(rct.Width()/2), 0);  // Make half width of IDC_DISCON checkbox (= 1/3 of cmb1 width)
    m_highlight.Create(_T("Highlight"),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_IMPORT_HIGHLIGHT);
    m_highlight.SetFont(pp->GetDlgItem(IDOK)->GetFont());
    m_highlight.SetCheck(theApp.import_highlight_);
    CHexFileDialog::OnInitDone();
}

BOOL CImportDialog::OnFileNameOK()
{
    theApp.import_discon_    = m_discon.GetCheck() == 1    ? TRUE : FALSE;
    theApp.import_highlight_ = m_highlight.GetCheck() == 1 ? TRUE : FALSE;

    //CRect rr;
    //GetParent()->GetDlgItem(lst1)->GetWindowRect(rr);

    return CHexFileDialog::OnFileNameOK();
}

/////////////////////////////////////////////////////////////////////////////
// CExportDialog - derived from CFileDialog (via CHexFileDialog) for handling extra control during export

void CExportDialog::OnInitDone()
{
    CRect rct;                          // Used to move/resize controls
    CWnd *pp;                           // Parent = the dialog window itself
    VERIFY(pp = GetParent());

    ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(IDOK) != NULL);

    // Create a new button below the "Type" drop down list
    pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get button rectangle
    pp->ScreenToClient(rct);
    rct.InflateRect(0, 4, -(rct.Width()/2), 4);   // make higher for 2 lines of text and half the width
    rct.OffsetRect(0, 30);

    m_discon_hl.Create(_T("Export (non-adjoining)\r\nhighlighted areas only"),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_DISCON);
    m_discon_hl.SetFont(pp->GetDlgItem(IDOK)->GetFont());
    m_discon_hl.SetCheck(theApp.import_discon_);

	FILE_ADDRESS start, end;
	m_pview->GetSelAddr(start, end);
	if (start >= end)
		m_discon_hl.SetCheck(1);
	if (!m_pview->HasHighlights())
	{
		// If no highlights disable the checkbox
		m_discon_hl.SetCheck(0);
		m_discon_hl.EnableWindow(FALSE);
	}

    CHexFileDialog::OnInitDone();
}

BOOL CExportDialog::OnFileNameOK()
{
    theApp.import_discon_ = m_discon_hl.GetCheck() == 1    ? TRUE : FALSE;

    return CHexFileDialog::OnFileNameOK();
}

/////////////////////////////////////////////////////////////////////////////
// CFileOpenDialog - derived from CFileDialog (via CHexFileDialog) for handling extra control
void CFileOpenDialog::OnInitDone()
{
    CRect rct;                          // Used to move/resize controls
    CWnd *pp;                           // Parent = the dialog window itself
    VERIFY(pp = GetParent());

    ASSERT(pp->GetDlgItem(cmb1) != NULL && pp->GetDlgItem(chx1) != NULL && pp->GetDlgItem(IDOK) != NULL);

    // Create a new check box next to Read only check box
    pp->GetDlgItem(cmb1)->GetWindowRect(rct); // Get "Type" drop down list width
	int width = rct.Width();
    pp->GetDlgItem(chx1)->GetWindowRect(rct); // Get "Read only" rectangle
    pp->ScreenToClient(rct);
	// Make read only check box half the width of "Type" box (it is sometimes much wider than it needs to be)
	rct.right = rct.left + width/2 - 2;
    pp->GetDlgItem(chx1)->MoveWindow(rct);
	// Work out rect of new check box
    rct.OffsetRect(width/2, 0);              // move over (so not on top of "read only" checkbox)
    //rct.InflateRect(0, 0, 10, 0);

    m_open_shared.Create(_T("Open shareable"),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE, rct, pp, IDC_OPEN_SHARED);
    m_open_shared.SetFont(pp->GetDlgItem(IDOK)->GetFont());
    m_open_shared.SetCheck(theApp.open_file_shared_);

    CHexFileDialog::OnInitDone();
}

BOOL CFileOpenDialog::OnFileNameOK()
{
    theApp.open_file_shared_ = m_open_shared.GetCheck() == 1 ? TRUE : FALSE;

    return CHexFileDialog::OnFileNameOK();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiplay dialog

CMultiplay::CMultiplay(CWnd* pParent /*=NULL*/)
        : CDialog(CMultiplay::IDD, pParent)
{
        //{{AFX_DATA_INIT(CMultiplay)
        plays_ = 0;
        //}}AFX_DATA_INIT
}

void CMultiplay::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CMultiplay)
	DDX_Control(pDX, IDC_PLAY_NAME, name_ctrl_);
        DDX_Text(pDX, IDC_PLAYS, plays_);
        DDV_MinMaxLong(pDX, plays_, 1, 999999999);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMultiplay, CDialog)
        //{{AFX_MSG_MAP(CMultiplay)
        ON_BN_CLICKED(IDC_PLAY_OPTIONS, OnPlayOptions)
        ON_WM_HELPINFO()
	ON_CBN_SELCHANGE(IDC_PLAY_NAME, OnSelchangePlayName)
	ON_BN_CLICKED(IDC_PLAY_HELP, OnHelp)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CMultiplay::FixControls()
{
    name_ctrl_.GetLBText(name_ctrl_.GetCurSel(), macro_name_);

    if (macro_name_ == DEFAULT_MACRO_NAME)
    {
        plays_ = 1;
    }
    else
    {
        std::vector<key_macro> tmp;
        CString comment;
        int halt_lev;
        long plays;
		int version;  // Version of HexEdit in which the macro was recorded

        ASSERT(theApp.mac_dir_.Right(1) == "\\");
        if (theApp.macro_load(theApp.mac_dir_ + macro_name_ + ".hem", &tmp, comment, halt_lev, plays, version))
            plays_ = plays;
        else
        {
            ASSERT(0);
            plays_ = 1;
        }
    }

    UpdateData(FALSE);  // Put number of plays into control
}

/////////////////////////////////////////////////////////////////////////////
// CMultiplay message handlers

BOOL CMultiplay::OnInitDialog() 
{
        CDialog::OnInitDialog();

        if (!theApp.recording_ && theApp.mac_.size() > 0)
            name_ctrl_.AddString(DEFAULT_MACRO_NAME);

        // Find all .hem files in macro dir
        CFileFind ff;
        ASSERT(theApp.mac_dir_.Right(1) == "\\");
        BOOL bContinue = ff.FindFile(theApp.mac_dir_ + "*.hem");

        while (bContinue)
        {
            // At least one match - check them all
            bContinue = ff.FindNextFile();

            // Hide macro files beginning with underscore unless recording.
            // This is so that "sub-macros" are not normally seen but can be
            // selected when recording a macro that invokes them.
            if (theApp.recording_ || ff.GetFileTitle().Left(1) != "_")
                name_ctrl_.AddString(ff.GetFileTitle());
        }
        ASSERT(name_ctrl_.GetCount() > 0);
        name_ctrl_.SetCurSel(0);

        ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
        ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_PLAYS))->SetRange(1, UD_MAXVAL);

        ASSERT(GetDlgItem(IDC_PLAYS) != NULL);
        ASSERT(GetDlgItem(IDC_DESC_PLAYS) != NULL);
        ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
        GetDlgItem(IDC_PLAYS)->EnableWindow(!theApp.recording_);
        GetDlgItem(IDC_DESC_PLAYS)->EnableWindow(!theApp.recording_);
        GetDlgItem(IDC_SPIN_PLAYS)->EnableWindow(!theApp.recording_);
        FixControls();

        return TRUE;
}

void CMultiplay::OnOK() 
{
    name_ctrl_.GetLBText(name_ctrl_.GetCurSel(), macro_name_);
	
    CDialog::OnOK();
}

void CMultiplay::OnHelp() 
{
    // Display help for this page
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_MULTIPLAY_HELP))
        AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs_play[] = { 
    IDC_PLAY_NAME, HIDC_PLAY_NAME,
    IDC_PLAY_OPTIONS, HIDC_PLAY_OPTIONS,
    IDC_DESC_PLAYS, HIDC_PLAYS,
    IDC_PLAYS, HIDC_PLAYS,
    IDC_SPIN_PLAYS, HIDC_PLAYS,
    IDC_PLAY_HELP, HIDC_HELP_BUTTON,
    IDOK, HID_MULTIPLAY_OK,
    0,0 
};

BOOL CMultiplay::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs_play);
    return TRUE;
}

void CMultiplay::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs_play);
}

void CMultiplay::OnSelchangePlayName()
{
    FixControls();
}

void CMultiplay::OnPlayOptions() 
{
    // Invoke the Options dlg with the macro page displayed
    theApp.display_options(MACRO_OPTIONS_PAGE, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro dialog


CSaveMacro::CSaveMacro(CWnd* pParent /*=NULL*/)
        : CDialog(CSaveMacro::IDD, pParent)
{
        //{{AFX_DATA_INIT(CSaveMacro)
        plays_ = 0;
        name_ = _T("");
        comment_ = _T("");
        halt_level_ = -1;
        //}}AFX_DATA_INIT
        halt_level_ = 1;
}

void CSaveMacro::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CSaveMacro)
        DDX_Text(pDX, IDC_PLAYS, plays_);
	DDV_MinMaxLong(pDX, plays_, 1, 999999999);
        DDX_Text(pDX, IDC_MACRO_NAME, name_);
        DDX_Text(pDX, IDC_MACRO_COMMENT, comment_);
        DDX_Radio(pDX, IDC_HALT0, halt_level_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveMacro, CDialog)
        //{{AFX_MSG_MAP(CSaveMacro)
        ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_MACRO_HELP, OnMacroHelp)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro message handlers

BOOL CSaveMacro::OnInitDialog() 
{
    aa_ = dynamic_cast<CHexEditApp *>(AfxGetApp());
    name_ = aa_->mac_filename_;
    plays_ = aa_->plays_;
    comment_ = aa_->mac_comment_;
//    halt_level_ = aa_->halt_level_;

    CDialog::OnInitDialog();

    ASSERT(GetDlgItem(IDC_SPIN_PLAYS) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_PLAYS))->SetRange(1, UD_MAXVAL);

    return TRUE;
}

void CSaveMacro::OnOK() 
{
    if (!UpdateData(TRUE))
        return;                         // DDV failed

    if (name_.IsEmpty())
    {
        AfxMessageBox("Please enter a macro name");
        ASSERT(GetDlgItem(IDC_MACRO_NAME) != NULL);
        GetDlgItem(IDC_MACRO_NAME)->SetFocus();
        return;
    }

    // If the macro was saved OK end the dialog
    ASSERT(aa_->mac_dir_.Right(1) == "\\");
    if (aa_->macro_save(aa_->mac_dir_ + name_ + ".hem", NULL, comment_, halt_level_, plays_, aa_->macro_version_))
        CDialog::OnOK();
}

static DWORD id_pairs_save[] = {
    IDC_MACRO_NAME, HIDC_MACRO_NAME,
    IDC_MACRO_COMMENT, HIDC_MACRO_COMMENT,
    IDC_PLAYS, HIDC_PLAYS,
    IDC_SPIN_PLAYS, HIDC_PLAYS,
    IDC_HALT0, HIDC_HALT0,
    IDC_HALT1, HIDC_HALT1,
    IDC_HALT2, HIDC_HALT2,
    IDOK, HID_MACRO_SAVE,
    IDC_MACRO_HELP, HIDC_HELP_BUTTON,
    0,0 
}; 

BOOL CSaveMacro::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	theApp.HtmlHelpWmHelp((HWND)pHelpInfo->hItemHandle, id_pairs_save);
    return TRUE;
}

void CSaveMacro::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs_save);
}

void CSaveMacro::OnMacroHelp() 
{
    // Display help for the dialog
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_MACRO_SAVE_HELP))
        AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

/////////////////////////////////////////////////////////////////////////////
// CMacroMessage dialog

CMacroMessage::CMacroMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CMacroMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMacroMessage)
	message_ = _T("");
	//}}AFX_DATA_INIT
}

void CMacroMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMacroMessage)
	DDX_Text(pDX, IDC_MACRO_MESSAGE, message_);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMacroMessage, CDialog)
	//{{AFX_MSG_MAP(CMacroMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMacroMessage message handlers

void CMacroMessage::OnOK() 
{
    UpdateData();

    if (message_.IsEmpty())
    {
        AfxMessageBox("Please enter the text of the message.");
        ASSERT(GetDlgItem(IDC_MACRO_MESSAGE) != NULL);
        GetDlgItem(IDC_MACRO_MESSAGE)->SetFocus();
        return;
    }

    CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// GetInt dialog

IMPLEMENT_DYNAMIC(GetInt, CDialog)

GetInt::GetInt(CWnd* pParent /*=NULL*/)
	: CDialog(GetInt::IDD, pParent)
{
}

void GetInt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_VALUE, value_);
    DDV_MinMaxLong(pDX, value_, min_, max_);
    DDX_Text(pDX, IDC_PROMPT, prompt_);
}

BEGIN_MESSAGE_MAP(GetInt, CDialog)
END_MESSAGE_MAP()

BOOL GetInt::OnInitDialog() 
{
    if (min_ > max_) min_ = max_;
    CDialog::OnInitDialog();

    GetDlgItem(IDC_VALUE)->SetFocus();                  // put user straight into control
    ((CEdit*)GetDlgItem(IDC_VALUE))->SetSel(0, -1);     // select all so they can overwrite the default
    if (min_ < SHRT_MIN || max_ > SHRT_MAX)
    {
        ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN))->SetRange((short)-32768, (short)32767);
        GetDlgItem(IDC_SPIN)->ShowWindow(FALSE);
    }
    else
        ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN))->SetRange((short)min_, (short)max_);
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// GetStr dialog

IMPLEMENT_DYNAMIC(GetStr, CDialog)

GetStr::GetStr(CWnd* pParent /*=NULL*/)
	: CDialog(GetStr::IDD, pParent)
{
}

void GetStr::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PROMPT, prompt_);
    DDX_Text(pDX, IDC_VALUE, value_);
}

BEGIN_MESSAGE_MAP(GetStr, CDialog)
END_MESSAGE_MAP()

BOOL GetStr::OnInitDialog() 
{
    CDialog::OnInitDialog();

    GetDlgItem(IDC_VALUE)->SetFocus();                  // put user straight into control
    ((CEdit*)GetDlgItem(IDC_VALUE))->SetSel(0, -1);     // select all so they can overwrite the default
    return FALSE;                                       // FALSE indicates we set focus
}

/////////////////////////////////////////////////////////////////////////////
// GetBool dialog

IMPLEMENT_DYNAMIC(GetBool, CDialog)

GetBool::GetBool(CWnd* pParent /*=NULL*/)
	: CDialog(GetBool::IDD, pParent)
{
	yes_ = "&YES";
	no_ = "&NO";
}

void GetBool::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PROMPT, prompt_);
	if (!pDX->m_bSaveAndValidate)
	{
		SetDlgItemText(IDOK, yes_);
		SetDlgItemText(IDCANCEL, no_);
	}
}

BEGIN_MESSAGE_MAP(GetBool, CDialog)
END_MESSAGE_MAP()
