// Print.cpp : part of implementation of the CHexEditView class
//
// Copyright (c) 2003-2010 by Andrew W. Phillips.
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
#include "HexEdit.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "ChildFrm.h"
#include "MainFrm.h"
#include "HexFileList.h"
#include "HexPrintDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CHexEditView printing

// Handles the File/Print command
void CHexEditView::OnFilePrint()
{
	CScrView::OnFilePrint();
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_print);
}

// This is basically called to display the Print dialog
BOOL CHexEditView::OnPreparePrinting(CPrintInfo* pInfo)
{
#ifdef USE_OWN_PRINTDLG
	//	Replace the standard print dialog with our own dialog
	if (pInfo->m_pPD != NULL)
	{
		if (pInfo->m_pPD->m_pd.hDC != 0)
			::DeleteDC(pInfo->m_pPD->m_pd.hDC);
		delete pInfo->m_pPD;
	}
	pInfo->m_pPD = new CHexPrintDialog(FALSE, PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOSELECTION);
	pInfo->SetMinPage(1);
	pInfo->SetMaxPage(65535);
#endif
	curpage_ = 0;

	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	pInfo->m_pPD->m_pd.Flags |= PD_SHOWHELP;
	if (!pInfo->m_bPreview && start_addr != end_addr)
	{
		pInfo->m_pPD->m_pd.Flags &= ~PD_NOSELECTION;    // Enable selection radio button
		pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;       // Select selection radio button
	}

	BOOL retval = DoPreparePrinting(pInfo);
	if (!retval)
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		aa->mac_error_ = 2;
	}
	return retval;
}

// Called at the start of the whole print job
void CHexEditView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
#ifdef USE_OWN_PRINTDLG
	if (pInfo->m_pPD->PrintSelection())
		dup_lines_ = ((CHexPrintDialog*)pInfo->m_pPD)->dup_lines_;
#endif
	print_offset_ = CSize(-1, 0);

