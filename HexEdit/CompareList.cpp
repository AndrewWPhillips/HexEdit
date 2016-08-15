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

// Number the different columns we can display
// Note these columns must match the heading strings in InitColumnHeadings
enum
{
	COL_ORIG_TYPE,        // Same, insert, delete, or replace
	COL_ORIG_HEX,         // Hex address in original file of the difference
	COL_ORIG_DEC,         // Dec address in original file of the difference
	COL_LEN_HEX,          // # of bytes inserted, deleted or replaced (hex)
	COL_LEN_DEC,          // # of bytes (decimal)
	COL_COMP_HEX,         // Hex address in compare file of the difference
	COL_COMP_DEC,         // Dec address in compare file of the difference
	COL_COMP_TYPE,        // Same as COL_ORIG_TYPE, except insert<->delete
	COL_LAST              // leave at end (not a real column but signals end of list)
};

// Grid column headings (array indices need to match the column enum)
static char *headingLong[COL_LAST+1] =
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

static char *heading[COL_LAST+1] =
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

static char *headingShort[COL_LAST+1] =
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

static char *headingTiny[COL_LAST+1] =
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

class RowAdder
{
public:
	RowAdder(CGridCtrlComp & g, CHexEditView * phev) : grid_(g)
	{
		fcc_ = grid_.GetFixedColumnCount();

		// Set fields for default item
		ClearItem(item_);

		// Set fields for text items
		ClearItem(item_ins_);
		item_ins_.strText = "Inserted";
		item_ins_.crBkClr = phev->GetCompareBgCol();
		ClearItem(item_rep_);
		item_rep_.strText = "Replaced";
		item_rep_.crFgClr = phev->GetCompareCol();
		item_rep_.crBkClr = phev->GetBackgroundCol();
		ClearItem(item_del_);
		item_del_.strText = "Deleted";
		item_del_.crFgClr = phev->GetCompareCol();
		item_del_.crBkClr = ::opp_hue(phev->GetCompareBgCol());
		ClearItem(item_equ_);
		item_equ_.strText = "Equal";

		// Set fields for address/length items
		ClearItem(item_hex_);
		item_hex_.nFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
		item_hex_.crBkClr = phev->GetBackgroundCol();
		item_hex_.crFgClr = phev->GetHexAddrCol();
		ClearItem(item_dec_);
		item_dec_.nFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
		item_dec_.crBkClr = phev->GetBackgroundCol();
		item_dec_.crFgClr = phev->GetDecAddrCol();

		item_.row = grid_.GetRowCount();       // start adding at the end
	}

	int RowCount() const { return item_.row; }

