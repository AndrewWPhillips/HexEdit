// Dialog.h - header for misc. dialogs
//
// Copyright (c) 1999-2010 by Andrew W. Phillips.
//
// No restrictions are placed on the noncommercial use of this code,
// as long as this text (from the above copyright notice to the
// disclaimer below) is preserved.
//
// This code may be redistributed as long as it remains unmodified
// and is not sold for profit without the author's written consent.
//
// This code, or any part of it, may not be used in any software that
// is sold for profit, without the author's written consent.
//
// DISCLAIMER: This file is provided "as is" with no expressed or
// implied warranty. The author accepts no liability for any damage
// or loss of business that this product may cause.
//

#if !defined(AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_)
#define AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HexEdit.h"
#include "HelpID.hm"
#include "HexEditView.h"

class CHexEditApp;

/////////////////////////////////////////////////////////////////////////////
// CHexFileDialog - just adds the facility to CFileDialog for saving and restoring
// the window size and position.  Used as base class for other file dialogs.
class CHexFileDialog : public CFileDialog
{
public:
	CHexFileDialog(LPCTSTR name,
				   DWORD help_id,
				   BOOL bOpenFileDialog,
				   LPCTSTR lpszDefExt = NULL,
				   LPCTSTR lpszFileName = NULL,
				   DWORD dwFlags = OFN_HIDEREADONLY  | OFN_OVERWRITEPROMPT,
				   LPCTSTR lpszFilter = NULL,
				   LPCTSTR lpszOKButtonName = NULL,
				   CWnd* pParentWnd = NULL) : 
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags | OFN_ENABLESIZING, lpszFilter, pParentWnd, 0, FALSE),
		strName(name), strOKName(lpszOKButtonName)
	{
		first_time = true;
		hid_last_file_dialog = help_id;
	}

	// Overriden members of CFileDialog
	virtual void OnInitDone()
	{
		if (!strOKName.IsEmpty())
			SetControlText(IDOK, strOKName);
		CFileDialog::OnInitDone();
	}
	virtual BOOL OnFileNameOK()
	{
		CRect rr;
		GetParent()->GetWindowRect(&rr);
		theApp.WriteProfileInt("Window-Settings", strName+"X1", rr.left);
		theApp.WriteProfileInt("Window-Settings", strName+"Y1", rr.top);
		theApp.WriteProfileInt("Window-Settings", strName+"X2", rr.right);
		theApp.WriteProfileInt("Window-Settings", strName+"Y2", rr.bottom);

		ASSERT(GetParent() != NULL);
		CWnd *psdv  = FindWindowEx(GetParent()->m_hWnd, NULL, "SHELLDLL_DefView", NULL);
		ASSERT(psdv != NULL);
		CWnd *plv   = FindWindowEx(psdv->m_hWnd, NULL, "SysListView32", NULL);
		ASSERT(plv != NULL);

		int mode = 0;
		switch (plv->SendMessage(LVM_FIRST + 143 /*LVM_GETVIEW*/))
		{
		case LVS_ICON:
		case LVS_SMALLICON:
			mode = ICON;
			break;
		case LVS_REPORT:
			mode = REPORT;
			break;
		case LVS_LIST:
			mode = LIST;
			break;
		default:
			mode = TILE;
			break;
		}
		theApp.WriteProfileInt("Window-Settings", strName+"Mode", mode);

		return CFileDialog::OnFileNameOK();
	}
	virtual void OnFileNameChange()
	{
		if (first_time)
		{
			// We put this stuff here as it appears to be too early when just in OnInitDone

			// Restore the window position and size
			CRect rr(theApp.GetProfileInt("Window-Settings", strName+"X1", -30000),
					 theApp.GetProfileInt("Window-Settings", strName+"Y1", -30000),
					 theApp.GetProfileInt("Window-Settings", strName+"X2", -30000),
					 theApp.GetProfileInt("Window-Settings", strName+"Y2", -30000));
			if (rr.top != -30000)
				GetParent()->MoveWindow(&rr);  // Note: there was a crash here until we set 8th
											   // param of CFileDialog (bVistaStyle) c'tor to FALSE.

			// Restore the list view display (details, report, icons, etc)
			ASSERT(GetParent() != NULL);
			CWnd *psdv  = FindWindowEx(GetParent()->m_hWnd, NULL, "SHELLDLL_DefView", NULL);
			if (psdv != NULL)
			{
				int mode = theApp.GetProfileInt("Window-Settings", strName+"Mode", REPORT);
				psdv->SendMessage(WM_COMMAND, mode, 0);
			}

			first_time = false;
		}

		CFileDialog::OnFileNameChange();
	}

private:
	enum ListViewMode
	{
		ICON     = 0x7029,
		LIST     = 0x702B,
		REPORT   = 0x702C,
		THUMBNAIL= 0x702D,
		TILE     = 0x702E,
	};

	CString strName;                    // Registry entries are based on this name
	CString strOKName;                  // Text to put on the OK button
	bool first_time;                    // Only restore window on first call of OnFileNameChange
};

