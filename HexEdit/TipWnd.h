// tipwnd.h
#pragma once

#include <vector>

// CTipWnd

// We have to load a ptr to KERNEL32.DLL API function SetLayeredWindowAttributes
// manually since it is only supported on W2K/XP (not 9X, ME, NT4)
typedef BOOL (__stdcall *PFSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);

class CTipWnd : public CWnd
{
	DECLARE_DYNAMIC(CTipWnd)
	friend class CHexEditView;

public:
	CTipWnd(UINT fmt = DT_NOCLIP|DT_NOPREFIX|DT_EXPANDTABS);

	void Show(int delay = 0, int fade = 200);
	void Hide(int fade = 200);

	void Move(CPoint pt, bool centre = true);

	void SetTextCol(COLORREF col) { m_text_colour = col; }
	void SetBgCol(COLORREF col)   { m_bg_colour = col; }
	void SetAlpha(int alpha);
	void SetStockFont(int idx) { m_stock_font = idx; }

	bool FadingIn()  { return m_in; }
	bool FadingOut() { return m_out; }

	void Clear();
#if _MSC_VER >= 1300
	void AddString(LPCTSTR ss, COLORREF col = -1, CPoint * ppt = NULL, UINT fmt = 0)
		{ AddString(CStringW(ss), col, ppt, fmt); }
	void AddString(LPCWSTR ss, COLORREF col = -1, CPoint * ppt = NULL, UINT fmt = 0);
#else
	void AddString(LPCTSTR ss, COLORREF col = -1, CPoint * ppt = NULL, UINT fmt = 0);
#endif
	size_t Count() { return str_.size(); }
	CRect GetRect(size_t idx) { ASSERT(idx<rct_.size()); return rct_[idx]; }
	CRect GetTotalRect() { return rct_all_; }

protected:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LRESULT OnSetText(WPARAM wp, LPARAM lp);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnMouseLeave(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()

private:
	UINT  m_fmt;                // Format flag passed to DrawText
	CSize m_margins;            // Default margins around the text

	COLORREF m_bg_colour;          // Colour of background
	COLORREF m_text_colour;        // Colour of text
	int m_stock_font;
	bool  m_visible;            // Is window visible or about to be?
	bool  m_down;               // Is mouse down?
	CPoint m_down_pt;           // Point when left mouse button went down

#if _MSC_VER >= 1300
	typedef CStringW StringType;
#else
	typedef CString StringType;
#endif
	std::vector<StringType> str_;
	std::vector<COLORREF> col_;
	std::vector<UINT> fmt_;
	std::vector<CRect> rct_;

	CRect rct_all_;

	void track_mouse(unsigned long); // Turns on receipt of mouse hover/leave messages
	static int m_2k;            // Are we using 2K or later (XP etc)?
	static PFSetLayeredWindowAttributes m_pSLWAfunc;  // Ptr to SetLayeredWindowAttributes API function
	int   m_alpha;              // Window transparency (0-255)

	int   m_fade;               // Current fade level
	int   m_fade_inc;           // How much to fade every timer event

	bool  m_in, m_out;          // Remember if we are fading in or out
	bool  m_hovering;           // Is the mouse hovering on our window?
};
