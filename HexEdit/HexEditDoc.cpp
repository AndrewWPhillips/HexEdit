// HexEditDoc.cpp : implementation of the CHexEditDoc class
//
// Copyright (c) 1998-2010 by Andrew W. Phillips. 
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

#include <algorithm>
#include <stdio.h>                              // for rename() and remove()
#include <io.h>                                 // for _access()

#include "HexEditDoc.h"
#include "HexEditView.h"
#include "HexFileList.h"
#include "NewFile.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "Bookmark.h"
#include "boyer.h"
#include "misc.h"
#include "Dialog.h"
#include "SpecialList.h"
#include "SystemSound.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef max
#undef max
#undef min
#endif

extern CHexEditApp theApp;

// I can't work out how to get constants into a header file
const FILE_ADDRESS doc_loc::mask  = 0x3fffFFFFffffFFFF;       // Masks top bits of dlen 
const FILE_ADDRESS doc_loc::fmask = 0x3fffFFFFffffFFFF;       // Masks off the top bits of fileaddr
const int doc_loc::max_data_files = 4;

/////////////////////////////////////////////////////////////////////////////
// CHexEditDoc

IMPLEMENT_DYNCREATE(CHexEditDoc, CDocument)

BEGIN_MESSAGE_MAP(CHexEditDoc, CDocument)
		//{{AFX_MSG_MAP(CHexEditDoc)
	ON_COMMAND(ID_DOCTEST, OnDocTest)
	ON_COMMAND(ID_KEEP_TIMES, OnKeepTimes)
	ON_UPDATE_COMMAND_UI(ID_KEEP_TIMES, OnUpdateKeepTimes)
	ON_COMMAND(ID_DFFD_REFRESH, OnDffdRefresh)
	ON_UPDATE_COMMAND_UI(ID_DFFD_REFRESH, OnUpdateDffdRefresh)
	//}}AFX_MSG_MAP
		ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
		ON_COMMAND(ID_FILE_SAVE, OnFileSave)
		ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)

	ON_COMMAND(ID_DFFD_NEW, OnDffdNew)
	ON_COMMAND_RANGE(ID_DFFD_OPEN_FIRST, ID_DFFD_OPEN_FIRST+DFFD_RESERVED-1, OnDffdOpen)
	ON_COMMAND(ID_DFFD_SAVE, OnDffdSave)
	ON_UPDATE_COMMAND_UI(ID_DFFD_SAVE, OnUpdateDffdSave)
	ON_COMMAND(ID_DFFD_SAVEAS, OnDffdSaveAs)
	ON_UPDATE_COMMAND_UI(ID_DFFD_SAVEAS, OnUpdateDffdSave)

	ON_COMMAND(ID_DFFD_TOGGLE_EDIT, OnEditMode)
	ON_UPDATE_COMMAND_UI(ID_DFFD_TOGGLE_EDIT, OnUpdateEditMode)
	ON_COMMAND(ID_DFFD_OPTIONS, OnDffdOptions)
	ON_UPDATE_COMMAND_UI(ID_DFFD_OPTIONS, OnUpdateDffdOptions)
	ON_COMMAND(ID_COMP_NEW, OnCompNew)

	ON_COMMAND(ID_OPEN_IN_EXPLORER, OnOpenInExplorer)
	ON_UPDATE_COMMAND_UI(ID_OPEN_IN_EXPLORER, OnUpdateOpenInExplorer)
	ON_COMMAND(ID_COPY_FULL_NAME, OnCopyFullName)
	ON_UPDATE_COMMAND_UI(ID_COPY_FULL_NAME, OnUpdateCopyFullName)

	ON_COMMAND(ID_TEST2, OnTest)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexEditDoc construction/destruction

CHexEditDoc::CHexEditDoc()
 : start_search_event_(FALSE, TRUE), search_buf_(NULL),
   start_aerial_event_(FALSE, TRUE), aerial_buf_(NULL),
   start_comp_event_  (FALSE, TRUE), comp_bufa_(NULL), comp_bufb_(NULL),
   stats_buf_(NULL), c32_(NULL), c64_(NULL)
{
	doc_changed_ = false;

	pfile1_ = pfile2_ = pfile3_ = pfile5_ = NULL;
	pfile1_compare_ = pfile4_ = pfile4_compare_ = NULL;  // Files used for compares

	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		data_file_[ii] = NULL;
		data_file2_[ii] = NULL;
		data_file3_[ii] = NULL;
		data_file4_[ii] = NULL;
		data_file5_[ii] = NULL;
		temp_file_[ii] = FALSE;
	}

	keep_times_ = theApp.open_keep_times_;
	length_ = 0L;

	dffd_edit_mode_ = 0;

	// BG search thread
	pthread2_ = NULL;

	// Aerial view thread
	pthread3_ = NULL;
	av_count_ = 0;
	dib_ = NULL;
	bpe_ = -1;

	// BG compare thread
	pthread4_ = NULL;
	cv_count_ = 0;
	bCompSelf_ = false;

	// BG stats thread
	pthread5_ = NULL;

	// Template
	ptree_ = NULL;         // XML tree wrapper for data format view
	df_init_ = FALSE;
	update_needed_ = false;

	hicon_ = HICON(0);

	// Background threads' flags
	search_fin_ = false; clear_found_ = false;
	aerial_fin_ = false;
	comp_fin_   = false;
#ifndef NDEBUG
	// Make default capacity for undo_ vector small to force reallocation sooner.
	// This increases likelihood of catching bugs related to reallocation.
	undo_.reserve(2);
#else
	// Pre-allocate room for 128 elts for initial speed
	undo_.reserve(128);
#endif
	xml_file_num_ = -1;
	need_change_track_ = false;
	base_type_ = 0;

	base_addr_ = 0;
}

