// DocData.cpp : part of implementation of the CHexEditDoc class
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//
// This module handles the retrieving and modification to the data, including
// updating an undo list.  Two data structures are used in this process:
//  - An undo array which tracks all changes made to the data.  This is
//    separate to undo facilities of the view(s) which track changes in the
//    display as well as keeping track of changes made to the document (via
//    an index into this array).
//  - A locations linked list which represents where every byte of the current
//    displayed "file" comes from.  This list is rebuilt from the original data
//    file plus the undo array everytime a change to the document is made.
//  Note that the original data file is never modified by the user making
//       changes until the file is saved.  When the file is saved the original
//       file plus the locations linked list are used to create the new file.
//       If no changes move any part of the file (i.e. all changes just
//       overwrite existing parts of the file, but do not insert/delete)
//       then the file is modified in place (unless a backup is required).
//       For a very large file this is a lot faster than copying the whole
//       file, and means you can get by without temporary disk space.

#include "stdafx.h"
#include "HexEdit.h"
#include "boyer.h"

#include "HexEditDoc.h"
#include "Mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//const size_t doc_undo::limit = 5;

IMPLEMENT_DYNAMIC(CHexHint, CObject)        // document has changed
IMPLEMENT_DYNAMIC(CUndoHint, CObject)       // undo display changes (no doc change)
IMPLEMENT_DYNAMIC(CRemoveHint, CObject)     // remove from undo stack (no doc changes)
IMPLEMENT_DYNAMIC(CBGSearchHint, CObject)   // background search has finished (no doc changes)
IMPLEMENT_DYNAMIC(CBGAerialHint, CObject)   // background scan has finished (bitmap has changed)
//IMPLEMENT_DYNAMIC(CRefreshHint, CObject)    // delayed tree view update when doc has changed
IMPLEMENT_DYNAMIC(CSaveStateHint, CObject)  // save tree state (typically before redraw)
IMPLEMENT_DYNAMIC(CRestoreStateHint, CObject) // restore tree state (typically after redraw)
IMPLEMENT_DYNAMIC(CDFFDHint, CObject)       // redraw required due to changed doc, template etc
IMPLEMENT_DYNAMIC(CCompHint, CObject)       // redraw required due to changes in compare file
IMPLEMENT_DYNAMIC(CBookmarkHint, CObject)   // A bookmark has been added/removed
IMPLEMENT_DYNAMIC(CTrackHint, CObject)      // Need to invalidate extra things for change tracking
IMPLEMENT_DYNAMIC(CBGPreviewHint, CObject)  // load of preview bitmap has finished

size_t CHexEditDoc::GetData(unsigned char *buf, size_t len, FILE_ADDRESS address, int use_bg /*= -1*/)
{
	ASSERT(use_bg == -1 || use_bg == 2 || use_bg == 3 || use_bg == 4 || use_bg == 5);   // 0 and 1 are no longer used
	ASSERT(address >= 0);
	FILE_ADDRESS pos;           // Tracks file position of current location record
	ploc_t pl;                  // Current location record

	CFile64 *pfile;
	switch (use_bg)
	{
	case 2:
		pfile = pfile2_;        // Background search file
		break;
	case 3:
		pfile = pfile3_;        // Aerial scan thread file
		break;
	case 4:
		pfile = pfile4_;        // Background compare thread file
		break;
	case 5:
		pfile = pfile5_;        // Stats thread file
		break;
	default:
		ASSERT(0);
		// fall through
	case -1:
		pfile = pfile1_;		// Normal file
	}

    CSingleLock sl(&docdata_, TRUE);

	// Find the 1st loc record that has (some of) the data
	for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
	{
		ASSERT((pl->dlen >> 62) != 0);
		if (address < pos + FILE_ADDRESS(pl->dlen&doc_loc::mask))
			break;
	}

	// Get the data from each loc record until buf is full
	size_t left;                        // How much is left to copy
	FILE_ADDRESS start = address - pos; // Where to start copy in this block
	size_t tocopy;                      // How much to copy in this block
	for (left = len; left > 0 && pl != loc_.end(); left -= tocopy, buf += tocopy)
	{
		tocopy = size_t(min(FILE_ADDRESS(left), FILE_ADDRESS(pl->dlen&doc_loc::mask) - start));
		ASSERT(tocopy < 0x10000000);
		if ((pl->dlen >> 62) == 1)
		{
			// Read data from the original file
			UINT actual;                // Number of bytes actually read from file

			pfile->Seek(pl->fileaddr + start, CFile::begin);
			if ((actual = pfile->Read((void *)buf, (UINT)tocopy)) < tocopy)
			{
				ASSERT(shared_);  // We should only run out of data if underlying file size has changed (should only happen if file was opened shareable)

				// File on disk is now shorter - just fill missing bytes with zero
				memset(buf + actual, '\0', tocopy - actual);
			}
		}
		else if ((pl->dlen >> 62) == 2)
		{
			memcpy(buf, pl->memaddr + start, tocopy);
		}
		else
		{
			ASSERT((pl->dlen >> 62) == 3);

			// Read data from the data file
			UINT actual;                // Number of bytes actually read from file

			FILE_ADDRESS fileaddr = pl->fileaddr&doc_loc::fmask;
			int idx = int((pl->fileaddr>>62)&0x3);   // must change when fmask/max_data_files changes

			switch (use_bg)
			{
			case 2:
				ASSERT(data_file2_[idx] != NULL);
				data_file2_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file2_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			case 3:
				ASSERT(data_file3_[idx] != NULL);
				data_file3_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file3_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			case 4:
				ASSERT(data_file4_[idx] != NULL);
				data_file4_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file4_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			case 5:
				ASSERT(data_file5_[idx] != NULL);
				data_file5_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file5_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			case 6:
				ASSERT(data_file6_[idx] != NULL);
				data_file6_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file6_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			default:
				ASSERT(0);
				// fall through
			case -1:
				ASSERT(data_file_[idx] != NULL);
				data_file_[idx]->Seek(fileaddr + start, CFile::begin);
				actual = data_file_[idx]->Read((void *)buf, (UINT)tocopy);
				break;
			}
			if (actual != tocopy)
			{
				// If we run out of file there is something wrong with our data
				ASSERT(0);
				left -= actual;
				break;
			}
		}

		// Move to start of next loc record
		start = 0;
		pos += (pl->dlen&doc_loc::mask);
		++pl;
	}

	// Return the actual number of bytes written to buf
	return len - left;
}

