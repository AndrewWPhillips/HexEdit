// RecentFileDlg.cpp : implements CRecentFileDlg class for the Recent File List Dialog
//

// Copyright (c) 2003 by Andrew W. Phillips.
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
#include <MultiMon.h>
#include <set>
#include <io.h>       // for _access()

#include "HexEdit.h"
#include "HexEditDoc.h"
#include "HexFileList.h"
#include "RecentFileDlg.h"
#include "SpecialList.h"
#ifdef USE_HTML_HELP
#include <HtmlHelp.h>
#endif

#include "resource.hm"
#include "HelpID.hm"            // For help IDs for the pages

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// We need access to the grid since the callback sorting function does not have
// access to it and we need to know which column we are sorting on (GetSortColumn).
static CGridCtrl *p_grid;

// The sorting callback function can't be a member function
static int CALLBACK rfl_compare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	(void)lParamSort;

    int retval;
	CGridCellBase* pCell1 = (CGridCellBase*) lParam1;
	CGridCellBase* pCell2 = (CGridCellBase*) lParam2;
	if (!pCell1 || !pCell2) return 0;

	switch (p_grid->GetSortColumn() - p_grid->GetFixedColumnCount())
	{
    case CRecentFileDlg::COL_NAME:
		// Do text compare (ignore case)
#if _MSC_VER >= 1300
		return _wcsicmp(pCell1->GetText(), pCell2->GetText());
#else
		return _stricmp(pCell1->GetText(), pCell2->GetText());
#endif

    case CRecentFileDlg::COL_TYPE:
    case CRecentFileDlg::COL_LOCN:
    case CRecentFileDlg::COL_ATTR:
#ifdef PROP_INFO
    case CRecentFileDlg::COL_CATEGORY:
    case CRecentFileDlg::COL_KEYWORDS:
    case CRecentFileDlg::COL_COMMENTS:
#endif
        // Do text compare. If the same then sort on file name
#if _MSC_VER >= 1300
        retval = _wcsicmp(pCell1->GetText(), pCell2->GetText());
        if (retval != 0)
            return retval;
        // Now compare based on file name
        pCell1 = (CGridCellBase*)pCell1->GetData();
        pCell2 = (CGridCellBase*)pCell2->GetData();
        return _wcsicmp(pCell1->GetText(), pCell2->GetText());
#else
        retval = _stricmp(pCell1->GetText(), pCell2->GetText());
        if (retval != 0)
            return retval;
        // Now compare based on file name
        pCell1 = (CGridCellBase*)pCell1->GetData();
        pCell2 = (CGridCellBase*)pCell2->GetData();
        return _stricmp(pCell1->GetText(), pCell2->GetText());
#endif
    case CRecentFileDlg::COL_USED:
    case CRecentFileDlg::COL_MODIFIED:
    case CRecentFileDlg::COL_CREATED:
    case CRecentFileDlg::COL_ACCESSED:
		// Do date compare
		if (pCell1->GetData() < pCell2->GetData())
			return -1;
		else if (pCell1->GetData() == pCell2->GetData())
			return 0;
		else
			return 1;

    case CRecentFileDlg::COL_SIZE:
		if (pCell1->GetData() < pCell2->GetData())
			return -1;
		else if (pCell1->GetData() == pCell2->GetData())
			return 0;
		else
			return 1;

	default:
		ASSERT(0);
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRecentFileDlg dialog


CRecentFileDlg::CRecentFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecentFileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRecentFileDlg)
	net_retain_ = FALSE;
	//}}AFX_DATA_INIT
	net_retain_ = TRUE;
	p_grid = &grid_;
}

void CRecentFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRecentFileDlg)
	DDX_Control(pDX, IDC_REMOVE_FILES, ctl_remove_);
	DDX_Control(pDX, IDC_OPEN_RO, ctl_open_ro_);
	DDX_Control(pDX, IDC_OPEN_FILES, ctl_open_);
	DDX_Check(pDX, IDC_NET_RETAIN, net_retain_);
	//}}AFX_DATA_MAP
    DDX_GridControl(pDX, IDC_GRID_RFL, grid_);
}


