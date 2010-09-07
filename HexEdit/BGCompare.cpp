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
	if (!GetCompareFile(true))
		return;

	CHexEditView * phev = NULL;            // remembers the active hex view
    CHexEditView * pactive = ::GetView();  // current active hex view (should be for this doc)

	// First we need to close all the compare views of this document.  (There
	// can be more than one if there is more than one hex view open on this doc.)
	// However, if we close a view while we are iterating through them (using
	// GetFirstViewPosition/GetNextView) then this really confuses MFC (even if
	// we just send a hint to the view since UpdateAllViews uses GetNextView).
	// So we make a list of all hex view, then later close their compare views.
	std::vector<CHexEditView *> pviews;

	// Find all hex views of this doc
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
	ASSERT(phev != NULL);

	// Kill all compare views (each must be associated with a hex view)
	for (std::vector<CHexEditView *>::const_iterator ppv = pviews.begin(); ppv != pviews.end(); ++ppv)
		(*ppv)->OnCompHide();

	ASSERT(cv_count_ == 0 && pthread4_ == NULL);  // we should have closed all compare views and hence killed the compare thread

	// This triggers opening of compare file and start of background compare.
	// Actually I should explain...
	// It calls DoCompSplit (to create the compare view in a split window)
	// then sends the new view a WM_INITIALUPDATE message which triggers 
	// CCompareView::OninitialUpdate.  OninitialUpdate allows the view to 
	// register itself with its document using CHexEditDoc::AddCompView()
	// which call CreateCompThread (sets up and starts the compare thread)
	// then pulses start_comp_event_ to trigger the thread to start comparing.
    if (phev != NULL)
		phev->DoCompSplit();
}

void CHexEditDoc::AddCompView(CHexEditView *pview)
{
	TRACE("===== Add Comp View %d\n", cv_count_);
	if (++cv_count_ == 1)
	{
		ASSERT(pthread4_ == NULL);

		// Create the background thread and start it scanning
		if (!CreateCompThread())
		{
			cv_count_ = 0;   // indicate that no compare views are open
			return;
		}
		TRACE("===== Pulsing compare event\n");
		start_comp_event_.SetEvent();
	}
    ASSERT(pthread4_ != NULL);
}

void CHexEditDoc::RemoveCompView()
{
    if (--cv_count_ == 0)
    {
		ASSERT(pthread4_ != NULL);
        if (pthread4_ != NULL)
            KillCompThread();
    }
	TRACE("===== Remove Comp View %d\n", cv_count_);
}

bool CHexEditDoc::GetCompareFile(bool bForcePrompt /*=false*/)
{
    CHexFileList *pfl = theApp.GetFileList();
	int idx = -1;                 // recent file list idx of current document
#ifdef  ONLY_FILE_COMPARE
	bCompSelf = false;  // xxx TBD

	// Get last name of the last compare file used
    if (pfl != NULL &&
		pfile1_ != NULL &&
		(idx = pfl->GetIndex(pfile1_->GetFilePath())) > -1)
	{
		compFileName_ = pfl->GetData(idx, CHexFileList::COMPFILENAME);
	}

	if (!bForcePrompt && !compFileName_.IsEmpty() && _access(compFileName_, 0) != -1)
		return true;         // we can just reopen the same file

	CHexFileDialog dlgFile("CompareFileDlg", HIDD_FILE_COMPARE, TRUE, NULL, compFileName_,
						   OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						   theApp.GetCurrentFilters(), "Compare", AfxGetMainWnd());

	dlgFile.m_ofn.lpstrTitle = "Compare To File";

	if (dlgFile.DoModal() != IDOK)
		return false;

	compFileName_ = dlgFile.GetPathName();
    if (pfl != NULL && idx > -1)
		pfl->SetData(idx, CHexFileList::COMPFILENAME, compFileName_);
#else
	if (bCompSelf && !compFileName_.IsEmpty())  // move to where pfile1_compare closed? xxx
		::remove(compFileName_);

	bCompSelf = true;

	// make temp file name
    char temp_dir[_MAX_PATH];
    char temp_file[_MAX_PATH];
    ::GetTempPath(sizeof(temp_dir), temp_dir);
    ::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);
	::CopyFile(pfile1_->GetFilePath(), temp_file, FALSE);
    compFileName_ = temp_file;

    if (pfl != NULL && (idx = pfl->GetIndex(pfile1_->GetFilePath())) > -1)
		pfl->SetData(idx, CHexFileList::COMPFILENAME, "*");   // indicates self-compare
