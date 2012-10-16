// Explorer.cpp : implementation of CExplorerWnd
//
// Copyright (c) 2004-2012 by Andrew W. Phillips.
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
#include <AfxWinAppEx.h>

#include "HexEdit.h"
#include "MainFrm.h"
#include "HexEditDoc.h"
#include "Explorer.h"
#include "HexFileList.h"
#include "CFile64.h"
#include "resource.hm"

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
// CHistoryShellList - keeps history of folders that have been shown

CHistoryShellList::~CHistoryShellList()
{
	if (m_pDropTarget != NULL)
		delete m_pDropTarget;
}

// This is a virtual function that is called whenever the current folder is to change
HRESULT CHistoryShellList::DisplayFolder(LPAFX_SHELLITEMINFO lpItemInfo)
{
	if (pExpl_ != NULL)
	{
		// Don't sync the very first time (startup) since this can really slow down the
		// startup time (especailly if there are missing network drives).
		static bool first = true;
		if (first)
			first = false;
		else if (theApp.sync_tree_)
			pExpl_->LinkToTree();    			// We should have already called tree_.SetRelatedList()
	}

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

void CHistoryShellList::ShowFile(LPCTSTR full_name)
{
	// Get the folder of the file and display it in list (and tree) ctrl
	CString path(full_name);
	::PathRemoveFileSpec(path.GetBuffer(1));
	path.ReleaseBuffer();
	DisplayFolder(path);

	// Get the file name (without path)
	LPCTSTR fname = ::PathFindFileName(full_name);

	// get the pidl for the file
	LPITEMIDLIST pidl;
	ULONG chEaten;
	ULONG dwAttributes = 0;
	OLECHAR olePath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fname, -1, olePath, MAX_PATH);
	HRESULT hr = m_psfCurFolder->ParseDisplayName(NULL, NULL, olePath, &chEaten, &pidl, &dwAttributes);
	if (hr != S_OK)
	{
		ASSERT(0);
		return;
	}

	// Find matching pidl in our list and enable it (disable all others)
	int curr = -1;
	LVITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));

	while ((curr = GetNextItem(curr, LVNI_ALL)) > -1)
	{
		// Get item pidl
		//if (((LPAFX_SHELLITEMINFO)GetItemData(curr))->pidlRel == pidl) ...
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = curr;
		if (GetItem(&lvItem))
		{
			LPAFX_SHELLITEMINFO pInfo = (LPAFX_SHELLITEMINFO)lvItem.lParam;

			// select the item
			if (m_psfCurFolder->CompareIDs(0, pidl, pInfo->pidlRel) == 0)
			{
				SetItemState(curr, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				EnsureVisible(curr, TRUE);
				SetFocus();
			}
		}
	}

	// Even though doc does not say so I believe you must free the PIDL created by ParseDisplayName
	theApp.GetShellManager()->FreeItem(pidl);
}

void CHistoryShellList::Back(int count)
{
	do_move(pos_ - count);
}

void CHistoryShellList::Forw(int count)
{
	do_move(pos_ + count);
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
	_T("Created"),          // COLCREATED
	_T("Accessed"),         // COLACCESSED
	// always add to the end of this list as existing registry strings may rely on this order
	NULL
};

