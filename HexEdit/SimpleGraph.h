#pragma once


// CSimpleGraph

class CSimpleGraph : public CStatic
{
	DECLARE_DYNAMIC(CSimpleGraph)

public:
	CSimpleGraph();

	void SetData(FILE_ADDRESS, std::vector<FILE_ADDRESS> val, std::vector<COLORREF> col); 

protected:
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()

private:
	CDC * m_pdc;
    CBitmap m_bm;
	CRect m_rct;

	FILE_ADDRESS m_max;
	std::vector<FILE_ADDRESS> m_val;
	std::vector<COLORREF> m_col;

	void update();
};