BEGIN_MESSAGE_MAP(CRecentFileDlg, CDialog)
	//{{AFX_MSG_MAP(CRecentFileDlg)
	ON_BN_CLICKED(IDC_VALIDATE, OnValidate)
	ON_BN_CLICKED(IDC_OPEN_FILES, OnOpen)
	ON_BN_CLICKED(IDC_OPEN_RO, OnOpenRO)
	ON_BN_CLICKED(IDC_REMOVE_FILES, OnRemove)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_RECENT_FILES_HELP, OnHelp)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
    ON_NOTIFY(NM_CLICK, IDC_GRID_RFL, OnGridClick)
    ON_NOTIFY(NM_DBLCLK, IDC_GRID_RFL, OnGridDoubleClick)
    ON_NOTIFY(NM_RCLICK, IDC_GRID_RFL, OnGridRClick)
END_MESSAGE_MAP()
    
void CRecentFileDlg::InitColumnHeadings()
{
    static char *heading[] =
    {
        "File Name",
        "Location",
        "Last Opened",
        "Size",
        "Type",
        "Modified",
        "Attributes",
        "Created",
        "Accessed",
#ifdef PROP_INFO
		"Category",
		"Keywords",
		"Comments",
#endif
        NULL
    };

    ASSERT(sizeof(heading)/sizeof(*heading) == COL_LAST + 1);

    CString strWidths = theApp.GetProfileString("File-Settings", "RecentFileDialogColumns", "145,145,144");
    int curr_col = grid_.GetFixedColumnCount();
    bool all_hidden = true;

    for (int ii = 0; ii < COL_LAST; ++ii)
    {
        CString ss;

        AfxExtractSubString(ss, strWidths, ii, ',');
        int width = atoi(ss);
        if (width > 0) all_hidden = false;                  // we found a visible column

        ASSERT(heading[ii] != NULL);
        grid_.SetColumnCount(curr_col + 1);
        if (width == 0)
            grid_.SetColumnWidth(curr_col, 0);              // make column hidden
        else
            grid_.SetUserColumnWidth(curr_col, width);      // set user specified size (or -1 to indicate fit to cells)

        // Set column heading text (centred).  Also set item data so we know what goes in this column
        GV_ITEM item;
        item.row = 0;                                       // top row is header
        item.col = curr_col;                                // column we are changing
        item.mask = GVIF_PARAM|GVIF_FORMAT|GVIF_TEXT;       // change data+centered+text
        item.lParam = ii;                                   // data that says what's in this column
        item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE;  // centre the heading
        item.strText = heading[ii];                         // text of the heading
        grid_.SetItem(&item);

        ++curr_col;
    }

    // Show at least one column
    if (all_hidden)
    {
        grid_.SetColumnWidth(grid_.GetFixedColumnCount(), 1);
        grid_.AutoSizeColumn(grid_.GetFixedColumnCount(), GVS_BOTH);
    }
}

