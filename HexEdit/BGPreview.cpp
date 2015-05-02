// BGPreview.cpp : implements preview of graphics files from memory - rendered in a background thread (part of CHexEditDoc)

#include "stdafx.h"
#include "HexEdit.h"

#include "HexEditDoc.h"
#include "HexEditView.h"
#include <FreeImage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// TODO xxx
//  test with a 2GByte file

void CHexEditDoc::AddPreviewView(CHexEditView *pview)
{
	TRACE("+++ preview +++ %d\n", preview_count_);
	if (++preview_count_ == 1)
	{
		// Create the background thread and start it scanning
		CreatePreviewThread();
		TRACE("+++ Pulsing preview event\n");
		start_preview_event_.SetEvent();
	}
	ASSERT(pthread6_ != NULL);
}

void CHexEditDoc::RemovePreviewView()
{
	if (--preview_count_ == 0)
	{
		if (pthread6_ != NULL)
			KillPreviewThread();
		FIBITMAP *dib = preview_dib_;
		preview_dib_ = NULL;
		TRACE("+++  preview: FreeImage_Unload(%d)\n", dib);
		if (dib != NULL)
			FreeImage_Unload(dib);
	}
	TRACE("+++  Preview --- %d\n", preview_count_);
}

// Doc or colours have changed - signal to redisplay
void CHexEditDoc::PreviewChange(CHexEditView *pview /*= NULL*/)
{
	if (preview_count_ == 0) return;
	ASSERT(pthread6_ != NULL);
	if (pthread6_ == NULL) return;

	// Wait for thread to stop if necessary
	bool waiting;
	docdata_.Lock();
	preview_command_ = STOP;
	docdata_.Unlock();
	SetThreadPriority(pthread6_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = preview_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ PreviewChange - thread not waiting (yet)\n");
		Sleep(1);
	}
	SetThreadPriority(pthread6_->m_hThread, THREAD_PRIORITY_LOWEST);
	ASSERT(waiting);

	// Restart the scan
	docdata_.Lock();
	preview_command_ = NONE;  // make sure we don't stop the scan before it starts
	preview_fin_ = false;
	docdata_.Unlock();

	TRACE("+++ Pulsing preview event (restart)\n");
	start_preview_event_.SetEvent();
}

int CHexEditDoc::PreviewProgress()
{
	return -1;
}

// Sends a message for the thread to kill itself then tides up shared members 
void CHexEditDoc::KillPreviewThread()
{
	ASSERT(pthread6_ != NULL);
	if (pthread6_ == NULL) return;

	HANDLE hh = pthread6_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
	TRACE1("+++ Killing preview thread for %p\n", this);

	bool waiting, dying;
	docdata_.Lock();
	preview_command_ = DIE;
	docdata_.Unlock();
	SetThreadPriority(pthread6_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = preview_state_ == WAITING;
		dying   = preview_state_ == DYING;
		docdata_.Unlock();
		if (waiting || dying)
			break;
		Sleep(1);
	}
	ASSERT(waiting || dying);

	timer tt(true);
	if (waiting)
		start_preview_event_.SetEvent();

	pthread6_ = NULL;
	DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
	ASSERT(wait_status == WAIT_OBJECT_0 || wait_status == WAIT_FAILED);
	tt.stop();
	TRACE1("+++ Thread took %g secs to kill\n", double(tt.elapsed()));

	// Free resources that are only needed during scan
	if (pfile6_ != NULL)
	{
		pfile6_->Close();
		delete pfile6_;
		pfile6_ = NULL;
	}
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file6_[ii] != NULL)
		{
			data_file6_[ii]->Close();
			delete data_file6_[ii];
			data_file6_[ii] = NULL;
		}
	}
}

// bg_func is the entry point for the thread.
// It just calls the RunPreviewThread member for the doc passed to it.
static UINT bg_func(LPVOID pParam)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

	TRACE1("+++ Preview thread started for doc %p\n", pDoc);

	return pDoc->RunPreviewThread();
}