CHexEditDoc::~CHexEditDoc()
{
	if (ptree_ != NULL)
	{
		delete ptree_;
		ptree_ = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditDoc diagnostics

#ifdef _DEBUG
void CHexEditDoc::AssertValid() const
{
		CDocument::AssertValid();
}

void CHexEditDoc::Dump(CDumpContext& dc) const
{
	if (pfile1_ != NULL)
	{
		dc << "\nFILE = " << pfile1_->GetFilePath();
		if (readonly_) dc << " (READ ONLY)";
	}
	else
		dc << "\n(No file is open)";
	dc << "\nLENGTH = " << long(length_);

	if (last_view_ != NULL)
	{
		CString ss;
		last_view_->GetWindowText(ss);
		dc << "\nLast change by: " << ss;
	}

	dc << "\nUNDO INFO";
		std::vector <doc_undo>::const_iterator pu;
	for (pu = undo_.begin(); pu != undo_.end(); ++pu)
	{
		dc << "\n  ";
		switch ((*pu).utype)
		{
		case mod_insert :     dc << "INSERT      "; break;
		case mod_insert_file: dc << "INSERT FILE "; break;
		case mod_replace:     dc << "REPLACE     "; break;
		case mod_repback:     dc << "REPLACE BACK"; break;
		case mod_delforw:     dc << "DEL FORWARD "; break;
		case mod_delback:     dc << "DEL BACK    "; break;
		default:              dc << "UNKNOWN TYPE"; break;
		}
		dc << " @" << long((*pu).address);
		dc << " len:" << long((*pu).len);
	}

	dc << "\nLOCATION INFO";
		std::list <doc_loc>::const_iterator pl;
	for (pl = loc_.begin(); pl != loc_.end(); ++pl)
	{
		dc << "\n  ";
		switch (pl->dlen >> 62)
		{
		case 3: dc << "DATA FILE"; break;
		case 2: dc << "MEMORY"; break;
		case 1: dc << "FILE  "; break;
		default:       dc << "??????"; break;
		}
		dc << " len = " << long(pl->dlen&doc_loc::mask);
	}

	dc << "\n";

	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHexEditDoc commands

// Called indirectly as part of File/New command
BOOL CHexEditDoc::OnNewDocument()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (!CDocument::OnNewDocument())
	{
		aa->mac_error_ = 20;
		return FALSE;
	}

	DeleteContents();
	SetModifiedFlag(FALSE);
	readonly_ = shared_ = FALSE;
	length_ = 0L;
	last_view_ = NULL;
	edit_time_.reset(true);

	if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_focus)
	{
		// We don't want focus change recorded (see CHexEditView::OnSetFocus)
		theApp.mac_.pop_back();
	}
	if (!theApp.no_ask_insert_)
	{
		if (!theApp.playing_)
		{
			if (!ask_insert())
				return FALSE;   // User cancelled or some error
		}
		else
		{
			CString ss = theApp.last_fill_str_;
			__int64 state = theApp.last_fill_state_;
			// This was for backward compatibility but a value of zero is valid (length 0 file)
			//if (state == 0)
			//{
			//	state = 0x0000200000000001;
			//	ss = "0";
			//}
			if (insert_block(0, state, ss) == -1)
				return FALSE;       // Some error
		}
	}

	if (pthread2_ == NULL && CanDoSearch())
	{
		CreateSearchThread();
		if (theApp.pboyer_ != NULL)        // If a search is active start a bg search on the newly opened file
			StartSearch();
	}
	if (pthread5_ == NULL && CanDoStats())
	{
		CreateStatsThread();
		StartStats();
	}

	OpenDataFormatFile("default");

//    CHexEditView *pv = GetBestView();
//    if (start_ebcdic)
//        pv->OnCharsetEbcdic();

	return TRUE;
}

void CHexEditDoc::GetFileStatus()
{
	ASSERT(pfile1_ != NULL && !IsDevice());
	VERIFY(pfile1_->GetStatus(saved_status_));
}

void CHexEditDoc::SetFileStatus(LPCTSTR lpszPathName)
{
	// Note: any fields of CFileStatus that are zero are not changed, at least
	// according to the documentation -- but actually this only applies to the
	// file times (attributes are always set and size/name are never set).
	saved_status_.m_size = 0;                   // We don't want to set the file length
	saved_status_.m_szFullName[0] = '\0';       // We don't want to change the file name
	CFile::SetStatus(lpszPathName, saved_status_);
}

// Called indirectly as part of File/Open command
BOOL CHexEditDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	int ii;
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	DeleteContents();
	SetModifiedFlag(FALSE);

#ifdef _DEBUG
	// Make sure no data files are open yet
	for (ii = 0; ii < doc_loc::max_data_files; ++ii)
		ASSERT(data_file2_[ii] == NULL &&
		       data_file3_[ii] == NULL &&
		       data_file4_[ii] == NULL &&
			   data_file5_[ii] == NULL &&
			   data_file_[ii] == NULL );
#endif

	// Get read-only flag from app (this was the only way to pass it here)
	readonly_ = theApp.open_current_readonly_ == -1 ? FALSE : theApp.open_current_readonly_;
	shared_   = theApp.open_file_shared_;
	theApp.open_current_readonly_ = -1;        // Set back to -1 to check that it's properly next time too.

	if (!open_file(lpszPathName))
		return FALSE;               // open_file has already set mac_error_ = 10

	// Save file status (times, attributes) so we can restore them if the "keep times" option is on
	if (!::IsDevice(lpszPathName))
		GetFileStatus();

	length_ = pfile1_->GetLength();
	last_view_ = NULL;

	// Init locations list with all of original file as only loc record
	ASSERT(pthread2_ == NULL);       // Must modify loc_ before creating thread (else docdata_ needs to be locked)
	loc_.push_back(doc_loc(FILE_ADDRESS(0), pfile1_->GetLength()));

	CHECK_SECURITY(195);

	load_icon(lpszPathName);
	show_icon();

	// Get state of this file when last closed (if in recent file list)
	CHexFileList *pfl = theApp.GetFileList();
	int recent_file_index = -1;
	if ((recent_file_index = pfl->GetIndex(lpszPathName)) != -1)
	{
		int flags = atoi(pfl->GetData(recent_file_index, CHexFileList::DOC_FLAGS));
		keep_times_ = (flags & 0x1) != 0;
		dffd_edit_mode_ = (flags & 0x2) != 0;
		view_time_ = timer(atof(pfl->GetData(recent_file_index, CHexFileList::VIEW_TIME)));
		edit_time_ = timer(atof(pfl->GetData(recent_file_index, CHexFileList::EDIT_TIME)));
	}

	if (CanDoSearch())
	{
		CreateSearchThread();
		if (theApp.pboyer_ != NULL)        // If a search is active start a bg search on the newly opened file
		{
			if (theApp.align_rel_ && recent_file_index != -1)
			{
				// we can't use GetSearchBase until the view has been opened
				//base_addr_ = _atoi64(pfl->GetData(recent_file_index, CHexFileList::SELSTART));
				base_addr_ = _atoi64(pfl->GetData(recent_file_index, CHexFileList::MARK));
			}
			StartSearch();
		}
	}
	if (CanDoStats())
	{
		CreateStatsThread();
		StartStats();
	}

	// Keep track of time viewing/editing the file
	if (readonly_)
		view_time_.restart();
	else
		edit_time_.restart();

	// Get all bookmarks for this file
	ASSERT(pfile1_ != NULL);
	CBookmarkList *pbl = theApp.GetBookmarkList();
	pbl->GetAll(pfile1_->GetFilePath(), bm_index_, bm_posn_);

	// Check that the bookmarks are valid
	CString mess;
	int bad_count = 0;
	for (ii = 0; ii < (int)bm_index_.size(); ++ii)
	{
		if (bm_posn_[ii]< 0 || bm_posn_[ii] > length_)
		{
			ASSERT(bm_index_[ii] < (int)pbl->file_.size());
			mess += pbl->name_[bm_index_[ii]] + CString("\n");
			bm_posn_[ii] = length_;
			pbl->filepos_[bm_index_[ii]] = length_;
			++bad_count;
		}
	}
	ASSERT(bad_count == 0 || !mess.IsEmpty());
	if (bad_count == 1)
		CAvoidableDialog::Show(IDS_BOOKMARKS_ADJUSTED, 
			"This file had an invalid bookmark: " + mess +
			"\nThe bookmark has been set to the end of file.");
	else if (bad_count > 1)
		CAvoidableDialog::Show(IDS_BOOKMARKS_ADJUSTED, 
			"The following bookmarks were invalid "
			"and were moved to the end of file:\n" + mess);
	((CMainFrame *)AfxGetMainWnd())->StatusBarText("Invalid bookmarks moved to EOF");

	// Load XML file that is used to display the format of the data
	// Note we load the template when the file is opened rather than if/when the template is needed:
	// + delay between opening and using XML file is good for performance
	// + we can check for "allow_editing" attribute so that menu item state can be set
	// - may slow down file opening for no purpose if template is not used
	// - user might get XML parse error messages even if not using templates
	OpenDataFormatFile();

	// Make sure file property page is updated
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.Update(GetBestView(), -1);

	return TRUE;
}

