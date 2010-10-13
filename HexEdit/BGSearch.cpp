// BGSearch.cpp : implements background searches (part of CHexEditDoc)
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

/*

Description of Background Searches
==================================

Document
--------

The document (CHexEditDoc) is where background searching is controlled.
A background thread is created for each open document (if bg searching is on).
The document with the active view has a thread with priority LOWEST.
Other documents have a thread with priority IDLE.

Several document data members are used by the background thread:

start_search_event_: signal from main thread to background thread to start searching
search_state_: what the thread is currently doing
  STARTING: thread has just started and not yet blocked at start_search_event_
  WAITING: thread is blocked at start_search_event_
  SCANNING: thread is doing what it is supposed to
  DYING: thread received message to kill itself and is doing so
search_command_: what we want the thread to do now
search_fin_: indicates that background search was finished succcesfully (not running or aborted)
docdata_: is a critical section used to protect access to the other shared data members below

pfile2_: is a ptr to file open the same as pfile1_.  Using a separate file allows the main thread
         to read from pfile1_ without having to lock docdata_.  Locking is only required
         when the background thread accesses the file or the main thread changes it.
         This means that file display should never be slowed by the background thread.
loc_: accessed (via GetData) in both threads to get data from the file
undo_: loc_ uses data stored in undo array

to_search_: list of areas of the file to search
found_: addresses where occurences were found
main_thread_id: used by background thread to signal the main thread (PostThreadMessage)

to_adjust_: list of adjustments to be done by bg thread when a file change is made
            allows bg thread to fix found_ to allow for file changes and the bg thread
            to fix its internal addresses to allow for insertions/deletions
    
Background thread
-----------------

The thread (see bg_func) waits for a signal to start searching (start_search_event_).
It then gets the next area to search from the list to_search_.  While searching
it adds any occurrences found to the set found_.

When to_search_ becomes empty it sets search_fin_ and goes back to the wait state.
When the doc sees that search_fin_ is true it updates all its views to show the new
occurrences and sets search_fin_ to false, so it doesn't do it again.

While searching the bg search also continually checks search_command_ to see
if the current search has been cancelled (eg, if a new search string has been
entered) or if the thread has been killed (eg, if background searches are
turned off).

When the search is finished or after it is cancelled the bg thread signals
that it is ready to start a new search by setting search_state_ to WAITING.

Application
-----------

The current search is system wide so that when a new background search is started
all documents are informed of the change. The application stores info on the
current search in the following members:

appdata_: is a critical section used to protect access to the application data below
pboyer_: determines the bytes to be searched for
icase_: determines if the search is case-insensitive
text_type_: characters set used (ASCII, EBCDIC, Unicode) for case-insensitive searches
wholeword_, alignment_, offset_, align_rel_, base_addr_: other search options

The app also has a few routines for background searches
StartSearches: starts background searches in the other (inactive) documents
NewSearch: changes pboyer_, icase_, text_type_

Searches (See CMainFrame::search_forw and CMainFrame::search_back)
--------

When a search is requested it is first checked if this is a new search or
a repeat of the last search.  If it is a new search any background search
is cancelled by setting search_command_ to STOP, then the search is performed 
as normal in the main thread, and the new background search is started after this
search is finished.  (We also wait on the background thread to signal that it
has aborted the current background search and is waiting to start a new search.)

When the background search is started some of the current file has already
been searched by the main thread.  The to_search_ list allows for this by
having only the area(s) not yet searched added to it.  If the first (main
thread) search has found something (ie. was not aborted or hit EOF) then
the to_search_ list is extended by one byte so that this occurrence is also
detected by the bg thread.

Other files currently open are also searched (at a lower priority) but in this
case the whole file is searched.

If the search is a repeat of a previous search then if bg searching is finished 
found_ is checked to quickly get the required address.  If the background
search is still in progress then a foreground search is performed as normal,
and the background search is left running.  This could possibly be optimised
to check the list of areas left to search and compare it with where the 
search is to take place.

Changes (CHexEditDoc::Change, CHexEditDoc::Undo in DocData.cpp)
-------

When a document is changed the changed area has to be updated in all views
in case some search occurrences have gone or been created.  Also if there
are deletions/insertions the displayed occurrences move within the views.

Any bg search currently in progress has to be informed of any address changes
due to insertions or deletions (see to_adjust_).  Also the changed area is added
to to_search_ so that any new occurrences are found.

In the bg thread whenever the document data is accessed the to_adjust_ list
of address adjustments is checked and internal variables adjusted first.
    
Views (see CBGSearchHint used by CHexEditView::OnUpdate)
-----

An advantage of bg searches is that all occurrences of a string can be
highlighted (eg. with a yellow background) once the background search has
completed.  When a search for a different string of bytes is started then
all views are signalled to remove any highlighting.  When a background
search has finished for one document then all its views are told to update
themselves using the new addresses in the document's found_ member.

Each view keeps a vector of areas where bg search occurrences were found
(in its current display rectangle) called search_pair_.  This is only updated
when the current display position is moved (via ValidateScroll) or when 
the current search string changes.  This saves recalc in every OnDraw.

*/