void CRecentFileDlg::FillGrid()
{
    // Load all the recent files into the grid control
    CHexFileList *pfl = theApp.GetFileList();
    ASSERT(pfl->name_.size() == pfl->opened_.size());

    int last_row = grid_.GetFixedRowCount() + pfl->name_.size();
    grid_.SetRowCount(last_row);

    GV_ITEM item;
    item.mask = GVIF_STATE|GVIF_FORMAT|GVIF_TEXT|GVIF_PARAM;
    item.nState = GVIS_READONLY;

    // Check if we can get WIN32 file attribute (GetFileAttributesEx) - not for Win95
    BOOL has_wfad = FALSE;
    HINSTANCE hh;
    typedef BOOL (__stdcall *PFGetFileAttributesEx)(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
    PFGetFileAttributesEx pproc = 0;
    if ((hh = ::LoadLibrary("KERNEL32.DLL")) != HINSTANCE(0) &&
        (pproc = (PFGetFileAttributesEx)::GetProcAddress(hh, "GetFileAttributesExA")) != 0)
    {
        has_wfad = TRUE;
    }

#if defined(BG_DEVICE_SEARCH)
	CSpecialList *psl = theApp.GetSpecialList();
	int device_idx = -1;
#endif

    int index;
    for (index = pfl->name_.size()-1, item.row = grid_.GetFixedRowCount(); item.row < last_row; index--, ++item.row)
    {
        // Get info about the file
        SHFILEINFO sfi;                 // Used with SHGetFileInfo to get file type string
        struct tm *timeptr;             // Used in displaying dates
        char disp[128];
		bool is_device = ::IsDevice(pfl->name_[index]) == TRUE;
		int ext_pos = -1;               // Offset of extension in path name (-1 if none)
		int path_len = -1;              // Length of path (full name without filename)
		CFileStatus fs;                 // Used to get file info (size etc)
		WIN32_FILE_ATTRIBUTE_DATA wfad; // Used to get better file info (NT4+, Win98+)
		BOOL got_status = FALSE;

		if (is_device)
		{
#if defined(BG_DEVICE_SEARCH)
			device_idx = psl->find(pfl->name_[index]);
#endif
		}
		else
		{
			got_status = CFile64::GetStatus(pfl->name_[index], fs);

			if (has_wfad)
			{
				ASSERT(pproc != 0);
				got_status = (*pproc)(pfl->name_[index], GetFileExInfoStandard,  &wfad);
			}

			// Split filename into name and path
			path_len = pfl->name_[index].ReverseFind('\\');
			if (path_len == -1) path_len = pfl->name_[index].ReverseFind('/');
			if (path_len == -1) path_len = pfl->name_[index].ReverseFind(':');
			if (path_len == -1)
				path_len = 0;
			else
				++path_len;

			ext_pos = pfl->name_[index].ReverseFind('.');
			if (ext_pos < path_len) ext_pos = -1;              // '.' in a sub-directory name
		}

        for (int column = 0; column < COL_LAST; ++column)
        {
            item.col = column + grid_.GetFixedColumnCount();

            switch (column)
            {
            case COL_NAME:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				if (is_device)
				{
#if defined(BG_DEVICE_SEARCH)
					if (device_idx == -1)
						item.strText = CString(" Unknown device");
					else
						item.strText = CString(" ") + psl->name(device_idx);
#else
					item.strText = CString(" ") + ::DeviceName(pfl->name_[index]);
#endif
				}
				else
                    item.strText = pfl->name_[index].Mid(path_len);
				item.lParam = index;       // Store index back into file list so we know where it came from after sorting
                break;
            case COL_LOCN:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				if (is_device)
					item.strText = pfl->name_[index];
				else
                    item.strText = pfl->name_[index].Left(path_len);
                // Store ptr to file name cell so we can sort filename within the same directory
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
                break;
            case COL_USED:
			    item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                if ((timeptr = localtime(&pfl->opened_[index])) == NULL)
                    item.strText = "Invalid";
                else
                {
                    strftime(disp, sizeof(disp), "%c", timeptr);
                    item.strText = disp;
                }
				item.lParam = pfl->opened_[index];
                break;
#ifdef PROP_INFO
			case CRecentFileDlg::COL_CATEGORY:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				item.strText = pfl->GetData(index, CHexFileList::CATEGORY);
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
				break;
			case CRecentFileDlg::COL_KEYWORDS:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				item.strText = pfl->GetData(index, CHexFileList::KEYWORDS);
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
				break;
			case CRecentFileDlg::COL_COMMENTS:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				item.strText = pfl->GetData(index, CHexFileList::COMMENTS);
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
				break;
#endif
            case COL_SIZE:
			    item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
				if (is_device)
				{
#if defined(BG_DEVICE_SEARCH)
					if (device_idx == -1)
						item.lParam = 0;
					else
						item.lParam = psl->total_size(device_idx);
#else
					item.lParam = ::DeviceSize(pfl->name_[index]);
#endif
					if (item.lParam == -1)
						item.strText.Empty();
					else
						item.strText = NumScale(double(item.lParam)) + "bytes";
				}
				else if (got_status)
				{
				    item.lParam = fs.m_size;
                    item.strText = NumScale(double(fs.m_size)) + "bytes";
				}
                else
				{
				    item.lParam = -1;
                    item.strText.Empty();
				}
                break;
            case COL_TYPE:
			    item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                if (ext_pos == -1)
                {
                    item.strText = "None";
                }
                else if (SHGetFileInfo(pfl->name_[index], 0, &sfi, sizeof(sfi), SHGFI_TYPENAME) &&
                    sfi.szTypeName[0] != '\0')
                {
                    item.strText = sfi.szTypeName;
                }
                else
                {
                    item.strText = pfl->name_[index].Mid(ext_pos+1) + " file";
                }
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
                break;
            case COL_MODIFIED:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                if (got_status && has_wfad)
                {
                    CTime tt(wfad.ftLastWriteTime);
                    item.strText = tt.Format("%c");
                }
                else if (got_status)
                    item.strText = fs.m_mtime.Format("%c");
                else
                    item.strText.Empty();

				if (is_device)
					item.lParam = -1;
				else
				    item.lParam = fs.m_mtime.GetTime();
                break;
            case COL_ATTR:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                item.strText.Empty();
                if (got_status && has_wfad)
                {
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                        item.strText += 'R';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                        item.strText += 'H';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
                        item.strText += 'S';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                        item.strText += 'A';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                        item.strText += 'C';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)  // FILE_ATTRIBUTE_ENCRYPTED 0x00004000 
                        item.strText += 'E';
                    if (wfad.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
                        item.strText += 'T';
                }
                else if (got_status)
                {
                    if ((fs.m_attribute & CFile::readOnly) != 0)
                        item.strText += 'R';
                    if ((fs.m_attribute & CFile::hidden) != 0)
                        item.strText += 'H';
                    if ((fs.m_attribute & CFile::system) != 0)
                        item.strText += 'S';
                    if ((fs.m_attribute & CFile::archive) != 0)
                        item.strText += 'A';
                }
    			item.lParam = (LONGLONG)grid_.GetCell(item.row, grid_.GetFixedColumnCount() + COL_NAME);
                break;
            case COL_CREATED:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                if (got_status && has_wfad)
                {
                    CTime tt(wfad.ftCreationTime);
                    item.strText = tt.Format("%c");
                }
                else if (got_status)
                    item.strText = fs.m_ctime.Format("%c");
                else
                    item.strText.Empty();

				if (is_device)
					item.lParam = -1;
				else
					item.lParam = fs.m_ctime.GetTime();
                break;
            case COL_ACCESSED:
			    item.nFormat = DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
                if (got_status && has_wfad)
                {
                    CTime tt(wfad.ftLastAccessTime);
                    item.strText = tt.Format("%c");
                }
                else if (got_status)
                    item.strText = fs.m_atime.Format("%c");
                else
                    item.strText.Empty();

				if (is_device)
					item.lParam = -1;
				else
					item.lParam = fs.m_atime.GetTime();
                break;
            }
            grid_.SetItem(&item);
        }

        // Set the data of column zero to be the index of the file.  This is so that if the order
		// of rows is changed (eg when sorted) that we still know which file in the HexFileList it is.
//        grid_.SetItemData(item.row, fcc, index);
    }
    if (hh != HINSTANCE(0))
        ::FreeLibrary(hh);
    grid_.Refresh();
}

// Delete all files in the to_delete_ list when dialog is closing
void CRecentFileDlg::DeleteEntries() 
{
    CHexFileList *pfl = theApp.GetFileList();
//	range_set<int>::const_reverse_iterator pp;
	range_set<int>::const_iterator pp;

	// Remove elements starting at end so that we don't change the index of a file yet to be done
	for (pp = to_delete_.end(); pp != to_delete_.begin(); )
		pfl->Remove(pfl->name_.size() - *(--pp) - 1);

    to_delete_.clear();
}

/////////////////////////////////////////////////////////////////////////////
// CRecentFileDlg message handlers

BOOL CRecentFileDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    // Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(TRUE);
    resizer_.Add(IDC_GRID_RFL, 0, 0, 100, 100);
    resizer_.Add(IDOK, 100, 0, 0, 0);
    resizer_.Add(IDCANCEL, 100, 0, 0, 0);
    resizer_.Add(IDC_OPEN_FILES, 100, 0, 0, 0);
    resizer_.Add(IDC_OPEN_RO, 100, 0, 0, 0);
    resizer_.Add(IDC_REMOVE_FILES, 100, 0, 0, 0);
    resizer_.Add(IDC_FILES_SELECTED, 100, 0, 0, 0);
    resizer_.Add(IDC_VALIDATE, 100, 0, 0, 0);
    resizer_.Add(IDC_NET_RETAIN, 100, 0, 0, 0);
    resizer_.Add(IDC_NET_RETAIN_DESC, 100, 0, 0, 0);
    resizer_.Add(IDC_RECENT_FILES_HELP, 100, 0, 0, 0);

    int posx = theApp.GetProfileInt("Window-Settings", "RecentFileDlgX", -30000);
    int posy = theApp.GetProfileInt("Window-Settings", "RecentFileDlgY", -30000);
    int width = theApp.GetProfileInt("Window-Settings", "RecentFileDlgWidth", -30000);
    int height = theApp.GetProfileInt("Window-Settings", "RecentFileDlgHeight", -30000);

    if (posx != -30000)
    {
        CRect rr;               // Rectangle where we will put the dialog
        GetWindowRect(&rr);

        // Move to where it was when it was last closed
//        rr.OffsetRect(posx - rr.left, posy - rr.top);
		rr.left = posx;
		rr.top = posy;
		rr.right = rr.left + width;
		rr.bottom = rr.top + height;

        CRect scr_rect;         // Rectangle that we want to make sure the window is within

        // Get the rectangle that contains the screen work area (excluding system bars etc)
        if (theApp.mult_monitor_)
        {
            HMONITOR hh = MonitorFromRect(&rr, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            if (hh != 0 && GetMonitorInfo(hh, &mi))
                scr_rect = mi.rcWork;  // work area of nearest monitor
            else
            {
                // Shouldn't happen but if it does use the whole virtual screen
                ASSERT(0);
                scr_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
            }
        }
        else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &scr_rect, 0))
        {
            // I don't know if this will ever happen since the Windows documentation
            // is pathetic and does not say when or why SystemParametersInfo might fail.
            scr_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
                                   ::GetSystemMetrics(SM_CYFULLSCREEN));
        }

        if (rr.left > scr_rect.right - 20)              // off right edge?
            rr.OffsetRect(scr_rect.right - (rr.left+rr.right)/2, 0);
        if (rr.right < scr_rect.left + 20)              // off left edge?
            rr.OffsetRect(scr_rect.left - (rr.left+rr.right)/2, 0);
        if (rr.top > scr_rect.bottom - 20)              // off bottom?
            rr.OffsetRect(0, scr_rect.bottom - (rr.top+rr.bottom)/2);
        // This is not analogous to the prev. 3 since we don't want the window
        // off the top at all, otherwise you can get to the drag bar to move it.
        if (rr.top < scr_rect.top)                      // off top at all?
            rr.OffsetRect(0, scr_rect.top - rr.top);

        MoveWindow(&rr);
    }

    // Set up the grid control
    grid_.SetDoubleBuffering();
    grid_.SetAutoFit();
	grid_.SetCompareFunction(&rfl_compare);