// Called when document closed probably as part of file closing command
void CHexEditDoc::OnCloseDocument()
{
	CheckSaveTemplate();

	// Store options for this file associated with the 1st CHexEditView for this doc
	// Note that this is done after saving the template file above so that the correct template
	// file is saved (in StoreOptions) if the template file has been saved to another name (Save As).
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pview = GetNextView(pos);
		if (pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
		{
			((CHexEditView *)pview)->StoreOptions();
			break;
		}
	}

	DeleteContents();

	if (hicon_ != HICON(0))
	{
		DestroyIcon(hicon_);
		hicon_ = HICON(0);
	}

	CDocument::OnCloseDocument();
}

void CHexEditDoc::load_icon(LPCTSTR lpszPathName)
{
	SHFILEINFO sfi;                     // Used with SHGetFileInfo to get small icon

	strTypeName_ = "None";              // Default to no extension

	// Destroy current icon if there is one
	if (hicon_ != HICON(0))
	{
		DestroyIcon(hicon_);
		hicon_ = HICON(0);
	}

	// Check if file has an extension
	if (strrchr(lpszPathName, '.') != NULL && !::IsDevice(lpszPathName))
	{
		// Get associated icon
		VERIFY(SHGetFileInfo(lpszPathName, 0, &sfi, sizeof(sfi),
							 SHGFI_ICON | SHGFI_SMALLICON | SHGFI_TYPENAME ));
		hicon_ = sfi.hIcon;
		strTypeName_ = sfi.szTypeName;          // Store file type for display in file page

		// xxx We prob should get the large icon here too to display in the file prop page
	}

	if (hicon_ == HICON(0))
		hicon_ = (HICON)::LoadImage(AfxGetInstanceHandle(),
									MAKEINTRESOURCE(IDR_HEXEDTYPE),
									IMAGE_ICON, 16, 16, 0);
#if 0
		// Get associated icon
		HINSTANCE hInstIcon=AfxFindResourceHandle(MAKEINTRESOURCE(nIDResource),RT_GROUP_ICON);
		m_hIcon = (HICON) LoadImage( hInstIcon, MAKEINTRESOURCE( nIDResource ), IMAGE_ICON, 16, 16,0 );
//        hicon_ = AfxGetApp()->LoadIcon(IDR_HEXEDTYPE);
		VERIFY(::SHGetFileInfo(__argv[0], 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON));
		hicon_ = sfi.hIcon;
#endif
}

void CHexEditDoc::show_icon()
{
	if (int(hicon_) != 0)
	{
		// For each view
		POSITION pos = GetFirstViewPosition();
		while (pos != NULL)
		{
			CView *pview = GetNextView(pos);

			// Get child frame and set icon of it
			CWnd *cc = pview->GetParent();
			while (cc != NULL && !cc->IsKindOf(RUNTIME_CLASS(CChildFrame)))  // view may be in splitter(s)
				cc = cc->GetParent();
			ASSERT_KINDOF(CChildFrame, cc);

			cc->SetIcon(hicon_, FALSE);
		}
	}
}

// Handles File/Close command
void CHexEditDoc::OnFileClose()
{
	// Note: no need to close the file here as CDocument::OnFileClose() calls DeleteContents()
	CDocument::OnFileClose();
	if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_focus)
	{
		// We don't want focus change recorded (see CHexEditView::OnSetFocus)
		theApp.mac_.pop_back();
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_close);

	CHECK_SECURITY(196);
}

BOOL CHexEditDoc::SaveModified()
{
	BOOL retval = CDocument::SaveModified();

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (!retval) aa->mac_error_ = 2;
	return retval;
}

// Save command
void CHexEditDoc::OnFileSave()
{
	if (IsDevice())
		OnSaveDocument(m_strPathName);  // Bypass file inappropriate file stuff for device
	else
		CDocument::OnFileSave();
	CHECK_SECURITY(199);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_save);

	// Make sure file property page is updated
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.Update(GetBestView(), -1);
}

// Save As command
void CHexEditDoc::OnFileSaveAs()
{
	CDocument::OnFileSaveAs();
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_saveas);
	CHECK_SECURITY(197);

	load_icon(GetPathName());
	show_icon();

	// Make sure file property page is updated
	((CMainFrame *)AfxGetMainWnd())->m_wndProp.Update(GetBestView(), -1);
}

// Called as part of Save and Save As commands
BOOL CHexEditDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
	BOOL modified = IsModified();
	if (modified) SetModifiedFlag(FALSE);                    // This gets rid of " *" from default filename
	BOOL retval = CDocument::DoSave(lpszPathName, bReplace);
	if (!retval && modified) SetModifiedFlag();              // If not saved restore modified status

	if (!retval && theApp.mac_error_ < 2) theApp.mac_error_ = 2;
	return retval;
}

// Called as part of Save and Save As commands
BOOL CHexEditDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	last_view_ = NULL;
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (aa->recording_)
	{
		if (TaskMessageBox("Undo Information will be lost",
			              "Saving the file to disk will lose all "
						  "undo information during macro playback."
						  "Do you want to continue?", MB_YESNO) != IDYES)
		{
			aa->mac_error_ = 10;
			return TRUE;
		}
		else
			aa->mac_error_ = 1;
	}

	// Backup the file if options say to (but don't backup drive/volumes)
	// Only backup if: already open file (eg not save as), not a device, and options say to do backup
	BOOL do_backup = pfile1_ != NULL &&
					 lpszPathName == pfile1_->GetFilePath() &&
					 !IsDevice() && 
					 aa->backup(lpszPathName, length_);

#ifdef INPLACE_MOVE    // In place save does not require only overtype changes
	if (!do_backup && pfile1_ != NULL && lpszPathName == pfile1_->GetFilePath())
#else
	if (!do_backup && only_over() && lpszPathName == pfile1_->GetFilePath())
