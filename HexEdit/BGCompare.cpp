// BGCompare.cpp : implements background compare (part of CHexEditDoc)
//
// Copyright (c) 2015 by Andrew W. Phillips.
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"

#include "HexEditDoc.h"
#include "HexEditView.h"
#include "HexFileList.h"
#include "Dialog.h"
#include "NewCompare.h"
#include "Misc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef min  // avoid corruption of std::min/std::max
#undef min
#undef max
#endif

// Start background compare, first killing any existing one
void CHexEditDoc::OnCompNew()
{
	DoCompNew(none);
}

void CHexEditDoc::DoCompNew(view_t view_type)
{
	CHexEditView * pactive = ::GetView();  // current active hex view (should be for this doc)
	std::vector<CHexEditView *> pviews;    // remembers all the hex views on this doc
	CHexEditView * phev = NULL;            // remembers the active hex view (if found)

	// Find all hex views of this doc (so we can later close any associated compare views).
	// Note that we have to find them and store them in a vector so that we can close 
	// them all at once -- if we close a view while we are iterating through them (using
	// GetFirstViewPosition/GetNextView) then this really confuses MFC (even if
	// we just send a hint to the view since UpdateAllViews() uses GetNextView()).
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pv = GetNextView(pos);
		if (pv->IsKindOf(RUNTIME_CLASS(CHexEditView)))
		{
			pviews.push_back((CHexEditView *)pv);
			if (pv == pactive || phev == NULL)
				phev = (CHexEditView *)pv;
		}
	}
	ASSERT(phev != NULL);               // There should be an active view that is a hex view of this doc
	if (phev == NULL)
		return;

	// Default the options to currently used ones in the current hex view
	bool auto_sync = true;   //phev->AutoSyncCompare();
	bool auto_scroll = true; //phev->AutoScrollCompare();
	CString compareFile;

	if ((view_type = GetCompareFile(view_type, auto_sync, auto_scroll, compareFile, true)) == none)
		return;

	// Kill all compare views (which are always associated with a hex view)
	for (std::vector<CHexEditView *>::const_iterator ppv = pviews.begin(); ppv != pviews.end(); ++ppv)
		(*ppv)->OnCompHide();

	ASSERT(cv_count_ == 0 && pthread4_ == NULL);  // we should have closed all compare views and hence killed the compare thread

	// Now open the compare view and start the background compare.
	// (DoCompSplit/DoCompTab create the new view and send WM_INITIALUPDATE
	// thence CCompareView::OninitialUpdate registers itself with the doc using
	// using CHexEditDoc::AddCompView() which starts the background thread.
	if (view_type == 1)
		phev->DoCompSplit(auto_sync, auto_scroll, compareFile);
	else if (view_type == 2)
		phev->DoCompTab(auto_sync, auto_scroll, compareFile);
}

void CHexEditDoc::AddCompView(CHexEditView *pview)
{
	TRACE("+++ Add Comp View %d\n", cv_count_);
	if (++cv_count_ == 1)
	{
		ASSERT(pthread4_ == NULL);

		// Create the background thread and start it scanning
		if (!CreateCompThread())
		{
			cv_count_ = 0;   // indicate that no compare views are open
			return;
		}
		// We don't need to start self-compare until we detect a change (see CheckBGProcessing)
		if (!bCompSelf_)
		{
			TRACE("+++ Pulsing compare event\n");
			start_comp_event_.SetEvent();
		}
	}
	ASSERT(pthread4_ != NULL);
}

void CHexEditDoc::RemoveCompView()
{
	if (--cv_count_ == 0)
	{
		if (pthread4_ != NULL)
			KillCompThread();
	}
	TRACE("+++ Remove Comp View %d\n", cv_count_);
}

// At the start of the compare process we get the name of the file
// to compare against (or whether it's a self-compare) options etc.
// Parameters:
//   view = 0 (don't care), 1 (force split), 2 (force tabbed)
//   auto_sync = sets/gets option to sync election
//   auto_scroll = sets/gets option to sync scrolling
//   compareFile = returns new compare file name or "*" for self compare
//   bForcePrompt = true forces prompting for file name, etc even if 
//                  we already have one from previous compare.
// Returns: 0 on error, 1 for split view, 2 for tabbed view
view_t CHexEditDoc::GetCompareFile(view_t view_type, bool & auto_sync, bool & auto_scroll, CString & compareFile, bool bForcePrompt /*=false*/)
{
	CNewCompare dlg;

	// Get previous compare file name (if any) to be used as the default
	compareFile = GetCompFileName();

	dlg.orig_shared_ = shared_;
	dlg.compare_type_ = shared_ ? 0 : 1;

	if (compareFile.IsEmpty())
	{
		// Looks like there was no previous compare for this file so use last file name for any file
		dlg.file_name_ = 	theApp.GetProfileString("File-Settings", "LastCompareFile", ""); // but get last used file name
	}
	else if (compareFile.Compare("*") == 0)
	{
		// If we don't have to prompt then we can proceed with self-compare
		if (!bForcePrompt)
		{
			return view_type;                 // return asked for view type (or error if asked for type is 0)
		}
		dlg.file_name_ = 	theApp.GetProfileString("File-Settings", "LastCompareFile", ""); // but also get last used file name (disabled)
	}
	else
	{
		// If we don't have to prompt and the current file name exists then we can proceed using that file
		if (!bForcePrompt && _access(compareFile, 0) != -1)
			return view_type;                 // return asked for view type (or error if asked for type is 0)

		dlg.compare_type_ = 1;                // Default to file compare since we have a file name
		dlg.file_name_ = compareFile;
	}
	dlg.compare_display_ = int(view_type) - 1;
	if (compMinMatch_ == 0)
	{
		dlg.insdel_ = 0;
		dlg.minmatch_ = 8;
	}
	else
	{
		dlg.insdel_ = 1;
		dlg.minmatch_ = compMinMatch_;
	}
	dlg.auto_sync_ = auto_sync;
	dlg.auto_scroll_= auto_scroll;

	if (dlg.DoModal() != IDOK)              // === run dialog ===
		return none;

	// Store selected values
	if (!dlg.insdel_)
	{
		compMinMatch_ = 0;
	}
	else
	{
		compMinMatch_ = dlg.minmatch_;
	}
	auto_sync = dlg.auto_sync_ != 0;
	auto_scroll = dlg.auto_scroll_ != 0;

	if (dlg.compare_type_ == 0)
	{
		if (pfile1_ == NULL)
		{
			TaskMessageBox("File has not been saved to disk", "You have not yet saved the current file to disk. "
						   "A self compare detects changes as they are made on disk. \n\n"
						   "Please save the file to disk before performing a \"self compare\".");
			return none;
		}
		compareFile = "*";
	}
	else
	{
		compareFile = dlg.file_name_;
		theApp.WriteProfileString("File-Settings", "LastCompareFile", compareFile);   // last used - may be used for default next time
	}

	return view_t(dlg.compare_display_ + 1);
}