//    grid_.SetEditable(TRUE);
    grid_.SetGridLines(GVL_BOTH); // GVL_HORZ | GVL_VERT
    grid_.SetTrackFocusCell(FALSE);
    grid_.SetFrameFocusCell(FALSE);
    grid_.SetListMode(TRUE);
    grid_.SetSingleRowSelection(FALSE);
    grid_.SetHeaderSort(TRUE);

    grid_.SetFixedRowCount(1);

    InitColumnHeadings();
    grid_.SetColumnResize();

    grid_.EnableRowHide(FALSE);
    grid_.EnableColumnHide(FALSE);
    grid_.EnableHiddenRowUnhide(FALSE);
    grid_.EnableHiddenColUnhide(FALSE);
	grid_.SetDoubleBuffering(TRUE);

    FillGrid();

    grid_.ExpandColsNice(FALSE);

	return TRUE;
}

void CRecentFileDlg::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_RECENT_FILES_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_RECENT_FILES_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = {
    IDC_GRID_RFL, HIDC_GRID_RFL,
    IDC_OPEN_FILES, HIDC_OPEN_FILES,
    IDC_OPEN_RO, HIDC_OPEN_RO,
    IDC_REMOVE_FILES, HIDC_REMOVE_FILES,
    IDC_FILES_SELECTED, HIDC_FILES_SELECTED,
    IDC_VALIDATE, HIDC_VALIDATE,
    IDC_NET_RETAIN, HIDC_NET_RETAIN,
    IDC_NET_RETAIN_DESC, HIDC_NET_RETAIN,
    0,0 
}; 

