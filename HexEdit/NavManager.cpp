#include "stdafx.h"
#include "HexEdit.h"
#include "HexEditView.h"
#include "HexEditDoc.h"
#include "ChildFrm.h"
#include "MainFrm.h"
#include "navmanager.h"

void CNavManager::Add(LPCTSTR desc, LPCTSTR info, CHexEditView * pv, FILE_ADDRESS aa, FILE_ADDRESS ee, FILE_ADDRESS as)
{
	if (in_move_)
		return;           // Disallow store of moves caused by messages created while moving to a nav pt

    if (v_.size() > 0 && pos_ == v_.size() - 1)
    {
        // If we swapped to a new view and the previous view had no nav moves then remove
        // the nav point from the end of the stack.  This avoids lots of nav pts when just
        // swapping between files.
        CHexEditView * pOldView =  v_.back().pview_;
        if (pOldView != pv && pOldView->NoNavMovesDone())
        {
            pos_--;
        }
    }

	ASSERT(pos_ >= -1 && pos_ < int(v_.size()));

	// Truncate the list past the current pos
	v_.erase(v_.begin() + (pos_ + 1), v_.end());

	// Push new entry on the end
	CHexEditDoc *pdoc = pv->GetDocument();
	ASSERT(pdoc != NULL);
	CString fname;
	if (pdoc->pfile1_ != NULL)  // We can't store a filename if yet to be written to disk
		fname = pdoc->pfile1_->GetFilePath();

	v_.push_back(nav(desc, info, pv, pdoc, fname, pdoc->read_only() == TRUE, aa, ee, as));
    TRACE("xxx ADDED %s\r\n", desc);
	pos_ = v_.size() - 1;               // Point to the last entry
}

void CNavManager::GoBack(int count /*= -1*/)
{
	if (count == -1)
	{
		// Go back to pos_ if not at it ELSE before it
		count = - pos_offset();
	}
    do_move(pos_ - count);
}

void CNavManager::GoForw(int count /*= 1*/)
{
    do_move(pos_ + count);
}

CString CNavManager::GetDesc(bool back, int ii)
{
	if (back)
		ii = pos_ - ii;
	else
		ii = pos_ + ii;
	if (ii < 0)
		ii = 0;
	ASSERT(ii < v_.size());
	return v_[ii].desc_;
}

CString CNavManager::GetInfo(bool back, int ii)
{
	if (back)
		ii = pos_ - ii;
	else
		ii = pos_ + ii;
	if (ii < 0)
		ii = 0;
	ASSERT(ii < v_.size());

	CString retval;

	// Get the address
	char buf[32];                   // used with sprintf (CString::Format can't handle __int64)
	if (is_open_view(v_[ii].pdoc_, v_[ii].pview_) && v_[ii].pview_->DecAddresses())
	{
		sprintf(buf, "%I64d", __int64(v_[ii].start_addr_));
		retval = buf;
		AddCommas(retval);
		if (v_[ii].end_addr_ != v_[ii].start_addr_)
		{
			CString ss;
			sprintf(buf, "%I64d", __int64(v_[ii].end_addr_));
			ss = buf;
			AddCommas(ss);

			retval += " - " + ss;
		}
	}
	else
	{
		if (theApp.hex_ucase_)
			sprintf(buf, "%I64X", __int64(v_[ii].start_addr_));
		else
			sprintf(buf, "%I64x", __int64(v_[ii].start_addr_));
		retval = buf;
		AddSpaces(retval);

		if (v_[ii].end_addr_ != v_[ii].start_addr_)
		{
			CString ss;
			if (theApp.hex_ucase_)
				sprintf(buf, "%I64X", __int64(v_[ii].end_addr_));
			else
				sprintf(buf, "%I64x", __int64(v_[ii].end_addr_));
			ss = buf;
			AddSpaces(ss);

			retval += " - " + ss;
		}
	}
	retval = "Address: " + retval;

	// Add info (sampling from file) on a new line
	retval += "\n" + v_[ii].info_;

	return retval;
}

