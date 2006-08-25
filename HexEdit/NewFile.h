#if !defined(AFX_NEWFILE_H__8FE0C87B_6BB1_49D9_9D7A_E7050118E141__INCLUDED_)
#define AFX_NEWFILE_H__8FE0C87B_6BB1_49D9_9D7A_E7050118E141__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewFile.h : header file
//

#include "range_set.h"

/////////////////////////////////////////////////////////////////////////////
// CNewFile dialog

class CNewFile : public CDialog
{
// Construction
public:
	CNewFile(CWnd* pParent = NULL, bool ibmode = false);     // standard constructor

    enum { FILL_HEX, FILL_STRING, FILL_CLIPBOARD, FILL_RANDOM, FILL_NUMBER };  // Reflects the order of radio buttons

// Dialog Data
	CDecEdit    ctl_size_factor_;
	CEdit       ctl_size_length_;
	CDecEdit    ctl_decimal_size_;
	CHexEdit    ctl_hex_size_;
	CHexEdit    ctl_hex_value_;
	//{{AFX_DATA(CNewFile)
	enum { IDD = IDD_NEW_FILE };
	CButton	ctl_big_endian_;
	CEdit	ctl_fill_random_range_;
	CEdit	ctl_fill_string_value_;
	CComboBox	ctl_fill_string_charset_;
	CEdit	ctl_fill_number_value_;
	CComboBox	ctl_fill_number_type_;
	CStatic	ctl_desc_fill_string_charset_;
	CStatic	ctl_desc_fill_number_type_;
	CString	fill_random_range_;
	CString	fill_hex_value_;
	CString	fill_number_value_;
	CString	fill_string_value_;
	//}}AFX_DATA
	CEdit	ctl_fill_clipboard_sample_;
	CEdit	ctl_fill_clipboard_length_;
    CStatic ctl_desc_fill_clipboard_length_;

	bool insert_block_mode_;			// Is dialog used for insert block? (rather than new file)
    bool first_time_;                   // First time we have fixed controls?

	__int64 fill_size_;                 // New file size when size_src == 0 (later stored in fill_.size_high/low)
	__int64 fill_repeat_;               // Repeat count when size_src == 1 (later stored in fill_.size_high/low)

	// This stores all the options (except strings) selected so that we can easily save them in a macro.
	// The string (containing the hex digits, text, number, or random range string) is stored separately.
	struct fill_bits
	{
		size_t Length(const char *data_str, const char * &pcb); // Data length in bytes (data_str is text for FILL_HEX/FILL_STRING)

		// Size of file to be created (if size_src == 0), or repetitions of data (size_src == 1)
		unsigned long size_low;			// Low/high size combine to make 40 bits (Terabyte) - enough?
        unsigned int size_high: 8;

		// General options
		unsigned int type: 4;           // What fill option was selected? 0=hex, etc (see enum above)
		unsigned int create_on_disk: 1;	// Does the user want to specify a disk file on create?
		unsigned int size_src: 3;		// Where do we get the length from? 1=data, 2=calc, 0=user entered

		// Fill specific options
		unsigned int string_charset: 3; // character set. 0=ASCII, 1=EBCDIC, 2=Unicode
		unsigned int number_type: 3;    // Determines size of number (0-3 for int, 0-1 for float)
		unsigned int number_is_float: 1;// We could work this out from the numeric string, but easier to save it
		unsigned int big_endian: 1;     // Is the number big-endian?

		// 56 bits to here?

		// Total bits cannot exceed 64 so we can save all options in a macro field
	};
    union
    {
        __int64 fill_state_;			// 64 bit value that contains the options
        struct fill_bits fill_;
    };

	int clipboard_format_;      // -1 = nothing, 0 = unrecognised, 1 = binary, 2 = text
	long clipboard_length_;     // Length of the data on the clipboard

	HWND next_cb_hwnd_;         // Next window in clipboard chain

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void fix_controls();
	void calc_length();
	void check_clipboard();
    BOOL in_update_;

	// Generated message map functions
	//{{AFX_MSG(CNewFile)
	afx_msg void OnChangeFill();
	afx_msg void OnChangeDecimalSize();
	afx_msg void OnChangeHexSize();
	afx_msg void OnChangeHex();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeFillNumber();
	afx_msg void OnSelchangeFillNumberType();
	//}}AFX_MSG
	afx_msg void OnChangeString();
	afx_msg void OnSelchangeCharset();
	afx_msg void OnDestroy();
	afx_msg void OnDrawClipboard();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnChangeSizeFactor();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWFILE_H__8FE0C87B_6BB1_49D9_9D7A_E7050118E141__INCLUDED_)