BOOL CRecentFileDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
                   HELP_WM_HELP, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CRecentFileDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    // Don't show context menu if right-click on grid top row (used to display column menu)
    if (pWnd->IsKindOf(RUNTIME_CLASS(CGridCtrl)))
    {
        grid_.ScreenToClient(&point);
        CCellID cell = grid_.GetCellFromPt(point);
        if (cell.row == 0)
            return;
    }

    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

LRESULT CRecentFileDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
	ASSERT(GetDlgItem(IDC_OPEN_FILES) != NULL);
	ASSERT(GetDlgItem(IDC_OPEN_RO) != NULL);
	ASSERT(GetDlgItem(IDC_REMOVE_FILES) != NULL);
    CCellRange sel = grid_.GetSelectedCellRange();
	GetDlgItem(IDC_OPEN_FILES)->EnableWindow(sel.IsValid());
	GetDlgItem(IDC_OPEN_RO)->EnableWindow(sel.IsValid());
	GetDlgItem(IDC_REMOVE_FILES)->EnableWindow(sel.IsValid());

    // Count the number of rows selected
    int fcc = grid_.GetFixedColumnCount();
    int rows_selected = 0;
	for (int row = sel.GetMinRow(); row <= sel.GetMaxRow(); ++row)
		if (grid_.IsCellSelected(row, fcc))
            ++rows_selected;
    CString ss;
    ss.Format("%d", rows_selected);
    AddCommas(ss);
	ASSERT(GetDlgItem(IDC_FILES_SELECTED) != NULL);
    GetDlgItem(IDC_FILES_SELECTED)->SetWindowText("Selected: " + ss);

    return FALSE;
}

