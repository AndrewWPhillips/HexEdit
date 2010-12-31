// ScrView.cpp : implements CScrView class
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
#include "ScrView.h"

#include "crtdbg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScrView

IMPLEMENT_DYNAMIC(CScrView, CView)

CScrView::CScrView()
{
	init_coord_ = false;
	total_ = page_ = line_ = null_size;
	bdr_top_ = bdr_bottom_ = bdr_left_ = bdr_right_ = 0;
	maxbar_ = null_size;
	scrollpos_ = caretpos_ = selpos_ = basepos_ = CPointAp(0,0);
	page_specified_ = line_specified_ = FALSE;
	in_update_ = FALSE;
	caret_changes_ = TRUE;              // Caret movement allowed
	caret_mode_ = FALSE;                // Initially in scroll mode
	caret_level_ = 1;                   // Initially caret is off
	caret_block_ = FALSE;               // Use line caret
	caret_seen_ = FALSE;                // Doesn't matter, but init anyway
	scroll_up_ = FALSE;

	horz_buffer_zone_ = 1;              // At least one char from caret to right edge of screen
	vert_buffer_zone_ = 2;
	scroll_past_ends_ = TRUE;
	autoscroll_accel_ = 10;

	mouse_down_ = FALSE;

	pen_= NULL;
	brush_ = NULL;
	font_ = NULL;
	bitmap_ = NULL;
	rgn_ = NULL;
	mapmode_ = MM_TEXT;
}

CScrView::~CScrView()
{
}


BEGIN_MESSAGE_MAP(CScrView, CView)
		//{{AFX_MSG_MAP(CScrView)
		ON_WM_SIZE()
		ON_WM_VSCROLL()
		ON_WM_HSCROLL()
		ON_WM_KEYDOWN()
		ON_WM_SETFOCUS()
		ON_WM_KILLFOCUS()
		ON_WM_LBUTTONDOWN()
		ON_WM_LBUTTONUP()
		ON_WM_MOUSEMOVE()
		ON_WM_TIMER()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScrView diagnostics

#ifdef _DEBUG
void CScrView::AssertValid() const
{
		CView::AssertValid();
}

void CScrView::Dump(CDumpContext& dc) const
{
		CView::Dump(dc);

		dc << "\n";
}
#endif //_DEBUG

const CSizeAp CScrView::null_size(0,0);
const CPointAp CScrView::null_point(-1,-1);

// Set the size of the document so that we can set up scrollbar range
// Also (optionally) allow "page" and "line" scroll sizes.  If page is
// "null_size" the "page" size is calculated based on the current window
// size.  If line is "null_size" then the "line" size is calculated based
// on the current font.  Note that the "line" (and page) X value refers
// to horizontal scroll measurements (really a column not line).
// Note the top-left of the view always has coordinates (0,0) and all sizes
// and locations use a Y axis which points down and an X axis pointing right.
// These classes are mapping mode safe except for this restriction.  That is,
// you should use logical coords but if the mapping mode has an upwards
// Y axis (as MM_LOENGLISH, MM_HIMETRIC etc) (or a leftwards X axis) then
// the values must be negated.
void CScrView::SetSize(CSizeAp total, CSizeAp page, CSizeAp line)
{
	ASSERT(total.cx >= -1 && total.cy >= -1);
	ASSERT(page.cx >= 0 && page.cy >= 0);
	ASSERT(line.cx >= 0 && line.cy >= 0);

	// Store info about the total doc size.  Also calc. the scrollbar
	// range -- note that the scrollbar range is limited to 32768, plus
	// we need a bit more for when the user scrolls past the end of the
	// document -- so just use a max of 16000 to be safe.
	if (total.cx > -1) total_.cx = total.cx;
	if (total.cy > -1) total_.cy = total.cy;
	maxbar_ = total_;
	if (maxbar_.cx > 16000)
		maxbar_.cx = 16000;
	if (maxbar_.cy > 16000)
		maxbar_.cy = 16000;

	// Store values for amount of scrolling by "page" and "line"
	// -- if not specified (null_size used) then they will be calculated
	// in update_bars().  Also store whether they were specified so that
	// we remember not to recalculate them.
	page_specified_ = page != null_size;
	if (page_specified_) page_ = page;
	line_specified_ = line != null_size;
	if (line_specified_) line_ = line;

	update_bars();
}

// Set size using text units (see SetSize() above)
void CScrView::SetTSize(CSizeAp total, CSizeAp page, CSizeAp line)
{
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	if (total.cx > 0) total.cx *= tm.tmAveCharWidth;
	if (total.cy > 0) total.cy *= tm.tmHeight + tm.tmExternalLeading;
	page.cx *= tm.tmAveCharWidth;
	page.cy *= tm.tmHeight + tm.tmExternalLeading;
	line.cx *= tm.tmAveCharWidth;
	line.cy *= tm.tmHeight + tm.tmExternalLeading;

	SetSize(total, page, line);
}

// Set the current position of the display within the document.
// Note that the vertical distance is always positive downwards and the
// horizontal distance is positive to the right.  This means that for a
// mapping mode where the positive Y axis points up (which includes most
// apart from MM_TEXT) then you need to reverse the sign.
void CScrView::SetScroll(CPointAp newpos, BOOL strict /*=FALSE*/)
{
	if (newpos.x < 0) newpos.x = scrollpos_.x;
	if (newpos.y < 0) newpos.y = scrollpos_.y;
	ValidateScroll(newpos, strict);
	update_bars(newpos);
}

// Set the current scroll position in "text" units (see SetScroll)
void CScrView::SetTScroll(CPointAp newpos)
{
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	newpos.x *= tm.tmAveCharWidth;
	newpos.y *= tm.tmHeight + tm.tmExternalLeading;

	SetScroll(newpos);
}