#endif
	return true;
}

// Check if file we are comparing against has chnaged since we last finished comparing
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

CString CHexEditDoc::GetCompFileName()
{
	if (bCompSelf)
		return CString("*");
	else
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
// the numbers of diffs if 'from' id past the last one.
int CHexEditDoc::FirstDiffAt(int dd, FILE_ADDRESS from)
{
	int bot = 0;
	int top = comp_[dd].m_addr.size();
	while (bot < top)
	{
		// Get the element roughly halfway in between (binary search)
		int curr = (bot+top)/2;
		if (from < comp_[dd].m_addr[curr] + comp_[dd].m_len[curr])
			top = curr;
		else
			bot = curr + 1;
	}
	return bot;
}

#ifdef DANIELS
int CHexEditDoc::FirstDiffAt(int dd, FILE_ADDRESS from)
{
	return FirstDiffAtInRange(0, comp_[dd].m_addr.size(), dd, from);

	// for address of diff at index n use: comp_[dd].m_addr[n]
	// for length of diff at index n use: comp_[dd].m_len[n]
}

// Returns the index of the first diff at or after address 'from' within a range.
// Recursive Function
int CHexEditDoc::FirstDiffAtInRange(int bot, int top, int dd, FILE_ADDRESS from)
{
	// If search has been narrowed to a single index
	if (bot == top)
	{
		// If no diff was found
		// return the length
		if (top == comp_[dd].m_addr.size())
		{
			return comp_[dd].m_addr.size();
		}
		// return the index
		return bot;
	}
	
	// If diff is in the top or bottom half of the bounds
	if (comp_[dd].m_addr[(bot+top)/2] + comp_[dd].m_len[(bot+top)/2] > from)
	{
		// Recurr
		return FirstDiffAtInRange(bot, (bot+top)/2, dd, from);
	}
	else
	{
		// Returns if an index has been found,
		// Without this, the function would continue reccuring.
		if (top - bot <= 1)
		{
			return top;
		}
		// Recurr
		return FirstDiffAtInRange((bot+top)/2, top, dd, from);
	}
}
#endif

// Returns the number of diffs (in bytes) OR
// -4 = bg compare not on
// -2 = bg compare is still in progress
int CHexEditDoc::CompareDifferences(int dd /*=0*/)
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

	return comp_[dd].m_addr.size(); 
}

// Return how far our background compare has progressed as a percentage (0 to 100).
// Also return the number of differences found so far (in bytes).
int CHexEditDoc::CompareProgress()
{
    CSingleLock sl(&docdata_, TRUE);
 	if (comp_state_ != SCANNING)
		return 100;

	return 1 + int((comp_progress_ * 99)/length_);  // 1-100 (don't start at zero)
}

// Returns:
// -4 = compare not running
// -2 = still in progress
// -1 = no more diffs
// else index of difference
int CHexEditDoc::GetNextDiff(FILE_ADDRESS from)
{
    if (pthread4_ == NULL)
        return -4;

	return 0; // xxx TBD
}

int CHexEditDoc::GetPrevDiff(FILE_ADDRESS from)
{
    if (pthread4_ == NULL)
        return -4;

	return 0; // xxx TBD
}