char * CHistoryShellList::defaultWidths = "150|60|150|150|60|150|60|100|200|150|150";  // name,size,type,mod-time,attr,last-opened-time,cat,kw,comments,cre,acc

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
	CHexFileList *pfl = theApp.GetFileList();

	// We use fl_idx_ to store the index into recent file list (pfl) of the current file,
	// which saves time by avoiding calls to SHGetPathFromIDList for each column.
	// This is obtained when getting the first of our columns (COLATTR) but this may need
	// fixing when the user can reorder columns (not sure COLATTR will still be done first).
	// (A better soln might be to save pItem to compare in the next call.)
	if (iColumn == COLNAME)
	{
		fl_idx_ = -2;  // Reset recent file list index at the start of a new row to help detect bugs/problems
		fl_path_[0] = '\0';
	}

	if (iColumn == COLSIZE)
	{
		// We get the index for this file from the recent file list here to save time repeating the
		// procedure (get path and then look up index) for the other columns.
		// IMPORTANT: This assumes that CMFCShellListCtrl::OnGetItemText calls process the list box items
		// a row at a time, from left to right (orig order) - if this assumption changes this will stuff up.
		if (SHGetPathFromIDList(pItem->pidlFQ, fl_path_))
		{
			ASSERT(fl_idx_ == -2);
			if (pfl != NULL)
				fl_idx_ = pfl->GetIndex(fl_path_);

			// Handle size column ourselves since base class can't handle file sizes > 2Gbytes [xxx - check this for new MFC base class]
			if (CFile64::GetStatus(fl_path_, fl_status_))
			{
				if ((fl_status_.m_attribute & (CFile::directory | CFile ::volume)) != 0)
					return _T("");
				CString ss = NumScale(double(fl_status_.m_size));
				if (ss.Right(1) == _T(" "))
					return ss;
				else
					return ss + _T("B");
			}
		}
	}

	// Call BCG base class for columns it can handle
	if (iColumn < COLATTR)
		return CMFCShellListCtrl::OnGetItemText(iItem, iColumn, pItem);

	CString retval;

	switch (iColumn)
	{
	case COLATTR:  // Attributes
		if (fl_path_[0] != '\0')
			return GetAttributes(fl_path_);
		break;

	case COLOPENED:  // Date last opened in HexEdit
		if (pfl != NULL && fl_idx_ > -1)
		{
			OnFormatFileDate(CTime(pfl->GetOpened(fl_idx_)), retval);
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

	case COLCREATED:
		if (fl_path_[0] != '\0')
		{
			CString str;
			OnFormatFileDate(fl_status_.m_ctime, str);
			return str;
		}
		break;
	case COLACCESSED:
		if (fl_path_[0] != '\0')
		{
			CString str;
			OnFormatFileDate(fl_status_.m_atime, str);
			return str;
		}
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

	CFileStatus fs1, fs2;

	// Handle size column ourselves since BCG base class can't handle file sizes > 2Gbytes
	switch (iColumn)
	{
	// Handle file size and modification date ourselves as base calss does not separate folders from files.
	case COLMOD:
	case COLSIZE:
	case COLCREATED:
	case COLACCESSED:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			CFile64::GetStatus(path1, fs1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			CFile64::GetStatus(path2, fs2) )
		{
			// Make sure folders are sorted away from files
			bool is_dir1, is_dir2;
			(void)GetAttributes(path1, &is_dir1);
			(void)GetAttributes(path2, &is_dir2);
			if (is_dir1 || is_dir2)
			{
				if (is_dir1 && is_dir2)
					return CString(path1).CompareNoCase(path2);   // Sort folders on name
				else if (is_dir1)
					return -1;
				else
					return 1;
			}
			if (iColumn == COLMOD)
				return fs1.m_mtime < fs2.m_mtime ? -1 : fs1.m_mtime > fs2.m_mtime ? 1 : 0;  // compare modification times
			else if (iColumn == COLCREATED)
				return fs1.m_ctime < fs2.m_ctime ? -1 : fs1.m_ctime > fs2.m_ctime ? 1 : 0;  // compare creation times
			else if (iColumn == COLACCESSED)
				return fs1.m_atime < fs2.m_atime ? -1 : fs1.m_atime > fs2.m_atime ? 1 : 0;  // compare access times
			else
				return fs1.m_size < fs2.m_size ? -1 : fs1.m_size > fs2.m_size ? 1 : 0;      // compare sizes
		}
		break;

	case COLATTR:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) )
		{
			bool is_dir1, is_dir2;
			CString s1 = GetAttributes(path1, &is_dir1);
			CString s2 = GetAttributes(path2, &is_dir2);

			// Check for folders so we can sort them separately from files
			if (is_dir1 != is_dir2)
			{
				// One (and only one) is a folder
				if (is_dir1)
					return -1;
				else
					return 1;
			}
			return s1 < s2 ? -1 : s1 > s2 ? 1 : 0;
		}
		break;

	case COLOPENED:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			(pfl = theApp.GetFileList()) != NULL)
		{
			bool is_dir1, is_dir2;
			(void)GetAttributes(path1, &is_dir1);
			(void)GetAttributes(path2, &is_dir2);
			if (is_dir1 || is_dir2)
			{
				if (is_dir1 && is_dir2)
					return CString(path1).CompareNoCase(path2);   // Sort folders on name
				else if (is_dir1)
					return -1;
				else
					return 1;
			}

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
		break;

	case COLCATEGORY:
		if (SHGetPathFromIDList(pItem1->pidlFQ, path1) &&
			SHGetPathFromIDList(pItem2->pidlFQ, path2) &&
			(pfl = theApp.GetFileList()) != NULL)
		{
			bool is_dir1, is_dir2;
			GetAttributes(path1, &is_dir1);
			GetAttributes(path2, &is_dir2);
			if (is_dir1 || is_dir2)
			{
				if (is_dir1 && is_dir2)
					return CString(path1).CompareNoCase(path2);   // Sort folders on name
				else if (is_dir1)
					return -1;
				else
					return 1;
			}

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
			bool is_dir1, is_dir2;
			GetAttributes(path1, &is_dir1);
			GetAttributes(path2, &is_dir2);
			if (is_dir1 || is_dir2)
			{
				if (is_dir1 && is_dir2)
					return CString(path1).CompareNoCase(path2);   // Sort folders on name
				else if (is_dir1)
					return -1;
				else
					return 1;
			}

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
			bool is_dir1, is_dir2;
			GetAttributes(path1, &is_dir1);
			GetAttributes(path2, &is_dir2);
			if (is_dir1 || is_dir2)
			{
				if (is_dir1 && is_dir2)
					return CString(path1).CompareNoCase(path2);   // Sort folders on name
				else if (is_dir1)
					return -1;
				else
					return 1;
			}

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

// Returns the attributes of a file as a string of the form "RHSAICE" where the letter
// is missing if the attribute is off.  Also returns true in *p_is_dir if it is a folder.
CString CHistoryShellList::GetAttributes(LPCTSTR path, bool * p_is_dir /*=NULL*/)
{
	CString retval;
	if (p_is_dir != NULL)
		*p_is_dir = false;
	DWORD attr = GetFileAttributes(path);

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		// GetFileAttributes() may fail on locked files like PAGEFILE.SYS,
		// so the backup is to use FindFirstFile (slower alternative)
		WIN32_FIND_DATA wfd;
		if (FindFirstFile(path, &wfd) == INVALID_HANDLE_VALUE)
			attr = -1;
		else
			attr = wfd.dwFileAttributes;
	}
	if (attr != -1)
	{
		if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0 && p_is_dir != NULL)
			*p_is_dir = true;

		if ((attr & FILE_ATTRIBUTE_READONLY) != 0)
			retval += _T("R");
		if ((attr & FILE_ATTRIBUTE_HIDDEN) != 0)
			retval += _T("H");
		if ((attr & FILE_ATTRIBUTE_SYSTEM) != 0)
			retval += _T("S");
		if ((attr & FILE_ATTRIBUTE_ARCHIVE) != 0)
			retval += _T("A");
		if (theApp.show_not_indexed_)
		{
			if ((attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0)  // not-indexed flag is on
				retval += _T("N");
		}
		else
		{
			if ((attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) == 0)  // not-indexed flag is off = indexed
				retval += _T("I");
		}
		if ((attr & FILE_ATTRIBUTE_COMPRESSED) != 0)
			retval += _T("C");
		if ((attr & FILE_ATTRIBUTE_ENCRYPTED) != 0)
			retval += _T("E");
	}
	return retval;
}

BEGIN_MESSAGE_MAP(CHistoryShellList, CMFCShellListCtrl)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)
END_MESSAGE_MAP()

// CHistoryShellList message handlers

int CHistoryShellList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCShellListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pDropTarget = new CExplorerDropTarget (this);
	m_pDropTarget->Register (this);

	return 0;
}