#endif
	{
		// Just write changes back to file in place
		// (avoid worries about disk space, creating temp files etc)
		ASSERT(base_type_ == 0);
		WriteInPlace();

		// Are we keeping the old times/attributes of the file
		if (keep_times_ && !IsDevice())
		{
			// We must close the file so we can set some attributes
			pfile1_->Close();
			SetFileStatus(lpszPathName);
			if (!open_file(lpszPathName))
				return FALSE;                       // already done: mac_error_ = 10
		}
		else if (!IsDevice())
			GetFileStatus();                        // make sure we have current times as on disk
	}
	else
	{
		ASSERT(!IsDevice());
		CString temp_name(lpszPathName);        // Name of saved file
		CString backup_name;                    // Name of backup file
		int path_len;                           // Length of path part of file name

		// Since we may change the file being accessed and the location data (loc_) we need to stop
		// the bg search from accessing it during this time.  (Auto Unlock when it goes out of scope)
		CSingleLock sl(&docdata_, TRUE);

		// Create temp file name
		if ( (path_len = temp_name.ReverseFind('\\')) != -1 ||
			 (path_len = temp_name.ReverseFind(':')) != -1 )
			temp_name = temp_name.Left(path_len+1);
		else
			temp_name.Empty();
		temp_name += "~HxEd";

		int ii;
		for (ii = 0; ii < 1000; ++ii)
		{
			CString tt;
			tt.Format("%03d", ii);
			if (_access(temp_name + tt + ".TMP", 0) < 0)
			{
				// File does not exist so use this file name
				temp_name += tt;
				temp_name += ".TMP";
				break;
			}
		}
		if (ii >= 1000)
		{
			aa->mac_error_ = 20;
			return FALSE;
		}

		// Save current file to temp file
		if (!WriteData(temp_name, 0, length_))
			return FALSE;                       // already done: mac_error_ = 10

		// If we have a file open then close it
		close_file();

		// If file we are writing exists (Save or SaveAs overwriting another file)
		if (_access(lpszPathName, 0) != -1)
		{
			if (do_backup)
			{
				int ext_len;
				backup_name = lpszPathName;

				// Make old file the backup file
				if ((ext_len = backup_name.ReverseFind('.')) > path_len)
					backup_name = backup_name.Left(ext_len);

				if (_stricmp(lpszPathName, (const char *)(backup_name + ".BAK")) == 0)
				{
					// We're editing .BAK so save old as .BAC
					backup_name += ".BAC";
					CString mess;
					mess.Format("Creating backup file\n \"%s\"",
						(const char *)backup_name);
					if (AfxMessageBox(mess, MB_OKCANCEL) == IDCANCEL)
					{
						// User doesn't want to wipe out .BAC
						(void)remove(temp_name);
						(void)open_file(lpszPathName);
						aa->mac_error_ = 2;
						return FALSE;
					}
				}
				else
					backup_name += ".BAK";

				// Remove previous backup file if present
				if (_access(backup_name, 0) != -1)
					if (remove(backup_name) == -1)
					{
						(void)remove(temp_name);
						(void)open_file(lpszPathName);

						CString mess;
						mess.Format("Could not remove previous backup file\n \"%s\"",
							(const char *)backup_name);
						AfxMessageBox(mess);
						aa->mac_error_ = 10;
						return FALSE;
					}

				// Rename the old file to the backup file name
				if (rename(lpszPathName, backup_name) == -1)
				{
					(void)remove(temp_name);
					(void)open_file(lpszPathName);              // Reopen orig. file

					CString mess;
					mess.Format("Could not create backup file\n \"%s\"",
						(const char *)backup_name);
					AfxMessageBox(mess);
					aa->mac_error_ = 10;
					return FALSE;
				}
			}
			else
			{
				// Delete the file.  Note if this was due to user selecting File-SaveAs
				// to an existing file then the user should have already been prompted
				// if he wants to overwrite by the File-Save common dialog.
				if (remove(lpszPathName) == -1)
				{
					(void)remove(temp_name);
					(void)open_file(lpszPathName);              // Reopen orig. file

					CString mess;
					mess.Format("Old file \"%s\" could not be removed", lpszPathName);
					AfxMessageBox(mess);
					aa->mac_error_ = 10;
					return FALSE;
				}
			}
		}

		// Rename the temp file to the real file name
		if (rename(temp_name, lpszPathName) == -1)
		{
			// This should not happen (since temp_name must exist and lpszPathName
			// was successfully removed if it existed) but allow for it anyway

			// Do we have a backup of the previous file?
			if (!backup_name.IsEmpty())
			{
				// Reopen the old file (keep undo info etc) - this allows the
				// user to possibly save the file as something else
				if (rename(backup_name, lpszPathName) != -1)
					(void)open_file(lpszPathName);
				else
					(void)open_file(backup_name);       // Open with backup name
				(void)remove(temp_name);
			}
			else
			{
				// As a last resort open the new file (with wrong name)
				(void)open_file(temp_name);

				// But now undo info, length_ etc are incompatible
				length_ = pfile1_->GetLength();
				// Remove all undo info and just use all of new file as only loc record
				CRemoveHint rh(undo_.size() - 1);
				UpdateAllViews(NULL, 0, &rh);
				undo_.clear();
				loc_.clear();
				loc_.push_back(doc_loc(FILE_ADDRESS(0), length_));
				// Reset change tracking
				replace_pair_.clear();
				insert_pair_.clear();
				delete_pair_.clear();
				need_change_track_ = false;            // Signal that rebuild not required
				base_type_ = 0;                        // Now we can use the saved file as base for compare
				SetModifiedFlag(FALSE);
			}

			CString mess;
			mess.Format("File \"%s\" could not be renamed\n"
						"  to \"%s\"", temp_name, lpszPathName);
			AfxMessageBox(mess);
			aa->mac_error_ = 10;
			return FALSE;
		}

		if (keep_times_ && !::IsDevice(lpszPathName))
			SetFileStatus(lpszPathName);

		base_type_ = 0;                         // Use new file as base for change tracking

		// Open the new file as the document file (pfile1_).
		if (!open_file(lpszPathName))
			return FALSE;                       // already done: mac_error_ = 10

		// Make sure we have latest file times (unless we have just set them to what they were before)
		if (!keep_times_ && !::IsDevice(lpszPathName))
			GetFileStatus();
	}

	length_ = pfile1_->GetLength();

	// Notify views to remove undo info up to the last doc undo
	CRemoveHint rh(undo_.size() - 1);
	UpdateAllViews(NULL, 0, &rh);

	// Remove all undo info and just use all of new file as only loc record
	undo_.clear();
	loc_.clear();
	loc_.push_back(doc_loc(FILE_ADDRESS(0), length_));

	int ii;
	// If we have data files open then close them
	for (ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file_[ii] != NULL)
		{
			CString ss = data_file_[ii]->CFile64::GetFilePath();  // save the file name in case we need to delete it
			data_file_[ii]->Close();
			delete data_file_[ii];
			data_file_[ii] = NULL;
			if (pthread2_ != NULL)
			{
				ASSERT(data_file2_[ii] != NULL);
				data_file2_[ii]->Close();
				delete data_file2_[ii];
				data_file2_[ii] = NULL;
			}
			if (pthread3_ != NULL)
			{
				ASSERT(data_file3_[ii] != NULL);
				data_file3_[ii]->Close();
				delete data_file3_[ii];
				data_file3_[ii] = NULL;
			}
			if (pthread4_ != NULL)
			{
				ASSERT(data_file4_[ii] != NULL);
				data_file4_[ii]->Close();
				delete data_file4_[ii];
				data_file4_[ii] = NULL;
			}
			if (pthread5_ != NULL)
			{
				ASSERT(data_file5_[ii] != NULL);
				data_file5_[ii]->Close();
				delete data_file5_[ii];
				data_file5_[ii] = NULL;
			}

			// If the data file was a temp file remove it now it is closed
			if (temp_file_[ii])
				remove(ss);
		}
	}

	// Reset change tracking
	replace_pair_.clear();
	insert_pair_.clear();
	delete_pair_.clear();
	need_change_track_ = false;            // Signal that rebuild not required
	base_type_ = 0;                        // Now we can use the saved file as base for compare

	// Remove all tracking changes from views
	CTrackHint th(0, length_);
	UpdateAllViews(NULL, 0, &th);

	// Update bookmarks
	ASSERT(bm_index_.size() == bm_posn_.size());
	CBookmarkList *pbl = theApp.GetBookmarkList();
	for (ii = 0; ii < (int)bm_index_.size(); ++ii)
	{
		ASSERT(bm_index_[ii] < (int)pbl->filepos_.size());
		pbl->filepos_[bm_index_[ii]] = bm_posn_[ii];
	}

	SetModifiedFlag(FALSE);

	return TRUE;
}

// Get a view for the document: the active view if there is one, or just the first one
CHexEditView *CHexEditDoc::GetBestView()
{
	CView *pbest = NULL;
	CView *pactive = ::GetView();

	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pview = GetNextView(pos);
		if (pview != NULL && pbest == NULL && pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
			pbest = pview;
		else if (pview == pactive)
		{
			pbest = pview;
			break;
		}
	}
	return (CHexEditView *)pbest;
}

void CHexEditDoc::AddBookmark(int index, FILE_ADDRESS pos)
{
	std::vector<int>::iterator pp = std::find(bm_index_.begin(), bm_index_.end(), index);
	if (pp != bm_index_.end())
	{
		// We already have this bookmark so just update it
		bm_posn_[pp-bm_index_.begin()] = pos;
	}
	else
	{
		// Add to the end of the vectors
		bm_index_.push_back(index);
		bm_posn_.push_back(pos);
	}
	CBookmarkHint bmh(pos);
	UpdateAllViews(NULL, 0, &bmh);
}