// Sets the location of the caret.  Note that this does not mean that the
// caret will actually become visible to the user unless it is within the
// current display (see SetScroll).  To let the user see the caret use
// DisplayCaret().
void CScrView::SetCaret(CPointAp newpos)
{
	SetSel(newpos, newpos);
}

// Set the current caret position in "text" units (see SetCaret)
void CScrView::SetTCaret(CPointAp newpos)
{
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	newpos.x *= tm.tmAveCharWidth;
	newpos.y *= tm.tmHeight + tm.tmExternalLeading;

	SetSel(newpos, newpos);
}

// Sets the selected region.  Note that this does not mean that any of this
// region will actually become visible to the user unless it is within the
// current display (see SetScroll).  To let the user see (at least some of)
// the region use DisplayCaret().  If p1 == p2 then this is the same as
// calling SetCaret().  The boolean parameter base1 (default false) determines
// how basepos_ (base address for selections) is set -- if base1 is false
// then basepos_ is the smaller of p1 and p2, if true the basepos_ becomes p1.
void CScrView::SetSel(CPointAp p1, CPointAp p2, bool base1)
{
	// Save current selection
	CPointAp start = caretpos_;
	CPointAp end = selpos_;

	// Check/get valid caret locations and move selection there
	ValidateCaret(p2);
	ValidateCaret(p1);

	caret_hide();
	if (p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x))
	{
		basepos_ = caretpos_ = p1;
		selpos_ = p2;
	}
	else
	{
		basepos_ = caretpos_ = p2;
		selpos_ = p1;
	}
	if (base1) basepos_ = p1;

	// Cause prev. selection to be redrawn (unselected) and new selection drawn (selected)
	if (start != caretpos_ || end != selpos_)
	{
		// We invalidate everything - TODO this should be optimised to only do the changed bit
		InvalidateRange(start, end);
		InvalidateRange(caretpos_, selpos_);
		// Alternative (whole selection range) invalidation
		InvalidateRange(start, end, true);
		InvalidateRange(caretpos_, selpos_, true);
	}

	caret_show();
}

// Set the current selection in "text" units (see SetSel above)
void CScrView::SetTSel(CPointAp p1, CPointAp p2)
{
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	p1.x *= tm.tmAveCharWidth;
	p1.y *= tm.tmHeight + tm.tmExternalLeading;
	p2.x *= tm.tmAveCharWidth;
	p2.y *= tm.tmHeight + tm.tmExternalLeading;

	SetSel(p1, p2);
}

// Allow caret changes
void CScrView::EnableCaret()
{
	caret_hide();                       // Allow call to caret_show below w/o changing caret_level_
	caret_changes_ = TRUE;
	caret_show();
}

// Stop caret being changed, moved
void CScrView::DisableCaret()
{
	caret_hide();                       // Force caret not to be displayed (without affecting caret_level_)
	caret_changes_ = FALSE;
	caret_show();                       // Allow above call to caret_hide w/o changing caret_level_
}

void CScrView::CaretMode()
{
	caret_hide();                       // Allow call to caret_show below w/o changing caret_level_
	caret_mode_ = TRUE;                 // Allow cursor move to scroll caret
	caret_show();                       // Actually display the caret
}

// Turn off use of caret so that cursor keys revert to scrolling rather
// than moving the caret etc. (as before call EnableCaret).
void CScrView::ScrollMode()
{
	caret_hide();                       // Force caret not to be displayed (without affecting caret_level_)
	caret_mode_ = FALSE;
	caret_show();                       // Allow above call to caret_hide w/o changing caret_level_
}

#if 0
// Make sure caret/selection visible.  The display is scrolled if necessary to
// ensure that the caret or selection is (just) within the display area.
// Does nothing if EnableCaret has not been called and there is no selection.
void CScrView::DisplayCaret()
{
	// Is the caret enabled?
	if (caret_on_ && caretpos_ == selpos_)
	{
		// Get the point and size of the caret (so we can move it into display)
		CPointAp point(caretpos_);
		CSizeAp size = caret_size();
		// Widen rect so that text on either side of caret will be visible
		TEXTMETRIC tm;
		{
			CClientDC dc(this);
			OnPrepareDC(&dc);
			dc.GetTextMetrics(&tm);
		}
		// Add extra char width on left and 2 on right
		point.x -= tm.tmMaxCharWidth;
		if (point.x < 0) point.x = 0;
		size.cx += 3*tm.tmMaxCharWidth;

		DisplayPart(point, size);
	}
	else if (caretpos_ != selpos_)
	{
		CSizeAp tt, pp, ll;
		GetSize(tt, pp, ll);
		CPointAp start(caretpos_);
		CPointAp end(selpos_);
		CSizeAp size = caret_size();
		start.x = 0;
		size.cy += end.y - start.y;
		size.cx = tt.cx;
		DisplayPart(start, size, caretpos_ == basepos_);
	}
}
#else
// Make sure caret/selection visible.  The display is scrolled if necessary to
// ensure that the caret is (just) within the display area.  If there is a
// selection then it makes sure that the end which can extend (!= basepos_)
// is within the display rather than the caret.
// Does nothing if EnableCaret has not been called and there is no selection.
void CScrView::DisplayCaret(int char_width /*=-1*/)
{
	// Is the caret enabled?
	if (caret_mode_ && caret_changes_)
	{
		// Get the rect (point + size) that needs to be within the display
		CPointAp start(caretpos_);
		CPointAp end(selpos_);
		CSizeAp size = CSizeAp(end - start) + caret_size();

		// Now widen rect according to buffer zones

		// Get text sizes since zones are in characters
		TEXTMETRIC tm;
		{
			CClientDC dc(this);
			OnPrepareDC(&dc);
			dc.GetTextMetrics(&tm);
		}
		if (char_width == -1)
			char_width = tm.tmMaxCharWidth;

		// Add extra char width on left and 2 on right
		start.x -= char_width;
		if (start.x < 0) start.x = 0;
		size.cx = (horz_buffer_zone_+1)*char_width;

		__int64 vert_adjust = (tm.tmHeight + tm.tmExternalLeading)*vert_buffer_zone_;
		if (size.cy < win_height_/2 && vert_adjust > win_height_/2 - size.cy)
			vert_adjust = win_height_/2 - size.cy;
		else if (vert_adjust > win_height_/2)
			vert_adjust = win_height_/2;

		start.y -= vert_adjust;
		if (start.y < 0) start.y = 0;
		size.cy += vert_adjust*2;
		if (start.y + size.cy > total_.cy)
			size.cy = total_.cy - start.y;

		DisplayPart(start, size, caretpos_ == basepos_);
	}
}
#endif