void CRecentFileDlg::OnOK() 
{
	CDialog::OnOK();

	DeleteEntries();
}

void CRecentFileDlg::OnOpen() 
{
    CHexFileList *pfl = theApp.GetFileList();
    int fcc = grid_.GetFixedColumnCount();
	std::set<CString> to_open;
    CCellRange sel = grid_.GetSelectedCellRange();
	if (!sel.IsValid())
        return;

	// Work out all the files to be opened.  Note that we do it in 2 stages (get list of files
    // then open them) as opening a file changes the order of files in the list (pfl->name_ etc)
	for (int row = sel.GetMinRow(); row <= sel.GetMaxRow(); ++row)
		if (grid_.IsCellSelected(row, fcc))
			to_open.insert(pfl->name_[grid_.GetItemData(row, fcc)]);

    // Force dialog to close before we open the files in case any dialogs to do with opening pop up
	CDialog::OnOK();

	// Now open the files
	std::set<CString>::const_iterator pp;

	for (pp = to_open.begin(); pp != to_open.end(); ++pp)
	{
	    ASSERT(theApp.open_current_readonly_ == -1);
        if (theApp.OpenDocumentFile(*pp) != NULL)
            theApp.SaveToMacro(km_open, *pp);
	}

	DeleteEntries();
}

