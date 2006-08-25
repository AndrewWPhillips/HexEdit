#include "stdafx.h"
#include "SimpleSplitter.h"


#define FULL_SIZE 32768


inline int MulDivRound(int x, int mul, int div)
{
	return (x * mul + div / 2) / div;
}


BEGIN_MESSAGE_MAP(CSimpleSplitter, CWnd)
	//{{AFX_MSG_MAP(CSimpleSplitter)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_NCCREATE()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CSimpleSplitter::CSimpleSplitter(int nPanes, UINT nOrientation, int nMinSize, int nBarThickness):
	m_nPanes(nPanes),
	m_nOrientation(nOrientation),
	m_nMinSize(nMinSize), 
	m_nBarThickness(nBarThickness)
{
	int total = 0;

	ASSERT(nPanes > 0);
	ASSERT(nOrientation == SSP_HORZ || nOrientation == SSP_VERT);
	ASSERT(nMinSize >= 0);
	ASSERT(nBarThickness >= 0);

	m_pane = new CWnd*[m_nPanes];
	m_size = new int[m_nPanes];
	m_orig = new int[m_nPanes + 1];
	::ZeroMemory(m_pane, m_nPanes * sizeof(CWnd*));
	
	for (int i = 0; i < m_nPanes - 1; i++)			// default, set equal size to all panes
	{
		m_size[i] = (FULL_SIZE + m_nPanes / 2) / m_nPanes;
		total += m_size[i];
	}
	m_size[m_nPanes - 1] = FULL_SIZE - total;
}

CSimpleSplitter::~CSimpleSplitter()
{
	delete[] m_pane;
	delete[] m_size;
	delete[] m_orig;
}

BOOL CSimpleSplitter::Create(CWnd* pParent, CRect rct, UINT nID)
{
	ASSERT(pParent);
//	pParent->GetClientRect(rcOuter);
//	HCURSOR crsResize = ::LoadCursor(0, MAKEINTRESOURCE(m_nOrientation == SSP_HORZ ? AFX_IDC_HSPLITBAR : AFX_IDC_VSPLITBAR));
//	if (crsResize == NULL)
//		crsResize = ::LoadCursor(0, m_nOrientation == SSP_HORZ ? IDC_SIZEWE : IDC_SIZENS);
	return CreateEx(WS_EX_CLIENTEDGE, 
                    AfxRegisterWndClass(CS_DBLCLKS, HCURSOR(0), NULL, NULL),
                    NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER,
                    rct,
                    pParent,
                    nID,
                    NULL);
}

BOOL CSimpleSplitter::CreatePane(int nIndex, CWnd* pPaneWnd, DWORD dwStyle, DWORD dwExStyle, LPCTSTR lpszClassName)
{
	CRect rcPane;

	ASSERT((nIndex >= 0) && (nIndex < m_nPanes));
	m_pane[nIndex] = pPaneWnd;
	dwStyle |= WS_CHILD | WS_VISIBLE;
	GetPaneRect(nIndex, rcPane);
	return pPaneWnd->CreateEx(dwExStyle, lpszClassName, NULL, dwStyle, rcPane, this, AFX_IDW_PANE_FIRST + nIndex);
}

void CSimpleSplitter::SetPane(int nIndex, CWnd* pPaneWnd)
{
	CRect rcPane;

	ASSERT((nIndex >= 0) && (nIndex < m_nPanes));
	ASSERT(pPaneWnd);
	ASSERT(pPaneWnd->m_hWnd);

	m_pane[nIndex] = pPaneWnd;
	GetPaneRect(nIndex, rcPane);
	pPaneWnd->MoveWindow(rcPane, false);
}

CWnd* CSimpleSplitter::GetPane(int nIndex) const
{
	ASSERT((nIndex >= 0) && (nIndex < m_nPanes));
	return m_pane[nIndex];
}

void CSimpleSplitter::SetActivePane(int nIndex)
{
	ASSERT((nIndex >= 0) && (nIndex < m_nPanes));
	m_pane[nIndex]->SetFocus();
}

CWnd* CSimpleSplitter::GetActivePane(int* pIndex) const
{
	for (int i = 0; i < m_nPanes; i++)
		if (m_pane[i]->GetFocus())
		{
			*pIndex = i;
			return m_pane[i];
		}
	return NULL;
}

void CSimpleSplitter::SetPaneSizes(const int* sizes)
{
	int i, total = 0, total_in = 0;
	
	for (i = 0; i < m_nPanes; i++)
	{
		ASSERT(sizes[i] >= 0);
		total += sizes[i];
	}
	for (i = 0; i < m_nPanes - 1; i++)
	{
		m_size[i] = MulDivRound(sizes[i], FULL_SIZE, total);
		total_in += m_size[i];
	}
	m_size[m_nPanes - 1] = FULL_SIZE - total_in;
	RecalcLayout();
	ResizePanes();
}