#include "stdafx.h"
#include "HexEdit.h"

#include "HexEditDoc.h"
#include "boyer.h"
#include "SystemSound.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Returns the number of search string occurrences found OR
// -4 = background searches are disabled
// -2 = bg search still in progress
int CHexEditDoc::SearchOccurrences()
{
    if (pthread2_ == NULL)
    {
        ASSERT(!theApp.bg_search_);
        return -4;
    }

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

    if (!to_search_.empty())
    {
        // background search still in progress
        return -2;
    }

    return found_.size();
}

// Return how far our background search has progressed as a percentage (0 to 100).
// Also return the number of occurrences found so far in the parameter 'occurrences'.
int CHexEditDoc::SearchProgress(int &occurrences)
{
    docdata_.Lock();
    FILE_ADDRESS file_len = length_;
    docdata_.Unlock();

    ASSERT(pthread2_ != NULL && theApp.bg_search_);

    FILE_ADDRESS curr;

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

	if (to_search_.empty())
		return 100;         // nothing left to search

    // First save the number of occurrences found so far (returned)
    occurrences = found_.size();

    FILE_ADDRESS start, end;
    start = to_search_.front().first;
    end = to_search_.front().second;
    if (start < 0) start = 0;
    if (end < 0) end = file_len;

    // Work out how much is in the entry currently being searched
    curr = end - start;

    FILE_ADDRESS total_left = 0;

    // Work out how much we have to search (including already searched part of top entry of to_search_ list)
    std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp;

    for (pp = to_search_.begin(); pp != to_search_.end(); ++pp)
    {
        start = pp->first;
        end = pp->second;
        if (start < 0) start = 0;
        if (end < 0) end = file_len;
        total_left += end - start;
    }
//    ASSERT(find_total_ >= total_left); // xxx why does this assert fail????

    if (find_total_ < 1024)
        return 100;    // How long could it take to search this little bit?
    else
        return int(((find_total_ - total_left + curr * find_done_)/find_total_)*100.0);
}

// Asks for the next bg search found address.  The first 4 parameters
// (pat, len, icase, tt) determine what is being searched for.  The address
// 'from' is where to start the search from.  There are several return values:
// -4 = background searches are disabled
// -3 = search is different to last/current bg search
// -2 = bg search still in progress
// -1 = bg search finished but there are no occurrences of the search bytes before eof
// otherwise the address of the next occurrence is returned.
FILE_ADDRESS CHexEditDoc::GetNextFound(const unsigned char *pat, const unsigned char *mask, size_t len,
                                       BOOL icase, int tt, BOOL wholeword,
									   int alignment, int offset, bool align_rel, FILE_ADDRESS base_addr,
                                       FILE_ADDRESS from)
{
    if (pthread2_ == NULL)
    {
        ASSERT(!theApp.bg_search_);
        return -4;
    }

    {
        CSingleLock s2(&theApp.appdata_, TRUE);

        if (theApp.pboyer_ == NULL ||
            icase != theApp.icase_ ||
            tt != theApp.text_type_ ||
            wholeword != theApp.wholeword_ ||
            alignment != theApp.alignment_ ||
			offset != theApp.offset_ ||
			align_rel != theApp.align_rel_ ||
			(theApp.align_rel_ && base_addr != base_addr_))
        {
            // Search params are different to last bg search (or no bg search done yet)
            return -3;
        }

        const unsigned char *curr_mask = theApp.pboyer_->mask();
        if (len != theApp.pboyer_->length() ||
            ::memcmp(pat, theApp.pboyer_->pattern(), len) != 0 ||
            !(mask==NULL && curr_mask==NULL || mask!=NULL && curr_mask!=NULL && ::memcmp(mask, curr_mask, len)==0) )
        {
            // Search text (or mask) is different to last bg search
            return -3;
        }
    }

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

    if (!to_search_.empty())
    {
        // background search still in progress
        return -2;
    }

    // Find the first address greater or equal to from in found_
    std::set<FILE_ADDRESS>::const_iterator pp = found_.lower_bound(from);

    if (pp == found_.end())
        return -1;                      // None found
    else
        return *pp;                     // Return the address found
}

