// BGAerial.cpp : background scan of file to create aerial view "bitmap"
//
// Copyright (c) 2008-2010 by Andrew W. Phillips.
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

/*
Aerial View Scanning
====================

A background thread is created to scan the file to build up a bitmap for
display in an "aerial view" which can show an alternative view to the
normal hex view of a document.

Note This background scan  may later also be used to generate stats
on the file such as byte counts.

Document
--------

A background thread is created for every document that has at least
one aerial view.  It has a low priority (LOWEST?).  Communication with
the thread is achieved using these document members:

start_aerial_event_: signals the thread it needs to do something when in wait state
aerial_command_: command to the thread to tell it what to do: scan, stop, die
aerial_state_: updated by the thread to say what it's doing: waiting, scanning or dying
aerial_fin_: true if last scan finished OK, false if none done yet or last stopped
docdata_: a critical section to protect access to shared document members

pfile3_: is a ptr to file open the same as pfile1_.  Using a separate file allows the main thread
		 to read from the file without having to lock docdata_.  Locking is only required
		 when the background thread accesses the file or the main thread changes it.
		 This means that file display should never be slowed by the background thread.
loc_: accessed (via GetData) in main/search/aerial threads to get data from the file
undo_: loc_ uses data stored in undo array


Background thread
-----------------

The thread has 4 states: starting, waiting, scanning and dying.  When waiting it is
blocked by the event (start_aerial_event_), and to change the state the event needs to be
pulsed (and a command given).

While scanning it regularly checks for a new command (aerial_command_) while in its
processing loop.  If it detects a "stop" command it goes back into the wait state.
If it detects a "die" command the thread terminates itself.

After the thread scans the whole file it also goes back into the wait state.
But first it signals the main thread which is passed on to the document
and thence to all aerial views so they can update themselves.
Note that if a modal dialog is active when the ::PostThreadMessage is called
to send the message to the main thread then the message is lost.
It also sets aerial_fin_ to true.

Changes (CHexEditDoc::Change, CHexEditDoc::Undo in DocData.cpp)
-------

When a document is changed the scan needs to be restarted.

This may later be changed to allow small document changes to update
the bitmap without having to rescan the whole thing.  This would work
fine for replacements.  For deletions a memove() would be required.
For small insertions only a memove may be required but it may be that
the bitmap has to be resized, which may be handled by realloc or
just a complete re-scan.


Views (see CBGAerialHint used by CHexEditView::OnUpdate)
-----

When the colour scheme of the view used to generate the aerial view
is changed then the file has to be rescanned as the bitmap
colours may be completely different.  This is complicated by the 
fact that a document may have more than one view using different
colour schemes.

*/

#include "stdafx.h"
#include "HexEdit.h"
#include <memory.h>

#include "HexEditDoc.h"
#include "HexEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Add an aerial view for this doc.  The doc has to remember how many there are so it
// can free up things when there are no more.  (There can be more than one if a 2nd
// window has been opened on the same document.)
// It has to know the associated CHexEditView in order to know the colour scheme and
// hence the colours assigned in the FreeImage bitmap.

void CHexEditDoc::AddAerialView(CHexEditView *pview)
{
	TRACE("+++ Aerial +++ %d\n", av_count_);
	if (++av_count_ == 1)
	{
		// Check if we are using a "boring" colour scheme in order to ask the user if they
		// want to use the "many" (colourful) colour scheme.
		CString strScheme = pview->GetSchemeName();
		if (strScheme == ASCII_NAME   || strScheme == PLAIN_NAME   ||
			strScheme == ANSI_NAME    || strScheme == OEM_NAME     ||
			strScheme == EBCDIC_NAME  || strScheme == UNICODE_NAME ||
			strScheme == CODEPAGE_NAME )
		{
			CString ss;
			ss.Format("The current %s scheme will not distinguish between "
			          "many different byte values.  It is recommended that "
			          "you first switch to a better scheme to more easily "
			          "see patterns in the Aerial View display.\n\n"
			          "Do you wish to switch to the \"Many\" %s scheme?",
			          ::IsUs() ? "color" : "colour",
					  ::IsUs() ? "color" : "colour");
			if (CAvoidableDialog::Show(IDS_USE_AERIAL_SCHEME, ss, NULL, MLCBF_YES_BUTTON | MLCBF_NO_BUTTON) == IDYES)
			{
				pview->SetScheme(MULTI_NAME);
			}
		}

		//get bitmap & clear it to closest grey to hex view's background
		GetAerialBitmap(GetRValue(same_hue(pview->GetBackgroundCol(), 0 /*saturation*/)));
		pview->get_colours(kala_);   // get colours for the bitmap pixels

		// Create the background thread and start it scanning
		CreateAerialThread();
		TRACE("+++ Pulsing aerial event\n");
		start_aerial_event_.SetEvent();
	}
	ASSERT(pthread3_ != NULL);
}