void CHexEditDoc::RemoveBookmark(int index)
{
	std::vector<int>::iterator pp = std::find(bm_index_.begin(), bm_index_.end(), index);
	if (pp != bm_index_.end())
	{
		size_t idx = pp-bm_index_.begin();    // index into bm_index_ and bm_posn_ (vectors) of the bookmark to delete
		FILE_ADDRESS address = bm_posn_[idx]; // keep file address of the bookmark so we can invalidate it

		// Remove from the doc's bookmark vectors
		ASSERT(bm_index_.size() == bm_posn_.size());
		bm_index_.erase(bm_index_.begin() + idx);
		bm_posn_.erase(bm_posn_.begin() + idx);

		// Invalidate bookmark rect in all views of this doc (redraws without the bookmark colour background)
		CBookmarkHint bmh(address);
		UpdateAllViews(NULL, 0, &bmh);
	}
	else
	{
		ASSERT(0);
	}
}

// Returns bookmark index or -1 if no bookmark there
int CHexEditDoc::GetBookmarkAt(FILE_ADDRESS pos)
{
	std::vector<FILE_ADDRESS>::const_iterator pp = std::find(bm_posn_.begin(), bm_posn_.end(), pos);
	if (pp != bm_posn_.end())
		return bm_index_[pp-bm_posn_.begin()];
	else
		return -1;
}

void CHexEditDoc::DeleteBookmarkList()
{
	bm_index_.clear();
	bm_posn_.clear();
	UpdateAllViews(NULL);
}

void CHexEditDoc::ClearBookmarks()
{
	ASSERT(pfile1_ != NULL);
	CBookmarkList *pbl = theApp.GetBookmarkList();
	pbl->Clear(pfile1_->GetFilePath());
	ASSERT(bm_index_.empty() && bm_posn_.empty());
//    DeleteBookmarkList();
}

void CHexEditDoc::close_file()
{
	// Close file if it was opened successfully
	if (pfile1_ != NULL)
	{
		pfile1_->Close();
		delete pfile1_;
		pfile1_ = NULL;
	}

	if (pthread2_ != NULL && pfile2_ != NULL)
	{
		pfile2_->Close();
		delete pfile2_;
		pfile2_ = NULL;
	}
	if (pthread3_ != NULL && pfile3_ != NULL)
	{
		pfile3_->Close();
		delete pfile3_;
		pfile3_ = NULL;
	}
	if (pthread4_ != NULL && pfile4_ != NULL)
	{
		CloseCompFile();
	}
	if (pthread5_ != NULL && pfile5_ != NULL)
	{
		pfile5_->Close();
		delete pfile5_;
		pfile5_ = NULL;
	}
}

