// CompareList.cpp - implements the file compare list
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"

#include "HexEdit.h"
//#include "HexEditDoc.h"
#include "HexEditView.h"
#include "CompareView.h"
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
static char *headingLong[CCompareListDlg::COL_LAST+1] =
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

static char *heading[CCompareListDlg::COL_LAST+1] =
{
	"Orig Type",
	"Orig Addr",
	"Orig Addr",
	"Length hex",
	"Length dec",
	"Comp Addr",
	"Comp Addr",
	"Comp Type",
	NULL
};

static char *headingShort[CCompareListDlg::COL_LAST+1] =
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

static char *headingTiny[CCompareListDlg::COL_LAST+1] =
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
	//ON_WM_KEYDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGridCtrlComp message handlers

//void CGridCtrlComp::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//	CGridCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
//}

BOOL CGridCtrlComp::PreTranslateMessage(MSG* pMsg)
{
	if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) && pMsg->wParam == VK_RETURN)
	{
		return TRUE;  // if we don't trap and ignore the return key then the grid control is somehow made invisible
	}

	return CGridCtrl::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CGridCtrlComp methods

BOOL CGridCtrlComp::OnResizeColumn(int col, UINT size)
{
	if (size > 0)
		FixHeading(col, size);
	return TRUE;
}

void CGridCtrlComp::FixHeading(int col, UINT size)
{
	if (col < 0 || col >= CCompareListDlg::COL_LAST)
	{
		assert(0);
		return;
	}

	if (size > 96)
	{
		SetItemText(0, col, headingLong[col]);
	}
	else if (size > 70)
	{
		SetItemText(0, col, heading[col]);
	}
	else if (size > 33)
	{
		SetItemText(0, col, headingShort[col]);
	}
	else
	{
		SetItemText(0, col, headingTiny[col]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCompareListDlg

static DWORD id_pairs[] = {
	0,0
};

CCompareListDlg::CCompareListDlg() : CDialog()
{
	help_hwnd_ = (HWND)0;
	m_first = true;
	last_change_ = -1;
}

/////////////////////////////////////////////////////////////////////////////
// CCompareListDlg methods

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

	grid_.SetColumnResize();

	grid_.EnableRowHide(FALSE);
	grid_.EnableColumnHide(FALSE);
	grid_.EnableHiddenRowUnhide(FALSE);
	grid_.EnableHiddenColUnhide(FALSE);

	grid_.SetEditable(FALSE);
	grid_.SetFixedColumnSelection(FALSE);
	grid_.SetFixedRowSelection(FALSE);
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
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	//ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	//ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_DBLCLK, IDC_GRID_DIFFS, OnGridDoubleClick)
	ON_NOTIFY(NM_RCLICK, IDC_GRID_DIFFS, OnGridRClick)
END_MESSAGE_MAP()

// Message handlers
void CCompareListDlg::OnDestroy()
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

		theApp.WriteProfileString("File-Settings", "CompareListsColumns", strWidths);
	}

	CDialog::OnDestroy();
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
	// Don't show context menu if right-click on grid top row (used to display column menu)
	if (pWnd->IsKindOf(RUNTIME_CLASS(CGridCtrl)))
	{
		grid_.ScreenToClient(&point);
		CCellID cell = grid_.GetCellFromPt(point);
		if (cell.row == 0)
			return;
	}

	theApp.HtmlHelpContextMenu(pWnd, id_pairs);
}

LRESULT CCompareListDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
	if (m_first)
	{
		m_first = false;
		InitColumnHeadings();
		grid_.ExpandColsNice(FALSE);
	}

	// Display context help for ctrl set up in OnHelpInfo
	if (help_hwnd_ != (HWND)0)
	{
		theApp.HtmlHelpWmHelp(help_hwnd_, id_pairs);
		help_hwnd_ = (HWND)0;
	}

	CHexEditView * pview;
	CHexEditDoc * pdoc;
	if ((pview = GetView()) != NULL &&                 // we have an active view
		(pdoc = pview->GetDocument()) != NULL)         // all views should have a doc!
	{
		// Check if grid needs updating - ie, switched to a different file or new compare done (based on time)
		if (pview != phev_ || pdoc->LastCompareFinishTime() != last_change_)
		{
			CString mess;
			last_change_ = pdoc->LastCompareFinishTime();
			phev_ = pview;

			// Clear the list in preparation for redrawing
			grid_.SetRowCount(grid_.GetFixedRowCount());

			int diffs = pdoc->CompareDifferences();

			if (diffs >= 32000)
			{
				AddMessage("Too many differences");
			}
			else if (diffs >= 0)
			{
				FillGrid(pdoc);      // fill grid with results
			}
			else if (diffs == -2)
			{
				// We are already displaying progress in the status bar - this is redundant (and may slow the search as it updates too often)
				//mess.Format("%d%% complete", pdoc->CompareProgress());
				//AddMessage(mess);
				//last_change_ = -1;   // force update of grid while compare is in progress
			}
		}
	}
	else if (pview == NULL && phev_ != NULL)
	{
		last_change_ = -1;
		phev_ = NULL;

		// Clear the list as there is now no active view
		grid_.SetRowCount(grid_.GetFixedRowCount());
	}

	return FALSE;
}