void CHistoryShellList::OnContextMenu(CWnd * pWnd, CPoint point)
{
	ASSERT_VALID(this);
	CPoint pt(point);
	ScreenToClient(&pt);
	CRect rct;
	GetHeaderCtrl().GetWindowRect(&rct);
	if (point.y > -1 && pt.y < rct.Height())
	{
		// Right click on header - display menu of available columns
		CMenu mm;
		mm.CreatePopupMenu();

		for (int col = 0; col < COLLAST; ++col)
		{
			if (col == COLOPENED || col == COLCREATED)
				mm.AppendMenu(MF_SEPARATOR);
			mm.AppendMenu(MF_ENABLED|(GetColumnWidth(col)>0?MF_CHECKED:0), col+1, colnames[col]);
		}

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

	ASSERT(m_bContextMenu);
	CDummyShellManager * psm = DYNAMIC_DOWNCAST(CDummyShellManager, theApp.GetShellManager());
	ASSERT(psm->m_pMalloc != NULL);

	if (m_pContextMenu2 != NULL || m_psfCurFolder == NULL) return;

	UINT nSelItems = GetSelectedCount();
	if (nSelItems == 0)	return;
	int nClickedItem = -1;

	if (point.x == -1 && point.y == -1)
	{
		// Keyboard, show menu for the currently selected item(s):
		int nCurItem = -1;
		int nLastSelItem = -1;

		for (UINT i = 0; i < nSelItems; i++)
		{
			nCurItem = GetNextItem(nCurItem, LVNI_SELECTED);
			nLastSelItem = nCurItem;  // xxx why do we need both vars?
		}

		// Get the location to display the context menu from the last selected item
		CRect rectItem;
		if (GetItemRect(nLastSelItem, rectItem, LVIR_BOUNDS))
		{
			point.x = rectItem.left;
			point.y = rectItem.bottom + 1;

			ClientToScreen(&point);
		}
	}
	else
	{
		// Clicked on specifed item:
		LVHITTESTINFO lvhti;
		lvhti.pt = point;
		ScreenToClient(&lvhti.pt);

		lvhti.flags = LVHT_NOWHERE;

		HitTest(&lvhti);

		if ((lvhti.flags & LVHT_ONITEM) == 0)
		{
			// Click ouside of items, do nothing
			return;
		}

		nClickedItem = lvhti.iItem;
	}

	LPITEMIDLIST* pPidls = (LPITEMIDLIST*)psm->m_pMalloc->Alloc(sizeof(LPITEMIDLIST) * nSelItems);
	ENSURE(pPidls != NULL);

	// Get the selected items:
	LVITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_PARAM;

	LPAFX_SHELLITEMINFO pClickedInfo = (LPAFX_SHELLITEMINFO)lvItem.lParam;

	if (nClickedItem >= 0)
	{
		// Put the item clicked on first in the list:
		lvItem.iItem = nClickedItem;

		if (GetItem(&lvItem))
		{
			pClickedInfo = (LPAFX_SHELLITEMINFO)lvItem.lParam;
			pPidls [0] = pClickedInfo->pidlRel;
		}
	}

	int nCurItem = -1;
	for (UINT i = nClickedItem >= 0 ? 1 : 0; i < nSelItems; i++)
	{
		nCurItem = GetNextItem(nCurItem, LVNI_SELECTED);
		if (nCurItem != nClickedItem)
		{
			lvItem.iItem = nCurItem;

			if (GetItem(&lvItem))
			{
				LPAFX_SHELLITEMINFO pInfo = (LPAFX_SHELLITEMINFO)lvItem.lParam;
				pPidls [i] = pInfo->pidlRel;

				if (pClickedInfo == NULL)
				{
					pClickedInfo = pInfo;
				}
			}
		}
		else
		{
			i--;
		}
	}

	if (pPidls [0] == NULL)
	{
		psm->m_pMalloc->Free(pPidls);
		return;
	}

	IContextMenu* pcm;
	HRESULT hr = m_psfCurFolder->GetUIObjectOf(GetSafeHwnd(), nSelItems, (LPCITEMIDLIST*)pPidls, IID_IContextMenu, NULL, (LPVOID*)&pcm);

	if (SUCCEEDED(hr))
	{
		hr = pcm->QueryInterface(IID_IContextMenu2, (LPVOID*)&m_pContextMenu2);

		if (SUCCEEDED(hr))
		{
			HMENU hPopup = ::CreatePopupMenu();
			if (hPopup != NULL)
			{
				// Note: if this call takes a long time (> 5 secs) then there is probably a problem with an Explorer
				// Shell Extension - use ShellExView to check and disable possible problem extensions.
				hr = m_pContextMenu2->QueryContextMenu(hPopup, 0, 1, 0x7fff, CMF_NORMAL /* | CMF_EXPLORE */);

				if (SUCCEEDED(hr))
				{
					// First work out if we have a folder selected
					BOOL bIsFolder = FALSE;
					if (nSelItems == 1)
					{
						ULONG ulAttrs = SFGAO_FOLDER;
						m_psfCurFolder->GetAttributesOf(1, (const struct _ITEMIDLIST **) &pClickedInfo->pidlRel, &ulAttrs);

						if (ulAttrs & SFGAO_FOLDER)
							bIsFolder = TRUE;
					}

					UINT firstCustomCmd = hr;            // QueryContextMenu returns first ID that we can use for our own items
					if (!bIsFolder && theApp.custom_explorer_menu_)
						AdjustMenu(hPopup, firstCustomCmd, nSelItems, (LPCITEMIDLIST*)pPidls);  // Add our own menu items

					UINT idCmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, GetSafeHwnd(), NULL);

					if (idCmd == 0)
					{
						// user cancelled - do nothing
					}
					else if (bIsFolder && idCmd == ::GetMenuDefaultItem(hPopup, FALSE, 0))
					{
						// Selected default operation (Open) on a single folder so just display it
						DisplayFolder(pClickedInfo);
					}
					else if (!bIsFolder && idCmd >= firstCustomCmd && idCmd < firstCustomCmd + ID_LAST)
					{
						// Handle our custom commands for files (not folders)
						HandleCustomCommand(idCmd - firstCustomCmd, nSelItems, (LPCITEMIDLIST*)pPidls);
					}
					else
					{
						// Default is to let Windows Shell handle it
						CMINVOKECOMMANDINFO cmi;
						cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
						cmi.fMask = 0;
						cmi.hwnd = (HWND) GetParent();
						cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - 1);
						cmi.lpParameters = NULL;
						cmi.lpDirectory = NULL;
						cmi.nShow = SW_SHOWNORMAL;
						cmi.dwHotKey = 0;
						cmi.hIcon = NULL;

						hr = pcm->InvokeCommand(&cmi);

						if (SUCCEEDED(hr) && GetParent() != NULL)
						{
							GetParent()->SendMessage(AFX_WM_ON_AFTER_SHELL_COMMAND, (WPARAM) idCmd);
						}
					}
				}
			}

			if (m_pContextMenu2 != NULL)
			{
				m_pContextMenu2->Release();
				m_pContextMenu2 = NULL;
			}
		}

		pcm->Release();
	}

	psm->m_pMalloc->Free(pPidls);
}