// Is the caret currently within the display window?
BOOL CScrView::CaretDisplayed(CSizeAp win_size)
{
	if (!caret_mode_ || ! caret_changes_)
		return FALSE;

	// Work out bounding rectangles of display and caret
	CRectAp disp_rect;            // Display rect within doc
	if (win_size != null_size)
	{
		// Use passed size
		disp_rect = CRectAp(scrollpos_, win_size);
	}
	else
	{
		CRect rct;
		GetDisplayRect(&rct);
		disp_rect = ConvertFromDP(rct);
	}

	CRectAp caret_rect(caretpos_, caret_size());

	CRectAp intersect_rect;               // Intersection of display & caret
	return intersect_rect.IntersectRect(disp_rect, caret_rect);
}

// This just checks that a display position is valid and changes it to the
// closest/best valid position if not.
// Typically overridden in derived class (but this base class should still be called).
void CScrView::ValidateScroll(CPointAp &pos, BOOL strict /* =FALSE */)
{
	if (!scroll_past_ends_) strict = TRUE;  // Disallow scroll past ends according to this setting
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	if (strict && pos.x > total_.cx - win_width_)
		pos.x = total_.cx - win_width_;
	else if (pos.x > total_.cx - tm.tmAveCharWidth)
		pos.x = total_.cx - tm.tmAveCharWidth;
	if (pos.x < 0)
		pos.x = 0;
	if (strict && pos.y > total_.cy - win_height_)
		pos.y = total_.cy - win_height_;
	else if (pos.y > total_.cy - (tm.tmHeight + tm.tmExternalLeading))
		pos.y = total_.cy - (tm.tmHeight + tm.tmExternalLeading);
	if (pos.y < 0)
		pos.y = 0;
}

// This just checks that a caret position is valid and changes it to the
// closest/best valid position if not.  Typically overridden in derived
// class to make sure it's at a valid character position.
void CScrView::ValidateCaret(CPointAp &pos, BOOL inside /*=true*/)
{
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	// Move caret if outside of valid range
	if (pos.x > total_.cx - tm.tmAveCharWidth)
		pos.x = total_.cx - tm.tmAveCharWidth;
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y > total_.cy - __int64(tm.tmHeight + tm.tmExternalLeading))
		pos.y = total_.cy - __int64(tm.tmHeight + tm.tmExternalLeading);
	if (pos.y < 0)
		pos.y = 0;
}

// Make sure a point (or rectangle) is visible within display (see DisplayCaret).
// If either coord of pos is -1 then no scrolling is done in that direction.
// If size is not specified (or null_size) then only the point pos is made visible.
// Else (size given) pos is made visible and the points below and to the right
// of it as given by size.  If size > display area then pos is made visible and
// as much of the rectangle specified by size as possible given the window size.
void CScrView::DisplayPart(CPointAp pos, CSizeAp size, BOOL show_end /*=false*/)
{
	// If a coord is -1 (or invalid) do not move in that direction
	if (pos.x < 0 || pos.x + size.cx > total_.cx + win_width_)
		pos.x = scrollpos_.x;
	if (pos.y < 0 || pos.y + size.cy > total_.cy + win_height_)
		pos.y = scrollpos_.y;

	CPointAp newpos = scrollpos_;
//    ASSERT(newpos.x >= 0 && newpos.x < total_.cx + win_width_);
//    ASSERT(newpos.y >= 0 && newpos.y < total_.cy + win_height_);

	if (show_end)
	{
		// Move left side then right (this ensures right side is displayed)
		if (newpos.x > pos.x)
			newpos.x = pos.x;
		if (newpos.x + win_width_ < pos.x + size.cx)
			newpos.x = pos.x + size.cx - win_width_;
		// Move top then bottom (ensures bottom of rect is displayed)
		if (newpos.y > pos.y)
			newpos.y = pos.y;
		if (newpos.y + win_height_ < pos.y + size.cy)
			newpos.y = pos.y + size.cy - win_height_;
	}
	else
	{
		// Move right side then left (this order ensures pos.x is displayed)
		if (newpos.x + win_width_ < pos.x + size.cx)
			newpos.x = pos.x + size.cx - win_width_;
		if (newpos.x > pos.x)
			newpos.x = pos.x;
		// Move bottom then top
		if (newpos.y + win_height_ < pos.y + size.cy)
			newpos.y = pos.y + size.cy - win_height_;
		if (newpos.y > pos.y)
			newpos.y = pos.y;
	}

	// If the rect was outside the display then we have to scroll
	if (newpos != scrollpos_)
	{
		ValidateScroll(newpos);
		update_bars(newpos);
	}
}