//    CScrView::OnPrepareDC(pDC, pInfo);  why does this call base class version?
	OnPrepareDC(pDC, pInfo);

	pDC->SetMapMode(print_map_mode_);

	// Create font based on screen font but scaled for printer
	LOGFONT print_lf = display_.FontRequired() == FONT_OEM ? oem_lf_
	                   : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	// Check that the font size does not make text go off right side of page
	int page_width = pDC->GetDeviceCaps(HORZRES);       // page width in pixels
	int display_width;                                  // display area width in pixels
	CSize size;                                         // width needed for printing

	ASSERT(print_font_ == NULL);
	print_font_ = new CFont;
	CFont *pf_old;
	print_lf.lfHeight = print_lfHeight_;
	print_lf.lfWidth = 0;                   // Width calced based on height

	int u_scale = pDC->GetDeviceCaps(LOGPIXELSX);       // units scaling (pixels/inch)
	if (theApp.print_units_ == CHexEditApp::PRN_CM)
		u_scale = int(u_scale/2.54);

	if ((theApp.left_margin_ + theApp.right_margin_)*u_scale > page_width*2/3)
		theApp.left_margin_ = theApp.right_margin_ = page_width/(3*u_scale);

	for (int ii = 0; ; ++ii)
	{
		print_font_->CreateFontIndirect(&print_lf);
		pf_old = pDC->SelectObject(print_font_);

		// Get text size
		::GetTextExtentPoint32(pDC->m_hDC, "D", 1, &size);
		print_text_width_ = size.cx;                        // width of a "D" in logical units
		::GetTextExtentPoint32(pDC->m_hDC, "W", 1, &size);
		print_text_width_w_ = size.cx;                      // width of a "W"

		// Work out width of actual printing area in pixels
		display_width = int(page_width - (theApp.left_margin_ + theApp.right_margin_)*u_scale);
		if (theApp.print_box_)
		{
			size.cx = print_text_width_ + print_text_width_w_;
			size.cy = 0;
			pDC->LPtoDP(&size);
			display_width -= size.cx;
		}

		// Get required page width using current font size in device coords (pixels)
		if (display_.char_area)
			size.cx = char_pos(rowsize_, print_text_width_, print_text_width_w_);
		else
			size.cx = hex_pos(rowsize_, print_text_width_);
		size.cy = 0;
		pDC->LPtoDP(&size);

		// Check if font is small enough now or has not changed size
		if (ii > 200 || abs(size.cx) < display_width)
		{
			print_offset_.cx = LONG((display_width - abs(size.cx))/2);
			if (print_offset_.cx < 0) print_offset_.cx = 0;
			break;
		}

		pDC->SelectObject(pf_old);
		print_font_->DeleteObject();

		long new_height = long(print_lf.lfHeight *
			(page_width - (theApp.left_margin_ + theApp.right_margin_)*u_scale) /
			 size.cx);
		if (new_height >= print_lf.lfHeight)
			print_lf.lfHeight--;
		else
			print_lf.lfHeight = new_height;
		print_lf.lfWidth = 0;
	}

	// Work out text height of the (printer) font
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	// Note: we get slightly different values during printing vs print preview:
	// - PP: DoPreparePrinting calls CPrintDialog::CreatePrinterDC to create hDC
	// - print: DoPreparePrinting calls ::PrintDlg which sets up PRINTDLG::hDC
	// which give different text sizes.  TBD: Is it important enough to address this?
	print_text_height_ = tm.tmHeight + tm.tmExternalLeading;

	// Make sure pprint_offset_ is in logical coords and allow for vert. print options
	pDC->DPtoLP(&print_offset_);
	if (theApp.print_box_)
		print_offset_.cy += print_text_height_/2;  // start half a row further down (allow for line at top)
	if (theApp.print_hdr_)
		print_offset_.cy += print_text_height_;    // start a row further down (allows for header text line)

	size.cx = 0;
	size.cy = print_text_height_;
	pDC->LPtoDP(&size);
	int page_height = pDC->GetDeviceCaps(VERTRES);  // page height in pixels
	if (page_height < page_width/5)                 // May be lines of text not pixels (Eg Text Only driver)
		page_height *= size.cy;

	if ((theApp.top_margin_ + theApp.bottom_margin_)*u_scale > page_height*2/3)
		theApp.top_margin_ = theApp.bottom_margin_ = page_height/(3*u_scale);

	switch (theApp.spacing_)
	{
	case 0:
		// nothing needed here (single line spacing)
		break;
	case 1:
		print_text_height_ += print_text_height_/2; // 1.5 spacing
		print_offset_.cy += print_text_height_/2;   // start half a row further down
		break;
	case 2:
		print_text_height_ += print_text_height_;   // double line spacing
		print_offset_.cy += print_text_height_;     // start a row further down
		break;
	default:
		ASSERT(0);
	}

	// Get text sizes (logical units)
	::GetTextExtentPoint32(pDC->m_hDC, "D", 1, &size);
	print_text_width_ = size.cx;
	::GetTextExtentPoint32(pDC->m_hDC, "W", 1, &size);
	print_text_width_w_ = size.cx;

	// Work out the page size, number of pages, etc
	// Note that we can't use ConvertFromDP here as this is for printer not screen
	page_size_ = CSize(page_width - int((theApp.left_margin_ + theApp.right_margin_)*u_scale),
					   page_height - int((theApp.top_margin_ + theApp.bottom_margin_)*u_scale));
	pDC->DPtoLP(&page_size_);                   // normalised (+ve) coords
	if (display_.vert_display)
		lines_per_page_ = page_size_.cy / (print_text_height_*3);
	else
		lines_per_page_ = page_size_.cy / print_text_height_;
	if (theApp.print_box_)
		lines_per_page_--;
	if (theApp.print_hdr_)
		lines_per_page_--;
	if (lines_per_page_ < 1)
		lines_per_page_ = 1;

	margin_size_ = CSize(int(theApp.left_margin_*u_scale), int(theApp.top_margin_*u_scale));
	pDC->DPtoLP(&margin_size_);
	margin_size_ += print_offset_;

	if (pInfo->m_pPD->PrintSelection())
	{
		print_sel_ = true;                              // Printing current selection

		// Work out the first and last line to be printed and hence the number of pages
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		start_addr = ((start_addr + offset_)/rowsize_)*rowsize_ - offset_;
		end_addr = ((end_addr + offset_ - 1)/rowsize_ + 1)*rowsize_ - offset_;
		ASSERT(start_addr < end_addr);
		pInfo->SetMaxPage(int(end_addr - start_addr - 1)/(rowsize_ * lines_per_page_) + 1);
	}
	else
	{
		print_sel_ = false;                             // NOT printing selection

		// Work out number of pages (so MFC knows when to stop trying to print)
		pInfo->SetMaxPage(int(GetDocument()->length() + offset_ - 1)/(rowsize_ * lines_per_page_) + 1);

		// If doing print preview then start on the page that contains the caret
		if (pInfo->m_bPreview)
		{
			pInfo->m_nCurPage = int(GetPos() + offset_ - 1)/(rowsize_ * lines_per_page_) + 1;

			// The above doesn't seem to work anymore due to a bug in MFC so also do this
			((CMainFrame *)AfxGetMainWnd())->preview_page_ = pInfo->m_nCurPage;
		}
	}

	// Get info for headers and footers
	print_time_ = CTime::GetCurrentTime();
}