void CHistoryShellList::HandleCustomCommand(UINT cmd, UINT nSelItems, LPCITEMIDLIST *piil)
{
	CString path_name;
	if (!GetCurrentFolder(path_name))
		return;

	// For non-reversible operations we ask the user if they are sure they want to do it
	if (cmd == ID_DELETE || cmd == ID_WIPE)
	{
		CString mess;
		mess.Format("Are you sure you want to %s %d file(s)?",
		            cmd == ID_DELETE ? "delete" : "wipe",
					nSelItems);
		if (TaskMessageBox("Confirm Operation", mess, MB_YESNO) != IDYES)
			return;
	}

	// If setting a file time: evaluate the date expression now
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	FILETIME new_time;
	if (cmd >= ID_TIME_MOD && cmd <= ID_LAST && mm != NULL)
	{
		// We really should get the date expression that was shown on the menu
		// item (now() or a date var name), but I couldn't work out how to get
		// the sub-menu text from the command ID.  Doing it this way relies on the
		// code matching the order in the menus.
		int var = 0;
		if (cmd < ID_TIME_CRE)
			var = cmd - ID_TIME_MOD;
		else if (cmd < ID_TIME_ACC)
			var = cmd - ID_TIME_CRE;
		else if (cmd < ID_TIME_ALL)
			var = cmd - ID_TIME_ACC;
		else
			var = cmd - ID_TIME_ALL;
		ASSERT(var >= 0 && var < ID_TIME_CRE - ID_TIME_MOD - 1);

		SYSTEMTIME st;
		GetLocalTime(&st);  // default to current time (local not GMT/UTC/Zulu)

		// Unless the first (Current Time) menu item get the time from the DATE var by evaluating it as an expression
		if (var > 0)
		{
			CString ss = mm->expr_.GetVarNames(CJumpExpr::TYPE_DATE)[var - 1];

			int ac;
			CHexExpr::value_t vv = mm->expr_.evaluate(ss, 0 /*unused*/, ac /*unused*/, 10, false);
			if ((expr_eval::type_t)vv.typ == CHexExpr::TYPE_DATE)
				VariantTimeToSystemTime(vv.date, &st);
			else
				AfxMessageBox("Date variable \"" + ss + "\" not found");
		}

		FILETIME local_time;
		SystemTimeToFileTime(&st, &local_time);          // system time (local) to file time (local)
		LocalFileTimeToFileTime(&local_time, &new_time); // file times are UTC, so convert from local to GMT
	}

	DWORD errnum = NO_ERROR;         // Windows error code if something goes wrong
	char fname[_MAX_FNAME], ext[_MAX_EXT];

	CWaitCursor wc;

	// Perform the operation on all the selected files
	for (int item = 0; item < nSelItems; ++item)
	{
		BOOL ok = TRUE;
		char tmp[_MAX_PATH];
		SHGetPathFromIDList(piil[item], tmp);

		_splitpath(tmp, NULL, NULL, fname, ext);

		CString full_name(path_name + "\\" + fname + ext);

		DWORD attr = GetFileAttributes(full_name);
		if (attr == INVALID_FILE_ATTRIBUTES)
		{
			// GetFileAttributes() may fail on locked files like PAGEFILE.SYS,
			// so the backup is to use FindFirstFile (slower alternative)
			WIN32_FIND_DATA wfd;
			if (FindFirstFile(full_name, &wfd) == INVALID_HANDLE_VALUE)
				attr = -1;
			else
				attr = wfd.dwFileAttributes;
		}
		if (attr == -1)
		{
			errnum = GetLastError();
			continue;
		}
		else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			continue;  // ignore directories
		}

		if (cmd >= ID_TIME_MOD && cmd < ID_TIME_CRE)
		{
			ok = SetFileTimes(full_name, NULL, NULL, &new_time);
		}
		else if (cmd >= ID_TIME_CRE && cmd < ID_TIME_ACC)
		{
			ok = SetFileTimes(full_name, &new_time);
		}
		else if (cmd >= ID_TIME_ACC && cmd < ID_TIME_ALL)
		{
			ok = SetFileTimes(full_name, NULL, &new_time);
		}
		else if (cmd >= ID_TIME_ALL && cmd < ID_LAST)
		{
			ok = SetFileTimes(full_name, &new_time, &new_time, &new_time);
		}
		else switch (cmd)
		{
		case ID_OPEN:
			ASSERT(theApp.open_current_readonly_ == -1);
			theApp.open_current_readonly_ = FALSE;
			ok = theApp.OpenDocumentFile(full_name) != NULL;
			break;
		case ID_OPEN_RO:
			ASSERT(theApp.open_current_readonly_ == -1);
			theApp.open_current_readonly_ = TRUE;
			ok = theApp.OpenDocumentFile(full_name) != NULL;
			break;
		case ID_READ_ONLY_ON:
			ok = SetFileAttributes(full_name, attr | FILE_ATTRIBUTE_READONLY);
			break;
		case ID_READ_ONLY_OFF:
			ok = SetFileAttributes(full_name, attr & ~FILE_ATTRIBUTE_READONLY);
			break;
		case ID_HIDDEN_ON:
			ok = SetFileAttributes(full_name, attr | FILE_ATTRIBUTE_HIDDEN);
			break;
		case ID_HIDDEN_OFF:
			ok = SetFileAttributes(full_name, attr & ~FILE_ATTRIBUTE_HIDDEN);
			break;
		case ID_SYSTEM_ON:
			ok = SetFileAttributes(full_name, attr | FILE_ATTRIBUTE_SYSTEM);
			break;
		case ID_SYSTEM_OFF:
			ok = SetFileAttributes(full_name, attr & ~FILE_ATTRIBUTE_SYSTEM);
			break;
		case ID_ARCHIVE_ON:
			ok = SetFileAttributes(full_name, attr | FILE_ATTRIBUTE_ARCHIVE);
			break;
		case ID_ARCHIVE_OFF:
			ok = SetFileAttributes(full_name, attr & ~FILE_ATTRIBUTE_ARCHIVE);
			break;
		case ID_INDEX_ON:
			ok = SetFileAttributes(full_name, attr & ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
			break;
		case ID_INDEX_OFF:
			ok = SetFileAttributes(full_name, attr | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
			break;
		case ID_COMPRESSED_ON:
			ok = SetFileCompression(full_name, COMPRESSION_FORMAT_DEFAULT);
			break;
		case ID_COMPRESSED_OFF:
			ok = SetFileCompression(full_name, COMPRESSION_FORMAT_NONE);
			break;
		case ID_ENCRYPTED_ON:
			ok = EncryptFile(full_name);
			break;
		case ID_ENCRYPTED_OFF:
			ok = DecryptFile(full_name, 0);
			break;

		case ID_DELETE:
			theApp.CloseByName(full_name);
			ok = DeleteFile(full_name);
			break;
		case ID_WIPE:
			theApp.CloseByName(full_name);
			ok = WipeFile(full_name, theApp.wipe_type_);
			if (ok)
				ok = DeleteFile(full_name);
			break;
		case ID_EXPLORER_OPTIONS:
			theApp.display_options(EXPLORER_OPTIONS_PAGE, TRUE);
			Refresh();
			ok = TRUE;
			break;
		}

		// Remember that we had an error on at least one file.
		// Note displayed info. will be incomplete if multiple files have errors.
		// Note 2. Be careful not to wipe out Windows error between failure (in fucntion above)
		// an the call to GetLastError() - eg the CWaitCursor d'tor sets the error to zero.
		if (!ok)
			errnum = GetLastError();
	}

	wc.Restore();

	if (errnum != NO_ERROR)
	{
		const char * op = "";
		if (cmd >= ID_TIME_MOD && cmd < ID_TIME_CRE)
			op = "setting the modification time of";
		else if (cmd >= ID_TIME_CRE && cmd < ID_TIME_ACC)
			op = "setting the creation time of";
		else if (cmd >= ID_TIME_ACC && cmd < ID_TIME_ALL)
			op = "setting the access time of";
		else if (cmd >= ID_TIME_ALL && cmd < ID_LAST)
			op = "setting the times of";
		else switch (cmd)
		{
		case ID_OPEN:
			op = "opening";
			break;
		case ID_OPEN_RO:
			op = "opening (read only)";
			break;

		case ID_READ_ONLY_ON:
			op = "setting the read only attribute of";
			break;
		case ID_READ_ONLY_OFF:
			op = "clearing the read only attribute of";
			break;
		case ID_HIDDEN_ON:
			op = "setting the hidden attribute of";
			break;
		case ID_HIDDEN_OFF:
			op = "clearing the hidden attribute of";
			break;
		case ID_SYSTEM_ON:
			op = "setting the system attribute of";
			break;
		case ID_SYSTEM_OFF:
			op = "clearing the system attribute of";
			break;
		case ID_ARCHIVE_ON:
			op = "setting the archive attribute of";
			break;
		case ID_ARCHIVE_OFF:
			op = "clearing the archive attribute of";
			break;
		case ID_INDEX_ON:
			if (theApp.show_not_indexed_)
				op = "clearing the non-indexed attribute for";
			else
				op = "adding indexing for";
			break;
		case ID_INDEX_OFF:
			if (theApp.show_not_indexed_)
				op = "setting the non-indexed attribute for";
			else
				op = "removing indexing for";
			break;
		case ID_COMPRESSED_ON:
			op = "compressing";
			break;
		case ID_COMPRESSED_OFF:
			op = "decompressing";
			break;
		case ID_ENCRYPTED_ON:
			op = "encrypting";
			break;
		case ID_ENCRYPTED_OFF:
			op = "decrypting";
			break;

		case ID_DELETE:
			op = "deleting";
			break;
		case ID_WIPE:
			op = "wiping";
			break;
		default:
			ASSERT(0);
			op = "???";
			break;
		}

		CString strError("unknown error");
		{
			LPTSTR emess;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							  NULL,
							  errnum,
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							  (LPTSTR) &emess,
							  0,
							  NULL))
			{
				strError = emess;
				LocalFree(emess);
			}
		}

		CString strMess;
		strMess.Format("The error:\n\n%s\noccurred while %s %s", strError, op, (nSelItems > 1 ? "at least one of the files" : CString(fname) + ext));
		CAvoidableDialog::Show(IDS_FILE_ATTR, strMess);
	}
}