// Check if disk file has changed
bool CHexEditDoc::OrigFileHasChanged()
{
	if (pfile1_ == NULL)
		return false;

	// Get current file time
	CFileStatus stat;
	pfile1_->GetStatus(stat);

	return stat.m_mtime != saved_status_.m_mtime;
}

// Check if file we are comparing against has changed since we last finished comparing
bool CHexEditDoc::CompFileHasChanged()
{
	if (pthread4_ == NULL || pfile1_compare_ == NULL)
		return false;

	// Get current file time
	CFileStatus stat;
	pfile1_compare_->GetStatus(stat);

	CSingleLock sl(&docdata_, TRUE);
	return stat.m_mtime != comp_[0].m_fileTime;
}

// Get the name of the file we are comparing against (or last compared) or
// return "*" is we are doing a self compare
CString CHexEditDoc::GetCompFileName()
{
	if (bCompSelf_)
	{
		compFileName_ = "*";
	}
	else if (compFileName_.IsEmpty())
	{
		CHexFileList *pfl = theApp.GetFileList();
		int idx = -1;                 // recent file list idx of current document

		if (pfl != NULL &&
			pfile1_ != NULL &&
			(idx = pfl->GetIndex(pfile1_->GetFilePath())) > -1)
		{
			compFileName_ = pfl->GetData(idx, CHexFileList::COMPFILENAME);
		}
	}

	return compFileName_;
}

// Get data from the file we are comparing against
size_t CHexEditDoc::GetCompData(unsigned char *buf, size_t len, FILE_ADDRESS loc, bool use_bg /*=false*/)
{
	CFile64 *pf = use_bg ? pfile4_compare_ : pfile1_compare_;  // Select background or foreground file

	if (pf->Seek(loc, CFile::begin) == -1)
		return -1;
	return pf->Read(buf, len);
}

// Returns the number of diffs (in bytes) OR
// -4 = bg compare not on
// -2 = bg compare is still in progress
int CHexEditDoc::CompareDifferences(int rr /*=0*/)
{
	if (pthread4_ == NULL)
	{
		return -4;
	}

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ == SCANNING)
	{
		return -2;
	}

	ASSERT(rr >= 0 && rr < comp_.size());
	if (rr < 0 || rr >= comp_.size())
		return -1;

	ASSERT(comp_[rr].m_replace_A.size() == comp_[rr].m_replace_len.size());
	ASSERT(comp_[rr].m_insert_A.size() == comp_[rr].m_insert_len.size());
	ASSERT(comp_[rr].m_delete_A.size() == comp_[rr].m_delete_len.size());
	return comp_[rr].m_replace_A.size() + comp_[rr].m_insert_A.size() + comp_[rr].m_delete_A.size();
}

// Return how far our background compare has progressed as a percentage (0 to 100).
// Also return the number of differences found so far (in bytes).
int CHexEditDoc::CompareProgress()
{
	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != SCANNING || length_ == 0)
		return 100;

	return 1 + int((comp_progress_ * 99)/length_);  // 1-100 (don't start at zero)
}

// Given an address in compare file get the corresponding address in the original file. If comp2orig
// is true then given an adddress in the compare file get the corresp. addr. in original file.
FILE_ADDRESS CHexEditDoc::GetCompAddress(FILE_ADDRESS addr, bool comp2orig /* = false */)
{
	FILE_ADDRESS thisAddr = 0;          // address of of diff in this file
	FILE_ADDRESS otherAddr = 0;         // address of corresp diff in other file
	FILE_ADDRESS offset = addr;         // offset from otherAddr in other file

	const std::vector<FILE_ADDRESS> * replace_addr;
	const std::vector<FILE_ADDRESS> * replace_other;
	//const std::vector<FILE_ADDRESS> * replace_len;   // not needed

	const std::vector<FILE_ADDRESS> * insert_addr;
	const std::vector<FILE_ADDRESS> * delete_other;
	const std::vector<FILE_ADDRESS> * insert_len;   // not needed

	const std::vector<FILE_ADDRESS> * delete_addr;
	const std::vector<FILE_ADDRESS> * insert_other;
	const std::vector<FILE_ADDRESS> * delete_len;

	if (comp2orig)
	{
		replace_addr = &comp_[0].m_replace_B;
		replace_other= &comp_[0].m_replace_A;

		insert_addr  = &comp_[0].m_insert_B;
		delete_other = &comp_[0].m_delete_A;
		insert_len   = &comp_[0].m_delete_len;

		delete_addr  = &comp_[0].m_delete_B;
		insert_other = &comp_[0].m_insert_A;
		delete_len   = &comp_[0].m_insert_len;   // size of deletion in B == size of insertion in A
	}
	else
	{
		replace_addr = &comp_[0].m_replace_A;
		replace_other= &comp_[0].m_replace_B;

		insert_addr  = &comp_[0].m_insert_A;
		delete_other = &comp_[0].m_delete_B;
		insert_len   = &comp_[0].m_insert_len;

		delete_addr  = &comp_[0].m_delete_A;
		insert_other = &comp_[0].m_insert_B;
		delete_len   = &comp_[0].m_delete_len;
	}

	std::vector<FILE_ADDRESS>::const_iterator pNext = std::lower_bound(replace_addr->begin(), replace_addr->end(), addr);
	if (pNext != replace_addr->begin())
	{
		// Get info from previous entry
		int idx = (pNext - replace_addr->begin()) - 1;
		thisAddr = (*replace_addr)[idx];
		otherAddr = (*replace_other)[idx];
		offset = addr - thisAddr;
	}
	pNext = std::lower_bound(insert_addr->begin(), insert_addr->end(), addr);
	if (pNext != insert_addr->begin() && *(pNext-1) > thisAddr)
	{
		int idx = (pNext - insert_addr->begin()) - 1;
		thisAddr = (*insert_addr)[idx];
		otherAddr = (*delete_other)[idx];
		offset = (addr - thisAddr) - (*insert_len)[idx];
		if (offset < 0)
			offset = 0;
	}
	pNext = std::lower_bound(delete_addr->begin(), delete_addr->end(), addr);
	if (pNext != delete_addr->begin() && *(pNext-1) > thisAddr)
	{
		int idx = (pNext - delete_addr->begin()) - 1;
		thisAddr = (*delete_addr)[idx];
		otherAddr = (*insert_other)[idx];
		offset = (addr - thisAddr) + (*delete_len)[idx];
	}

	return otherAddr + offset;
}

