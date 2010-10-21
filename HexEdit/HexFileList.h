// HexFileList.h : header file for CHexFileList overrride of MFC CRecentFileList class
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

	enum param_num { CMD, TOP, LEFT, BOTTOM, RIGHT, 
					 SELSTART, SELEND, POS, MARK, SCHEME, HIGHLIGHTS,
					 DISPLAY, DOC_FLAGS, COLUMNS, GROUPING, OFFSET,
					 FONT, HEIGHT, OEMFONT, OEMHEIGHT, FORMAT,
					 EDIT_TIME, VIEW_TIME, VERT_BUFFER_ZONE,
					 CATEGORY, KEYWORDS, COMMENTS,
					 DFFDVIEW, DFFDWIDTHS,
					 AERIALVIEW, AERIALDISPLAY, AERIALPOS,
					 COMPVIEW, COMPFILENAME,
	};

// Attributes
public:
	int GetIndex(LPCTSTR lpszPathName) const;

// Operations
public:
	virtual void ClearAll();
	virtual void Remove(int nIndex);
	virtual void Add(LPCTSTR lpszPathName);
	virtual void ReadList();    // reads recent file list from user's file (or registry)
	virtual void WriteList();   // writes recent file list to user-specific file
	virtual BOOL GetDisplayName(CString& strName, int nIndex,
		LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual int GetVersion() { return ver_; }

	time_t GetOpened(int nIndex) { ASSERT(nIndex > -1 && nIndex < int(opened_.size())); return opened_[nIndex]; }
	void SetDefaults();
	bool SetDV(param_num param, LPCTSTR value);
	bool SetDV(param_num param, __int64 vv);
	CString GetDV(param_num param) const;
	bool SetData(int index, param_num param, LPCTSTR value);
	bool SetData(int index, param_num param, __int64 vv);
	CString GetData(int index, param_num param) const;
	void ChangeSize(int nSize);

	size_t GetCount() { return name_.size(); }

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
	std::vector<unsigned short> crc_;   // CRC of file name for fast searches
	std::vector<time_t> opened_;        // When the file was last opened
	std::vector<CString> data_;         // Extra data associated with the file
	CString default_data_;              // Default values for data
	CString filename_;                  // Name of file where recent file list is written

	int ver_;
};

/////////////////////////////////////////////////////////////////////////////