// Same as GetNextFound but finds the previous occurrence if any
FILE_ADDRESS CHexEditDoc::GetPrevFound(const unsigned char *pat, const unsigned char *mask, size_t len,
                                       BOOL icase, int tt, BOOL wholeword,
									   int alignment, int offset, bool align_rel, FILE_ADDRESS base_addr,
                                       FILE_ADDRESS from)
{
    if (pthread2_ == NULL)
    {
        ASSERT(!theApp.bg_search_);
        return -4;
    }

    {
        CSingleLock s2(&theApp.appdata_, TRUE);

        if (theApp.pboyer_ == NULL ||
            icase != theApp.icase_ ||
            tt != theApp.text_type_ ||
            wholeword != theApp.wholeword_ ||
            alignment != theApp.alignment_ ||
			offset != theApp.offset_ ||
			align_rel != theApp.align_rel_ ||
			(theApp.align_rel_ && base_addr != base_addr_))
        {
            // Search params are different to last bg search (or no bg search done yet)
            return -3;
        }

        const unsigned char *curr_mask = theApp.pboyer_->mask();
        if (len != theApp.pboyer_->length() ||
            ::memcmp(pat, theApp.pboyer_->pattern(), len) != 0 ||
            !(mask==NULL && curr_mask==NULL || mask!=NULL && curr_mask!=NULL && ::memcmp(mask, curr_mask, len) == 0) )
        {
            // Search text (or mask) is different to last bg search
            return -3;
        }
    }

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

    if (!to_search_.empty())
    {
        // background search still in progress
        return -2;
    }

    // Find the first address greater or equal to form in found_
    std::set<FILE_ADDRESS>::const_iterator pp = found_.upper_bound(from);

    if (pp == found_.begin())
        return -1;                      // None found
    else
        return *(--pp);                 // Return the address
}

// Get all the found search addresses in a range
std::vector<FILE_ADDRESS> CHexEditDoc::SearchAddresses(FILE_ADDRESS start, 
                                                       FILE_ADDRESS end)
{
    std::vector<FILE_ADDRESS> retval;

    // Protect access to shared data
    CSingleLock sl(&docdata_, TRUE);

    // Return nothing until background searching has finished
    if (!to_search_.empty())
        return retval;

#if 0 // this won't compile - needs inserter?
    retval.insert(retval.end(),
                  found_.lower_bound(start),
                  found_.lower_bound(end));
#else
    std::set<FILE_ADDRESS>::const_iterator pp = found_.lower_bound(start);
    std::set<FILE_ADDRESS>::const_iterator pend = found_.lower_bound(end);
    while (pp != pend)
    {
        retval.push_back(*pp);
        ++pp;
    }
#endif
    return retval;
}