BOOL CHexEditDoc::open_file(LPCTSTR lpszPathName)
{
	ASSERT(pfile1_ == NULL);
	bool is_device = ::IsDevice(lpszPathName) == TRUE;
	if (is_device)
	{
		// Make sure we don't try to open a read only device incorrectly (eg from recent file list)
		CSpecialList *psl = theApp.GetSpecialList();
		int idx = psl->find(lpszPathName);
		if (!readonly_ && idx != -1)
		{
			readonly_ = psl->read_only(idx);
		}
		pfile1_ = new CFileNC();
	}
	else
		pfile1_ = new CFile64();

	ASSERT(base_type_ == 0);  // Base for change tracking should be the file opened

	CFileException fe;

	if (!shared_ && !readonly_ && pfile1_->Open(lpszPathName,
				CFile::modeReadWrite|CFile::shareDenyWrite|CFile::typeBinary, &fe))
	{
		// Opened OK for writing (unshared)
	}
	else if (!readonly_ && pfile1_->Open(lpszPathName,
				CFile::modeReadWrite|CFile::shareDenyNone|CFile::typeBinary, &fe))
	{
		// Opened OK for write (shared)
		if (!shared_)
		{
			// Open for shared (above) failed
			CString mess;
			mess.Format("%s could not be opened for exclusive access.\n\n"
				"It is opened shared (file contents may change).", lpszPathName);
			CAvoidableDialog::Show(IDS_FILE_OPEN_NON_EXCLUSIVE, mess);
			((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: File opened non-exclusive");
			theApp.mac_error_ = 1;
			shared_ = TRUE;
		}
	}
	else if (!shared_ && pfile1_->Open(lpszPathName,
				CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary, &fe) )
	{
		// Opened for read only
		if (!readonly_)
		{
			// Open for write failed but open for read was OK
			CString mess;
			mess.Format("%s is in use or is a read only file.\n\n"
				"It is opened for read only (no changes possible).", lpszPathName);
			CAvoidableDialog::Show(IDS_FILE_OPEN_READ_ONLY, mess);
			((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: File opened read-only");
			theApp.mac_error_ = 1;
			readonly_ = TRUE;
		}
	}
	else if (pfile1_->Open(lpszPathName,
				CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary, &fe) )
	{
		// Opened for read others may write

		// Check if any of the above attempted opens failed and warn user,
		// but don't worry if it's a device.
		if (!is_device && (!readonly_ || !shared_))
		{
			// Open for write failed but open for read was OK
			CString mess;
			mess.Format("%s is in use or is a read only file.\n\n"
				"It is opened for read only (no changes possible) "
				"with shared access (file contents can change).", lpszPathName);
			CAvoidableDialog::Show(IDS_FILE_OPEN_READ_ONLY, mess);
			((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: File opened non-exclusive, read-only");
			theApp.mac_error_ = 1;
		}
		readonly_ = TRUE;
		shared_ = TRUE;
	}
	else
	{
		// Display info about why the open failed
		CString mess(lpszPathName);
		CFileStatus fs;

		switch (fe.m_cause)
		{
		case CFileException::fileNotFound:
			mess += "\ndoes not exist";
			break;
		case CFileException::badPath:
			mess += "\nis an invalid file name or the drive/path does not exist";
			break;
		case CFileException::tooManyOpenFiles:
			mess += "\n- too many files already open";
			break;
		case CFileException::accessDenied:
			if (is_device)
				mess += "\n- you need to run as administrator to access devices";
			else if (!CFile::GetStatus(lpszPathName, fs))
				mess += "\ndoes not exist";
			else
			{
				if (fs.m_attribute & CFile::directory)
					mess += "\nis a directory";
				else if (fs.m_attribute & (CFile::volume|CFile::hidden|CFile::system))
					mess += "\nis a special file";
				else
					mess += "\ncannot be used (reason unknown)";
			}
			break;
		case CFileException::sharingViolation:
		case CFileException::lockViolation:
			mess += "\nis in use";
			break;
		case CFileException::hardIO:
			mess += "\n- hardware error";
			break;
		case 0xDEAD:
			mess += "\nUnrecognised or corrupt volume"
					"\n(you may need to use the corresponding physical device)";
			break;
		default:
			mess += "\ncould not be opened (reason unknown)";
			break;
		}
		//CAvoidableDialog::Show(IDS_FILE_OPEN_ERROR, mess, 0, 0, MAKEINTRESOURCE(IDI_CROSS));
		TaskMessageBox("Open Error", mess, 0, 0, MAKEINTRESOURCE(IDI_CROSS));
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("Error: File not opened");
		theApp.mac_error_ = 10;
		return FALSE;
	}

	// If doing background searches and the newly opened file is not the same
	// as pfile2_ then close pfile2_ and open it as the new file.
	if (pthread2_ != NULL && 
		(pfile2_ == NULL || pfile1_->GetFilePath() != pfile2_->GetFilePath()) )
	{
		if (pfile2_ != NULL)
		{
			pfile2_->Close();
			delete pfile2_;
			pfile2_ = NULL;
		}

		if (IsDevice())
			pfile2_ = new CFileNC();
		else
			pfile2_ = new CFile64();
		if (!pfile2_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("BG search file open failed for %p\n", this);
			return FALSE;
		}
	}
	if (pthread3_ != NULL && 
		(pfile3_ == NULL || pfile1_->GetFilePath() != pfile3_->GetFilePath()) )
	{
		if (pfile3_ != NULL)
		{
			pfile3_->Close();
			delete pfile3_;
			pfile3_ = NULL;
		}

		if (IsDevice())
			pfile3_ = new CFileNC();
		else
			pfile3_ = new CFile64();
		if (!pfile3_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("BG aerial scan file open failed for %p\n", this);
			return FALSE;
		}
	}
	if (pthread4_ != NULL)
	{
		OpenCompFile();
	}
	if (pthread5_ != NULL && 
		(pfile5_ == NULL || pfile1_->GetFilePath() != pfile5_->GetFilePath()) )
	{
		if (pfile5_ != NULL)
		{
			pfile5_->Close();
			delete pfile5_;
			pfile5_ = NULL;
		}

		if (IsDevice())
			pfile5_ = new CFileNC();
		else
			pfile5_ = new CFile64();
		if (!pfile5_->Open(pfile1_->GetFilePath(),
					CFile::modeRead|CFile::shareDenyNone|CFile::typeBinary) )
		{
			TRACE1("BG Stats file open failed for %p\n", this);
			return FALSE;
		}
	}

	return TRUE;
}

void CHexEditDoc::DeleteContents()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CString strPath;

	// Close file if it was opened successfully
	if (pfile1_ != NULL)
	{
		strPath = pfile1_->GetFilePath();
		pfile1_->Close();
		delete pfile1_;
		pfile1_ = NULL;
	}

	if (pthread2_ != NULL)
		KillSearchThread();

	// KillAerialThread() and KillCompThread() are called here even though they are also killed when
	// the last view is closed because we need to close the data files (data_file2_ and data_file3_).
	if (pthread3_ != NULL)
		KillAerialThread();
	if (pthread4_ != NULL)
		KillCompThread();

	if (pthread5_ != NULL)
		KillStatsThread();

	undo_.clear();
	loc_.clear();               // Done after thread killed so no docdata_ lock needed
	base_type_ = 0;

	for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
	{
		if (data_file_[ii] != NULL)
		{
			CString ss = data_file_[ii]->CFile64::GetFilePath();  // save the file name in case we need to delete it
			data_file_[ii]->Close();
			delete data_file_[ii];
			data_file_[ii] = NULL;
			// If the data file was a temp file remove it now it is closed
			if (temp_file_[ii])
				remove(ss);
		}
		ASSERT(data_file2_[ii] == NULL);  // should have been closed in KillSearchThread() call
		ASSERT(data_file3_[ii] == NULL);  // should have been closed in KillAerialThread() call
		ASSERT(data_file4_[ii] == NULL);  // should have been closed in KillCompThread() call
		ASSERT(data_file5_[ii] == NULL);  // should have been closed in KillStatsThread() call
	}

	// Reset change tracking
	replace_pair_.clear();
	insert_pair_.clear();
	delete_pair_.clear();
	need_change_track_ = false;            // Signal that rebuild not required
	base_type_ = 0;                        // Now we can use the saved file as base for compare
	length_ = 0L;

	CDocument::DeleteContents();
}

void CHexEditDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU /*=TRUE*/)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU);
// xxx not sure about this
	if (::IsDevice(lpszPathName))
		SetTitle(::DeviceName(lpszPathName));
}

void CHexEditDoc::SetModifiedFlag(BOOL bMod /*=TRUE*/)
{
	if (IsModified() != bMod)
	{
		// Modified status has changed (on or off)
		if (bMod)
			SetTitle(GetTitle() + " *");
		else
		{
			CString title = GetTitle();
			int len = title.GetLength();
			if (len > 1 && title[len-1] == '*')
				SetTitle(title.Left(len-2));
		}
		UpdateFrameCounts();
		CDocument::SetModifiedFlag(bMod);
	}
}

void CHexEditDoc::CheckBGProcessing()
{
	// First check if bg processing needs to be restarted due to file changes
	if (doc_changed_)
	{
		doc_changed_ = false;     // Reset flag for next time
		AerialChange();
		StatsChange();
	}

	// Now check if any bg processing has just finished so we can update the display
	bool search_finished = false;
	bool aerial_finished = false;
	bool comp_finished = false;
	docdata_.Lock();
	search_finished = search_fin_;
	if (search_finished)
	{
		search_fin_ = false;              // Stop further updates
		find_total_ = 0;
	}
	aerial_finished = aerial_fin_;
	aerial_fin_ = false;
	comp_finished = comp_fin_;
	comp_fin_ = false;
	docdata_.Unlock();

	if (search_finished)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Background Search Finished");
#endif
		CBGSearchHint bgsh(TRUE);
		UpdateAllViews(NULL, 0, &bgsh);
	}

	// Check if aerial view bitmap has just finished being built
	if (aerial_finished)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Background Scan Finished");
#endif
		CBGAerialHint bgah;
		UpdateAllViews(NULL, 0, &bgah);
	}

	if (comp_finished)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Background Compare Finished");
#endif
		TRACE("Detected bg compare finished - update\r\n");
		CCompHint cch;
		UpdateAllViews(NULL, 0, &cch);
	}

	// For bg compares we also need to check if the compare file has changed since
	// we last scanned it and if so start a new scan else if we just finished a
	// scan we have to update the views.
	if (!bCompSelf_ && CompFileHasChanged())
	{
		TRACE("oooooooo compare file change detected - start compare\r\n");
		StartComp();
	}
	else if (bCompSelf_ && OrigFileHasChanged())
	{
		TRACE("oooooooo external change to file detected\r\n");
		if (IsCompWaiting())
		{
			TRACE("oooooooo pushing empty compare result\r\n");
			// Previous compare has finished so keep it (if not empty) and add a new one
			docdata_.Lock();
			if (!comp_[0].m_addrA.empty())
				comp_.push_front(CompResult());
			docdata_.Unlock();
		}
		else
			StopComp();   // just stop the compare in progress

		ASSERT(IsCompWaiting());  // the thread cannot be doing anything while files are closed

		// Get new copy of file
		CloseCompFile();
		TRACE("oooooooo copying file to temp\r\n");
		// Remove older temp file since we no longer need it
		if (!tempFileB_.IsEmpty())
		{
			VERIFY(::remove(tempFileB_) == 0);
			tempFileB_.Empty();
		}
		// Move newer temp file to older temp file
		tempFileB_ = tempFileA_;
		tempFileA_.Empty();
		// Make new temp file from current file
		VERIFY(MakeTempFile());
		if (OpenCompFile())
			StartComp();
		GetFileStatus();
	}
}

BOOL CHexEditDoc::HasSectorErrors() const
{
#ifdef _DEBUG
//    return TRUE;
#endif


	CFileNC *pf = dynamic_cast<CFileNC *>(pfile1_);

	if (pf == NULL)
		return FALSE;
	else
		return BOOL(pf->HasError());
}

DWORD CHexEditDoc::SectorError(FILE_ADDRESS sec) const
{
#ifdef _DEBUG
//   return sec%5 == 1 || sec%5 == 2;
#endif

	ASSERT(pfile1_ != NULL);
	CFileNC *pf = dynamic_cast<CFileNC *>(pfile1_);

	if (pf == NULL)
		return NO_ERROR;
	else
		return BOOL(pf->Error(sec));
}

