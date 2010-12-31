// Bookmark.cpp: implements CBookmarkList class for storing bookmarks
//
// Copyright (c) 2003-2010 by Andrew W. Phillips.
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

// Notes:
// 1. After the bookmarks have been read in their index never changes.
//    - each view then just needs a vector of bookmark indices and the file location
//    - when deleted they are simply marked deleted (but never written back to file)
//    - new bookmarks are added to the end of the vectors
//    - hence bookmarks are stored in order of adding (oldest at top)
// 2. Every bookmark can have an associated data string (not yet used)
// 3. Every bookmark stores the times of modification (created/last changed) and access
// 4. When bytes are added/removed before a bookmark in a file the bookmark is
//    moved accordingly.  This does not change the modification time.

#include "stdafx.h"
#include <algorithm>

#include <imagehlp.h>       // For ::MakeSureDirectoryPathExists()

#include "HexEdit.h"
#include "Mainfrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "Bookmark.h"
#include "BookmarkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

// The following are not in a public header
extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

CBookmarkList::CBookmarkList(LPCTSTR lpszSection)
{
#ifdef _DEBUG  // Force reallocation sooner in debug to catch bad iterator bugs
	name_.reserve(2);
	file_.reserve(2);
	filepos_.reserve(2);
	modified_.reserve(2);
	accessed_.reserve(2);
	data_.reserve(2);
#else
	name_.reserve(512);
	file_.reserve(512);
	filepos_.reserve(512);
	modified_.reserve(512);
	accessed_.reserve(512);
	data_.reserve(512);
#endif
	ver_ = -1;

	// Get user's Application Data folder
	if (!::GetDataPath(filename_))
		return;

	// Create HexEdit folder within that
	filename_ += CString(lpszSection);
	if (!MakeSureDirectoryPathExists(filename_))
	{
		filename_.Empty();
		return;
	}
}

// Get index of a bookmark given its name.  Returns -1 if not found.
int CBookmarkList::GetIndex(LPCTSTR nn) const
{
	std::vector<CString>::const_iterator pn = std::find(name_.begin(), name_.end(), nn);

	if (pn == name_.end())
		return -1;
	else
		return pn - name_.begin();
}

// Get index of a bookmark given its name and file name.  Returns -1 if not found.
int CBookmarkList::GetIndex(LPCTSTR nn, LPCTSTR fn) const
{
	std::vector<CString>::const_iterator pn = std::find(name_.begin(), name_.end(), nn);

	while (pn != name_.end())
	{
		if (GetFileName(pn - name_.begin()).CompareNoCase(fn) == 0)
			return pn - name_.begin();
		++pn;
		pn = std::find(pn, name_.end(), nn);
	}
	return -1;
}

// Get closest bookmark given a filename and address within that file.
// "Closest" currently means the first one with a lesser address.
// If there is no bookmark before the start of file then -1 is returned.
int CBookmarkList::GetIndex(LPCTSTR filename, __int64 pos) const
{
	int diff = INT_MAX;
	std::vector<CString>::const_iterator pn = file_.end();               // xxx this looks wrong to me (never tested/used)
	std::vector<CString>::const_iterator curr = file_.begin();

	while ((curr = std::find(pn, file_.end(), filename)) != file_.end())
	{
		int curr_diff = int(pos - *(filepos_.begin() + (curr - file_.begin())));

		if (curr_diff < diff)
		{
			pn = curr;
			diff = curr_diff;
		}
		++curr;
	}

	if (pn == file_.end())
		return -1;
	else
		return pn - file_.begin();
}

// Searches for all bookmarks of the form <basename><number> and returns the
// value one past the end (ie the number to use to add another bookmark of
// the same form).  For example GetSetLast("unnamed") would return 12 if there were
// bookmarks with the names: unnamed001, unnamed2, unnamed011, unnamed22not_matched
// Zero is returned if there are no matching bookmarks found.
long CBookmarkList::GetSetLast(LPCTSTR basename, int &count)
{
	long retval = 0;
	size_t len = strlen(basename);
	std::vector<CString>::const_iterator pn;

	for (count = 0, pn = name_.begin(); pn != name_.end(); ++pn)
	{
		if (strncmp(basename, *pn, len) == 0)
		{
			char *pend;

			long ii = strtol((const char *)*pn + len, &pend, 10);

			if (*pend == '\0')
			{
				if (ii >= retval)
					retval = ii + 1;
				++count;
			}
		}
	}

	return retval;
}

