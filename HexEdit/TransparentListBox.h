#if !defined(AFX_LSTRANSLISTBOX_H__1DE15F21_6DE3_11D4_8CDF_0008C73F82B8__INCLUDED_)
#define AFX_LSTRANSLISTBOX_H__1DE15F21_6DE3_11D4_8CDF_0008C73F82B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CTransparentListBox.h : header file
//

//#include "LSRgnBmpScrollButton.h"
#include <afxtempl.h>

/////////////////////////////////////////////////////////////////////////////
// CTransparentListBox window
typedef CMap<long,long,long,long> CMapWordToWord;

class CTransparentListBox : public CListBox
{
	// Construction
public:
	CTransparentListBox();

	// Attributes
public:

	// Operations
public:
	void SetFont(int nPointSize, CString FaceName,COLORREF Color,COLORREF SelColor,BOOL Shadow=TRUE,int SOffset = 2,COLORREF ShadowColor = RGB(0,0,0));
	void SetColor(COLORREF Color,COLORREF SelColor,COLORREF ShadowColor = RGB(0,0,0));

	BOOL SetTopIndex(int Index);
	int ScrollUp(int Lines=1);
	int ScrollDown(int Lines=1);
	int AddString(CString Text,DWORD ItemData = 0,BOOL Enabled = TRUE);
	int InsertString(int Index,CString Text, DWORD ItemData = 0,BOOL Enabled = TRUE);
	virtual void ResetContent(BOOL bInvalidate = TRUE);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransparentListBox)
protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CTransparentListBox();

	// Generated message map functions
protected:
	int            m_ItemHeight;
	COLORREF       m_Color;
	COLORREF       m_SelColor;
	COLORREF       m_ShadowColor;
	CFont          m_Font;
	CBitmap        m_Bmp;
	BOOL           m_HasBackGround;
	int            m_ShadowOffset;
	BOOL           m_Shadow;
	long           m_PointSize;


	virtual CFont *GetFont();
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
	virtual void DrawItem(CDC &Dc,int Index,CRect &Rect,BOOL Selected);


	//{{AFX_MSG(CTransparentListBox)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnLbnSelchange();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSTRANSLISTBOX_H__1DE15F21_6DE3_11D4_8CDF_0008C73F82B8__INCLUDED_)