bool CHexEditDoc::ask_insert()
{
	bool retval = false;
	CNewFile dlg;

	if (dlg.DoModal() == IDOK)
	{
		const char *data_str = NULL;
		switch (dlg.fill_.type)
		{
		default:
			ASSERT(0);
			// fall through
		case CNewFile::FILL_HEX:   // count hex digits
			data_str = dlg.fill_hex_value_;
			break;
		case CNewFile::FILL_STRING:   // string
			data_str = dlg.fill_string_value_;
			break;
		case CNewFile::FILL_CLIPBOARD:   // clipboard
			break;
		case CNewFile::FILL_RANDOM:   // random - default to one byte
			data_str = dlg.fill_random_range_;
			break;
		case CNewFile::FILL_NUMBER:   // number - get size from type
			data_str = dlg.fill_number_value_;
			break;
		}
		retval = insert_block(0, dlg.fill_state_, data_str) != -1;
		if (retval)
		{
			theApp.SaveToMacro(km_new, dlg.fill_state_);
			theApp.SaveToMacro(km_new_str, data_str);
		}
	}

	return retval;
}

// Returns length of block inserted or -1 if error
// If pview is NULL then we have been called from OnNewFile, else from OnInsertBlock
FILE_ADDRESS CHexEditDoc::insert_block(FILE_ADDRESS addr, _int64 params, const char *data_str, CView *pview /*=NULL*/)
{
#if 0 // seems to have compiler problems under VC7
	union
	{
		__int64 fill_state;         // 64 bit value that contains the options
		CNewFile::fill_bits fill_bits;
	};
	fill_state = params;
#else
	CNewFile::fill_bits fill_bits;
	*(_int64 *)(&fill_bits) = params;
#endif
	// Work out the length of the data selected/entered
	if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
	{
		if (!::OpenClipboard(AfxGetMainWnd()->m_hWnd))
		{
			// No need to insert a zero length file
			((CMainFrame *)AfxGetMainWnd())->StatusBarText("Could not open clipboard");
			theApp.mac_error_ = 10;
			return -1;
		}
	}
	const char *pcb = NULL;             // Ptr to clipboard data
	size_t data_len = fill_bits.Length(data_str, pcb);          // Length of the data we are using
	if (data_len == 0)
	{
		if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
			::CloseClipboard();
		return 0;     // Note: old macro files that stored 0 as km_new parameter should end up here

	}

	// Work out the length of the new file
	FILE_ADDRESS file_len;
	switch (fill_bits.size_src)
	{
	case 0:      // get user entered value
		file_len = (FILE_ADDRESS(fill_bits.size_high)<<32) + fill_bits.size_low;
		break;
	default:
		ASSERT(0);
		// fall through
	case 1:    // from data
		file_len = data_len * ((FILE_ADDRESS(fill_bits.size_high)<<32) + fill_bits.size_low);
		break;
	case 2:      // get length from calculator
		file_len = FILE_ADDRESS(((CMainFrame *)AfxGetMainWnd())->m_wndCalc.GetValue());
		break;
	}

	if (file_len < 0)
	{
		if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
			::CloseClipboard();
		return -1;
	}
	if (file_len == 0)
	{
		if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
			::CloseClipboard();
		return 0;
	}

	// Work out if data is stored on disk (or in memory) and what the file name is
	bool write_file = false;        // Write to file (user wants to or too big for memory)
	CString file_name;

	if (pview == NULL)
		base_type_ = 1;					// default to making initial memory block the base

	if (fill_bits.create_on_disk)
	{
		// Ask the user for the file name to use
		CHexFileDialog dlgFile("NewFileDlg", HIDD_FILE_NEW, FALSE, NULL, NULL,
							OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
							theApp.GetCurrentFilters(), "Create", AfxGetMainWnd());

		// Set up the title of the dialog
		dlgFile.m_ofn.lpstrTitle = "New File Name";

		if (dlgFile.DoModal() != IDOK)
		{
			theApp.mac_error_ = 2;
			if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
				::CloseClipboard();
			return -1;
		}

		file_name = dlgFile.GetPathName();

		write_file = true;
		ASSERT(pview == NULL);          // create_on_disk option can only be on for new file
		base_type_ = 0;					// base is the file
	}
	else if (file_len > 8*1024*1024)
	{
		// Create a temp file
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);
		file_name = temp_file;

		write_file = true;
		if (pview == NULL)
			base_type_ = 2;					// base is temp file
	}

	// Allocate the block of memory and fill it
	size_t buf_len;
	if (write_file)
	{
		buf_len = (size_t)std::min((int)file_len, 16384); // Make sure we have a big enough buffer that disk writing is not slow
		int num_copies = buf_len/data_len;
		if (num_copies < 1) num_copies = 1;
		buf_len = data_len*num_copies;
	}
	else
	{
		// allocate a block big enough for all the data (to be kept in memory)
		buf_len = size_t(file_len);
	}

	char *srcbuf = NULL;            // Where we put source copy of data if we have to create it
	const char *pp;
	char *pout;
	const char *psrc = NULL;        // Ptr to source data
	char *buf = new char[buf_len];  // Where we put data to be inserted
	CString ss;                     // Holds number without commas
	range_set<int> rand_rs;         // The range(s) of values for random fills

	switch (fill_bits.type)
	{
	default:
		ASSERT(0);
		// fall through
	case CNewFile::FILL_HEX:
		srcbuf = new char[data_len];
		// Get the hex digits into srcbuf
		for (pp = data_str, pout = srcbuf; size_t(pout - srcbuf) < data_len; ++pout)
		{
			while (!isxdigit(*pp) && *pp != '\0')
				++pp;
			if (*pp == '\0')
				break;

			if (isdigit(*pp))
				*pout = *pp - '0';
			else if (isupper(*pp))
				*pout = *pp - 'A' + 10;
			else
				*pout = *pp - 'a' + 10;
			++pp;

			while (!isxdigit(*pp) && *pp != '\0')
				++pp;
			if (*pp == '\0')
			{
				++pout;     // Byte has only onbe digit
				break;
			}

			if (isdigit(*pp))
				*pout = (*pout << 4) | (*pp - '0');
			else if (isupper(*pp))
				*pout = (*pout << 4) | (*pp - 'A' + 10);
			else
				*pout = (*pout << 4) | (*pp - 'a' + 10);
			++pp;
		}
		ASSERT(pout - srcbuf == data_len);

		psrc = srcbuf;
		break;

	case CNewFile::FILL_STRING:
		switch (fill_bits.string_charset)
		{
		default:
			ASSERT(0);
			// fall through
		case 0:                     // ASCII
			psrc = data_str;
			break;
		case 1:                     // EBCDIC
			srcbuf = new char[data_len+1];
			for (pp = data_str, pout = srcbuf; *pp != '\0' && pout < srcbuf + data_len; ++pp)
			{
				if (a2e_tab[(unsigned char)*pp] != '\0')
					*pout++ = a2e_tab[(unsigned char)*pp];
				else
					*pout++ = '\0';
			}
			ASSERT(data_len > 0 && pout == srcbuf + data_len);

			psrc = srcbuf;
			break;
		case 2:                     // Unicode
			srcbuf = new char[data_len+2];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
								data_str, data_len,
								(WCHAR*)srcbuf, (data_len+2)/sizeof(WCHAR));
			psrc = srcbuf;
			break;
		}
		break;

	case CNewFile::FILL_CLIPBOARD:
		psrc = pcb;
		break;

	case CNewFile::FILL_RANDOM:
		{
			std::istringstream str((const char *)(data_str));
			str >> rand_rs;
		}
		break;

	case CNewFile::FILL_NUMBER:
		srcbuf = new char[8];
		ss = data_str;
		ss.Remove(',');

		if (fill_bits.number_is_float)
		{
			switch (fill_bits.number_type)
			{
			default:
				ASSERT(0);
				// fall through
			case 1:                     // 64 bit
				*((double *)srcbuf) = strtod(ss, NULL);
				break;
			case 0:                     // 32 bit
				*((float *)srcbuf) = float(strtod(ss, NULL));
				break;
			}
		}
		else
		{
			switch (fill_bits.number_type)
			{
			default:
				ASSERT(0);
				// fall through
			case 2:                     // 32 bit
				*((long *)srcbuf) = strtol(ss, NULL, 10);
				break;
			case 0:                     // 8 bit
				*((signed char *)srcbuf) = signed char(strtol(ss, NULL, 10));
				break;
			case 1:                     // 16 bit
				*((short *)srcbuf) = short(strtol(ss, NULL, 10));
				break;
			case 3:                     // 64 bit
				*((__int64 *)srcbuf) = _atoi64(ss);
				break;
			}
		}

		if (fill_bits.big_endian)
			flip_bytes((unsigned char *)srcbuf, data_len);

		psrc = srcbuf;
		break;
	}
	ASSERT(psrc != NULL || fill_bits.type == CNewFile::FILL_RANDOM);

	if (psrc != NULL)
	{
		if (data_len == 1)
			memset(buf, *psrc, buf_len);
		else
			for (pout = buf; pout < buf + buf_len; pout += data_len)
				memcpy(pout, psrc, std::min(data_len, buf_len - (pout - buf)));
	}

	// Tidy up temp buffers etc
	if (srcbuf != NULL) delete[] srcbuf;
	if (fill_bits.type == CNewFile::FILL_CLIPBOARD)
	{
		ASSERT(pcb != NULL);
		::CloseClipboard();
	}

	if (write_file)
	{
		CFile64 *pfile = new CFile64();
		CFileException fe;
		if (!pfile->Open(file_name, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary, &fe))
		{
			TaskMessageBox("File Open Error", ::FileErrorMessage(&fe, CFile::modeWrite));
			theApp.mac_error_ = 10;
			return -1;
		}
		CWaitCursor wc;

		// Init progress bar
		CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
		clock_t last_checked = clock();

		// Write out the file
		FILE_ADDRESS num_out;
		FILE_ADDRESS towrite;
		for (num_out = 0; num_out < file_len; num_out += towrite)
		{
			towrite = std::min((FILE_ADDRESS)buf_len, file_len - num_out);
			if (fill_bits.type == CNewFile::FILL_RANDOM)
				fill_rand(buf, size_t(towrite), rand_rs);
			pfile->Write(buf, size_t(towrite));

			// Update scan progress no more than once a second
			if ((clock() - last_checked)/CLOCKS_PER_SEC > 1)
			{
				mm->Progress(int((num_out*100)/file_len));
				last_checked = clock();
			}
		}

		mm->Progress(-1);
		delete pfile;

		if (fill_bits.create_on_disk)
		{
			// Set up the file as the "original" file
			if (!open_file(file_name))
				return -1;              // open_file has already set mac_error_ = 10

			length_ = file_len;
			ASSERT(pthread2_ == NULL && pthread3_ == NULL && pthread4_ == NULL && pthread5_ == NULL);   // Must modify loc_ before creating threads (else docdata_ needs to be locked)
			loc_.push_back(doc_loc(FILE_ADDRESS(0), file_len));

			// Save original status
			GetFileStatus();

			load_icon(file_name);
			show_icon();
			SetPathName(file_name);

			// Make sure file property page is updated
			((CMainFrame *)AfxGetMainWnd())->m_wndProp.Update(GetBestView(), -1);
		}
		else
		{
			// Set up the file as a temp data file
			int idx = AddDataFile(file_name, TRUE);
			if (idx == -1)
			{
				TaskMessageBox("Too Many Temporary Files",
					"To insert a large file HexEdit Pro needs to create a temporary "
					"file but has run out of temporary file handles.\n\n"
					"Please save the file to deallocate "
					"temporary file handles and try again.");
				theApp.mac_error_ = 10;
				return -1;
			}
			Change(mod_insert_file, addr, file_len, NULL, idx, pview);
		}
	}
	else
	{
		// Add memory block
		if (fill_bits.type == CNewFile::FILL_RANDOM)
			fill_rand(buf, size_t(file_len), rand_rs);
		Change(mod_insert, addr, file_len, (unsigned char *)buf, 0, pview);
	}

	delete[] buf;
	return file_len;
} // insert_block