void CBookmarkList::RemoveBookmark(int index)
{
	Remove(index);

	// Remove from the bookmarks dialog
	((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.RemoveBookmark(index);
}

// Remove a bookmark.  The entry is only marked removed in list since a doc may have stored the index.
void CBookmarkList::Remove(int index)
{
	ASSERT(index > -2);      // -1 may be passed (ignored) but other -ve values are an error
	if (index < 0) return;

	// If the file is open then remove from the local (doc) bookmarks as well
	CDocument *pdoc;
	if (theApp.m_pDocTemplate->MatchDocType(file_[index], pdoc) == 
		CDocTemplate::yesAlreadyOpen)
	{
		((CHexEditDoc *)pdoc)->RemoveBookmark(index);
	}

	// Flag as deleted in the lists
	ASSERT(index >= 0 && index < (int)file_.size());
	name_[index].Empty();
	file_[index].Empty();
	filepos_[index] = -1;
}

// Remove all bookmarks in a set (ie, of the form <basename><number>)
int CBookmarkList::RemoveSet(LPCTSTR basename)
{
	int count = 0;
	size_t len = strlen(basename);
	std::vector<CString>::const_iterator pn;

	for (pn = name_.begin(); pn != name_.end(); ++pn)
	{
		if (strncmp(basename, *pn, len) == 0)
		{
			char *pend;

			long ii = strtol((const char *)*pn + len, &pend, 10);

			if (*pend == '\0')
			{
				RemoveBookmark(pn - name_.begin());
				++count;
			}
		}
	}

	return count;
}

// Remove all bookmarks for a particular file
void CBookmarkList::Clear(LPCTSTR filename)
{
	std::vector<CString>::iterator pf;

	for (pf = file_.begin(); (pf = std::find(pf, file_.end(), CString(filename))) != file_.end(); ++pf)
	{
#if 0
		name_[pf - file_.begin()].Empty();
		pf->Empty();
		filepos_[pf - file_.begin()] = -1;
#else
		RemoveBookmark(pf - file_.begin());
#endif
	}
}

// Remove all bookmarks from the list
void CBookmarkList::ClearAll()
{
#if 0
	CString curr_file;                  // Used to check if consecutive entries are for the same file name

	ASSERT(file_.size() == name_.size());
	ASSERT(filepos_.size() == name_.size());
	for (int ii = 0; ii < name_.size(); ++ii)
	{
		if (name_[ii].IsEmpty()) continue;
		ASSERT(!file_[ii].IsEmpty());

		// Is this a new file name?
		if (file_[ii] != curr_file)
		{
			CDocument *pdoc;            // Pointer to document of current file (if open)
			curr_file = file_[ii];

			// Check for open document and remove from the local (doc) bookmarks as well
			if (theApp.m_pDocTemplate->MatchDocType(file_[ii], pdoc) == 
				CDocTemplate::yesAlreadyOpen)
			{
				ASSERT(pdoc != NULL);
				((CHexEditDoc *)pdoc)->DeleteBookmarkList();
			}
		}

		// Mark entry as empty
		name_[ii].Empty();
		file_[ii].Empty();
		filepos_[ii] = -1;
	}
#else
	for (int ii = 0; ii < (int)name_.size(); ++ii)
		RemoveBookmark(ii);
#endif
}

void CBookmarkList::GetAll(LPCTSTR filename, std::vector<int> &bookmark, std::vector<__int64> &posn)
{
	bookmark.clear();
	posn.clear();

	std::vector<CString>::iterator pf;
	for (pf = file_.begin(); pf != file_.end(); ++pf)
	{
		if (AfxComparePath(filename, *pf))
		{
			bookmark.push_back(pf - file_.begin());
			posn.push_back(filepos_[pf - file_.begin()]);
		}
	}
}

int CBookmarkList::AddBookmark(LPCTSTR nn)
{
	CHexEditView *pview = GetView();
	if (pview == NULL)
	{
		AfxMessageBox("There is no file open to add the bookmark to.");
		theApp.mac_error_ = 2;
		return -1;
	}
	CHexEditDoc *pdoc = pview->GetDocument();
	ASSERT(pdoc != NULL);
	ASSERT(pdoc->pfile1_ != NULL);
	ASSERT(CString(nn).FindOneOf("|") == -1);

	// Check for overwriting an existing bookmark here?

	FILE_ADDRESS pos = pview->GetPos();
	return AddBookmark(nn, pview->GetDocument()->pfile1_->GetFilePath(), pos, NULL, pdoc);
}

// Add a new bookmark or change the location of an existing one.
// Returns the index of the new or replaced bookmark.
int CBookmarkList::AddBookmark(LPCTSTR nn, LPCTSTR filename, __int64 filepos, LPCTSTR data /*= NULL*/, CHexEditDoc *pdoc /*=NULL*/)
{
	int retval = -1;

	std::vector<CString>::const_iterator pn = std::find(name_.begin(), name_.end(), nn);

	// Search all bookmarks for those with the same name
	while (pn != name_.end())
	{
		// If bookmark already exists in the same file remove it
		if (GetFileName(pn - name_.begin()).CompareNoCase(filename) == 0)
			RemoveBookmark(pn - name_.begin());
		++pn;
		pn = std::find(pn, std::vector<CString>::const_iterator(name_.end()), nn);
	}

	// Add the new one at the end of the list
	retval = name_.size();

	name_.push_back(CString(nn));
	file_.push_back(CString(filename));
	filepos_.push_back(filepos);
	modified_.push_back(time(NULL));
	accessed_.push_back(time(NULL));
	data_.push_back(CString(data));

	// Find if there is a doc open for this file
	if (pdoc == NULL)
	{
		CDocument *pdoc2;            // Pointer to document of current file (if open)
		if (theApp.m_pDocTemplate->MatchDocType(file_[retval], pdoc2) == CDocTemplate::yesAlreadyOpen)
			pdoc = (CHexEditDoc *)pdoc2;
	}

	// Add to the doc bookmark list if it is open
	if (pdoc != NULL)
		pdoc->AddBookmark(retval, filepos);

	// Add to the bookmarks dialog
	((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.UpdateBookmark(retval);

	ASSERT(file_.size() == name_.size());
	ASSERT(filepos_.size() == name_.size());
	ASSERT(modified_.size() == name_.size());
	ASSERT(accessed_.size() == name_.size());
	ASSERT(data_.size() == name_.size());
	return retval;
}

// Move a bookmark implicitly (due to file insertion/deletion above it)
void CBookmarkList::Move(int index, int amount)
{
	if (index == -1) return;   // ignore deleted bookmarks

	ASSERT(index >= 0 && index < (int)file_.size());
	filepos_[index] += amount;
	ASSERT(filepos_[index] > -1);
}

BOOL CBookmarkList::GoTo(LPCTSTR name)
{
	int index = GetIndex(name);
	if (index == -1)
	{
		CString ss;
		ss.Format("Bookmark %s was not found", name);
		AfxMessageBox(ss);
		theApp.mac_error_ = 2;
		return FALSE;
	}
	else
		return GoTo(index);
}

// Returns FALSE if the bookmark was removed because the file is gone
BOOL CBookmarkList::GoTo(int index)
{
	ASSERT(index >= 0 && index < (int)file_.size());
	accessed_[index] = time(NULL);

	// Open the file (just activates if already open)
	CHexEditDoc *pdoc;
	ASSERT(theApp.open_current_readonly_ == -1);
	if ((pdoc = (CHexEditDoc*)(theApp.OpenDocumentFile(file_[index]))) != NULL)
	{
		// Find the active view and jump to the bookmark location
		POSITION pos = pdoc->GetFirstViewPosition();
		while (pos != NULL)
		{
			CView *pview = pdoc->GetNextView(pos);
			if (pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
			{
				CString ss;
				ss.Format("Jump to bookmark \"%s\" ", name_[index]);  // space at end means significant nav pt
				((CHexEditView *)pview)->MoveWithDesc(ss, pdoc->GetBookmarkPos(index));
				pview->SetFocus();
				break;
			}
		}

		// Update the row so the new accessed time is displayed
		((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.UpdateBookmark(index);

		theApp.SaveToMacro(km_bookmarks_goto, name_[index]);
		return TRUE;
	}
	else
	{
		theApp.mac_error_ = 2;

		CString ss;
		ss.Format("The bookmark could not be accessed as the file\r"
				  "%s\rcould not be found/opened.\r\r"
				  "Do you want to remove bookmark \"%s\"?",
				  file_[index], name_[index]);
		if (AfxMessageBox(ss, MB_YESNO) == IDYES)
		{
			RemoveBookmark(index);
			return FALSE;
		}
		else
			return TRUE;
	}
}

void CBookmarkList::SetData(int index, LPCTSTR value)
{
	ASSERT(index >= 0 && index < (int)data_.size());

	data_[index] = value;
}

CString CBookmarkList::GetData(int index) const
{
	ASSERT(index >= 0 && index < (int)data_.size());

	return data_[index];
}

bool CBookmarkList::ReadList()
{
	CStdioFile ff;                      // Text file we are reading from
	CFileException fe;                  // Stores file exception info
	CString strLine;                    // One line read in from the file

	// Open the file
	if (!ff.Open(filename_, CFile::modeRead|CFile::shareDenyWrite|CFile::typeText, &fe))
		return false;

	// Read all the recent file names (and associated data) from the file
	ver_ = -1; // default to unknown version
	for (int line_no = 0; ff.ReadString(strLine); ++line_no)
	{
		if (strLine[0] == ';')
		{
			if (line_no == 0 && _strnicmp(strLine, "; version ", 10) == 0)
				ver_ = atoi((const char *)strLine + 10);

			continue;
		}

		CString ss;

		// Get the bookmark's name
		AfxExtractSubString(ss, strLine, 0, '|');
		if (ss.IsEmpty()) continue;
		name_.push_back(ss);

		// Get the file name and location
		AfxExtractSubString(ss, strLine, 1, '|');
		file_.push_back(ss);

		AfxExtractSubString(ss, strLine, 2, '|');
		filepos_.push_back(_atoi64(ss));

		// Get the date last modified
		AfxExtractSubString(ss, strLine, 3, '|');
		modified_.push_back(time_t(strtol(ss, NULL, 10)));

		// Get the date last accessed
		AfxExtractSubString(ss, strLine, 4, '|');
		accessed_.push_back(time_t(strtol(ss, NULL, 10)));

		ss.Empty();                     // Default data
#if 0   // TBD: TODO bookmarks need to store category (+ comments?)
		// We don't store any extra data in bookmarks yet so just save empty string
		// This will also fix up old bookmarks files that had crap at the end of line

		// Get the data string (everything after the fifth '|'
		int curr;                       // Where in the input string the extra data is stored

		if ((curr = strLine.Find('|')) && 
			(curr = strLine.Find('|', curr+1)) != -1 &&
			(curr = strLine.Find('|', curr+1)) != -1 &&
			(curr = strLine.Find('|', curr+1)) != -1 &&
			(curr = strLine.Find('|', curr+1)) != -1)
			ss = strLine.Mid(curr+1);
#endif

		data_.push_back(ss);
	}

	ff.Close();

#if 0
	int max_keep = 20000;
#ifdef _DEBUG
	max_keep = 20;
#endif

	if (name_.size() > max_keep)
	{
		CString ss;
		ss.Format("Truncated bookmark list to most recent %d files", max_keep);
		name_.erase(name_.begin(), name_.begin() + (name_.size() - max_keep));
		file_.erase(file_.begin(), file_.begin() + (file_.size() - max_keep));
		filepos_.erase(filepos_.begin(), filepos_.begin() + (filepos_.size() - max_keep));
		modified_.erase(modified_.begin(), modified_.begin() + (modified_.size() - max_keep));
		accessed_.erase(accessed_.begin(), accessed_.begin() + (accessed_.size() - max_keep));
		data_.erase(data_.begin(), data_.begin() + (data_.size() - max_keep));
	}
#endif

	ASSERT(file_.size() == name_.size());
	ASSERT(filepos_.size() == name_.size());
	ASSERT(modified_.size() == name_.size());
	ASSERT(accessed_.size() == name_.size());
	ASSERT(data_.size() == name_.size());
	return true;
}

bool CBookmarkList::WriteList() const
{
	CStdioFile ff;                      // Text file we are reading from
	CFileException fe;                  // Stores file exception info
	CString strLine;                    // One line read in from the file

	// Open the file
	if (!ff.Open(filename_, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, &fe))
		return false;

	ff.WriteString("; version 3\n");
	for (int ii = 0; ii < (int)name_.size(); ++ii)
	{
		if (!name_[ii].IsEmpty())
		{
			char buf[48];
			sprintf(buf, "|%I64d|%ld|%ld|", __int64(filepos_[ii]), long(modified_[ii]), long(accessed_[ii]));
			ff.WriteString(name_[ii] + '|' + file_[ii] + CString(buf) + data_[ii] + '\n');
		}
	}

	ff.Close();
	return true;
}