// GetFirstDiff returns the first difference in the original file.
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetFirstDiff(int rr /*=0*/)
{
	return get_first_diff(false, rr);
}

// GetFirstOtherDiff returns the address/length of the 1st difference in the compare file.
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacment, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetFirstOtherDiff(int rr /*=0*/)
{
	return get_first_diff(true, rr);
}

std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::get_first_diff(bool other, int rr)
{
	ASSERT(rr >= 0 && rr < comp_.size());
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = LLONG_MAX;                           // default value indicates "not found"

	if (pthread4_ == NULL) return retval;              // no background compare is happening

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	const std::vector<FILE_ADDRESS> * replace_addr;
	const std::vector<FILE_ADDRESS> * replace_len;
	const std::vector<FILE_ADDRESS> * insert_addr;
	const std::vector<FILE_ADDRESS> * insert_len;
	const std::vector<FILE_ADDRESS> * delete_addr;
	if (other)
	{
		replace_addr = &comp_[rr].m_replace_B;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_B;
		insert_len   = &comp_[rr].m_delete_len;   // size of deletion in A == insertion in B
		delete_addr  = &comp_[rr].m_delete_B;
	}
	else
	{
		replace_addr = &comp_[rr].m_replace_A;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_A;
		insert_len   = &comp_[rr].m_insert_len;
		delete_addr  = &comp_[rr].m_delete_A;
	}

	if (!replace_addr->empty())
	{
		retval.first  = (*replace_addr)[0];
		retval.second = (*replace_len)[0];
	}

	if (!insert_addr->empty())
	{
		if ((*insert_addr)[0] < retval.first)
		{
			retval.first = (*insert_addr)[0];
			retval.second = - (*insert_len)[0];        // use -ve length to indicate insertion
		}
	}

	if (!delete_addr->empty())
	{
		if ((*delete_addr)[0] < retval.first)
		{
			retval.first = (*delete_addr)[0];
			retval.second = 0;                        // use zero length to indicate a deletion
		}
	}

	if (retval.first == LLONG_MAX)
		retval.first = -1;           // signal not found
	return retval;
}

// GetFirstDiffAll returns the first difference of all diffs in self-compare
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetFirstDiffAll()
{
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = LLONG_MAX;                          // default value indicates "not found"

	if (pthread4_ == NULL) return retval;              // no background compare is happening

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	for (int rr = 0; rr < comp_.size(); ++rr)
	{
		if (!comp_[rr].m_replace_A.empty())
		{
			retval.first  = comp_[rr].m_replace_A[0];
			retval.second = comp_[rr].m_replace_len[0];
		}

		if (!comp_[rr].m_insert_A.empty())
		{
			if (comp_[rr].m_insert_A[0] < retval.first)
			{
				retval.first = comp_[rr].m_insert_A[0];
				retval.second = - comp_[rr].m_insert_len[0];        // use -ve length to indicate insertion
			}
		}

		if (!comp_[rr].m_delete_A.empty())
		{
			if (comp_[rr].m_delete_A[0] < retval.first)
			{
				retval.first = comp_[rr].m_delete_A[0];
				retval.second = 0;                        // use zero length to indicate a deletion
			}
		}
	}

	if (retval.first == LLONG_MAX)
		retval.first = -1;           // signal not found
	return retval;
}

// GetPrevDiff returns the first difference before a specified address in the original file.
//   from = the address to start looking backwards from
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetPrevDiff(FILE_ADDRESS from, int rr /*=0*/)
{
	return get_prev_diff(false, from, rr);
}

// GetPrevDiff returns the first difference before a specified address in the original file.
//   from = the address to start looking backwards from
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetPrevOtherDiff(FILE_ADDRESS from, int rr /*=0*/)
{
	return get_prev_diff(true, from, rr);
}

std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::get_prev_diff(bool other, FILE_ADDRESS from, int rr)
{
	ASSERT(rr >= 0 && rr < comp_.size());
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = -1;                                 // default to "not found"

	if (pthread4_ == NULL) return retval;              // no background compare is happening

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	const std::vector<FILE_ADDRESS> * replace_addr;
	const std::vector<FILE_ADDRESS> * replace_len;
	const std::vector<FILE_ADDRESS> * insert_addr;
	const std::vector<FILE_ADDRESS> * insert_len;
	const std::vector<FILE_ADDRESS> * delete_addr;
	if (other)
	{
		replace_addr = &comp_[rr].m_replace_B;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_B;
		insert_len   = &comp_[rr].m_delete_len;
		delete_addr  = &comp_[rr].m_delete_B;
	}
	else
	{
		replace_addr = &comp_[rr].m_replace_A;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_A;
		insert_len   = &comp_[rr].m_insert_len;
		delete_addr  = &comp_[rr].m_delete_A;
	}

	int idx;

	// First check for a replacement that is before the address
	if ((idx = AddressAt(*replace_addr, from) - 1) >= 0)
	{
		ASSERT(idx < replace_addr->size());
		retval.first  = (*replace_addr)[idx];
		retval.second = (*replace_len)[idx];
	}

	// Check for an insertion that is before the address
	if ((idx = AddressAt(*insert_addr, from) - 1) >= 0)
	{
		// Check if it is closer to the address than what we have (for replacement)
		if ((*insert_addr)[idx] > retval.first)
		{
			retval.first = (*insert_addr)[idx];
			retval.second = - (*insert_len)[idx];      // use -ve length to indicate insertion
		}
	}

	// Check if there is a deletion before the address
	if ((idx = AddressAt(*delete_addr, from) - 1) >= 0)
	{
		// Check if it is closer to the address than what we have
		if ((*delete_addr)[idx] > retval.first)
		{
			retval.first = (*delete_addr)[idx];
			retval.second = 0;                                  // use zero length to indicate a deletion
		}
	}

	return retval;
}

