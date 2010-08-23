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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef min  // avoid corruption of std::min/std::max
#undef min
#undef max
#endif

// New compare - open the file to compare against and show the compare view
void CHexEditDoc::OnCompNew()
{
	// Get the file name from the user - TODO xxx
	CString fileName("C:\\tmp\\daily\\xxx.bin");

	// Open the file and start the background compare
	// Note: there are two files used for the compare (the original file and the file comparing against)
	// pfile1_compare_ = Compare file opened for use in the main (GUI) thread
	// pfile4_         = Original file opened again to use for comparing in the background thread (see below)
	// pfile4_compare_ = Compare file opened again for use in the background thread (used by CCompareView)
	pfile1_compare_ = new CFile64();
	pfile4_compare_ = new CFile64();
	if (!pfile1_compare_->Open(fileName, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) ||
		!pfile4_compare_->Open(fileName, CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary))
	{
		TRACE1("Compare file opens failed for %p\n", this);
		return;
	}

	// Should we update here or wait till the background compare has finished
    CCompHint comph;
    UpdateAllViews(NULL, 0, &comph);

	CHexEditView *pv = GetBestView();
	if (pv != NULL)
		pv->ShowComp();  // This triggers opening of files and start of background compare.
	                     // Actually I should explain: ChexEditView::ShowComp calls OnCompSplit which
	                     // calls DoCompSplit (to create the compare view in a split window) then
	                     // sends the new view a WM_INITIALUPDATE message which triggers 
	                     // CCompareView::OninitialUpdate.  OninitialUpdate allows the view to 
	                     // register itself with its doc using CHexEditDoc::AddCompView (below)
	                     // which call CreateCompThread (sets up and starts the compare thread)
	                     // then pulses start_comp_event_ to trigger the thread to start comparing.
}

// Get data from the file we are comparing against
size_t CHexEditDoc::GetCompData(unsigned char *buf, size_t len, FILE_ADDRESS loc, bool use_bg /*=false*/)
{
	CFile64 *pf = use_bg ? pfile4_compare_ : pfile1_compare_;  // Select background or foreground file

	if (pf->Seek(loc, CFile::begin) == -1)
		return -1;
	return pf->Read(buf, len);
}

static UINT bg_func(LPVOID pParam)
{
    CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

    TRACE1("Compare thread started for doc %p\n", pDoc);

    return pDoc->RunCompThread();
}

void CHexEditDoc::AddCompView(CHexEditView *pview)
{
	if (++cv_count_ == 1)
	{
		ASSERT(pthread4_ == NULL);
		// xxx init things

		// Create the background thread and start it scanning
		CreateCompThread();
		TRACE("Pulsing compare event\n");
		start_comp_event_.SetEvent();
	}
    ASSERT(pthread4_ != NULL);
}

void CHexEditDoc::RemoveCompView()
{
    if (--cv_count_ == 0)
    {
        if (pthread4_ != NULL)
            KillCompThread();
		// xxx close comp files?
    }
}

void CHexEditDoc::CreateCompThread()
{
    ASSERT(pthread4_ == NULL);
    ASSERT(pfile4_ == NULL);

    // Init some things
    comp_fin_ = FALSE;

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
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file4_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file4_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

    // Create new thread
    TRACE1("Creating thread for %p\n", this);
	comp_command_ = RESTART;
	comp_state_ = STARTING;
    pthread4_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
    ASSERT(pthread4_ != NULL);
}

// Kill background task and wait until it is dead
void CHexEditDoc::KillCompThread()
{
    ASSERT(pthread4_ != NULL);
    if (pthread4_ == NULL) return;

    HANDLE hh = pthread4_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
    TRACE1("FG: Killing compare thread for %p\n", this);

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

	// xxx reset any other things used by the thread
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

	// xxx set up new compare

	// Restart the compare
	docdata_.Lock();
	waiting = comp_state_ == WAITING;
	comp_command_ = RESTART;
	comp_fin_ = FALSE;
	docdata_.Unlock();

	// Now that the compare is stopped we need to update all views to remove existing compare display
    CBGSearchHint bgsh(FALSE);
    UpdateAllViews(NULL, 0, &bgsh);

	if (waiting)
	{
		TRACE("Pulsing compare event (restart)\r\n");
		start_comp_event_.SetEvent();
	}
}

// Returns the number of diffs (in bytes) OR
// -4 = bg compare not on
// -2 = bg compare is still in progress
__int64 CHexEditDoc::CompareDifferences()
{
    if (pthread4_ == NULL)
    {
        return -4;
    }

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

	return 0; // xxx TBD 
}

// Return how far our background compare has progressed as a percentage (0 to 100).
// Also return the number of differences found so far (in bytes).
int CHexEditDoc::CompareProgress(__int64 &diffs)
{
    docdata_.Lock();
    FILE_ADDRESS file_len = length_;
    docdata_.Unlock();

	diffs = 0; return 0; // xxx TBD
}

// Returns:
// -4 = compare not running
// -2 = still in progress
// -1 = no more diffs
// else file address of first byte of difference
FILE_ADDRESS CHexEditDoc::GetNextDiff(FILE_ADDRESS from)
{
    if (pthread4_ == NULL)
        return -4;

	return 0; // xxx TBD
}

FILE_ADDRESS CHexEditDoc::GetPrevDiff(FILE_ADDRESS from)
{
    if (pthread4_ == NULL)
        return -4;

	return 0; // xxx TBD
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
					addr = 0;
					result.Reset();
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
				// We store
				CFileStatus stat;
				pfile4_compare_->GetStatus(stat);
				result.Final(stat.m_mtime);

                TRACE("BGCompare: finished scan for %p\n", this);
                CSingleLock sl(&docdata_, TRUE); // Protect shared data access

				comp_result_ = result;
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