void CCompareListDlg::OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;

	if (pItem->iRow < grid_.GetFixedRowCount())
		return;                         // Don't do anything for header rows

	// Do a sanity check on the current view
	if (phev_ != GetView() && phev_->pcv_ != NULL)
	{
		ASSERT(0);
		return;
	}

	CCellRange sel = grid_.GetSelectedCellRange();
	if (sel.IsValid() && sel.GetMinRow() == sel.GetMaxRow())
	{
		FILE_ADDRESS addr, len = 0;     // bytes to select
		int row = sel.GetMinRow();
		CString ss;
		char cType = 'E';

		bool sync_saved = phev_->AutoSyncCompare();
		phev_->SetAutoSyncCompare(false);                               // since we set selection in both view we don't want this

		// We need the type of the difference in order to set the length correctly for insertions deletions
		ss = (CString)grid_.GetItemText(row, COL_ORIG_TYPE);
		if (!ss.IsEmpty()) cType = ss[0];                               // First char of type string (E,R,I,D)

		// Get length of difference
		ss = (CString)grid_.GetItemText(row, COL_LEN_HEX);
		ss.Replace(" ", "");
		len = ::_strtoi64(ss, NULL, 16);

		// Get location in compare file and move the selection
		ss = (CString)grid_.GetItemText(row, COL_COMP_HEX);
		ss.Replace(" ", "");
		addr = ::_strtoi64(ss, NULL, 16);
		phev_->pcv_->MoveToAddress(addr, addr + (cType == 'I' ? 0 : len));  // I (insertion) is a deletion in the compare file

		ss = (CString)grid_.GetItemText(row, COL_ORIG_HEX);
		ss.Replace(" ", "");
		addr = ::_strtoi64(ss, NULL, 16);
		phev_->MoveToAddress(addr, addr + (cType == 'D' ? 0 : len));    // for D (deletion) len is zero

		phev_->SetAutoSyncCompare(sync_saved);                          // restore sync setting
	}
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
		for (int ii = 0; headingLong[ii] != NULL; ++ii)
			mm.AppendMenu(MF_ENABLED|(grid_.GetColumnWidth(ii+fcc)>0?MF_CHECKED:0), ii+1, headingLong[ii]);

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
												"20,20,,,,,,");

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
		item.mask = GVIF_PARAM|GVIF_FORMAT|GVIF_TEXT|GVIF_FGCLR; // change data+centered+text+colour
		item.lParam = ii;                                   // data that says what's in this column
		item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE;  // centre the heading
		item.strText = heading[ii];                         // text of the heading
		switch (ii)
		{
		case COL_ORIG_HEX:
		case COL_LEN_HEX:
		case COL_COMP_HEX:
			item.crFgClr = ::BestHexAddrCol();
			break;
		case COL_ORIG_DEC:
		case COL_LEN_DEC:
		case COL_COMP_DEC:
			item.crFgClr = ::BestDecAddrCol();
			break;
		default:
			item.crFgClr = CLR_DEFAULT;
			break;
		}
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