// Start a new comparison.
void CHexEditDoc::StartComp()
{
    if (cv_count_ == 0) return;
    ASSERT(pthread4_ != NULL);
    if (pthread4_ == NULL) return;

	// stop background compare (if any)
	docdata_.Lock();
	bool waiting = comp_state_ == WAITING;
	comp_command_ = STOP;
	comp_fin_ = FALSE;
	docdata_.Unlock();

	if (!waiting)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
	    SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_NORMAL);
		Sleep(1);
	    SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_LOWEST);
		docdata_.Lock();
		waiting = comp_state_ == WAITING;
		docdata_.Unlock();
	}
	ASSERT(waiting);

	// Restart the compare
	docdata_.Lock();

	comp_command_ = RESTART;
	comp_state_ = STARTING;
	comp_progress_ = 0;
	comp_fin_ = FALSE;

	// Save current file modification time so we don't keep restarting the compare
	CFileStatus stat;
	pfile4_compare_->GetStatus(stat);
	comp_[0].Reset(stat.m_mtime);

	docdata_.Unlock();

	// Now that the compare is stopped we need to update all views to remove existing compare display
    CCompHint comph;
    UpdateAllViews(NULL, 0, &comph);

	if (waiting)
	{
		TRACE("Pulsing compare event (restart)\r\n");
		start_comp_event_.SetEvent();
	}
}

// Sends a message for the thread to kill itself then tidies up shared members 
void CHexEditDoc::KillCompThread()
{
    ASSERT(pthread4_ != NULL);
    if (pthread4_ == NULL) return;

    HANDLE hh = pthread4_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
    TRACE1("===== Killing compare thread for %p\n", this);

    // Signal thread to kill itself
    docdata_.Lock();
    bool waiting = comp_state_ == WAITING;
    comp_command_ = DIE;
    docdata_.Unlock();

    SetThreadPriority(pthread4_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death

	if (!waiting)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		Sleep(500);
		docdata_.Lock();
		bool waiting = comp_state_ == WAITING;
		docdata_.Unlock();
	}

    // Send start message in case it is on hold
    if (waiting)
        start_comp_event_.SetEvent();

    pthread4_ = NULL;
    DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
    ASSERT(wait_status == WAIT_OBJECT_0);

    // Free resources that are only needed during bg searches
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
    if (pfile4_compare_ != NULL)
	{
		pfile4_compare_->Close();
		delete pfile4_;
		pfile4_compare_ = NULL;
	}
    if (pfile1_compare_ != NULL)
	{
		pfile1_compare_->Close();
		delete pfile4_;
		pfile1_compare_ = NULL;
	}
}

static UINT bg_func(LPVOID pParam)
{
    CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

    TRACE1("Compare thread started for doc %p\n", pDoc);

    return pDoc->RunCompThread();
}

