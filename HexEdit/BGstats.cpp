// BGStats.cpp : statistics scan in background thread (part of CHexEditDoc)
//
// Copyright (c) 2016 by Andrew W. Phillips.
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include "HexEdit.h"
#include "HexEditDoc.h"
#include "md5.h"                    // For MD5 (not from Crypto++)
#include "include/Crypto++/cryptlib.h"
#include "include/Crypto++/sha.h"   // For SHA-1 and SHA-2 (SHA 256, SHA 512)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Returns true if global options says we can do background stats on this file
bool CHexEditDoc::CanDoStats()
{
	bool retval = false;
	if (theApp.bg_stats_)
	{
		retval = true;
		if (pfile1_ != NULL)
		{
			CString ss = pfile1_->GetFilePath().Left(3);
			if (IsDevice())
				retval = !theApp.bg_exclude_device_;
			else
				switch (::GetDriveType(ss))
				{
				case DRIVE_REMOVABLE:
					retval = !theApp.bg_exclude_removeable_;
					break;
				case DRIVE_REMOTE:
					retval = !theApp.bg_exclude_network_;
					break;
				case DRIVE_CDROM:
					retval = !theApp.bg_exclude_optical_;
					break;
				}
		}
	}
	return retval;
}

// Create/kill background thread as appropriate depending on current options
void CHexEditDoc::AlohaStats()
{
	if (pthread5_ == NULL && CanDoStats())  // no thread but we should have one
	{
		// create the stats thread
		CreateStatsThread();
		StartStats();
	}
	else if (pthread5_ != NULL && !CanDoStats())  // have thread but we shouldn't
	{
		KillStatsThread();
	}
}

// Get a vector of 256 values which are the counts of all bytes value in the file.
// It will return the largest byte count OR
//   -4 if background stats is off for this file
//   -2 if stats calcs are in progress
FILE_ADDRESS CHexEditDoc::GetByteCounts(std::vector<FILE_ADDRESS> & cnt)
{
	cnt.clear();

	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	cnt.reserve(256);   // this prevents us getting more memory than we need
	cnt.resize(256);    // for byte value 0 to 255

	FILE_ADDRESS retval = 0;
	for (int ii = 0; ii < 256; ++ii)
	{
		cnt[ii] = FILE_ADDRESS(count_[ii]);
		if (count_[ii] > retval)
			retval = count_[ii];
	}

	return retval;
}
int CHexEditDoc::GetCRC32(unsigned long &retval)
{
	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!theApp.bg_stats_crc32_)
		return -1;

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	retval = crc32_;
	return 0;
}

int CHexEditDoc::GetMd5(unsigned char buf[16])
{
	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!theApp.bg_stats_md5_)
		return -1;

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	memcpy(buf, md5_, sizeof(md5_));
	return 0;
}

int CHexEditDoc::GetSha1(unsigned char buf[20])
{
	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!theApp.bg_stats_sha1_)
		return -1;

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	memcpy(buf, sha1_, sizeof(sha1_));
	return 0;
}

int CHexEditDoc::GetSha256(unsigned char buf[32])
{
	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!theApp.bg_stats_sha256_)
		return -1;

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	memcpy(buf, sha256_, sizeof(sha256_));
	return 0;
}

int CHexEditDoc::GetSha512(unsigned char buf[64])
{
	if (!CanDoStats() || pthread5_ == NULL)
		return -4;         // stats not done on this file

	// Protect access to shared data
	CSingleLock sl(&docdata_, TRUE);

	if (!theApp.bg_stats_sha512_)
		return -1;

	if (!stats_fin_)
		return -2;         // stats calcs in progress

	memcpy(buf, sha512_, sizeof(sha512_));
	return 0;
}

// Doc has changed - signal current scan to stop then start new scan
void CHexEditDoc::StatsChange()
{
	if (pthread5_ == NULL) return;

	// Make sure the current (main) thread does not have docdata_ locked
	ASSERT(docdata_.m_sect.LockCount == -1 ||                                    // not locked OR
		   (DWORD)docdata_.m_sect.OwningThread != ::GetCurrentThreadId() ||      // locked by other thread OR
		   docdata_.m_sect.RecursionCount == 0);                                 // finished with it now

	// Wait for thread to stop if necessary
	bool waiting;
	docdata_.Lock();
	stats_command_ = STOP;
	docdata_.Unlock();
	SetThreadPriority(pthread5_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = stats_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ StatsChange - thread not waiting [lock=%d recurse=%d]\n", docdata_.m_sect.LockCount, docdata_.m_sect.RecursionCount);
		Sleep(1);
	}
	SetThreadPriority(pthread5_->m_hThread, THREAD_PRIORITY_LOWEST);
	//ASSERT(waiting);

	// Reset and restart the scan
	docdata_.Lock();
	stats_command_ = NONE;  // make sure we don't stop the scan before it starts
	stats_fin_ = false;
	stats_progress_ = 0;
	docdata_.Unlock();

	TRACE("+++ Pulsing stats event (restart) for %p\n", this);
	start_stats_event_.SetEvent();
}