	void AddRow(CHexEditDoc::diff_t typ, FILE_ADDRESS orig, FILE_ADDRESS len, FILE_ADDRESS comp)
	{
		char disp[128];                                         // for generating displayed text

		grid_.SetRowCount(item_.row + 1);                        // append a row
		item_ins_.row = item_del_.row = item_rep_.row = item_equ_.row =
			item_hex_.row = item_dec_.row = item_.row;

		switch (typ)
		{
		case CHexEditDoc::Deletion:
			// Original Type
			item_del_.col = fcc_ + COL_ORIG_TYPE;
			grid_.SetItem(&item_del_);

			// Compare  Type
			item_ins_.col = fcc_ + COL_COMP_TYPE;
			grid_.SetItem(&item_ins_);
			break;
		case CHexEditDoc::Replacement:
			// Original Type
			item_rep_.col = fcc_ + COL_ORIG_TYPE;
			grid_.SetItem(&item_rep_);

			// Compare  Type
			item_rep_.col = fcc_ + COL_COMP_TYPE;
			grid_.SetItem(&item_rep_);
			break;
		case CHexEditDoc::Insertion:
			// Original Type
			item_ins_.col = fcc_ + COL_ORIG_TYPE;
			grid_.SetItem(&item_ins_);

			// Compare  Type
			item_del_.col = fcc_ + COL_COMP_TYPE;
			grid_.SetItem(&item_del_);
			break;
		case CHexEditDoc::Equal:
			// Original Type
			item_equ_.col = fcc_ + COL_ORIG_TYPE;
			grid_.SetItem(&item_equ_);

			// Compare  Type
			item_equ_.col = fcc_ + COL_COMP_TYPE;
			grid_.SetItem(&item_equ_);
			break;
		}

		// Orig Addr hex
		item_hex_.col = fcc_ + COL_ORIG_HEX;
		if (theApp.hex_ucase_)
			int2str(disp, sizeof(disp), orig, 16, 4, ' ', true);
		else
			int2str(disp, sizeof(disp), orig, 16, 4, ' ', false);
		item_hex_.strText = disp;
		grid_.SetItem(&item_hex_);

		// Orig Addr dec
		item_dec_.col = fcc_ + COL_ORIG_DEC;
		int2str(disp, sizeof(disp), orig);
		item_dec_.strText = disp;
		grid_.SetItem(&item_dec_);

		// Length - hex
		item_hex_.col = fcc_ + COL_LEN_HEX;
		if (theApp.hex_ucase_)
			int2str(disp, sizeof(disp), len, 16, 4, ' ', true);
		else
			int2str(disp, sizeof(disp), len, 16, 4, ' ', false);
		item_hex_.strText = disp;
		grid_.SetItem(&item_hex_);

		// Length - dec
		item_dec_.col = fcc_ + COL_LEN_DEC;
		int2str(disp, sizeof(disp), len);
		item_dec_.strText = disp;
		grid_.SetItem(&item_dec_);

		// Comp Addr hex
		item_hex_.col = fcc_ + COL_COMP_HEX;
		if (theApp.hex_ucase_)
			int2str(disp, sizeof(disp), comp, 16, 4, ' ', true);
		else
			int2str(disp, sizeof(disp), comp, 16, 4, ' ', false);
		item_hex_.strText = disp;
		grid_.SetItem(&item_hex_);

		// Comp Addr dec
		item_dec_.col = fcc_ + COL_COMP_DEC;
		int2str(disp, sizeof(disp), comp);
		item_dec_.strText = disp;
		grid_.SetItem(&item_dec_);

		++item_.row;
	}

	// Add a message row to the end of the grid
	void AddMessage(const char * mess, int id = 0)
	{
		grid_.SetRowCount(item_.row + 1);                        // append a row

		// Clear all the cells in the new row (before adding mess)
		item_.strText = CString("");
		item_.lParam = id;

		int widest_col, best_width = -1;   // used to work out the widest column
		for (item_.col = fcc_ + COL_ORIG_TYPE; item_.col < fcc_ + COL_LAST; ++item_.col)
		{
			// Check if the column of this cell is widest so far
			int width = grid_.GetColumnWidth(item_.col);
			if (width > best_width)
			{
				best_width = width;
				widest_col = item_.col;
			}

			// Clear the cell
			grid_.SetItem(&item_);
		}
		ASSERT(best_width > -1);

		// Add the message to the widest cell
		item_.col = widest_col;
		item_.strText = CString(mess);
		grid_.SetItem(&item_);

		++item_.row;
	}
private:
	static void ClearItem(GV_ITEM &item)
	{
		item.row = item.col = -1;  // row, col and strText are filled in later
		item.mask = GVIF_STATE | GVIF_FORMAT | GVIF_TEXT | GVIF_FGCLR | GVIF_BKCLR | GVIF_PARAM;
		item.nState = GVIS_READONLY;
		item.nFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
		item.crFgClr = CLR_DEFAULT;
		item.crBkClr = CLR_DEFAULT;
		//item.iImage = -1;          // GVIF_IMAGE
		item.lParam = 0;           // GVIF_PARAM
		//item.nMargin = 0;          // GVIF_MARGIN
	}

	CGridCtrlComp & grid_;         // the grid we are adding rows to the end of
	int fcc_;                      // number of fixed colums
	GV_ITEM item_;                 // Default cell colours, attributes, etc
	GV_ITEM item_ins_;             // "Inserted"
	GV_ITEM item_rep_;             // "Replaced"
	GV_ITEM item_del_;             // "Deleted"
	GV_ITEM item_equ_;             // "Equal" (bytes in both files are the same)
	GV_ITEM item_hex_;             // Hex address/length field
	GV_ITEM item_dec_;             // Decimal address/length
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
	if (col < 0 || col >= COL_LAST)
	{
		assert(0);
		return;
	}
	double fact = GetDC()->GetDeviceCaps(LOGPIXELSX) / 96.0;  // Size comparison need to be adjusted for current DPI setting