void CHexEditDoc::RemoveAerialView()
{
	if (--av_count_ == 0)
	{
		if (pthread3_ != NULL)
			KillAerialThread();
		FIBITMAP *dib = dib_;
		dib_ = NULL;
		TRACE("+++  FreeImage_Unload(%d)\n", dib);
		FreeImage_Unload(dib);
	}
	TRACE("+++  Aerial --- %d\n", av_count_);
}

// Doc or colours have changed - signal current scan to stop then start new scan
void CHexEditDoc::AerialChange(CHexEditView *pview /*= NULL*/)
{
	if (av_count_ == 0) return;
	ASSERT(pthread3_ != NULL);
	if (pthread3_ == NULL) return;

	// Wait for thread to stop if necessary
	bool waiting;
	docdata_.Lock();
	aerial_command_ = STOP;
	docdata_.Unlock();
	SetThreadPriority(pthread3_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = aerial_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ AerialChange - thread not waiting (yet)\n");
		Sleep(1);
	}
	SetThreadPriority(pthread3_->m_hThread, THREAD_PRIORITY_LOWEST);
	ASSERT(waiting);

	// Make sure we have a big enough bitmap and the right colours
	docdata_.Lock();
	if (pview != NULL)
	{
		// ensure bitmap is big enough & clear it to closest grey to hex view's background
		GetAerialBitmap(GetRValue(same_hue(pview->GetBackgroundCol(), 0 /*saturation*/)));
		pview->get_colours(kala_);     // get colour ranges in case they have changed
	}
	else
		GetAerialBitmap(-1);

	// Restart the scan
	aerial_command_ = NONE;  // make sure we don't stop the scan before it starts
	aerial_fin_ = false;
	docdata_.Unlock();

	TRACE("+++ Pulsing aerial event (restart)\n");
	start_aerial_event_.SetEvent();
}

int CHexEditDoc::AerialProgress()
{
	if (length_ == 0)
	{
		//ASSERT(0); xxx track down how we get to here when closing file
		return 100;     // avoid divide by zero
	}

	CSingleLock sl(&docdata_, TRUE);
	if (aerial_state_ != SCANNING) return -1;

	return 1 + int((aerial_addr_ * 99)/length_);
}

