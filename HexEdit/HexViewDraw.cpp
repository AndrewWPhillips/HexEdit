// HexViewDraw.cpp : drawing code of CHexEditView class
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "HexEditDoc.h"
#include "HexEditView.h"

/////////////////////////////////////////////////////////////////////////////
// CHexEditView drawing

// There are several different coordinate systems used.
//  - device: pixels on the physical device (screen or printer)
//  - logical: units set by windows mapping mode
//             for screen MM_TEXT is used (= device coords) with Y axis down
//             for printer MM_HIMETRIC is used with Y axis up
//  - normalised: this is the same as logical but the sign is changed so
//             that Y axis is always down (and X axis is always right)
//             in order to simplify comparisons etc
//  - document: this is the normalised logical coord system with the origin at
//             the very top of the document.
// Note that different mapping modes are used for screen and printer.
//  MM_TEXT is used for screen otherwise scrolling becomes slightly
//  out of whack resulting in missing or extra lines of pixels.
//  MM_TEXT cannot be used for the printer as it comes out tiny on lasers.
//

void CHexEditView::OnDraw(CDC* pDC)
{
	if (pfont_ == NULL) return;

	CHexEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	/* This was to allow print preview to be grey when active printer was
	 * monochrome, but somehow it's already OK.
	bool preview =  pDC->GetDeviceCaps(NUMCOLORS) == 2 &&
					pDC->m_hDC != pDC->m_hAttribDC;
	 */

	pDC->SetBkMode(TRANSPARENT);

	// Are we in overtype mode and file is empty?
	if (display_.overtype && pDoc->length() == 0)
	{
		// Get the reason that there's no data from the document & display it
		CRect cli;
		GetDisplayRect(&cli);
		pDC->DPtoLP(&cli);
		if (pDC->IsPrinting())
		{
			if (cli.bottom < 0)
				cli.top = -2 * print_text_height_;
			else
				cli.top = 2 * print_text_height_;
		}
		const char *mm = pDoc->why0();
		pDC->SetTextColor(GetDefaultTextCol());
		pDC->DrawText(mm, -1, &cli, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		return;
	}

	const char *hex;
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	CRectAp doc_rect;                           // Display area relative to whole document
	CRect norm_rect;                            // Display area (norm. logical coords)
	// Note that norm_rect may extend slightly outside the physical view area, for example,
	// if only half a line is visible at the top of the screen.

	// These are the first and last "virtual" addresses of the top and (one past) the end
	// of the addresses within norm_rect.  Note that these are not necessarilly the first
	// and last real addresses.  For example, if offset_ != 0 and at the top of file then
	// first_virt will be -ve.  Similarly the file may not even be as long as last_virt.
	FILE_ADDRESS first_virt, last_virt;
	FILE_ADDRESS first_line;                    // First line that needs displaying
	FILE_ADDRESS last_line;                     // One past last line to display

	FILE_ADDRESS first_addr = 0;                // First address to actually display
	FILE_ADDRESS last_addr = pDoc->length();    // One past last address actually displayed

	FILE_ADDRESS line_inc;                      // 1 or -1 depending on draw dirn (up/down)
	CSize rect_inc;                             // How much to move norm_rect each time
	FILE_ADDRESS start_addr, end_addr;          // Address of current selection
	bool neg_x(false), neg_y(false);            // Does map mode have -ve to left or down
	bool has_focus = true;                      // Draw selection darker when we have focus
	int bitspixel = pDC->GetDeviceCaps(BITSPIXEL);
	if (bitspixel > 24) bitspixel = 24;         // 32 is really only 24 bits of colour
	long num_colours = 1L << (bitspixel*pDC->GetDeviceCaps(PLANES));

	int line_height, char_width, char_width_w;  // Text sizes

	ASSERT(offset_ >= 0 && offset_ < rowsize_);
	if (offset_ < 0 || offset_ >= rowsize_) offset_ = 0; // xxx kludge - need to track down why offset is wrong
	ASSERT(rowsize_ > 0 && rowsize_ <= max_buf);
	ASSERT(group_by_ > 0);

	if (pDC->IsPrinting())
	{
		// Work out "client" rect of a printer page
		norm_rect.top = norm_rect.left = 0;
		norm_rect.bottom = pDC->GetDeviceCaps(VERTRES);
		norm_rect.right  = pDC->GetDeviceCaps(HORZRES);

		// Convert to logical units but with origin at top left of window
		// Note we can't use ConvertFromDP here as this is for printer not screen
		pDC->DPtoLP(&norm_rect);
		if (norm_rect.right < 0)
		{
			neg_x = true;
			norm_rect.right = -norm_rect.right;
		}
		if (norm_rect.bottom < 0)
		{
			neg_y = true;
			norm_rect.bottom = -norm_rect.bottom;
		}

		// Text-only printer drivers under Win98 return number of text lines not pixels for VERTRES
		if (norm_rect.bottom < norm_rect.right/5)
			norm_rect.bottom *= print_text_height_;

		// Since there is no translation (only scaling) between device and logical coords origin does not change
		ASSERT(norm_rect.top == 0 && norm_rect.left == 0);

		// Work out which part of document to display
		if (display_.vert_display)
			doc_rect = CRectAp(norm_rect) + 
				CSizeAp(-margin_size_.cx, curpage_ * FILE_ADDRESS(lines_per_page_) * print_text_height_*3 - margin_size_.cy);
		else
			doc_rect = CRectAp(norm_rect) + 
				CSizeAp(-margin_size_.cx, curpage_ * FILE_ADDRESS(lines_per_page_) * print_text_height_ - margin_size_.cy);

		line_height = print_text_height_;
		if (display_.vert_display) line_height *= 3;
		char_width = print_text_width_;
		char_width_w = print_text_width_w_;
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
		norm_rect.left  = doc_rect.left  - GetScroll().x + bdr_left_;
		norm_rect.right = doc_rect.right - GetScroll().x + bdr_left_;
		norm_rect.top    = int(doc_rect.top    - GetScroll().y) + bdr_top_;
		norm_rect.bottom = int(doc_rect.bottom - GetScroll().y) + bdr_top_;

		// Get the current selection so that we can display it in reverse video
		GetSelAddr(start_addr, end_addr);

		line_height = line_height_;
		char_width = text_width_;
		char_width_w = text_width_w_;
	}

	// Get range of addresses that are visible the in window (overridden for printing below)
	first_virt = (doc_rect.top/line_height) * rowsize_ - offset_;
	last_virt  = (doc_rect.bottom/line_height + 1) * rowsize_ - offset_;

	// Work out which lines could possibly be in the display area
	if (pDC->IsPrinting() && print_sel_)
	{
		GetSelAddr(first_addr, last_addr);

		// Start drawing from start of selection (+ allow for subsequent pages)
		first_line = (first_addr+offset_)/rowsize_ + curpage_*FILE_ADDRESS(lines_per_page_);
		last_line = first_line + lines_per_page_;

		// Also if the selection ends on this page set the end
		if (last_line > (last_addr - 1 + offset_)/rowsize_ + 1)
			last_line = (last_addr - 1 + offset_)/rowsize_ + 1;

		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		first_virt = first_line * rowsize_ - offset_;
		last_virt  = first_virt + lines_per_page_*rowsize_;

		// Things are a bit different if merging duplicate lines
		if (dup_lines_)
		{
			if (curpage_ == 0)
				print_next_line_ = first_line;
			else
				first_line = print_next_line_;
			//  We don't know what the last line will be so set to end of selection
			last_line = (last_addr - 1 + offset_)/rowsize_ + 1;
			last_virt = last_line * rowsize_ - offset_;
		}

		/* Work out where to display the 1st line */
		norm_rect.top += margin_size_.cy;
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;

		doc_rect.top = first_line * line_height - margin_size_.cy;
		doc_rect.bottom = doc_rect.top + lines_per_page_*print_text_height_;
	}
	else if (pDC->IsPrinting())
	{
		// Draw just the lines on this page
		first_line = curpage_ * FILE_ADDRESS(lines_per_page_);
		last_line = first_line + lines_per_page_;
		first_addr = max(0, first_line*rowsize_ - offset_);
		last_addr = min(pDoc->length(), last_line*rowsize_ - offset_);

		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		first_virt = first_line * rowsize_ - offset_;
		last_virt  = first_virt + lines_per_page_*rowsize_;

		/* Work out where to display the 1st line */
//      norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.top += margin_size_.cy;
		norm_rect.bottom = norm_rect.top + line_height;
//        norm_rect.left +=  margin_size_.cx - doc_rect.left;
		norm_rect.left -= doc_rect.left;
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
	CPen pen1(PS_SOLID, 0, same_hue(sector_col_, 100, 30));    // dark sector_col_
	CPen pen2(PS_SOLID, 0, same_hue(addr_bg_col_, 100, 30));   // dark addr_bg_col_
	CPen *psaved_pen;
	CBrush brush(sector_col_);

	// Skip background drawing in this case because it's too hard.
	// Note that this goto greatly simplifies the tests below.
	if (pDC->IsPrinting() && print_sel_ && dup_lines_)
		goto end_of_background_drawing;

	// Preread device blocks so we know if there are bad sectors
	if (GetDocument()->IsDevice())
	{
		// As long as we ensure that the CFileNC buffer is at least as big as the display area (the
		// distance from start of top line to end of bottom line) then this should read all
		// sectors to be dipslayed.
		unsigned char cc;
		TRACE("---- Display size is %ld\n", long(last_addr - first_addr));
		pDoc->GetData(&cc, 1, (first_addr + last_addr)/2);
	}

	if (display_.borders)
		psaved_pen = pDC->SelectObject(&pen1);
	else
		psaved_pen = pDC->SelectObject(&pen2);

	// Column related screen stuff (ruler, vertical lines etc)
	if (!pDC->IsPrinting())
	{
		ASSERT(!neg_y && !neg_x);       // This should be true when drawing on screen (uses MM_TEXT)
		// Vert. line between address and hex areas
		pt.y = bdr_top_ - 4;
		pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
		pDC->MoveTo(pt);
		pt.y = 30000;
		pDC->LineTo(pt);
		if (!display_.vert_display && display_.hex_area)
		{
			// Vert line to right of hex area
			pt.y = bdr_top_ - 4;
			pt.x = char_pos(0, char_width) - char_width_w/2 - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (display_.vert_display || display_.char_area)
		{
			// Vert line to right of char area
			pt.y = bdr_top_ - 4;
			pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
				   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (theApp.ruler_)
		{
			int horz = bdr_left_ - GetScroll().x;       // Horiz. offset to window posn of left side

			ASSERT(bdr_top_ > 0);
			// Draw horiz line under ruler
			pt.y = bdr_top_ - 4;
			pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.x = 30000;
			pDC->LineTo(pt);

			// Draw ticks using hex offsets for major ticks (if using hex addresses) or
			// decimal offsets (if using decimal addresses/line numbers and/or hex addresses)
			int major = 1;
			if (display_.decimal_addr || display_.line_nums)
				major = theApp.ruler_dec_ticks_;        // decimal ruler or both hex and decimal
			else
				major = theApp.ruler_hex_ticks_;        // only showing hex ruler
			// Hex area ticks
			if (!display_.vert_display && display_.hex_area)
				for (int column = 1; column < rowsize_; ++column)
				{
					if ((!display_.decimal_addr && !display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = bdr_top_ - 5;
					pt.x = hex_pos(column) - char_width/2 + horz;
					if (column%group_by_ == 0)
						pt.x -= char_width/2;
					pDC->MoveTo(pt);
					pt.y -= (column%major) ? 3 : 7;
					pDC->LineTo(pt);
				}
			// Char area or stacked display ticks
			if (display_.vert_display || display_.char_area)
				for (int column = 0; column <= rowsize_; ++column)
				{
					if ((!display_.decimal_addr && !display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = bdr_top_ - 5;
					pt.x = char_pos(column) + horz;
					if (display_.vert_display && column%group_by_ == 0)
						if (column == rowsize_)    // skip last one
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
			int hicol = -1;                     // Column with cursor is to be highlighted

			bool hl_box = true;               // Highlight mouse/cursor in the ruler using box around col number (or just a small line)
			if (display_.hex_addr && theApp.ruler_hex_nums_ > 1) hl_box = false;
			if ((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1) hl_box = false;

			CRect hi_rect(-1, 0, -1, bdr_top_ - 4);
			if (!hl_box)
				hi_rect.top = hi_rect.bottom - 2;    // A flat rect just inside the ruler
			// Current caret position shown in the ruler
			if (theApp.hl_caret_ && !mouse_down_)
			{
				// Do highlighting with a background rectangle in ruler
				hicol = int((start_addr + offset_)%rowsize_);
				CBrush * psaved_brush = pDC->SelectObject(&brush);
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				if (!display_.vert_display && display_.hex_area)
				{
					hi_rect.left  = hex_pos(hicol) + horz;
					hi_rect.right = hi_rect.left + text_width_*2 + 1;
					pDC->Rectangle(&hi_rect);
				}
				if (display_.vert_display || display_.char_area)
				{
					hi_rect.left  = char_pos(hicol) + horz;
					hi_rect.right = hi_rect.left + text_width_ + 1;
					pDC->Rectangle(&hi_rect);
				}
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}
			// Current mouse position in the ruler
			if (theApp.hl_mouse_ && mouse_addr_ > -1)
			{
				int mousecol = int((mouse_addr_ + offset_)%rowsize_);   // Mouse column to be highlighted
				CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				if (!display_.vert_display && display_.hex_area)
				{
					hi_rect.left  = hex_pos(mousecol) + horz;
					hi_rect.right = hi_rect.left + text_width_*2 + 1;
					pDC->Rectangle(&hi_rect);
				}
				if (display_.vert_display || display_.char_area)
				{
					hi_rect.left  = char_pos(mousecol) + horz;
					hi_rect.right = hi_rect.left + text_width_ + 1;
					pDC->Rectangle(&hi_rect);
				}
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			// Get display rect for clipping at right and left
			CRect cli;
			GetDisplayRect(&cli);

			// Show hex offsets in the top border (ruler)
			if (display_.hex_addr)
			{
				bool between = theApp.ruler_hex_nums_ > 1;      // Only display numbers above cols if displaying for every column

				// Do hex numbers in ruler
				CRect rect(-1, vert, -1, vert + text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetHexAddrCol());   // Colour of hex addresses

				// Show hex offsets above hex area
				if (!display_.vert_display && display_.hex_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // usee doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;
						if (!between)
						{
							// Draw 2 digit number above every column
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", (column + display_.addrbase1)%256);
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
				if (display_.vert_display || display_.char_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (display_.vert_display && column > 0 && column%group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;

						if (!between)
						{
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", (column + display_.addrbase1)%16);
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
				vert += text_height_;  // Move down for anything to be drawn underneath
			}
			// Show decimal offsets in the ruler
			if (display_.decimal_addr || display_.line_nums)
			{
				bool between = theApp.ruler_dec_nums_ > 1;      // Only display numbers above cols if displaying for every column

				CRect rect(-1, vert, -1, vert + text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses

				// Decimal offsets above hex area
				if (!display_.vert_display && display_.hex_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left  = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;
						if (!between)
						{
							ss.Format("%02d", (column + display_.addrbase1)%100);
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
				if (display_.vert_display || display_.char_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (display_.vert_display && column > 0 && column%group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;

						if (!between)
						{
							ss.Format("%1d", (column + display_.addrbase1)%10);
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
				vert += text_height_;   // Move down for anything to be drawn underneath (currently nothing)
			}
#ifdef RULER_ADJUST
			draw_adjusters(pDC);           // Draw adjustment controls in the ruler
#endif
		} // end ruler drawing
		if (real_offset_ > 0 && real_offset_ >= first_virt && real_offset_ < last_virt)
		{
			// Horizontal line at offset
			pt.y = int(((real_offset_ - 1)/rowsize_ + 1) * line_height - doc_rect.top + bdr_top_);
			if (neg_y) pt.y = -pt.y;
			pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			//pt.x = 30000;
			if (!display_.vert_display && display_.hex_area && !display_.char_area)
				pt.x = char_pos(0, char_width) - char_width_w/2 - doc_rect.left + bdr_left_;
			else
				pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
					   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
			pDC->LineTo(pt);
		}
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
		rct.left = (addr_width_ - 1)*char_width + norm_rect.left + bdr_left_;
		pDC->ExcludeClipRect(&rct);
	}
#endif

	// Note: We draw bad sectors, change-tracking deletions, etc first
	// as they are always drawn from the top of screen (or page) downwards.
	// This is so the user is less likely to notice the wrong direction.
	// (Major things are drawn from the bottom up when scrolling backwards.)

	// First decide is we need to draw sector borders
	int seclen = pDoc->GetSectorSize();
	bool draw_borders = pDoc->pfile1_ != NULL && display_.borders && seclen > 0;
	if (pDC->IsPrinting() && !theApp.print_sectors_)
		draw_borders = false;

	if (draw_borders)
	{
		ASSERT(seclen > 0);
		bool prev_bad = first_addr == 0;                  // don't display sector separator above top of file

		for (FILE_ADDRESS sector = (first_addr/seclen)*seclen; sector < last_addr; sector += seclen)
		{
			// Note that "sector" is the address of the start of the sector
			if (pDoc->HasSectorErrors() && pDoc->SectorError(sector/seclen) != NO_ERROR)
			{
				// Draw colour behind the bytes to indicate there is a problem with this sector
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, sector_bg_col_,
						max(sector, first_addr),
						min(sector + seclen, last_addr));
				prev_bad = true;
			}
			else if (!prev_bad && sector >= first_addr && sector < last_addr)
			{
				// Just draw a line above the top of the sector
				if (!display_.vert_display && display_.hex_area)
				{
					// Hex area
					pt.y = int(((sector + offset_)/rowsize_) * line_height -
							   doc_rect.top + bdr_top_);     // This is just above the first byte of the sector
					if (neg_y) pt.y = -pt.y;

					//pt.x = hex_pos(rowsize_ - 1, char_width) + 2*char_width - doc_rect.left + bdr_left_;
					pt.x = char_pos(0, char_width) - char_width/2 - doc_rect.left + bdr_left_;  // Right side of hex area
					if (neg_x) pt.x = - pt.x;
					pDC->MoveTo(pt);
					pt.x = hex_pos(int((sector + offset_)%rowsize_), char_width) - 
						   char_width/2 - doc_rect.left + bdr_left_;  // This is just to left of first byte of sector
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);

					if ((sector + offset_)%rowsize_ != 0)
					{
						// Draw on line below and vertical bit too
						pt.y = int(((sector + offset_)/rowsize_ + 1) * line_height - 
								   doc_rect.top + bdr_top_);
						if (neg_y) pt.y = -pt.y;
						pDC->LineTo(pt);
						pt.x = hex_pos(0, char_width) - doc_rect.left + bdr_left_;
						if (neg_x) pt.x = - pt.x;
						pDC->LineTo(pt);
					}
					pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
					pDC->LineTo(pt);
				}
				if (display_.vert_display || display_.char_area)
				{
					// Do char area (or stacked mode)
					pt.y = int(((sector + offset_)/rowsize_) * line_height -
							   doc_rect.top + bdr_top_);
					if (neg_y) pt.y = -pt.y;

					//pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + char_width_w - doc_rect.left + bdr_left_;
					pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
						   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->MoveTo(pt);
					pt.x = char_pos(int((sector + offset_)%rowsize_), char_width, char_width_w) -
						   doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);

					if ((sector + offset_)%rowsize_ != 0)
					{
						// Draw on line below and vertical bit too
						pt.y = int(((sector + offset_)/rowsize_ + 1) * line_height -
								   doc_rect.top + bdr_top_);
						if (neg_y) pt.y = -pt.y;
						pDC->LineTo(pt);
						pt.x = char_pos(0, char_width, char_width_w) - doc_rect.left + bdr_left_;
						if (neg_x) pt.x = - pt.x;
						pDC->LineTo(pt);
					}
					// Fill in a little bit to join with the vertical line on the left
					if (display_.vert_display || !display_.hex_area)
						pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
					else
						pt.x = char_pos(0, char_width, char_width_w) -
							   char_width_w/2 - doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);
				}
				// Draw a little bit more in the gap between address area and left side
				pt.x = addr_width_*char_width - char_width/2 - doc_rect.left + bdr_left_;
				if (neg_x) pt.x = - pt.x;
				pDC->MoveTo(pt);
				pt.x = addr_width_*char_width + char_width/8 -  // Extra char_width/8 due to one pixel out on screen (and many on printer)
					   doc_rect.left + bdr_left_;
				if (neg_x) pt.x = - pt.x;
				pDC->LineTo(pt);
			}
			else
			{
				prev_bad = false;  // don't draw border at bottom of previous block fill (just remember this one was OK)
			}
		}
	}
	pDC->SelectObject(psaved_pen);      // restore pen after drawing borders etc

	// Draw bookmarks (and mark next) before highlights etc as they are drawn solid not transparent.
	// [Don't print bookmarks if hide_bookmarks is on OR printing and print_bookmarks_ is off]
	if (!(display_.hide_bookmarks || pDC->IsPrinting() && !theApp.print_bookmarks_))
	{
		// Draw bookmarks
		for (std::vector<FILE_ADDRESS>::const_iterator pbm = pDoc->bm_posn_.begin();
			 pbm != pDoc->bm_posn_.end(); ++pbm)
		{
			if (*pbm >= first_addr && *pbm <= last_addr)
			{
				CRect mark_rect;

				mark_rect.top = int(((*pbm + offset_)/rowsize_) * line_height - 
									doc_rect.top + bdr_top_ + 1);
//                mark_rect.bottom = mark_rect.top + line_height - 3;
				mark_rect.bottom = mark_rect.top + line_height - 1;
				if (neg_y)
				{
					mark_rect.top = -mark_rect.top;
					mark_rect.bottom = -mark_rect.bottom;
				}

				if (!display_.vert_display && display_.hex_area)
				{
					mark_rect.left = hex_pos(int((*pbm + offset_)%rowsize_), char_width) - 
									doc_rect.left + bdr_left_;
//                        mark_rect.right = mark_rect.left + 2*char_width;
					mark_rect.right = mark_rect.left + 2*char_width + 2;
					if (neg_x)
					{
						mark_rect.left = -mark_rect.left;
						mark_rect.right = -mark_rect.right;
					}

					pDC->FillSolidRect(&mark_rect, bm_col_);
				}

				if (display_.vert_display || display_.char_area)
				{
					mark_rect.left = char_pos(int((*pbm + offset_)%rowsize_), char_width, char_width_w) - 
									doc_rect.left  + bdr_left_ + 1;
//                        mark_rect.right = mark_rect.left + char_width_w - 2;
					mark_rect.right = mark_rect.left + char_width_w;
					if (neg_x)
					{
						mark_rect.left = -mark_rect.left;
						mark_rect.right = -mark_rect.right;
					}
					pDC->FillSolidRect(&mark_rect, bm_col_);
				}
			}
		}
	}

	// First work out if we draw the mark - in display and not turned off for printing
	bool draw_mark;
	if (pDC->IsPrinting())
		draw_mark = theApp.print_mark_ && mark_ >= first_virt && mark_ < last_virt;
	else
		draw_mark = mark_ >= first_addr && mark_ <= last_addr;
	if (draw_mark)
	{
		CRect mark_rect;                // Where mark is drawn in logical coords

		mark_rect.top = int(((mark_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
//        mark_rect.bottom = mark_rect.top + line_height - 3;
		mark_rect.bottom = mark_rect.top + line_height - 1;
		if (neg_y)
		{
			mark_rect.top = -mark_rect.top;
			mark_rect.bottom = -mark_rect.bottom;
		}

		if (!display_.vert_display && display_.hex_area)
		{
			mark_rect.left = hex_pos(int((mark_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
//            mark_rect.right = mark_rect.left + 2*char_width;
			mark_rect.right = mark_rect.left + 2*char_width + 2;
			if (neg_x)
			{
				mark_rect.left = -mark_rect.left;
				mark_rect.right = -mark_rect.right;
			}

			pDC->FillSolidRect(&mark_rect, mark_col_);
		}

		if (display_.vert_display || display_.char_area)
		{
			mark_rect.left = char_pos(int((mark_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
//            mark_rect.right = mark_rect.left + char_width_w - 2;
			mark_rect.right = mark_rect.left + char_width_w;
			if (neg_x)
			{
				mark_rect.left = -mark_rect.left;
				mark_rect.right = -mark_rect.right;
			}
			pDC->FillSolidRect(&mark_rect, mark_col_);
		}
	}

	// Draw indicator around byte that is used for info tips
	if (!pDC->IsPrinting() && last_tip_addr_ > -1 && last_tip_addr_ >= first_virt && last_tip_addr_ < last_virt)
	{
		CRect info_rect;                // Where mark is drawn in logical coords

		info_rect.top = int(((last_tip_addr_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
		info_rect.bottom = info_rect.top + line_height - 1;
		if (neg_y)
		{
			info_rect.top = -info_rect.top;
			info_rect.bottom = -info_rect.bottom;
		}

		CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
		CPen * psaved_pen = pDC->SelectObject(&pen1);
		if (!display_.vert_display && display_.hex_area)
		{
			info_rect.left = hex_pos(int((last_tip_addr_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
			info_rect.right = info_rect.left + 2*char_width + 1;
			if (neg_x)
			{
				info_rect.left = -info_rect.left;
				info_rect.right = -info_rect.right;
			}

			//pDC->FillSolidRect(&info_rect, sector_bg_col_);
			pDC->Rectangle(&info_rect);
		}

		if (display_.vert_display || display_.char_area)
		{
			info_rect.left = char_pos(int((last_tip_addr_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
			info_rect.right = info_rect.left + char_width_w - 1;
			if (neg_x)
			{
				info_rect.left = -info_rect.left;
				info_rect.right = -info_rect.right;
			}
			//pDC->FillSolidRect(&info_rect, sector_bg_col_);
			pDC->Rectangle(&info_rect);
		}
		(void)pDC->SelectObject(psaved_pen);
		(void)pDC->SelectObject(psaved_brush);
	}

	// Draw indicator around byte where bookmark is being moved to
	if (!pDC->IsPrinting() && mouse_down_ && (drag_mark_ || drag_bookmark_ > -1))
	{
		CRect drag_rect;                // Where mark is drawn in logical coords

		drag_rect.top = int(((drag_address_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
		drag_rect.bottom = drag_rect.top + line_height - 2;
		if (neg_y)
		{
			drag_rect.top = -drag_rect.top;
			drag_rect.bottom = -drag_rect.bottom;
		}

		CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
		CPen * psaved_pen = pDC->SelectObject(&pen1);
		if (!display_.vert_display && display_.hex_area)
		{
			drag_rect.left = hex_pos(int((drag_address_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
			drag_rect.right = drag_rect.left + 2*char_width;
			if (neg_x)
			{
				drag_rect.left = -drag_rect.left;
				drag_rect.right = -drag_rect.right;
			}

			pDC->FillSolidRect(&drag_rect, drag_mark_ ? mark_col_ : bm_col_);
			pDC->Rectangle(&drag_rect);
		}

		if (display_.vert_display || display_.char_area)
		{
			drag_rect.left = char_pos(int((drag_address_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
			drag_rect.right = drag_rect.left + char_width_w - 1;
			if (neg_x)
			{
				drag_rect.left = -drag_rect.left;
				drag_rect.right = -drag_rect.right;
			}
			pDC->FillSolidRect(&drag_rect, drag_mark_ ? mark_col_ : bm_col_);
			pDC->Rectangle(&drag_rect);
		}
		(void)pDC->SelectObject(psaved_pen);
		(void)pDC->SelectObject(psaved_brush);
	}

	// ---------------------------------------------------------
	// Things which are "merged" (don't completely obscure what has already been drawn) are drawn last

	// Change tracking (deletions, insertions, replacements)
	// Note that these are *not* drawn/printed if:
	//  - during printing and global print_change_ settings is off
	//  - deletions are not shown if view's hide_delete option is on
	//  - insertions are not shown if view's hide_insert option is on
	//  - replacements are not shown if view's hide_replace option is on
	if (!pDC->IsPrinting() || theApp.print_change_)
	{
		// Draw deletions unless hidden
		if (!display_.hide_delete)
		{
			pair<vector<FILE_ADDRESS> *, vector<FILE_ADDRESS> *> alp = GetDocument()->Deletions();
			draw_deletions(pDC, *alp.first, *alp.second,
						   first_virt, last_virt, doc_rect, neg_x, neg_y,
						   line_height, char_width, char_width_w, trk_col_);
		}

		// Draw insertions unless hidden
		if (!display_.hide_insert)
		{
			pair<vector<FILE_ADDRESS> *, vector<FILE_ADDRESS> *> alp = GetDocument()->Insertions();
			draw_backgrounds(pDC, *alp.first, *alp.second,
							 first_virt, last_virt, doc_rect, neg_x, neg_y,
							 line_height, char_width, char_width_w, trk_bg_col_);
		}

		// Draw replacements unless hidden
		if (!display_.hide_replace)
		{
			pair<vector<FILE_ADDRESS> *, vector<FILE_ADDRESS> *> alp = GetDocument()->Replacements();
			draw_backgrounds(pDC, *alp.first, *alp.second,
							 first_virt, last_virt, doc_rect, neg_x, neg_y,
							 line_height, char_width, char_width_w,	trk_col_,
							 true, (pDC->IsPrinting() ? print_text_height_ : text_height_)/8);
		}
	}

	if (!((GetDocument()->CompareDifferences() <= 0) || pDC->IsPrinting() && !theApp.print_compare_))
	{
		CSingleLock sl(&(GetDocument()->docdata_), TRUE); // Protect shared data access to the returned vectors

		// xxx just draw revision 0 for now (other revisions are for self-coompare)
		pair<const vector<FILE_ADDRESS> *, const vector<FILE_ADDRESS> *> alp = GetDocument()->OrigDeletions();
		draw_deletions(pDC, *alp.first, *alp.second,
						first_virt, last_virt, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, comp_col_);

		alp = GetDocument()->OrigInsertions();
		draw_backgrounds(pDC, *alp.first, *alp.second,
							first_virt, last_virt, doc_rect, neg_x, neg_y,
							line_height, char_width, char_width_w, comp_bg_col_);

		alp = GetDocument()->OrigReplacements();
		draw_backgrounds(pDC, *alp.first, *alp.second,
							first_virt, last_virt, doc_rect, neg_x, neg_y,
							line_height, char_width, char_width_w,	comp_col_,
							true, (pDC->IsPrinting() ? print_text_height_ : text_height_)/8);
	}

	// Don't print highlights if hide_highlight is on OR printing and print_highlights_ is off
	if (!(display_.hide_highlight || pDC->IsPrinting() && !theApp.print_highlights_))
	{
		if (!pDC->IsPrinting() && ScrollUp())
		{
			// Draw highlighted areas bottom up
			range_set<FILE_ADDRESS>::range_t::reverse_iterator pr;
			for (pr = hl_set_.range_.rbegin(); pr != hl_set_.range_.rend(); ++pr)
				if (pr->sfirst < last_addr)
					break;
			for ( ; pr != hl_set_.range_.rend(); ++pr)
			{
				if (pr->slast <= first_addr)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, hi_col_,
						max(pr->sfirst, first_addr), 
						min(pr->slast, last_addr));
			}
		}
		else
		{
			// Draw highlighted areas top down
			range_set<FILE_ADDRESS>::range_t::const_iterator pr;
			for (pr = hl_set_.range_.begin(); pr != hl_set_.range_.end(); ++pr)
				if (pr->slast > first_addr)
					break;
			for ( ; pr != hl_set_.range_.end(); ++pr)
			{
				if (pr->sfirst > last_addr)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, hi_col_,
						max(pr->sfirst, first_addr), 
						min(pr->slast, last_addr));
			}
		}
	}

	if (pDC->IsPrinting())
	{
		// Only print search occurrences if print_search_ is on
		if (theApp.print_search_)
		{
			// Draw search string occurrences
			// Note this goes through all search occurrences (since search_pair_ is
			// calculated for the current window) which may be slow but then so is printing.
			std::vector<FILE_ADDRESS> sf = GetDocument()->SearchAddresses(first_addr-search_length_, last_addr+search_length_);
			std::vector<FILE_ADDRESS>::const_iterator pp;

			for (pp = sf.begin(); pp != sf.end(); ++pp)
			{
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, search_col_,
						max(*pp, first_addr), 
						min(*pp + search_length_, last_addr));
			}
		}
	}
	else if (ScrollUp())
	{
		// Draw search string occurrences bottom up
		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::reverse_iterator pp, pend;
		for (pp = search_pair_.rbegin(), pend = search_pair_.rend(); pp != pend; ++pp)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, search_col_,
					max(pp->first, first_addr), 
					min(pp->second, last_addr));
		}

		// Draw template field backgrounds bottom up
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> >::reverse_iterator pdffd, penddffd;
		for (pdffd = dffd_bg_.rbegin(), penddffd = dffd_bg_.rend(); pdffd != penddffd; ++pdffd)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
				line_height, char_width, char_width_w, pdffd->get<2>(),
				max(pdffd->get<0>(), first_addr), 
				min(pdffd->get<1>(), last_addr));
		}
	}
	else
	{
		// Draw search string occurrences from top down
		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp, pend;
		for (pp = search_pair_.begin(), pend = search_pair_.end(); pp != pend; ++pp)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, search_col_,
					max(pp->first, first_addr), 
					min(pp->second, last_addr));
		}

		// Draw template field backgrounds from top down
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> >::const_iterator pdffd, penddffd;
		for (pdffd = dffd_bg_.begin(), penddffd = dffd_bg_.end(); pdffd != penddffd; ++pdffd)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
				line_height, char_width, char_width_w, pdffd->get<2>(),
				max(pdffd->get<0>(), first_addr), 
				min(pdffd->get<1>(), last_addr));
		}
	}

end_of_background_drawing:

	unsigned char buf[max_buf];              // Holds bytes for current line being displayed
	size_t last_col = 0;                     // Number of bytes in buf to display
	unsigned char prev_buf[max_buf];         // Copy of last buf - used for merging repeated lines
	size_t prev_last_col;                    // Number of bytes in last buf that were displayed
	int repeat_count = 0;                    // Number of consec. duplicate lines found so far

	// Move declarations outside loop (faster?)
	CString ss(' ', 24);                     // Temp string for formatting
	CRect tt;                                // Temp rect
	CRect addr_rect;                         // Where address is drawn

	// This was added for vert_display
	int vert_offset = 0;
	if (display_.vert_display)
	{
		if (pDC->IsPrinting())
			vert_offset = print_text_height_;
		else
			vert_offset = text_height_;
		if (neg_y)
			vert_offset = - vert_offset;
//      vert_offset = (vert_offset*15)/16;  // Leave a gap between rows
	}

	// This was added for codepages
	std::vector<FILE_ADDRESS> cp_first;
	if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ > 1)
		cp_first = codepage_startchar(first_addr, last_addr);

	// THIS IS WHERE THE ACTUAL LINES ARE DRAWN
	// Note: we use != (line != last_line) since we may be drawing from bottom or top
	FILE_ADDRESS line;
	for (line = first_line; line != last_line;
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
		if (!pDC->RectVisible(&tt) || line*rowsize_ - offset_ > pDoc->length())
			continue;

		// Take a copy of the last line output to check for repeated lines
		if (pDC->IsPrinting() && print_sel_ && dup_lines_ && last_col > 0)
			memcpy(prev_buf, buf, last_col);
		prev_last_col = last_col;

		// Get the bytes to display
		size_t ii;                      // Column of first byte
		const int extra_bytes = 8;      // we need to read extra bytes past the end of the line for displaying MBCS characters

		if (line*rowsize_ - offset_ < first_addr)
		{
			last_col = pDoc->GetData(buf + offset_, rowsize_ - offset_ + extra_bytes, line*rowsize_) +
						offset_;
			ii = size_t(first_addr - (line*rowsize_ - offset_));
			ASSERT(int(ii) < rowsize_);
		}
		else
		{
			last_col = pDoc->GetData(buf, rowsize_ + extra_bytes, line*rowsize_ - offset_);
			ii = 0;
		}
		if (last_col > rowsize_) last_col = rowsize_;  // Don't let extra_bytes affect number of columns to display

		if (line*rowsize_ - offset_ + last_col - last_addr >= rowsize_)
			last_col = 0;
		else if (line*rowsize_ - offset_ + last_col > last_addr)
			last_col = size_t(last_addr - (line*rowsize_ - offset_));

//        TRACE("xxx line %ld rowsize_ %ld offset_ %ld last_col %ld first_addr %ld last_addr %ld\n",
//              long(line), long(rowsize_), long(offset_), long(last_col), long(first_addr), long(last_addr));

		if (pDC->IsPrinting() && print_sel_ && dup_lines_)
		{
			// Check if the line is the same as the last one
			// But NOT if very first line (line 0 on page 0) since we may not display the whole line
			if ((curpage_ > 0 || line > first_line+1) &&
				last_col == prev_last_col && memcmp(buf, prev_buf, last_col) == 0)
			{
				++repeat_count;
				norm_rect -= rect_inc;
				continue;
			}

			if (repeat_count > 0)
			{
				// Display how many times the line was repeated
				CString mess;
				if (repeat_count == 1)
					mess = "Repeated once";
				else
					mess.Format("Repeated %d more times", repeat_count);
				CRect mess_rect = tt;
				mess_rect.left += hex_pos(0, char_width);
				pDC->DrawText(mess, &mess_rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);

				// Move the current display line down one
				norm_rect += rect_inc;
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

				// Reset the repeated line counter since this was a different line
				repeat_count = 0;
			}

			// See if we are at the bottom of the page yet
			if (norm_rect.bottom > margin_size_.cy + print_text_height_*lines_per_page_)
			{
				print_next_line_ = line;
				break;
			}
		}

		// Draw address if ...
		if ((addr_width_ - 1)*char_width + tt.left > 0 &&    // not off to the left
			(tt.top + text_height_/4 >= bdr_top_ || pDC->IsPrinting()))   // and does not encroach into ruler
		{
			addr_rect = tt;            // tt with right margin where addresses end
			addr_rect.right = addr_rect.left + addr_width_*char_width - char_width - 1;
			if (pDC->IsPrinting())
				if (neg_y)
					addr_rect.bottom = addr_rect.top - print_text_height_;
				else
					addr_rect.bottom = addr_rect.top + print_text_height_;
			else
				if (neg_y)
					addr_rect.bottom = addr_rect.top - text_height_;
				else
					addr_rect.bottom = addr_rect.top + text_height_;
			// Not highlighting when the mouse is down avoids a problem with invalidation of
			// the address area when autoscrolling (old highlights sometimes left behind).
			if (theApp.hl_caret_ && !pDC->IsPrinting() && !mouse_down_ && start_addr >= line*rowsize_ - offset_ && start_addr < (line+1)*rowsize_ - offset_)
			{
				CBrush * psaved_brush = pDC->SelectObject(&brush);
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				pDC->Rectangle(&addr_rect);
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			// Show address of current row with a different background colour
			if (theApp.hl_mouse_ && !pDC->IsPrinting() && mouse_addr_ >= line*rowsize_ - offset_ && mouse_addr_ < (line+1)*rowsize_ - offset_)
			{
				CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				pDC->Rectangle(&addr_rect);
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			if (hex_width_ > 0)
			{
				int ww = hex_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
					theApp.hex_ucase_ ? "%0*I64X:" : "%0*I64x:",
						hex_width_,
						(line*rowsize_ - offset_ > first_addr ? line*rowsize_ - offset_ : first_addr) + display_.addrbase1);
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
						(line*rowsize_ - offset_ > first_addr ? line*rowsize_ - offset_ : first_addr) + display_.addrbase1);
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
						line + display_.addrbase1);
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

		wchar_t dot = 0x00B7;    // Unicode char for middle dot
		int dot_pad = 0, cont_pad = 0, bad_pad = 0;
		if (text_width_w_ > text_width_)
		{
			CSize size;
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &dot, 1, &size);
			dot_pad = (char_width_w - size.cx)/2;

			wchar_t wc = *ContChar();
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
			cont_pad = (char_width_w - size.cx)/2;

			wc = *BadChar();
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
			bad_pad = (char_width_w - size.cx)/2;
		}
		int idx = -1;                      // Index into cp_first of the line we are doing
		int start_col = -1;                // Column of the next start char (not continuation char) in MBCS

		if (cp_first.size() > 0)
		{
			idx = int(line - doc_rect.top/line_height_);
			ASSERT(cp_first[idx] >= line*rowsize_ - offset_ && cp_first[idx] < (line+1)*rowsize_ - offset_ + 8 ||
			       idx == cp_first.size() - 1 && cp_first[idx] == -1);
			start_col = int(cp_first[idx] - line*rowsize_ - offset_);
		}

		// Keep track of the current colour so we only set it when it changes
		COLORREF current_colour = kala[buf[ii]];
		pDC->SetTextColor(current_colour);
		if (display_.vert_display || display_.hex_area)
		{
			int posx = tt.left + hex_pos(0, char_width);                 // Horiz pos of 1st hex column

			// Display each byte as hex (and char if nec.)
			for (size_t jj = ii ; jj < last_col; ++jj)
			{
				if (display_.vert_display)
				{
					if (posx + int(jj + 1 + jj/group_by_)*char_width_w < 0)
						continue;
					else if (posx + int(jj + jj/group_by_)*char_width_w >= tt.right)
						break;
				}
				else
				{
					if (posx + int((jj+1)*3 + jj/group_by_)*char_width < 0)
						continue;
					else if (posx + int(jj*3 + jj/group_by_)*char_width >= tt.right)
						break;
				}

				if (current_colour != kala[buf[jj]])
				{
					current_colour = kala[buf[jj]];
					pDC->SetTextColor(current_colour);
				}

				if (display_.vert_display)
				{
					// Display byte in char display area (as ASCII, EBCDIC etc)
					if (display_.char_set == CHARSET_CODEPAGE &&
						cp_first.size() > 0 &&
						jj < start_col)
					{
						// Continuation byte of MBCS character
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + cont_pad, tt.top, ContChar(), 1);
					}
					else if (buf[jj] < 32 && display_.char_set != CHARSET_EBCDIC && display_.char_set != CHARSET_OEM)
					{
						// Control characters are diplayed the same for all char sets except for 
						//  - EBCDIC (some chars < 32 may not be graphical) and 
						//  - OEM (there are graphic characters for chars < 32)
						if (display_.control == 0)
						{
							// Display control char and other chars as a dot (normally in red)
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}
						else if (display_.control == 1)
						{
							// Display control chars as red uppercase equiv.
							char cc = buf[jj] + 0x40;
							pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, &cc, 1);
						}
						else if (display_.control == 2)
						{
							// Display control chars as C escape code (in red)
							const char *check = "\a\b\f\n\r\t\v\0";
							const char *display = "abfnrtv0";
							const char *pp;
							if ((pp = strchr(check, buf[jj])) != NULL)
								pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, display + (pp-check), 1);
							else
								::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}

						// For MBCS char sets we need to keep track that we have done this character
						if (display_.char_set == CHARSET_CODEPAGE && cp_first.size() > 0)
							++start_col;
					}
					else if (display_.char_set == CHARSET_CODEPAGE && code_page_ == CP_UTF8)
					{
						// Handle UTF 8 specially since it is not like other Windows code pages
						ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 0);
						if ((buf[jj] & 0xC0) == 0x80)
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + cont_pad, tt.top, ContChar(), 1);
						else
						{
							// The top 2 bits determine how many following (continuation) bytes there are.  If top bit
							// is zero then len == 1 (no cont bytes) since it is ASCII
							size_t len;
							if ((buf[jj] & 0x80) == 0x00)
								len = 1;
							else if ((buf[jj] & 0xE0) == 0xC0)
								len = 2;
							else if ((buf[jj] & 0xF0) == 0xE0)
								len = 3;
							else if ((buf[jj] & 0xF8) == 0xF0)
								len = 4;
							else if ((buf[jj] & 0xFC) == 0xF8)
								len = 5;
							else if ((buf[jj] & 0xFE) == 0xFC)
								len = 6;
							else
								len = 0;   // invalid UTF-8

							wchar_t wc;                               // equivalent Unicode (UTF-16) character
							if (len == 0 || 
								MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], len, &wc, 1) == 0 ||
								wc >= 0xE000 && wc < 0xF900)
							{
								// Character translation returned no bytes or Unicode value in "Private Use Area"
								wc = *BadChar();
							}

							CSize size;
							::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
						}
					}
					else if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ <= 1)
					{
						// Handle single byte code page
						ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 1);
						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], 1, &wc, 1) != 1 ||
							wc >= 0xE000 && wc < 0xF900)
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + bad_pad, tt.top, BadChar(), 1);
							wc = *BadChar();
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
					else if (display_.char_set == CHARSET_CODEPAGE)
					{
						// Handle MBCS code pages
						ASSERT(cp_first.size() > 0 && jj == start_col);
						size_t len = CharNextExA(code_page_, (const char *)&buf[jj], 0) - (char *)&buf[jj];
						// Display the multibyte character that starts at this byte then find the start of the next one
						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], len, &wc, 1) != 1 ||
							wc >= 0xE000 && wc < 0xF900)
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							wc = *BadChar();
							++start_col;
						}
						else
						{
							// Character translation was good so ouput the character
							start_col += len;
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
					else if (display_.char_set == CHARSET_EBCDIC)
					{
						// Display EBCDIC (or red dot if not valid EBCDIC char)
						if (e2a_tab[buf[jj]] == '\0')
						{
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}
						else
						{
							pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, (char *)&e2a_tab[buf[jj]], 1);
						}
					}
					else if (display_.char_set == CHARSET_ASCII && buf[jj] < 127 ||
							 display_.char_set != CHARSET_ASCII && buf[jj] >= first_char_ && buf[jj] <= last_char_)
					{
						// Display normal char or graphic char if in font
						pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, (char *)&buf[jj], 1);
					}
					else
					{
						// Display chars "out of range" as dots
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						//::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + bad_pad, tt.top, BadChar(), 1);
  					}

					// Display the hex digits below that, one below the other
					pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top + vert_offset,   &hex[(buf[jj]>>4)&0xF], 1);
					pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top + vert_offset*2, &hex[buf[jj]&0xF], 1);
				}
				else
				{
					char hh[2];

					// Create hex digits and display them
					hh[0] = hex[(buf[jj]>>4)&0xF];
					hh[1] = hex[buf[jj]&0xF];

					// This actually displays the bytes (in hex)!
					// Note: removed calcs that were previously encapsulated in hex_pos
					pDC->TextOut(posx + (jj*3 + jj/group_by_)*char_width, tt.top, hh, 2);
				}
			}
		}

		if (!display_.vert_display && display_.char_area)
		{
			int posc = tt.left + char_pos(0, char_width, char_width_w);  // Horiz pos of 1st char column

			for (size_t kk = ii ; kk < last_col; ++kk)
			{
				if (posc + int(kk+1)*char_width_w < 0)
					continue;
				else if (posc + int(kk)*char_width_w >= tt.right)
					break;

				if (current_colour != kala[buf[kk]])
				{
					current_colour = kala[buf[kk]];
					pDC->SetTextColor(current_colour);
				}

				// Display byte in the char (right) area (as ASCII, EBCDIC etc)
				if (display_.char_set == CHARSET_CODEPAGE &&
					cp_first.size() > 0 &&
					kk < start_col)
				{
					// Continuation byte of MBCS character
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + cont_pad, tt.top, ContChar(), 1);
				}
				else if (buf[kk] < 32 && display_.char_set != CHARSET_EBCDIC && display_.char_set != CHARSET_OEM)
				{
					// Control characters are diplayed the same for all char sets except for
					//  - EBCDIC (some chars < 32 may not be graphical) and 
					//  - OEM (there are graphic characters for chars < 32)
					if (display_.control == 0)
					{
						// Display control char and other chars as a dot (normally in red)
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}
					else if (display_.control == 1)
					{
						// Display control chars as red uppercase equiv.
						char cc = buf[kk] + 0x40;
						pDC->TextOut(posc + kk*char_width_w, tt.top, &cc, 1);
					}
					else if (display_.control == 2)
					{
						// Display control chars as C escape code (in red)
						const char *check = "\a\b\f\n\r\t\v\0";
						const char *display = "abfnrtv0";
						const char *pp;
						if ((pp = strchr(check, buf[kk])) != NULL)
							pDC->TextOut(posc + kk*char_width_w, tt.top, display + (pp-check), 1);
						else
							::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}

					// For MBCS char sets we need to keep track that we have done this character
					if (display_.char_set == CHARSET_CODEPAGE && cp_first.size() > 0)
						++start_col;
				}
				else if (display_.char_set == CHARSET_CODEPAGE && code_page_ == CP_UTF8)
				{
					// Handle UTF 8 specially since it is not like other Windows code pages
					ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 0);
					if ((buf[kk] & 0xC0) == 0x80)
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + cont_pad, tt.top, ContChar(), 1);
					else
					{
						// The top 2 bits determine how many following (continuation) bytes there are.  If top bit
						// is zero then len == 1 (no cont bytes) since it is ASCII
						size_t len;
						if ((buf[kk] & 0x80) == 0x00)
							len = 1;
						else if ((buf[kk] & 0xE0) == 0xC0)
							len = 2;
						else if ((buf[kk] & 0xF0) == 0xE0)
							len = 3;
						else if ((buf[kk] & 0xF8) == 0xF0)
							len = 4;
						else if ((buf[kk] & 0xFC) == 0xF8)
							len = 5;
						else if ((buf[kk] & 0xFE) == 0xFC)
							len = 6;
						else
							len = 0;   // invalid UTF-8

						// MultiByteToWideChar (at least in XP) does not handle some invalid sequences properly,
						// eg C0 67 return "g" (the C0 appears to be ignored) - so we need to check ourselves
						bool isBad = false;
						for (int nn = 1; nn < len; ++nn)
							if ((buf[kk+nn] &0x80) != 0x80)
							{
								isBad = true;
								break;
							}

						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (len == 0 ||                                                                // 1111111X is not a valid byte
							isBad ||                                                                   // not enough cont bytes
							MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], len, &wc, 1) == 0 ||  // could not convert for some other reason
							wc >= 0xE000 && wc < 0xF900)                                               // invalid byte sequence
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							wc = *BadChar();
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
				}
				else if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ <= 1)
				{
					// Handle single byte code page
					ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 1);
					wchar_t wc;                               // equivalent Unicode (UTF-16) character
					if (MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], 1, &wc, 1) != 1 ||
						wc >= 0xE000 && wc < 0xF900)
					{
						// Character translation returned no bytes or Unicode value in "Private Use Area"
						wc = *BadChar();
					}

					CSize size;
					::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
				}
				else if (display_.char_set == CHARSET_CODEPAGE)
				{
					// Handle MBCS code pages
					ASSERT(cp_first.size() > 0 && kk == start_col);
					size_t len = CharNextExA(code_page_, (const char *)&buf[kk], 0) - (char *)&buf[kk];
					// Display the multibyte character that starts at this byte then find the start of the next one
					wchar_t wc;                               // equivalent Unicode (UTF-16) character
					if (MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], len, &wc, 1) != 1 ||
						wc >= 0xE000 && wc < 0xF900)
					{
						// Character translation returned no bytes or Unicode value in "Private Use Area"
						wc = *BadChar();
						++start_col;
					}
					else
					{
						// Character translation was good so ouput the character
						start_col += len;
					}

					CSize size;
					::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
				}
				else if (display_.char_set == CHARSET_EBCDIC)
				{
					// Display EBCDIC (or red dot if not valid EBCDIC char)
					if (e2a_tab[buf[kk]] == '\0')
					{
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}
					else
					{
						pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&e2a_tab[buf[kk]], 1);
					}
				}
				else if (display_.char_set == CHARSET_ASCII && buf[kk] < 127 ||
					     display_.char_set != CHARSET_ASCII && buf[kk] >= first_char_ && buf[kk] <= last_char_)
				{
					// Display normal char or graphic char if in font
					pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&buf[kk], 1);
				}
				else
				{
					// Display chars "out of range" as dots
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + bad_pad, tt.top, &dot, 1);
					//::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + bad_pad, tt.top, BadChar(), 1);
  				}
			}
		}

		// If any part of the line is within the current selection
		if (!pDC->IsPrinting() && start_addr < end_addr &&
			end_addr > line*rowsize_ - offset_ && start_addr < (line+1)*rowsize_ - offset_)
		{
			FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
			FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);
//            ASSERT(end > start);

			ASSERT(display_.hex_area || display_.char_area);
			if (!display_.vert_display && display_.hex_area)
			{
				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_) - 1)) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
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
				if (has_focus && !display_.edit_char || num_colours <= 256)
					pDC->InvertRect(&rev);  // Full contrast reverse video only if in editing in hex area
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}

			if (display_.vert_display || display_.char_area)
			{
				// Draw char selection in inverse
				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_) - 1)) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
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
				if (num_colours <= 256 || has_focus && (display_.vert_display || display_.edit_char))
					pDC->InvertRect(&rev);
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}
		}
		else if (theApp.show_other_ && has_focus &&
				 !display_.vert_display &&
				 display_.char_area && display_.hex_area &&  // we can only display in the other area if both exist
				 !pDC->IsPrinting() &&
				 start_addr == end_addr && 
				 start_addr >= line*rowsize_ - offset_ && 
				 start_addr < (line+1)*rowsize_ - offset_)
		{
			// Draw "shadow" cursor in the other area
			FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
			FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);

			CRect rev(norm_rect);
			if (display_.edit_char)
			{
				ASSERT(display_.char_area);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_))) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
			}
			else
			{
				ASSERT(display_.hex_area);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_))) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
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
				 start_addr >= line*rowsize_ - offset_ && 
				 start_addr < (line+1)*rowsize_ - offset_)
		{
			// Draw "shadow" for current byte when lost focus
			if (!display_.vert_display && display_.hex_area)
			{
				// Get rect for hex area
				FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
				FILE_ADDRESS end = min(end_addr, (line+1)*rowsize_ - offset_);

				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_))) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
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
			if (display_.vert_display || display_.char_area)
			{
				// Get rect for char area or stacked mode
				FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
				FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);

				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_))) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
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

	if (pDC->IsPrinting() && print_sel_ && dup_lines_)
	{
		if (line >= last_line)
			curpage_ = -1;  // signal that there is no more to print (so we can set m_bContinuePrinting to FALSE later)

		// Display any residual repeated lines count
		if (repeat_count > 0)
		{
			// Display how many times the line was repeated
			CString mess;
			if (repeat_count == 1)
				mess = "Repeated once";
			else
				mess.Format("Repeated %d more times", repeat_count);
			CRect mess_rect = norm_rect;
			if (neg_x)
			{
				mess_rect.left = -mess_rect.left;
				mess_rect.right = -mess_rect.right;
			}
			if (neg_y)
			{
				mess_rect.top = -mess_rect.top;
				mess_rect.bottom = -mess_rect.bottom;
			}
			mess_rect.left += hex_pos(0, char_width);
			pDC->DrawText(mess, &mess_rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
		}
	}

	if (!pDC->IsPrinting())
	{
		ShowCaret();
	}
//    move_dlgs();
}

