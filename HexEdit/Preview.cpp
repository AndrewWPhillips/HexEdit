// Preview.cpp : show preview window (open dlg or recent file list)
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"

#ifdef FILE_PREVIEW
#include "Preview.h"
//#include "misc.h"

// CPreview

IMPLEMENT_DYNAMIC(CPreview, CStatic)

CPreview::CPreview() : m_dib(NULL)
{
}

CPreview::~CPreview()
{
	if (m_dib != NULL)
	{
		FreeImage_Unload(m_dib);
		m_dib = NULL;
	}
}

void CPreview::SetFile(const char * fname)
{
	if (fname == m_fname) return;   // avoid flicker by continually redisplaying the same file
	m_fname = fname;

	if (m_dib != NULL)
	{
		FreeImage_Unload(m_dib);
		m_dib = NULL;
	}

	// If a file name was given
	if (fname != NULL && fname[0] != '\0')
	{
		FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(fname);  // Try to work out the file type
		m_dib = FreeImage_Load(fmt, fname);
	}

	Invalidate();
}

BEGIN_MESSAGE_MAP(CPreview, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

// CPreview message handlers
void CPreview::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect rct;
	GetClientRect(&rct);

	dc.FillSolidRect(&rct, m_clr);   // Fill background first

	if (m_dib != NULL)
	{
		// Draw the thumbnail
		int width = (int)(FreeImage_GetWidth(m_dib)*theApp.thumb_zoom_);
		if (width > rct.Width()) width = rct.Width();
		int height = (int)(FreeImage_GetHeight(m_dib)*theApp.thumb_zoom_);
		if (height > rct.Height()) height = rct.Height();
		int left = (rct.Width() - width)/2;
		if (left < 0) left = 0;
		::StretchDIBits(dc.GetSafeHdc(),
					left, 0, width, height,
					0, 0, FreeImage_GetWidth(m_dib), FreeImage_GetHeight(m_dib),
					FreeImage_GetBits(m_dib), FreeImage_GetInfo(m_dib), DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		// Just draw some text to say there is no preview
		dc.SetTextAlign(TA_CENTER);
		dc.TextOut(rct.Width()/2, 20, "No Preview      ", 12);
		dc.TextOut(rct.Width()/2, 40, "Available       ", 12);
	}
}
#endif // FILE_PREVIEW