// GetPrevDiffAll returns the first difference before a specified address of all diffs in self-compare
//   from = the address to start looking backwards from
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetPrevDiffAll(FILE_ADDRESS from)
{
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = -1;

	if (pthread4_ == NULL) return retval;

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	int idx;

	for (int rr = 0; rr < comp_.size(); ++rr)
	{
		// First check for a replacement that is before the address
		if ((idx = AddressAt(comp_[rr].m_replace_A, from) - 1) >= 0)
		{
			ASSERT(idx < comp_[rr].m_replace_A.size());
			retval.first  = comp_[rr].m_replace_A[idx];
			retval.second = comp_[rr].m_replace_len[idx];
		}

		// Check for an insertion that is before the address
		if ((idx = AddressAt(comp_[rr].m_insert_A, from) - 1) >= 0)
		{
			// Check if it is closer to the address than what we have (for replacement)
			if (comp_[rr].m_insert_A[idx] > retval.first)
			{
				retval.first = comp_[rr].m_insert_A[idx];
				retval.second = - comp_[rr].m_insert_len[idx];      // use -ve length to indicate insertion
			}
		}

		// Check if there is a deletion before the address
		if ((idx = AddressAt(comp_[rr].m_delete_A, from) - 1) >= 0)
		{
			// Check if it is closer to the address than what we have
			if (comp_[rr].m_delete_A[idx] > retval.first)
			{
				retval.first = comp_[rr].m_delete_A[idx];
				retval.second = 0;                                  // use zero length to indicate a deletion
			}
		}
	}

	return retval;
}

// GetNextDiff returns the first difference after a specified address in the original file.
//   from = the address to start looking (forward) from
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetNextDiff(FILE_ADDRESS from, int rr /*=0*/)
{
	return get_next_diff(false, from, rr);
}

// GetNextOtherDiff returns the first difference after a specified address in the compare file.
//   from = the address to start looking (forward) from
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetNextOtherDiff(FILE_ADDRESS from, int rr /*=0*/)
{
	return get_next_diff(true, from, rr);
}

std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::get_next_diff(bool other, FILE_ADDRESS from, int rr)
{
	ASSERT(rr >= 0 && rr < comp_.size());
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = LLONG_MAX;                          // default value indicates "not found"

	if (pthread4_ == NULL) return retval;              // no background compare is happening

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	const std::vector<FILE_ADDRESS> * replace_addr;
	const std::vector<FILE_ADDRESS> * replace_len;
	const std::vector<FILE_ADDRESS> * insert_addr;
	const std::vector<FILE_ADDRESS> * insert_len;
	const std::vector<FILE_ADDRESS> * delete_addr;
	if (other)
	{
		replace_addr = &comp_[rr].m_replace_B;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_B;
		insert_len   = &comp_[rr].m_delete_len;
		delete_addr  = &comp_[rr].m_delete_B;
	}
	else
	{
		replace_addr = &comp_[rr].m_replace_A;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_A;
		insert_len   = &comp_[rr].m_insert_len;
		delete_addr  = &comp_[rr].m_delete_A;
	}

	int idx;

	if ((idx = AddressAt(*replace_addr, from)) < replace_addr->size())
	{
		retval.first  = (*replace_addr)[idx];
		retval.second = (*replace_len)[idx];
	}

	if ((idx = AddressAt(*insert_addr, from)) < insert_addr->size())
	{
		if ((*insert_addr)[idx] < retval.first)
		{
			retval.first  = (*insert_addr)[idx];
			retval.second = - (*insert_len)[idx];      // use -ve length to indicate insertion
		}
	}

	if ((idx = AddressAt(*delete_addr, from)) < delete_addr->size())
	{
		if ((*delete_addr)[idx] < retval.first)
		{
			retval.first = (*delete_addr)[idx];
			retval.second = 0;                                  // use zero length to indicate a deletion
		}
	}

	if (retval.first == LLONG_MAX)
		retval.first = -1;           // signal not found
	return retval;
}

// GetNextDiffAll returns the first difference before a specified address of all diffs in self-compare
//   from = the address to start looking backwards from
// returns a pair of numbers representing the type of difference and where it occurs in the original file
//   first = address of the difference in the original file (or -1 if not found)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetNextDiffAll(FILE_ADDRESS from)
{
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = LLONG_MAX;                          // default value indicates "not found"

	if (pthread4_ == NULL) return retval;

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	int idx;

	for (int rr = 0; rr < comp_.size(); ++rr)
	{
		// First check for a replacement that is before the address
		if ((idx = AddressAt(comp_[rr].m_replace_A, from) - 1) < comp_[rr].m_replace_A.size())
		{
			retval.first  = comp_[rr].m_replace_A[idx];
			retval.second = comp_[rr].m_replace_len[idx];
		}

		// Check for an insertion that is before the address
		if ((idx = AddressAt(comp_[rr].m_insert_A, from) - 1) < comp_[rr].m_insert_A.size())
		{
			// Check if it is closer to the address than what we have (for replacement)
			if (comp_[rr].m_insert_A[idx] < retval.first)
			{
				retval.first = comp_[rr].m_insert_A[idx];
				retval.second = - comp_[rr].m_insert_len[idx];      // use -ve length to indicate insertion
			}
		}

		// Check if there is a deletion before the address
		if ((idx = AddressAt(comp_[rr].m_delete_A, from) - 1) < comp_[rr].m_delete_A.size())
		{
			// Check if it is closer to the address than what we have
			if (comp_[rr].m_delete_A[idx] < retval.first)
			{
				retval.first = comp_[rr].m_delete_A[idx];
				retval.second = 0;                                  // use zero length to indicate a deletion
			}
		}
	}

	if (retval.first == LLONG_MAX)
		retval.first = -1;           // signal not found
	return retval;
}