// Adds navigation points to end of passed menu.
// If forward is true then forward nav pts are added else backward nav pts are used.
void CNavManager::AddItems(CMFCPopupMenu* pMenuPopup, bool forward, UINT first, int max_items)
{
	ASSERT( forward || pos_ /* + pos_offset() */ >= 0);        // Can only show backward pts if there are any
	ASSERT(!forward || pos_ < int(v_.size()) - 1);          // Can only show forward pts if there are any

	// Create a sub-menu for each window here.  Note that this items appear on time order
	// so the same window may appear more than once in the lists (but not consecutively).
	CMenu *submenu = NULL;
	int menu_items = 0;           // Items added so far to current sub-menu

	CHexEditView *pview = NULL;   // Previous view
	CHexEditDoc *pdoc = NULL;     // Previous doc
	CString pname;                // Previous file name

	for (int ii = 0, curr = (forward ? pos_ + 1 : pos_ /* + pos_offset() */);
	     ii < max_items && ((forward && curr < int(v_.size())) || (!forward && curr >= 0));
		 ++ii, (forward ? ++curr : curr--) )
	{
		// Check if we need to start a new sub-menu
		if (v_[curr].pview_ != pview)
		{
			// save current submenu and start a new one
			if (submenu != NULL)
			{
				CString ss;

				ASSERT(pview != NULL && pdoc != NULL);
				if (is_open_view(pdoc, pview))
					pview->GetFrame()->GetWindowText(ss);
				else if (!pname.IsEmpty())
					ss = pname;
				else
					ss = "Closed file";         // Closed without saving to disk?
                pMenuPopup->InsertItem(CMFCToolBarMenuButton(-1, submenu->GetSafeHmenu(), -1, ss));
				delete submenu;
				submenu = NULL;
			}

			submenu = new CMenu;
			submenu->CreatePopupMenu();
			pview = v_[curr].pview_;
			pdoc = v_[curr].pdoc_;
			pname = v_[curr].fname_;
			menu_items = 0;                     // Starting new sub-menu
		}

		// Only add max 50 items per sub-menu
		if (menu_items < 50)
		{
			UINT flags = MF_ENABLED;
			if (!forward && ii == 0 && pos_offset() == -1)
				flags |= MF_CHECKED;

#if 0  // Can we make this work???
			// Note: We tried saving nav point in menu item data rather than adding index to "first" to
			// get the menu item ID (command ID) to save IDs but by the time the command is received (see
			// CMainFrame::OnNavBack/Forw) the menu is already gone.
			// Check if we have run out of items (reserved command IDs)
			if (total_items >= max_items)
				break;                     // out of command ids
			submenu->AppendMenu(flags, first + total_items, v_[curr].desc_);

			// Save nav point index in menu item data so we know which nav pt is selected by the menu item
			MENUITEMINFO mii;
			memset(&mii, '\0', sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			mii.dwItemData = ii;
			submenu->SetMenuItemInfo(first + total_items, &mii);
			++total_items;
#else
			submenu->AppendMenu(flags, first + ii, v_[curr].desc_);
#endif
			++menu_items;                       // Track how many items are in this sub-menu
		}
	}
	ASSERT(submenu != NULL && pview != NULL);   // This should not be NULL since there must be at least one item
	if (submenu != NULL)
	{
		CString ss;
		if (is_open_view(pdoc, pview))
			pview->GetFrame()->GetWindowText(ss);
		else if (!pname.IsEmpty())
			ss = pname;
		else
			ss = "Closed file";
        pMenuPopup->InsertItem(CMFCToolBarMenuButton(-1, submenu->GetSafeHmenu(), -1, ss));
		delete submenu;
		submenu = NULL;
	}
}

// There has been an insertion/deletion is pdoc so we need to adjust nav pts accordingly
void CNavManager::Adjust(CHexEditDoc *pdoc, FILE_ADDRESS aa, FILE_ADDRESS diff)
{
	for (std::vector<nav>::iterator pv = v_.begin(); pv != v_.end(); ++pv)
	{
		if (pdoc != pv->pdoc_ || pv->end_addr_ < aa)
			continue;

		if (aa < pv->start_addr_)
			pv->start_addr_ += max(diff, aa + diff - pv->start_addr_);
		if (aa < pv->end_addr_)
			pv->end_addr_   += max(diff, aa + diff - pv->end_addr_);
	}
}

// If the current active position is at pos_ it returns -1, else it returns 0.  This allows nav list
// to show the current nav pt if we have moved off it but not far enough to create a new one.
int CNavManager::pos_offset()
{
	ASSERT(pos_ >= -1 && pos_ < int(v_.size()));
	if (pos_ == -1 ||
		!is_open_view(v_[pos_].pdoc_, v_[pos_].pview_) ||
		GetView() != v_[pos_].pview_)
	{
		return -1;
	}

	FILE_ADDRESS start, end;
	v_[pos_].pview_->GetSelAddr(start, end);
	if (start != v_[pos_].start_addr_ || end != v_[pos_].end_addr_)
		return 0;
	else
		return -1;
}

// Is the specified doc/view still around
bool CNavManager::is_open_view(CHexEditDoc *pdoc, CHexEditView *pview)
{
	// First try to find the doc
	POSITION posd = theApp.m_pDocTemplate->GetFirstDocPosition();
	while (posd != NULL)
	{
		if (pdoc == dynamic_cast<CHexEditDoc *>(theApp.m_pDocTemplate->GetNextDoc(posd)))
		{
			// Doc still open but view may not be
			// Found the doc - now try to find the view
			POSITION posv = pdoc->GetFirstViewPosition();
			while (posv != NULL)
			{
				CView *pv2 = pdoc->GetNextView(posv);
				if (pv2->IsKindOf(RUNTIME_CLASS(CHexEditView)) && pview == (CHexEditView *)pv2)
				{
					return true;  // found!
				}
			}
			return false;   // view not found
		}
	}
	return false;    // doc not found
}

void CNavManager::do_move(int ii)
{
	in_move_ = true;

	// Detect problems but handle them in release too (defensive programming)
	ASSERT(ii >= 0 && ii < int(v_.size()));
	if (ii < 0)
		ii = 0;
	else if (ii >= int(v_.size()))
		ii = v_.size() - 1;

    CHexEditView *pview = NULL;
    CHexEditDoc *pdoc = NULL;

	if (is_open_view(v_[ii].pdoc_, v_[ii].pview_))
	{
		pdoc = v_[ii].pdoc_;
		pview = v_[ii].pview_;
	}
	else if (!v_[ii].fname_.IsEmpty() &&
		     (pdoc = (CHexEditDoc*)(theApp.OpenDocumentFile(v_[ii].fname_))) != NULL)  // reopen the file
    {
        // Use the first view of the correct type
        POSITION pos = pdoc->GetFirstViewPosition();
        while (pos != NULL)
        {
            CView *pv2 = pdoc->GetNextView(pos);
            if (pv2->IsKindOf(RUNTIME_CLASS(CHexEditView)))
            {
				pview = (CHexEditView *)pv2;
                break;
            }
        }
		// Update all list entries for this file with the newly open doc/view
		CHexEditDoc *pclosed = v_[ii].pdoc_;  // Remember ptr to (now closed) document
		for (std::vector<nav>::iterator pd = v_.begin(); pd != v_.end(); ++pd)
		{
			// If this entry is for the newly opened file update its doc/view ptrs
			if (pd->pdoc_ == pclosed)
			{
				pd->pdoc_ = pdoc;
				pd->pview_ = pview;
			}
		}
    }

	// Can't find it so display an error
	if (pview == NULL)
	{
        theApp.mac_error_ = 2;

        CString ss;ss = "The location/file could not found";
		if (!v_[ii].fname_.IsEmpty())
			ss.Format("%s\rcould not be found/opened.", v_[ii].fname_);
		else
			ss = "The location/file could not found";
        AfxMessageBox(ss);
		in_move_ = false;
		return;
    }

	// Now activate the view and move to the location
    CChildFrame *pf = pview->GetFrame();
    if (pf->IsIconic())
        pf->MDIRestore();
    ((CMainFrame *)AfxGetMainWnd())->MDIActivate(pf);
	CPointAp scroll = pview->addr2pos(v_[ii].scroll_);
	scroll.x = 0;
	pview->SetScroll(scroll);
    pview->MoveToAddress(v_[ii].start_addr_, v_[ii].end_addr_);
	pos_ = ii;                          // Set current point to where we just moved to
	in_move_ = false;
    TRACE("xxx MOVE\r\n");
}