void CRecentFileDlg::OnOpenRO() 
{
    CHexFileList *pfl = theApp.GetFileList();
    int fcc = grid_.GetFixedColumnCount();
	std::set<CString> to_open;
    CCellRange sel = grid_.GetSelectedCellRange();
	ASSERT(sel.IsValid());

	// Work out all the files to be opened.  Note that we do it in 2 stages (get list of files
    // then open them) as opening a file changes the order of files in the list (pfl->name_ etc)
	for (int row = sel.GetMinRow(); row <= sel.GetMaxRow(); ++row)
		if (grid_.IsCellSelected(row, fcc))
			to_open.insert(pfl->name_[grid_.GetItemData(row, fcc)]);

		std::set<CString>::const_iterator pp;

	// Now open the files
    CHexEditDoc *pdoc;
	for (pp = to_open.begin(); pp != to_open.end(); ++pp)
    {
	    ASSERT(theApp.open_current_readonly_ == -1);
        theApp.open_current_readonly_ = TRUE;  // This must be done for each file to ensure open_file opens read_only
        if ((pdoc = (CHexEditDoc*)theApp.OpenDocumentFile(*pp)) != NULL)
            theApp.SaveToMacro(km_open, *pp);
    }

	CDialog::OnOK();

	DeleteEntries();
}

void CRecentFileDlg::OnRemove() 
{
    int fcc = grid_.GetFixedColumnCount();
	range_set<int> tt;                  // the grid rows to be removed
    CCellRange sel = grid_.GetSelectedCellRange();
	ASSERT(sel.IsValid());

	// Mark all selected rows for deletion
	for (int row = sel.GetMinRow(); row <= sel.GetMaxRow(); ++row)
		if (grid_.IsCellSelected(row, fcc))
		{
			to_delete_.insert(grid_.GetItemData(row, fcc));
			tt.insert(row);
		}

	// Now remove the rows from the display starting from the end
	range_set<int>::const_iterator pp;

	// Remove elements starting at end so that we don't muck up the row order
	for (pp = tt.end(); pp != tt.begin(); )
        grid_.DeleteRow(*(--pp));
    grid_.Refresh();
}

void CRecentFileDlg::OnValidate() 
{
    UpdateData();            // Make sure we know the current state of the keep network files check box

	CWaitCursor wc;
    CHexFileList *pfl = theApp.GetFileList();
    int fcc = grid_.GetFixedColumnCount();
	range_set<int> tt;                  // the grid rows to be removed
    int count = 0;                      // Number of files removed (validation failed)
    CString last_name;                  // Name of last file removed

	for (int row = grid_.GetFixedRowCount(); row < grid_.GetRowCount(); ++row)
	{
		int index = grid_.GetItemData(row, fcc);
		ASSERT(index > -1 && index < pfl->name_.size());
		if (::IsDevice(pfl->name_[index]))
			continue;                                   // Just ignore devices for now

		ASSERT(pfl->name_[index].Mid(1, 2) == ":\\");   // GetDriveType expects "D:\" under win9x
		if (_access(pfl->name_[index], 0) == -1 &&
			(!net_retain_ || ::GetDriveType(pfl->name_[index].Left(3)) == DRIVE_FIXED))
		{
            ++count;
            TRACE1("Validate: removing %s\n", pfl->name_[index]);
            last_name = pfl->name_[index];
			to_delete_.insert(index);
			tt.insert(row);
		}
	}

	// Now remove the rows from the display startinf from the end
	range_set<int>::const_iterator pp;

	// Remove elements starting at end so that we don't muck up the row order
	for (pp = tt.end(); pp != tt.begin(); )
        grid_.DeleteRow(*(--pp));
    grid_.Refresh();

    // Teel the user what we did
    CString ss;
    if (count == 0)
        ss = "No files removed from the\r"
             "recently used file list.";
    else if (count == 1)
        ss.Format("One file removed:\r%s", last_name);
    else
        ss.Format("%ld files removed from the\r"
                  "recently used file list.",  long(count));
    AfxMessageBox(ss);
}

