// BGCompare.cpp : implements background compare (part of CHexEditDoc)
//
// Copyright (c) 2010 by Andrew W. Phillips.
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
#include "HexEdit.h"

#include "HexEditDoc.h"
#include "HexEditView.h"
#include "HexFileList.h"
#include "Dialog.h"
#include "NewCompare.h"

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

	if (compareFile.IsEmpty())
	{
		dlg.compare_type_ = 0;                                                               // default to self-compare
		dlg.file_name_ = 	theApp.GetProfileString("File-Settings", "LastCompareFile", ""); // but get last used file name
	}
	else if (compareFile.Compare("*") == 0)
	{
		// If we don't have to prompt then we can proceed with self-compare
		if (!bForcePrompt)
		{
			return view_type;                 // return asked for view type (or error if asked for type is 0)
		}
		dlg.compare_type_ = 0;                                                           // "*" means to use self-compare
		dlg.file_name_ = 	theApp.GetProfileString("File-Settings", "LastCompareFile", ""); // but also get last used file name (disabled)
	}
	else
	{
		// If we don't have to prompt and the current file name exists then we can proceed using that file
		if (!bForcePrompt && _access(compareFile, 0) != -1)
			return view_type;                 // return asked for view type (or error if asked for type is 0)

		dlg.compare_type_ = 1;
		dlg.file_name_ = compareFile;
	}
	dlg.compare_display_ = int(view_type) - 1;
	dlg.auto_sync_ = auto_sync;
	dlg.auto_scroll_= auto_scroll;

	if (dlg.DoModal() != IDOK)
		return none;

	// Store selected values
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