// GetLastDiff returns the last difference in the original file.
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacment, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetLastDiff(int rr /*=0*/)
{
	return get_last_diff(false, rr);
}

// GetLastOtherDiff returns the last difference in the compare file.
//   rr = revision to look at (must be zero unless doing a self-compare)
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacment, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetLastOtherDiff(int rr /*=0*/)
{
	return get_last_diff(true, rr);
}

std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::get_last_diff(bool other, int rr)
{
	ASSERT(rr >= 0 && rr < comp_.size());
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = -1;                                 // default to "not found"

	if (pthread4_ == NULL) return retval;              // no background compare is happening

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	const std::vector<FILE_ADDRESS> * replace_addr;
	const std::vector<FILE_ADDRESS> * replace_len;
	const std::vector<FILE_ADDRESS> * insert_addr;
	const std::vector<FILE_ADDRESS> * insert_len;
	const std::vector<FILE_ADDRESS> * delete_addr;
	if (other)
	{
		replace_addr = &comp_[rr].m_replace_B;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_B;
		insert_len   = &comp_[rr].m_delete_len;
		delete_addr  = &comp_[rr].m_delete_B;
	}
	else
	{
		replace_addr = &comp_[rr].m_replace_A;
		replace_len  = &comp_[rr].m_replace_len;
		insert_addr  = &comp_[rr].m_insert_A;
		insert_len   = &comp_[rr].m_insert_len;
		delete_addr  = &comp_[rr].m_delete_A;
	}

	if (!replace_addr->empty())
	{
		retval.first  = (*replace_addr).back();
		retval.second = (*replace_len).back();
	}

	if (!insert_addr->empty())
	{
		if ((*insert_addr).back() > retval.first)
		{
			retval.first = (*insert_addr).back();
			retval.second = - (*insert_len).back();        // use -ve length to indicate insertion not replacement
		}
	}

	if (!delete_addr->empty())
	{
		if ((*delete_addr).back() > retval.first)
		{
			retval.first = (*delete_addr).back();
			retval.second = 0;                                  // use zero length to indicate a deletion
		}
	}

	return retval;
}

// GetLastDiffAll returns the first difference in the original file of all diffs (in self compare)
// returns a pair of numbers representing the type of difference and location in the original file
//   first = address of the difference in the original file (or -1 if there are no diffs)
//   second = length of the difference (+ve for replacement, -ve for insertion, zero for deletion)
std::pair<FILE_ADDRESS, FILE_ADDRESS> CHexEditDoc::GetLastDiffAll()
{
	std::pair<FILE_ADDRESS, FILE_ADDRESS> retval;
	retval.first = -1;

	if (pthread4_ == NULL) return retval;

	CSingleLock sl(&docdata_, TRUE);
	if (comp_state_ != WAITING) return retval;         // not finished

	for (int rr = 0; rr < comp_.size(); ++rr)
	{
		if (!comp_[rr].m_replace_A.empty())
		{
			retval.first  = comp_[rr].m_replace_A.back();
			retval.second = comp_[rr].m_replace_len.back();
		}

		if (!comp_[rr].m_insert_A.empty())
		{
			if (comp_[rr].m_insert_A.back() > retval.first)
			{
				retval.first = comp_[rr].m_insert_A.back();
				retval.second = - comp_[rr].m_insert_len.back();        // use -ve length to indicate insertion
			}
		}

		if (!comp_[rr].m_delete_A.empty())
		{
			if (comp_[rr].m_delete_A.back() > retval.first)
			{
				retval.first = comp_[rr].m_delete_A.back();
				retval.second = 0;                        // use zero length to indicate a deletion
			}
		}
	}

	return retval;
}

// Open CompFile just opens the files for comparing.
// Note when comparing that there are 4 files involved:
//   pfile1_         = Original file opened for general use in GUI thread
//   pfile1_compare_ = Compare file opened for use in the main (GUI) thread (used by CCompareView)
//   pfile4_         = Original file opened again to use for comparing in the background thread (see below)
//   pfile4_compare_ = Compare file opened again for use in the background thread
// When comparing different files:
//   pfile1_, pfile4_ is original file
//   pfile1_compare_, pfile4_compare_ is file to compare with
// When doing self-compare:
//   pfile1_ is original file
//   pfile4_ is newer temp file (file name = tempFileA_)
//   pfile1_compare_, pfile4_compare_ is older temp file (file name = tempFileB_)
bool CHexEditDoc::OpenCompFile()
{
	// Open file to be used by background thread
	if (pfile1_ != NULL)
	{
		CString fileName;
		if (bCompSelf_)
			fileName = tempFileA_;
		else
			fileName = pfile1_->GetFilePath();
		ASSERT(!fileName.IsEmpty());

		if (IsDevice() && !bCompSelf_)
			pfile4_ = new CFileNC();
		else
			pfile4_ = new CFile64();
		if (!pfile4_->Open(fileName, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("+++ Compare (file4) open failed for %p\n", this);
			return false;
		}
	}

	// Open any data files in use too, since we need to compare against the in-memory file
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file4_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file4_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

	// Open file to compare against
	ASSERT(pfile1_compare_ == NULL && pfile4_compare_ == NULL);

	CString fileName;
	if (!bCompSelf_)
		fileName = compFileName_;
	else if (pfile1_ != NULL)
		fileName = tempFileB_.IsEmpty() ? pfile1_->GetFilePath() : tempFileB_;
	else
		return false;

	pfile1_compare_ = new CFile64();
	pfile4_compare_ = new CFile64();
	if (!pfile1_compare_->Open(fileName, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) ||
		!pfile4_compare_->Open(fileName, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary))
	{
		TRACE1("+++ Compare file open failed for %p\n", this);
		pfile1_compare_ = pfile4_compare_ = NULL;
		return false;
	}
	return true;
}

void CHexEditDoc::CloseCompFile()
{
	// Close the file instance used in background thread
	if (pfile4_ != NULL)
	{
		pfile4_->Close();
		delete pfile4_;
		pfile4_ = NULL;
	}
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file4_[ii] != NULL)
		{
			data_file4_[ii]->Close();
			delete data_file4_[ii];
			data_file4_[ii] = NULL;
		}
	}

	// Close the compare file (both open instances)
	ASSERT(pfile1_compare_ != NULL && pfile4_compare_ != NULL);
	if (pfile1_compare_ != NULL)
	{
		pfile1_compare_->Close();
		delete pfile1_compare_;
		pfile1_compare_ = NULL;
	}
	if (pfile4_compare_ != NULL)
	{
		pfile4_compare_->Close();
		delete pfile4_compare_;
		pfile4_compare_ = NULL;
	}
}

