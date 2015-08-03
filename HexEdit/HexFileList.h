// HexFileList.h : header file for CHexFileList overrride of MFC CRecentFileList class
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include <afxadv.h>
#include <vector>
// #include "Partition.h" // no longer used when schemes added

/////////////////////////////////////////////////////////////////////////////
// CHexFileList class
// Overrides the CRecentFileList class so that we can save some options for
// each of the files that appears in the recent file list.

class CHexFileList : public CRecentFileList
{
	friend class CRecentFileDlg;

// Construction
public:
	CHexFileList(UINT nStart, LPCTSTR lpszSection,
				 int nSize, int nMaxDispLen = AFX_ABBREV_FILENAME_LEN);

	// These are the fields stored in the list where each field is separated by |
	// You can add to the end but DO NOT CHANGE THE ORDER of existing fields as
	// this will cause the data in the recent file list to be wrongly read.
	enum param_num { CMD, TOP, LEFT, BOTTOM, RIGHT, 
					 SELSTART, SELEND, POS, MARK, SCHEME, HIGHLIGHTS,
					 DISPLAY, DOC_FLAGS, COLUMNS, GROUPING, OFFSET,
					 FONT, HEIGHT, OEMFONT, OEMHEIGHT, FORMAT,
					 EDIT_TIME, VIEW_TIME, VERT_BUFFER_ZONE,
					 CATEGORY, KEYWORDS, COMMENTS,
					 DFFDVIEW, DFFDWIDTHS,
					 AERIALVIEW, AERIALDISPLAY, AERIALPOS,
					 COMPVIEW, COMPFILENAME,
					 CODEPAGE, MBFONT, MBHEIGHT, 
					 PREVIEWFILENAME,                     // used for preview thumbnails (not to be confused with bitmap file preview)
					 PREVIEWVIEW, PREVIEWZOOM,            // used for preview of a file (no relationship to PREVIEWFILENAME) - currently just bitmap files
					   PREVIEWX, PREVIEWY, PREVIEWBG,     // position (when zooomed in) and background type (for bitmaps with alpha channel)
					 COMPMINMATCH, COMPMAXDIST,           // more compare params used by bg compare - (ie used by CHexEditDoc not CCompareView)
	};

// Attributes
public:
	int GetIndex(LPCTSTR lpszPathName) const;

// Operations
public:
	virtual void ClearAll();
	virtual void Remove(int nIndex);
	virtual void Add(LPCTSTR lpszPathName);
	virtual void Add(LPCTSTR lpszPathName, LPCTSTR lpszAppID) { Add(lpszPathName); }  // Just call old version (ignore Win7 task bar stuff for now)
	virtual void ReadList();    // reads recent file list from user's file (or registry)
	virtual void WriteList();   // writes recent file list to user-specific file
	virtual BOOL GetDisplayName(CString& strName, int nIndex,
		LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual int GetVersion() { return ver_; }

	time_t GetOpened(int nIndex) { ASSERT(nIndex > -1 && nIndex < int(opened_.size())); return opened_[nIndex]; }
	int GetOpenedCount(int nIndex) { ASSERT(nIndex < int(name_.size())); return open_count_[nIndex]; }
	void SetDefaults();
	bool SetDV(param_num param, LPCTSTR value);
	bool SetDV(param_num param, __int64 vv);
	CString GetDV(param_num param) const;
	bool SetData(int index, param_num param, LPCTSTR value);
	bool SetData(int index, param_num param, __int64 vv);
	CString GetData(int index, param_num param) const;
	void IncOpenCount(int index);
	void ChangeSize(int nSize);

	size_t GetCount() { return name_.size(); }
	std::vector<int> Search(LPCTSTR str, bool ignoreCase, bool keywordsOnly);

	void SetupJumpList();           // configure m_jumpList then commit it to Windows 7 TaskBar

// Implementation
public:
	virtual ~CHexFileList() {}

protected:
	bool ReadFile();
	bool WriteFile();
#ifdef _DEBUG
	bool Check();
#endif

	std::vector<CString> name_;         // Name of file
	std::vector<unsigned long> hash_;   // hash of (uppercased) file name for fast searches
	std::vector<time_t> opened_;        // When the file was last opened
	std::vector<int> open_count_;       // How many times the file has been opened
	std::vector<CString> data_;         // Extra data associated with the file

	CString default_data_;              // Default values for data
	CString filename_;                  // Name of file where recent file list is written

	int ver_;
};

/////////////////////////////////////////////////////////////////////////////