// Create a new temp data file so that we can save to disk rather than using lots of memory
int CHexEditDoc::AddDataFile(LPCTSTR name, BOOL temp /*=FALSE*/)
{
	int ii;

	// First check if the file is already there
	for (ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file_[ii] != NULL && data_file_[ii]->GetFilePath().CompareNoCase(name) == 0)
			return ii;
	}

	// Now find an empty slot and open the file
	for (ii = 0; ii < doc_loc::max_data_files; ++ii)
		if (data_file_[ii] == NULL)
		{
			data_file_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);

			// If background searching on also open 2nd copy of the file
			if (pthread2_ != NULL)
			{
				ASSERT(data_file2_[ii] == NULL);
				data_file2_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
			}
			// If aerial scan is on also open 3rd copy of the file
			if (pthread3_ != NULL)
			{
				ASSERT(data_file3_[ii] == NULL);
				data_file3_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
			}
			// If bg compare is on also open 4th copy of the file
			if (pthread4_ != NULL)
			{
				ASSERT(data_file4_[ii] == NULL);
				data_file4_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
			}
			// If background stats are on also open 5th copy of the file
			if (pthread5_ != NULL)
			{
				ASSERT(data_file5_[ii] == NULL);
				data_file5_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
			}
			// If preview is on also open 6th copy of the file
			if (pthread6_ != NULL)
			{
				ASSERT(data_file6_[ii] == NULL);
				data_file6_[ii] = new CFile64(name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);
			}

			temp_file_[ii] = temp;
			return ii;
		}

	return -1;  // no empty slots
}

void CHexEditDoc::RemoveDataFile(int idx)
{
	ASSERT(idx >= 0 && idx < doc_loc::max_data_files);
	if (data_file_[idx] != NULL)
	{
		CString ss;
		if (temp_file_[idx])
			ss = data_file_[idx]->CFile64::GetFilePath();  // save the file name so we can delete it
		data_file_[idx]->Close();
		delete data_file_[idx];
		data_file_[idx] = NULL;
		if (pthread2_ != NULL)
		{
			ASSERT(data_file2_[idx] != NULL);
			data_file2_[idx]->Close();
			delete data_file2_[idx];
			data_file2_[idx] = NULL;
		}
		if (pthread3_ != NULL)
		{
			ASSERT(data_file3_[idx] != NULL);
			data_file3_[idx]->Close();
			delete data_file3_[idx];
			data_file3_[idx] = NULL;
		}
		if (pthread4_ != NULL)
		{
			ASSERT(data_file4_[idx] != NULL);
			data_file4_[idx]->Close();
			delete data_file4_[idx];
			data_file4_[idx] = NULL;
		}
		if (pthread5_ != NULL)
		{
			ASSERT(data_file5_[idx] != NULL);
			data_file5_[idx]->Close();
			delete data_file5_[idx];
			data_file5_[idx] = NULL;
		}
		if (pthread6_ != NULL)
		{
			ASSERT(data_file6_[idx] != NULL);
			data_file6_[idx]->Close();
			delete data_file6_[idx];
			data_file6_[idx] = NULL;
		}
		// If the data file was a temp file remove it now it is closed
		if (temp_file_[idx])
			remove(ss);
	}
}

