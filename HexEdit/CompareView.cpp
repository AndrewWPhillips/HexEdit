// CompareView.cpp : implementation of the CCompareView class
//

#include "stdafx.h"
#include "HexEdit.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "CompareView.h"

// xxx TBD TODO
// Testing:
// - self-compare: temp file creation and use is OK (was buggy)
// - new file not saved to disk
// - new file self-compare
// - device file self-compare
// - stacked mode auto-sync
// - self-compare fade
// Bugs:
// - diff length files -  bit past end is not updated properly when top of block is off top of screen

extern CHexEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCompareView

IMPLEMENT_DYNCREATE(CCompareView, CScrView)

BEGIN_MESSAGE_MAP(CCompareView, CScrView)
	//ON_WM_CLOSE()  // doesn't seem to be called due to fiddling with views
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)

	ON_COMMAND(ID_COMP_FIRST, OnCompFirst)
	ON_COMMAND(ID_COMP_PREV, OnCompPrev)
	ON_COMMAND(ID_COMP_NEXT, OnCompNext)
	ON_COMMAND(ID_COMP_LAST, OnCompLast)
	ON_UPDATE_COMMAND_UI(ID_COMP_FIRST, OnUpdateCompFirst)
	ON_UPDATE_COMMAND_UI(ID_COMP_PREV, OnUpdateCompPrev)
	ON_UPDATE_COMMAND_UI(ID_COMP_NEXT, OnUpdateCompNext)
	ON_UPDATE_COMMAND_UI(ID_COMP_LAST, OnUpdateCompLast)

	// These are here to simply disable inappropriate commands to prevent them
	// being passed on to the "owner" hex view (see OnCmdMsg).

		// The compare view only shows the most recent diffs so makes no sense to show all diffs
		ON_UPDATE_COMMAND_UI(ID_COMP_ALL_FIRST, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_COMP_ALL_PREV, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_COMP_ALL_NEXT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_COMP_ALL_LAST, OnUpdateDisable)

		// Display stuff that would be confusing
		ON_UPDATE_COMMAND_UI(ID_AUTOFIT, OnUpdateDisable)

		// Printing
		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, OnUpdateDisable)

		// Editing
		ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_PASTE_ASCII, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_PASTE_EBCDIC, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_PASTE_UNICODE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_READFILE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DEL, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ALLOW_MODS, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INSERT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COMPARE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ASC2EBC, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ENCRYPT_ENCRYPT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_UNDO_CHANGES, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_IMPORT_MOTOROLA_S, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_IMPORT_INTEL, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EXPORT_INTEL, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_IMPORT_HEX_TEXT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EXPORT_HEX_TEXT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INSERT_BLOCK, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_TOGGLE_ENDIAN, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BIG_ENDIAN, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LITTLE_ENDIAN, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ZLIB_COMPRESS, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ZLIB_DECOMPRESS, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_UPPERCASE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LOWERCASE, OnUpdateDisable)

		// Other hex view specific stuff
		ON_UPDATE_COMMAND_UI(ID_TRACK_CHANGES, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EXTENDTO_MARK, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SWAP_MARK, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MARK, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GOTO_MARK, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SWAP, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_PREV, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_NEXT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_HIDE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_HIDE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_NEXT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_PREV, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_CLEAR, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_BOOKMARK_TOGGLE, OnUpdateDisable)

		// Template
		ON_UPDATE_COMMAND_UI(ID_DFFD_AUTO_SYNC, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DFFD_SYNC, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_SEL, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EDIT_WRITEFILE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_EBC2ASC, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ANSI2IBM, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_IBM2ANSI, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ENCRYPT_DECRYPT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DFFD_HIDE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DFFD_SPLIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DFFD_TAB, OnUpdateDisable)

		ON_UPDATE_COMMAND_UI(ID_AERIAL_HIDE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AERIAL_SPLIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AERIAL_TAB, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_SELECT, OnUpdateDisable)

		// Operations
		ON_UPDATE_COMMAND_UI(ID_INC_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INC_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INC_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INC_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DEC_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DEC_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DEC_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DEC_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FLIP_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FLIP_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_FLIP_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_INVERT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_NEG_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_NEG_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_NEG_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_NEG_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_XOR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_XOR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_XOR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_XOR_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_RAND_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_RAND_FAST, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ADD_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ADD_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ADD_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ADD_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AND_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AND_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AND_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_AND_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_OR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_OR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_OR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_OR_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MUL_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MUL_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MUL_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MUL_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_REV_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_REV_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_REV_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_REV_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTR_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESS_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESS_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESS_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESS_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTRU_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTRU_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTRU_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_GTRU_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESSU_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESSU_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESSU_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LESSU_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROL_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROL_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROL_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROL_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ROR_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSL_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSL_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSL_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSL_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_LSR_64BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ASR_BYTE, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ASR_16BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ASR_32BIT, OnUpdateDisable)
		ON_UPDATE_COMMAND_UI(ID_ASR_64BIT, OnUpdateDisable)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompareView construction/destruction
CCompareView::CCompareView() : phev_(NULL), addr_width_(0)
{
}

void CCompareView::OnInitialUpdate()
{
	ASSERT(phev_ != NULL);
	GetDocument()->AddCompView(phev_);

	calc_addr_width(GetDocument()->CompLength() + phev_->display_.addrbase1);

	CScrView::SetMapMode(MM_TEXT);

	ASSERT(phev_ != NULL && phev_->pfont_ != NULL && phev_->pbrush_ != NULL);
	SetFont(phev_->pfont_);
	SetBrush(phev_->pbrush_);

	CScrView::OnInitialUpdate();

	phev_->recalc_display();
	if (theApp.large_cursor_)
		BlockCaret();
	else
		LineCaret();

	CaretMode();
	// TBD xxx check what happens with empty compare file

	ValidateScroll(GetScroll());
} // OnInitialUpdate

