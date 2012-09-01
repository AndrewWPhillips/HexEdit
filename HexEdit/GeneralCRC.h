#pragma once

#include <vector>
#include "misc.h"

// CGeneralCRC dialog - accepts parameters for general CRC calculations
class CGeneralCRC : public CDialog
{
	DECLARE_DYNAMIC(CGeneralCRC)

public:
	CGeneralCRC(CWnd* pParent = NULL);   // standard constructor

	CString SaveParams();
	void LoadParams(LPCTSTR params);

// Dialog Data
	enum { IDD = IDD_CRC };

	CComboBox          ctl_bits_;
	CMFCMenuButton     select_menu_;
	CHexEdit           ctl_poly_;
	CHexEdit           ctl_final_xor_;
	CHexEdit           ctl_init_rem_;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnCbnSelchangeCrcBits();
	afx_msg void OnChangeName();
	afx_msg void OnChange();
	afx_msg void OnSelect();
	afx_msg void OnSave();
	afx_msg void OnDelete();
	DECLARE_MESSAGE_MAP()

private:
	int find_name(LPCTSTR name); // Searches for settings name and returns idx into settings_ (or -1 if not found)

	static const char * reg_locn;
	std::vector<CString> settings_; // Settings for predefined algorithms
	int last_predefined_;      // Index one past last predefined CRC settings (= index of first user-defined setting) 
	int last_selected_;        // Index of last item selected or -1 if changes have been made to params since last selection

	CString	note_;             // Just used to display a bit more info on CRCs
	int bits_idx_;             // index into IDC_CRC_BITS drop down - which indicates how many bits for the CRC to use
	struct crc_params par_;    // raw parameters for CRC calcs 
	unsigned __int64  max_;    // This is the max value for poly_, final_xor_, init_rem_ - it depends on the value for bits_
};
