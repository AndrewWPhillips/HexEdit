// GridCellDateTime.h: interface for the CGridCellDateTime class.
//
// Provides the implementation for a datetime picker cell type of the
// grid control.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATETIMECELL_H__A0B7DA0A_0AFE_4D28_A00E_846C96D7507A__INCLUDED_)
#define AFX_DATETIMECELL_H__A0B7DA0A_0AFE_4D28_A00E_846C96D7507A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../GridCtrl_src/GridCell.h"
#include "afxdtctl.h"	// for CDateTimeCtrl

class CGridCellDateTime : public CGridCell
{
  friend class CGridCtrl;
  DECLARE_DYNCREATE(CGridCellDateTime)

public:
	CGridCellDateTime();

  // editing cells
public:
  void Init(COleDateTime odt, LPCTSTR format);
  virtual BOOL  Edit(int nRow, int nCol, CRect rect, CPoint point, UINT nID, 
UINT nChar);
  virtual CWnd* GetEditWnd() const;
  virtual void  EndEdit();

  COleDateTime GetTime() { return m_odt; }
  void SetTime(COleDateTime odt) { m_odt = odt; }

private:
  COleDateTime m_odt;    // stores the date/time
  CString m_format;
};

class CInPlaceDateTime : public CDateTimeCtrl
{
// Construction
public:
	CInPlaceDateTime(CWnd* pParent,       // parent
                   CRect& rect,           // dimensions & location
                   UINT nID,              // control ID
                   int nRow, int nColumn, // row and column
                   COLORREF crFore, COLORREF crBack,  // Foreground, background colour
          		   UINT nFirstChar,      // first character to pass to control
				   LPCTSTR format,       // Date format string
				   CGridCellDateTime *); // cell which we are editing

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceList)
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
    void EndEdit();

// Generated message map functions
protected:
	//{{AFX_MSG(CInPlaceList)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	//}}AFX_MSG
	//afx_msg void OnSelendOK();

	DECLARE_MESSAGE_MAP()

private:
	CGridCellDateTime *m_pcell;
	int		 m_nRow, m_nCol;
    COLORREF m_crForeClr, m_crBackClr;
	UINT     m_nLastChar;
	BOOL	 m_bExitOnArrows;
};

#endif // !defined(AFX_DATETIMECELL_H__A0B7DA0A_0AFE_4D28_A00E_846C96D7507A__INCLUDED_)