void CSimpleSplitter::GetPaneRect(int nIndex, CRect& rcPane) const
{
	ASSERT(nIndex >= 0 && nIndex < m_nPanes);
	GetClientRect(&rcPane);
	if (m_nOrientation == SSP_HORZ)
	{
		rcPane.left = m_orig[nIndex];
		rcPane.right = m_orig[nIndex + 1] - m_nBarThickness;
	}
	else
	{
		rcPane.top = m_orig[nIndex];
		rcPane.bottom = m_orig[nIndex + 1] - m_nBarThickness;
	}
}

void CSimpleSplitter::GetBarRect(int nIndex, CRect& rcBar) const
{
	ASSERT(nIndex > 0 && nIndex < m_nPanes);
	GetClientRect(&rcBar);
	if (m_nOrientation == SSP_HORZ)
	{
		rcBar.left = m_orig[nIndex];
		rcBar.right = m_orig[nIndex + 1] - m_nBarThickness;
	}
	else
	{
		rcBar.top = m_orig[nIndex];
		rcBar.bottom = m_orig[nIndex + 1] - m_nBarThickness;
	}
}

void CSimpleSplitter::Flip()
{
    double factor;
    CRect rct;
    GetClientRect(&rct);
    if (m_nOrientation == SSP_HORZ)
    {
        m_nOrientation = SSP_VERT;
        factor = (double)rct.Height() / rct.Width();
    }
    else
    {
        m_nOrientation = SSP_HORZ;
        factor = (double)rct.Width() / rct.Height();
    }

    int i;
	for (i = 1; i <= m_nPanes; i++)
        m_orig[i] = int(factor*m_orig[i]);
	for (i = 0; i < m_nPanes; i++)
        m_size[i] = int(factor*m_size[i]);
    m_nMinSize = int(factor*m_nMinSize);

	RecalcLayout();
	ResizePanes();
}

// CSimpleSplitter protected

void CSimpleSplitter::RecalcLayout()
{
	int i, size_sum, remain, remain_new = 0;
	bool bGrow = true;
	CRect rcOuter;

	GetClientRect(rcOuter);
	size_sum = m_nOrientation == SSP_HORZ ? rcOuter.Width() : rcOuter.Height();
	size_sum -= (m_nPanes - 1) * m_nBarThickness;

	while (bGrow)									// adjust sizes on the beginning
	{												// and while we have growed something
		bGrow = false;
		remain = remain_new = FULL_SIZE;
		for (i = 0; i < m_nPanes; i++)				// grow small panes to minimal size
			if (MulDivRound(m_size[i], size_sum, FULL_SIZE) <= m_nMinSize)
			{
				remain -= m_size[i];
				if (MulDivRound(m_size[i], size_sum, FULL_SIZE) < m_nMinSize)
				{
					if (m_nMinSize > size_sum)
						m_size[i] = FULL_SIZE;
					else
						m_size[i] = MulDivRound(m_nMinSize, FULL_SIZE, size_sum);
					bGrow = true;
				}
				remain_new -= m_size[i];
			}
		if (remain_new <= 0)						// if there isn't place to all panes
		{											// set the minimal size to the leftmost/topmost
			remain = FULL_SIZE;						// and set zero size to the remainimg
			for (i = 0; i < m_nPanes; i++)
			{
				if (size_sum == 0)
					m_size[i] = 0;
				else
					m_size[i] = MulDivRound(m_nMinSize, FULL_SIZE, size_sum);
				if (m_size[i] > remain)
					m_size[i] = remain;
				remain -= m_size[i];
			}
			break;
		}
		if (remain_new != FULL_SIZE)				// adjust other pane sizes, if we have growed some
			for (i = 0; i < m_nPanes; i++)
				if (MulDivRound(m_size[i], size_sum, FULL_SIZE) != m_nMinSize)
					m_size[i] = MulDivRound(m_size[i], remain_new, remain);
	}

	m_orig[0] = 0;									// calculate positions (in pixels) from relative sizes
	for (i = 0; i < m_nPanes - 1; i++)
		m_orig[i + 1] = m_orig[i] + MulDivRound(m_size[i], size_sum, FULL_SIZE) + m_nBarThickness;
	m_orig[m_nPanes] = size_sum + m_nBarThickness * m_nPanes;
}

void CSimpleSplitter::ResizePanes()
{
	int i;
	CRect rcOuter;

	GetClientRect(rcOuter);
	if (m_nOrientation == SSP_HORZ)	
		for (i = 0; i < m_nPanes; i++)
		{
			if (m_pane[i])
				m_pane[i]->MoveWindow(m_orig[i], 0, m_orig[i + 1] - m_orig[i] - m_nBarThickness, rcOuter.Height());
		}
	else
		for (i = 0; i < m_nPanes; i++)
		{
			if (m_pane[i])
				m_pane[i]->MoveWindow(0, m_orig[i], rcOuter.Width(), m_orig[i + 1] - m_orig[i] - m_nBarThickness);
		}
}

