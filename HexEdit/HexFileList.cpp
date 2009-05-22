// HexFileList: override of MFC CRecentFileList class
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

#include "stdafx.h"
#include <imagehlp.h>       // For ::MakeSureDirectoryPathExists()

#include "HexEdit.h"
#include "MainFrm.h"
#include "HexFileList.h"
#include "HexEditView.h"

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
    default_data_ = "1|-30000|-30000|-30000|-30000|0|0|0|0||||0|16|4|0|Courier|16|Terminal|18|default";
	SetDefaults();

    ASSERT(AfxGetMainWnd() != NULL);

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

#ifdef _DEBUG  // Force reallocation sooner in debug to catch bad iterator bugs
    name_.reserve(2);
	crc_.reserve(2);
    opened_.reserve(2);
    data_.reserve(2);
#else
    name_.reserve(1024);
	crc_.reserve(1024);
    opened_.reserve(1024);
    data_.reserve(1024);
#endif
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

    SetDV(COLUMNS, theApp.open_rowsize_);
    SetDV(GROUPING, theApp.open_group_by_);
    SetDV(OFFSET, theApp.open_offset_);
    SetDV(VERT_BUFFER_ZONE, theApp.open_vertbuffer_);

    ASSERT(theApp.open_disp_state_ != -1);      // Ensures we have read default values
    SetDV(DISPLAY, theApp.open_disp_state_);
    SetDV(AERIALDISPLAY, 0x000010B1F);          // xxx allow user to set default values later
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

    ASSERT(crc_.size() == name_.size());
    ASSERT(opened_.size() == name_.size());
    ASSERT(data_.size() == name_.size());

    CString saved_data;

    // Find and remove any existing entry(s) for this file
    // Note: we use an index rather than iterator since an iterator is invalid after erase() is called
    for (int ii = name_.size()-1; ii >= 0; ii--)
    {
//        if (_stricmp(name_[ii], szTemp) == 0)
        if (AfxComparePath(name_[ii], szTemp))     // Handle DBCS case-insensitive comparisons etc
        {
			// Remove the existing entry for the file
            name_.erase(name_.begin() + ii);
			crc_.erase(crc_.begin() + ii);
            opened_.erase(opened_.begin() + ii);
            saved_data = data_[ii];                //  save data_ for when file read (below)
            data_.erase(data_.begin() + ii);
        }
    }

    // Add to the end of the vectors (most recent files at end)
	CString ss(szTemp);
    name_.push_back(ss);
	ss.MakeUpper();
	crc_.push_back(crc16(ss, ss.GetLength()));
    opened_.push_back(time(NULL));
    data_.push_back(saved_data);

    ASSERT(crc_.size() == name_.size());
    ASSERT(opened_.size() == name_.size());
    ASSERT(data_.size() == name_.size());

    // Let base class also keep its silly little list
    CRecentFileList::Add(lpszPathName);

    ((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(szTemp);  // Let explorer update (last opened time)
    ASSERT(Check());
}

void CHexFileList::ClearAll()
{
    name_.clear();
	crc_.clear();
    opened_.clear();
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
    ASSERT(crc_.size() == name_.size());
    ASSERT(opened_.size() == name_.size());
    ASSERT(data_.size() == name_.size());

    ((CMainFrame *)AfxGetMainWnd())->UpdateExplorer(name_[nIndex]);  // Let explorer update (last opened time is now gone)

    name_  .erase(name_  .begin() + (name_  .size() - nIndex - 1));
	crc_   .erase(crc_   .begin() + (crc_   .size() - nIndex - 1));
    opened_.erase(opened_.begin() + (opened_.size() - nIndex - 1));
    data_  .erase(data_  .begin() + (data_  .size() - nIndex - 1));

    ASSERT(crc_.size() == name_.size());
    ASSERT(opened_.size() == name_.size());
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
			continue;
		}

        CString ss;

        // Get the file's name
        AfxExtractSubString(ss, strLine, 0, '|');
        if (ss.IsEmpty()) continue;
        name_.push_back(ss);
		ss.MakeUpper();
        crc_.push_back(crc16(ss, ss.GetLength()));

        // Get the last opened date
        AfxExtractSubString(ss, strLine, 1, '|');
        time_t tt = strtol(ss, NULL, 10);
        opened_.push_back(tt);

        // Get the data string
        int curr;                           // Where in the input string the extra data is stored

        ss.Empty();                         // Default data
        if ((curr = strLine.Find('|')) && (curr = strLine.Find('|', curr+1)) != -1)
            ss = strLine.Mid(curr+1);

        data_.push_back(ss);
    }

    ff.Close();

    int max_keep = 20000;

    if (name_.size() > max_keep)
    {
        CString ss;
        ss.Format("Truncated recent file list to most recent %d files", max_keep);
        name_.erase(name_.begin(), name_.begin() + (name_.size() - max_keep));
        crc_.erase(crc_.begin(), crc_.begin() + (crc_.size() - max_keep));
        opened_.erase(opened_.begin(), opened_.begin() + (opened_.size() - max_keep));
        data_.erase(data_.begin(), data_.begin() + (data_.size() - max_keep));
    }

    ASSERT(crc_.size() == name_.size());
    ASSERT(opened_.size() == name_.size());
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
            crc_.push_back(crc16(ss, ss.GetLength()));
            opened_.push_back(time(NULL));
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

    ff.WriteString("; version 3\n");
    for (int ii = 0; ii < name_.size(); ++ii)
    {
        CString ss;
        ss.Format("|%ld|", opened_[ii]);
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

	// xxx this may need improvement
    strName = ::DeviceName(m_arrNames[nIndex]);

    if (strName.IsEmpty())
        return FALSE;

	return TRUE;
}

#include "timer.h"

// GetIndex returns index into list given a file name or -1 if not found.

// Note: We use a CRC of the filename to speed up searching.  (We could
// have done even better by creating an index or something, but that was
// deemed too hard for the benfit - maybe one day.)  Test show that the CRC
// speeds up search about 10-fold, while loading of the file at startup is
// only slowed down about 10% (< 100 msecs for 20000 files on fast machine).
// Also crc16 and crc_32 have almost identical speeds on P4.

int CHexFileList::GetIndex(LPCTSTR lpszPathName) const
{
    ASSERT(lpszPathName != NULL);
    ASSERT(AfxIsValidString(lpszPathName));

	CString ss(lpszPathName);
	ss.MakeUpper();
	unsigned short crc = crc16(ss, ss.GetLength());
    for (int ii = 0; ii < name_.size(); ++ii)
    {
//		if (AfxComparePath(name_[ii], lpszPathName))
		if (crc == crc_[ii] && AfxComparePath(name_[ii], lpszPathName))
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
    ASSERT(index < name_.size());

    CString retval;
    if (index > - 1)
        AfxExtractSubString(retval, data_[index], param, '|');
    if (retval.IsEmpty())
        AfxExtractSubString(retval, default_data_, param, '|');
    return retval;
}