void CHistoryShellList::AdjustMenu(HMENU hm, UINT firstCustomCmd, UINT nSelItems, LPCITEMIDLIST *piil)
{
	int ii = 0;                 // Current menu item number
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	// Attach menu handle to MFC class to make it easier to work with
	CMenu mPopup;
	VERIFY(mPopup.Attach(hm));

	mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_BYPOSITION);
	mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_OPEN, _T("Open in HexEdit Pro"));
	mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_OPEN_RO, _T("Open Read-Only"));

	// Create 3 sub-menus that allows setting of modified, accessed, created time of files to
	// current time or a time taken from a "TYPE_DATE" variable
	CMenu mMod, mCre, mAcc, mAll;

	mMod.CreatePopupMenu();
	mCre.CreatePopupMenu();
	mAcc.CreatePopupMenu();
	mAll.CreatePopupMenu();
	mMod.AppendMenu(MF_STRING, firstCustomCmd + ID_TIME_MOD, "Current Time");
	mCre.AppendMenu(MF_STRING, firstCustomCmd + ID_TIME_CRE, "Current Time");
	mAcc.AppendMenu(MF_STRING, firstCustomCmd + ID_TIME_ACC, "Current Time");
	mAll.AppendMenu(MF_STRING, firstCustomCmd + ID_TIME_ALL, "Current Time");

	// Get all date variables and add them to the 3 menus
	std::vector<CString> varNames = mm->expr_.GetVarNames(CJumpExpr::TYPE_DATE);
	for (int nn = 0; nn < varNames.size(); ++nn)
	{
		mMod.AppendMenuA(MF_STRING, firstCustomCmd + ID_TIME_MOD + nn + 1, varNames[nn]);
		mCre.AppendMenuA(MF_STRING, firstCustomCmd + ID_TIME_CRE + nn + 1, varNames[nn]);
		mAcc.AppendMenuA(MF_STRING, firstCustomCmd + ID_TIME_ACC + nn + 1, varNames[nn]);
		mAll.AppendMenuA(MF_STRING, firstCustomCmd + ID_TIME_ALL + nn + 1, varNames[nn]);
		if (nn > ID_TIME_CRE - ID_TIME_MOD - 2)
			break;                             // don't add too many
	}

	mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_BYPOSITION);
	mPopup.InsertMenu(ii++, MF_POPUP | MF_BYPOSITION, (UINT)mMod.m_hMenu, _T("Set Modified Time"));
	mPopup.InsertMenu(ii++, MF_POPUP | MF_BYPOSITION, (UINT)mCre.m_hMenu, _T("Set Creation Time"));
	mPopup.InsertMenu(ii++, MF_POPUP | MF_BYPOSITION, (UINT)mAcc.m_hMenu, _T("Set Accessed Time"));
	mPopup.InsertMenu(ii++, MF_POPUP | MF_BYPOSITION, (UINT)mAll.m_hMenu, _T("Set All Times"));
	mMod.DestroyMenu();
	mCre.DestroyMenu();
	mAcc.DestroyMenu();
	mAll.DestroyMenu();

	CString path_name;
	if (GetCurrentFolder(path_name))
	{
		// Menu items for toggleable flags - first we need to get the current state
		bool is_ro = true, is_hidden = true, is_sys = true, is_arc = true, is_index = true;
		bool is_compressed = true, is_encrypted = true;
		for (int item = 0; item < nSelItems; ++item)
		{
			char tmp[_MAX_PATH];
			SHGetPathFromIDList(piil[item], tmp);

			char fname[_MAX_FNAME], ext[_MAX_EXT];
			_splitpath(tmp, NULL, NULL, fname, ext);

			CString full_name(path_name + "\\" + fname + ext);

			DWORD attr = GetFileAttributes(full_name);
			if (attr == INVALID_FILE_ATTRIBUTES)
			{
				// GetFileAttributes() may fail on locked files like PAGEFILE.SYS,
				// so the backup is to use FindFirstFile (slower alternative)
				WIN32_FIND_DATA wfd;
				if (FindFirstFile(full_name, &wfd) == INVALID_HANDLE_VALUE)
					attr = -1;
				else
					attr = wfd.dwFileAttributes;
			}
			if (attr == -1 || (attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
				continue;     // skip directories

			if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
				is_ro = false;
			if ((attr & FILE_ATTRIBUTE_HIDDEN) == 0)
				is_hidden = false;
			if ((attr & FILE_ATTRIBUTE_SYSTEM) == 0)
				is_sys = false;
			if ((attr & FILE_ATTRIBUTE_ARCHIVE) == 0)
				is_arc = false;
			if ((attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0)
				is_index = false;
			if ((attr & FILE_ATTRIBUTE_COMPRESSED) == 0)
				is_compressed = false;
			if ((attr & FILE_ATTRIBUTE_ENCRYPTED) == 0)
				is_encrypted = false;
		}
		mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_BYPOSITION);
		if (is_ro)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_READ_ONLY_OFF, _T("Read Only"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_READ_ONLY_ON, _T("Read Only"));
		if (is_hidden)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_HIDDEN_OFF, _T("Hidden"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_HIDDEN_ON, _T("Hidden"));
		if (is_sys)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_SYSTEM_OFF, _T("System"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_SYSTEM_ON, _T("System"));
		if (is_arc)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_ARCHIVE_OFF, _T("Archive"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_ARCHIVE_ON, _T("Archive"));
		if (theApp.show_not_indexed_)
		{
			if (is_index)
				mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_INDEX_OFF, _T("Not Indexed"));
			else
				mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_INDEX_ON, _T("Not Indexed"));
		}
		else
		{
			if (is_index)
				mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_INDEX_OFF, _T("Indexed"));
			else
				mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_INDEX_ON, _T("Indexed"));
		}
		if (is_compressed)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_COMPRESSED_OFF, _T("Compressed"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_COMPRESSED_ON, _T("Compressed"));
		if (is_encrypted)
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION | MF_CHECKED, firstCustomCmd + ID_ENCRYPTED_OFF, _T("Encrypted"));
		else
			mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_ENCRYPTED_ON, _T("Encrypted"));
	}

	mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_BYPOSITION);
	mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_DELETE, "Delete...");
	CString strMenu("Wipe... ");
	switch (theApp.wipe_type_)
	{
	case WIPE_FAST:
		strMenu += "(Fast)";
		break;
	case WIPE_GOOD: 
		strMenu += "(Good)";
		break;
	case WIPE_THOROUGH:
		strMenu += "(Thorough)";
		break;
	}
	mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_WIPE, strMenu);

	mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_BYPOSITION);
	mPopup.InsertMenu(ii++, MF_STRING | MF_BYPOSITION, firstCustomCmd + ID_EXPLORER_OPTIONS, "Options...");

	mPopup.InsertMenu(ii++, MF_SEPARATOR | MF_MENUBARBREAK | MF_BYPOSITION);
	mPopup.Detach();
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

