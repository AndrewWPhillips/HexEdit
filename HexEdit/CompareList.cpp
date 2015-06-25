// CompareList.cpp - implements the calculator history (tape)
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"

#include "HexEdit.h"
#include "MainFrm.h"
#include "CompareList.h"
#include <HtmlHelp.h>
#include "resource.hm"      // Help IDs
#include "HelpID.hm"        // For dlg help ID

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Grid column headings (array indices need to match the column enum)
static char *headingLong[] =
{
	"Original Type",
	"Orig Addr hex",
	"Orig Addr dec",
	"Length - hex",
	"Length - dec",
	"Comp Addr hex",
	"Comp Addr dec",
	"Compare  Type",
	NULL
};

static char *heading[] =
{
	"Orig Type",
	"Addr . hex",
	"Addr . dec",
	"Len - hex",
	"Len - dec",
	"Addr . hex",
	"Addr . dec",
	"Comp Type",
	NULL
};

static char *headingShort[] =
{
	"Type",
	"Addr",
	"Addr",
	"Len",
	"Len",
	"Addr",
	"Addr",
	"Type",
	NULL
};

static char *headingTiny[] =
{
	"T",
	"A",
	"A",
	"L",
	"L",
	"A",
	"A",
	"T",
	NULL
};

/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CGridCtrlComp, CGridCtrl)

BEGIN_MESSAGE_MAP(CGridCtrlComp, CGridCtrl)
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGridCtrlComp message handlers
void CGridCtrlComp::OnLButtonUp(UINT nFlags, CPoint point)
{
	// When columns resized allow for Column headings to be changed to fit
	int col_resize = -1;
	if (m_MouseMode == MOUSE_SIZING_COL)
		col_resize = m_LeftClickDownCell.col;

	CGridCtrl::OnLButtonUp(nFlags, point);

	if (col_resize != -1)
		FixHeading(col_resize);
}

void CGridCtrlComp::FixHeading(int col)
{
	int col_id = GetItemData(0, col);
	int col_width = GetColumnWidth(col);

	if (col_width > 96)
	{
		SetItemText(0, col, headingLong[col_id]);
	}
	else if (col_width > 70)
	{
		SetItemText(0, col, heading[col_id]);
	}
	else if (col_width > 33)
	{
		SetItemText(0, col, headingShort[col_id]);
	}
	else
	{
		SetItemText(0, col, headingTiny[col_id]);
	}
}

////////////////////////////////////////////////////////////////////

static DWORD id_pairs[] = { 
	0,0
};

CCompareListDlg::CCompareListDlg() : CDialog()
{
	help_hwnd_ = (HWND)0;
	m_first = true;
}

BOOL CCompareListDlg::Create(CWnd *pParentWnd)
{
	if (!CDialog::Create(MAKEINTRESOURCE(IDD), pParentWnd)) // IDD_COMPARE_LIST
	{
		TRACE0("Failed to create calculator history window\n");
		return FALSE; // failed to create
	}

	// Set up the grid control
	ASSERT(GetDlgItem(IDC_GRID_DIFFS) != NULL);
	if (!grid_.SubclassWindow(GetDlgItem(IDC_GRID_DIFFS)->m_hWnd))
	{
		TRACE0("Failed to subclass compare list grid control\n");
		return FALSE;
	}

	grid_.SetDoubleBuffering();
	grid_.SetAutoFit();
	grid_.SetGridLines(GVL_BOTH); // GVL_HORZ | GVL_VERT
	grid_.SetTrackFocusCell(FALSE);
	grid_.SetFrameFocusCell(FALSE);
	grid_.SetListMode(TRUE);
	grid_.SetSingleRowSelection(TRUE);
	grid_.SetFixedRowCount(1);

	InitColumnHeadings();
	grid_.SetColumnResize();

	grid_.EnableRowHide(FALSE);
	grid_.EnableColumnHide(FALSE);
	grid_.EnableHiddenRowUnhide(FALSE);
	grid_.EnableHiddenColUnhide(FALSE);

	grid_.SetEditable(FALSE);
	grid_.SetFixedColumnSelection(FALSE);
	grid_.SetFixedRowSelection(FALSE);
	//FillGrid();

	//grid_.ExpandColsNice(FALSE);

	// Set up resizer control
	// We must set the 4th parameter true else we get a resize border
	// added to the dialog and this really stuffs things up inside a pane.
	m_resizer.Create(GetSafeHwnd(), TRUE, 100, TRUE);

	// It needs an initial size for it's calcs
	CRect rct;
	GetWindowRect(&rct);
	m_resizer.SetInitialSize(rct.Size());
	m_resizer.SetMinimumTrackingSize(rct.Size());

	// This can cause problems if done too early (OnCreate or OnInitDialog)
	m_resizer.Add(IDC_GRID_DIFFS, 0, 0, 100, 100);

	return TRUE;
}

void CCompareListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_GridControl(pDX, IDC_GRID_DIFFS, grid_);
}

BEGIN_MESSAGE_MAP(CCompareListDlg, CDialog)
	//ON_WM_DESTROY()
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	//ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	//ON_WM_ERASEBKGND()
	//ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_RCLICK, IDC_GRID_DIFFS, OnGridRClick)
END_MESSAGE_MAP()

// Message handlers
LRESULT CCompareListDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (m_first)
	{
		m_first = false;
	}

	// Display context help for ctrl set up in OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;
	}
	return FALSE;
}

void CCompareListDlg::OnHelp()
{
	if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_COMPARE_LIST_HELP))
		AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CCompareListDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// Note calling theApp.HtmlHelpWmHelp here seems to make the window go behind 
	// and then disappear when mouse up evenet is seen.  The only soln I could
	// find after a lot of experimenetation is to do it later (in OnKickIdle).
	help_hwnd_ = (HWND)pHelpInfo->hItemHandle;
	return TRUE;
}

void CCompareListDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}


void CCompareListDlg::OnGridRClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;
	TRACE("Right button click on row %d, col %d\n", pItem->iRow, pItem->iColumn);
	int fcc = grid_.GetFixedColumnCount();

	if (pItem->iRow < grid_.GetFixedRowCount() && pItem->iColumn >= fcc)
	{
		// Right click on column headings - create menu of columns available
		CMenu mm;
		mm.CreatePopupMenu();

		// Add a menu item for each column
		for (int ii = 0; heading[ii] != NULL; ++ii)
			mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(ii+fcc)>0?MF_CHECKED:0), ii+1, heading[ii]);

		// Work out where to display the popup menu
		CRect rct;
		grid_.GetCellRect(pItem->iRow, pItem->iColumn, &rct);
		grid_.ClientToScreen(&rct);
		int item = mm.TrackPopupMenu(
				TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
				(rct.left+rct.right)/2, (rct.top+rct.bottom)/2, this);

		if (item != 0)
		{
			item += fcc-1;                        // convert menu item to corresponding column number
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

// Private methods
void CCompareListDlg::InitColumnHeadings()
{
	ASSERT(sizeof(heading)/sizeof(*heading) == COL_LAST + 1);

	CString strWidths = theApp.GetProfileString("File-Settings", 
												"CompareListsColumns",
												"20,20,,20,,20,,20");

	int curr_col = grid_.GetFixedColumnCount();
	bool all_hidden = true;

	for (int ii = 0; ii < COL_LAST; ++ii)
	{
		CString ss;

		AfxExtractSubString(ss, strWidths, ii, ',');
		int width = atoi(ss);
		if (width != 0) all_hidden = false;                  // we found a visible column

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
		grid_.SetColumnWidth(grid_.GetFixedColumnCount(), 10);
		grid_.AutoSizeColumn(grid_.GetFixedColumnCount(), GVS_BOTH);
	}
}