#ifdef RULER_ADJUST
// Draws adjuster handles in the ruler
void CHexEditView::draw_adjusters(CDC* pDC)
{
	int xpos;

	// Set up pen and brush colours (all adjusters are the same colour)
	CPen pen(PS_SOLID, 0, RGB(0,0,0));          // black pen
	CPen pdash(PS_DOT, 0, RGB(0,0,0));          // for dashed black line
	CBrush bwhite(RGB(255,255,255));            // white brush
	CBrush bred(RGB(192,0,0));                  // red brush
	CPen * psp = pDC->SelectObject(&pen);       // ptr to saved pen
	CBrush * psb = pDC->SelectObject(&bwhite);  // ptr to saved brush

	// Show rowsize_ in the ruler
	ASSERT(rowsize_ > 3);
	if (!display_.vert_display && display_.hex_area)
	{
		if (adjusting_rowsize_ == -1 || adjusting_rowsize_ == rowsize_)
			xpos = char_pos(0) - text_width_w_/2 - scrollpos_.x;
		else
			xpos = hex_pos(adjusting_rowsize_) - scrollpos_.x;
		if (display_.autofit)
			pDC->SelectObject(&bred);
		draw_rowsize(pDC, xpos-1);
		if (display_.autofit)
			pDC->SelectObject(&bwhite);
		if (adjusting_rowsize_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}
	else if (display_.vert_display || display_.char_area)
	{
		if (adjusting_rowsize_ == -1 || adjusting_rowsize_ == rowsize_)
			xpos = char_pos(rowsize_ - 1) + (3*text_width_w_)/2 - scrollpos_.x;
		else
			xpos = char_pos(adjusting_rowsize_) - scrollpos_.x;
		if (display_.autofit)
			pDC->SelectObject(&bred);
		draw_rowsize(pDC, xpos-1);
		if (display_.autofit)
			pDC->SelectObject(&bwhite);
		if (adjusting_rowsize_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}

	// Show position of "offset_" in the ruler (in hex and/or char areas)
	ASSERT(offset_ < rowsize_);
	if (!display_.vert_display && display_.hex_area)
	{
		if (adjusting_offset_ > -1)
			xpos = hex_pos(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1) - 
			       scrollpos_.x;
		else
			xpos = hex_pos(offset_) - scrollpos_.x;
		draw_offset(pDC, xpos-1);
		if (adjusting_offset_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}
	if (display_.vert_display || display_.char_area)
	{
		if (adjusting_offset_ > -1)
			xpos = char_pos(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1) -
			       scrollpos_.x;
		else
			xpos = char_pos(offset_) - scrollpos_.x;
		if (display_.vert_display || !display_.hex_area)
			draw_offset(pDC, xpos-1);
		if (adjusting_offset_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}

	// Show current grouping if not adjusting (adjusting_group_by_ == -1) OR
	// current adjust column if not dragged past the edge (adjusting_group_by_ < 9999)
	if (adjusting_group_by_ == -1 && group_by_ < rowsize_ ||
		adjusting_group_by_ > -1 && adjusting_group_by_ < rowsize_)
	{
		if (display_.vert_display)
		{
			if (adjusting_group_by_ > -1)
				xpos = char_pos(adjusting_group_by_) - scrollpos_.x;
			else
				xpos = char_pos(group_by_) - scrollpos_.x;
			if (display_.vert_display && 
				(adjusting_group_by_ == -1 || adjusting_group_by_%group_by_ == 0))
				xpos -= text_width_w_/2;
			draw_group_by(pDC, xpos);
			if (adjusting_group_by_ > -1)
			{
				// Draw vertical dashed line
				(void)pDC->SelectObject(pdash);
				pDC->MoveTo(xpos, bdr_top_);
				pDC->LineTo(xpos, 30000);
				(void)pDC->SelectObject(pen);
			}
		}
		else if (display_.hex_area)
		{
			if (adjusting_group_by_ > -1)
				xpos = hex_pos(adjusting_group_by_) - 2 - scrollpos_.x;
			else
				xpos = hex_pos(group_by_) - 2 - scrollpos_.x;
			if (adjusting_group_by_ == -1 || adjusting_group_by_%group_by_ == 0)
				xpos -= (text_width_ - 1);
			draw_group_by(pDC, xpos);
			if (adjusting_group_by_ > -1)
			{
				(void)pDC->SelectObject(pdash);
				pDC->MoveTo(xpos, bdr_top_);
				pDC->LineTo(xpos, 30000);
				(void)pDC->SelectObject(pen);
			}
		}
	}

	(void)pDC->SelectObject(psp);
	(void)pDC->SelectObject(psb);
}

// Draws row size adjustment handle in the ruler
void CHexEditView::draw_rowsize(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos + 2, bdr_top_ - 7);
	pDC->LineTo(xpos,     bdr_top_ - 7);
	pDC->LineTo(xpos - 3, bdr_top_ - 4);
	pDC->LineTo(xpos,     bdr_top_ - 1);
	pDC->LineTo(xpos + 2, bdr_top_ - 1);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}

// Draws offset handle in the ruler
void CHexEditView::draw_offset(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos - 1, bdr_top_ - 6);
	pDC->LineTo(xpos - 1, bdr_top_ - 2);
	pDC->LineTo(xpos,     bdr_top_ - 1);
	pDC->LineTo(xpos + 3, bdr_top_ - 4);
	pDC->LineTo(xpos,     bdr_top_ - 7);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}

// Draws group by handle in the ruler
void CHexEditView::draw_group_by(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos - 3, bdr_top_ - 7);
	pDC->LineTo(xpos - 3, bdr_top_ - 5);
	pDC->LineTo(xpos,     bdr_top_ - 2);
	pDC->LineTo(xpos + 3, bdr_top_ - 5);
	pDC->LineTo(xpos + 3, bdr_top_ - 7);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}
#endif

void CHexEditView::draw_bg(CDC* pDC, const CRectAp &doc_rect, bool neg_x, bool neg_y,
						   int line_height, int char_width, int char_width_w,
						   COLORREF clr, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
						   bool merge /*=true*/, int draw_height /*=-1*/)
{
	if (end_addr < start_addr) return;

	if (draw_height > -1 && draw_height < 2) draw_height = 2;  // make it at least 2 pixels (1 does not draw properly)

	int saved_rop = pDC->SetROP2(R2_NOTXORPEN);
	CPen pen(PS_SOLID, 0, clr);
	CPen * psaved_pen = pDC->SelectObject(&pen);
	CBrush brush(clr);
	CBrush * psaved_brush = pDC->SelectObject(&brush);

	FILE_ADDRESS start_line = (start_addr + offset_)/rowsize_;
	FILE_ADDRESS end_line = (end_addr + offset_)/rowsize_;
	int start_in_row = int((start_addr+offset_)%rowsize_);
	int end_in_row = int((end_addr+offset_)%rowsize_);

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

		if (!display_.vert_display && display_.hex_area)
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
			{
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
			}
		}

		if (display_.vert_display || display_.char_area)
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
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}

		pDC->SetROP2(saved_rop);
		pDC->SelectObject(psaved_pen);
		pDC->SelectObject(psaved_brush);
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
	if (!display_.vert_display && display_.hex_area)
	{
		rct.left = hex_pos(start_in_row, char_width) -
				   doc_rect.left + bdr_left_;
		rct.right = hex_pos(rowsize_ - 1, char_width) +
					2*char_width - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
	}

	if (display_.vert_display || display_.char_area)
	{
		rct.left = char_pos(start_in_row, char_width, char_width_w) -
				   doc_rect.left + bdr_left_;
		rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
					char_width_w - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
	}

	// Last (partial) line
	rct.bottom = int((end_line+1) * line_height - doc_rect.top + bdr_top_);
	rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
	if (neg_y)
	{
		rct.top = -rct.top;
		rct.bottom = -rct.bottom;
	}
	if (!display_.vert_display && display_.hex_area)
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
		{
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}
	}

	if (display_.vert_display || display_.char_area)
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
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
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
			if (!display_.vert_display && display_.hex_area)
			{
				rct.left = hex_pos(0, char_width) -
						   doc_rect.left + bdr_left_;
				rct.right = hex_pos(rowsize_ - 1, char_width) +
							2*char_width - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
				ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
			}

			if (display_.vert_display || display_.char_area)
			{
				rct.left = char_pos(0, char_width, char_width_w) -
						   doc_rect.left + bdr_left_;
				rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
							char_width_w - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
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

		if (!display_.vert_display && display_.hex_area)
		{
			rct.left = hex_pos(0, char_width) -
					   doc_rect.left + bdr_left_;
			rct.right = hex_pos(rowsize_ - 1, char_width) +
						2*char_width - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}

		if (display_.vert_display || display_.char_area)
		{
			rct.left = char_pos(0, char_width, char_width_w) -
					   doc_rect.left + bdr_left_;
			rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
						char_width_w - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}
	}

	pDC->SetROP2(saved_rop);
	pDC->SelectObject(psaved_pen);
	pDC->SelectObject(psaved_brush);
	return;
}