void CHistoryShellList::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	// NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;  // we ignore this and get all selected items

	// Get all selected files/folders
	std::vector<CString> src;           // List of all the file/folder names
	size_t total_len = 0;               // Length of all strings including nul terminators
	for (POSITION pos = GetFirstSelectedItemPosition(); pos != NULL; )
	{
		// Get info (item data) about this selected item
		int iItem = GetNextSelectedItem(pos);
		LPAFX_SHELLITEMINFO pItem = (LPAFX_SHELLITEMINFO) GetItemData (iItem);

		// Get file/folder name of the selected item
		TCHAR szPath [MAX_PATH];
		VERIFY(SHGetPathFromIDList(pItem->pidlFQ, szPath));
		src.push_back(szPath);

		total_len += src.back().GetLength() + 1;
	}

	// Get global memory for CF_HDROP (DROPFILES structure followed by list of names)
	HGLOBAL hg = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + sizeof(TCHAR)*(total_len + 1));
	if (hg == NULL)
		return;

	DROPFILES * pdf = (DROPFILES *)::GlobalLock(hg);    // Ptr to DROPFILES
	pdf->pFiles = sizeof(*pdf);                         // Size of DROPFILES
#ifdef _UNICODE
	pdf->fWide = TRUE;
#endif
	ASSERT(pdf != NULL);
	TCHAR * pp = (TCHAR *)((char *)pdf + sizeof(*pdf)); // Ptr to where we put the names
	for (std::vector<CString>::const_iterator psrc = src.begin(); psrc != src.end(); ++psrc)
	{
		size_t len = psrc->GetLength();
		_tcscpy(pp, (LPCTSTR)(*psrc));
		pp += len;
		*pp++ = '\0';
	}
	*pp = '\0';
	::GlobalUnlock(hg);

	COleDataSource ods;
	FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	ods.CacheGlobalData(CF_HDROP, hg, &etc);

	bool success = false;

	switch (ods.DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE))
	{
	case DROPEFFECT_COPY:
	case DROPEFFECT_MOVE:
		// The files/folders were copied/moved
		success = true;
		break;

	case DROPEFFECT_NONE:
		if ((::GetVersion() & 0x80000000) == 0)  // NT family (NT4/W2K/XP etc)
		{
			// Under NT a move returns DROPEFFECT_NONE so we have to see if
			// the files are still there before we know if the move went ahead.
			for (std::vector<CString>::const_iterator psrc = src.begin(); psrc != src.end(); ++psrc)
			{
				if (!::PathFileExists(*psrc))
				{
					// We found a (at least one) file is gone (presumably moved)
					success = true;
					break;
				}
			}
		}
		break;
	default:
		ASSERT(0);
		success = true;         // Safer to not delete memory in case we don't own it
		break;
	}
	if (success)
		Refresh();
	else
		::GlobalFree(hg);       // We still own memory so free it

	*pResult = 0;
}

// This is very similar the base class version CMFCShellListCtrl::EnumObjects() but I had
// to duplicate the code in order to do filtering on the file name.
HRESULT CHistoryShellList::EnumObjects(LPSHELLFOLDER pParentFolder, LPITEMIDLIST pidlParent)
{
	ASSERT_VALID(this);

	LPENUMIDLIST pEnum;
	HRESULT hRes = pParentFolder->EnumObjects(NULL, m_nTypes, &pEnum);

	if (SUCCEEDED(hRes))
	{
		LPITEMIDLIST pidlTemp;
		DWORD dwFetched = 1;
		LPAFX_SHELLITEMINFO pItem;

		//enumerate the item's PIDLs
		while (pEnum->Next(1, &pidlTemp, &dwFetched) == S_OK && dwFetched)
		{
			SHFILEINFO	sfi;
			sfi.dwAttributes = SFGAO_FOLDER;
			VERIFY(SHGetFileInfo((LPCTSTR)pidlTemp, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED));
			if ((sfi.dwAttributes & SFGAO_FOLDER) == 0 && !PathMatchSpec(sfi.szDisplayName, m_filter))
			{
				dwFetched = 0;
				continue;       // skip this file as it does not match the current filter
			}

			LVITEM lvItem;
			ZeroMemory(&lvItem, sizeof(lvItem));

			//fill in the TV_ITEM structure for this item
			lvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;

			//AddRef the parent folder so it's pointer stays valid
			pParentFolder->AddRef();

			//put the private information in the lParam
			pItem = (LPAFX_SHELLITEMINFO)GlobalAlloc(GPTR, sizeof(AFX_SHELLITEMINFO));

			pItem->pidlRel = pidlTemp;
			pItem->pidlFQ = theApp.GetShellManager()->ConcatenateItem(pidlParent, pidlTemp);

			pItem->pParentFolder = pParentFolder;
			lvItem.lParam = (LPARAM)pItem;

			lvItem.pszText = _T("");
			lvItem.iImage = OnGetItemIcon(GetItemCount(), pItem);

			//determine if the item is shared
			DWORD dwAttr = SFGAO_DISPLAYATTRMASK;
			pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlTemp, &dwAttr);

			if (dwAttr & SFGAO_SHARE)
			{
				lvItem.mask |= LVIF_STATE;
				lvItem.stateMask |= LVIS_OVERLAYMASK;
				lvItem.state |= INDEXTOOVERLAYMASK(1); //1 is the index for the shared overlay image
			}

			if (dwAttr & SFGAO_GHOSTED)
			{
				lvItem.mask |= LVIF_STATE;
				lvItem.stateMask |= LVIS_CUT;
				lvItem.state |= LVIS_CUT;
			}

			int iItem = InsertItem(&lvItem);
			if (iItem >= 0)
			{
				// Set columns:
				const int nColumns = m_wndHeader.GetItemCount();
				for (int iColumn = 0; iColumn < nColumns; iColumn++)
				{
					SetItemText(iItem, iColumn, OnGetItemText(iItem, iColumn, pItem));
				}
			}

			dwFetched = 0;
		}

		pEnum->Release();
	}

	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// CExplorerDropTarget
CExplorerDropTarget::CExplorerDropTarget(CHistoryShellList* pWnd) : m_pParent(pWnd)
{
	if (FAILED(CoCreateInstance(CLSID_DragDropHelper,
								NULL, 
								CLSCTX_INPROC_SERVER,
								IID_IDropTargetHelper, 
								(void**)&m_pHelper)))
	{
		m_pHelper = NULL;
	}
}

CExplorerDropTarget::~CExplorerDropTarget()
{
	if (m_pHelper != NULL)
		m_pHelper->Release();
}

DROPEFFECT CExplorerDropTarget::OnDragEnter(CWnd * pWnd, COleDataObject* pDataObject,
							DWORD dwKeyState, CPoint point )
{
	DROPEFFECT retval = DragCheck(pDataObject, dwKeyState, point);

	if (m_pHelper != NULL)
	{
		IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE);
		m_pHelper->DragEnter(pWnd->GetSafeHwnd(), piDataObj, &point, retval);
	}
	return retval;
}

DROPEFFECT CExplorerDropTarget::OnDragOver(CWnd *, COleDataObject* pDataObject,
						DWORD dwKeyState, CPoint point)
{
	DROPEFFECT retval = DragCheck(pDataObject, dwKeyState, point);

	if (m_pHelper != NULL)
		m_pHelper->DragOver(&point, retval);

	return retval;
}