// Change allows the document to be modified.  After adding it to the undo
// array it regenerates the loc list and sends update notices to all views.
//   utype indicates the type of change
//   address is the byte within the document for the start of the change
//   clen is the number of bytes inserted/deleted or replaced (64 bit eg if whole file deleted)
//   buf points to the characters being inserted or replaced (NULL for deletes)
//       - the data pointed to by buf is copied so buf does not have to be kept
//   num_done indicates if this change is to be merged with a previous change,
//            0 means that this is the start of a new change
//            BUT if utype == mod_insert_file num_done = idx into data_file_[]
//   pview is a pointer to the view that is making the change (or NULL)
void CHexEditDoc::Change(enum mod_type utype, FILE_ADDRESS address, FILE_ADDRESS clen,
						 unsigned char *buf, 
						 int num_done, CView *pview, BOOL ptoo /*=FALSE*/)
{
	// Lock the doc data (automatically releases the lock when it goes out of scope)
	CSingleLock sl(&docdata_, TRUE);

	int index;          // index into undo array
	ASSERT(utype == mod_insert  || utype == mod_insert_file || utype == mod_replace ||
		   utype == mod_delforw || utype == mod_delback     || utype == mod_repback);
	ASSERT(address <= length_);
	ASSERT(clen > 0);

	// Can this change be merged with the previous change?
	// Note: if num_done is odd then we must merge this change
	// since it is the 2nd nybble of a hex edit change
	if (utype != mod_insert_file &&
		(((num_done % 2) == 1 && clen == 1) ||
		 (num_done > 0 && clen + undo_.back().len < theApp.undo_limit_)))
	{
		// Join this undo to the previous one (end of undo array).
		// This allows (up to 8) multiple consecutive changes in the same
		// view of the same type to be merged together.
		ASSERT(pview == last_view_ || num_done == 1);  // num_done may be 1 for first bottom nybble in vert_display mode
		ASSERT(utype == undo_.back().utype);

		if (utype == mod_delforw)
		{
			// More deletes forward
			ASSERT(buf == NULL);
			ASSERT(undo_.back().address == address);
			ASSERT(address + clen <= length_);
			undo_.back().len += clen;
		}
		else if (utype == mod_delback)
		{
			// More deletes backward
			ASSERT(buf == NULL);
			ASSERT(undo_.back().address == address + clen);
			undo_.back().address = address;
			undo_.back().len += clen;
		}
		else if (utype == mod_repback)
		{
			ASSERT(buf != NULL);
			ASSERT(undo_.back().address == address + clen);
			ASSERT(clen < 0x100000000);
			memmove(undo_.back().ptr + size_t(clen),
					undo_.back().ptr, size_t(undo_.back().len));
			memcpy(undo_.back().ptr, buf, size_t(clen));
			undo_.back().address = address;
			undo_.back().len += clen;
		}
		else if (undo_.back().address + undo_.back().len == address)
		{
			// mod_insert/mod_replace - add to end of current insert/replace
			ASSERT(buf != NULL);
			memcpy(undo_.back().ptr + size_t(undo_.back().len), buf, size_t(clen));
			undo_.back().len += clen;
		}
		else
		{
			// mod_insert/mod_replace but last byte of previous undo changed.
			// This happens in hex edit mode when 2nd nybble entered
			ASSERT(undo_.back().address + undo_.back().len - 1 == address);
			memcpy(undo_.back().ptr+undo_.back().len - 1, buf, size_t(clen));
			clen -= 1;                   // Fix for later calcs of length_
			undo_.back().len += clen;
		}
		index = -1;             // Signal that this is not a new undo
	}
	else
	{
		// Add a new elt to undo array
		if (utype == mod_insert_file)
		{
			ASSERT(num_done > -1 && data_file_[num_done] != NULL);
			undo_.push_back(doc_undo(utype, address, clen, NULL, num_done));
		}
		else if (utype == mod_insert || utype == mod_replace || utype == mod_repback)
			undo_.push_back(doc_undo(utype, address, clen, buf));
		else
			undo_.push_back(doc_undo(utype, address, clen));
		index = undo_.size() - 1;
	}

	last_view_ = pview;

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// If there is a current search string and background searches are on
	if (aa->pboyer_ != NULL && pthread2_ != NULL)
	{
		ASSERT(CanDoSearch());
		FILE_ADDRESS adjust;

		if (aa->alignment_ > 1 && (utype == mod_delforw ||
								   utype == mod_delback ||
								   utype == mod_insert  ||
								   utype == mod_insert_file) )
		{
			// Remove pending searches after current address (to avoid double search)
			std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::iterator pcurr, pend;
			for (pcurr = to_search_.begin(), pend = to_search_.end(); pcurr != pend; ++pcurr)
			{
				if (pcurr->first >= address)
					pcurr->second = pcurr->first + 1;  // simple way to "remove" this one
				else if (pcurr->second >= address)
					pcurr->second = address;           // truncate at address
			}

			// Remove all found occurrences to EOF
			found_.erase(found_.lower_bound(address), found_.lower_bound(length_));

			// Invalidate area of change (rest towards EOF is invalidated below)
			CBGSearchHint bgsh(address - aa->pboyer_->length() + 1, address + clen);
			UpdateAllViews(NULL, 0, &bgsh);
		}
		else
		{
			if (utype == mod_delforw || utype == mod_delback)
				adjust = -clen;
			else if (utype == mod_insert || utype == mod_insert_file)
				adjust = clen;
			else if (utype == mod_replace || utype == mod_repback)
				adjust = 0;
			else
				ASSERT(0);

			// Adjust pending search addresses
			std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::iterator pcurr, pend;
			for (pcurr = to_search_.begin(), pend = to_search_.end(); pcurr != pend; ++pcurr)
			{
				if (pcurr->first >= address)
				{
					pcurr->first += adjust;
					if (pcurr->first < address)  // Make sure it doesn't go too far backwards
						pcurr->first = address;
				}
				if (pcurr->second >= address)
				{
					pcurr->second += adjust;
					if (pcurr->second < address)  // Make sure it doesn't go too far backwards
						pcurr->second = address;
				}
			}

			// Remove invalidated found_ occurrences and adjust those for insertions/deletions
			if (!to_search_.empty())
			{
				// Ask bg thread to do it so that changes are kept consistent
				ASSERT(!search_fin_);
				to_adjust_.push_back(adjustment(address - (aa->pboyer_->length() - 1),
												(utype == mod_insert || utype == mod_insert_file) ? address : address + clen,
												address,
												adjust));
				// Tell views to remove currently displayed occurrences
				CBGSearchHint bgsh(FALSE);
				UpdateAllViews(NULL, 0, &bgsh);
			}
			else
			{
				FixFound(address - (aa->pboyer_->length() - 1),
						(utype == mod_insert || utype == mod_insert_file) ? address : address + clen,
						address,
						adjust);

#if 0
				// Signal views to update display for changed search string occurrences
				if (utype == mod_replace || utype == mod_repback)
				{
					// Just invalidate replaced area (plus overlaps at ends for search string size)
					CBGSearchHint bgsh(address - aa->pboyer_->length() + 1, address + clen);
					UpdateAllViews(NULL, 0, &bgsh);
				}
				else if (utype == mod_insert || utype == mod_insert_file)
				{
					// Invalidate to new EOF
					CBGSearchHint bgsh(address - aa->pboyer_->length() + 1, length_ + adjust);
					UpdateAllViews(NULL, 0, &bgsh);
				}
				else
				{
					// Invalidate to old EOF
					CBGSearchHint bgsh(address - aa->pboyer_->length() + 1, length_);
					UpdateAllViews(NULL, 0, &bgsh);
				}
#else
				// Invalidate area of change (for deletions/insertions displayed bit towards EOF is invalidated below)
				CBGSearchHint bgsh(address - aa->pboyer_->length() + 1, address + (clen == 0 ? 1 : clen));
				UpdateAllViews(NULL, 0, &bgsh);
#endif
			}
		}

		if (to_search_.empty())
			find_done_ = 0.0;           // Clear amount searched before adding a new search

		// Add new area to be searched
		if (aa->alignment_ > 1 && (utype == mod_delforw ||
								   utype == mod_delback ||
								   utype == mod_insert  ||
								   utype == mod_insert_file) )
		{
			to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(address - (aa->pboyer_->length() - 1), length_));
			find_total_ += length_ - (address - (aa->pboyer_->length() - 1));
		}
		else if (utype == mod_delforw || utype == mod_delback)
		{
			to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(address - (aa->pboyer_->length() - 1), address));
			find_total_ += aa->pboyer_->length() - 1;
		}
		else
		{
			to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(address - (aa->pboyer_->length() - 1), address + (clen <= 0 ? 1 : clen)));
			find_total_ += clen + aa->pboyer_->length() - 1;
		}

		// Restart bg search thread in case it is waiting
		search_fin_ = false;
		TRACE1("Restarting bg search (change) for %p\n", this);
		start_search_event_.SetEvent();
	}

	// Adjust file length according to bytes added or removed
	if (utype == mod_delforw || utype == mod_delback)
		length_ -= clen;
	else if (utype == mod_insert || utype == mod_insert_file)
		length_ += clen;
	else if (utype == mod_replace && address + clen > length_)
		length_ = address + clen;
	else
		ASSERT(utype == mod_replace || utype == mod_repback);

	// Update bookmarks
	if (utype == mod_delforw || utype == mod_delback)
	{
		for (std::vector<FILE_ADDRESS>::iterator pp = bm_posn_.begin();
			 pp != bm_posn_.end(); ++pp)
		{
			if (*pp > address + clen)
				*pp -= clen;
			else if (*pp > address)
				*pp = address;
		}
	}
	else if (utype == mod_insert || utype == mod_insert_file)
	{
		for (std::vector<FILE_ADDRESS>::iterator pp = bm_posn_.begin();
			 pp != bm_posn_.end(); ++pp)
		{
			if (*pp >= address)
				*pp += clen;
		}
	}

	// Rebuild the location list
	regenerate();

	update_needed_ = true;
	send_change_hint(address);

	// Unlock now since nothing below is protected by the docdata_
	sl.Unlock();

	doc_changed_ = true;        // Remember to restart bg scans when we get a chance

	// Update views to show changed doc
	CHexHint hh(utype, clen, address, pview, index, FALSE, ptoo);
	if (utype == mod_insert && clen == 0)
	{
		// Insert hex changed low nybble without inserting
		hh.utype = mod_replace;
		hh.len = 1;
	}
	SetModifiedFlag(TRUE);
	UpdateAllViews(NULL, 0, &hh);
}