void CSimpleSplitter::InvertTracker()
{
	CDC* pDC = GetDC();
	CBrush* pBrush = CDC::GetHalftoneBrush();
	HBRUSH hOldBrush;
	
	hOldBrush = (HBRUSH)SelectObject(pDC->m_hDC, pBrush->m_hObject);
	if (m_nOrientation == SSP_HORZ)
		pDC->PatBlt(m_nTracker - m_nBarThickness, 0, m_nBarThickness, m_nTrackerLength, PATINVERT);
	else
		pDC->PatBlt(0, m_nTracker - m_nBarThickness, m_nTrackerLength, m_nBarThickness, PATINVERT);
	if (hOldBrush != NULL)
		SelectObject(pDC->m_hDC, hOldBrush);
	ReleaseDC(pDC);
}

// CSimpleSplitter messages

void CSimpleSplitter::OnPaint() 
{
	CPaintDC dc(this);
	CRect rcPaint = dc.m_ps.rcPaint;
	COLORREF clrBar = ::GetSysColor(COLOR_BTNFACE);
	int i, x = 0;

	if (m_nOrientation == SSP_HORZ)
		for (i = 1; i < m_nPanes; i++)
			dc.FillSolidRect(m_orig[i] - m_nBarThickness, rcPaint.top, m_nBarThickness, rcPaint.Height(), clrBar);
	else
		for (i = 1; i < m_nPanes; i++)
			dc.FillSolidRect(rcPaint.left, m_orig[i] - m_nBarThickness, rcPaint.Width(), m_nBarThickness, clrBar);
}

BOOL CSimpleSplitter::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    if (pWnd != this)
        return FALSE;

    SetCursor(AfxGetApp()->LoadStandardCursor(m_nOrientation == SSP_HORZ ? IDC_SIZEWE : IDC_SIZENS));
    return TRUE;
}

void CSimpleSplitter::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	RecalcLayout();
	ResizePanes();
}

void CSimpleSplitter::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect rcClient;
	int mouse_pos = m_nOrientation == SSP_HORZ ? point.x : point.y;

	SetCapture();
	for (m_nTrackIndex = 1; (m_nTrackIndex < m_nPanes && m_orig[m_nTrackIndex] < mouse_pos); m_nTrackIndex++);
	m_nTracker = m_orig[m_nTrackIndex];
	m_nTrackerMouseOffset = mouse_pos - m_nTracker;
	GetClientRect(&rcClient);
	m_nTrackerLength = m_nOrientation == SSP_HORZ ? rcClient.Height() : rcClient.Width();
	InvertTracker();
}

void CSimpleSplitter::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (GetCapture() != this) 
		return;

	CRect rcOuter;
	int size_sum;

	GetClientRect(rcOuter);
	size_sum = m_nOrientation == SSP_HORZ ? rcOuter.Width() : rcOuter.Height();
	size_sum -= (m_nPanes - 1) * m_nBarThickness;

	InvertTracker();
	ReleaseCapture();
	m_orig[m_nTrackIndex] = m_nTracker;
	m_size[m_nTrackIndex - 1] = MulDivRound(m_orig[m_nTrackIndex] - m_orig[m_nTrackIndex - 1] - m_nBarThickness, FULL_SIZE, size_sum);
	m_size[m_nTrackIndex]     = MulDivRound(m_orig[m_nTrackIndex + 1] - m_orig[m_nTrackIndex] - m_nBarThickness, FULL_SIZE, size_sum);
	ResizePanes();
}

void CSimpleSplitter::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (GetCapture() != this) 
		return;
	InvertTracker();
	m_nTracker = (m_nOrientation == SSP_HORZ ? point.x : point.y) - m_nTrackerMouseOffset;
	if (m_nTracker > m_orig[m_nTrackIndex + 1] - m_nBarThickness - m_nMinSize)
		m_nTracker = m_orig[m_nTrackIndex + 1] - m_nBarThickness - m_nMinSize;
	else if (m_nTracker < m_orig[m_nTrackIndex - 1] + m_nBarThickness + m_nMinSize)
		m_nTracker = m_orig[m_nTrackIndex - 1] + m_nBarThickness + m_nMinSize;
	InvertTracker();
}

BOOL CSimpleSplitter::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CWnd::OnNcCreate(lpCreateStruct))
		return FALSE;

	CWnd* pParent = GetParent();
	ASSERT_VALID(pParent);
	pParent->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_DRAWFRAME);
	return TRUE;
}

void CSimpleSplitter::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	lpwndpos->flags |= SWP_NOCOPYBITS;
	CWnd::OnWindowPosChanging(lpwndpos);
}