// Return how far our scan has progressed as a percentage (0 to 100).
int CHexEditDoc::StatsProgress()
{
	CSingleLock sl(&docdata_, TRUE); // Protect shared data access
	return stats_progress_;
}

// Stops the current background stats scan (if any).  It does not return 
// until the scan is aborted and the thread is waiting again.
void CHexEditDoc::StopStats()
{
	if (pthread5_ == NULL) return;

	bool waiting;
	docdata_.Lock();
	stats_command_ = STOP;
	docdata_.Unlock();
	SetThreadPriority(pthread5_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = stats_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ StopStats - thread not waiting (yet)\n");
		Sleep(1);
	}
	SetThreadPriority(pthread5_->m_hThread, THREAD_PRIORITY_LOWEST);
	ASSERT(waiting);
}

// Start a new background scan.
void CHexEditDoc::StartStats()
{
	StopStats();

	// Setup up the info for the new scan
	docdata_.Lock();

	// Restart the scan
	stats_command_ = NONE;
	stats_fin_ = false;
	stats_progress_ = 0;
	docdata_.Unlock();

	TRACE("+++ Pulsing stats event for %p\n", this);
	start_stats_event_.SetEvent();
}

// Kill background task and wait until it is dead
void CHexEditDoc::KillStatsThread()
{
	ASSERT(pthread5_ != NULL);
	if (pthread5_ == NULL) return;

	HANDLE hh = pthread5_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
	TRACE("+++ Killing stats thread for %p\n", this);

	// Signal thread to kill itself
	docdata_.Lock();
	stats_command_ = DIE;
	docdata_.Unlock();

	SetThreadPriority(pthread5_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death
	bool waiting, dying;
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = stats_state_ == WAITING;
		dying   = stats_state_ == DYING;
		docdata_.Unlock();
		if (waiting || dying)
			break;
		Sleep(1);
	}
	ASSERT(waiting || dying);

	// Send start message if it is on hold
	if (waiting)
		start_stats_event_.SetEvent();

	pthread5_ = NULL;
	DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
	ASSERT(wait_status == WAIT_OBJECT_0 || wait_status == WAIT_FAILED);

	// Free resources that are only needed during bg scan
	if (pfile5_ != NULL)
	{
		pfile5_->Close();
		delete pfile5_;
		pfile5_ = NULL;
	}
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file5_[ii] != NULL)
		{
			data_file5_[ii]->Close();
			delete data_file5_[ii];
			data_file5_[ii] = NULL;
		}
	}
}

static UINT bg_func(LPVOID pParam)
{
	CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

	TRACE("+++ Stats thread started for doc %p\n", pDoc);

	return pDoc->RunStatsThread();
}