// Makes a new temp file name and copies the current file to it
bool CHexEditDoc::MakeTempFile()
{
	if (!tempFileA_.IsEmpty() && _access(tempFileA_, 0) != -1)
		return true;    // temp file already present when swapping between tab/split views

	char temp_dir[_MAX_PATH];
	char temp_file[_MAX_PATH];
	CWaitCursor wc;     // displays Hour glass until destroyed on function exit
	if (pfile1_ == NULL ||
		!::GetTempPath(sizeof(temp_dir), temp_dir) ||
		!::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file) ||
		!::CopyFile(pfile1_->GetFilePath(), temp_file, FALSE))
	{
		return false;
	}
	tempFileA_ = temp_file;
	return true;
}

bool CHexEditDoc::IsCompWaiting()
{
	CSingleLock sl(&docdata_, TRUE);
	return comp_state_ == WAITING;
}

// Stop any running comparison
void CHexEditDoc::StopComp()
{
	if (cv_count_ == 0) return;
	ASSERT(pthread4_ != NULL);
	if (pthread4_ == NULL) return;

	// stop background compare (if any)
	bool waiting;
	docdata_.Lock();
	comp_command_ = STOP;
	docdata_.Unlock();
	SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = comp_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ Stop Compare - thread not waiting (yet)\n");
		Sleep(1);
	}
	SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_LOWEST);
	ASSERT(waiting);
}

// Start a new comparison.
void CHexEditDoc::StartComp()
{
	// Make sure it is stopped
	StopComp();

	// Restart the compare
	docdata_.Lock();
	comp_progress_ = 0;

	// Save current file modification time so we don't keep restarting the compare
	CFileStatus stat;
	pfile4_compare_->GetStatus(stat);
	comp_[0].Reset(stat.m_mtime);

	comp_command_ = NONE;
	comp_fin_ = false;
	comp_clock_ = clock();
	docdata_.Unlock();

	// Now that the compare is stopped we need to update all views to remove existing compare display
	CCompHint comph;
	UpdateAllViews(NULL, 0, &comph);

	TRACE("+++ Pulsing compare event (restart)\r\n");
	start_comp_event_.SetEvent();
}

// Sends a message for the thread to kill itself then tidies up shared members. 
void CHexEditDoc::KillCompThread()
{
	ASSERT(pthread4_ != NULL);
	if (pthread4_ == NULL) return;

	HANDLE hh = pthread4_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
	TRACE1("+++ Killing compare thread for %p\n", this);

	// Signal thread to kill itself
	docdata_.Lock();
	comp_command_ = DIE;
	docdata_.Unlock();

	SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death
	bool waiting, dying;
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = comp_state_ == WAITING;
		dying   = comp_state_ == DYING;
		docdata_.Unlock();
		if (waiting || dying)
			break;
		Sleep(1);
	}
	ASSERT(waiting || dying);

	// Send start message in case it is on hold
	if (waiting)
		start_comp_event_.SetEvent();

	TRACE1("+++ Setting compare thread to NULL for %p\n", this);
	pthread4_ = NULL;
	DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
	ASSERT(wait_status == WAIT_OBJECT_0 || wait_status == WAIT_FAILED);

	// Free resources that are only needed during bg compares
	CloseCompFile();
	if (bCompSelf_)
	{
		if (!tempFileA_.IsEmpty())
		{
			VERIFY(::remove(tempFileA_) == 0);
			tempFileA_.Empty();
		}
		if (!tempFileB_.IsEmpty())
		{
			VERIFY(::remove(tempFileB_) == 0);
			tempFileB_.Empty();
		}
	}
	ASSERT(tempFileA_.IsEmpty() && tempFileB_.IsEmpty());
}

static UINT bg_func(LPVOID pParam)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

	TRACE1("+++ Compare thread started for doc %p\n", pDoc);

	return pDoc->RunCompThread();
}

bool CHexEditDoc::CreateCompThread()
{
	ASSERT(pthread4_ == NULL);
	ASSERT(pfile4_ == NULL);

	if (!OpenCompFile())
		return false;

	comp_progress_ = 0;
	comp_.clear();
	comp_.push_front(CompResult());  // always has at least one elt = current/last compare results

	// Save current file modification time so we don't keep restarting the compare
	CFileStatus stat;
	pfile4_compare_->GetStatus(stat);
	comp_[0].Reset(stat.m_mtime);

	// Create new thread
	comp_command_ = NONE;
	comp_state_ = STARTING;
	comp_fin_ = false;
	comp_clock_ = 0;
	TRACE1("+++ Creating compare thread for %p\n", this);
	pthread4_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
	ASSERT(pthread4_ != NULL);
	return true;
}