void CRecentFileDlg::OnDestroy() 
{
    if (grid_.m_hWnd != 0)
    {
        // Save column widths
        CString strWidths;

        for (int ii = grid_.GetFixedColumnCount(); ii < grid_.GetColumnCount(); ++ii)
        {
            CString ss;
            ss.Format("%ld,", long(grid_.GetUserColumnWidth(ii)));
            strWidths += ss;
        }

        theApp.WriteProfileString("File-Settings", "RecentFileDialogColumns", strWidths);
    }

	// Save window position so t can be restored when dialog is reopened
    CRect rr;
    GetWindowRect(&rr);
    theApp.WriteProfileInt("Window-Settings", "RecentFileDlgX", rr.left);
    theApp.WriteProfileInt("Window-Settings", "RecentFileDlgY", rr.top);
    theApp.WriteProfileInt("Window-Settings", "RecentFileDlgWidth", rr.right - rr.left);
    theApp.WriteProfileInt("Window-Settings", "RecentFileDlgHeight", rr.bottom - rr.top);

	CDialog::OnDestroy();
}

// Handlers for messages from grid control (IDC_GRID_RFL)
void CRecentFileDlg::OnGridClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    TRACE("Left button click on row %d, col %d\n", pItem->iRow, pItem->iColumn);

    if (pItem->iRow < grid_.GetFixedRowCount())
        return;                         // Don't do anything for header rows
}

void CRecentFileDlg::OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    TRACE("Left button click on row %d, col %d\n", pItem->iRow, pItem->iColumn);

    if (pItem->iRow < grid_.GetFixedRowCount())
        return;                         // Don't do anything for header rows

    OnOpen();
}

void CRecentFileDlg::OnGridRClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
    NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
    TRACE("Right button click on row %d, col %d\n", pItem->iRow, pItem->iColumn);
    int fcc = grid_.GetFixedColumnCount();

    if (pItem->iRow < grid_.GetFixedRowCount() && pItem->iColumn >= fcc)
    {
        // Right click on column headings - create menu of columns available
        CMenu mm;
        mm.CreatePopupMenu();

		int ii = 0;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "File Name");   ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Location");    ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Last Opened"); ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Size");        ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Type");        ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Modified");    ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Attributes");  ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Created");     ++ii;
        mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Accessed");    ++ii;
#ifdef PROP_INFO
		mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Category");    ++ii;
		mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Keywords");    ++ii;
		mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(fcc+ii)>0?MF_CHECKED:0), ii+1, "Comments");    ++ii;
#endif
		ASSERT(ii == COL_LAST);

        // Work out where to display the popup menu
        CRect rct;
        grid_.GetCellRect(pItem->iRow, pItem->iColumn, &rct);
        grid_.ClientToScreen(&rct);
        int item = mm.TrackPopupMenu(
                TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                (rct.left+rct.right)/2, (rct.top+rct.bottom)/2, this);
        if (item != 0)
        {
            item += fcc-1;
            if (grid_.GetColumnWidth(item) > 0)
                grid_.SetColumnWidth(item, 0);
            else
            {
                grid_.SetColumnWidth(item, 1);
                grid_.AutoSizeColumn(item, GVS_BOTH);
            }
            grid_.ExpandColsNice(FALSE);
        }
    }
}