// Responds to a cursor movement key to move the current caret position (if
// the caret is being displayed) or the current scroll position.
BOOL CScrView::MovePos(UINT nChar, UINT nRepCnt,
						 BOOL control_down, BOOL shift_down, BOOL caret_on)
{
	CPointAp newpos;

	// Get current position (if no caret then keys just do scrolling)
	if (!caret_on)
		newpos = scrollpos_;
	else if (shift_down && basepos_ == caretpos_)
		newpos = selpos_;
	else if (shift_down && basepos_ == selpos_)
		newpos = caretpos_;
	else if (nChar == VK_LEFT || nChar == VK_UP || nChar == VK_HOME || nChar == VK_PRIOR)
		newpos = caretpos_;
	else
		newpos = selpos_;

	// Calculate new position
	switch (nChar)
	{
	case VK_LEFT:
		if (!control_down)
			newpos.x -= line_.cx * nRepCnt;
		break;
	case VK_RIGHT:
		if (!control_down)
			newpos.x += line_.cx * nRepCnt;
		break;
	case VK_UP:
		if (!control_down)
			newpos.y -= line_.cy * nRepCnt;
		break;
	case VK_DOWN:
		if (!control_down)
			newpos.y += line_.cy * nRepCnt;
		break;
	case VK_HOME:
		if (!control_down)
			newpos.x = 0;
		else
		{
			newpos.x = 0;
			newpos.y = 0;
		}
		break;
	case VK_END:
		if (!control_down)
		{
			if (caret_on)
				newpos.x = total_.cx;
			else
				newpos.x = total_.cx - win_width_;
		}
		else
		{
			newpos.x = 0;
			if (caret_on)
				newpos.y = total_.cy;
			else
				newpos.y = total_.cy - win_height_;
		}
		break;
	case VK_PRIOR:
		if (!control_down)
			newpos.y -= page_.cy * nRepCnt;
		break;
	case VK_NEXT:
		if (!control_down)
			newpos.y += page_.cy * nRepCnt;
		break;
	}

	// Move to new position
	if (caret_on && shift_down)
	{
		// Using shifted cursor keys just extends the current selection
		OnSelUpdate(ConvertToDP(newpos));
	}
	else if (caret_on)
	{
		if (newpos == caretpos_ && caretpos_ == selpos_)
			return FALSE;           // Key not handled/had no effect

		SetCaret(newpos);
	}
	else
	{
		if (newpos == scrollpos_)
			return FALSE;           // Key not handled/had no effect

		SetScroll(newpos);
	}

	return TRUE;        // Indicate that keystroke used
}

// update_bars() is called if anything changes that will cause the scroll
// bars to change (move/resize etc).  This will happen if total_, page_
// or line_ sizes change, the window is resized, font changed, etc.
// If the parameter 'newpos' is not null_point then the display position
// will be moved to that position.  Note that the display may be redrawn.
void CScrView::update_bars(CPointAp newpos)
{
	if (in_update_ || m_hWnd == NULL)
		return;
	in_update_ = TRUE;

//    TRACE0("--- update_bars\n");
	check_coords();                 // Check that mapping mode not changed
	caret_hide();

	// Get info needed in later calcs
	TEXTMETRIC tm;
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
	}

	// Get display area and convert to logical units (but with +ve values)
	CRect cli;
	GetDisplayRect(&cli);
	//CRectAp doc_rect = ConvertFromDP(cli) - GetScroll();

	// If newpos not specified use old pos
	if (newpos == null_point)
		newpos = scrollpos_;
	ASSERT(newpos.x >= 0 && newpos.y >= 0);

	if (!page_specified_ || !line_specified_)
	{
		// Calculate amount of line and page scroll based on size of font
		if (!page_specified_)
		{
			// Recalc page size
			int lines = cli.Height()/tm.tmHeight + tm.tmExternalLeading - 1;
			if (lines < 1)
				lines = 1;
			page_.cx = cli.Width() - tm.tmAveCharWidth;
			page_.cy = lines * (tm.tmHeight + tm.tmExternalLeading);
		}

		if (!line_specified_)
		{
			// Recalc line size
			line_.cx = tm.tmAveCharWidth;
			line_.cy = tm.tmHeight + tm.tmExternalLeading;
		}
	}

	// Update display and scroll bars.
	ASSERT(total_.cx >= 0 && total_.cy >= 0);

	// If display position has changed redraw/scroll to it
	if (newpos != scrollpos_)
	{
		scroll_up_ = newpos.y < scrollpos_.y;

		// Get scroll & new positions in real logical coords
		CPointAp t_scroll = scrollpos_;
		if (negx()) t_scroll.x = -t_scroll.x;
		if (negy()) t_scroll.y = -t_scroll.y;
		CPointAp t_newpos = newpos;
		if (negx()) t_newpos.x = -t_newpos.x;
		if (negy()) t_newpos.y = -t_newpos.y;

//      // For OWNDC window and mapping mode != MM_TEXT ScrollWindow
//      // does not move caret properly!?!? - so we do it ourselves!

		__int64 tmp = t_scroll.y < t_newpos.y ? t_newpos.y - t_scroll.y : t_scroll.y - t_newpos.y;
		if (abs(int(t_scroll.x - t_newpos.x)) > cli.Width() || tmp > __int64(cli.Height()))
			DoInvalidate();     // ScrollWindow() can't handle big numbers
		else
		{
			// Work out scroll amount in device coordinates
			// LPtoDP can't handle more than 16 bits (signed)
			CPoint dev_scroll, dev_newpos;
			dev_scroll.x = t_scroll.x;
			dev_scroll.y = int(t_scroll.y);
			dev_newpos.x = t_newpos.x;
			dev_newpos.y = int(t_newpos.y);
			{
				// Put in compound statement so DC released ASAP
				CClientDC dc(this);
				OnPrepareDC(&dc);
				dc.LPtoDP(&dev_scroll);
				dc.LPtoDP(&dev_newpos);
			}
			DoScrollWindow(dev_scroll.x - dev_newpos.x, dev_scroll.y - dev_newpos.y);
		}
	}

	if (total_.cx <= 0)
		DoHScroll(0, 0, 0);                     // disable scroll bar
	else if (newpos.x > total_.cx - cli.Width())
	{
		// Handle scroll position past right edge
		DoHScroll(int(((__int64)(newpos.x + cli.Width()) * maxbar_.cx) / total_.cx),
				  (cli.Width() * maxbar_.cx) / total_.cx + 1,
				  int(((__int64)newpos.x * maxbar_.cx) / total_.cx) );
	}
	else
	{
		DoHScroll(maxbar_.cx,
				  (cli.Width() * maxbar_.cx) / total_.cx + 1,
				  int(((__int64)newpos.x * maxbar_.cx) / total_.cx) );
	}

	if (total_.cy <= 0)
		DoVScroll(0, 0, 0);                     // disable scroll bar
	else if (newpos.y > total_.cy - cli.Height())
	{
		// Handle scroll position if (somehow) moved past end
		DoVScroll(int(((__int64)(newpos.y + cli.Height()) * maxbar_.cy) / total_.cy),
				  int((cli.Height() * maxbar_.cy) / total_.cy + 1),
				  int(((__int64)newpos.y * maxbar_.cy) / total_.cy) );
	}
	else
	{
		DoVScroll(int(maxbar_.cy),
				  int((cli.Height() * maxbar_.cy) / total_.cy + 1),
				  int(((__int64)newpos.y * maxbar_.cy) / total_.cy) );
	}

	// Store new position and force redraw before restore of scroll_up_
	scrollpos_ = newpos;        // Keep track of where we moved to
	ASSERT(scrollpos_.x >= 0 && scrollpos_.y >= 0);
