// tipwnd.h
#pragma once


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

    void SetTextCol(DWORD col) { m_text_colour = col; }
    void SetBgCol(DWORD col)   { m_bg_colour = col; }
	void SetAlpha(int alpha);

	bool FadingIn()  { return m_in; }
	bool FadingOut() { return m_out; }

protected:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LRESULT OnSetText(WPARAM wp, LPARAM lp);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnMouseLeave(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()

private:
	UINT  m_fmt;                // Format flag passed to DrawText
	CSize m_margins;            // Default margins around the text

    DWORD m_bg_colour;          // Colour of background
	DWORD m_text_colour;        // Colour of text
	bool  m_visible;            // Is window visible or about to be?

    void track_mouse(unsigned long); // Turns on receipt of mouse hover/leave messages
	static int m_2k;            // Are we using 2K or later (XP etc)?
    static PFSetLayeredWindowAttributes m_pSLWAfunc;  // Ptr to SetLayeredWindowAttributes API function
	int   m_alpha;              // Window transparency (0-255)

	int   m_fade;               // Current fade level
	int   m_fade_inc;           // How much to fade every timer event

	bool  m_in, m_out;          // Remember if we are fading in or out
};