// Gets a new FreeImage bitmap if we haven't got one yet or the current one is too small.
// On entry the background thread must not be scanning the bitmap - eg waiting.
// The parameter 'clear' is the value to clear the bitmap to - effectively clears to
// a grey of colour RGB(clear, clear, clear), or -1 to not clear.
void CHexEditDoc::GetAerialBitmap(int clear /*= 0xC0*/)
{
	// If we already have a bitmap make sure it is big enough
	if (dib_ != NULL)
	{
		int dib_size = FreeImage_GetPitch(dib_) * FreeImage_GetHeight(dib_);
		int dib_rows = int(length_/bpe_/MAX_WIDTH) + 2;
		if (dib_size >= MAX_WIDTH*dib_rows*3)
		{
			if (clear > -1)
				memset(FreeImage_GetBits(dib_), clear, dib_size);
			return;
		}

		// Not big enough so free it and reallocate (below)
		FIBITMAP *dib = dib_;
		dib_ = NULL;
		TRACE("+++ FreeImage_Unload(%d)\n", dib);
		FreeImage_Unload(dib);
		if (clear <= -1)
			clear = 0xC0;  // if we get a new bitmap we must clear it
	}

	// TBD: TODO we need user options for default bpe and MAX_BMP (min 16Mb = 1 TByte file @ BPE 65536)
	bpe_ = 1;

	// Keep increasing bpe_ by powers of two until we get a small enough bitmap
	while (bpe_ <= 65536 && (length_*3)/bpe_ > theApp.aerial_max_)
		bpe_ = bpe_<<1;

	// Work out the number of bitmap rows we would need at the widest bitmap size
	int rows = int(length_/bpe_/MAX_WIDTH) + 2;    // Round up to next row plus add one more row to allow for "reshaping"
	ASSERT((rows-2)*MAX_WIDTH < theApp.aerial_max_);

	dib_ = FreeImage_Allocate(MAX_WIDTH, rows, 24);

	dib_size_ = MAX_WIDTH*rows*3;           // DIB size in bytes since we have 3 bytes per pixel and no pad bytes at the end of each scan line
	ASSERT(dib_size_ == FreeImage_GetPitch(dib_) * FreeImage_GetHeight(dib_));
	TRACE("+++ FreeImage_Allocate %d at %p end %p\n", dib_, FreeImage_GetBits(dib_), FreeImage_GetBits(dib_)+dib_size_);
	if (clear > -1)
		memset(FreeImage_GetBits(dib_), clear, dib_size_);       // Clear to a grey
}

// Sends a message for the thread to kill itself then tides up shared members 
void CHexEditDoc::KillAerialThread()
{
	ASSERT(pthread3_ != NULL);
	if (pthread3_ == NULL) return;

	HANDLE hh = pthread3_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
	TRACE1("+++ Killing aerial thread for %p\n", this);

	bool waiting, dying;
	docdata_.Lock();
	aerial_command_ = DIE;
	docdata_.Unlock();
	SetThreadPriority(pthread3_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = aerial_state_ == WAITING;
		dying   = aerial_state_ == DYING;
		docdata_.Unlock();
		if (waiting || dying)
			break;
		Sleep(1);
	}
	ASSERT(waiting || dying);

	timer tt(true);
	if (waiting)
		start_aerial_event_.SetEvent();

	pthread3_ = NULL;
	DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
	ASSERT(wait_status == WAIT_OBJECT_0 || wait_status == WAIT_FAILED);
	tt.stop();
	TRACE1("+++ Thread took %g secs to kill\n", double(tt.elapsed()));

	// Free resources that are only needed during scan
	if (pfile3_ != NULL)
	{
		pfile3_->Close();
		delete pfile3_;
		pfile3_ = NULL;
	}
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file3_[ii] != NULL)
		{
			data_file3_[ii]->Close();
			delete data_file3_[ii];
			data_file3_[ii] = NULL;
		}
	}
}

// bg_func is the entry point for the thread.
// It just calls the RunAerialThread member for the doc passed to it.
static UINT bg_func(LPVOID pParam)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

	TRACE1("+++ Aerial thread started for doc %p\n", pDoc);

	return pDoc->RunAerialThread();
}