void CHexEditDoc::CreateStatsThread()
{
	ASSERT(CanDoStats());
	ASSERT(pthread5_ == NULL);
	ASSERT(pfile5_ == NULL);

	// Open copy of file to be used by background thread
	if (pfile1_ != NULL)
	{
		if (IsDevice())
			pfile5_ = new CFileNC();
		else
			pfile5_ = new CFile64();
		if (!pfile5_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE("+++ File5 open failed for %p\n", this);
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file5_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file5_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

	// Create new thread
	stats_command_ = NONE;
	stats_state_ = STARTING;
	stats_fin_ = false;
	stats_progress_ = 0;
	TRACE("+++ Creating stats thread for %p\n", this);
	pthread5_ = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_LOWEST);
	ASSERT(pthread5_ != NULL);
}

// This is the main loop for the worker thread
UINT CHexEditDoc::RunStatsThread()
{
	// Keep looping until we get the kill signal
	for (;;)
	{
		{
			CSingleLock sl(&docdata_, TRUE);
			stats_state_ = WAITING;
		}
		//TRACE("+++ BGstats: waiting [lock=%d recurse=%d]\n", docdata_.m_sect.LockCount, docdata_.m_sect.RecursionCount);
		DWORD wait_status = ::WaitForSingleObject(HANDLE(start_stats_event_), INFINITE);
		docdata_.Lock();
		stats_state_ = SCANNING;
		docdata_.Unlock();
		start_stats_event_.ResetEvent();      // Force ourselves to wait
		ASSERT(wait_status == WAIT_OBJECT_0);
		//TRACE("+++ BGstats: got event for %p\n", this);

		if (StatsProcessStop())
			continue;

		docdata_.Lock();
		stats_fin_ = false;
		stats_progress_ = 0;
		FILE_ADDRESS file_len = length_;
		BOOL do_crc32 = theApp.bg_stats_crc32_;
		BOOL do_md5   = theApp.bg_stats_md5_;
		BOOL do_sha1 = theApp.bg_stats_sha1_;
		BOOL do_sha256 = theApp.bg_stats_sha256_;
		BOOL do_sha512 = theApp.bg_stats_sha512_;
		docdata_.Unlock();

		FILE_ADDRESS addr = 0;
		void * hcrc32 = NULL;

		struct MD5Context md5_ctx;
		//sha1_context sha1_ctx;
		CryptoPP::SHA1 sha1;
		CryptoPP::SHA256 sha256;
		CryptoPP::SHA512 sha512;

		const size_t buf_size = 16384;
		ASSERT(stats_buf_ == NULL && c32_ == NULL && c64_ == NULL);
		stats_buf_ = new unsigned char[buf_size];

		if (file_len < LONG_MAX)
		{
			c32_ = new long[256];   // for small files only do 32-bit counts (faster)
			memset(c32_, '\0', 256*sizeof(*c32_));
		}
		else
		{
			c64_ = new __int64[256];
			memset(c64_, '\0', 256*sizeof(*c64_));
		}

		// Init CRC
		if (do_crc32)
			hcrc32 = crc_32_init();

		// Init digests (MD5, SHA1, etc)
		if (do_md5)
			MD5Init(&md5_ctx);
		// For Crypto++ digests (SHA256 etc) we don't need to init anything (ctor/Final/Restart will init. for us).

		// Scan all the data blocks of the file
		for (;;)
		{
			if (StatsProcessStop())
			{
				// Reset the calcs if we don't call Final()
				if (do_sha1)
					sha1.Restart();
				if (do_sha256)
					sha256.Restart();
				if (do_sha512)
					sha512.Restart();
				break;
			}

			size_t got;

			if ((got = GetData(stats_buf_, buf_size, addr, 5)) <= 0)
			{
				// We reached the end of the file at last - save results and go back to wait state
				CSingleLock sl(&docdata_, TRUE); // Protect shared data access

				if (c32_ != NULL)
				{
					for (int ii = 0; ii < 256; ++ii)
						count_[ii] = c32_[ii];
				}
				else
				{
					ASSERT(c64_ != NULL);
					for (int ii = 0; ii < 256; ++ii)
						count_[ii] = c64_[ii];
				}

				// Get results of any digests to be calculated
				if (do_crc32)
					crc32_ = crc_32_final(hcrc32);
				if (do_md5)
					MD5Final(md5_, &md5_ctx);
				if (do_sha1)
					sha1.Final(sha1_);
				if (do_sha256)
					sha256.Final(sha256_);
				if (do_sha512)
					sha512.Final(sha512_);
#ifdef _DEBUG
				__int64 total_count = 0;
				for (int ii = 0; ii < 256; ++ii)
					total_count += count_[ii];
				TRACE("+++ BGStats: finished scan for %p +++++++++++ TOTAL = %d\n", this, int(total_count));
#endif
				stats_fin_ = true;
				stats_progress_ = 100;
				break;
			}

			// Do count of different bytes
			if (c32_ != NULL)
			{
				for (size_t ii = 0; ii < got; ++ii)
				{
					++c32_[stats_buf_[ii]];
				}
			}
			else
			{
				ASSERT(c64_ != NULL);
				for (size_t ii = 0; ii < got; ++ii)
				{
					++c64_[stats_buf_[ii]];
				}
			}

			// Do CRC
			if (do_crc32)
				crc_32_update(hcrc32, stats_buf_, got);

			// Update any digests (MD5, SHA1, etc) being calculated
			if (do_md5)
				MD5Update(&md5_ctx, stats_buf_, got);
			if (do_sha1)
				sha1.Update(stats_buf_, got);
			if (do_sha256)
				sha256.Update(stats_buf_, got);
			if (do_sha512)
				sha512.Update(stats_buf_, got);

			addr += got;
			{
				CSingleLock sl(&docdata_, TRUE); // Protect shared data access
				stats_progress_ = int((addr * 100)/file_len);
			}
		} // for

		if (c32_ != NULL) (delete[] c32_), c32_ = NULL;
		if (c64_ != NULL) (delete[] c64_), c64_ = NULL;

		delete[] stats_buf_;
		stats_buf_ = NULL;
	}
	return 0;  // never reached
}

bool CHexEditDoc::StatsProcessStop()
{
	bool retval = false;

	CSingleLock sl(&docdata_, TRUE);
	switch (stats_command_)
	{
	case STOP:                      // stop scan and wait
		//TRACE("+++ BGstats: stop for %p\n", this);
		retval = true;
		break;
	case DIE:                       // terminate this thread
		//TRACE("+++ BGstats: killed thread for %p\n", this);
		stats_state_ = DYING;
		sl.Unlock();                // we need this here as AfxEndThread() never returns so d'tor is not called
		delete[] stats_buf_;
		stats_buf_ = NULL;
		if (c32_ != NULL) (delete[] c32_), c32_ = NULL;
		if (c64_ != NULL) (delete[] c64_), c64_ = NULL;
		AfxEndThread(1);            // kills thread (no return)
		break;                      // Avoid warning
	case NONE:                      // nothing needed here - just continue scanning
		break;
	default:                        // should not happen
		ASSERT(0);
	}

	stats_command_ = NONE;
	return retval;
}