// Sets up shared members and creates the thread using bg_func (above)
void CHexEditDoc::CreatePreviewThread()
{
	ASSERT(pthread6_ == NULL);
	ASSERT(pfile6_ == NULL);

	// Open copy of file to be used by background thread
	if (pfile1_ != NULL)
	{
		if (IsDevice())
			pfile6_ = new CFileNC();
		else
			pfile6_ = new CFile64();
		if (!pfile6_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("+++ Preview file open failed for %p\n", this);
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file6_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file6_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

	// Create new thread
	preview_command_ = NONE;
	preview_state_ = STARTING;    // pre start and very beginning
	preview_fin_ = false;
	TRACE1("+++ Creating preview thread for %p\n", this);
	pthread6_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
	ASSERT(pthread6_ != NULL);
}

//---------------------------------------------------------------------------------------
// These 4 funcs (fi_read(), etc) are used with FreeImage for in-memory processing of bitmap data.
// Pointers to these funcs are added to a FreeImageIO struct (see fi_funcs below) and passed
// to FreeImage functions like FreeImage_LoadFromHandle() or FreeImage_GetFileTypeFromHandle().

// fi_read() should behave like fread (returns count if all items were read OK, returns 0 on eof/error)
unsigned __stdcall CHexEditDoc::fi_read(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;

	if (pDoc->PreviewProcessStop())
		return 0;       // indicate that processing should stop

	size_t got = pDoc->GetData((unsigned char *)buffer, size*count, pDoc->preview_address_);
	pDoc->preview_address_ += got;
	return got/size;
}

unsigned __stdcall CHexEditDoc::fi_write(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	ASSERT(0);   // writing should not happen - we are only reading to display the bitmap
	return 0;
}

int __stdcall CHexEditDoc::fi_seek(fi_handle handle, long offset, int origin)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;
	FILE_ADDRESS addr = 0;
	switch (origin)
	{
	case SEEK_SET:
		addr = offset;
		break;
	case SEEK_CUR:
		addr = pDoc->preview_address_ + offset;
		break;
	case SEEK_END:
		addr = pDoc->length_ + offset;
		break;
	}
	if (addr > LONG_MAX)
		return -1L;        // seek failed

	pDoc->preview_address_ = (long)addr;
	return 0L;             // seek succeeded
}

long __stdcall CHexEditDoc::fi_tell(fi_handle handle)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)handle;

	return pDoc->preview_address_;
}

FreeImageIO fi_funcs = { &CHexEditDoc::fi_read, &CHexEditDoc::fi_write, &CHexEditDoc::fi_seek, &CHexEditDoc::fi_tell };

// This is what does the work in the background thread
UINT CHexEditDoc::RunPreviewThread()
{
	// Keep looping until we are told to die
	for (;;)
	{
		// Signal that we are waiting then wait for start_preview_event_ to be pulsed
		{
			CSingleLock sl(&docdata_, TRUE);
			preview_state_ = WAITING;
		}
		TRACE1("+++ BGPreview: waiting for %p\n", this);
		DWORD wait_status = ::WaitForSingleObject(HANDLE(start_preview_event_), INFINITE);
		docdata_.Lock();
		preview_state_ = SCANNING;
		docdata_.Unlock();
		start_preview_event_.ResetEvent();
		ASSERT(wait_status == WAIT_OBJECT_0);
		TRACE1("+++ BGPreview: got event for %p\n", this);

		if (PreviewProcessStop())
			continue;

		// Reset for new scan
		docdata_.Lock();
		preview_fin_ = false;
		preview_address_ = 0;
		if (preview_dib_ != NULL)
		{
			FreeImage_Unload(preview_dib_);
			preview_dib_ = NULL;
		}
		docdata_.Unlock();

		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(&fi_funcs, this);
		TRACE("Image format is %d\n", fif);
		FIBITMAP * dib = FreeImage_LoadFromHandle(fif, &fi_funcs, this);

		TRACE1("+++ BGPreview: finished load for %p\n", this);
		docdata_.Lock();
		preview_fin_ = true;
		preview_address_ = 0;
		preview_dib_ = dib;
		docdata_.Unlock();
	}
	return 0;   // never reached
}

// Check for a stop scanning (or kill) of the background thread
bool CHexEditDoc::PreviewProcessStop()
{
	bool retval = false;

	CSingleLock sl(&docdata_, TRUE);
	switch(preview_command_)
	{
	case STOP:                      // stop scan and wait
		TRACE1("+++ BGPreview: stop for %p\n", this);
		retval = true;
		break;
	case DIE:                       // terminate this thread
		TRACE1("+++ BGPreview: killed thread for %p\n", this);
		preview_state_ = DYING;
		sl.Unlock();                // we need this here as AfxEndThread() never returns so d'tor is not called
		AfxEndThread(1);            // kills thread (no return)
		break;                      // Avoid warning
	case NONE:                      // nothing needed here - just continue scanning
		break;
	default:                        // should not happen
		ASSERT(0);
	}

	// Prevent reprocessing of the same command
	preview_command_ = NONE;
	return retval;
}