bool CHexEditDoc::CreateCompThread()
{
    ASSERT(pthread4_ == NULL);
    ASSERT(pfile4_ == NULL);

    // Open copy of original file to be used by background thread
    if (pfile1_ != NULL)
	{
		if (IsDevice())
			pfile4_ = new CFileNC();
		else
			pfile4_ = new CFile64();
		if (!pfile4_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("Compare (file4) open failed for %p\n", this);
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

	// Open the file to compare against
	// Note: there are two files (the original file and the file comparing against).
	// We open them both twice so we have a CFile64 instance for foreground and background threads
	//   pfile1_         = Original file opened for general use in GUI thread
	//   pfile1_compare_ = Compare file opened for use in the main (GUI) thread (used by CCompareView)
	//   pfile4_         = Original file opened again to use for comparing in the background thread (see below)
	//   pfile4_compare_ = Compare file opened again for use in the background thread
	pfile1_compare_ = new CFile64();
	pfile4_compare_ = new CFile64();
	if (!pfile1_compare_->Open(compFileName_, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) ||
		!pfile4_compare_->Open(compFileName_, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary))
	{
		TRACE1("Compare file open failed for %p\n", this);
		return false;
	}

    // Create new thread
    TRACE1("===== Creating thread for %p\n", this);
	comp_command_ = RESTART;
	comp_state_ = STARTING;
	comp_progress_ = 0;
	comp_fin_ = FALSE;

	// Save current file modification time so we don't keep restarting the compare
	CFileStatus stat;
	pfile4_compare_->GetStatus(stat);
	comp_[0].Reset(stat.m_mtime);

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
        TRACE1("BGCompare: waiting for %p\n", this);
        DWORD wait_status = ::WaitForSingleObject(HANDLE(start_comp_event_), INFINITE);
        start_comp_event_.ResetEvent();
        ASSERT(wait_status == WAIT_OBJECT_0);
        TRACE1("BGCompare: got event for %p\n", this);

		FILE_ADDRESS addr = 0;
		CompResult result;

		// Get buffers for each source
		const int buf_size = 8192;   // xxx may need to be dynamic later (based on sync length)
		unsigned char *bufa = new unsigned char[buf_size];
		unsigned char *bufb = new unsigned char[buf_size];

		// Keep looping until we are finished processing blocks or we receive a command to stop etc
        for (;;)
        {
             // First check what we need to do
            {
                CSingleLock sl(&docdata_, TRUE);
                switch(comp_command_)
                {
                case RESTART:                   // restart scan from beginning
                    comp_command_ = NONE;
                    comp_state_ = SCANNING;
					comp_progress_ = addr = 0;
					result = comp_[0];
                    TRACE1("BGCompare: restart for %p\n", this);
                    break;
                case STOP:                      // stop scan and wait
                    comp_command_ = NONE;
                    comp_state_ = WAITING;
                    TRACE1("BGCompare: stop for %p\n", this);
                    goto end_scan;
                case DIE:                       // terminate this thread
                    comp_command_ = NONE;
                    comp_state_ = DYING;
                    TRACE1("BGCompare: killed thread for %p\n", this);
                    return 1;
                case NONE:                      // nothing needed here - just continue scanning
                    ASSERT(comp_state_ == SCANNING);
					comp_progress_ = addr;                // Used for displaying progress bar
                    break;
                default:                        // should not happen
                    ASSERT(0);
                }
            }  // end lock of docdata_

			int gota, gotb;  // Amount of data obtained from each file

			// Get the next chunks
			if ((gota = GetData    (bufa, buf_size, addr, 4)) <= 0 ||
			    (gotb = GetCompData(bufb, buf_size, addr, true)) <= 0)
			{
				// We save the results of the compare along with when it was done
				result.Final();

                TRACE("BGCompare: finished scan for %p\n", this);
                CSingleLock sl(&docdata_, TRUE); // Protect shared data access

				comp_[0] = result;
                comp_fin_ = TRUE;
                break;                          // falls out to end_scan
			}

			int pos = 0, endpos = std::min(gota, gotb);   // The bytes of bufa/bufb to compare

			// Process this block into same/different sections
			while (pos < endpos)
			{
				// TODO xxx get rid of memcmp and byte compares - we can do 32-bit compares
				// for speed and so we know where the scan stopped (even REPE/REPNE + CMPSD)
				// First see if the (rest of the) block is the same
				if (memcmp(bufa+pos, bufb+pos, endpos-pos) == 0)
				{
					// The rest of the block is the same
					pos = endpos;
					break;
				}
				// Find out where the difference starts
				for ( ; pos < endpos; ++pos)
					if (bufa[pos] != bufb[pos])
						break;
				result.m_addr.push_back(addr + pos);

				ASSERT(pos < endpos);   // must have found a difference
				for ( ; pos < endpos; ++pos)
					if (bufa[pos] == bufb[pos])
						break;

				result.m_len.push_back(int(addr + pos - result.m_addr.back()));
				ASSERT(result.m_addr.size() == result.m_len.size());     // must always be same length
			}

			addr += std::min(gota, gotb);
        }
    end_scan:
        ;
    }
    return 1;
}
