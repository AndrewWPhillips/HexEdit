#if !defined(AFX_TABVIEW_H__158477E7_178F_46E1_8410_3B4EA1967116__INCLUDED_)
#define AFX_TABVIEW_H__158477E7_178F_46E1_8410_3B4EA1967116__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHexTabView view

class CHexTabView : public CTabView
{
protected:
	CHexTabView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CHexTabView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHexTabView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

// Implementation
protected:
	virtual ~CHexTabView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CHexTabView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
    bool init_;       // Has OnCreate been called yet?
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABVIEW_H__158477E7_178F_46E1_8410_3B4EA1967116__INCLUDED_)