void CHexEditDoc::fill_rand(char *buf, size_t len, range_set<int> &rr)
{
	unsigned char *pp;

	if (rr.range_.begin()->sfirst <= 0 && rr.range_.begin()->slast > 255)
	{
		// All byte values allowed so quickly fill 'er up
		for (pp = (unsigned char *)buf; pp < (unsigned char *)(buf + len); ++pp)
			*pp = (unsigned char)((::rand_good()>>16)&0xFF);
	}
	else
	{
		for (pp = (unsigned char *)buf; pp < (unsigned char *)(buf + len); ++pp)
		{
			// Keep getting random values until we get one in the range we want
			do
			{
				// Get the next random value
				*pp = (unsigned char)((::rand_good()>>16)&0xFF);
			}
			while (rr.find(int(*pp)) == rr.end());
		}
	}
}

void CHexEditDoc::OnKeepTimes()
{
	keep_times_ = !keep_times_;
}

void CHexEditDoc::OnUpdateKeepTimes(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pfile1_ != NULL && !IsDevice() && !readonly_);
	pCmdUI->SetCheck(keep_times_);
}

void CHexEditDoc::OnOpenInExplorer() 
{
	ASSERT(AfxGetMainWnd() != NULL);
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (pfile1_ != NULL && !IsDevice() && mm != NULL)
	{
		mm->m_paneExpl.ShowAndUnroll();
		mm->m_wndExpl.ShowFile(pfile1_->GetFilePath());
	}
}

void CHexEditDoc::OnUpdateOpenInExplorer(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(pfile1_ != NULL && !IsDevice());
}

void CHexEditDoc::OnCopyFullName() 
{
	if (pfile1_ != NULL)
		::StringToClipboard(pfile1_->GetFilePath());
}

void CHexEditDoc::OnUpdateCopyFullName(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(pfile1_ != NULL);
}

void CHexEditDoc::OnDocTest()
{
}

void CHexEditDoc::OnTest()
{
#if 0
	CXmlTree xt;

	if (!xt.LoadString("<?xml version=\"1.0\" ?>\n"
					   "<b>\n"
					   "  <s>\n"
					   "    <d1/>\n"
					   "  </s>\n"
					   "  <d2/>\n"
					   "</b>\n"
					   ))
	{
		CString mess;

		mess.Format("Error: %s\n\nLine: %s", xt.ErrorMessage(), xt.ErrorLineText());
		AfxMessageBox(mess);
	}
	else
	{
		MSXML2::IXMLDOMElementPtr pnew = xt.m_pdoc->createElement(_bstr_t("Elem1"));
		pnew->insertBefore(xt.m_pdoc->createElement(_bstr_t("Elem2")), _variant_t());
		CXmlTree::CElt root = xt.GetRoot();
		CXmlTree::CElt ee = root.GetChild("s");
		CXmlTree::CElt ee2 = root.GetChild("d2");
		ee2.InsertClone(ee, NULL);
//        root.m_pelt->insertBefore(pnew, _variant_t());

		AfxMessageBox(xt.DumpXML());
	}
#endif
}