// Undo removes the last change made by calling Change() [above]
//   pview is a pointer to the view requesting the undo
//   index is the undo index [at the moment should always be undo_.size()-1]
//   same_view is true if the view undo change originally made the change
BOOL CHexEditDoc::Undo(CView *pview, int index, BOOL same_view)
{
	ASSERT(index == undo_.size() - 1);
	ASSERT(undo_.back().utype == mod_insert  ||
		   undo_.back().utype == mod_insert_file  ||
		   undo_.back().utype == mod_replace ||
		   undo_.back().utype == mod_delforw ||
		   undo_.back().utype == mod_delback ||
		   undo_.back().utype == mod_repback );

	// If undoing change made in different view ask the user to confirm
	if (!same_view)
	{
		CString mess;
		mess.Format("Undo %s?",
			undo_.back().utype == mod_insert       ? "insertion" :
			undo_.back().utype == mod_insert_file  ? "insertion" :
			undo_.back().utype == mod_replace      ? "change"    :
			undo_.back().utype == mod_repback      ? "backspace" : "deletion");
		if (TaskMessageBox(mess, "This change was made in a different window opened on the same file.\n\n"
			              "Are you sure you want to undo this change?", MB_OKCANCEL) == IDCANCEL)
			return FALSE;
	}

	// Inform views to undo everything up to last doc change
	// before changing the document.  This ensures that any view changes
	// that rely on the document as it was are undone before the document
	// is change is undone.
	CUndoHint uh(pview, index);
	UpdateAllViews(NULL, 0, &uh);

	// Work out the hint which says where and how the document
	// has changed.  Note that undoing an insert is equivalent
	// to deleting bytes, undoing a delete == insert etc.
	CHexHint hh(undo_.back().utype == mod_insert      ? mod_delforw :
				undo_.back().utype == mod_insert_file ? mod_delforw :
				undo_.back().utype == mod_replace     ? mod_replace :
				undo_.back().utype == mod_repback     ? mod_replace : mod_insert,
				undo_.back().len, undo_.back().address, pview, index, TRUE);
	FILE_ADDRESS change_address;

	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

		// Lock the doc data (automatically releases the lock when it goes out of scope)
		CSingleLock sl(&docdata_, TRUE);

		// If there is a current search string and background searches are on
		if (aa->pboyer_ != NULL && pthread2_ != NULL)
		{
			ASSERT(CanDoSearch());

			if (aa->alignment_ > 1 && (undo_.back().utype == mod_delforw ||
									undo_.back().utype == mod_delback ||
									undo_.back().utype == mod_insert  ||
									undo_.back().utype == mod_insert_file) )
			{
				// Remove pending searches after current address (to avoid double search)
				std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::iterator pcurr, pend;
				for (pcurr = to_search_.begin(), pend = to_search_.end(); pcurr != pend; ++pcurr)
				{
					if (pcurr->first >= undo_.back().address)
						pcurr->second = pcurr->first + 1;               // simple way to "remove" this one
					else if (pcurr->second >= undo_.back().address)
						pcurr->second = undo_.back().address;           // truncate at address
				}

				// Remove all found occurrences to EOF
				found_.erase(found_.lower_bound(undo_.back().address), found_.lower_bound(length_));

				// Invalidate area of change (rest towards EOF is invalidated below)
				CBGSearchHint bgsh(undo_.back().address - aa->pboyer_->length() + 1, undo_.back().address + undo_.back().len);
				UpdateAllViews(NULL, 0, &bgsh);
			}
			else
			{
				FILE_ADDRESS adjust;

				if (undo_.back().utype == mod_delforw || undo_.back().utype == mod_delback)
					adjust = undo_.back().len;
				else if (undo_.back().utype == mod_insert || undo_.back().utype == mod_insert_file)
					adjust = -undo_.back().len;
				else if (undo_.back().utype == mod_replace || undo_.back().utype == mod_repback)
					adjust = 0;
				else
					ASSERT(0);

				// Adjust pending search addresses
				std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::iterator pcurr, pend;
				for (pcurr = to_search_.begin(), pend = to_search_.end(); pcurr != pend; ++pcurr)
				{
					if (pcurr->first >= undo_.back().address)
					{
						pcurr->first += adjust;
						if (pcurr->first < undo_.back().address)  // Make sure it doesn't go too far backwards
							pcurr->first = undo_.back().address;
					}
					if (pcurr->second >= undo_.back().address)
					{
						pcurr->second += adjust;
						if (pcurr->second < undo_.back().address)  // Make sure it doesn't go too far backwards
							pcurr->second = undo_.back().address;
					}
				}

				// Remove invalidated found_ occurrences and adjust those for insertiosn deletions
				if (!to_search_.empty())
				{
					// Ask bg thread to do it so that changes are kept consistent
					ASSERT(!search_fin_);
					to_adjust_.push_back(adjustment(undo_.back().address - (aa->pboyer_->length() - 1),
													undo_.back().utype == mod_delforw || undo_.back().utype == mod_delback ?
														undo_.back().address : undo_.back().address + undo_.back().len,
													undo_.back().address,
													adjust));

					// Tell views to remove currently displayed occurrences
					CBGSearchHint bgsh(FALSE);
					UpdateAllViews(NULL, 0, &bgsh);
				}
				else
				{
					FixFound(undo_.back().address - (aa->pboyer_->length() - 1),
							undo_.back().utype == mod_delforw || undo_.back().utype == mod_delback ? 
								undo_.back().address : undo_.back().address + undo_.back().len,
							undo_.back().address,
							adjust);

#if 0
					// Signal view to update display for changed search string occurrences
					if (undo_.back().utype == mod_replace || undo_.back().utype == mod_repback)
					{
						// Just invalidate replaced area (plus overlaps at ends for search string size)
						CBGSearchHint bgsh(undo_.back().address - aa->pboyer_->length() + 1, undo_.back().address + undo_.back().len);
						UpdateAllViews(NULL, 0, &bgsh);
					}
					else if (undo_.back().utype == mod_insert || undo_.back().utype == mod_insert_file)
					{
						// Invalidate to old EOF
						CBGSearchHint bgsh(undo_.back().address - aa->pboyer_->length() + 1, length_);
						UpdateAllViews(NULL, 0, &bgsh);
					}
					else
					{
						// Invalidate to new EOF
						CBGSearchHint bgsh(undo_.back().address - aa->pboyer_->length() + 1, length_ + adjust);
						UpdateAllViews(NULL, 0, &bgsh);
					}
#else
					// Invalidate area of change (for deletions/insertions displayed bit towards EOF is invalidated below)
					CBGSearchHint bgsh(undo_.back().address - aa->pboyer_->length() + 1, undo_.back().address + undo_.back().len);
					UpdateAllViews(NULL, 0, &bgsh);
#endif
				}
			}

			if (to_search_.empty())
				find_done_ = 0.0;           // Clear amount searched before adding a new search

			// Add new area to be searched
			if (aa->alignment_ > 1 && (undo_.back().utype == mod_delforw ||
									   undo_.back().utype == mod_delback ||
									   undo_.back().utype == mod_insert  ||
									   undo_.back().utype == mod_insert_file) )
			{
				// xxx we still need to delete old occurrences to end of file
				to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>(undo_.back().address - (aa->pboyer_->length() - 1), length_));
				find_total_ += length_ - (undo_.back().address - (aa->pboyer_->length() - 1));
			}
			else if (undo_.back().utype == mod_insert || undo_.back().utype == mod_insert_file)
			{
				to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>
										 (undo_.back().address - aa->pboyer_->length() + 1,
										  undo_.back().address));
				find_total_ += aa->pboyer_->length() - 1;
			}
			else
			{
				to_search_.push_back(pair<FILE_ADDRESS, FILE_ADDRESS>
										 (undo_.back().address - aa->pboyer_->length() + 1,
										  undo_.back().address + undo_.back().len));
				find_total_ += undo_.back().len + aa->pboyer_->length() - 1;
			}

			// Restart bg search thread in case it is waiting
			search_fin_ = false;
			TRACE1("Restarting bg search (undo) for %p\n", this);
			start_search_event_.SetEvent();
		}

		// Recalc doc size if nec.
		if (undo_.back().utype == mod_delforw || undo_.back().utype == mod_delback)
			length_ += undo_.back().len;
		else if (undo_.back().utype == mod_insert || undo_.back().utype == mod_insert_file)
			length_ -= undo_.back().len;
		else if (undo_.back().utype == mod_replace &&
				 undo_.back().address + undo_.back().len > length_)
			length_ = undo_.back().address + undo_.back().len;

		// Update bookmarks
		if (undo_.back().utype == mod_delforw || undo_.back().utype == mod_delback)
		{
			for (std::vector<FILE_ADDRESS>::iterator pp = bm_posn_.begin();
				 pp != bm_posn_.end(); ++pp)
			{
				if (*pp > undo_.back().address)
					*pp += undo_.back().len;
			}
		}
		else if (undo_.back().utype == mod_insert || undo_.back().utype == mod_insert_file)
		{
			for (std::vector<FILE_ADDRESS>::iterator pp = bm_posn_.begin();
				 pp != bm_posn_.end(); ++pp)
			{
				if (*pp > undo_.back().address + undo_.back().len)
					*pp -= undo_.back().len;
				else if (*pp > undo_.back().address)
					*pp = undo_.back().address;
			}
		}

		change_address = undo_.back().address;

		// Remove the change from the undo array since it has now been undone
		undo_.pop_back();
		if (undo_.size() == 0)
			SetModifiedFlag(FALSE);     // Undid everything so clear changed flag

		// Rebuild locations list as undo array has changed
		regenerate();
	}

	doc_changed_ = true;        // Remember to restart bg scans when we get a chance

	update_needed_ = true;
	send_change_hint(change_address);

	// Update views because doc contents have changed
	UpdateAllViews(NULL, 0, &hh);
	return TRUE;
}