void CCompareListDlg::FillGrid(CHexEditDoc * pdoc)
{
	// Get the compare data from the document
	const std::vector<FILE_ADDRESS> *p_replace_A;
	const std::vector<FILE_ADDRESS> *p_replace_B;
	const std::vector<FILE_ADDRESS> *p_replace_len;
	const std::vector<FILE_ADDRESS> *p_insert_A;
	const std::vector<FILE_ADDRESS> *p_delete_B;
	const std::vector<FILE_ADDRESS> *p_insert_len;
	const std::vector<FILE_ADDRESS> *p_delete_A;
	const std::vector<FILE_ADDRESS> *p_insert_B;
	const std::vector<FILE_ADDRESS> *p_delete_len;
	pdoc->GetCompareData(&p_insert_A, &p_insert_B, &p_insert_len,
						 &p_delete_A, &p_delete_B, &p_delete_len,
						 &p_replace_A, &p_replace_B, &p_replace_len);

	ASSERT(p_replace_A->size() == p_replace_B->size() && p_replace_A->size() == p_replace_len->size());
	ASSERT(p_insert_A->size() == p_delete_B->size() && p_insert_A->size() == p_insert_len->size());
	ASSERT(p_delete_A->size() == p_insert_B->size() && p_delete_A->size() == p_delete_len->size());

	FILE_ADDRESS addrA, addrB;                              // current address in original and compare file
	FILE_ADDRESS endA = pdoc->length();                     // length of original file
	FILE_ADDRESS endB = pdoc->CompLength();
	int curr_replace = 0, curr_insert = 0, curr_delete = 0; // current indices into the arrays

	// Work out some field colours
	//inverse_col_ = ::opp_hue(phev_->GetCompareCol());
	inverse_bg_col_ = ::opp_hue(phev_->GetCompareBgCol());

	for (addrA = addrB = 0; addrA < endA || addrB < endB; )
	{
		CHexEditDoc::diff_t next_diff = CHexEditDoc::Equal;
		FILE_ADDRESS newA = endA;
		FILE_ADDRESS newB = endB;
		FILE_ADDRESS len;

		// Find next difference by checking replacements, insertions and deletions
		if (curr_replace < p_replace_A->size())
		{
			newA = (*p_replace_A)[curr_replace];
			newB = (*p_replace_B)[curr_replace];
			len  = (*p_replace_len)[curr_replace];
			next_diff = CHexEditDoc::Replacement;
		}
		if (curr_insert < p_insert_A->size() &&
			(*p_insert_A)[curr_insert] <= newA)
		{
			newA = (*p_insert_A)[curr_insert];
			newB = (*p_delete_B)[curr_insert];
			len  = (*p_insert_len)[curr_insert];
			next_diff = CHexEditDoc::Insertion;
		}
		if (curr_delete < p_delete_A->size() &&
			(*p_delete_A)[curr_delete] <= newA)
		{
			newA = (*p_delete_A)[curr_delete];
			newB = (*p_insert_B)[curr_delete];
			len  = (*p_delete_len)[curr_delete];
			next_diff = CHexEditDoc::Deletion;
		}

		if (newA > addrA)
		{
			// There are matching blocks before the next difference
			ASSERT(newA - addrA == newB - addrB);       // if they are the same they must have the same length
			AddRow(CHexEditDoc::Equal, addrA, newA - addrA, addrB);
		}
		if (next_diff == CHexEditDoc::Equal)
			break;                                      // we ran out of differences

		// Add the row for the found difference
		AddRow(next_diff, newA, len, newB);

		// Move to the next one
		addrA = newA;
		addrB = newB;
		switch (next_diff)
		{
		case CHexEditDoc::Deletion:
			++curr_delete;
			addrB += len;
			break;
		case CHexEditDoc::Replacement:
			++curr_replace;
			addrA += len;
			addrB += len;
			break;
		case CHexEditDoc::Insertion:
			++curr_insert;
			addrA += len;
			break;
		}
	}
}

void CCompareListDlg::AddMessage(const char * mess)
{
	int fcc = grid_.GetFixedColumnCount();

	GV_ITEM item;
	item.row = grid_.GetRowCount();
	grid_.SetRowCount(item.row + 1);                        // append a row

	// Set item attributes that are the same for each field (column)
	item.mask = GVIF_STATE|GVIF_FORMAT|GVIF_TEXT|GVIF_FGCLR|GVIF_BKCLR;
	item.nState = GVIS_READONLY;
	item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE;
	item.crFgClr = CLR_DEFAULT;
	item.crBkClr = CLR_DEFAULT;

	item.col = fcc + COL_ORIG_TYPE;
	item.strText = mess;
	grid_.SetItem(&item);

	item.strText = "";
	for (++item.col; item.col < fcc + COL_LAST; ++item.col)
	{
		grid_.SetItem(&item);
	}
}