BOOL CExplorerDropTarget::OnDrop(CWnd *, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	BOOL retval = FALSE;

	if (m_pHelper != NULL)
	{
		IDataObject* piDataObj = pDataObject->GetIDataObject(FALSE);
		m_pHelper->Drop(piDataObj, &point, dropEffect);
	}

	// Get destination folder and make sure there are 2 nul terminating bytes (required according to doco)
	CString dst = GetDst(point);
	ASSERT(!dst.IsEmpty());                     // target should be able to be dropped on
	size_t lenDst = dst.GetLength();
	LPTSTR pDst = dst.GetBuffer(lenDst + 2);    // reserve room for an extra nul byte
	pDst[lenDst+1] = '\0';

	// Get the source files and put into a list as expected by SHFILEOPSTRUCT
	std::vector<CString> src;
	size_t total_len = GetNames(pDataObject, src);
	TCHAR * buf = new TCHAR[total_len + 1];
	TCHAR * pp = buf;
	for (std::vector<CString>::const_iterator psrc = src.begin(); psrc != src.end(); ++psrc)
	{
		size_t len = psrc->GetLength();
		_tcscpy(pp, (LPCTSTR)(*psrc));
		pp += len;
		*pp++ = '\0';
	}
	ASSERT(pp == buf + total_len);
	*pp = '\0';

	// Start the copy/move
	SHFILEOPSTRUCT fos;
	fos.hwnd = m_pParent->m_hWnd;
	fos.wFunc = (dropEffect == DROPEFFECT_COPY) ? FO_COPY : FO_MOVE;
	fos.pFrom = buf;
	fos.pTo = dst;
	fos.fFlags = FOF_ALLOWUNDO;
	//fos.lpszProgressTitle = (dropEffect == DROPEFFECT_COPY) ? _T("Copying") : _T("Moving");
	retval = (::SHFileOperation(&fos) == 0);
	m_pParent->Refresh();

	delete buf;
	return retval;
}

void CExplorerDropTarget::OnDragLeave(CWnd *)
{
	if (m_pHelper != NULL)
		m_pHelper->DragLeave();
}

// Get destination - this is a sub-folder of the current folder if the user is dragging
// over an item in the list that is a folder, otherwise it is just the current folder itself.
// Returns the folder name or an empty string if the folder cannot be dropped on.
CString CExplorerDropTarget::GetDst(CPoint point)
{
	CString retval;                        // Destination folder for drop
	int iItem;
	LPAFX_SHELLITEMINFO pInfo;
	ULONG ulAttrs = SFGAO_FOLDER | SFGAO_DROPTARGET;  // We need to ask for flags we want (although sometimes GetAttributesOf returns others too - be careful)

	// Find current drag item and see if it is a folder
	if ((iItem = m_pParent->HitTest(point)) != -1 &&
		(pInfo = (LPAFX_SHELLITEMINFO)m_pParent->GetItemData(iItem)) != NULL &&
		pInfo->pParentFolder != NULL &&
		pInfo->pidlRel != NULL &&
		pInfo->pParentFolder->GetAttributesOf(1, (const struct _ITEMIDLIST **)&pInfo->pidlRel, &ulAttrs) == S_OK &&
		(ulAttrs & SFGAO_FOLDER) != 0)
	{
		// Check if the folder is a drop target
		// Note that this flag is wrongly set for some folders like "My Computer" but then SHGetPathFromIDList (below) fails
		if ((ulAttrs & SFGAO_DROPTARGET) != 0)
		{
			// Get the name of the folder to return it
			TCHAR szPath[MAX_PATH];
			::SHGetPathFromIDList(pInfo->pidlFQ, szPath);  // This may fail for some folders that should not be drop targets (luckilly szPath remains empty)
			retval = szPath;
		}
	}
	else
	{
		// Check if current folder is a drop target
		SHFILEINFO	sfi;
		sfi.dwAttributes = SFGAO_DROPTARGET;
		VERIFY(SHGetFileInfo((LPCTSTR)m_pParent->m_pidlCurFQ, 0, &sfi, sizeof(sfi),
							  SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED));
		if ((sfi.dwAttributes & SFGAO_DROPTARGET) != 0)
			m_pParent->GetCurrentFolder(retval);     // Return name of current folder (if drop target)
	}
	return retval;
}

// Get names of files/folders involved in the drag and drop operation.  This just
// involves decoding the DROPFILES "structure" obtained from CF_HDROP clipboard format.
size_t CExplorerDropTarget::GetNames(COleDataObject* pDataObject, std::vector<CString> & retval)
{
	size_t total_len = 0;
	HGLOBAL hg;
	DROPFILES * pdf;
	if ((hg = pDataObject->GetGlobalData(CF_HDROP)) != HGLOBAL(0) &&
		(pdf = (DROPFILES *)::GlobalLock(hg)) != NULL)
	{
		char *pp = (char *)pdf + pdf->pFiles;  // The string list starts here
		CString name;                          // Next name from the list
		for (;;)
		{
			if (pdf->fWide)                    // Unicode names
				name = CString((wchar_t *)pp);
			else
				name = CString(pp);

			size_t len = name.GetLength();
			if (len == 0)
				break;                  // Empty string signals end of list
			total_len += len + 1;
			pp += (len + 1) * (pdf->fWide ? 2 : 1);

			retval.push_back(name);
		}
		::GlobalUnlock(hg);
	}

	return total_len;
}

// Get destination folder + list of files/folders to move/copy. If any of them cannot
// be done (eg drag file onto itself or drag folder into descendant) then DROPEFFECT_NONE
// is returned.  If any cannot be moved without copying and deleting (ie source and
// dest are on different drives) then DROPEFFECT_COPY is returned, else DROPEFFECT_MOVE.
// However, if user holds Ctrl key down DROPEFFECT_COPY is forced (even on same drive), OR
// if they hold Shift key down then DROPEFFECT_MOVE is forced (even across drives).
DROPEFFECT CExplorerDropTarget::DragCheck (COleDataObject* pDataObject,
						DWORD dwKeyState, CPoint point )
{
	DROPEFFECT retval = DROPEFFECT_MOVE;

	// Get the destination folder of the move and the folders/files to move
	CString dst = GetDst(point);
	if (dst.IsEmpty())
		return DROPEFFECT_NONE;       // Current drag folder is not a drop target
	std::vector<CString> src;
	GetNames(pDataObject, src);

	for (std::vector<CString>::const_iterator psrc = src.begin(); psrc != src.end(); ++psrc)
	{
		DROPEFFECT de;
		if (::PathIsDirectory(*psrc))
			de = CanMoveFolder(*psrc, dst);
		else
			de = CanMove(*psrc, dst);

		if (de == DROPEFFECT_NONE)
		{
			retval = DROPEFFECT_NONE;      // if we can't drag any then return NONE
			break;
		}
		else if (de == DROPEFFECT_COPY)
			retval = DROPEFFECT_COPY;     // if we can't move any then return COPY
	}
	if (retval == DROPEFFECT_MOVE && (dwKeyState & MK_CONTROL) != 0)
		retval = DROPEFFECT_COPY;
	if (retval == DROPEFFECT_COPY && (dwKeyState & MK_SHIFT) != 0)
		retval = DROPEFFECT_MOVE;
	return retval;
}