#if 0
// Adjust addresses in file and remove invalidated already found addresses
// start, end = the range of found addresses to remove
// address = where address adjustments start, adjust = amount to adjust
void CHexEditDoc::fix_address(FILE_ADDRESS start, FILE_ADDRESS end,
							  FILE_ADDRESS address, FILE_ADDRESS adjust)
{
	// Remove invalidated found_ occurrences and adjust those for insertiosn deletions
	if (!to_search_.empty())
	{
		// Ask bg thread to do it so that changes are kept consistent
		ASSERT(!search_fin_);
		to_adjust_.push_back(adjustment(start, end, address, adjust));
	}
	else
		FixFound(start, end, address, adjust);

	// Adjust pending search addresses
	std::list<pair<FILE_ADDRESS, FILE_ADDRESS> >::iterator pcurr, pend;
	for (pcurr = to_search_.begin(), pend = to_search_.end(); pcurr != pend; ++pcurr)
	{
		if (pcurr->first >= address)
		{
			pcurr->first += adjust;
			if (pcurr->first < address)  // Make sure it doesn't go too far backwards
				pcurr->first = address;
		}
		if (pcurr->second >= address)
		{
			pcurr->second += adjust;
			if (pcurr->second < address)  // Make sure it doesn't go too far backwards
				pcurr->second = address;
		}
	}
}
#endif

