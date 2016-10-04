// Preview.h - header for CPreview class
//
// Copyright (c) 2016 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#pragma once
// CPreview - used to display preview bitmap in file open dialog

#include <FreeImage.h>

class CPreview : public CStatic
{
	DECLARE_DYNAMIC(CPreview)

public:
	CPreview();
	~CPreview();

	void SetBackground(COLORREF clr) { m_clr = clr; }
	void SetFile(const char * fname);

protected:
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()

private:
	FIBITMAP * m_dib;     // Free Image bitmap that holds the thumbnail
	COLORREF m_clr;
	CString m_fname;
};