void CCompareView::OnDraw(CDC* pDC)
{
	if (phev_ == NULL || phev_->pfont_ == NULL ||
		GetDocument()->CompareDifferences() == -4)
	{
		return;   // We're not yet ready to draw anything
	}

	pDC->SetBkMode(TRANSPARENT);

	const char *hex;
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	CRectAp doc_rect;                           // Display area relative to whole document
	CRect norm_rect;                            // Display area (norm. logical coords)

	// These are the first and last "virtual" addresses of the top and (one past) the end
	// of the addresses within norm_rect.  Note that these are not necessarilly the first
	// and last real addresses.  For example, if offset_ != 0 and at the top of file then
	// first_virt will be -ve.  Similarly the file may not even be as long as last_virt.
	FILE_ADDRESS first_virt, last_virt;
	FILE_ADDRESS first_line;                    // First line that needs displaying
	FILE_ADDRESS last_line;                     // One past last line to display

	FILE_ADDRESS first_addr = 0;                // First address to actually display
	FILE_ADDRESS last_addr = GetDocument()->CompLength(); // One past last address actually displayed

	FILE_ADDRESS line_inc;                      // 1 or -1 depending on draw dirn (up/down)
	CSize rect_inc;                             // How much to move norm_rect each time
	int offset = phev_->offset_;
	FILE_ADDRESS start_addr, end_addr;          // Address of current selection
	bool neg_x(false), neg_y(false);            // Does map mode have -ve to left or down
	bool has_focus;                             // Does this window have focus?
	int bitspixel = pDC->GetDeviceCaps(BITSPIXEL);
	if (bitspixel > 24) bitspixel = 24;         // 32 is really only 24 bits of colour
	long num_colours = 1L << (bitspixel*pDC->GetDeviceCaps(PLANES));

	int line_height, char_width, char_width_w;  // Text sizes

	ASSERT(phev_->rowsize_ > 0 && phev_->rowsize_ <= CHexEditView::max_buf);
	ASSERT(phev_->group_by_ > 0);

	if (pDC->IsPrinting())
	{
		ASSERT(0); // TBD xxx
	}
	else
	{
		has_focus = (GetFocus() == this);
		HideCaret();
		neg_x = negx(); neg_y = negy();

		// Get display rect in logical units but with origin at top left of display area in window
		CRect rct;
		GetDisplayRect(&rct);
		doc_rect = ConvertFromDP(rct);

		// Display = client rectangle translated to posn in document
//        norm_rect = doc_rect - GetScroll();
		norm_rect.left  = doc_rect.left  - GetScroll().x + phev_->bdr_left_;
		norm_rect.right = doc_rect.right - GetScroll().x + phev_->bdr_left_;
		norm_rect.top    = int(doc_rect.top    - GetScroll().y) + phev_->bdr_top_;
		norm_rect.bottom = int(doc_rect.bottom - GetScroll().y) + phev_->bdr_top_;

		// Get the current selection so that we can display it in reverse video
		GetSelAddr(start_addr, end_addr);

		line_height = phev_->line_height_;
		char_width = phev_->text_width_;
		char_width_w = phev_->text_width_w_;
	}

	// Get range of addresses that are visible the in window (overridden for printing below)
	first_virt = (doc_rect.top/line_height) * phev_->rowsize_ - offset;
	last_virt  = (doc_rect.bottom/line_height + 1) * phev_->rowsize_ - offset;

	// Work out which lines could possibly be in the display area
	if (pDC->IsPrinting())
	{
		// TBD xxx
	}
	else if (ScrollUp())
	{
		// Draw from bottom of window up since we're scrolling up (looks better)
		first_line = doc_rect.bottom/line_height;
		last_line = doc_rect.top/line_height - 1;
		line_inc = -1L;
		rect_inc = CSize(0, -line_height);

		/* Work out where to display the 1st line */
		norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;
	}
	else
	{
		// Draw from top of window down
		first_line = doc_rect.top/line_height;
		last_line = doc_rect.bottom/line_height + 1;
		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		/* Work out where to display the 1st line */
		norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;
	}

	if (first_addr < first_virt) first_addr = first_virt;
	if (last_addr > last_virt) last_addr = last_virt;

	// These are for drawing things on the screen
	CPoint pt;   // moved this here to avoid a spurious compiler error C2362
	CPen pen1(PS_SOLID, 0, same_hue(phev_->sector_col_, 100, 30));    // dark sector_col_
	CPen pen2(PS_SOLID, 0, same_hue(phev_->addr_bg_col_, 100, 30));   // dark addr_bg_col_
	CPen *psaved_pen;
	CBrush brush(phev_->sector_col_);

	if (phev_->display_.borders)
		psaved_pen = pDC->SelectObject(&pen1);
	else
		psaved_pen = pDC->SelectObject(&pen2);

	// Column related screen stuff (ruler, vertical lines etc)
	if (!pDC->IsPrinting())
	{
		ASSERT(!neg_y && !neg_x);       // This should be true when drawing on screen (uses MM_TEXT)
		// Vert. line between address and hex areas
		pt.y = phev_->bdr_top_ - 2;
		pt.x = addr_width_*char_width - char_width - doc_rect.left + phev_->bdr_left_;
		pDC->MoveTo(pt);
		pt.y = 30000;
		pDC->LineTo(pt);
		if (!phev_->display_.vert_display && phev_->display_.hex_area)
		{
			// Vert line to right of hex area
			pt.y = phev_->bdr_top_ - 2;
			pt.x = char_pos(0, char_width) - char_width_w/2 - doc_rect.left + phev_->bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (phev_->display_.vert_display || phev_->display_.char_area)
		{
			// Vert line to right of char area
			pt.y = phev_->bdr_top_ - 2;
			pt.x = char_pos(phev_->rowsize_ - 1, char_width, char_width_w) + 
				   (3*char_width_w)/2 - doc_rect.left + phev_->bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (theApp.ruler_)
		{
			int horz = phev_->bdr_left_ - GetScroll().x;       // Horiz. offset to window posn of left side

			ASSERT(phev_->bdr_top_ > 0);
			// Draw horiz line under ruler
			pt.y = phev_->bdr_top_ - 3;
			pt.x = addr_width_*char_width - char_width - doc_rect.left + phev_->bdr_left_;
			pDC->MoveTo(pt);
			pt.x = 30000;
			pDC->LineTo(pt);

			// Draw ticks using hex offsets for major ticks (if using hex addresses) or
			// decimal offsets (if using decimal addresses/line numbers and/or hex addresses)
			int major = 1;
			if (phev_->display_.decimal_addr || phev_->display_.line_nums)
				major = theApp.ruler_dec_ticks_;        // decimal ruler or both hex and decimal
			else
				major = theApp.ruler_hex_ticks_;        // only showing hex ruler
			// Hex area ticks
			if (!phev_->display_.vert_display && phev_->display_.hex_area)
				for (int column = 1; column < phev_->rowsize_; ++column)
				{
					if ((!phev_->display_.decimal_addr && !phev_->display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((phev_->display_.decimal_addr || phev_->display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = phev_->bdr_top_ - 4;
					pt.x = hex_pos(column) - char_width/2 + horz;
					if (column%phev_->group_by_ == 0)
						pt.x -= char_width/2;
					pDC->MoveTo(pt);
					pt.y -= (column%major) ? 3 : 7;
					pDC->LineTo(pt);
				}
			// Char area or stacked display ticks
			if (phev_->display_.vert_display || phev_->display_.char_area)
				for (int column = 0; column <= phev_->rowsize_; ++column)
				{
					if ((!phev_->display_.decimal_addr && !phev_->display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((phev_->display_.decimal_addr || phev_->display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = phev_->bdr_top_ - 4;
					pt.x = char_pos(column) + horz;
					if (phev_->display_.vert_display && column%phev_->group_by_ == 0)
						if (column == phev_->rowsize_)    // skip last one
							break;
						else
							pt.x -= char_width/2;
					pDC->MoveTo(pt);
					pt.y -= (column%major) ? 2 : 5;
					pDC->LineTo(pt);
				}

			// Draw numbers in the ruler area
			// Note that if we are displaying hex and decimal addresses we show 2 rows
			//      - hex offsets at top (then after moving vert down) decimal offsets
			int vert = 0;                       // Screen y pixel to the row of nos at

			// Get display rect for clipping at right and left
			CRect cli;
			GetDisplayRect(&cli);

			// Show hex offsets in the top border (ruler)
			if (phev_->display_.hex_addr)
			{
				bool between = theApp.ruler_hex_nums_ > 1;      // Only display numbers above cols if displaying for every column

				// Do hex numbers in ruler
				CRect rect(-1, vert, -1, vert + phev_->text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetHexAddrCol());   // Colour of hex addresses

				// Show hex offsets above hex area
				if (!phev_->display_.vert_display && phev_->display_.hex_area)
					for (int column = 0; column < phev_->rowsize_; ++column)
					{
						if (between && phev_->display_.addrbase1 && column == 0)
							continue;           // usee doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%phev_->group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + phev_->text_width_ + phev_->text_width_;
						if (!between)
						{
							// Draw 2 digit number above every column
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", (column + phev_->display_.addrbase1)%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%16 == 0 && theApp.ruler_hex_nums_ > 3)
						{
							// Draw 2 digit numbers to mark end of 16 columns
							rect.left -= (char_width+1)/2;
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", column%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							// Draw single digit number in between columns
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", column%16);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				// Show hex offsets above char area or stacked display
				if (phev_->display_.vert_display || phev_->display_.char_area)
					for (int column = 0; column < phev_->rowsize_; ++column)
					{
						if (between && phev_->display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (phev_->display_.vert_display && column > 0 && column%phev_->group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + phev_->text_width_ + phev_->text_width_;

						if (!between)
						{
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", (column + phev_->display_.addrbase1)%16);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%16 == 0 && theApp.ruler_hex_nums_ > 3)
						{
							rect.left -= (char_width+1)/2;
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", column%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", column%16);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				vert += phev_->text_height_;  // Move down for anything to be drawn underneath
			}
			// Show decimal offsets in the ruler
			if (phev_->display_.decimal_addr || phev_->display_.line_nums)
			{
				bool between = theApp.ruler_dec_nums_ > 1;      // Only display numbers above cols if displaying for every column

				CRect rect(-1, vert, -1, vert + phev_->text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses

				// Decimal offsets above hex area
				if (!phev_->display_.vert_display && phev_->display_.hex_area)
					for (int column = 0; column < phev_->rowsize_; ++column)
					{
						if (between && phev_->display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left  = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%phev_->group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + phev_->text_width_ + phev_->text_width_;
						if (!between)
						{
							ss.Format("%02d", (column + phev_->display_.addrbase1)%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%10 == 0 && theApp.ruler_dec_nums_ > 4)
						{
							rect.left -= (char_width+1)/2;
							ss.Format("%02d", column%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							ss.Format("%1d", column%10);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				// Decimal offsets above char area or stacked display
				if (phev_->display_.vert_display || phev_->display_.char_area)
					for (int column = 0; column < phev_->rowsize_; ++column)
					{
						if (between && phev_->display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (phev_->display_.vert_display && column > 0 && column%phev_->group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + phev_->text_width_ + phev_->text_width_;

						if (!between)
						{
							ss.Format("%1d", (column + phev_->display_.addrbase1)%10);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%10 == 0 && theApp.ruler_dec_nums_ > 4)
						{
							// If displaying nums every 5 or 10 then display 2 digits fo tens column
							rect.left -= (char_width+1)/2;
							ss.Format("%02d", column%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							// Display single dit between columns
							ss.Format("%1d", column%10);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				vert += phev_->text_height_;   // Move down for anything to be drawn underneath (currently nothing)
			}
		} // end ruler drawing
	}

	// Mask out the ruler so we don't get top of topmost line drawn into it.
	// Doing it this way allows the address of the top line to be drawn
	// higher (ie into ruler area) without being clipped.
	// Note that we currently only use bdr_top_ (for the ruler) but if
	// other borders are used similarly we would need to clip them too.
	// Note: This needs to be done after drawing the ruler.
#ifndef TEST_CLIPPING
	if (!pDC->IsPrinting())
	{
		CRect rct;
		GetDisplayRect(&rct);
		rct.bottom = rct.top;
		rct.top = 0;
		rct.left = (addr_width_ - 1)*char_width + norm_rect.left + phev_->bdr_left_;
		pDC->ExcludeClipRect(&rct);
	}
#endif
	pDC->SelectObject(psaved_pen);      // restore pen after drawing borders etc

	ASSERT(GetDocument()->ResultCount() > 0);

	// Just display most recent diffs (0) which is how this file differs from the original
	for (int dd = GetDocument()->FirstDiffAt(false, 0, first_virt); dd < GetDocument()->CompareDifferences(0); ++dd)
	{
		FILE_ADDRESS addr;
		int len;
		bool insert;

		GetDocument()->GetCompDiff(0, dd, addr, len, insert);

		if (addr + len < first_virt)
			continue;         // before top of window
		else if (addr >= last_virt)
			break;            // after end of window

		COLORREF comp_col;    // compare tracking colour - usedto shown underline (replace) or background (insert)
		int vert;
		if (insert)
		{
			// Use full height toned down background colour
			comp_col = phev_->comp_bg_col_;
			vert = -1;
		}
		else
		{
			comp_col = phev_->comp_col_;
			vert = (pDC->IsPrinting() ? phev_->print_text_height_ : phev_->text_height_)/8;
		}

		// TBD xxx need to handle inserts/deletes here one day
		TRACE2("drawing compare background %d to %d\n", int(addr), int(len));
		draw_bg(pDC, doc_rect, neg_x, neg_y,
				line_height, char_width, char_width_w,
				comp_col,
				max(addr, first_addr), 
				min(addr+len, last_addr),
				vert);
	}

	unsigned char buf[CHexEditView::max_buf];  // Holds bytes for current line being displayed
	size_t last_col = 0;                     // Number of bytes in buf to display

	// Move declarations outside loop (faster?)
	CString ss(' ', 24);                     // Temp string for formatting
	CRect tt;                                // Temp rect
	CRect addr_rect;                         // Where address is drawn

	// This was added for vert_display
	int vert_offset = 0;
	if (phev_->display_.vert_display)
	{
		if (pDC->IsPrinting())
			vert_offset = phev_->print_text_height_;
		else
			vert_offset = phev_->text_height_;
		if (neg_y)
			vert_offset = - vert_offset;
	}

	// THIS IS WHERE THE ACTUAL LINES ARE DRAWN
	// Note: we use != (line != last_line) since we may be drawing from bottom or top
	for (FILE_ADDRESS line = first_line; line != last_line;
							line += line_inc, norm_rect += rect_inc)
	{
		// Work out where to display line in logical coords (correct sign)
		tt = norm_rect;
		if (neg_x)
		{
			tt.left = -tt.left;
			tt.right = -tt.right;
		}
		if (neg_y)
		{
			tt.top = -tt.top;
			tt.bottom = -tt.bottom;
		}

		// No display needed if outside display area or past end of doc
		// Note: we don't break when past end since we may be drawing from bottom
		if (!pDC->RectVisible(&tt) || line*phev_->rowsize_ - offset > GetDocument()->CompLength())
			continue;

		// Get the bytes to display
		size_t ii;                      // Column of first byte

		if (line*phev_->rowsize_ - offset < first_addr)
		{
			last_col = GetDocument()->GetCompData(buf + offset, phev_->rowsize_ - offset, line*phev_->rowsize_) +
						offset;
			ii = size_t(first_addr - (line*phev_->rowsize_ - offset));
			ASSERT(int(ii) < phev_->rowsize_);
		}
		else
		{
			last_col = GetDocument()->GetCompData(buf, phev_->rowsize_, line*phev_->rowsize_ - offset);
			ii = 0;
		}

		if (line*phev_->rowsize_ - offset + last_col - last_addr >= phev_->rowsize_)
			last_col = 0;
		else if (line*phev_->rowsize_ - offset + last_col > last_addr)
			last_col = size_t(last_addr - (line*phev_->rowsize_ - offset));

		// Draw address if ...
		if ((addr_width_ - 1)*char_width + tt.left > 0 &&    // not off to the left
			(tt.top + phev_->text_height_/4 >= phev_->bdr_top_ || pDC->IsPrinting()))   // and does not encroach into ruler
		{
			addr_rect = tt;            // tt with right margin where addresses end
			addr_rect.right = addr_rect.left + addr_width_*char_width - char_width - 1;
			if (pDC->IsPrinting())
				if (neg_y)
					addr_rect.bottom = addr_rect.top - phev_->print_text_height_;
				else
					addr_rect.bottom = addr_rect.top + phev_->print_text_height_;
			else
				if (neg_y)
					addr_rect.bottom = addr_rect.top - phev_->text_height_;
				else
					addr_rect.bottom = addr_rect.top + phev_->text_height_;

			if (hex_width_ > 0)
			{
				int ww = hex_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
					theApp.hex_ucase_ ? "%0*I64X:" : "%0*I64x:",
						hex_width_,
						(line*phev_->rowsize_ - offset > first_addr ? line*phev_->rowsize_ - offset : first_addr) + phev_->display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddSpaces(ss);
					ww += (hex_width_-1)/4;
				}
				pDC->SetTextColor(GetHexAddrCol());   // Colour of hex addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}

			if (dec_width_ > 0)
			{
				int ww = dec_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
						"%*I64d:",
						dec_width_,
						(line*phev_->rowsize_ - offset > first_addr ? line*phev_->rowsize_ - offset : first_addr) + phev_->display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddCommas(ss);
					ww += (dec_width_-1)/3;
				}
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}

			if (num_width_ > 0)
			{
				int ww = num_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
						"%*I64d:",
						num_width_,
						line + phev_->display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddCommas(ss);
					ww += (num_width_-1)/3;
				}
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}
		}

		// Keep track of the current colour so we only set it when it changes
		COLORREF current_colour = phev_->kala[buf[ii]];
		pDC->SetTextColor(current_colour);

		// Display hex values or stacked
		if (phev_->display_.vert_display || phev_->display_.hex_area)
		{
			int posx = tt.left + hex_pos(0, char_width);                 // Horiz pos of 1st hex column

			// Display each byte in the row as hex (and char if nec.)
			for (size_t jj = ii ; jj < last_col; ++jj)
			{
				// First eliminate anything to left or right of the display area
				if (phev_->display_.vert_display)
				{
					if (posx + int(jj + 1 + jj/phev_->group_by_)*char_width_w < 0)
						continue;
					else if (posx + int(jj + jj/phev_->group_by_)*char_width_w >= tt.right)
						break;
				}
				else
				{
					if (posx + int((jj+1)*3 + jj/phev_->group_by_)*char_width < 0)
						continue;
					else if (posx + int(jj*3 + jj/phev_->group_by_)*char_width >= tt.right)
						break;
				}

				// Change colour if byte range has changed
				if (current_colour != phev_->kala[buf[jj]])
				{
					current_colour = phev_->kala[buf[jj]];
					pDC->SetTextColor(current_colour);
				}

				if (phev_->display_.vert_display)
				{
					// Now display the character in the top row
					if (phev_->display_.char_set != CHARSET_EBCDIC)
					{
						if ((buf[jj] >= 32 && buf[jj] < 127) ||
							(phev_->display_.char_set != CHARSET_ASCII && buf[jj] >= phev_->first_char_ && buf[jj] <= phev_->last_char_) )
						{
							// Display normal char or graphic char if in font
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, (char *)&buf[jj], 1);
						}
						else if (phev_->display_.control == 0 || buf[jj] >= 32)
						{
							// Display control char and other chars as red '.'
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, ".", 1);
						}
						else if (phev_->display_.control == 1)
						{
							// Display control chars as red uppercase equiv.
							char cc = buf[jj] + 0x40;
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, &cc, 1);
						}
						else if (phev_->display_.control == 2)
						{
							// Display control chars as C escape code (in red)
							const char *check = "\a\b\f\n\r\t\v\0";
							const char *display = "abfnrtv0";
							const char *pp;
							if (/*buf[jj] != '\0' && */(pp = strchr(check, buf[jj])) != NULL)
								pp = display + (pp-check);
							else
								pp = ".";
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, pp, 1);
						}
					}
					else
					{
						// Display EBCDIC (or red dot if not valid EBCDIC char)
						if (e2a_tab[buf[jj]] == '\0')
						{
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, ".", 1);
						}
						else
						{
							pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top, (char *)&e2a_tab[buf[jj]], 1);
						}
					}

					// Display the hex digits below that, one below the other
					pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top + vert_offset,   &hex[(buf[jj]>>4)&0xF], 1);
					pDC->TextOut(posx + (jj + jj/phev_->group_by_)*char_width_w, tt.top + vert_offset*2, &hex[buf[jj]&0xF], 1);
				}
				else
				{
					char hh[2];

					// Create hex digits and display them
					hh[0] = hex[(buf[jj]>>4)&0xF];
					hh[1] = hex[buf[jj]&0xF];

					// This actually displays the bytes (in hex)!
					// Note: removed calcs that were previously encapsulated in hex_pos
					pDC->TextOut(posx + (jj*3 + jj/phev_->group_by_)*char_width, tt.top, hh, 2);
				}
			}
		}

		// Display char values (for char and hex+char modes but not stacked mode)
		if (!phev_->display_.vert_display && phev_->display_.char_area)
		{
			// Keep track of the current colour so we only set it when it changes
			int posc = tt.left + char_pos(0, char_width, char_width_w);  // Horiz pos of 1st char column

			for (size_t kk = ii ; kk < last_col; ++kk)
			{
				if (posc + int(kk+1)*char_width_w < 0)
					continue;
				else if (posc + int(kk)*char_width_w >= tt.right)
					break;

				if (current_colour != phev_->kala[buf[kk]])
				{
					current_colour = phev_->kala[buf[kk]];
					pDC->SetTextColor(current_colour);
				}

				// Display byte in char display area (as ASCII, EBCDIC etc)
				if (phev_->display_.char_set != CHARSET_EBCDIC)
				{
					if ((buf[kk] >= 32 && buf[kk] < 127) ||
						(phev_->display_.char_set != CHARSET_ASCII && buf[kk] >= phev_->first_char_ && buf[kk] <= phev_->last_char_) )
					{
						// Display normal char or graphic char if in font
						pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&buf[kk], 1);
					}
					else if (phev_->display_.control == 0 || buf[kk] > 31)
					{
						// Display control char and other chars as red '.'
						pDC->TextOut(posc + kk*char_width_w, tt.top, ".", 1);
					}
					else if (phev_->display_.control == 1)
					{
						// Display control chars as red uppercase equiv.
						char cc = buf[kk] + 0x40;
						pDC->TextOut(posc + kk*char_width_w, tt.top, &cc, 1);
					}
					else if (phev_->display_.control == 2)
					{
						// Display control chars as C escape code (in red)
						const char *check = "\a\b\f\n\r\t\v\0";
						const char *display = "abfnrtv0";
						const char *pp;
						if (/*buf[kk] != '\0' && */(pp = strchr(check, buf[kk])) != NULL)
							pp = display + (pp-check);
						else
							pp = ".";
						pDC->TextOut(posc + kk*char_width_w, tt.top, pp, 1);
					}
				}
				else
				{
					// Display EBCDIC (or red dot if not valid EBCDIC char)
					if (e2a_tab[buf[kk]] == '\0')
					{
						pDC->TextOut(posc + kk*char_width_w, tt.top, ".", 1);
					}
					else
					{
						pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&e2a_tab[buf[kk]], 1);
					}
				}
			}
		}

		// If any part of the line is within the current selection
		if (!pDC->IsPrinting() && start_addr < end_addr &&
			end_addr > line*phev_->rowsize_ - offset && start_addr < (line+1)*phev_->rowsize_ - offset)
		{
			FILE_ADDRESS start = max(start_addr, line*phev_->rowsize_ - offset);
			FILE_ADDRESS   end = min(end_addr, (line+1)*phev_->rowsize_ - offset);
//            ASSERT(end > start);

			ASSERT(phev_->display_.hex_area || phev_->display_.char_area);
			if (!phev_->display_.vert_display && phev_->display_.hex_area)
			{
				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*phev_->rowsize_ - offset) - 1)) + 2*phev_->text_width_;
				rev.left += hex_pos(int(start - (line*phev_->rowsize_ - offset)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				if (has_focus && !phev_->display_.edit_char || num_colours <= 256)
					pDC->InvertRect(&rev);  // Full contrast reverse video only if in editing in hex area
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}

			if (phev_->display_.vert_display || phev_->display_.char_area)
			{
				// Draw char selection in inverse
				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*phev_->rowsize_ - offset) - 1)) + phev_->text_width_w_;
				rev.left += char_pos(int(start - (line*phev_->rowsize_ - offset)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				if (num_colours <= 256 || has_focus && (phev_->display_.vert_display || phev_->display_.edit_char))
					pDC->InvertRect(&rev);
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}
		}
		else if (theApp.show_other_ && has_focus &&
				 !phev_->display_.vert_display &&
				 phev_->display_.char_area && phev_->display_.hex_area &&  // we can only display in the other area if both exist
				 !pDC->IsPrinting() &&
				 start_addr == end_addr &&
				 start_addr >= line*phev_->rowsize_ - offset && 
				 start_addr < (line+1)*phev_->rowsize_ - offset)
		{
			// Draw "shadow" cursor in the other area
			FILE_ADDRESS start = max(start_addr, line*phev_->rowsize_ - offset);
			FILE_ADDRESS   end = min(end_addr, (line+1)*phev_->rowsize_ - offset);

			CRect rev(norm_rect);
			if (phev_->display_.edit_char)
			{
				ASSERT(phev_->display_.char_area);
				rev.right = rev.left + hex_pos(int(end - (line*phev_->rowsize_ - offset))) + 2*phev_->text_width_;
				rev.left += hex_pos(int(start - (line*phev_->rowsize_ - offset)));
			}
			else
			{
				ASSERT(phev_->display_.hex_area);
				rev.right = rev.left + char_pos(int(end - (line*phev_->rowsize_ - offset))) + phev_->text_width_w_;
				rev.left += char_pos(int(start - (line*phev_->rowsize_ - offset)));
			}
			if (neg_x)
			{
				rev.left = -rev.left;
				rev.right = -rev.right;
			}
			if (neg_y)
			{
				rev.top = -rev.top;
				rev.bottom = -rev.bottom;
			}
			pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
		}
		else if (!has_focus &&
				 !pDC->IsPrinting() &&
				 start_addr == end_addr &&
				 start_addr >= line*phev_->rowsize_ - offset && 
				 start_addr < (line+1)*phev_->rowsize_ - offset)
		{
			// Draw "shadow" for current byte when lost focus
			if (!phev_->display_.vert_display && phev_->display_.hex_area)
			{
				// Get rect for hex area or stacked mode
				FILE_ADDRESS start = max(start_addr, line*phev_->rowsize_ - offset);
				FILE_ADDRESS end = min(end_addr, (line+1)*phev_->rowsize_ - offset);

				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*phev_->rowsize_ - offset))) + 2*phev_->text_width_;
				rev.left += hex_pos(int(start - (line*phev_->rowsize_ - offset)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
			}
			if (phev_->display_.vert_display || phev_->display_.char_area)
			{
				// Get rect for char area
				FILE_ADDRESS start = max(start_addr, line*phev_->rowsize_ - offset);
				FILE_ADDRESS   end = min(end_addr, (line+1)*phev_->rowsize_ - offset);

				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*phev_->rowsize_ - offset))) + phev_->text_width_w_;
				rev.left += char_pos(int(start - (line*phev_->rowsize_ - offset)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
			}
		}
	} // for each display (text) line

	if (!pDC->IsPrinting())
	{
		ShowCaret();
	}
} // OnDraw

void CCompareView::draw_bg(CDC* pDC, const CRectAp &doc_rect, bool neg_x, bool neg_y,
						   int line_height, int char_width, int char_width_w,
						   COLORREF clr, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
						   int draw_height /*=-1*/)
{
	if (end_addr < start_addr) return;

	if (draw_height > -1 && draw_height < 2) draw_height = 2;  // make it at least 2 pixels (1 does not draw properly)

#ifdef USE_ROP2
	int saved_rop = pDC->SetROP2(R2_NOTXORPEN);
	CPen pen(PS_SOLID, 0, clr);
	CPen * psaved_pen = pDC->SelectObject(&pen);
	CBrush brush(clr);
	CBrush * psaved_brush = pDC->SelectObject(&brush);
#endif

	FILE_ADDRESS start_line = (start_addr + phev_->offset_)/phev_->rowsize_;
	FILE_ADDRESS end_line = (end_addr + phev_->offset_)/phev_->rowsize_;
	int start_in_row = int((start_addr + phev_->offset_)%phev_->rowsize_);
	int end_in_row = int((end_addr + phev_->offset_)%phev_->rowsize_);

	CRect rct;

	if (start_line == end_line)
	{
		// Draw the block (all on one line)
		rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
		rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
		if (neg_y)
		{
			rct.top = -rct.top;
			rct.bottom = -rct.bottom;
		}

		if (!phev_->display_.vert_display && phev_->display_.hex_area)
		{
			rct.left = hex_pos(start_in_row, char_width) - 
					   doc_rect.left + bdr_left_;
			rct.right = hex_pos(end_in_row - 1, char_width) +
						2*char_width - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}

			if (neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right)
#ifdef USE_ROP2
				pDC->Rectangle(&rct);
#else
				pDC->FillSolidRect(&rct, clr);
#endif
		}

		if (phev_->display_.vert_display || phev_->display_.char_area)
		{
			// rct.top = start_line * line_height;
			// rct.bottom = rct.top + line_height;
			rct.left = char_pos(start_in_row, char_width, char_width_w) -
					   doc_rect.left + bdr_left_;
			rct.right = char_pos(end_in_row - 1, char_width, char_width_w) +
						char_width_w - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
#ifdef USE_ROP2
			pDC->Rectangle(&rct);
#else
			pDC->FillSolidRect(&rct, clr);
#endif
		}

#ifdef USE_ROP2
		pDC->SetROP2(saved_rop);
		pDC->SelectObject(psaved_pen);
		pDC->SelectObject(psaved_brush);
#endif
		return;  // All on one line so that's it
	}

	// Block extends over (at least) 2 lines so draw the partial lines at each end
	rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
	rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
	if (neg_y)
	{
		rct.top = -rct.top;
		rct.bottom = -rct.bottom;
	}
	if (!phev_->display_.vert_display && phev_->display_.hex_area)
	{
		rct.left = hex_pos(start_in_row, char_width) -
				   doc_rect.left + bdr_left_;
		rct.right = hex_pos(phev_->rowsize_ - 1, char_width) +
					2*char_width - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
#ifdef USE_ROP2
		pDC->Rectangle(&rct);
#else
		pDC->FillSolidRect(&rct, clr);
#endif
	}

	if (phev_->display_.vert_display || phev_->display_.char_area)
	{
		rct.left = char_pos(start_in_row, char_width, char_width_w) -
				   doc_rect.left + bdr_left_;
		rct.right = char_pos(phev_->rowsize_ - 1, char_width, char_width_w) +
					char_width_w - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
#ifdef USE_ROP2
		pDC->Rectangle(&rct);
#else
		pDC->FillSolidRect(&rct, clr);
#endif
	}

	// Last (partial) line
	rct.bottom = int((end_line+1) * line_height - doc_rect.top + bdr_top_);
	rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
	if (neg_y)
	{
		rct.top = -rct.top;
		rct.bottom = -rct.bottom;
	}
	if (!phev_->display_.vert_display && phev_->display_.hex_area)
	{
		rct.left = hex_pos(0, char_width) -
				   doc_rect.left + bdr_left_;
		rct.right = hex_pos(end_in_row - 1, char_width) +
					2*char_width - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		if (neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right)
#ifdef USE_ROP2
			pDC->Rectangle(&rct);
#else
			pDC->FillSolidRect(&rct, clr);
#endif
	}

	if (phev_->display_.vert_display || phev_->display_.char_area)
	{
		rct.left = char_pos(0, char_width, char_width_w) -
				   doc_rect.left + bdr_left_;
		rct.right = char_pos(end_in_row - 1, char_width, char_width_w) +
					char_width_w - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
#ifdef USE_ROP2
		pDC->Rectangle(&rct);
#else
		pDC->FillSolidRect(&rct, clr);
#endif
	}

	// Now draw all the full lines
	if (draw_height > 0)
	{
		// Since we ar not doing a complete fill of the lines (eg underline)
		// we have to do each line of text individually
		for (++start_line; start_line < end_line; ++start_line)
		{
			rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
			rct.top = rct.bottom - draw_height;
			if (neg_y)
			{
				rct.top = -rct.top;
				rct.bottom = -rct.bottom;
			}
			if (!phev_->display_.vert_display && phev_->display_.hex_area)
			{
				rct.left = hex_pos(0, char_width) -
						   doc_rect.left + bdr_left_;
				rct.right = hex_pos(phev_->rowsize_ - 1, char_width) +
							2*char_width - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
				ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
#ifdef USE_ROP2
				pDC->Rectangle(&rct);
#else
				pDC->FillSolidRect(&rct, clr);
#endif
			}

			if (phev_->display_.vert_display || phev_->display_.char_area)
			{
				rct.left = char_pos(0, char_width, char_width_w) -
						   doc_rect.left + bdr_left_;
				rct.right = char_pos(phev_->rowsize_ - 1, char_width, char_width_w) +
							char_width_w - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
#ifdef USE_ROP2
				pDC->Rectangle(&rct);
#else
				pDC->FillSolidRect(&rct, clr);
#endif
			}
		}
	}
	else if (start_line + 1 < end_line)
	{
		// Draw the complete lines as one block
		rct.top = int((start_line + 1) * line_height - doc_rect.top + bdr_top_);
		rct.bottom = int(end_line * line_height - doc_rect.top + bdr_top_);
		if (neg_y)
		{
			rct.top = -rct.top;
			rct.bottom = -rct.bottom;
		}

		if (!phev_->display_.vert_display && phev_->display_.hex_area)
		{
			rct.left = hex_pos(0, char_width) -
					   doc_rect.left + bdr_left_;
			rct.right = hex_pos(phev_->rowsize_ - 1, char_width) +
						2*char_width - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
#ifdef USE_ROP2
			pDC->Rectangle(&rct);
#else
			pDC->FillSolidRect(&rct, clr);
#endif
		}

		if (phev_->display_.vert_display || phev_->display_.char_area)
		{
			rct.left = char_pos(0, char_width, char_width_w) -
					   doc_rect.left + bdr_left_;
			rct.right = char_pos(phev_->rowsize_ - 1, char_width, char_width_w) +
						char_width_w - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
#ifdef USE_ROP2
			pDC->Rectangle(&rct);
#else
			pDC->FillSolidRect(&rct, clr);
#endif
		}
	}

#ifdef USE_ROP2
	pDC->SetROP2(saved_rop);
	pDC->SelectObject(psaved_pen);
	pDC->SelectObject(psaved_brush);
#endif
	return;
}

// Move scroll or caret position in response to a key press.
// Note that this overrides CScrView::MovePos().
BOOL CCompareView::MovePos(UINT nChar, UINT nRepCnt,
						 BOOL control_down, BOOL shift_down, BOOL caret_on)
{
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);   // Is selection base at end of selection?
	int row = 0;
	if (start_addr == end_addr && phev_->display_.vert_display)
		row = pos2row(GetCaret());
	FILE_ADDRESS new_address;

	// Start with start of (or end of, if moving forwards) current selection
	if (shift_down)
	{
		// Work out which end of selection is being extended
		if (end_base)
			new_address = start_addr;
		else
			new_address = end_addr;
		//++shift_moves_;
	}
	else if (start_addr == end_addr )
		new_address = start_addr;                       // No current selection
	else if (nChar == VK_DOWN || nChar == VK_NEXT)
		new_address = end_addr;                         // Move from char after selection
	else if (nChar == VK_RIGHT || nChar == VK_END)
		new_address = end_addr - 1;                     // Move from last char of selection
	else
		new_address = start_addr;                       // Move from start of selection

	CSizeAp tt, pp, ll;                   // Size of document total,page,line

	switch (nChar)
	{
	case VK_LEFT:
		if (control_down)
		{
			// Work out how many groups there are to start of file
			long gpr = (phev_->rowsize_ - 1)/phev_->group_by_ + 1;    // groups per row
			FILE_ADDRESS groups = ((new_address + phev_->offset_)/phev_->rowsize_) * gpr +
						  ((new_address + phev_->offset_)%phev_->rowsize_ + phev_->group_by_ - 1)/phev_->group_by_;
			// Calculate the group to move to and address of 1st byte
			groups -= nRepCnt;
			new_address = (groups/gpr) * phev_->rowsize_ - phev_->offset_ + (groups%gpr) * phev_->group_by_;
		}
		else
		{
			new_address -= nRepCnt;
		}
		break;
	case VK_RIGHT:
		if (control_down)
		{
			// First work out how many groups there are to start of file
			long gpr = (phev_->rowsize_ - 1)/phev_->group_by_ + 1;    // groups per row
			FILE_ADDRESS groups = ((new_address + phev_->offset_)/phev_->rowsize_) * gpr +
						  ((new_address + phev_->offset_)%phev_->rowsize_)/phev_->group_by_;
			// Calculate the group to move to
			groups += nRepCnt;
			new_address = (groups/gpr) * phev_->rowsize_ - phev_->offset_ + (groups%gpr) * phev_->group_by_;
		}
		else
		{
			new_address += nRepCnt;
		}
		break;
	case VK_UP:
		if (phev_->display_.vert_display && !shift_down)
		{
			new_address -= phev_->rowsize_ * ((2 - row + nRepCnt)/3);
			row = (3333 + row - nRepCnt)%3;   // Add a large number div. by 3 to make sure % operand is +ve
		}
		else
			new_address -= phev_->rowsize_ * nRepCnt;
		break;
	case VK_DOWN:
		if (phev_->display_.vert_display && !shift_down)
		{
			new_address += phev_->rowsize_ * ((row + nRepCnt)/3);
			row = (row + nRepCnt)%3;
		}
		else
			new_address += phev_->rowsize_ * nRepCnt;
		break;
	case VK_HOME:
		if (control_down)
		{
			new_address = 0;
		}
		else
		{
			new_address = ((new_address + phev_->offset_)/phev_->rowsize_) * phev_->rowsize_ - phev_->offset_;
		}
		break;
	case VK_END:
		if (control_down)
		{
			new_address = GetDocument()->CompLength();
		}
		else
		{
			new_address = ((new_address + phev_->offset_)/phev_->rowsize_ + 1) * phev_->rowsize_ - phev_->offset_ - 
								(shift_down ? 0 : 1);
		}
		break;
	case VK_PRIOR:
		GetSize(tt, pp, ll);
		new_address -= phev_->rowsize_ * (pp.cy/phev_->line_height_) * nRepCnt;
		break;
	case VK_NEXT:
		GetSize(tt, pp, ll);
		new_address += phev_->rowsize_ * (pp.cy/phev_->line_height_) * nRepCnt;
		break;
	default:
		return CScrView::MovePos(nChar, nRepCnt, control_down, shift_down, caret_on);
	}

	if (new_address < 0)
	{
		new_address = 0;
		row = 0;
	}
	else if (new_address > GetDocument()->CompLength())
	{
		new_address = GetDocument()->CompLength();
		if (phev_->display_.vert_display && !shift_down)
			row = 2;
	}

	// Scroll addresses into view if moved to left column of hex area or 
	// left column of char area when no hex area displayed
	if ((new_address + phev_->offset_) % phev_->rowsize_ == 0 &&
		(phev_->display_.vert_display || !phev_->display_.edit_char || !phev_->display_.hex_area))
	{
		SetScroll(CPointAp(0,-1));
	}

	if (shift_down && end_base)
	{
		MoveToAddress(end_addr, new_address);
	}
	else if (shift_down)
	{
		MoveToAddress(start_addr, new_address);
	}
	else
		MoveToAddress(new_address, -1, row);

	return TRUE;                // Indicate that keystroke used
}

void CCompareView::MoveToAddress(FILE_ADDRESS astart, FILE_ADDRESS aend /*=-1*/, int row /*=0*/)
{
	ASSERT((astart & ~0x3fffFFFFffffFFFF) == 0); // Make sure top 2 bits not on

	if (astart < 0 || astart > GetDocument()->length())
		astart = GetDocument()->length();
	if (aend < 0 || aend > GetDocument()->length())
		aend = astart;

	FILE_ADDRESS pstart, pend;
	GetSelAddr(pstart, pend);
	int prow = 0;   // Row of cursor if vert_display mode
	if (pstart == pend && phev_->display_.vert_display)
		prow = pos2row(GetCaret());

	// Is the caret/selection now in a different position
	if (astart != pstart || aend != pend || row != prow)
	{
		// Move the caret/selection (THIS IS THE IMPORTANT BIT)
		SetSel(addr2pos(astart, row), addr2pos(aend, row), true);

		if (phev_->AutoSyncCompare())
		{
			phev_->SetAutoSyncCompare(false);  // avoid inf. recursion
			phev_->MoveWithDesc("Compare Auto-sync", astart, aend, -1, -1, FALSE, FALSE, row);
			phev_->SetAutoSyncCompare(true);
		}
	}

	DisplayCaret();                             // Make sure caret is in the display
}

int CCompareView::pos2row(CPointAp pos)
{
	return int((pos.y%phev_->line_height_)/phev_->text_height_);
}

// These are like the CHexEditView versions (pos_hex and pos_char)
// but are duplicated here as we have our own addr_width_ member.
int CCompareView::pos_hex(int pos, int inside) const
{
	int col = pos - addr_width_*phev_->text_width_;
	col -= (col/(phev_->text_width_*(phev_->group_by_*3+1)))*phev_->text_width_;
	col = col/(3*phev_->text_width_);

	// Make sure col is within valid range
	if (col < 0) col = 0;
	else if (inside == 1 && col >= phev_->rowsize_) col = phev_->rowsize_ - 1;
	else if (inside == 0 && col > phev_->rowsize_) col = phev_->rowsize_;

	return col;
}

int CCompareView::pos_char(int pos, int inside) const
{
	int col;
	if (phev_->display_.vert_display)
	{
		col = (pos - addr_width_*phev_->text_width_)/phev_->text_width_w_;
		col -= col/(phev_->group_by_+1);
	}
	else if (phev_->display_.hex_area)
		col = (pos - addr_width_*phev_->text_width_ -
			   phev_->rowsize_*3*phev_->text_width_ -
			   ((phev_->rowsize_-1)/phev_->group_by_)*phev_->text_width_) / phev_->text_width_w_;
	else
		col = (pos - addr_width_*phev_->text_width_) / phev_->text_width_w_;

	// Make sure col is within valid range
	if (col < 0) col = 0;
	else if (inside == 1 && col >= phev_->rowsize_) col = phev_->rowsize_ - 1;
	else if (inside == 0 && col > phev_->rowsize_) col = phev_->rowsize_;

	return col;
}

// Relies on CHexEditView::recalc_display to do most of the work.
void CCompareView::recalc_display()
{
	if (phev_ == NULL) return;

	// Ensure ScrView scrolling matches global options
	if (GetScrollPastEnds() != theApp.scroll_past_ends_)
	{
		SetScrollPastEnds(theApp.scroll_past_ends_);
		SetScroll(GetScroll());
	}
	SetAutoscroll(theApp.autoscroll_accel_);

	// phev_->recalc_display();  // we now assume that the hex view is up to date

	// Make sure our borders are the same as hex view's
	bdr_top_ = phev_->bdr_top_;
	bdr_bottom_ = phev_->bdr_bottom_;
	bdr_left_ = phev_->bdr_left_;
	bdr_right_ = phev_->bdr_right_;

	// Calculate width of address area which may be different than hex view's address width
	calc_addr_width(GetDocument()->CompLength() + phev_->display_.addrbase1);

	if (phev_->display_.vert_display)
		SetTSize(CSizeAp(-1, ((GetDocument()->CompLength() + phev_->offset_)/phev_->rowsize_ + 1)*3));  // 3 rows of text
	else
		SetTSize(CSizeAp(-1, (GetDocument()->CompLength() + phev_->offset_)/phev_->rowsize_ + 1));

	// Make sure we know the width of the display area
	if (phev_->display_.vert_display || phev_->display_.char_area)
		SetSize(CSize(char_pos(phev_->rowsize_-1)+phev_->text_width_w_+phev_->text_width_w_/2+1, -1));
	else
	{
		ASSERT(phev_->display_.hex_area);
		SetSize(CSize(hex_pos(phev_->rowsize_-1)+2*phev_->text_width_+phev_->text_width_/2+1, -1));
	}
} /* recalc_display() */

// This allows the compre view to keep track of it's current selection before
// a display change that may invalidate the CScrView (displayed) selection
void CCompareView::begin_change()
{
	previous_caret_displayed_ = CaretDisplayed();
	previous_end_base_ = GetSelAddr(previous_start_addr_, previous_end_addr_);
	previous_row_ = 0;
	if (previous_start_addr_ == previous_end_addr_ && phev_->display_.vert_display)
		previous_row_ = pos2row(GetCaret());
}

void CCompareView::end_change()
{
	//recalc_display();
	if (!phev_->display_.vert_display) previous_row_ = 0;  // If vert mode turned off make sure row is zero
	if (previous_end_base_)
		SetSel(addr2pos(previous_end_addr_, previous_row_), addr2pos(previous_start_addr_, previous_row_), true);
	else
		SetSel(addr2pos(previous_start_addr_, previous_row_), addr2pos(previous_end_addr_, previous_row_));
	if (previous_caret_displayed_)
		DisplayCaret();                 // Keep caret within display
	DoInvalidate();
}

void CCompareView::calc_addr_width(FILE_ADDRESS length)
{
	hex_width_ = phev_->display_.hex_addr ? SigDigits(length, 16) : 0;
	dec_width_ = phev_->display_.decimal_addr ? SigDigits(length) : 0;
	num_width_ = phev_->display_.line_nums ? SigDigits(length/phev_->rowsize_) : 0;

	addr_width_ = hex_width_ + dec_width_ + num_width_;

	// Allow for separators (spaces and commas)
	if (theApp.nice_addr_)
		addr_width_ += (hex_width_-1)/4 + (dec_width_-1)/3 + (num_width_-1)/3;

	// Also add 1 for the colon after each number
	addr_width_ += hex_width_ > 0 ? 1 : 0;
	addr_width_ += dec_width_ > 0 ? 1 : 0;
	addr_width_ += num_width_ > 0 ? 1 : 0;
	++addr_width_;  // Add one for the separator line
}

FILE_ADDRESS CCompareView::pos2addr(CPointAp pos, BOOL inside /*= TRUE*/) const
{
	FILE_ADDRESS address;
	address = (pos.y/phev_->line_height_)*phev_->rowsize_ - phev_->offset_;
	if (phev_->display_.vert_display || phev_->display_.edit_char)
		address += pos_char(pos.x, inside);
	else
		address += pos_hex(pos.x + phev_->text_width_/2, inside);
	return address;
}


CPointAp CCompareView::addr2pos(FILE_ADDRESS address, int row /*=0*/) const
{
	ASSERT(row == 0 || (phev_->display_.vert_display && row < 3));
	address += phev_->offset_;
	if (phev_->display_.vert_display)
		return CPointAp(char_pos(int(address%phev_->rowsize_)), (address/phev_->rowsize_) * phev_->line_height_ + row * phev_->text_height_);
	else if (phev_->display_.edit_char)
		return CPointAp(char_pos(int(address%phev_->rowsize_)), address/phev_->rowsize_ * phev_->line_height_);
	else
		return CPointAp(hex_pos(int(address%phev_->rowsize_)), address/phev_->rowsize_ * phev_->line_height_);
}

/////////////////////////////////////////////////////////////////////////////
// Overrides

void CCompareView::ValidateCaret(CPointAp &pos, BOOL inside /*=true*/)
{
	ASSERT(phev_ != NULL);
	// Ensure pos is a valid caret position or move it to the closest such one
	FILE_ADDRESS address = pos2addr(pos, inside);
	if (address < 0)
		address = 0;
	else if (address > GetDocument()->CompLength())
		address = GetDocument()->CompLength();
	pos = addr2pos(address);
}

void CCompareView::DoScrollWindow(int xx, int yy)
{
	if (theApp.ruler_ && xx != 0)
	{
		// We need to scroll the ruler (as it's outside the scroll region)
		CRect rct;
		GetDisplayRect(&rct);
		rct.top = 0;
		rct.bottom = bdr_top_;
		ScrollWindow(xx, 0, &rct, &rct);
		// Also since we do not draw partial numbers at either end
		// we have to invalidate a bit more at either end than
		// is invalidated by ScrollWindow.
		if (xx > 0)
			rct.right = rct.left + xx + phev_->text_width_*3;
		else
			rct.left = rct.right + xx - phev_->text_width_*3;
		DoInvalidateRect(&rct);
	}
	CScrView::DoScrollWindow(xx, yy);
	if (yy < 0)
	{
		// We need to invalidate a bit of the address area near the top so that partial addresses are not drawn
		CRect rct;
		GetDisplayRect(&rct);
		rct.bottom = rct.top + phev_->line_height_;
		rct.top -= phev_->line_height_/4;
		rct.right = rct.left + addr_width_*phev_->text_width_;
		DoInvalidateRect(&rct);
	}
	else if (yy > 0)
	{
		// We need to invalidate a bit below the scrolled bit in the address area since
		// it may be blank when scrolling up (blank area avoids drawing partial address)
		CRect rct;
		GetDisplayRect(&rct);
		rct.top += yy;
		rct.bottom = rct.top + phev_->line_height_;
		rct.right = rct.left + addr_width_*phev_->text_width_;
		DoInvalidateRect(&rct);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCompareView message handlers

void CCompareView::OnSize(UINT nType, int cx, int cy)
{
	if (phev_ != NULL && (cx != 0 || cy != 0))
		phev_->recalc_display();     // this may result in it being called twice when using a splitter (unfortunate)

	CScrView::OnSize(nType, cx, cy);
}

BOOL CCompareView::OnEraseBkgnd(CDC* pDC)
{
	if (phev_ == NULL || phev_->bg_col_ == -1)
		return CScrView::OnEraseBkgnd(pDC);

	CRect rct;
	GetClientRect(rct);

	// Fill background with bg_col_
	CBrush backBrush;
	backBrush.CreateSolidBrush(phev_->bg_col_);
	backBrush.UnrealizeObject();
	pDC->FillRect(rct, &backBrush);

	// Get rect for address area
	rct.right = addr_width_*phev_->text_width_ - GetScroll().x - phev_->text_width_ + phev_->bdr_left_;

	// If address area is visible and address background is different to normal background ...
	if (rct.right > rct.left && phev_->addr_bg_col_ != phev_->bg_col_)
	{
		// Draw address background too
		CBrush addrBrush;
		addrBrush.CreateSolidBrush(phev_->addr_bg_col_);
		addrBrush.UnrealizeObject();
		pDC->FillRect(rct, &addrBrush);
	}
	if (theApp.ruler_ && phev_->addr_bg_col_ != phev_->bg_col_)
	{
		// Ruler background
		GetClientRect(rct);
		rct.bottom = phev_->bdr_top_ - 2;
		CBrush addrBrush;
		addrBrush.CreateSolidBrush(phev_->addr_bg_col_);
		addrBrush.UnrealizeObject();
		pDC->FillRect(rct, &addrBrush);
	}

	return TRUE;
}

void CCompareView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// We just need to handle a change in the selection
	CScrView::OnLButtonUp(nFlags, point);
	if (phev_->AutoSyncCompare())
	{
		FILE_ADDRESS start_addr, end_addr, hev_start, hev_end;
		GetSelAddr(start_addr, end_addr);
		phev_->GetSelAddr(hev_start, hev_end);
		if (start_addr == hev_start && end_addr == hev_end)
			return;
		// Make sure new selection is not past EOF of hex view file
		if (start_addr > GetDocument()->length()) start_addr = GetDocument()->length();
		if (end_addr > GetDocument()->length()) end_addr = GetDocument()->length();
		phev_->SetAutoSyncCompare(false);  // avoid inf. recursion
		phev_->MoveWithDesc("Compare Auto-sync", start_addr, end_addr);
		phev_->SetAutoSyncCompare(true);
	}
}

void CCompareView::OnSetFocus(CWnd* pNewWnd)
{
	CScrView::OnSetFocus(pNewWnd);
	if (phev_ == NULL || phev_->text_height_ == 0)
		return;

	// Invalidate the current selection so its drawn lighter in inactive window
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		++end_addr;   // if no selection invalidate current byte
	InvalidateRange(addr2pos(start_addr), addr2pos(end_addr));
}

void CCompareView::OnKillFocus(CWnd* pNewWnd)
{
	CScrView::OnKillFocus(pNewWnd);
	if (phev_ == NULL || phev_->text_height_ == 0)
		return;

	// Invalidate the current selection so its drawn lighter in inactive window
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		++end_addr;   // if no selection invalidate current byte
	InvalidateRange(addr2pos(start_addr), addr2pos(end_addr));
}

void CCompareView::OnMouseMove(UINT nFlags, CPoint point)
{
	CScrView::OnMouseMove(nFlags, point);
	//track_mouse(TME_HOVER);
}

LRESULT CCompareView::OnMouseHover(WPARAM, LPARAM lp)
{
	//track_mouse(TME_LEAVE);
	return 0;
}

LRESULT CCompareView::OnMouseLeave(WPARAM, LPARAM lp)
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CCompareView command handlers

// Command to go to first recent difference in compare view
void CCompareView::OnCompFirst()
{
	if (GetDocument()->CompareDifferences(0) > 0)
	{
		FILE_ADDRESS addr;
		int len;
		bool insert;
		GetDocument()->GetCompDiff(0, 0, addr, len, insert);
		MoveToAddress(addr, addr+len);
	}
}

void CCompareView::OnUpdateCompFirst(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->CompareDifferences(0) > 0);
}

// Command to go to previous recent difference in compare view
void CCompareView::OnCompPrev()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	int idx = GetDocument()->FirstDiffAt(false, 0, start - 1);
	if (idx > 0)
	{
		FILE_ADDRESS addr;
		int len;
		bool insert;
		GetDocument()->GetCompDiff(0, idx - 1, addr, len, insert);
		MoveToAddress(addr, addr+len);
	}
}

void CCompareView::OnUpdateCompPrev(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);
	pCmdUI->Enable(GetDocument()->CompareDifferences(0) >= 0 &&
				   GetDocument()->FirstDiffAt(false, 0, start - 1) > 0);
}

// Command to go to next recent difference in compare view
void CCompareView::OnCompNext()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	int idx = GetDocument()->FirstDiffAt(false, 0, start);
	if (idx < GetDocument()->CompareDifferences(0))
	{
		FILE_ADDRESS addr;
		int len;
		bool insert;
		GetDocument()->GetCompDiff(0, idx, addr, len, insert);
		MoveToAddress(addr, addr+len);
	}
}

void CCompareView::OnUpdateCompNext(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);
	pCmdUI->Enable(GetDocument()->CompareDifferences(0) >= 0 &&
				   GetDocument()->FirstDiffAt(false, 0, start) < 
							   GetDocument()->CompareDifferences(0));
}

// Command to go to last recent difference in compare view
void CCompareView::OnCompLast()
{
	int count = GetDocument()->CompareDifferences(0);
	if (count > 0)
	{
		FILE_ADDRESS addr;
		int len;
		bool insert;
		GetDocument()->GetCompDiff(0, count - 1, addr, len, insert);
		MoveToAddress(addr, addr+len);
	}
}

void CCompareView::OnUpdateCompLast(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->CompareDifferences(0) > 0);
}

