// HexFileList: override of MFC CRecentFileList class
//
// Copyright (c) 2003-2012 by Andrew W. Phillips.
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

#include "stdafx.h"
#include <sys/stat.h>       // Use stat() for file size by name
#include <imagehlp.h>       // For ::MakeSureDirectoryPathExists()

#include "HexEdit.h"
#include "MainFrm.h"
#include "HexFileList.h"
#include "HexEditView.h"

#include <queue>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

// The following are not in a public header
extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

CHexFileList::CHexFileList(UINT nStart, LPCTSTR lpszSection, int nSize, int nMaxDispLen /*= AFX_ABBREV_FILENAME_LEN*/)
		: CRecentFileList(nStart, lpszSection, _T("File%d"), nSize, nMaxDispLen)
{
	// Set up the default values
	// Note: the default scheme should always be empty in which case a file extension
	// scheme is searched for then a scheme based on current char set is used
	// Also note that some values are set in the default string here just to show the fields
	// and are overwritten with the current default value from the registry below.
	// The entries correspond to the parm_num enum as:
	//               CMD                           SEL     CS DOC  GRP                        FORMAT
	//               | TOP    LEFT   BOTTOM RIGHT  |-| POS  HL| COLS   FONT    HEIGHT         |
	//               | |      |      |      |      | | | MK  DISPLAY OFF       |  OEMFONT  HEIGHT
	default_data_ = "1|-30000|-30000|-30000|-30000|0|0|0|0||||0|16|4|0|Courier|16|Terminal|18|default";
	SetDefaults();

	ASSERT(AfxGetMainWnd() != NULL);

	// Get user's Application Data folder
	if (!::GetDataPath(filename_))
		return;

	filename_ += CString(lpszSection);   // append file name

	// Create HexEdit folder within that
	if (!MakeSureDirectoryPathExists(filename_))
	{
		filename_.Empty();
		return;
	}

	size_t capacity = 256;    // Initial vector capacities
#ifdef _DEBUG
	capacity = 1;     // Force reallocation sooner in debug to catch bad iterator bugs
#else
	// Work out roughly how many recent files we have from the recent file list's file size
	struct _stat stat;
	if (::_stat(filename_, &stat) != -1)
		capacity = stat.st_size / 100;  // Most lines of the text file are longer than 100 chars
#endif
	name_.reserve(capacity);
	hash_.reserve(capacity);
	opened_.reserve(capacity);
	open_count_.reserve(capacity);
	data_.reserve(capacity);

	ver_ = -1;
}

void CHexFileList::SetDefaults()
{
	if (theApp.open_max_)
		SetDV(CMD, int(SW_SHOWMAXIMIZED));
	else
		SetDV(CMD, int(SW_SHOW));

	if (theApp.open_plf_ != NULL)
	{
		SetDV(FONT, theApp.open_plf_->lfFaceName);
		SetDV(HEIGHT, theApp.open_plf_->lfHeight);
	}
	if (theApp.open_oem_plf_ != NULL)
	{
		SetDV(OEMFONT, theApp.open_oem_plf_->lfFaceName);
		SetDV(OEMHEIGHT, theApp.open_oem_plf_->lfHeight);
	}
	if (theApp.open_mb_plf_ != NULL)
	{
		SetDV(MBFONT, theApp.open_mb_plf_->lfFaceName);
		SetDV(MBHEIGHT, theApp.open_mb_plf_->lfHeight);
	}

	SetDV(COLUMNS, theApp.open_rowsize_);
	SetDV(GROUPING, theApp.open_group_by_);
	SetDV(OFFSET, theApp.open_offset_);
	SetDV(VERT_BUFFER_ZONE, theApp.open_vertbuffer_);

	ASSERT(theApp.open_disp_state_ != -1);      // Ensures we have read default values
	SetDV(DISPLAY, theApp.open_disp_state_);

	SetDV(DFFDVIEW, theApp.dffdview_);
	SetDV(AERIALVIEW, theApp.aerialview_);
	SetDV(AERIALDISPLAY, theApp.aerial_disp_state_);
	SetDV(COMPVIEW, theApp.compview_);

	SetDV(SCHEME, theApp.open_scheme_name_);
	SetDV(CODEPAGE, theApp.open_code_page_);
}

