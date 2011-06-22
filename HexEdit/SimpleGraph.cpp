// SimpleGraph.cpp : implementation file
//

#include "stdafx.h"
#include "SimpleGraph.h"

// CSimpleGraph

IMPLEMENT_DYNAMIC(CSimpleGraph, CStatic)

CSimpleGraph::CSimpleGraph()
{
	m_pdc = NULL;
}

void CSimpleGraph::SetData(FILE_ADDRESS maximum, std::vector<FILE_ADDRESS> val, std::vector<COLORREF> col)
{
	if (val == m_val && col == m_col)
		return;                         // no change

	ASSERT(val.size() == col.size());
	m_max = maximum;
	m_val.swap(val);
	m_col.swap(col);

	update();
}

void CSimpleGraph::update()
{
	CRect rct;
	GetClientRect(rct);

	// If the graph has been resized we need a new DC/bitmap
	if (m_rct != rct)
	{
		// Delete old DC/bitmap
		if (m_pdc != NULL)
		{
			delete m_pdc;
			m_pdc = NULL;
			m_bm.DeleteObject();
		}
		// Recreate DC/bitmap for drawing into
		CDC* pDC = GetDC();
		m_pdc = new CDC();
		m_pdc->CreateCompatibleDC(pDC);
		m_bm.CreateCompatibleBitmap(pDC, rct.Width(), rct.Height());
		m_pdc->SelectObject(&m_bm);

		m_rct = rct;
	}

	COLORREF back = RGB(255, 255, 255);
	COLORREF axes = RGB(0, 0, 0);

	int off_left = 10, off_right = 2;
	int off_top = 2, off_bottom = 6;

	// Fill background
	m_pdc->FillSolidRect(0, 0, rct.Width(), rct.Height(), back);

	// Draw axes

	// Draw bars of graph
	ASSERT(m_val.size() == m_col.size());
	int width = rct.Width() - off_left - off_right;
	int height = rct.Height() - off_top - off_bottom;
	int ypos = rct.Height() - off_bottom;
	for (int ii = 0; ii < m_val.size(); ++ii)
	{
		int xpos = off_left + (width*ii)/m_val.size();
		int cy = int((m_val[ii]*height)/m_max);

		m_pdc->FillSolidRect(xpos, ypos - cy, width/m_val.size(), cy, m_col[ii]);
	}

	Invalidate();
}

BEGIN_MESSAGE_MAP(CSimpleGraph, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

// CSimpleGraph message handlers
void CSimpleGraph::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect rct;
	GetClientRect(rct);

	// Just BLT the current graph into the display
	dc.BitBlt(rct.left, rct.top, rct.Width(), rct.Height(), m_pdc, 0, 0, SRCCOPY);
}