//    if (GetFocus() == this)
	caret_show();
	DoUpdateWindow();           // Force redraw before changing scroll_up_
	scroll_up_ = FALSE; // Make sure redraw defaults to downwards

	// Get client window again as changing the scroll bars may have changed it
	// (and nested size event was blocked by in_update_ flag)
	GetDisplayRect(&cli);
	CRectAp doc_rect = ConvertFromDP(cli);

	win_height_ = doc_rect.bottom - doc_rect.top;
	win_width_ = doc_rect.right - doc_rect.left;

	AfterScroll(newpos);
	in_update_ = FALSE;
}

int CScrView::SetMapMode(int mm)
{
	int rv = mapmode_;
	mapmode_ = mm;
	return rv;
}

// The following handle conversion between our view coords and device coords
// The view coords are logical coords (with origin  the top left corner of
// the window) BUT the direction of the axes is always +ve down and right,
// like MM_TEXT but unlike MM_LOENGLISH etc.
// Routines are provided to convert CRect and CPoint between view coords and
// device coords.  To convert a CSize object just use DPToLP and LPToDP
// (since they always return +ve results for CSize).
CRect CScrView::ConvertToDP(CRectAp rr)
{
	ASSERT(init_coord_);
	rr -= scrollpos_;

	CRect retval;
	if (negx_)
	{
		retval.left  = -rr.left;
		retval.right = -rr.right;
	}
	else
	{
		retval.left  = rr.left;
		retval.right = rr.right;
	}
	if (negy_)
	{
		retval.top    = -int(rr.top);
		retval.bottom = -int(rr.bottom);
	}
	else
	{
		retval.top    = int(rr.top);
		retval.bottom = int(rr.bottom);
	}
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.LPtoDP(&retval);
	retval.left += bdr_left_; retval.right  += bdr_left_;
	retval.top  += bdr_top_;  retval.bottom += bdr_top_;
	return retval;
}

CPoint CScrView::ConvertToDP(CPointAp pp)
{
	ASSERT(init_coord_);
	pp -= scrollpos_;

	CPoint retval;
	if (negx_)
		retval.x = -pp.x;
	else
		retval.x = pp.x;
	if (negy_)
		retval.y = -int(pp.y);
	else
		retval.y = int(pp.y);
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.LPtoDP(&retval);
	retval.x += bdr_left_; retval.y += bdr_top_;
	return retval;
}

CSize CScrView::ConvertToDP(CSizeAp ss)
{
	ASSERT(init_coord_);

	CSize retval;
	retval.cx = ss.cx;
	retval.cy = int(ss.cy);
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.LPtoDP(&retval);
	return retval;
}

CRectAp CScrView::ConvertFromDP(CRect rr)
{
	ASSERT(init_coord_);
	CClientDC dc(this);
	OnPrepareDC(&dc);
	rr.left -= bdr_left_; rr.right -= bdr_left_;
	rr.top -= bdr_top_; rr.bottom -= bdr_top_;
	dc.DPtoLP(&rr);
	if (negx_)
	{
		rr.left  = -rr.left;
		rr.right = -rr.right;
	}
	if (negy_)
	{
		rr.top    = -rr.top;
		rr.bottom = -rr.bottom;
	}
	CRectAp retval = rr;
	retval += scrollpos_;
	return retval;
}

CPointAp CScrView::ConvertFromDP(CPoint pp)
{
	ASSERT(init_coord_);
	CClientDC dc(this);
	OnPrepareDC(&dc);
	pp.x -= bdr_left_; pp.y -= bdr_top_;
	dc.DPtoLP(&pp);
	if (negx_)
		pp.x = -pp.x;
	if (negy_)
		pp.y = -pp.y;

	CPointAp retval = pp;
	retval += scrollpos_;
	return retval;
}

CSizeAp CScrView::ConvertFromDP(CSize ss)
{
	ASSERT(init_coord_);
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&ss);

	CSizeAp retval = ss;
//    retval += scrollpos_;
	return retval;
}


void CScrView::check_coords()
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	CPoint dirn_test(10,10);    // Don't use (1,1) - may be rounded to zero
	dc.DPtoLP(&dirn_test);
	negx_ = dirn_test.x < 0;
	negy_ = dirn_test.y < 0;
	init_coord_ = true;
}

void CScrView::LineCaret()
{
	caret_hide();
	caret_block_ = FALSE;
	caret_show();
}

