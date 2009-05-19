// BGAerial.cpp : background scan of file to create aerial view "bitmap"
//
// Copyright (c) 2008 by Andrew W. Phillips.
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
// hence the colours assigned in the FreeImage bitmap.  Note that this is a little
// inconsistent if there is more than one window and they are using different colour
// schemes.  The colour scheme used in the first opened one is used.

void CHexEditDoc::AddAerialView(CHexEditView *pview)
{
    if (++av_count_ == 1)
    {
        // xxx we need user options for default bpe and MAX_BMP (min MAX_BMP should be 16MB)
        bpe_ = 1; 
        
        // Keep increasing bpe_ by powers of two until we get a small enough bitmap
        while (bpe_ <= 65536 && (length_*3)/bpe_ > MAX_BMP)
            bpe_ = bpe_<<1;

        // Work out the number of bitmap rows we would need at the widest bitmap size
        int rows = int(length_/bpe_/MAX_WIDTH) + 2;    // Round up to next row plus add one more row to allow for "reshaping"
        ASSERT((rows-2)*MAX_WIDTH < MAX_BMP);

        dib_ = FreeImage_Allocate(MAX_WIDTH, rows, 24);
        dib_size_ = MAX_WIDTH*rows*3;           // DIB size in bytes since we have 3 bytes per pixel and no pad bytes at the end of each scan line
        ASSERT(dib_size_ == FreeImage_GetPitch(dib_) * FreeImage_GetHeight(dib_));
        memset(FreeImage_GetBits(dib_), 0xC0, dib_size_);       // Clear to light grey
#ifdef _DEBUG
        // Fill the bitmap with a known pattern for testing of scrolling etc
        BYTE *pp = FreeImage_GetBits(dib_);
        for (int f4 = 0; ; ++f4)
        {
            for (int f3 = 0; f3 < 32; ++f3)
            {
                for (int f2 = 0; f2 < 32; ++f2)
                {
                    for (int f1 = 0; f1 < 32; ++f1)
                    {
                        if ((((f4*32 + f3)*32 + f2)*32 + f1)*32 > NumElts())
                            goto stop_now;
                        *(pp + ((((f4*32) + f3)*32 + f2)*32 + f1)*32*3) = (BYTE)255;
                        *(pp + ((((f4*32) + f3)*32 + f2)*32 + f1)*32*3 + 1) = (BYTE)0;
                        *(pp + ((((f4*32) + f3)*32 + f2)*32 + f1)*32*3 + 2) = (BYTE)0;
                    }
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3) = (BYTE)0;
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3 + 1) = (BYTE)255;
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3 + 2) = (BYTE)0;
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3 + 3) = (BYTE)0;
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3 + 4) = (BYTE)255;
                    *(pp + ((f4*32 + f3)*32 + f2)*32*32*3 + 5) = (BYTE)0;
                }
                *(pp + (f4*32 + f3)*32*32*32*3) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 1) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 2) = (BYTE)255;
                *(pp + (f4*32 + f3)*32*32*32*3 + 3) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 4) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 5) = (BYTE)255;
                *(pp + (f4*32 + f3)*32*32*32*3 + 6) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 7) = (BYTE)0;
                *(pp + (f4*32 + f3)*32*32*32*3 + 8) = (BYTE)255;
            }
            memset(pp + f4*32*32*32*32*3, 255, 30);  // 10 white pixels
        }
    stop_now:
        memset(FreeImage_GetBits(dib_), 0, 6);                    // 2 black pixels at the start
        memset(FreeImage_GetBits(dib_) + NumElts()*3 - 6, 0, 6);  // 2 black pixels at the end
#endif
    }
}

void CHexEditDoc::RemoveAerialView()
{
    if (--av_count_ == 0)
    {
        FIBITMAP *dib = dib_;
        dib_ = NULL;
        FreeImage_Unload(dib);
    }
}

static UINT bg_func(LPVOID pParam)
{
    CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

    TRACE1("Aerial thread started for doc %p\n", pDoc);

    return pDoc->RunAerialThread();
}

void CHexEditDoc::CreateAerialThread()
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
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
			TRACE1("Aerial file open failed for %p\n", this);
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file3_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file2_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

    // Create new thread
    TRACE1("Creating aerial thread for %p\n", this);
    pthread3_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
    ASSERT(pthread3_ != NULL);
}

void CHexEditDoc::KillAerialThread()
{
    ASSERT(pthread3_ != NULL);
    if (pthread3_ == NULL) return;

    HANDLE hh = pthread3_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
    TRACE1("Killing aerial thread for %p\n", this);

    // Signal thread to kill itself
    docdata_.Lock();
    // xxx
    docdata_.Unlock();

    SetThreadPriority(pthread3_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death

    // Send start message in case it is on hold
    // xxx

    DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
    ASSERT(wait_status == WAIT_OBJECT_0);
    pthread3_ = NULL;

    // Free resources that are only needed during bg searches
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
	
	// xxx reset any other things used by the thread
}

// This is what does the work in the background thread
UINT CHexEditDoc::RunAerialThread()
{
    return 1; // xxx 
}