// CImportDialog - derived from CFileDialog to support extra controls
class CImportDialog : public CHexFileDialog
{
public:
	CImportDialog(LPCTSTR lpszFileName = NULL,
				  DWORD help_id = HIDD_FILE_IMPORT_MOTOROLA,
				  DWORD dwFlags = OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
				  LPCTSTR lpszFilter = NULL,
				  CWnd* pParentWnd = NULL) : 
		CHexFileDialog("FileImportDlg", help_id, TRUE, NULL, lpszFileName, dwFlags, lpszFilter, "Import", pParentWnd)
	{
	}

	// Overriden members of CFileDialog
	virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

private:
	CButton m_discon;                   // Check box for discontiguous addresses
	CButton m_highlight;                // Check box for highlight of imported records
};

// CExportDialog - derived from CFileDialog to support extra checkbox (export of highlights)
class CExportDialog : public CHexFileDialog
{
public:
	CExportDialog(LPCTSTR lpszFileName = NULL,
				  DWORD help_id = HIDD_FILE_EXPORT_MOTOROLA,
				  DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
				  LPCTSTR lpszFilter = NULL,
				  CHexEditView* pView = NULL,
				  BOOL enable_discon_hl = TRUE) : 
		CHexFileDialog("ExportFileDlg", help_id, FALSE, NULL, lpszFileName, dwFlags, lpszFilter, "Export", pView)
	{
		m_pview = pView;
		m_enable_discon_hl = enable_discon_hl;
	}

	// Overriden members of CFileDialog
	virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

private:
	CHexEditView *m_pview;
	CButton m_discon_hl;                // Only export highlights (as discontinuous records)
	BOOL    m_enable_discon_hl;         // Enable checkbox?
};

// CFileOpenDialog - derived from CFileDialog to support open shared control (see theApp.open_file_shared_)
class CFileOpenDialog : public CHexFileDialog
{
public:
	CFileOpenDialog(LPCTSTR lpszFileName = NULL,
					DWORD dwFlags = OFN_SHOWHELP | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
					LPCTSTR lpszFilter = NULL,
					CWnd* pParentWnd = NULL) : 
		CHexFileDialog("FileOpenDlg", HIDD_FILE_OPEN, TRUE, NULL, lpszFileName, dwFlags, lpszFilter, NULL, pParentWnd)
	{
#ifdef FILE_PREVIEW
		m_wndHook.m_pOwner = this;
#endif
	}

	// Overriden members of CFileDialog
	virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

#ifdef FILE_PREVIEW
	class CHookWnd : public CWnd
	{
	public:
		CFileOpenDialog * m_pOwner;
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	};

	virtual void OnFolderChange();
    CHookWnd m_wndHook;         // Window that contians list control and is notified when the selection chnages
    void UpdatePreview();
    CString m_strPreview;       // File name of selection (current previewed file)
#endif

private:
	CButton m_open_shared;
};


/////////////////////////////////////////////////////////////////////////////
// CMultiplay dialog

class CMultiplay : public CDialog
{
// Construction
public:
	CMultiplay(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMultiplay)
	enum { IDD = IDD_MULTIPLAY };
	CComboBox	name_ctrl_;
	long	plays_;
	//}}AFX_DATA
		CString macro_name_;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiplay)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMultiplay)
	afx_msg void OnPlayOptions();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnSelchangePlayName();
	virtual void OnOK();
	afx_msg void OnHelp();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	void FixControls() ;
};

/////////////////////////////////////////////////////////////////////////////
// CSaveMacro dialog

class CSaveMacro : public CDialog
{
// Construction
public:
	CSaveMacro(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveMacro)
	enum { IDD = IDD_MACROSAVE };
	long	plays_;
	CString	name_;
	CString	comment_;
	int		halt_level_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveMacro)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveMacro)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual void OnOK();
	afx_msg void OnMacroHelp();
	//}}AFX_MSG
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

	CHexEditApp *aa_;                       // Ptr to the app (mainly for macro handling)
};
/////////////////////////////////////////////////////////////////////////////
// CMacroMessage dialog

class CMacroMessage : public CDialog
{
// Construction
public:
	CMacroMessage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMacroMessage)
	enum { IDD = IDD_MACRO_MESSAGE };
	CString	message_;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMacroMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMacroMessage)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// GetInt dialog - used byt getint() function of expressions to ask the user for an integer value
class GetInt : public CDialog
{
	DECLARE_DYNAMIC(GetInt)

public:
	GetInt(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_GETINT };

	CString prompt_;               // Text to display to the user
	long value_;                   // value in/out
	long min_, max_;               // Allowed range

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
};

// GetStr dialog - used by getstring() function of expressions to ask the user for a string
class GetStr : public CDialog
{
	DECLARE_DYNAMIC(GetStr)

public:
	GetStr(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_GETSTR };

	CString prompt_;               // Text to display to the user
	CString value_;                // value in/out

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
};

// GetBool dialog - used byt getbool() function of expressions to ask the user a question
class GetBool : public CDialog
{
	DECLARE_DYNAMIC(GetBool)

public:
	GetBool(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_GETBOOL };

	CString prompt_;               // Text to display to the user
	CString yes_, no_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_)