void CScrView::BlockCaret()
{
	caret_hide();
	caret_block_ = TRUE;
	caret_show();
}

// Return the size of the caret (in logical coordinates).
// (Put this calculation in one place for ease of maintenance.)
CSizeAp CScrView::caret_size()
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);

	// Make caret twice as wide as border and as high as text
	CSize retval;
	if (caret_block_)
	{
		// Use the average character width as the caret width
		retval.cx = tm.tmAveCharWidth;
	}
	else
	{
		// Use twice the border width as the caret width
		// Get the border width in pixels (device units) and convert to logical units
		retval = CSize(::GetSystemMetrics(SM_CXBORDER)*2, 0);
		dc.DPtoLP(&retval);
	}
	retval.cy = tm.tmHeight + tm.tmExternalLeading;
	return CSizeAp(retval);
}

// Turn on the display of the caret (see caret_hide)
void CScrView::caret_show()
{
	// Is the caret enabled?
	if (--caret_level_ == 0 && caret_mode_ && caret_changes_)
	{
		CSizeAp size = caret_size();
		CPoint scroll = ConvertToDP(scrollpos_);  // scroll pos in dev coords
		CPoint caret = ConvertToDP(caretpos_);    // caret pos in dev coords

		// We have to handle hiding of the caret ourselves because CWnd::SetCaretPos can't
		// handle big numbers and may show the cursor in the wrong spot with big files, and
		// we don't want the cursor appearing in the borders (notably the ruler).
		if (abs(scrollpos_.x - caretpos_.x) < win_width_ + size.cx &&  // this will need fixing if we put something in bdr_left_
			caretpos_.y >= scrollpos_.y && caretpos_.y <= scrollpos_.y + win_height_ - size.cy)
		{
			// Create a solid caret (device coords)
			CSize dev_size;
			dev_size.cx = size.cx;
			dev_size.cy = int(size.cy);
			{
				CClientDC dc(this);
				OnPrepareDC(&dc);
				dc.LPtoDP(&dev_size);
			}
			CreateSolidCaret(dev_size.cx, dev_size.cy);
			caret_seen_ = TRUE;                 // Caret has been displayed

			// Set the caret location in device coords
			DoSetCaretPos(CPoint(caret.x - scroll.x + bdr_left_, int(caret.y - scroll.y) + bdr_top_));

			// Display it (may be "on" but not visible within display area)
			ShowCaret();
		}
		else
		{
			// Hide and destroy it
			caret_seen_ = FALSE;
			HideCaret();
			::DestroyCaret();
		}
	}
}

// Hide and destroy the caret (probably due to losing focus).
void CScrView::caret_hide()
{
	// Is the caret currently displayed?
	if (caret_level_++ == 0 && caret_mode_ && caret_changes_ && caret_seen_)
	{
		// Hide and destroy it
		HideCaret();
		::DestroyCaret();
	}
}

void CScrView::DoHScroll(int total, int page, int pos)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;                // Always use base of zero

	si.nMax = total;
	si.nPage = page;
	si.nPos = pos;

	SetScrollInfo(SB_HORZ, &si);
}

void CScrView::DoVScroll(int total, int page, int pos)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;                // Always use base of zero

	si.nMax = total;
	si.nPage = page;
	si.nPos = pos;

	SetScrollInfo(SB_VERT, &si);
}
/////////////////////////////////////////////////////////////////////////////
// CScrView message handlers

void CScrView::OnSize(UINT nType, int cx, int cy)
{
		CView::OnSize(nType, cx, cy);
		update_bars();
}

void CScrView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
		OnScroll(MAKEWORD(nSBCode, -1), nPos);
}

void CScrView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
		OnScroll(MAKEWORD(-1, nSBCode), nPos);
}

BOOL CScrView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	CPointAp newpos = scrollpos_;

	switch (LOBYTE(nScrollCode))
	{
	case SB_TOP:
		newpos.x = 0;
		break;
	case SB_BOTTOM:
		newpos.x = total_.cx - win_width_;
		break;
	case SB_LINEUP:
		newpos.x -= line_.cx;
		break;
	case SB_LINEDOWN:
		newpos.x += line_.cx;
		break;
	case SB_PAGEUP:
		newpos.x -= page_.cx;
		break;
	case SB_PAGEDOWN:
		newpos.x += page_.cx;
		break;
	case SB_THUMBPOSITION:
		// DoInvalidate here as this event sent after mouse released after
		// all SB_THUMBTRACK events.  This removes any bits of the scroll-
		// bar left after tracking caused the scroll bar to "evaporate".
		DoInvalidate();
		// fall through
	case SB_THUMBTRACK:
		newpos.x = (int)(((__int64)nPos * total_.cx) / maxbar_.cx);
		if (total_.cx > maxbar_.cx && newpos.x > total_.cx - win_width_)
			newpos.x = total_.cx - win_width_;
		break;
	}

	switch (HIBYTE(nScrollCode))
	{
	case SB_TOP:
		newpos.y = 0;
		break;
	case SB_BOTTOM:
		newpos.y = total_.cy - win_height_;
		break;
	case SB_LINEUP:
		newpos.y -= line_.cy;
		break;
	case SB_LINEDOWN:
		newpos.y += line_.cy;
		break;
	case SB_PAGEUP:
		newpos.y -= page_.cy;
		break;
	case SB_PAGEDOWN:
		newpos.y += page_.cy;
		break;
	case SB_THUMBPOSITION:
		// Invalidate here as this event sent after mouse released after
		// SB_THUMBTRACK events finished.  This removes any bits of the
		// scrollbar left after tracking caused the scroll bar to "evaporate".
		DoInvalidate();
		// fall through
	case SB_THUMBTRACK:
		newpos.y = ((__int64)nPos * total_.cy) / maxbar_.cy;
		if (total_.cy > maxbar_.cy && newpos.y > total_.cy - win_height_)
			newpos.y = total_.cy - win_height_;
		break;
	}