// Returns TRUE if all changes to the document are only overtyping of
// parts of original file.  That is, all parts of the original file that
// would be written by a save are not moved from their orig. location.
// (ie2, all changes = replaces or inserts with matching deletes).
BOOL CHexEditDoc::only_over()
{
	FILE_ADDRESS pos;
	ploc_t pl;          // Current location record

	// Check that each file record is at right place in file
	for (pos = 0L, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
	{
		ASSERT((pl->dlen >> 62) != 0);
		if ((pl->dlen >> 62) == 1 && pl->fileaddr != pos)
			return FALSE;
	}
	// Make sure file length not changed
	if (pfile1_ == NULL || pos != pfile1_->GetLength())
		return FALSE;

	return TRUE;
}

// Write changes from memory to file (all mods must be 'in place')
void CHexEditDoc::WriteInPlace()
{
	ASSERT(pfile1_ != NULL);

	FILE_ADDRESS pos;
	ploc_t pl;          // Current location record
	ploc_t plprev;      // Record before this one

	// Lock the doc data (automatically releases the lock when it goes out of scope)
	CSingleLock sl(&docdata_, TRUE);

	const size_t copy_buf_len = 16384;
	unsigned char *buf = new unsigned char[copy_buf_len];   // Where we store data
#ifdef INPLACE_MOVE
	FILE_ADDRESS previous_length = pfile1_->GetLength();
	FILE_ADDRESS total_todo = 0;                            // Number of bytes to be moved in file or copied in
	FILE_ADDRESS total_done = 0;                            // Number of bytes done so far
	for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
	{
		if ((pl->dlen >> 62) > 1 || pos != pl->fileaddr)
			total_todo += (pl->dlen&doc_loc::mask);
	}

	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	mm->m_wndStatusBar.EnablePaneProgressBar(0);
	clock_t last_checked = clock();
#endif

	try
	{
#ifdef INPLACE_MOVE
		// Bits of file from the original file can be moved forward or backward in the "new" file,
		// BUT they never overlap, so we can move them all within the file. We just need to:
		// 1. move blocks that have moved forward starting from the end of the file
		// 2. move blocks that have moved backward starting from the beginning of the file
		// 3. do nothing for blocks that have not moved
		// 4. write memory blocks to the file (as before)
		for (pos = length_, pl = loc_.end(); pl != loc_.begin(); pos -= (plprev->dlen&doc_loc::mask), pl--)
		{
			plprev = pl;
			plprev--;

			if ((plprev->dlen >> 62) == 1 && pos - FILE_ADDRESS(plprev->dlen&doc_loc::mask) > plprev->fileaddr)
			{
				// Copy the block in chunks (copy_buf_len) starting at the end
				FILE_ADDRESS src = plprev->fileaddr + (plprev->dlen&doc_loc::mask);
				FILE_ADDRESS dst = pos;

				while (src > plprev->fileaddr)
				{
					UINT count;
					if (src - copy_buf_len > plprev->fileaddr)
						count = copy_buf_len;
					else
						count = UINT(src - plprev->fileaddr);
					src -= count;
					dst -= count;

					VERIFY(pfile1_->Seek(src, CFile::begin) == src);
					VERIFY(pfile1_->Read(buf, count) == count);
					VERIFY(pfile1_->Seek(dst, CFile::begin) == dst);
					pfile1_->Write(buf, count);

					// Update progress
					total_done += count;
					if ((clock() - last_checked)/CLOCKS_PER_SEC > 2)
					{
						mm->m_wndStatusBar.SetPaneProgress(0, long(total_done*100/total_todo));
						last_checked = clock();
						AfxGetApp()->OnIdle(0);
					}
				}
				ASSERT(src == plprev->fileaddr);
				ASSERT(dst == pos - (plprev->dlen&doc_loc::mask));
			}
		}
		for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
		{
			if ((pl->dlen >> 62) == 1 && pos < pl->fileaddr)
			{
				FILE_ADDRESS src = pl->fileaddr;
				FILE_ADDRESS dst = pos;

				while (src < pl->fileaddr + FILE_ADDRESS(pl->dlen&doc_loc::mask))
				{
					UINT count;
					if (src + copy_buf_len <= pl->fileaddr + FILE_ADDRESS(pl->dlen&doc_loc::mask))
						count = copy_buf_len;
					else
						count = UINT(pl->fileaddr + (pl->dlen&doc_loc::mask) - src);

					VERIFY(pfile1_->Seek(src, CFile::begin) == src);
					VERIFY(pfile1_->Read(buf, count) == count);
					VERIFY(pfile1_->Seek(dst, CFile::begin) == dst);
					pfile1_->Write(buf, count);

					src += count;
					dst += count;

					// Update progress
					total_done += count;
					if ((clock() - last_checked)/CLOCKS_PER_SEC > 2)
					{
						mm->m_wndStatusBar.SetPaneProgress(0, long(total_done*100/total_todo));
						last_checked = clock();
						AfxGetApp()->OnIdle(0);
					}
				}
				ASSERT(src == pl->fileaddr + (pl->dlen&doc_loc::mask));
				ASSERT(dst == pos + (pl->dlen&doc_loc::mask));

			}
		}
#endif
		for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
		{
			if ((pl->dlen >> 62) == 2)
			{
				// Write in memory bits at appropriate places in file
				VERIFY(pfile1_->Seek(pos, CFile::begin) == pos);
				pfile1_->Write(pl->memaddr, DWORD(pl->dlen&doc_loc::mask));
#ifdef INPLACE_MOVE
				// Update progress bar
				total_done += (pl->dlen&doc_loc::mask);
				if ((clock() - last_checked)/CLOCKS_PER_SEC > 2)
				{
					mm->m_wndStatusBar.SetPaneProgress(0, long(total_done*100/total_todo));
					last_checked = clock();
					AfxGetApp()->OnIdle(0);
				}
#endif
			}
			else if ((pl->dlen >> 62) == 3)
			{
				// Copy data file into original file
				ASSERT((pl->dlen >> 62) == 3);
				FILE_ADDRESS fileaddr = pl->fileaddr&doc_loc::fmask;
				int idx = int((pl->fileaddr>>62)&0x3);   // 62 must change when fmask/max_data_files changes

				// Move to position in file that we are copying from
				ASSERT(data_file_[idx] != NULL);
				data_file_[idx]->Seek(fileaddr, CFile::begin);

				// Move to position to copy to
				VERIFY(pfile1_->Seek(pos, CFile::begin) == pos);
				UINT tocopy;                      // How much to copy in this block
				for (FILE_ADDRESS left = FILE_ADDRESS(pl->dlen&doc_loc::mask); left > 0; left -= tocopy)
				{
					tocopy = size_t(min(left, FILE_ADDRESS(copy_buf_len)));
					UINT actual = data_file_[idx]->Read((void *)buf, tocopy);
					ASSERT(actual == tocopy);
					pfile1_->Write(buf, tocopy);
#ifdef INPLACE_MOVE
					// Update progress bar
					total_done += tocopy;
					if ((clock() - last_checked)/CLOCKS_PER_SEC > 2)
					{
						mm->m_wndStatusBar.SetPaneProgress(0, long(total_done*100/total_todo));
						last_checked = clock();
						AfxGetApp()->OnIdle(0);
					}
#endif
				}

			}

		}
#ifdef INPLACE_MOVE
		ASSERT(total_done == total_todo);
		// Truncate the file if it is now shorter
		if (length_ < previous_length)
		{
			ASSERT(!IsDevice());
			pfile1_->SetLength(length_);
		}
#endif
		pfile1_->Flush();
	}
	catch (CFileException *pfe)
	{
		TaskMessageBox("File Save Error", ::FileErrorMessage(pfe, CFile::modeWrite));
		pfe->Delete();

#ifdef INPLACE_MOVE
		delete[] buf;
		mm->m_wndStatusBar.EnablePaneProgressBar(0, -1);  // disable progress bar
#endif

		theApp.mac_error_ = 10;
		return;
	}

#ifdef INPLACE_MOVE
	delete[] buf;
	mm->m_wndStatusBar.EnablePaneProgressBar(0, -1);  // disable progress bar
#endif
}

// Write the document (or part thereof) to file with name 'filename'.
// The range to write is given by 'start' and 'end'.
BOOL CHexEditDoc::WriteData(const CString filename, FILE_ADDRESS start, FILE_ADDRESS end, BOOL append /*=FALSE*/)
{
	// First warn if there may not be enough disk space
	if (AvailableSpace(filename) < end - start)
	{
		if (TaskMessageBox("Insufficient Disk Space",
			"There may not be enough disk space to write to the file.\n\n"
			"Do you want to continue?",
			MB_YESNO) == IDNO)
		{
			theApp.mac_error_ = 2;
			return FALSE;
		}
	}

	// Open the file for overwriting (or appending)
	CFile64 ff;
	CFileException fe;
	FILE_ADDRESS start_pos = 0;
	UINT flags = CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary;
	if (append)
		flags = CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary;

	// Open the file to write to
	if (!ff.Open(filename, flags, &fe))
	{
		TaskMessageBox("File Open Error", ::FileErrorMessage(&fe, CFile::modeWrite));
		theApp.mac_error_ = 10;
		return FALSE;
	}
	if (append)
		start_pos = ff.GetLength();

	// Get memory for a buffer
	const size_t copy_buf_len = 16384;
	unsigned char *buf = new unsigned char[copy_buf_len];   // Where we store data
	size_t got;                                 // How much we got from GetData

	// Copy the range to file catching exceptions (probably disk full)
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	mm->m_wndStatusBar.EnablePaneProgressBar(0);
	clock_t last_checked = clock();
	try
	{
		if (append)
			ff.SeekToEnd();

		FILE_ADDRESS address;
		for (address = start; address < end; address += FILE_ADDRESS(got))
		{
			got = GetData(buf, size_t(min(end-address, FILE_ADDRESS(copy_buf_len))), address);
			ASSERT(got > 0);

			ff.Write(buf, got);
			// Update save progress no more than once every 5 seconds
			if ((clock() - last_checked)/CLOCKS_PER_SEC > 5)
			{
				mm->Progress(int(((address-start)*100)/(end-start)));
				last_checked = clock();
				//mm->m_wndStatusBar.SetPaneProgress(0, long(((address-start)*98)/(end-start) + 1));
				//last_checked = clock();
				//AfxGetApp()->OnIdle(0);
			}
		}
		ASSERT(address == end);
	}
	catch (CFileException *pfe)
	{
		TaskMessageBox("File Write Error", ::FileErrorMessage(pfe, CFile::modeWrite));
		pfe->Delete();

		mm->Progress(-1);

		// Close the file and restore things as they were
		if (append)
			ff.SetLength(start_pos);   // truncate to original length
		ff.Close();
		if (!append)
			remove(filename);
		delete[] buf;

		theApp.mac_error_ = 10;
		return FALSE;
	}

	mm->Progress(-1);

	ff.Close();
	delete[] buf;
	return TRUE;
}

void CHexEditDoc::regenerate()
{
	pundo_t pu;         // Current modification (undo record) being checked
	FILE_ADDRESS pos;   // Tracks file position of current location record
	ploc_t pl;          // Current location record

	// This is used to track which data files are still in use - so we can release them when no longer needed
	std::vector<bool> file_used(doc_loc::max_data_files);

	// Rebuild locations list starting with original file as only loc record
	loc_.clear();
	if (pfile1_ != NULL && pfile1_->GetLength() > 0)
		loc_.push_back(doc_loc(FILE_ADDRESS(0), pfile1_->GetLength()));

	// Now check each modification in order to build up location list
	for (pu = undo_.begin(); pu != undo_.end(); ++pu)
	{
		// Find loc record where this modification starts
		for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
			if (pu->address < pos + FILE_ADDRESS(pl->dlen&doc_loc::mask))
				break;

		// Modify locations list here (according to type of mod)
		// Note: loc_add & loc_del may modify pos and pl (passed by reference)
		switch (pu->utype)
		{
		case mod_insert_file:
			ASSERT(pu->idx < doc_loc::max_data_files);
			file_used[pu->idx] = true;  // remember that this data file is still in use
			// fall through
		case mod_insert:
			loc_add(pu, pos, pl);       // Insert record into list
			break;
		case mod_replace:
		case mod_repback:
			loc_del(pu->address, pu->len, pos, pl); // Delete what's replaced
			loc_add(pu, pos, pl);       // Add replacement before next record
			break;
		case mod_delforw:
		case mod_delback:
			loc_del(pu->address, pu->len, pos, pl); // Just delete them
			break;
		default:
			ASSERT(0);
		}
	} /* for */

	// Signal that change tracking structures need rebuilding
	need_change_track_ = true;

	// If file is open shared then the underlying file length may change which could affect length_
	if (shared_)
	{
		// Zip through all the bits of the file adding up the length (in pos)
		for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
			;

		length_ = pos;
	}
#ifdef _DEBUG
	else
	{
		// Check that the length of all the records gels with stored doc length
		for (pos = 0, pl = loc_.begin(); pl != loc_.end(); pos += (pl->dlen&doc_loc::mask), ++pl)
			;
		ASSERT(pos == length_);
	}
#endif

	// Release any data files that are no longer used (presumably after an undo)
	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
		if (!file_used[ii])
			RemoveDataFile(ii);
}

// loc_add inserts a record into the location list
// pu describes the record to insert
// pos is the location in the doc of "pl"
// - on return is the new location of pl (the rec after the one inserted)
// pl is the record before or in which the insertion is to take place
// - on return is the rec after one inserted (!= entry value if rec split)
// (if pl is the end of the list then we just append)
void CHexEditDoc::loc_add(pundo_t pu, FILE_ADDRESS &pos, ploc_t &pl)
{
	if (pu->address != pos)
	{
		// We need to split a block into 2 to insert between the two pieces
		loc_split(pu->address, pos, pl);
		pos += (pl->dlen&doc_loc::mask);
		++pl;
	}
	if (pu->utype == mod_insert_file)
		loc_.insert(pl, doc_loc(0, pu->len, pu->idx));
	else
		loc_.insert(pl, doc_loc(pu->ptr, pu->len));
	pos += pu->len;
}

// loc_del deletes record(s) or part(s) thereof from the location list
// It returns the next undeleted record [which may be loc_.end()]
// address is where the deletions are to commence
// len is the number of bytes to be deleted
// pos is the location in the doc of "pl"
// - on return it's the loc of "pl" which may be different if a record was split
// pl is the record from or in which the deletion is to start
// - on return points to the record after the deletion [may be loc_.end()]
// Note: loc_del can be called for a mod_replace modification.  Replacements
// can go past EOF so deleting past EOF is also required to be handled here.
void CHexEditDoc::loc_del(FILE_ADDRESS address, FILE_ADDRESS len, FILE_ADDRESS &pos, ploc_t &pl)
{
	ploc_t byebye;              // Saved current record to be deleted

	if (address != pos)
	{
		ASSERT(pl != loc_.end());

		// We need to split this block so we can erase just the 2nd bit of it
		loc_split(address, pos, pl);
		pos += (pl->dlen&doc_loc::mask);
		++pl;
	}

	// Erase all the blocks until we get a block or part thereof to keep
	// or we hit end of document location list (EOF)
	FILE_ADDRESS deleted = 0;
	while (pl != loc_.end() && len >= deleted + FILE_ADDRESS(pl->dlen&doc_loc::mask))
	{
		byebye = pl;            // Save current record to delete later
		deleted += (pl->dlen&doc_loc::mask);   // Keep track of how much we've seen
		++pl;                   // Advance to next rec before deleting current
		loc_.erase(byebye);
	}

	if (pl != loc_.end() && len > deleted)
	{
		// We need to split this block and erase the 1st bit
		loc_split(address + len, address + deleted, pl);
		byebye = pl;
		deleted += (pl->dlen&doc_loc::mask);
		++pl;
		loc_.erase(byebye);
	}
//    ASSERT(deleted == len);
}

// address is where split takes place in the file
// pos is the file address of pl
// pl is a ptr to the doc_loc that needs to be split
void CHexEditDoc::loc_split(FILE_ADDRESS address, FILE_ADDRESS pos, ploc_t pl)
{
	ASSERT(pl != loc_.end());
	ASSERT(address > pos && address < pos + FILE_ADDRESS(pl->dlen&doc_loc::mask));

	// Work out exactly where to split the record
	FILE_ADDRESS split = pos + (pl->dlen&doc_loc::mask) - address;

	// Insert a new record before the next one and store location and length
	ploc_t plnext = pl; ++plnext;
	if ((pl->dlen >> 62) == 1)
	{
		loc_.insert(plnext, doc_loc(pl->fileaddr + (pl->dlen&doc_loc::mask) - split, split));
		pl->dlen = ((pl->dlen&doc_loc::mask) - split) | (unsigned __int64(1) << 62);
	}
	else if ((pl->dlen >> 62) == 2)
	{
		loc_.insert(plnext, doc_loc(pl->memaddr + (pl->dlen&doc_loc::mask) - split, split));
		pl->dlen = ((pl->dlen&doc_loc::mask) - split) | (unsigned __int64(2) << 62);
	}
	else
	{
		ASSERT((pl->dlen >> 62) == 3);
		FILE_ADDRESS fileaddr = pl->fileaddr&doc_loc::fmask;
		int idx = int((pl->fileaddr>>62)&0x3);   // 62 must change when fmask/max_data_files changes
		loc_.insert(plnext, doc_loc(fileaddr + (pl->dlen&doc_loc::mask) - split, split, idx));
		pl->dlen = ((pl->dlen&doc_loc::mask) - split) | (unsigned __int64(3) << 62);
	}
}

// The following is for change tracking.  This builds three vectors of replacements,
// insertions and deletions. Each vector element is a pair storing the address of the
// change and the length (ie no of bytes replaced, inserted or deleted).
// Note that this does not show all the changes that have been done only the changes
// between the current file and the file on disk.  For example, deleting 3 bytes then
// inserting 3 bytes at the same address would appear as a replacement of 3 bytes.
// Consequently there cannot be an insertion and deletion at the same address.

void CHexEditDoc::rebuild_change_tracking()
{
	// Clear the info before rebuild
	clear_change_tracking();

	FILE_ADDRESS pos = 0;       // Tracks position in current (displayed file)
	FILE_ADDRESS last_fileaddr = 0; // Tracks last byte used from orig file
	ploc_t pl = loc_.begin();   // Current location record
	FILE_ADDRESS nf_bytes = 0;  // Number of non-file bytes since last file record

	FILE_ADDRESS diff;          // Amt of orig file skipped since last file record
	FILE_ADDRESS repl;          // How many bytes are replaced
	FILE_ADDRESS orig_length = 0; // Length of file on which changes are based

	switch (base_type_)
	{
	case 0:
		if (pfile1_ != NULL)
			orig_length = pfile1_->GetLength();
		break;
	case 1:
	case 2:
		orig_length = undo_[0].len;
		break;
	default:
		ASSERT(0);
	}

	while (pl != loc_.end())
	{
		// Check if file record
		if ((pl->dlen >> 62) == 1)  // orig file data
		{
			ASSERT(base_type_ == 0);

			// Next orig file record found
			diff = pl->fileaddr - last_fileaddr;
			repl = min(diff, nf_bytes);
			if (repl > 0)
			{
				replace_addr_.push_back(pos);
				replace_len_.push_back(repl);
			}
			if (diff < nf_bytes)
			{
				insert_addr_.push_back(pos + repl);
				insert_len_.push_back(nf_bytes - diff);
			}
			else if (diff > nf_bytes)
			{
				delete_addr_.push_back(pos);
				delete_len_.push_back(diff - nf_bytes);
			}

			// Work out curr address by adding length of non-file recs skipped + current file rec
			pos += nf_bytes + (pl->dlen&doc_loc::mask);
			last_fileaddr = pl->fileaddr + (pl->dlen&doc_loc::mask); // remember last orig file addr
			nf_bytes = 0;
		}
		else if (base_type_ == 1 && (pl->dlen >> 62) == 2 &&
				 pl->memaddr >= undo_[0].ptr &&
				 pl->memaddr < undo_[0].ptr + size_t(undo_[0].len) )
		{
			// Record of the first memory block insertion with no orig file
			// (If no orig file we treat the first memory record as the base for comparison)
			diff = (pl->memaddr - undo_[0].ptr) - last_fileaddr;
			repl = min(diff, nf_bytes);

			if (repl > 0)
			{
				replace_addr_.push_back(pos);
				replace_len_.push_back(repl);
			}
			if (diff < nf_bytes)
			{
				insert_addr_.push_back(pos + repl);
				insert_len_.push_back(nf_bytes - diff);
			}
			else if (diff > nf_bytes)
			{
				delete_addr_.push_back(pos);
				delete_len_.push_back(diff - nf_bytes);
			}

			// Work out curr address by adding length of non-file recs skipped + current file rec
			pos += nf_bytes + (pl->dlen&doc_loc::mask);
			last_fileaddr = (pl->memaddr - undo_[0].ptr) + (pl->dlen&doc_loc::mask); // remember last mem addr
			nf_bytes = 0;
		}
		else if (base_type_ == 2 && (pl->dlen >> 62) == 3 &&
				 pl->memaddr >= undo_[0].ptr &&
				 pl->memaddr < undo_[0].ptr + size_t(undo_[0].len) )
		{
			// Record of the first temp file block insertion with no orig file
			// (If no orig file we treat the first temp file record as the base for comparison)
			diff = pl->fileaddr - last_fileaddr;
			repl = min(diff, nf_bytes);
			if (repl > 0)
			{
				replace_addr_.push_back(pos);
				replace_len_.push_back(repl);
			}
			if (diff < nf_bytes)
			{
				insert_addr_.push_back(pos + repl);
				insert_len_.push_back(nf_bytes - diff);
			}
			else if (diff > nf_bytes)
			{
				delete_addr_.push_back(pos);
				delete_len_.push_back(diff - nf_bytes);
			}

			// Work out curr address by adding length of non-file recs skipped + current file rec
			pos += nf_bytes + (pl->dlen&doc_loc::mask);
			last_fileaddr = pl->fileaddr + (pl->dlen&doc_loc::mask); // remember last mem addr
			nf_bytes = 0;
		}
		else
		{
			ASSERT((pl->dlen >> 62) == 2 || (pl->dlen >> 62) == 3);  // make sure not "unknown" type
			// Non-file record - just track no of consec. bytes in nf_bytes
			nf_bytes += (pl->dlen&doc_loc::mask);
		}
		++pl;
	}
	// Handle any unfinished business
	diff = orig_length - last_fileaddr;
	repl = min(diff, nf_bytes);
	if (repl > 0)
	{
		replace_addr_.push_back(pos);
		replace_len_.push_back(repl);
	}
	if (diff < nf_bytes)
	{
		insert_addr_.push_back(pos + repl);
		insert_len_.push_back(nf_bytes - diff);
	}
	else if (diff > nf_bytes)
	{
		delete_addr_.push_back(pos);
		delete_len_.push_back(diff - nf_bytes);
	}
	pos += nf_bytes;

	ASSERT(pos == length_);
	need_change_track_ = false;            // Signal that they have been rebuilt
}

void CHexEditDoc::clear_change_tracking()
{
	replace_addr_.clear();
	replace_len_.clear();
	insert_addr_.clear();
	insert_len_.clear();
	delete_addr_.clear();
	delete_len_.clear();
}

void CHexEditDoc::send_change_hint(FILE_ADDRESS address)
{
	FILE_ADDRESS pos = 0;         // Tracks position in current (displayed file)
	FILE_ADDRESS prev_change = 0; // Address of previous change in the file
	for (ploc_t pl = loc_.begin(); pl != loc_.end(); ++pl)
	{
		pos += (pl->dlen&doc_loc::mask);
		if (pos > address)
			break;
		if ((pl->dlen >> 62) == 1)
		{
			prev_change = pos;
		}
	}
	// Everything from start of previous change to current change needs invalidating
	CTrackHint th(prev_change, address);
	UpdateAllViews(NULL, 0, &th);
}
