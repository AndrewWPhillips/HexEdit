// Bookmark.h : header file for CBookmarkList class
//

// Copyright (c) 2003 by Andrew W. Phillips.
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

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CBookmarkList class - stores bookmarks

class CBookmarkList
{
	friend class CBookmarkDlg;
    friend class CHexEditDoc;

// Construction
public:
    CBookmarkList(LPCTSTR lpszSection);

    enum param_num { NAME, FILENAME, FILEPOS, DATE, DATA };

// Attributes
public:
    int GetIndex(LPCTSTR name) const;						// Get bookmark idx by name (-1 if not found)
	int GetIndex(LPCTSTR nn, LPCTSTR fn) const;             // Get bookmark idx from name + filename

    int GetIndex(LPCTSTR filename, __int64 filepos) const;  // Find closest bookmark to this location or -1

// Operations
public:
    virtual void Remove(int index);         // Remove a single bookmark
    virtual void RemoveBookmark(int index); // Remove bookmark and update bookmark dlg
    virtual void Clear(LPCTSTR filename);   // Clear all bookmarks in a file
    virtual void ClearAll();
	CString GetFileName(int index) const { return file_[index]; }
    // Get all bookmarks for a file
    virtual void GetAll(LPCTSTR filename, std::vector<int> &bookmark, std::vector<__int64> &posn); 
    virtual  int AddBookmark(LPCTSTR name); // Add a bookmark at the current position of the active file
    virtual  int AddBookmark(LPCTSTR name, LPCTSTR filename, __int64 filepos, LPCTSTR data = NULL, CHexEditDoc *pdoc = NULL);
    virtual bool ReadList();                // Reads bookmark list from user's file (or registry)
    virtual bool WriteList() const;         // Write bookmark to user-specific file
    virtual BOOL GoTo(int index);           // Open file (if nec.), jump to line and set focus to the window
    virtual BOOL GoTo(LPCTSTR name);        // As above except specifies name not index
    virtual void Move(int index, int amount);
	virtual int GetVersion() { return ver_; } // Version from last file read (ReadList)

    // These are special functions for handling "sets" of bookmarks.  The bookmarks of a set
    // all have the same prefix (basename) followed by a number.
    long GetSetLast(LPCTSTR basename, int &count); // Get number of last bookmark of a set (+1)
    int RemoveSet(LPCTSTR basename);         // Remove all bookmarks of a set (returns no removed)

    // Each bookmark can have extra data (a string) associated with it
    void SetData(int index, LPCTSTR value);
    CString GetData(int index) const;

// Implementation
public:
    virtual ~CBookmarkList() {}

protected:
    CString filename_;                  // Name of file where bookmarks are written

public:
    // These vectors store info about the bookmarks
    std::vector<CString> name_;         // Name of bookmark
    std::vector<CString> file_;         // Name of file containing the bookmark
    std::vector<__int64> filepos_;      // Location in the above file of the bookmark
    std::vector<time_t> modified_;      // When the bookmark was created or last modified
    std::vector<time_t> accessed_;      // When the bookmark was last jumped to
    std::vector<CString> data_;         // Extra data associated with the bookmark

private:
	int ver_;
};

/////////////////////////////////////////////////////////////////////////////