#if 0
	if (bDoScroll)
		ValidateScroll(newpos);
	// Work out amount to scroll (in device coordinates)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		CSize amt = CSize(newpos - scrollpos_); // This won't work
		dc.LPtoDP(&amt);
	}
	BOOL retval = OnScrollBy(amt, bDoScroll);
	if (retval && bDoScroll)
		DoUpdateWindow();

	return retval;
#else
	if (bDoScroll)
	{
		ValidateScroll(newpos);
		update_bars(newpos);
		DoUpdateWindow();
	}
	return TRUE;
#endif
}

BOOL CScrView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	// Update window and scroll bars if position has changed
	if (bDoScroll)
	{
		// Update scroll bars (using logical coords)
		{
			CClientDC dc(this);
			OnPrepareDC(&dc);
			dc.DPtoLP(&sizeScroll);
		}
		update_bars(scrollpos_ + CSizeAp(sizeScroll));
	}
	return TRUE;
}

void CScrView::OnInitialUpdate()
{
	check_coords();
	CView::OnInitialUpdate();
}

BOOL CScrView::PreCreateWindow(CREATESTRUCT& cs)
{
#ifdef USE_OWNDC
	BOOL retval = CView::PreCreateWindow(cs);

	// Get window class info. and modify it to set OWN DC bit
	static CString cls =
		AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_OWNDC,
				::LoadCursor(NULL, IDC_ARROW),          // Cursor
				(HBRUSH)::GetStockObject(WHITE_BRUSH),  // Background brush
				::LoadIcon(NULL, IDI_APPLICATION));     // Icon
	cs.lpszClass = cls;

	return retval;
#else
	return CView::PreCreateWindow(cs);
#endif
}

void CScrView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch(nChar)
	{
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_HOME:
	case VK_END:
	case VK_PRIOR:
	case VK_NEXT:
		if (MovePos(nChar, nRepCnt, control_down(), shift_down(), caret_mode_))
		{
			DisplayCaret();
			return;
		}
		break;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CScrView::OnKillFocus(CWnd* pNewWnd)
{
	caret_hide();
	CView::OnKillFocus(pNewWnd);
}

void CScrView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	caret_show();
}

void CScrView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (mouse_down_) return;    // Happens sometimes while debugging

	// Keep control of the pointer until mouse button released
	mouse_down_ = TRUE;

	// Create timer so that we regularly call OnSelUpdate when the mouse is
	// stationery outside the window so that the window is automatically 
	// scrolled (see OnTimer below for what the timer does).
	VERIFY(timer_id_ = SetTimer(1, 100, NULL));

	// Hide caret while dragging.  Plus: calling caret_hide here also means
	// that the caret is displayed in the correct position when caret_show
	// is called at the end of the drag (in OnLButtonUp below).
	caret_hide();
	SetCapture();

	if (shift_down())
	{
		// Shift click restarts selection from where it left off (on button up)
		OnSelUpdate(point);
	}
	else
	{
		// Move caret to where selection started
		CPointAp newpos = ConvertFromDP(point);
		ValidateCaret(newpos, FALSE);
		SetCaret(newpos);
	}

	CView::OnLButtonDown(nFlags, point);
}

void CScrView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (mouse_down_)
	{
		// Update selection to where we finished
		OnSelUpdate(point);

		// Release pointer now that drag has finished
		ReleaseCapture();
		mouse_down_ = FALSE;
		VERIFY(KillTimer(timer_id_));
		timer_id_ = 0;

		// Show caret in new position
		caret_show();
	}

	CView::OnLButtonUp(nFlags, point);
}

void CScrView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (mouse_down_)
	{
		// Drag: update display based on new position
		OnSelUpdate(point);
	}

	CView::OnMouseMove(nFlags, point);
}

BOOL CScrView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CPointAp pp = GetScroll();
	if (abs(zDelta) > line_.cy)
		pp.y -= (zDelta/line_.cy) * line_.cy;   // Restrict movement to multiple of text height
	else
		pp.y -= zDelta;
	if (pp.y < 0) pp.y = 0;                     // Scroll to start (else SetScroll ignores -ve position)
	SetScroll(pp);
	return TRUE;
//	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CScrView::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == timer_id_)
	{
		// Handle timer event that we have set up for autoscroll
		ASSERT(mouse_down_);

		// Update display based on new position
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);
		OnSelUpdate(point);
	}
	else
		CView::OnTimer(nIDEvent);
}