void CHexEditDoc::FixFound(FILE_ADDRESS start, FILE_ADDRESS end,
                           FILE_ADDRESS address, FILE_ADDRESS adjust)
{
    if (theApp.wholeword_)
    {
        start--;
        ++end;
    }
    if (start < 0) start = 0;

    // Erase any found occurrences that are no longer valid
    found_.erase(found_.lower_bound(start), found_.lower_bound(end));

    if (adjust == 0)
        return;

#if 0  // Should not modify a set through iterator
    if (adjust < 0)
    {
        // Adjust already found addresses to account for insertion/deletion
        std::set<FILE_ADDRESS>::iterator paddr, paddr_end;
        for (paddr = found_.begin(), paddr_end = found_.end(); paddr != paddr_end; ++paddr)
        {
            if (*paddr >= address)
            {
                ASSERT(*paddr + adjust >= address); // Anything in this range should have been deleted by erase above
                *paddr += adjust;
            }
        }
    }
    else
    {
        // Adjust already found addresses to account for insertion/deletion
        std::set<FILE_ADDRESS>::reverse_iterator paddr, paddr_end;
        for (paddr = found_.rbegin(), paddr_end = found_.rend(); paddr != paddr_end; ++paddr)
        {
            if (*paddr >= address)
            {
                ASSERT(*paddr + adjust >= address); // Anything in this range should have been deleted by erase above
                *paddr += adjust;
            }
        }
    }
#else
    std::set<FILE_ADDRESS> tmp;
    if (adjust < 0)
    {
        for (std::set<FILE_ADDRESS>::iterator paddr = found_.begin(); paddr != found_.end(); ++paddr)
        {
	        if (*paddr >= address)
			{
                ASSERT(*paddr + adjust >= address); // Anything in this range should have been deleted by erase above
	            tmp.insert(*paddr + adjust);
			}
	        else
	            tmp.insert(*paddr);
        }
    }
    else
    {
        for (std::set<FILE_ADDRESS>::reverse_iterator paddr = found_.rbegin(); paddr != found_.rend(); ++paddr)
        {
	        if (*paddr >= address)
			{
                ASSERT(*paddr + adjust >= address); // Anything in this range should have been deleted by erase above
	            tmp.insert(*paddr + adjust);
			}
	        else
	            tmp.insert(*paddr);
        }
    }
    found_.swap(tmp);  // Quick way to put tmp into found_ (tmp d'tor will destroy old found_)
#endif
}

// Stops the current background search (if any).  It does not return 
// until the search is aborted and the thread is waiting again.
void CHexEditDoc::StopSearch()
{
    ASSERT(pthread2_ != NULL);
    if (pthread2_ == NULL) return;

	bool waiting;
	docdata_.Lock();
	search_command_ = STOP;
	docdata_.Unlock();
    SetThreadPriority(pthread2_->m_hThread, THREAD_PRIORITY_NORMAL);
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = search_state_ == WAITING;
		docdata_.Unlock();
		if (waiting)
			break;
		TRACE("+++ StopSearch - thread not waiting (yet)\n");
		Sleep(1);
	}
    SetThreadPriority(pthread2_->m_hThread, search_priority_);
	ASSERT(waiting);
}

// Start a new background search.  The start and end parameters say what part of
// the file does not need to be searched (if any).
void CHexEditDoc::StartSearch(FILE_ADDRESS start /*=-1*/, FILE_ADDRESS end /*=-1*/)
{
	StopSearch();

    // -1 for end means EOF (unless both start and end are -1)
    if (start != -1 && end == -1) end = length_;

    // This is not done until we know the bg thread has stopped searching
    // else the bg thread may add more entries to found that apply to the old
    // search string which would be wrongly displayed.
    CBGSearchHint bgsh(FALSE);
    UpdateAllViews(NULL, 0, &bgsh);

	// Setup up the info for the new search
	docdata_.Lock();

    to_search_.clear();
    to_adjust_.clear();
    find_total_ = 0;
    find_done_ = 0.0;

    if (start == -1)
    {
        // Search whole file
        ASSERT(end == -1);
        to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(0,-1));
        find_total_ += length_;
        TRACE("+++ StartSearch: 0 to -1\n");
    }
    else
    {
        // Only search the part of the file not already searched
        ASSERT(start <= end);

        // Search end of file first
        if (end < length_)
        {
            to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(end, -1));
            find_total_ += length_ - end;
        }
        if (start > 0)
        {
            to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(0, start));
            find_total_ += start;
        }
    }

    found_.clear();                     // Clear set of found addresses

	// Restart the search
	search_command_ = NONE;
	search_fin_ = false;
	docdata_.Unlock();

	TRACE("+++ Pulsing search event for %p\n", this);
	start_search_event_.SetEvent();
}

void CHexEditDoc::SearchThreadPriority(int pri)
{
    ASSERT(pri == THREAD_PRIORITY_IDLE || pri == THREAD_PRIORITY_LOWEST || pri == THREAD_PRIORITY_BELOW_NORMAL);
    search_priority_ = pri;
    if (pthread2_ != NULL)
        SetThreadPriority(pthread2_->m_hThread, search_priority_);
}