void CHexEditView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	((CMainFrame *)AfxGetMainWnd())->preview_page_ = -1;

	pDC->SelectObject(pfont_);
	delete print_font_;
	print_font_ = NULL;
}

void CHexEditView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CScrView::OnPrepareDC(pDC, pInfo);

	// Store page number for OnPrint & OnDraw (called by OnPrint)
	if (pDC->IsPrinting())
	{
		if (curpage_ == -1)
			pInfo->m_bContinuePrinting = FALSE;
		else
			curpage_ = pInfo->m_nCurPage - 1;   // Store page number (zero offset)
	}
}

// Print (or preview) a single page
void CHexEditView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	ASSERT(print_offset_.cx >= 0 && print_offset_.cy >= 0);
	pDC->SetMapMode(print_map_mode_);
	int u_scale = pDC->GetDeviceCaps(LOGPIXELSX); // units scaling (pixels/inch)
	if (theApp.print_units_ == CHexEditApp::PRN_CM)
		u_scale = int(u_scale/2.54);

	CRect rct;                              // Encloses all incl. header/footer - ie nothing is printed outside this box
	CRect margin_rct;                       // Smaller box based on margins - does not include header/footer
	CString ss;                             // String to display at top of page

	// Work out text height of the (printer) font
	pDC->SelectObject(print_font_);
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	int text_height = tm.tmHeight + tm.tmExternalLeading;

	// Work out size of the page
	int vert_res = pDC->GetDeviceCaps(VERTRES);
	int horz_res = pDC->GetDeviceCaps(HORZRES);
	if (vert_res < horz_res/5)                // May be lines of text not pixels (Eg Text Only driver)
		vert_res *= text_height;              // xxx should be device units not logical

	// Work out where the margins are
	margin_rct.top = LONG(theApp.top_margin_*u_scale);
	margin_rct.left = LONG(theApp.left_margin_*u_scale);
	margin_rct.bottom = vert_res - LONG(theApp.bottom_margin_*u_scale);
	margin_rct.right  = horz_res - LONG(theApp.right_margin_*u_scale);
	pDC->DPtoLP(&margin_rct);

	// Work out where to print the header and footer
	rct.top = LONG(theApp.header_edge_*u_scale);
	rct.left = LONG(theApp.left_margin_*u_scale);
	rct.bottom = vert_res - LONG(theApp.footer_edge_*u_scale);
	rct.right  = horz_res - LONG(theApp.right_margin_*u_scale);

	// Note we can't use ConvertFromDP here as this is for printer not screen
	pDC->DPtoLP(&rct);

	pDC->SetBkMode(TRANSPARENT);
	if (theApp.print_watermark_)
	{
		// Work out angle of diagonal from bottom left to top right
		ASSERT(rct.Height() != 0 && rct.Width() != 0);  // else we get divide by zero error
		double diag = (int)sqrt((double)(rct.Height()*rct.Height()) + rct.Width()*rct.Width());
		double angle = asin(rct.Height()/diag);

		// Create a large font at the angle of the diagonal
		int fangle = int(angle * 1800 / 3.141592 /*M_PI*/);   // convert diag angle to tenth of degrees from X-axis
		CFont fontWatermark;
		fontWatermark.CreateFont(rct.Height()/10, 0, fangle, fangle, FW_BOLD, 0, 0, 0,
								 ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
								 DEFAULT_PITCH | FF_SWISS, _T("Arial"));
		CFont * pSaved = pDC->SelectObject(&fontWatermark);

		// Create the text
		CString str = create_header(theApp.watermark_, pInfo->m_nCurPage);

		// Get length of text then work out how far from the bottom page of the corner should the text start
		double d = (diag - pDC->GetTextExtent(str).cx)/2;  // distance along diag of start of text
		int x = int(d * cos(angle));
		int y = int(d * sin(angle));

		pDC->SetTextColor(RGB(208, 208, 208));          // light grey
		pDC->SetTextAlign(TA_BASELINE);
		pDC->TextOut(x, rct.Height() - y, str);

		pDC->SetTextAlign(TA_TOP);
		(void)pDC->SelectObject(pSaved);
		fontWatermark.DeleteObject();
	}

	pDC->SetTextColor(RGB(0,0,0));          // Display headers/footers in black

	int left, mid=1, right;
	if (theApp.even_reverse_ && pInfo->m_nCurPage%2 == 0)
		left = 2, right = 0;
	else
		left = 0, right = 2;

	if (rct.top - text_height > margin_rct.top)  // Note y values are always -ve down in printer map mode
	{
		// Get normal header unless using diff header for 1st page and its the first page
		CString strHeader;
		if (theApp.diff_first_header_ && pInfo->m_nCurPage == 1)
			strHeader = theApp.first_header_;
		else
			strHeader = theApp.header_;

		// Print the 3 parts of the header
		AfxExtractSubString(ss, strHeader, left, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);

		AfxExtractSubString(ss, strHeader, mid, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);

		AfxExtractSubString(ss, strHeader, right, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_RIGHT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
	}

	if (rct.bottom + text_height < margin_rct.bottom)  // Note y values are -ve down
	{
		// Get normal footer unless using diff footer for 1st page and it's the first page
		CString strFooter;
		if (theApp.diff_first_footer_ && pInfo->m_nCurPage == 1)
			strFooter = theApp.first_footer_;
		else
			strFooter = theApp.footer_;

		// Print the 3 parts of the footer
		AfxExtractSubString(ss, strFooter, left, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_LEFT | DT_BOTTOM | DT_NOPREFIX | DT_SINGLELINE);

		AfxExtractSubString(ss, strFooter, mid, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_CENTER | DT_BOTTOM | DT_NOPREFIX | DT_SINGLELINE);

		AfxExtractSubString(ss, strFooter, right, '|');
		pDC->DrawText(create_header(ss, pInfo->m_nCurPage), &rct, DT_RIGHT | DT_BOTTOM | DT_NOPREFIX | DT_SINGLELINE);
	}

	if (theApp.print_hdr_)
	{
		// Print column headings
		rct = margin_rct + CSize(print_offset_.cx, 0);
		pDC->DrawText("Address", &rct, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		for (int ii = 0; ii < rowsize_; ++ii)
		{
			char buf[10];
			if (!display_.hex_addr)            // Probably showing dec addresses and/or line numbers so show dec hdr
				sprintf(buf, "%2d", ii%100);
			else if (theApp.hex_ucase_)
				sprintf(buf, "%02X", ii%0x100);
			else
				sprintf(buf, "%02x", ii%0x100);
			if (!display_.vert_display && display_.hex_area)
				pDC->TextOut(rct.left + hex_pos(ii, print_text_width_), rct.top, buf, 2);
			if (display_.vert_display || display_.char_area)
				pDC->TextOut(rct.left + char_pos(ii, print_text_width_, print_text_width_w_), rct.top, buf+1, 1);
		}
	}

	// Draw margins in print preview
	if (pInfo != NULL && pInfo->m_bPreview && pDC->m_hAttribDC != NULL)
	{
		CPen pen(PS_DOT, 0, RGB(0,0,0));
		CPen* ppen = pDC->SelectObject(&pen);
		pDC->MoveTo(-30000, margin_rct.top);
		pDC->LineTo(30000, margin_rct.top);
		pDC->MoveTo(margin_rct.left, -30000);
		pDC->LineTo(margin_rct.left, 30000);
		pDC->MoveTo(-30000, margin_rct.bottom);
		pDC->LineTo(30000, margin_rct.bottom);
		pDC->MoveTo(margin_rct.right, -30000);
		pDC->LineTo(margin_rct.right, 30000);
		pDC->SelectObject(ppen);
	}

	if (theApp.print_box_)
	{
		// Work out width of total printed text
		CSize size;

		rct = margin_rct + CSize(print_offset_.cx, 0);

		if (display_.char_area)
			size.cx = char_pos(rowsize_, print_text_width_, print_text_width_w_);
		else
			size.cx = hex_pos(rowsize_, print_text_width_);
		size.cy = 0;

		// Draw a box around it
		CPen pen(PS_SOLID, 0, RGB(0,0,0));
		CPen* ppen = pDC->SelectObject(&pen);
		pDC->MoveTo(rct.left - print_text_width_, rct.top);
		pDC->LineTo(rct.left + size.cx + print_text_width_, rct.top);
		pDC->LineTo(rct.left + size.cx + print_text_width_, rct.bottom);
		pDC->LineTo(rct.left - print_text_width_, rct.bottom);
		pDC->LineTo(rct.left - print_text_width_, rct.top);

#if 0
		// Draw line between address and hex areas
		size.cx = hex_pos(0, print_text_width_);
		size.cy = 0;
		pDC->MoveTo(rct.left + size.cx - print_text_width_, rct.top);
		pDC->LineTo(rct.left + size.cx - print_text_width_, rct.bottom);

		// Draw line between areas
		if (display_.hex_area && display_.char_area)
		{
			size.cx = hex_pos(rowsize_, print_text_width_);
			size.cy = 0;
			pDC->MoveTo(rct.left + size.cx - print_text_width_/2, rct.top);
			pDC->LineTo(rct.left + size.cx - print_text_width_/2, rct.bottom);
		}
#endif
		pDC->SelectObject(ppen);
	}

	// Do this last so pen changes etc do not affect header, footer etc drawing
	CScrView::OnPrint(pDC, pInfo);          // Calls OnDraw to print rest of page
}

CString CHexEditView::create_header(const char *fmt, long pagenum)
{
	bool bDiskFile = GetDocument()->pfile1_ != NULL;
	bool bDevice = bDiskFile && GetDocument()->IsDevice();
	CString retval;                     // Return string
	CString sin = fmt;                  // Rest of input string
	int pos;                            // Posn in string of param.
	CString ss;                         // Temporary string
	CHexFileList *pfl = theApp.GetFileList();
	int ii = -1;
	if (GetDocument()->pfile1_ != NULL) // make sure there is a disk file (pfl requires a disk file name)
		ii = pfl->GetIndex(GetDocument()->pfile1_->GetFilePath());

	CFileStatus status;                 // Get status of file (for times)
	if (bDiskFile && !bDevice)
		GetDocument()->pfile1_->GetStatus(status);

	while ((pos = sin.Find("&")) != -1)
	{
		retval += sin.Left(pos);
		if (sin.GetLength() > pos + 1)
		{
			switch (toupper(sin[pos+1]))
			{
			case 'F':
				if (bDiskFile)
					retval += GetDocument()->pfile1_->GetFileName();
				break;
			case 'A':
				if (bDevice)
					retval += GetDocument()->pfile1_->GetFileName();
				else if (bDiskFile)
					retval += GetDocument()->pfile1_->GetFilePath();
				break;
			case 'P':
				ss.Format("%ld", long(pagenum));
				retval += ss;
				break;
			case 'D':
				retval += print_time_.Format("%x");
				break;
			case 'T':
				retval += print_time_.Format("%X");
				break;
			case 'N':
				retval += print_time_.Format("%#c");
				break;
			case 'C':
				if (bDiskFile && !bDevice)
					retval += status.m_ctime.Format("%c");
				break;
			case 'M':
				if (bDiskFile && !bDevice)
					retval += status.m_mtime.Format("%c");
				break;
#if 0 // Since we have the file open the last access time is now so don't bother with this one
			case 'U':
				if (bDiskFile && !bDevice)
					retval += status.m_atime.Format("%c");
				break;
#endif
			case 'G':
				if (ii > -1)
					retval += pfl->GetData(ii, CHexFileList::CATEGORY);
				break;
			case 'K':
				if (ii > -1)
					retval += pfl->GetData(ii, CHexFileList::KEYWORDS);
				break;
			case 'X':
				if (ii > -1)
					retval += pfl->GetData(ii, CHexFileList::COMMENTS);
				break;
			default:
			case '&':
				retval += sin[pos+1];
			}
			sin = sin.Mid(pos+2);
		}
		else
		{
			sin.Empty();
			break;
		}
	}
	retval += sin;

	return retval;
}

void CHexEditView::OnFilePrintPreview()
{
//    CScrView::OnFilePrintPreview();
	AFXPrintPreview(this);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_preview);

	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	mm->m_wndProp.Update(NULL, -1);                   // Blank out property dlg fields
}

void CHexEditView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView)
{
	CScrView::OnEndPrintPreview(pDC, pInfo, point, pView);

	//GetFrame()->ptv_->UpdateWindow();  // try to get tabs to redisplay
}