#ifdef _DEBUG
// Make sure the first entries in our vector match the filenames in the MFC string array
bool CHexFileList::Check()
{
	for (int ii = 0; ii < m_nSize-1 && int(name_.size())-ii-1 > 0; ++ii)
		if (!AfxComparePath(m_arrNames[ii], name_[name_.size()-ii-1]))
		{
			TRACE2("Recent File List ERROR: <%s> does not match <%s>", m_arrNames[ii], name_[name_.size()-ii-1]);
			return false;
		}
	return true;
}
#endif

void CHexFileList::ChangeSize(int nSize)
{
	// Recreate the base class names array used to update menus
	delete[] m_arrNames;
	m_arrNames = new CString[nSize];

	m_nSize = nSize;

	// Fill in all the entries
	for (int iMRU = 0; iMRU < m_nSize && iMRU < name_.size(); iMRU++)
	{
		m_arrNames[iMRU] = name_[name_.size()-iMRU-1];
	}

	ASSERT(Check());
}

void CHexFileList::Add(LPCTSTR lpszPathName)
{
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	// Get the fully qualified path name
	TCHAR szTemp[_MAX_PATH];
	AfxFullPath(szTemp, lpszPathName);

	ASSERT(hash_.size() == name_.size());
	ASSERT(opened_.size() == name_.size());
	ASSERT(open_count_.size() == name_.size());
	ASSERT(data_.size() == name_.size());

	CString saved_data;

	// Get hash to speed search of the list
	CString ss(szTemp);
	ss.MakeUpper();
	unsigned long hash = str_hash(ss);

	int open_count = 0;

	// Find and remove any existing entry(s) for this file
	// Note: we use an index rather than iterator since an iterator is invalid after erase() is called
	for (int ii = name_.size()-1; ii >= 0; ii--)
	{
		// Using AfxComparePath is more reliable as it handles DBCS case-insensitive comparisons etc
		if (hash == hash_[ii] && AfxComparePath(name_[ii], szTemp))
		{
			// Remove the existing entry for the file
			name_.erase(name_.begin() + ii);
			hash_.erase(hash_.begin() + ii);
			opened_.erase(opened_.begin() + ii);
			open_count = open_count_[ii];          // save open count
			open_count_.erase(open_count_.begin() + ii);
			saved_data = data_[ii];                //  save data_ for when file read (below)
			data_.erase(data_.begin() + ii);

			break;   // A file should only appear in the list once (but we should have a way to remove duplicates?)
		}
	}

	// Add to the end of the vectors (so most recent files are at the end)
	name_.push_back(szTemp);
	hash_.push_back(hash);
	opened_.push_back(time(NULL));
	open_count_.push_back(open_count + 1);  // inc open count (do we need IncOpenCount()?)
	data_.push_back(saved_data);

	ASSERT(hash_.size() == name_.size());
	ASSERT(opened_.size() == name_.size());
	ASSERT(open_count_.size() == name_.size());
	ASSERT(data_.size() == name_.size());

	// Let base class also keep its silly little list
	CRecentFileList::Add(lpszPathName);

	((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(szTemp);  // Let explorer update (last opened time)
	ASSERT(Check());
}

void CHexFileList::ClearAll()
{
	name_.clear();
	hash_.clear();
	opened_.clear();
	open_count_.clear();
	data_.clear();

	// Remove from the base class list starting at the end
	for (int ii = m_nSize; ii > 0; ii--)
		CRecentFileList::Remove(ii-1);

	((CMainFrame *)AfxGetMainWnd())->UpdateExplorer();     // some of the files may have been in current explorer folder
	ASSERT(Check());
}

void CHexFileList::Remove(int nIndex)
{
	ASSERT(nIndex < name_.size());
	ASSERT(hash_.size() == name_.size());
	ASSERT(opened_.size() == name_.size());
	ASSERT(open_count_.size() == name_.size());
	ASSERT(data_.size() == name_.size());

	((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(name_[nIndex]);  // Let explorer update (last opened time is now gone)

	name_  .erase(name_  .begin() + (name_  .size() - nIndex - 1));
	hash_  .erase(hash_  .begin() + (hash_  .size() - nIndex - 1));
	opened_.erase(opened_.begin() + (opened_.size() - nIndex - 1));
	open_count_.erase(open_count_.begin() + (open_count_.size() - nIndex - 1));
	data_  .erase(data_  .begin() + (data_  .size() - nIndex - 1));

	ASSERT(hash_.size() == name_.size());
	ASSERT(opened_.size() == name_.size());
	ASSERT(open_count_.size() == name_.size());
	ASSERT(data_.size() == name_.size());

	// Let base class also update its list
	if (nIndex < m_nSize)
		CRecentFileList::Remove(nIndex);

	// Fill in empty entry created at end of base class list
	for (int iMRU = nIndex; iMRU < m_nSize; iMRU++)
	{
		if (m_arrNames[iMRU].IsEmpty() && iMRU < name_.size())
			m_arrNames[iMRU] = name_[name_.size()-iMRU-1];
	}

	ASSERT(Check());
}

bool CHexFileList::ReadFile()
{
	CStdioFile ff;                      // Text file we are reading from
	CFileException fe;                  // Stores file exception info
	CString strLine;                    // One line read in from the file

	// Open the file
	if (!ff.Open(filename_, CFile::modeRead|CFile::shareDenyWrite|CFile::typeText, &fe))
		return false;

	// Read all the recent file names (and associated data) from the file
	ver_ = -1;
	for (int line_no = 0; ff.ReadString(strLine); ++line_no)
	{
		// Ignore comment lines
		if (strLine[0] == ';')
		{
			// Get file version from 1st comment line
			if (line_no == 0 && _strnicmp(strLine, "; version ", 10) == 0)
				ver_ = atoi((const char *)strLine + 10);
			if (ver_ > 4)
				return false;   // Can't handle new fileformat in this old program
			continue;
		}

		CString ss;

		// Get the file's name
		AfxExtractSubString(ss, strLine, 0, '|');
		if (ss.IsEmpty()) continue;
		name_.push_back(ss);
		ss.MakeUpper();
		hash_.push_back(str_hash(ss));

		// Get the last opened date
		AfxExtractSubString(ss, strLine, 1, '|');
		time_t tt = strtol(ss, NULL, 10);
		opened_.push_back(tt);

		if (ver_ > 3)
		{
			AfxExtractSubString(ss, strLine, 2, '|');
			open_count_.push_back(strtol(ss, NULL, 10));
		}
		else
			open_count_.push_back(1);   // it must have been opened at least once before

		// Get the data string
		int curr;                           // Where in the input string the extra data is stored

		ss.Empty();                         // Default data
		if (ver_ > 3)
		{
			// Skip 1st three vertical bars (|)
			if ((curr = strLine.Find('|')) != -1  && (curr = strLine.Find('|', curr+1)) != -1 && (curr = strLine.Find('|', curr+1)) != -1)
				ss = strLine.Mid(curr+1);
		}
		else
		{
			// Skip 1st two vertical bars (|)
			if ((curr = strLine.Find('|')) != -1  && (curr = strLine.Find('|', curr+1)) != -1)
				ss = strLine.Mid(curr+1);
		}


		data_.push_back(ss);
	}

	ff.Close();

	int max_keep = 20000;

	if (name_.size() > max_keep)
	{
		CString ss;
		ss.Format("Truncated recent file list to most recent %d files", max_keep);
		name_.erase(name_.begin(), name_.begin() + (name_.size() - max_keep));
		hash_.erase(hash_.begin(), hash_.begin() + (hash_.size() - max_keep));
		opened_.erase(opened_.begin(), opened_.begin() + (opened_.size() - max_keep));
		open_count_.erase(open_count_.begin(), open_count_.begin() + (open_count_.size() - max_keep));
		data_.erase(data_.begin(), data_.begin() + (data_.size() - max_keep));
	}

	ASSERT(hash_.size() == name_.size());
	ASSERT(opened_.size() == name_.size());
	ASSERT(open_count_.size() == name_.size());
	ASSERT(data_.size() == name_.size());

	return true;
}

void CHexFileList::ReadList()
{
	ClearAll();

	if (ReadFile())
	{
		// Update m_arrNames to match
		for (int ii = 0; ii < name_.size() && ii < m_nSize; ++ii)
			m_arrNames[ii] = name_[name_.size()-ii-1];
	}
	else
	{
		// Call base class to get file names from the registry
		CRecentFileList::ReadList();

		// Get other parameters using old registry entries (for backward compatibility)
		int ii;
		CString fnum;
		struct
		{
			union
			{
				DWORD disp_state_;
				struct display_bits display_;
			};
		} tt;

		// Read options for each file of the MRU list
		for (ii = m_nSize - 1; ii >= 0; ii--)
		{
			if (m_arrNames[ii].IsEmpty())
				continue;

			CString ss;

			ss = m_arrNames[ii];
			name_.push_back(ss);
			ss.MakeUpper();
			hash_.push_back(str_hash(ss));
			opened_.push_back(time(NULL));
			open_count_.push_back(1);
			data_.push_back("");

			fnum.Format("File%d", ii+1);
			SetData(name_.size()-1, CMD, theApp.GetProfileInt(fnum, "WindowState", SW_SHOWNORMAL));
			SetData(name_.size()-1, TOP, (int)theApp.GetProfileInt(fnum, "WindowTop", -30000));
			SetData(name_.size()-1, LEFT, (int)theApp.GetProfileInt(fnum, "WindowLeft", -30000));
			SetData(name_.size()-1, BOTTOM, (int)theApp.GetProfileInt(fnum, "WindowBottom", -30000));
			SetData(name_.size()-1, RIGHT, (int)theApp.GetProfileInt(fnum, "WindowRight", -30000));

			SetData(name_.size()-1, COLUMNS, __min(CHexEditView::max_buf, __max(4, theApp.GetProfileInt(fnum, "Columns", theApp.open_rowsize_))));
			SetData(name_.size()-1, GROUPING,__max(2, theApp.GetProfileInt(fnum, "Grouping", theApp.open_group_by_)));
			SetData(name_.size()-1, OFFSET, __min(atoi(GetData(name_.size()-1, COLUMNS))-1, theApp.GetProfileInt(fnum, "Offset", theApp.open_offset_)));

			ss = theApp.GetProfileString(fnum, "Scheme");
			ss.Replace("|", "_");                               // A scheme name may no longer contain a vertical bar (|)
			SetData(name_.size()-1, SCHEME, ss);

			ss = theApp.GetProfileString(fnum, "Font");
			CString strTemp;
			AfxExtractSubString(strTemp, ss, 0, ',');
			SetData(name_.size()-1, FONT, strTemp);
			AfxExtractSubString(strTemp, ss, 1, ',');
			SetData(name_.size()-1, HEIGHT, strTemp);

			ss = theApp.GetProfileString(fnum, "OemFont");
			AfxExtractSubString(strTemp, ss, 0, ',');
			SetData(name_.size()-1, OEMFONT, strTemp);
			AfxExtractSubString(strTemp, ss, 1, ',');
			SetData(name_.size()-1, OEMHEIGHT, strTemp);

			// Read the option values, defaulting to the global (theApp.open_*) values
			if ((tt.disp_state_ = (int)theApp.GetProfileInt(fnum, "DisplayState", -1)) == -1)

			if (!tt.display_.hex_area && !tt.display_.char_area)
				tt.display_.hex_area = TRUE;
			if (!tt.display_.hex_area)
				tt.display_.edit_char = TRUE;
			else if (!tt.display_.char_area)
				tt.display_.edit_char = FALSE;
			if (tt.display_.control > 2)
				tt.display_.control = 0;
			SetData(name_.size()-1, DISPLAY, tt.disp_state_);

			SetData(name_.size()-1, DOC_FLAGS, theApp.GetProfileInt(fnum, "KeepTimes", 0));
			SetData(name_.size()-1, FORMAT, theApp.GetProfileString(fnum, "FormatFile"));

			SetData(name_.size()-1, SELSTART, theApp.GetProfileString(fnum, "SelStart64"));
			SetData(name_.size()-1, SELEND, theApp.GetProfileString(fnum, "SelEnd64"));
			SetData(name_.size()-1, POS, theApp.GetProfileString(fnum, "Pos64"));
			SetData(name_.size()-1, MARK, theApp.GetProfileString(fnum, "Mark64"));

			SetData(name_.size()-1, HIGHLIGHTS, theApp.GetProfileString(fnum, "Highlights"));
		}

		// Now delete the recent file list key (and old one) and all files
		theApp.WriteProfileString(_T("RecentFiles"), NULL, NULL);
		theApp.WriteProfileString(_T("Recent File List"), NULL, NULL);
		for (ii = 0; ii < 16; ++ii)                      // There may be up to 16 entries (some old unused ones)
		{
			fnum.Format("File%d", ii+1);

			theApp.WriteProfileString(fnum, NULL, NULL); // Delete this recent file entry
		}
	}

	((CMainFrame *)AfxGetMainWnd())->UpdateExplorer();  // make sure our explorer window shows info we have just loaded
}

bool CHexFileList::WriteFile()
{
	CStdioFile ff;                      // Text file we are reading from
	CFileException fe;                  // Stores file exception info
	CString strLine;                    // One line read in from the file

	// Open the file
	if (!ff.Open(filename_, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, &fe))
		return false;

	ff.WriteString("; version 4\n");
	for (int ii = 0; ii < name_.size(); ++ii)
	{
		CString ss;
		// ss.Format("|%ld|", opened_[ii]);  // Version 3 files used this
		ss.Format("|%ld|%ld|", (long)opened_[ii], (long)open_count_[ii]);  // Version 4 added open count
		ff.WriteString(name_[ii] + ss + data_[ii] + '\n');
	}

	ff.Close();
	return true;
}

void CHexFileList::WriteList()
{
	WriteFile();
}

BOOL CHexFileList::GetDisplayName(CString& strName, int nIndex,
	LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const
{
	ASSERT(m_arrNames != NULL);
	ASSERT(nIndex < m_nSize);
	if (!::IsDevice(m_arrNames[nIndex]))
		return CRecentFileList::GetDisplayName(strName, nIndex, lpszCurDir, nCurDir, bAtLeastName);

	strName = ::DeviceName(m_arrNames[nIndex]);

	if (strName.IsEmpty())
		return FALSE;

	return TRUE;
}

#include "timer.h"

// GetIndex returns index into list given a file name or -1 if not found.

// Note: We use a hash of the filename to speed up searching.  (We could
// have done even better by creating an index or something, but that was deemed
// too hard for the benfit - maybe one day.)  Testing shows this speeds up 
// searches a lot but only slows down file read (at startup) a tiny bit.

int CHexFileList::GetIndex(LPCTSTR lpszPathName) const
{
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	CString ss(lpszPathName);
	ss.MakeUpper();
	unsigned long hash = str_hash(ss);
	for (int ii = 0; ii < name_.size(); ++ii)
	{
		if (hash == hash_[ii] && AfxComparePath(name_[ii], lpszPathName))
			return ii;
	}

	return -1;
}

bool CHexFileList::SetDV(param_num param, __int64 vv)
{
	char buf[32];

	sprintf(buf, "%I64d", vv);
	return SetDV(param, buf);
}

// Set the default value for a parameter
bool CHexFileList::SetDV(param_num param, LPCTSTR value)
{
	ASSERT(strchr(value, '|') == NULL);   // Value string should not contain separator chars

	// First make sure there are enough separator chars that we can use the sub-string
	int count, curr = -1;
	for (count = 0; (curr = default_data_.Find('|', curr+1)) != -1; ++count)
		;

	if (count < param)
		default_data_ += CString('|', param - count);  // Add more param separator chars

	curr = -1;
	for (int ii = 0; ii < param; ++ii)
		curr = default_data_.Find('|', curr+1);

	int next = default_data_.Find('|', curr+1);
	if (next == -1) next = default_data_.GetLength();

	default_data_ = default_data_.Left(curr+1) + value + default_data_.Mid(next);
	return true;
}

// Get the default value for a parameter
CString CHexFileList::GetDV(param_num param) const
{
	CString retval;
	AfxExtractSubString(retval, default_data_, param, '|');
	return retval;
}

bool CHexFileList::SetData(int index, param_num param, __int64 vv)
{
	char buf[32];

	sprintf(buf, "%I64d", vv);
	return SetData(index, param, buf);
}

bool CHexFileList::SetData(int index, param_num param, LPCTSTR vv)
{
	CString value(vv);
	ASSERT(index > -1);

#if 0 // the following is brain-dead since if the default setting is changed then the used value becomes wrong
	// If we are setting to the default value make it blank
	if (GetDV(param) == value)
	{
		CString ss;

		AfxExtractSubString(ss, data_[index], param, '|');
		if (ss.IsEmpty())
			return true;                  // Current is empty so leave it that way to indicate d.v.
		else
			value.Empty();
	}
#endif

	ASSERT(index < name_.size());
	ASSERT(strchr(value, '|') == NULL);   // Value string should not contain separator chars
	value.Replace("|", "!");

	// First make sure there are enough separator chars that we can use the sub-string
	int count, curr = -1;
	for (count = 0; (curr = data_[index].Find('|', curr+1)) != -1; ++count)
		;

	if (count < param)
		data_[index] += CString('|', param - count);  // Add more param separator chars

	curr = -1;
	for (int ii = 0; ii < param; ++ii)
		curr = data_[index].Find('|', curr+1);

	int next = data_[index].Find('|', curr+1);
	if (next == -1) next = data_[index].GetLength();

	data_[index] = data_[index].Left(curr+1) + value + data_[index].Mid(next);
	return true;
}

// Get current setting for a file or the default settings if not yet set
// This now handles an index of -1 to get the default setting
CString CHexFileList::GetData(int index, param_num param) const
{
	ASSERT(index < int(name_.size()));

	CString retval;
	if (index > - 1)
		AfxExtractSubString(retval, data_[index], param, '|');
	if (retval.IsEmpty())
		AfxExtractSubString(retval, default_data_, param, '|');
	return retval;
}

void CHexFileList::IncOpenCount(int index)
{
	ASSERT(index > -1 && index < int(open_count_.size()));
	++open_count_[index];
}

std::vector<int> CHexFileList::Search(LPCTSTR str, bool ignoreCase, bool keywordsOnly)
{
	CString strSearch;
	if (ignoreCase)
		strSearch = CString(str).MakeUpper();
	else
		strSearch = str;

	std::vector<int> retval;
	ASSERT(name_.size() == data_.size());   // sanity check
	for (int ii = 0; ii < name_.size(); ++ii)
	{
		if (keywordsOnly)
		{
			// Get the string containign all keywords
			CString kk;
			if (ignoreCase)
				kk = GetData(ii, KEYWORDS).MakeUpper();
			else
				kk = GetData(ii, KEYWORDS);
			const char *start = kk.GetBuffer();
			size_t len = strSearch.GetLength();

			// For all matches of the search string 
			for (const char * pp = start; ; ++pp)
			{
				if ((pp = strstr(pp, strSearch)) == NULL)
					break;    // strSearch not found

				// If the string was found and it is a whole word ...
				if ((pp == start || !isalnum(*(pp-1))) && !isalnum(*(pp+len)))
				{
					// Save it and stop searching these keywords
					retval.push_back(ii);
					break;
				}
				// else we continue scan from the next byte in the string
			}
		}
		else
		{
			CString nn, dd;
			if (ignoreCase)
			{
				nn = name_[ii].MakeUpper();
				dd = data_[ii].MakeUpper();
			}
			else
			{
				nn = name_[ii];
				dd = data_[ii];
			}

			// Check if the string is found in name or any of the other parameters
			// Note that as dd use | as a separator between params this would be confused by 
			// using | in a search string, which is unlikely but could be protected against

			if (strstr(nn, strSearch) != NULL || strstr(dd, strSearch) != NULL)
				retval.push_back(ii);
		}
	}
	return retval;
}

void CHexFileList::SetupJumpList()
{
#if _MFC_VER >= 0x0A00  // earlier versions of MFC do not support CJumpList
	CJumpList jumpList;

	if (!jumpList.InitializeList()) return;
	int maxSlots = jumpList.GetMaxSlots();

	// We need to get three list, the total size of which adds up to maxSlots (or less):
	// recent files: just the last n files of the recent file list
	// frequent files: the files that have been opened the most (using the open_count_ array)
	//   - we use a priority queue that stores the most frequently opened files
	//   - note that the "highest" priority are the least frequently opened so we can pop them off the top
	// favorite: the last n files that have CATEGORY of favorite or favourite
	//   - we just store these in a vector and stop when we have enough
	// Note that we build the recent and frequent lists ourselves, rather than relying on
	// KDC_RECENT and KDC_FREQUENT so we know what file extensions need to be registered.

	// This class is a functor used to compare elements added to the frequency priority queue
	class freq_compare
	{
	private:
		CHexFileList *pfl;
	public:
		freq_compare(CHexFileList *pfl): pfl(pfl) { }
		bool operator() (const int &lhs, const int &rhs) const
		{
			return pfl->GetOpenedCount(lhs) > pfl->GetOpenedCount(rhs);
		}
	};
	// This stores the first "favourite" files found
	std::vector<int> fav;
	// This stores the files that have been opened the most
	typedef std::priority_queue<int, std::vector<int>, freq_compare> freq_type;
	freq_type freq(freq_compare(this));

	for (int ii = name_.size() - 1; ii >= 0; ii--)
	{
		if (open_count_[ii] <= 1)
		{
		}
		else if (freq.size() < maxSlots/3)  // fill up to 1/3 of total slots available
		{
			freq.push(ii);
		}
		else if (open_count_[ii] > open_count_[freq.top()])
		{
			// First get rid of all the lowest frequency elements
			// Note: This can result in the size of freq dropping unexpectedly (eg: from 10 to 1) if
			//       there are a large number of files with the same open count. This is NOT a bug.
			int lowest = open_count_[freq.top()];
			while (freq.size() > 0 && open_count_[freq.top()] == lowest)
			{
				freq.pop();
			}
			freq.push(ii);
		}

		if (fav.size() < maxSlots/2)      // may fill up to 1/2 of all slots available
		{
			CString ss = GetData(ii, CATEGORY);
			if (ss.CompareNoCase("Favourites") == 0 || ss.CompareNoCase("Favorites") == 0)
				fav.push_back(ii);
		}
	}

	// work out how many of each to use
	int numFreq = freq.size(), numFav = fav.size();
	ASSERT(numFreq <= maxSlots/3);
	if (numFreq + numFav > (2*maxSlots)/3)         // make sure freq + fav is not more than 2/3 of total
		numFav = (2*maxSlots)/3 - numFreq; 
	int numRecent = maxSlots - numFreq - numFav;
	if (numRecent > name_.size())
		numRecent = name_.size();

	ASSERT(numRecent + numFreq + numFav <= maxSlots);

	// Make sure we are associated with the extensions of all the files we are adding to the jump list
	std::set<CString> ext;

	// Get extensions of recent files
	for (int ii = name_.size() - 1; ii >= (int)name_.size() - numRecent; ii--)
	{
		ext.insert(CString(::PathFindExtension(name_[ii])));
	}

	// Get extensions of frequently opened files
	freq_type temp = freq;
	while (temp.size() > 0)
	{
		ext.insert(CString(::PathFindExtension(name_[temp.top()])));
		temp.pop();
	}

	// Get extensions of favourite files
	for (int ii = 0; ii < fav.size(); ++ii)
	{
		ext.insert(CString(::PathFindExtension(name_[fav[ii]])));
	}

	// Check if appid or exe name is wrong in "HKCR\HexEditPro.file"
	bool need_reg = false;
	HKEY hkey;

	// Check that our "file" registry setting is present in the registry and APPID is correct
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
	                 CString(CHexEditApp::ProgID), 
	                 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
	{
		need_reg = true;  // HexEdit Pro file registry setting is not present
	}
	else
	{
		char buf[1024];
		DWORD len = sizeof(buf)-1;
		CString ss = theApp.m_pszAppID;
		if (RegQueryValueEx(hkey, "AppUserModelID", NULL, NULL, (LPBYTE)buf, &len) != ERROR_SUCCESS ||
			ss.CompareNoCase(buf) != 0)
		{
			need_reg = true;   // command line setting is not present or it is using a different .exe
		}
        RegCloseKey(hkey);
	}

	if (!need_reg)
	{
		// Also check that command setting is present and points to our .exe
		if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
						 CString(CHexEditApp::ProgID) + "\\shell\\open\\command", 
						 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		{
			need_reg = true;  // HexEdit Pro file command setting is not present
		}
		else
		{
			char buf[1024];
			DWORD len = sizeof(buf)-1;
			CString ss;
			AfxGetModuleFileName(0, ss);   // new in MFC 10?
			ss += " %1";
			if (RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)buf, &len) != ERROR_SUCCESS ||
				ss.CompareNoCase(buf) != 0)
			{
				need_reg = true;   // command line setting is not present or it is using a different .exe
			}
			RegCloseKey(hkey);
		}
	}

	// Put all extensions (that have yet to be registered) into a string for RegisterExtensions
	CString strExt;
	for (std::set<CString>::const_iterator pext = ext.begin(); pext != ext.end(); ++pext)
	{
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, *pext + "\\OpenWithProgids", 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		{
			strExt += *pext + "|";
			need_reg = true;
		}
		else
		{
			if (RegQueryValueEx(hkey, CHexEditApp::ProgID, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			{
				strExt += *pext + "|";
				need_reg = true;
			}
			RegCloseKey(hkey);
		}
	}

	// Only fire up reghelper if need to register something (need_reg is true)
	// (ie, there are extensions to register OR the appid or exe path is wrong)
	if (need_reg)
	{
		if (CAvoidableDialog::Show(IDS_REG_REQUIRED, 
		                           "In order to use customized jump lists under Windows 7, "
		                           "the file extensions for recently opened, frequently opened "
		                           "and favorites need to be registered for opening with "
		                           "HexEdit Pro.  These are registry settings for all users, "
		                           "which require Administrator privileges to modify.\n\n"
		                           "Do you wish to associate HexEdit Pro with these file extensions?\n\n"
								   + strExt,
		                           "File Registration",
		                           MLCBF_YES_BUTTON | MLCBF_NO_BUTTON) != IDYES ||
			!theApp.RegisterExtensions(strExt))
		{
			// There is no point in doing anything else if we could not register the file extensions since
			// even jump list categories KDC_RECENT and KDC_FREQUENT require the extensions to be registered.
			return;
		}

		// TODO: xxx We need keep track of all extensions we have registered in a reg string
		//       xxx so we can unregister them all if necessary.  (Perhaps this can be done in 
		//       xxx RegHelper.exe, so we know if we added the .ext or just the OpenWithProgids entry)
	}

	// Add the 3 types of files to the task list
	CString strCategory;

	if (theApp.is_us_)
		strCategory = "Favorites";
	else
		strCategory = "Favourites";
	for (int ii = 0; ii < numFav; ++ii)
		jumpList.AddDestination(strCategory, name_[fav[ii]]);

//#define USE_KNOWN_CATEGORIES 1
#ifdef USE_KNOWN_CATEGORIES
	jumpList.AddKnownCategory(KDC_FREQUENT);
	jumpList.AddKnownCategory(KDC_RECENT);
#else
	strCategory = "Frequent Files";
	for (temp = freq; temp.size() > 0; temp.pop())
		jumpList.AddDestination(strCategory, name_[temp.top()]);

	strCategory = "Recent Files";
	for (int ii = name_.size() - 1; ii >= (int)name_.size() - numRecent; ii--)
		jumpList.AddDestination(strCategory, name_[ii]);
#endif

	jumpList.CommitList();
#endif // _MFC_VER >= 0x0A00
}