// Check if move is allowed - if moving to same folder (ie no move at all) return DROPEFFECT_NONE,
// else return DROPEFFECT_MOVE or DROPEFFECT_COPY depending on whether moving across drives.
DROPEFFECT CExplorerDropTarget::CanMove(LPCTSTR file, LPCTSTR dst)
{
	TCHAR folder[MAX_PATH];
	_tcsncpy(folder, file, MAX_PATH);
	LPTSTR pfn = ::PathFindFileName(folder);
	pfn--;
	ASSERT(pfn > folder && pfn < folder + MAX_PATH);        // Make sure it points into the buffer
	ASSERT(*pfn == '\\');
	*pfn = '\0';                                            // Truncate at file name to get folder
	ASSERT(::PathIsDirectory(folder) && ::PathIsDirectory(dst));
	if (::StrCmpI(folder, dst) == 0)
		return DROPEFFECT_NONE;
	else if (::PathIsSameRoot(folder, dst))
		return DROPEFFECT_MOVE;
	else
		return DROPEFFECT_COPY;
}

// Check if move of a folder is allowed - checks that not moving to parent folder (ie,
// no move at all) and also that it is not moved inside itself (to itself or descendant).
DROPEFFECT CExplorerDropTarget::CanMoveFolder(LPCTSTR folder, LPCTSTR dst)
{
	DROPEFFECT retval = CanMove(folder, dst);
	if (retval != DROPEFFECT_NONE)
	{
		// Also make sure that we don't move a folder to itself or a descendant
		TCHAR common[MAX_PATH];
		size_t len = ::PathCommonPrefix(folder, dst, common);
		if (len == _tcslen(folder))
			retval = DROPEFFECT_NONE;
	}
	return retval;
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
	if (!CDialog::Create(MAKEINTRESOURCE(IDD), pParentWnd))
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
	//rct.InflateRect(0, 0, 3, 30);
	splitter_.Create(this, rct, IDC_EXPLORER);
	// Create the 2 pane windows and add them to the splitter
	tree_.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
						  CRect(0, 0, 0, 0), &splitter_, IDC_EXPLORER_TREE);
	splitter_.SetPane(0, &tree_);
	list_.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT,
						  CRect(0, 0, 0, 0), &splitter_, IDC_EXPLORER_LIST);
	splitter_.SetPane(1, &list_);

	// Set options for the tree and list
	//tree_.SetFlags(SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN);  // moved to update_types
	tree_.SetRelatedList(&list_);
	UnlinkToTree();     // This is so first list update does not slow startup for update of tree
	tree_.EnableShellContextMenu();
	list_.EnableShellContextMenu();

	// The rest of this was moved from HandleInitDialog
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

	show_hidden = theApp.GetProfileInt("File-Settings", "ExplorerShowHidden", 1);
	update_types();

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
	if ((GetStyle() & WS_VISIBLE) != 0)
	{
		CString dir = theApp.GetProfileString("File-Settings", "ExplorerDir", "C:\\");
		ctl_name_.SetWindowText(dir);
		// Disable this since it can take a while
		//tree_.SelectPath(dir);
		list_.DisplayFolder((const char *)dir);
		list_.Start(this);
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

	//VERIFY(arrow_icon_ = AfxGetApp()->LoadIcon(IDI_ARROW));
	//ctl_filter_opts_.SetIcon(arrow_icon_);
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

	// Create the resizer that move the controls when dialog is resized
	m_resizer.Create(GetSafeHwnd(), TRUE, 100, TRUE);
	GetWindowRect(&rct);
	m_resizer.SetInitialSize(rct.Size());
	m_resizer.SetMinimumTrackingSize(rct.Size());

	m_resizer.Add(IDC_FOLDER_FILTER,    0, 0, 100,   0);  // move right edge
	m_resizer.Add(IDC_FOLDER_REFRESH, 100, 0,   0,   0);  // stick to right side
	m_resizer.Add(IDC_FILTER_OPTS,    100, 0,   0,   0);
	m_resizer.Add(IDC_FOLDER_VIEW,    100, 0,   0,   0);
	m_resizer.Add(IDC_FOLDER_NAME,      0, 0, 100,   0);
	m_resizer.Add(IDC_EXPLORER,         0, 0, 100, 100);  // move right & bottom edges

	init_ = true;
	return TRUE;
}

void CExplorerWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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

BEGIN_MESSAGE_MAP(CExplorerWnd, CDialog)
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
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CExplorerWnd::OnDestroy()
{
	theApp.WriteProfileInt("File-Settings", "ExplorerOrientation", splitter_.Orientation());
	theApp.WriteProfileInt("File-Settings", "ExplorerTreeSize", splitter_.GetPaneSize(0));
	theApp.WriteProfileInt("File-Settings", "ExplorerListSize", splitter_.GetPaneSize(1));

	list_.SaveLayout();         // save columns etc for list

	theApp.WriteProfileInt("File-Settings", "ExplorerShowHidden", show_hidden);

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

	CDialog::OnDestroy();
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

CBrush * CExplorerWnd::m_pBrush = NULL;

BOOL CExplorerWnd::OnEraseBkgnd(CDC *pDC)
{
	// We check for changed look in erase background event as it's done
	// before other drawing.  This is necessary (to update m_pBrush etc)
	// because there is no message sent when the look changes.
	static UINT saved_look = 0;
	if (theApp.m_nAppLook != saved_look)
	{
		// Create new brush used for background of static controls
		if (m_pBrush != NULL)
			delete m_pBrush;
		m_pBrush = new CBrush(afxGlobalData.clrBarFace);

		saved_look = theApp.m_nAppLook;
	}

	CRect rct;
	GetClientRect(&rct);

	// Fill the background with a colour that matches the current BCG theme (and hence sometimes with the Windows Theme)
	pDC->FillSolidRect(rct, afxGlobalData.clrBarFace);
	return TRUE;
}

HBRUSH CExplorerWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);                            // Make sure text has no background
		if (m_pBrush != NULL)
			return (HBRUSH)*m_pBrush;
	}

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
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

	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_HIDDEN_HIDE, show_hidden == 0 ? MF_CHECKED : MF_UNCHECKED);
	m_menu_.GetSubMenu(0)->CheckMenuItem(ID_HIDDEN_SHOW, show_hidden == 1 ? MF_CHECKED : MF_UNCHECKED);
	if (!theApp.is_win7_)
		m_menu_.GetSubMenu(0)->DeleteMenu(ID_SHOW_ALL, MF_BYCOMMAND);
	else
		m_menu_.GetSubMenu(0)->CheckMenuItem(ID_SHOW_ALL, show_hidden == 2 ? MF_CHECKED : MF_UNCHECKED);

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
		show_hidden = 0;
		update_types();
		break;
	case ID_HIDDEN_SHOW:
		show_hidden = 1;
		update_types();
		break;
	case ID_SHOW_ALL:
		show_hidden = 2;
		update_types();
		break;

	default:
		ASSERT(0);
		return;
	}
}

// This function changes the types of objects display in the tree and the list
// both: always show folders
// list: also shows non-folders (ie files)
// show_hidden says whether hidden and system files are shown
void CExplorerWnd::update_types()
{
	SHCONTF flags = SHCONTF_FOLDERS;

	if (show_hidden > 0)
		flags |= SHCONTF_INCLUDEHIDDEN;

#if _MFC_VER >= 0x0A00
	// Note that the SHCONTF_INCLUDESUPERHIDDEN option has no effect if the Windows Explorer option
	// "Hide protected (operating system) files (Recommended)" is off since HIDDEN files are already shown
	if (theApp.is_win7_ && show_hidden > 1)
		flags |= SHCONTF_INCLUDESUPERHIDDEN;  
#endif

	tree_.SetFlags(flags);
	list_.SetItemTypes(flags | SHCONTF_NONFOLDERS);
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