void CCompareListDlg::AddRow(CHexEditDoc::diff_t typ, FILE_ADDRESS orig, FILE_ADDRESS len, FILE_ADDRESS comp)
{
	ASSERT(phev_ != NULL);
	char disp[128];                                         // for generating displayed text
	int fcc = grid_.GetFixedColumnCount();

	GV_ITEM item;
	item.row = grid_.GetRowCount();
	grid_.SetRowCount(item.row + 1);                        // append a row

	// Set item attributes that are the same for each field (column)
	item.mask = GVIF_STATE|GVIF_FORMAT|GVIF_TEXT|GVIF_FGCLR|GVIF_BKCLR;
	item.nState = GVIS_READONLY;

	item.col = fcc + COL_ORIG_TYPE;
	item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE;
	item.crFgClr = CLR_DEFAULT;
	item.crBkClr = CLR_DEFAULT;
	switch (typ)
	{
	case CHexEditDoc::Deletion:
		item.strText = "Deleted";
		item.crFgClr = phev_->GetCompareCol();
		item.crBkClr = inverse_bg_col_;
		break;
	case CHexEditDoc::Replacement:
		item.strText = "Replaced";
		item.crFgClr = phev_->GetCompareCol();
		item.crBkClr = phev_->GetBackgroundCol();
		break;
	case CHexEditDoc::Insertion:
		item.strText = "Inserted";
		//item.crFgClr = inverse_col_;
		item.crBkClr = phev_->GetCompareBgCol();
		break;

	case CHexEditDoc::Equal:
		item.strText = "Equal";
		break;
	}
	grid_.SetItem(&item);

	// Set background back to the default for the rest of the fields
	if (typ == CHexEditDoc::Equal)
		item.crBkClr = CLR_DEFAULT;
	else
		item.crBkClr = phev_->GetBackgroundCol();

	item.col = fcc + COL_ORIG_HEX;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	if (theApp.hex_ucase_)
		sprintf(disp, "%I64X", orig);
	else
		sprintf(disp, "%I64x", orig);
	item.strText = disp;
	::AddSpaces(item.strText);
	item.crFgClr = phev_->GetHexAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_ORIG_DEC;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	sprintf(disp, "%I64d", orig);
	item.strText = disp;
	::AddCommas(item.strText);
	item.crFgClr = phev_->GetDecAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_LEN_HEX;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	if (theApp.hex_ucase_)
		sprintf(disp, "%I64X", len);
	else
		sprintf(disp, "%I64x", len);
	item.strText = disp;
	::AddSpaces(item.strText);
	item.crFgClr = phev_->GetHexAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_LEN_DEC;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	sprintf(disp, "%I64d", len);
	item.strText = disp;
	::AddCommas(item.strText);
	item.crFgClr = phev_->GetDecAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_COMP_HEX;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	if (theApp.hex_ucase_)
		sprintf(disp, "%I64X", comp);
	else
		sprintf(disp, "%I64x", comp);
	item.strText = disp;
	::AddSpaces(item.strText);
	item.crFgClr = phev_->GetHexAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_COMP_DEC;
	item.nFormat = DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
	sprintf(disp, "%I64d", comp);
	item.strText = disp;
	::AddCommas(item.strText);
	item.crFgClr = phev_->GetDecAddrCol();
	grid_.SetItem(&item);

	item.col = fcc + COL_COMP_TYPE;
	item.nFormat = DT_CENTER|DT_VCENTER|DT_SINGLELINE;
	item.crFgClr = CLR_DEFAULT;
	item.crBkClr = CLR_DEFAULT;
	switch (typ)
	{
	case CHexEditDoc::Deletion:
		item.strText = "Inserted";  // Deletion in orig == insertion in compare
		//item.crFgClr = inverse_col_;
		item.crBkClr = phev_->GetCompareBgCol();
		break;
	case CHexEditDoc::Replacement:
		item.strText = "Replaced";
		item.crFgClr = phev_->GetCompareCol();
		item.crBkClr = phev_->GetBackgroundCol();
		break;
	case CHexEditDoc::Insertion:
		item.strText = "Deleted";  // Insertion in orig == deletion in compare file
		item.crFgClr = phev_->GetCompareCol();
		item.crBkClr = inverse_bg_col_;
		break;

	case CHexEditDoc::Equal:
		item.strText = "Equal";
		break;
	}
	grid_.SetItem(&item);
}