	if (size > int(96*fact))
	{
		SetItemText(0, col, headingLong[col]);
	}
	else if (size > int(70*fact))
	{
		SetItemText(0, col, heading[col]);
	}
	else if (size > int(33*fact))
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
	last_change_ = 0;
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
	// Note: calling theApp.HtmlHelpWmHelp here seems to make the window go behind 
	// and then disappear when mouse up event is seen.  The only soln I could 
	// find after a lot of experimentation is to do it later (in OnKickIdle).
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

	CHexEditView * pview = GetView();     // active file's view (or NULL if none)
	CHexEditDoc * pdoc = NULL;            // active file's doc
	int diffs = -1;                       // number of diffs, or -2 if bg compare in progress
	clock_t curr_change = -1;             // time that current compare finished
	int curr_progress = 0;                // progress value if background compare in progress
	if (pview != NULL)
	{
		pdoc = pview->GetDocument();
		ASSERT(pdoc != NULL);
		diffs = pdoc->CompareDifferences();
		curr_change = pdoc->LastCompareFinishTime();
	}

	// First work out if we have to redraw the list
	bool redraw = false;
	if (pview != phev_)
	{
		redraw = true;         // we have to redraw after switching files
		phev_ = pview;         // track which view we last used
	}
	// Check if we have to redraw because the current file status has changed
	if (!redraw && pview != NULL)
	{
		if (diffs == -2)
		{
			// Currently comparing (background) - check if we need to update progress
			static int last_progress;
			curr_progress = (pdoc->CompareProgress()/5) * 5;  // nearest 5%
			if (curr_progress != last_progress)
			{
				last_progress = curr_progress;
				redraw = true;             // need to update progress
			}
		}
		else if (diffs >= 0  && curr_change != 0 && curr_change != last_change_)
		{
			redraw = true;                // need to redraw the whole list
		}
	}

	if (redraw)
	{
		TRACE("]]]] %p %p %d %d %d\n", pview, pdoc, diffs, (int)last_change_, (int)curr_change);
		if (pview == NULL)
		{
			grid_.SetRowCount(grid_.GetFixedRowCount());   // No view so display empty list
		}
		else if (diffs >= 0)
		{
			last_change_ = curr_change;

			// Grid needs updating (switched to diff view or compare just finished)
			grid_.SetRowCount(grid_.GetFixedRowCount());   // Clear grid before refilling
			FillGrid(pdoc);                                // fill grid with results
		}
		else if (diffs == -2)
		{
			grid_.SetRowCount(grid_.GetFixedRowCount());   // Clear grid before adding message

			RowAdder rowAdder(grid_, pview);

			CString mess;
			mess.Format("%d%% ...", curr_progress);
			rowAdder.AddMessage(mess, IDS_COMPARE_INPROGRESS);
		}
		else
		{
			grid_.SetRowCount(grid_.GetFixedRowCount());  // clear list if no compare done
		}
	}

	return FALSE;
}

void CCompareListDlg::OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;

	if (pItem->iRow < grid_.GetFixedRowCount())
		return;                         // Don't do anything for header rows

	CCellRange sel = grid_.GetSelectedCellRange();

	if (!sel.IsValid() || sel.GetMinRow() != sel.GetMaxRow())
		return;                        // Don't do anything for multiple selected rows

	if (phev_ != GetView() || phev_->pcv_ == NULL)
		return;                        // Don't do anything if there is no compare view

	int row = sel.GetMinRow();
	int id = (int)grid_.GetItemData(row, COL_ORIG_TYPE);
	if (id != 0)
	{
		AvoidableTaskDialog(id);
		return;
	}

	FILE_ADDRESS addr, len = 0;     // bytes to select
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

	RowAdder rowAdder(grid_, phev_);

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
			rowAdder.AddRow(CHexEditDoc::Equal, addrA, newA - addrA, addrB);
		}
		if (next_diff == CHexEditDoc::Equal)
			break;                                      // we ran out of differences

		// Add the row for the found difference
		rowAdder.AddRow(next_diff, newA, len, newB);

		if (rowAdder.RowCount() > 10000)
		{
			rowAdder.AddMessage("Too Many", IDS_TOO_MANY_DIFFS);
			break;
		}

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
