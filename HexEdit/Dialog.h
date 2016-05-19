// Dialog.h - header for misc. dialogs
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#if !defined(AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_)
#define AFX_DIALOG_H__55A0CEE1_3245_11D2_B012_0020AFDC3196__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HexEdit.h"
#include "HelpID.hm"
#include "HexEditView.h"
#include "Preview.h"

class CHexEditApp;

/////////////////////////////////////////////////////////////////////////////
// CHexDialog - base class for dialogs that allows saving and restoring the window
// size and position and display mode. Can be used as a replacement for CDialog.

class CHexDialog : public CDialog
{
	DECLARE_DYNAMIC(CHexDialog)

public:
	explicit CHexDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL) : CDialog(nIDTemplate, pParentWnd) {}
	//CHexDialog();
	//virtual ~CHexDialog();
	void RestorePos();

	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();

protected:
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CHexFileDialog - just adds the facility to CFileDialog for saving and restoring
// the window size, position and display mode.  Can be used as a replacement for
// CFileDialog or as base class for file dlgs that add controls (see below)

class CHexFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CHexFileDialog)

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
		hid_last_file_dialog = help_id;
	}

	// Overriden members of CFileDialog
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();

	afx_msg LRESULT OnPostInit(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

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

// CFileOpenDialog - derived from CFileDialog to support open shared control
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
		m_wndHook.m_pOwner = this;  // Hook window needs access to this class (eg to store name of selected file)
#endif
	}

protected:
	// Overriden members of CFileDialog
	virtual void OnInitDone();
	virtual BOOL OnFileNameOK();

#ifdef FILE_PREVIEW
	virtual void OnFolderChange();  // When the current folder changes we need to unhook and rehook (and clear preview)

private:
	// This is used to "subclass" the window containing the "list" control so we can tell when the current selection changes
	class CHookWnd : public CWnd
	{
	public:
		CFileOpenDialog * m_pOwner;
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	};

    CHookWnd m_wndHook;         // Window that contains list control and is notified when the selection changes
    void UpdatePreview();       // Called when selected file is changed
	CPreview m_preview;         // preview panel in the dialog
	CString m_strPreview;       // name of preview file name
#endif

private:
	CButton m_open_shared;      // Check box that allows files to be opened cooperatively
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