// Kill background task and wait until it is dead
void CHexEditDoc::KillSearchThread()
{
    ASSERT(theApp.bg_search_);
    ASSERT(pthread2_ != NULL);
    if (pthread2_ == NULL) return;

    HANDLE hh = pthread2_->m_hThread;    // Save handle since it will be lost when thread is killed and object is destroyed
    TRACE1("+++ Killing search thread for %p\n", this);

    // Signal thread to kill itself
    docdata_.Lock();
	search_command_ = DIE;
    docdata_.Unlock();

    SetThreadPriority(pthread2_->m_hThread, THREAD_PRIORITY_NORMAL); // Make it a quick and painless death
	bool waiting, dying;
	for (int ii = 0; ii < 100; ++ii)
	{
		// Wait just a little bit in case the thread was just about to go into wait state
		docdata_.Lock();
		waiting = search_state_ == WAITING;
		dying   = search_state_ == DYING;
		docdata_.Unlock();
		if (waiting || dying)
			break;
		Sleep(1);
	}
	ASSERT(waiting || dying);

    // Send start message if it is on hold
	if (waiting)
		start_search_event_.SetEvent();

    pthread2_ = NULL;
    DWORD wait_status = ::WaitForSingleObject(hh, INFINITE);
    ASSERT(wait_status == WAIT_OBJECT_0 || wait_status == WAIT_FAILED);

    // Free resources that are only needed during bg searches
    if (pfile2_ != NULL)
	{
		pfile2_->Close();
		delete pfile2_;
		pfile2_ = NULL;
	}
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file2_[ii] != NULL)
		{
			data_file2_[ii]->Close();
			delete data_file2_[ii];
			data_file2_[ii] = NULL;
		}
	}

    found_.clear();
    to_search_.clear();
    find_total_ = 0;
}

static UINT bg_func(LPVOID pParam)
{
    CHexEditDoc *pDoc = (CHexEditDoc *)pParam;

    TRACE1("+++ Search thread started for doc %p\n", pDoc);

    return pDoc->RunSearchThread();
}

void CHexEditDoc::CreateSearchThread()
{
    ASSERT(theApp.bg_search_);
    ASSERT(pthread2_ == NULL);
    ASSERT(pfile2_ == NULL);

    // Open copy of file to be used by background thread
    if (pfile1_ != NULL)
	{
		if (IsDevice())
			pfile2_ = new CFileNC();
		else
			pfile2_ = new CFile64();
		if (!pfile2_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("+++ File2 open failed for %p\n", this);
			return;
		}
	}

	// Open copy of any data files in use too
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		ASSERT(data_file2_[ii] == NULL);
		if (data_file_[ii] != NULL)
			data_file2_[ii] = new CFile64(data_file_[ii]->GetFilePath(), 
										  CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
	}

    // Create new thread
    search_command_ = NONE;
	search_state_ = STARTING;
    search_fin_ = false;
    TRACE1("+++ Creating search thread for %p\n", this);
    search_priority_ = THREAD_PRIORITY_LOWEST;
    pthread2_ = AfxBeginThread(&bg_func, this, search_priority_);
    ASSERT(pthread2_ != NULL);
}