void CScrView::OnSelUpdate(CPoint point)
{
	// Keep the last drag position so we only invalidate what has changed.
	CPointAp last;
	ASSERT(basepos_ == caretpos_ || basepos_ == selpos_);
	if (caretpos_ == basepos_)
		last = selpos_;
	else
		last = caretpos_;

	CRect cli;                          // Display area (client coords)
	GetDisplayRect(&cli);
	CRectAp disp = ConvertFromDP(cli);  // Display area ("cli") in our coords
	CPointAp pp = ConvertFromDP(point); // Mouse point ("point") in our coords

	// Get display rect shrunk to only contain whole rows/columns
	CRectAp rr(disp);
	rr.top  = ((rr.top - 1)/line_.cy + 1)*line_.cy;
	rr.left = ((rr.left- 1)/line_.cx + 1)*line_.cx;
	rr.bottom = (rr.bottom/line_.cy)*line_.cy;
	rr.right  = (rr.right /line_.cx)*line_.cx;

	// Test if we are about to scroll or very close to edge
	if (!rr.PtInRect(pp))
	{
		CPointAp newpos(-1, -1);  // New scroll position (our coords)
		static clock_t last_clock = 0;

		if (pp.x < rr.left)
		{
			newpos.x = scrollpos_.x - line_.cx;
		}
		else if (pp.x >= rr.right)
		{
			newpos.x = scrollpos_.x + line_.cx;
		}

		if (pp.y < rr.top)
		{
			// Use clock to ensure scrolling does not get faster with more mouse move events
			clock_t curr_clock = clock();
			clock_t diff_clock = curr_clock - last_clock;
			if (diff_clock > CLOCKS_PER_SEC/5) diff_clock = CLOCKS_PER_SEC/5;

			// Scroll speed depends on distance mouse is above window
//            int diff_pixel = ((disp.top - pp.y)*(disp.top - pp.y))/10;
			int diff_pixel = int(disp.top - pp.y);
			int autoscroll = (int)pow((double)diff_pixel, autoscroll_accel_/10.0);

			__int64 to_move = 0;                        // How much to scroll up

			// Always scroll so that top of text line is at top of window
			if (disp.top < rr.top) to_move = line_.cy - (rr.top - disp.top);

			// If enough time has passed or mouse is far enough above the window...
			if (disp.top > pp.y && diff_clock * autoscroll >= CLOCKS_PER_SEC/5)
			{
				// Scroll according to time and distance
				to_move += line_.cy*((diff_clock * autoscroll)/(CLOCKS_PER_SEC/5));
				last_clock = curr_clock;
			}
			newpos.y = scrollpos_.y - to_move;
		}
		else if (pp.y >= rr.bottom)
		{
			// See above comments (same but for bottom of window not top)
			clock_t curr_clock = clock();
			clock_t diff_clock = curr_clock - last_clock;
			if (diff_clock > CLOCKS_PER_SEC/5) diff_clock = CLOCKS_PER_SEC/5;
			int diff_pixel = int(pp.y - disp.bottom);
			int autoscroll = (int)pow((double)diff_pixel, autoscroll_accel_/10.0);

			__int64 to_move = 0;
			if (disp.bottom > rr.bottom) to_move = line_.cy - (disp.bottom - rr.bottom);
			if (disp.bottom < pp.y && diff_clock * autoscroll >= CLOCKS_PER_SEC/5)
			{
				to_move += line_.cy*((diff_clock * autoscroll)/(CLOCKS_PER_SEC/5));
				last_clock = curr_clock;
			}
			newpos.y = scrollpos_.y + to_move;
		}

		if (newpos.y < 0)
			newpos.y = 0;
		SetScroll(newpos, TRUE);
	}

	// Set end of selection based on this point
	CPoint ptmp = point;                         // Get pp based on new value of "point"

	// Don't allow selection to extend outside window
	if (ptmp.y < cli.top) ptmp.y = cli.top;
	else if (ptmp.y >= cli.bottom) ptmp.y = cli.bottom - 1;

	if (ptmp.x < cli.left) ptmp.x = cli.left;
	else if (ptmp.x >= cli.right) ptmp.x = cli.right - 1;

	// Work out new selection
	pp = ConvertFromDP(ptmp);
	ValidateCaret(pp, FALSE);

	if (pp.y < basepos_.y || (pp.y == basepos_.y && pp.x < basepos_.x))
	{
		caretpos_ = pp;
		selpos_ = basepos_;
	}
	else
	{
		caretpos_ = basepos_;
		selpos_ = pp;
	}

	// Allow derived class to invalidate the whole selection
	if (pp.y != last.y || pp.x != last.x)
		InvalidateRange(caretpos_, selpos_, true);

	// Invalidate everything that's changed
	if (pp.y < last.y || (pp.y == last.y && pp.x < last.x))
		InvalidateRange(pp, last);
	else if (pp.y != last.y || pp.x != last.x)
		InvalidateRange(last, pp);
}

// InvalidateRange is an overrideable function that is used to invalidate the view
// between two caret positions.  This is used to invalidate bits of the window
// when the selection is changed (using mouse selection or with SetSel()).
// The default behaviour invalidates the lines (whole width of document) from
// the top of start to the bottom of end (using the current character height).
void CScrView::InvalidateRange(CPointAp start, CPointAp end, bool f /*=false*/)
{
	if (start.y > scrollpos_.y + win_height_ || start.x > scrollpos_.x + win_width_ ||
		end.y < scrollpos_.y || end.x < scrollpos_.x)
	{
		// All outside display area so do nothing.  Note that this may appear to not be nec.
		// as Windows does this too but due to overflow problems is safer to do it here.
		return;
	}

	CSizeAp ss = caret_size();

	// Also invalidate a row above and up to 3 rows below (this is necessary for stacked mode)
	start.y -= ss.cy;
	end.y += 3*ss.cy;

	if (start.x < scrollpos_.x) start.x = scrollpos_.x;
	if (start.y < scrollpos_.y) start.y = scrollpos_.y;
	if (end.y > scrollpos_.y + win_height_)
		end.y = scrollpos_.y + win_height_;

	// Invalidate the full width of display from top of start to bottom of end
	CRectAp rr(0, start.y, total_.cx, end.y);

	// Convert to device coords
	CRect norm_rect = ConvertToDP(rr);

	CRect cli;
	GetDisplayRect(&cli);

	// Invalidate the previous selection so that it is drawn unselected
	CRect rct;
	if (rct.IntersectRect(&cli, &norm_rect))
		DoInvalidateRect(&norm_rect);
}

void CScrView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnPrepareDC(pDC, pInfo);

	if (pen_ != NULL) pDC->SelectObject(pen_);
	if (brush_ != NULL) pDC->SelectObject(brush_);
	if (font_ != NULL) pDC->SelectObject(font_);
	if (bitmap_ != NULL) pDC->SelectObject(bitmap_);
	if (rgn_ != NULL) pDC->SelectObject(rgn_);
	if (mapmode_ != MM_TEXT) pDC->SetMapMode(mapmode_);
}