// Returns index of first diff at or after address 'from', which may be
// the numbers of diffs if 'from' is past the last one.
// If we have a list of "historical" results then rr is the desired one, but a value
// of zero for rr is always valid - gives the recent diffs (could be empty).
// We can look through the diffs from the perspective of the original file (other==false)
// or the comare file (other==true), since the addresses may be different between
// them if there have been insertions/deletions.
int CHexEditDoc::FirstDiffAt(bool other, int rr, FILE_ADDRESS from)
{
	std::vector<__int64> & addr = other ? comp_[rr].m_addrB : comp_[rr].m_addrA;

	CSingleLock sl(&docdata_, TRUE);
	int bot = 0;
	int top = addr.size();
	while (bot < top)
	{
		// Get the element roughly halfway in between (binary search)
		int curr = (bot+top)/2;
		if (from <= addr[curr])
			top = curr;         // now check bottom half
		else
			bot = curr + 1;     // check top half
	}
	return bot;
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
	// xxx TBD?
	ASSERT(rr >= 0 && rr < comp_.size());  // also ensures
	ASSERT(comp_[rr].m_addrA.size() == comp_[rr].m_addrB.size());
	return comp_[rr].m_addrA.size();
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

// Returns the address and length of the first difference or
// -1 if there are no differences.
void CHexEditDoc::GetFirstDiffAll(FILE_ADDRESS &addr, int &len)
{
	addr = -1;
	if (pthread4_ == NULL) return;

	CSingleLock sl(&docdata_, TRUE);

	for (int rr = 0; rr < ResultCount(); ++rr)
	{
		if (comp_[rr].m_addrA.size() == 0)
			continue;    // no diffs at all in this result set

		if (addr < 0 || comp_[rr].m_addrA[0] < addr)
		{
			addr = comp_[rr].m_addrA[0];
			len  = comp_[rr].m_len[0];
		}
	}
}

void CHexEditDoc::GetPrevDiffAll(FILE_ADDRESS from, FILE_ADDRESS &addr, int &len)
{
	addr = -1;
	if (pthread4_ == NULL) return;

	for (int rr = 0; rr < ResultCount(); ++rr)
	{
		int idx = FirstDiffAt(false, rr, from);

		CSingleLock sl(&docdata_, TRUE);
		if (idx == 0)
			continue;     // before first diff

		idx--;           // goto previous one

		ASSERT(idx > -1 && idx < comp_[rr].m_addrA.size());
		if (addr < 0 || comp_[rr].m_addrA[idx] + comp_[rr].m_len[idx] > addr + len)
		{
			addr = comp_[rr].m_addrA[idx];
			len  = comp_[rr].m_len[idx];
		}
	}
}

void CHexEditDoc::GetNextDiffAll(FILE_ADDRESS from, FILE_ADDRESS &addr, int &len)
{
	addr = -1;
	if (pthread4_ == NULL) return;

	for (int rr = 0; rr < ResultCount(); ++rr)
	{
		int idx = FirstDiffAt(false, rr, from);

		CSingleLock sl(&docdata_, TRUE);
		if (idx == comp_[rr].m_addrA.size())
			continue;     // past last diff in this result

		ASSERT(idx > -1 && idx < comp_[rr].m_addrA.size());
		if (addr < 0 || comp_[rr].m_addrA[idx] < addr)
		{
			addr = comp_[rr].m_addrA[idx];
			len  = comp_[rr].m_len[idx];
		}
	}
}

// Returns the address and length of the last difference or
// -1 for the address if there are no differences.
void CHexEditDoc::GetLastDiffAll(FILE_ADDRESS &addr, int &len)
{
	addr = -1;
	if (pthread4_ == NULL) return;

	CSingleLock sl(&docdata_, TRUE);

	for (int rr = 0; rr < ResultCount(); ++rr)
	{
		if (comp_[rr].m_addrA.size() == 0)
			continue;    // no diffs in this result set

		int last = comp_[rr].m_addrA.size() - 1;
		if (addr < 0 || comp_[rr].m_addrA[last] > addr)
		{
			addr = comp_[rr].m_addrA[last];
			len  = comp_[rr].m_len[last];
		}
	}
}

// Returns:
// -4 = compare not running
// -2 = still in progress
// -1 = no more diffs
// else index of difference
int CHexEditDoc::GetNextDiff(bool other, FILE_ADDRESS from, int rr /*=0*/)
{
	if (pthread4_ == NULL)
		return -4;

	{
		CSingleLock sl(&docdata_, TRUE);
		if (comp_state_ != WAITING)
			return -2;
	}

	int ii = FirstDiffAt(other, rr, from);
	ASSERT(comp_[rr].m_addrA.size() == comp_[rr].m_addrB.size());
	if (ii == comp_[rr].m_addrA.size())
		return -1;

	return ii;
}

int CHexEditDoc::GetPrevDiff(bool other, FILE_ADDRESS from, int rr /*=0*/)
{
	if (pthread4_ == NULL)
		return -4;

	{
		CSingleLock sl(&docdata_, TRUE);
		if (comp_state_ != WAITING)
			return -2;
	}

	int ii = FirstDiffAt(other, rr, from);
	if (ii == 0)
		return -1;

	return ii - 1;
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
	docdata_.Unlock();

	// Now that the compare is stopped we need to update all views to remove existing compare display
	CCompHint comph;
	UpdateAllViews(NULL, 0, &comph);

	TRACE("+++ Pulsing compare event (restart)\r\n");
	start_comp_event_.SetEvent();
}

// Sends a message for the thread to kill itself then tidies up shared members 
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

		comp_progress_ = 0;
		FILE_ADDRESS addr = 0;
		CompResult result;
		result = comp_[0];

		// Get buffers for each source
		const int buf_size = 8192;   // xxx may need to be dynamic later (based on sync length)
		ASSERT(comp_bufa_ == NULL && comp_bufb_ == NULL);
		comp_bufa_ = new unsigned char[buf_size];
		comp_bufb_ = new unsigned char[buf_size];

		// Keep looping until we are finished processing blocks or we receive a command to stop etc
		for (;;)
		{
			// First check if we need to stop
			if (CompProcessStop())
				break;   // stop processing and go back to WAITING state

			int gota, gotb;  // Amount of data obtained from each file

			// Get the next chunks
			gota = GetData    (comp_bufa_, buf_size, addr, 4);
			gotb = GetCompData(comp_bufb_, buf_size, addr, true);

			int pos = 0, endpos = std::min(gota, gotb);   // The bytes of comp_bufa_/comp_bufb_ to compare

			// Process this block into same/different sections
			while (pos < endpos)
			{
				// TODO xxx get rid of memcmp and byte compares - we can do 32-bit compares
				// for speed and so we know where the scan stopped (even REPE/REPNE + CMPSD)
				// First see if the (rest of the) block is the same
				if (memcmp(comp_bufa_ + pos, comp_bufb_ + pos, endpos-pos) == 0)
				{
					// The rest of the block is the same
					pos = endpos;
					break;
				}
				// Find out where the difference starts
				for ( ; pos < endpos; ++pos)
					if (comp_bufa_[pos] != comp_bufb_[pos])
						break;
				// xxx for now addresses are always the same
				result.m_addrA.push_back(addr + pos);
				result.m_addrB.push_back(addr + pos);

				ASSERT(pos < endpos);   // must have found a difference
				for ( ; pos < endpos; ++pos)
					if (comp_bufa_[pos] == comp_bufb_[pos])
						break;

				result.m_len.push_back(int(addr + pos - result.m_addrA.back()));
				result.m_type.push_back(CompResult::Replacement);

				ASSERT(result.m_addrA.size() == result.m_len.size());     // must always be same length
				ASSERT(result.m_addrA.size() == result.m_addrB.size());
				ASSERT(result.m_addrA.size() == result.m_type.size());
			}

			addr += std::min(gota, gotb);
			if (gota < gotb)
			{
				gotb -= gota;
				gota = 0;

				result.m_addrA.push_back(addr);
				result.m_addrB.push_back(addr);
				result.m_len.push_back(gotb);
				result.m_type.push_back(CompResult::Deletion);  // extra bytes in compare file
			}
			else if (gotb < gota)
			{
				gota -= gotb;
				gotb = 0;

				result.m_addrA.push_back(addr);
				result.m_addrB.push_back(addr);
				result.m_len.push_back(gota);
				result.m_type.push_back(CompResult::Insertion);  // extra byte(s) in orig file
			}

			if (gota <= 0 || gotb <= 0)
			{
				// We save the results of the compare along with when it was done
				result.Final();

				TRACE("+++ BGCompare: finished scan for %p\n", this);
				CSingleLock sl(&docdata_, TRUE); // Protect shared data access

				comp_[0] = result;
				comp_fin_ = true;
				break;                          // falls out to wait state
			}
		}
		delete[] comp_bufa_; comp_bufa_ = NULL;
		delete[] comp_bufb_; comp_bufb_ = NULL;
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
		delete[] comp_bufa_; comp_bufa_ = NULL;
		delete[] comp_bufb_; comp_bufb_ = NULL;
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