// Sets up shared members and creates the thread using bg_func (above)
void CHexEditDoc::CreateAerialThread()
{
	ASSERT(pthread3_ == NULL);
	ASSERT(pfile3_ == NULL);

	// Open copy of file to be used by background thread
	if (pfile1_ != NULL)
	{
		if (IsDevice())
			pfile3_ = new CFileNC();
		else
			pfile3_ = new CFile64();
		if (!pfile3_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("+++ Aerial file open failed for %p\n", this);
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file3_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file3_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

	// Create new thread
	aerial_command_ = NONE;
	aerial_state_ = STARTING;    // pre start and very beginning
	aerial_fin_ = false;
	TRACE1("+++ Creating aerial thread for %p\n", this);
	pthread3_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
	ASSERT(pthread3_ != NULL);
}

// This is what does the work in the background thread
UINT CHexEditDoc::RunAerialThread()
{
	// Keep looping until we are told to die
	for (;;)
	{
		// Signal that we are waiting then wait for start_aerial_event_ to be pulsed
		{
			CSingleLock sl(&docdata_, TRUE);
			aerial_state_ = WAITING;
		}
		TRACE1("+++ BGAerial: waiting for %p\n", this);
		DWORD wait_status = ::WaitForSingleObject(HANDLE(start_aerial_event_), INFINITE);
		docdata_.Lock();
		aerial_state_ = SCANNING;
		docdata_.Unlock();
		start_aerial_event_.ResetEvent();
		ASSERT(wait_status == WAIT_OBJECT_0);
		TRACE1("+++ BGAerial: got event for %p\n", this);

		if (AerialProcessStop())
			continue;

		// Reset for new scan
		docdata_.Lock();
		aerial_fin_ = false;
		aerial_addr_ = 0;
		FILE_ADDRESS file_len = length_;
		int file_bpe = bpe_;
		unsigned char *file_dib = FreeImage_GetBits(dib_);
		unsigned dib_size = FreeImage_GetDIBSize(dib_);
		docdata_.Unlock();
		TRACE("+++ BGAerial: using bitmap at %p\n", file_dib);

		// Get the file buffer
		size_t buf_len = (size_t)min(file_len, 65536);
		ASSERT(aerial_buf_ == NULL);
		aerial_buf_ = new unsigned char[buf_len];

		for (;;)
		{
			// First check if we need to stop
			if (AerialProcessStop())
				break;   // stop processing and go back to WAITING state

			// Check if we have finished scanning the file
			if (aerial_addr_ >= file_len)
			{
				TRACE2("+++ BGAerial: finished scan for %p at address %p\n", this, file_dib + 3*size_t(aerial_addr_/file_bpe));
				CSingleLock sl(&docdata_, TRUE); // Protect shared data access

				aerial_fin_ = true;
				break;                          // falls out to end_scan
			}

			// Get the next buffer full from the file and scan it
			size_t got = GetData(aerial_buf_, buf_len, aerial_addr_, 3);
			ASSERT(got <= buf_len);

			unsigned char *pbm = file_dib + 3*size_t(aerial_addr_/file_bpe);    // where we write to bitmap
			unsigned char *pbuf;                                        // where we read from the file buffer
			for (pbuf = aerial_buf_; pbuf < aerial_buf_ + got; pbuf += file_bpe, pbm += 3)
			{
				int r, g, b;
				r = g = b = 0;
				for (unsigned char *pp = pbuf; pp < pbuf + file_bpe; ++pp)
				{
					r += GetRValue(kala_[*pp]);
					g += GetGValue(kala_[*pp]);
					b += GetBValue(kala_[*pp]);
				}
				*pbm     = unsigned char(b/file_bpe);
				*(pbm+1) = unsigned char(g/file_bpe);
				*(pbm+2) = unsigned char(r/file_bpe);
			}
			aerial_addr_ += got;
		}

		delete[] aerial_buf_;
		aerial_buf_ = NULL;
	}
	return 0;   // never reached
}

// Check for a stop scanning (or kill) of the background thread
bool CHexEditDoc::AerialProcessStop()
{
	bool retval = false;

	CSingleLock sl(&docdata_, TRUE);
	switch(aerial_command_)
	{
	case STOP:                      // stop scan and wait
		TRACE1("+++ BGAerial: stop for %p\n", this);
		retval = true;
		break;
	case DIE:                       // terminate this thread
		TRACE1("+++ BGAerial: killed thread for %p\n", this);
		aerial_state_ = DYING;
		sl.Unlock();                // we need this here as AfxEndThread() never returns so d'tor is not called
		delete[] aerial_buf_;
		aerial_buf_ = NULL;
		AfxEndThread(1);            // kills thread (no return)
		break;                      // Avoid warning
	case NONE:                      // nothing needed here - just continue scanning
		break;
	default:                        // should not happen
		ASSERT(0);
	}

	// Prevent reprocessing of the same command
	aerial_command_ = NONE;
	return retval;
}