// This is the main loop for the worker thread
UINT CHexEditDoc::RunSearchThread()
{
    // Keep looping until we get the kill signal
    for (;;)
    {
        {
            CSingleLock sl(&docdata_, TRUE);
			search_state_ = WAITING;
        }
        TRACE1("+++ BGSearch: waiting %p\n", this);
        DWORD wait_status = ::WaitForSingleObject(HANDLE(start_search_event_), INFINITE);
        docdata_.Lock();
		search_state_ = SCANNING;
        docdata_.Unlock();
        start_search_event_.ResetEvent();      // Force ourselves to wait
        ASSERT(wait_status == WAIT_OBJECT_0);
        TRACE1("+++ BGSearch: got event for %p\n", this);

		if (SearchProcessStop())
			continue;

        size_t buf_len;
        int count = 0;

        theApp.appdata_.Lock();
        ASSERT(theApp.pboyer_ != NULL);
        boyer bb(*theApp.pboyer_);             // Take a copy of needed info
        BOOL ignorecase = theApp.icase_;
        int tt = theApp.text_type_;
        BOOL wholeword = theApp.wholeword_;
        int alignment = theApp.alignment_;
		int offset = theApp.offset_;
        theApp.appdata_.Unlock();

        docdata_.Lock();
		search_fin_ = false;
        FILE_ADDRESS file_len = length_;
		FILE_ADDRESS base_addr = base_addr_;
        docdata_.Unlock();

        ASSERT(bb.length() > 0);
        ASSERT(tt == 0 || tt == 1 || tt == 2 || tt == 3);

        if (bb.length() > file_len)
        {
            // Nothing can be found
            CSingleLock sl(&docdata_, TRUE);
            to_search_.clear();
            found_.clear();
            find_total_ = 0;
            search_fin_ = true;
            continue;
        }

        buf_len = (size_t)min(file_len, 32768 + bb.length() - 1);
		ASSERT(search_buf_ == NULL);
        search_buf_ = new unsigned char[buf_len + 1];

        // Search all to_search_ blocks
        for (;;)
        {
			if (SearchProcessStop())
				break;

            FILE_ADDRESS start, end;    // Current part of file to search

            // Get the next block to search
            {
                CSingleLock sl(&docdata_, TRUE); // Protect shared data access

                // Check if there has been a file insertion/deletion
                while (!to_adjust_.empty())
                {
                    FixFound(to_adjust_.front().start_, 
                             to_adjust_.front().end_,
                             to_adjust_.front().address_,
                             to_adjust_.front().adjust_);

                    // start, end should have already been adjusted at this point
                    to_adjust_.pop_front();
                }
                file_len = length_;

                // Find where we have to search
                if (to_search_.empty())
                {
                    find_total_ = 0;
                    search_fin_ = true;
					TRACE1("+++ BGSearch: finished search of %p\n", this);
                    break;
                }
                start = to_search_.front().first;
                if (start < 0) start = 0;
                end = to_search_.front().second;
                if (end < 0) end = file_len;
            }

            // We need to extend the search a little for wholeword searches since even though the pattern match
            // does not change the fact that a match is discarded due to the "alphabeticity" of characters at
            // either end changing when chars are inserted or deleted.
            if (wholeword)
            {
                if (start > 0) start--;
                if (tt == 2 && start > 0) start--;  // Go back 2 bytes for Unicode searches
                ++end;                  // No test needed here since "end" is adjusted below if past EOF
            }

            FILE_ADDRESS addr_buf = start;  // Current location in doc of start of search_buf_
            // Make sure we get extra bytes past end for length of search string
            end = min(end + bb.length() - 1, file_len);

            find_done_ = 0.0;               // We haven't searched any of this to_search_ block yet

            while (addr_buf + bb.length() <= end)
            {
                size_t got;
                bool alpha_before = false;
                bool alpha_after = false;

                // Get the next block
                {
                    CSingleLock sl(&docdata_, TRUE);   // For accessing file data

                    // Check if search cancelled or thread killed
                    if (search_command_ != NONE)
                        goto stop_search;

                    // Check for any file insertions/deletions
                    while (!to_adjust_.empty())
                    {
                        TRACE("+++ Adjusting already found\n");
                        FixFound(to_adjust_.front().start_, 
                                 to_adjust_.front().end_,
                                 to_adjust_.front().address_,
                                 to_adjust_.front().adjust_);
                        TRACE("+++ Finished adjusting\n");

                        if (start >= to_adjust_.front().address_)
                        {
                            start += to_adjust_.front().adjust_;
                            if (start < to_adjust_.front().address_)
                                start = to_adjust_.front().address_;
                        }
                        if (end >= to_adjust_.front().address_)
                        {
                            end += to_adjust_.front().adjust_;
                            if (end < to_adjust_.front().address_)
                                end = to_adjust_.front().address_;
                        }
                        if (addr_buf >= to_adjust_.front().address_)
                        {
                            addr_buf += to_adjust_.front().adjust_;
                            if (addr_buf < to_adjust_.front().address_)
                                addr_buf = to_adjust_.front().address_;
                        }
                        to_adjust_.pop_front();
                    }
                    file_len = length_;   // file length may have changed

                    // Get a buffer full (plus an extra char for wholeword test at end of buffer)
                    got = GetData(search_buf_, size_t(min(FILE_ADDRESS(buf_len), end - addr_buf)) + 1, addr_buf, 2);
                    ASSERT(got == min(buf_len, end - addr_buf) || got == min(buf_len, end - addr_buf) + 1);

                    if (wholeword)
                    {
                        // Work out whether the character before the buf is alphabetic
                        if (addr_buf > 0 && tt == 1)
                        {
                            // Check if alphabetic ASCII
                            unsigned char cc;
                            VERIFY(GetData(&cc, 1, addr_buf-1, 2) == 1);

                            alpha_before = isalnum(cc) != 0;
                        }
                        else if (addr_buf > 1 && tt == 2)
                        {
                            // Check if alphabetic Unicode
                            unsigned char cc[2];
                            VERIFY(GetData(cc, 2, addr_buf-2, 2) == 2);

                            alpha_before = isalnum(cc[0]) != 0;  // Check if low byte has ASCII alpha
                        }
                        else if (addr_buf > 0 && tt == 3)
                        {
                            // Check if alphabetic EBCDIC
                            unsigned char cc;
                            VERIFY(GetData(&cc, 1, addr_buf-1, 2) == 1);

                            alpha_before = isalnum(e2a_tab[cc]) != 0;
                        }

                        // If we read an extra character check if it is alphabetic
                        if (got == min(buf_len, end - addr_buf) + 1)
                        {
                            if (tt == 3)
                                alpha_after = isalnum(e2a_tab[search_buf_[got-1]]) != 0;
                            else
                                alpha_after = isalnum(search_buf_[got-1]) != 0;
                        }
                    }

                    // Remove extra character obtained for wholeword test
                    if (got == min(buf_len, end - addr_buf) + 1)
                        got--;
                }
#ifdef TESTING1
                // For testing we allow 2 seconds for some changes to be made to the first
                // search block so that we can check that found_ is updated correctly
                if (addr_buf == 0)
                {
                    ::Sleep(2000);
                    TRACE1("+++ Finished sleep in %p\n", this);
                }
#endif

                for (unsigned char *pp = search_buf_;
                     (pp = bb.findforw(pp, got - (pp-search_buf_), ignorecase, tt, wholeword,
                          alpha_before, alpha_after, alignment, offset, base_addr, addr_buf + (pp-search_buf_))) != NULL;
                     ++pp)
                {
                    // Found one
                    ++count;

                    CSingleLock sl(&docdata_, TRUE);

                    if (search_command_ != NONE)
                        goto stop_search;

                    found_.insert(addr_buf + (pp - search_buf_));

                    if (tt == 1)
                        alpha_before = isalnum(*pp) != 0;
                    else if (tt == 3)
                        alpha_before = isalnum(e2a_tab[*pp]) != 0;
                    else if (pp > search_buf_)
                        alpha_before = isalnum(*(pp-1)) != 0;   // Check low byte of Unicode
                    else
                        alpha_before = false;                   // Only one byte before - we need 2 for Unicode
                }

                addr_buf += got - (bb.length() - 1);

                find_done_ = double(addr_buf - start) / double(end - start);
            } // while there is more to search

            {
                CSingleLock sl(&docdata_, TRUE);

                // Check for any file insertions/deletions
                while (!to_adjust_.empty())
                {
                    FixFound(to_adjust_.front().start_, 
                             to_adjust_.front().end_,
                             to_adjust_.front().address_,
                             to_adjust_.front().adjust_);

                    to_adjust_.pop_front();
                }
                file_len = length_;

                // Remove the block just searched from to_search_
                to_search_.pop_front();
            }
		stop_search:
			;
        } // for
		delete[] search_buf_;
		search_buf_ = NULL;
    }
}

bool CHexEditDoc::SearchProcessStop()
{
	bool retval = false;

    CSingleLock sl(&docdata_, TRUE);
    switch (search_command_)
    {
    case STOP:                      // stop scan and wait
        TRACE1("+++ BGSearch: stop for %p\n", this);
        retval = true;
		break;
    case DIE:                       // terminate this thread
        TRACE1("+++ BGSearch: killed thread for %p\n", this);
        search_state_ = DYING;
		sl.Unlock();                // we need this here as AfxEndThread() never returns so d'tor is not called
		delete[] search_buf_;
		search_buf_ = NULL;
		AfxEndThread(1);            // kills thread (no return)
		break;                      // Avoid warning
    case NONE:                      // nothing needed here - just continue scanning
        break;
    default:                        // should not happen
        ASSERT(0);
    }

	search_command_ = NONE;
	return retval;
}