// xxx TODO TBD test deletion at end (when last_virt == last_addr)
// xxx can we pass first_addr/last_addr instead of first_virt/last_virt
// xxx comments

void CHexEditView::draw_deletions(CDC* pDC, const vector<FILE_ADDRESS> & addr, const vector<FILE_ADDRESS> & len, 
								  FILE_ADDRESS first_virt, FILE_ADDRESS last_virt,
								  const CRectAp &doc_rect, bool neg_x, bool neg_y,
								  int line_height, int char_width, int char_width_w,
								  COLORREF colour)
{
	ASSERT(addr.size() == len.size());               // there should be equal numbers of addresses and lengths

	COLORREF prev_col = pDC->SetTextColor(bg_col_);

	int ii;
	// Skip blocks above the top of the display area
	// [This needs to be a binary search in case we get a large number of compare diffs]
	for (ii = 0; ii < addr.size(); ++ii)
		if (addr[ii] >= first_virt)
			break;

	for ( ; ii < addr.size(); ++ii)
	{
		// Check if we are now past the end of the display area
		if (addr[ii] > last_virt)
			break;

		CRect draw_rect;

		draw_rect.top = int(((addr[ii] + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_);
		draw_rect.bottom = draw_rect.top + line_height;
		if (neg_y)
		{
			draw_rect.top = -draw_rect.top;
			draw_rect.bottom = -draw_rect.bottom;
		}

		if (!display_.vert_display && display_.hex_area)
		{
			draw_rect.left = hex_pos(int((addr[ii] + offset_)%rowsize_), char_width) - 
								char_width - doc_rect.left + bdr_left_;
			draw_rect.right = draw_rect.left + char_width;
			if (neg_x)
			{
				draw_rect.left = -draw_rect.left;
				draw_rect.right = -draw_rect.right;
			}
			pDC->FillSolidRect(&draw_rect, colour);
			char cc = (len[ii] > 9 || !display_.delete_count) ? '*' : '0' + char(len[ii]);
			pDC->DrawText(&cc, 1, &draw_rect, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		}
		if (display_.vert_display || display_.char_area)
		{
			draw_rect.left = char_pos(int((addr[ii] + offset_)%rowsize_), char_width, char_width_w) - 
								doc_rect.left + bdr_left_ - 2;
			draw_rect.right = draw_rect.left + char_width_w/5+1;
			if (neg_x)
			{
				draw_rect.left = -draw_rect.left;
				draw_rect.right = -draw_rect.right;
			}
			pDC->FillSolidRect(&draw_rect, colour);
		}
	}

	pDC->SetTextColor(prev_col);   // restore text colour
}

void CHexEditView::draw_backgrounds(CDC* pDC,
									const vector<FILE_ADDRESS> & addr, const vector<FILE_ADDRESS> & len, 
									FILE_ADDRESS first_virt, FILE_ADDRESS last_virt,
									const CRectAp &doc_rect, bool neg_x, bool neg_y,
									int line_height, int char_width, int char_width_w,
									COLORREF colour, bool merge /*=true*/, int draw_height /*=-1*/)
{
	ASSERT(addr.size() == len.size());                                 // arrays should be equal size
	FILE_ADDRESS first_addr = max(0, first_virt);                      // First address to actually display
	FILE_ADDRESS last_addr  = min(GetDocument()->length(), last_virt); // One past last address actually displayed

	int ii;
	if (!ScrollUp())
	{
		// Skip blocks above the top of the display area
		// [This needs to be a binary search in case we get a large number of compare diffs]
		for (ii = 0; ii < addr.size(); ++ii)
			if (addr[ii] + len[ii] > first_virt)
				break;

		for ( ; ii < addr.size(); ++ii)
		{
			// Check if we are now past the end of the display area
			if (addr[ii] > last_virt)
				break;

			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, colour,
					max(addr[ii], first_addr), 
					min(addr[ii] + len[ii], last_addr),
					merge, draw_height);
		}
	}
	else
	{
		// Starting at end skip blocks below the display area
		// [This needs to be a binary search in case we get a large number of compare diffs]
		for (ii = addr.size() - 1; ii >= 0; ii--)
			if (addr[ii] < last_virt)
				break;

		for ( ; ii >= 0; ii--)
		{
			if (addr[ii] + len[ii] <= first_virt)
				break;

			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, colour,
					max(addr[ii], first_addr), 
					min(addr[ii] + len[ii], last_addr),
					merge, draw_height);
		}
	}
}