// This is what does the work in the background thread
UINT CHexEditDoc::RunCompThread()
{
	// Keep looping until we are told to die
	for (;;)
	{
		// Signal that we are waiting then wait for start_comp_event_ to be pulsed
		{
			CSingleLock sl(&docdata_, TRUE);
			comp_state_ = WAITING;
		}
		TRACE1("+++ BGCompare: waiting for %p\n", this);
		DWORD wait_status = ::WaitForSingleObject(HANDLE(start_comp_event_), INFINITE);
		docdata_.Lock();
		comp_state_ = SCANNING;
		comp_fin_ = false;
		docdata_.Unlock();
		start_comp_event_.ResetEvent();
		ASSERT(wait_status == WAIT_OBJECT_0);
		TRACE1("+++ BGCompare: got event for %p\n", this);

		if (CompProcessStop())
			continue;

		docdata_.Lock();
		comp_progress_ = 0;
		CompResult result;
		int min_match = compMinMatch_;
		result = comp_[0];
		docdata_.Unlock();

		// Get buffers for each source
		const size_t buf_size = 8192;   // xxx may need to be dynamic later (based on sync length)
		ASSERT(comp_bufa_ == NULL && comp_bufb_ == NULL);
		ASSERT(min_match == 0 || min_match >= 3+4 && min_match < 64+4);

		// We need buffers aligned on 16-byte boundaries for SSE2 instructions
		comp_bufa_ = (unsigned char *)_aligned_malloc(buf_size + (min_match - 4), 16);
		comp_bufb_ = (unsigned char *)_aligned_malloc(buf_size + (min_match - 4), 16);
		if (comp_bufa_ == NULL || comp_bufb_ == NULL)
		{
			CSingleLock sl(&docdata_, TRUE); // Protect shared data access
			comp_fin_ = true;
			TRACE("+++ BGCompare: _aligned_malloc error in %p\n", this);
			continue;
		}
		size_t gota = 0, gotb = 0;              // Current amount of data obtained from each file (at addra, addrb)
		FILE_ADDRESS addra = 0, addrb = 0;      // Address of byte at start of buffers (comp_bufa_, comp_bufb_)
		FILE_ADDRESS cumulative_replace = 0;    // Keeps track of a long differrence - treated as a replacement

		// Keep looping until we are finished processing blocks or we receive a command to stop etc
		for (;;)
		{
			// First check if we need to stop
			if (CompProcessStop())
				break;   // stop processing and go back to WAITING state

			// Update progress based on how far we are through the original file
			{
				CSingleLock sl(&docdata_, TRUE); // Protect shared data access
				comp_progress_ = addra;
			}

			// Get the next chunks
			if (gota >= buf_size)
				gota = buf_size;
			else
				gota += GetData(comp_bufa_ + gota, buf_size - gota, addra + gota, 4);
			if (gotb >= buf_size)
				gotb = buf_size;
			else
				gotb += GetCompData(comp_bufb_ + gotb, buf_size - gotb, addrb + gotb, true);

			size_t to_check = std::min(gota, gotb);   // The bytes of comp_bufa_/comp_bufb_ to compare
			size_t diff = ::FindFirstDiff(comp_bufa_, comp_bufb_, to_check);

			// CompMinMatch_ of zero means we are not allowing insertions/deletions
			if (min_match == 0)
			{
				if (diff > 0 && cumulative_replace > 0)
				{
					// A difference just happened to finish exactly at end of last read block
					result.m_replace_A.push_back(addra - cumulative_replace);
					result.m_replace_B.push_back(addrb - cumulative_replace);
					result.m_replace_len.push_back(cumulative_replace);
					cumulative_replace = 0;
				}

				if (diff < to_check)
				{
					// Diff. found so scan for next same bit then subsequent diff/same blocks
					while (diff < to_check)
					{
						// No same byte found so this is the start of a diff block
						size_t same = diff + ::FindFirstSame(comp_bufa_ + diff, comp_bufb_ + diff, to_check - diff);
						if (same >= to_check)
						{
							assert(same == to_check);
							cumulative_replace += same - diff;
							break;
						}

						// Add this replacement block
						assert(diff == 0 || cumulative_replace == 0);
						result.m_replace_A.push_back(addra + diff - cumulative_replace);
						result.m_replace_B.push_back(addrb + diff - cumulative_replace);
						result.m_replace_len.push_back(same - diff + cumulative_replace);
						cumulative_replace = 0;

						// Look for start of next diff block
						diff = same + ::FindFirstDiff(comp_bufa_ + same, comp_bufb_ + same, to_check - same);
					}

					addra += to_check;
					addrb += to_check;
					gota = 0;
					gotb = 0;
					continue;
				}
			}
			else
			{
				// Allowing insertions/deletions - first see if a difference was found
				if (diff < to_check)
				{
					// Move unchecked pieces of buffer down so they're 16-byte aligned
					addra += diff;
					addrb += diff;
					gota -= diff;
					gotb -= diff;
					memmove(comp_bufa_, comp_bufa_+diff, gota);
					memmove(comp_bufb_, comp_bufb_+diff, gotb);

					// Top up the buffers
					gota += GetData    (comp_bufa_ + gota, buf_size - gota + (min_match - 4), addra + gota, 4);
					gotb += GetCompData(comp_bufb_ + gotb, buf_size - gotb + (min_match - 4), addrb + gotb, true);

					const unsigned char * pfound;     // Pointer to the found bytes (in whichever buffer was searched)
					const unsigned char * pa, * pb;   // if found these point to matching bytes in the respective buffers
					size_t best, next;                // best (closest) found so far, and next offset to check
					int offset;                       // 0, 1, 2, or 3 dep on which pattern we match

					// We gradually move through both buffers searching if the patterns of bytes in one buffer 
					// matches anything further forward in the other buffer.  Note that Search4() effectively
					// performs 4 searches at once by finding any pattern starting with the next 4 bytes, and
					// the returned value (offset) from Search4() indicates which of the 4 patterns was found.
					for (next = 0, best = buf_size; next < best; next += 4)
					{
						size_t next16 = next - next%16;   // zero bottom 4 bits - this is used to ensure that the buffer searched is always 16-byte aligned

						// Search in buffer a for any pattern starting at any of the first 4 bytes of buffer b
						const unsigned char * to_search = comp_bufa_ + next16;
						size_t search_len = std::min(gota, best) - next16;         // restrict search to anything closer than best so far
	// xxx check gotb-next less than 7
						if (next < gota &&
							(pfound = ::Search4(to_search, search_len, comp_bufb_ + next, next, gotb - next, offset, min_match)) != NULL &&
							pfound - comp_bufa_ < best)
						{
							// remember that this is the closest match found so far
							best = pfound - comp_bufa_;
							// Remember where in both buffers that the match was found
							pa = pfound;
							pb = comp_bufb_ + next + offset;
						}

						// Now scan buffer b for the 4 patterns from the next position in buffer a
						to_search = comp_bufb_ + next16;
						search_len = std::min(gotb, best) - next16;
						if (next < gotb &&
							(pfound = ::Search4(to_search, search_len, comp_bufa_ + next, next, gota - next, offset, min_match)) != NULL &&
							pfound - comp_bufb_ < best)
						{
							best = pfound - comp_bufb_;
							pa = comp_bufa_ + next + offset;
							pb = pfound;
						}
					}

					if (best == buf_size)
					{
						// No match so add to current "replace" block
						size_t diff_len = std::min(gota, gotb); // xxx min buf_size or %16 xxx
						cumulative_replace += diff_len;
						addra += diff_len;
						addrb += diff_len;
						gota -= diff_len;
						gotb -= diff_len;
						// xxx memmove?
						continue;
					}

					size_t lena = pa - comp_bufa_;
					size_t lenb = pb - comp_bufb_;
					size_t replace_len = std::min(lena, lenb);

					if (cumulative_replace > 0 || replace_len > 0)
					{
						// Replace block
						result.m_replace_A.push_back(addra - cumulative_replace);
						result.m_replace_B.push_back(addrb - cumulative_replace);
						result.m_replace_len.push_back(cumulative_replace + replace_len);
						addra += replace_len;
						addrb += replace_len;
						gota -= replace_len;
						gotb -= replace_len;
						lena -= replace_len;
						lenb -= replace_len;
						cumulative_replace = 0;
					}

					if (lena < lenb)
					{
						// Deletion from a == insertion in b
						result.m_delete_A.push_back(addra);
						result.m_insert_B.push_back(addrb);
						result.m_delete_len.push_back(lenb - lena);
					}
					else if (lenb < lena)
					{
						// Insertion in a == deletion from b
						result.m_insert_A.push_back(addra);
						result.m_delete_B.push_back(addrb);
						result.m_insert_len.push_back(lena - lenb);
					}

					// Move the part of the buffer after the difference down
					addra += lena;
					addrb += lenb;
					gota -= lena;
					gotb -= lenb;
					memmove(comp_bufa_, comp_bufa_ + replace_len + lena, gota);
					memmove(comp_bufb_, comp_bufb_ + replace_len + lenb, gotb);
					continue;
				}  // end if difference
			}

			if (gota < buf_size || gotb < buf_size)
			{
				if (cumulative_replace > 0)
				{
					// A difference just happened to finish exactly at end of last read block
					result.m_replace_A.push_back(addra - cumulative_replace);
					result.m_replace_B.push_back(addrb - cumulative_replace);
					result.m_replace_len.push_back(cumulative_replace);
					cumulative_replace = 0;
				}

				// We have reached the end of one or both files
				if (gota < gotb)
				{
					gotb -= gota;
					gota = 0;

					result.m_delete_A.push_back(addra + diff);
					result.m_insert_B.push_back(addrb + diff);
					result.m_delete_len.push_back(CompLength() - (addrb + diff));  // to EOF of compare file
				}
				else if (gotb < gota)
				{
					gota -= gotb;
					gotb = 0;

					result.m_insert_A.push_back(addra + diff);
					result.m_delete_B.push_back(addrb + diff);
					result.m_insert_len.push_back(length_ - (addra + diff)); // to eof
				}

				// We save the results of the compare along with when it was done
				assert(cumulative_replace == 0);   // ensure we didn't miss this
				result.Final();

				// Check that vectors are in sync
				ASSERT(result.m_replace_A.size() == result.m_replace_B.size());
				ASSERT(result.m_replace_A.size() == result.m_replace_len.size());
				ASSERT(result.m_insert_A.size() == result.m_delete_B.size());
				ASSERT(result.m_insert_A.size() == result.m_insert_len.size());
				ASSERT(result.m_delete_A.size() == result.m_insert_B.size());
				ASSERT(result.m_delete_A.size() == result.m_delete_len.size());

				TRACE("+++ BGCompare: finished scan for %p\n", this);
				{
					CSingleLock sl(&docdata_, TRUE); // Protect shared data access

					comp_[0] = result;
					comp_fin_ = true;
					comp_progress_ = length_;
				}
				break;                          // falls out to wait state
			}

			// Skip the bits that compared equal
			addra += to_check;
			addrb += to_check;
			gota -= to_check;
			gotb -= to_check;
		}
		_aligned_free(comp_bufa_); comp_bufa_ = NULL;
		_aligned_free(comp_bufb_); comp_bufb_ = NULL;
	}
	return 0;  // never reached
}

// Check for a stop scanning (or kill) of the background thread
bool CHexEditDoc::CompProcessStop()
{
	bool retval = false;

	CSingleLock sl(&docdata_, TRUE);
	switch(comp_command_)
	{
	case STOP:                      // stop scan and wait
		TRACE1("+++ BGCompare: stop for %p\n", this);
		retval = true;
		break;
	case DIE:                       // terminate this thread
		TRACE1("+++ BGCompare: killed thread for %p\n", this);
		comp_state_ = DYING;
		sl.Unlock();                // we need this here as AfxEndThread() never returns so d'tor is not called
		_aligned_free(comp_bufa_); comp_bufa_ = NULL;
		_aligned_free(comp_bufb_); comp_bufb_ = NULL;
		AfxEndThread(1);            // kills thread (no return)
		break;                      // Avoid warning
	case NONE:                      // nothing needed here - just continue scanning
		break;
	default:                        // should not happen
		ASSERT(0);
	}

	// Prevent reprocessing of the same command
	comp_command_ = NONE;
	return retval;
}
