// Preview.h - header for CPreview class

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
