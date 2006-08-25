//
//  CSimpleSplitter
//
//  Splitter window with CWnd-derived panes
//  (C) Robert A. T. Káldy <kaldy@matfyz.cz>
//  last updated on 24.3.2004


#if !defined(SIMPLE_SPLITTER)
#define SIMPLE_SPLITTER

#define SSP_HORZ		1
#define SSP_VERT		2


class CSimpleSplitter : public CWnd
{
public:
	CSimpleSplitter(int nPanes, UINT nOrientation = SSP_HORZ, int nMinSize = 30, int nBarThickness = 3);
	~CSimpleSplitter();

	BOOL Create(CWnd* pParent, CRect rct, UINT nID = AFX_IDW_PANE_FIRST);
	BOOL CreatePane(int nIndex, CWnd* pPaneWnd, DWORD dwStyle, DWORD dwExStyle, LPCTSTR lpszClassName = NULL);
	
	int GetPaneCount() const							{ return m_nPanes; }
	void SetPane(int nIndex, CWnd* pPaneWnd);
	CWnd* GetPane(int nIndex) const;
	virtual void SetActivePane(int nIndex);
	CWnd* GetActivePane(int* pIndex) const;
	void SetPaneSizes(const int* sizes);
    int GetPaneSize(int idx) { ASSERT(idx < m_nPanes); return m_size[idx]; }
	void GetPaneRect(int nIndex, CRect& rcPane) const;
	void GetBarRect(int nIndex, CRect& rcBar) const;
    void Flip();
    void SetOrientation(UINT orient) { if (orient != m_nOrientation) Flip(); }
    UINT Orientation() { return m_nOrientation; }

protected:
	void RecalcLayout();
	void ResizePanes();
	void InvertTracker();
	
	//{{AFX_MSG(CSimpleSplitter)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	DECLARE_MESSAGE_MAP()

protected:
	const int m_nPanes;
	UINT m_nOrientation;
	int m_nMinSize, m_nBarThickness;

	int m_nTrackIndex, m_nTracker, m_nTrackerLength, m_nTrackerMouseOffset;
	
	CWnd** m_pane;
	int *m_size, *m_orig;
};


#endif
