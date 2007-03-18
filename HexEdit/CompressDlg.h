#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CCompressDlg dialog

class CCompressDlg : public CDialog
{
	DECLARE_DYNAMIC(CCompressDlg)

public:
	CCompressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompressDlg();

// Dialog Data
	enum { IDD = IDD_COMPRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_defaultLevel, m_defaultWindow, m_defaultMemory;
	UINT m_level;
	UINT m_window;
	UINT m_memory;
	UINT m_sync;
	int m_headerType, m_strategy;

	CEdit m_ctlLevel;
	CSpinButtonCtrl m_ctlLevelSpin;
	CEdit m_ctlWindow;
	CSpinButtonCtrl m_ctlWindowSpin;
	CEdit m_ctlMemory;
	CSpinButtonCtrl m_ctlMemorySpin;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCompressionLevelDefault();
	afx_msg void OnBnClickedCompressionWindowSizeDefault();
	afx_msg void OnBnClickedCompressionMemoryUsageDefault();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

private:
	void FixControls();
public:
	afx_msg void OnBnClickedCompressHelp();
};
