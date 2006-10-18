// HexEditDoc.cpp : implementation of the CHexEditDoc class
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
#include "SaveDffd.h"
#include "misc.h"
#include "Dialog.h"
#include "DFFDMisc.h"
#if defined(BG_DEVICE_SEARCH)
#include "SpecialList.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
        ON_COMMAND(ID_TEST2, OnTest)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexEditDoc construction/destruction

CHexEditDoc::CHexEditDoc()
 : start_event_(FALSE, TRUE), stopped_event_(FALSE, TRUE)
{
    pfile1_ = pfile2_ = NULL;

    for (int ii = 0; ii < doc_loc::max_data_files; ++ii)
    {
        data_file_[ii] = NULL;
        data_file2_[ii] = NULL;
        temp_file_[ii] = FALSE;
    }

    keep_times_ = theApp.open_keep_times_;
    length_ = 0L;

    pthread_ = NULL;

    ptree_ = NULL;         // XML tree wrapper for data format view
    df_init_ = FALSE;
    update_needed_ = false;

    hicon_ = HICON(0);

    view_update_ = FALSE;
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
    if (!theApp.playing_)
	{
        if (!ask_insert())
			return FALSE;   // User cancelled or some error
	}
    else
	{
		CString ss = theApp.last_fill_str_;
		__int64 state = theApp.last_fill_state_;
		if (state == 0)
		{
			state = 0x0000200000000001;
			ss = "0";
		}
		if (insert_block(0, state, ss) == -1)
			return FALSE;       // Some error
	}

    if (aa->bg_search_ && pthread_ == NULL)
        CreateThread();

    // Load XML DFFD file (should use "default")
    if (theApp.tree_view_ > 0 && xml_file_num_ == -1)
        OpenDataFormatFile();

//    CHexEditView *pv = GetBestView();
//    if (start_ebcdic)
//        pv->OnCharsetEbcdic();

    return TRUE;
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
        ASSERT(data_file_[ii] == NULL && data_file2_[ii] == NULL);
#endif

    // Get read-only flag from app (this was the only way to pass it here)
	readonly_ = theApp.open_current_readonly_ == -1 ? FALSE : theApp.open_current_readonly_;
	shared_   = theApp.open_file_shared_;
    theApp.open_current_readonly_ = -1;        // Set back to -1 to check that it's properly next time too.

	if (!open_file(lpszPathName))
        return FALSE;               // open_file has already set mac_error_ = 10

    // Save file status (times, attributes) so we can restore them if the "keep times" option is on
    if (!::IsDevice(lpszPathName))
    {
        VERIFY(pfile1_->GetStatus(saved_status_));
        saved_status_.m_size = 0;                   // We don't want to set the file length
        saved_status_.m_szFullName[0] = '\0';       // We don't want to change the file name
    }

    length_ = pfile1_->GetLength();
    last_view_ = NULL;

    // Init locations list with all of original file as only loc record
    ASSERT(pthread_ == NULL);       // Must modify loc_ before creating thread (else docdata_ needs to be locked)
    loc_.push_back(doc_loc(FILE_ADDRESS(0), pfile1_->GetLength()));

    CHECK_SECURITY(195);

    load_icon(lpszPathName);
    show_icon();

    // Get state of this file when last closed (if in recent file list)
    CHexFileList *pfl = theApp.GetFileList();
    int recent_file_index = -1;
    if ((recent_file_index = pfl->GetIndex(lpszPathName)) != -1)
    {
        keep_times_ = BOOL(atoi(pfl->GetData(recent_file_index, CHexFileList::KEEP_TIMES)));
		view_time_ = timer(atof(pfl->GetData(recent_file_index, CHexFileList::VIEW_TIME)));
		edit_time_ = timer(atof(pfl->GetData(recent_file_index, CHexFileList::EDIT_TIME)));
    }

    if (aa->bg_search_)
    {
        CreateThread();
        if (theApp.pboyer_ != NULL)        // If a search has already been done do bg search on newly opened file
		{
			if (theApp.align_mark_ && recent_file_index != -1)
				base_addr_ = _atoi64(pfl->GetData(recent_file_index, CHexFileList::MARK));
            StartSearch();
		}
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
            mess += pbl->name_[bm_index_[ii]] + CString("\r");
            bm_posn_[ii] = length_;
            pbl->filepos_[bm_index_[ii]] = length_;
            ++bad_count;
        }
    }
    ASSERT(bad_count == 0 || !mess.IsEmpty());
    if (bad_count == 1)
        AfxMessageBox(CString("This file has an invalid bookmark: ") + mess +
                      CString("\rThis bookmark has been moved to the end of file."));
    else if (bad_count > 1)
        AfxMessageBox(CString("The following bookmarks were invalid\r"
                              "and were moved to the end of file:\r") + mess);

    // Load XML file that is used to display the format of the data
//    GetXMLFileList();
    OpenDataFormatFile();

    return TRUE;
}

// Called when document closed probably as part of file closing command
void CHexEditDoc::OnCloseDocument() 
{
    // If the template for this file has been modified ask the user if they want to save it
    if (ptree_ != NULL && ptree_->IsModified())
    {
        CString path_name = ptree_->GetFileName();
        
        char fname[_MAX_FNAME], ext[_MAX_EXT];
        _splitpath(path_name, NULL, NULL, fname, ext);

        CString file_name = CString(fname) + ext;

        CSaveDffd dlg;
        dlg.message_.Format("The template \"%s\" has been modified.\n\n"
                            "Do you want to save it?", file_name);

        if (file_name.CompareNoCase("default.xml") == 0)
        {
            if (AfxMessageBox(dlg.message_, MB_YESNO) != IDNO)
            {
                CHexFileDialog dlgFile("TemplateFileDlg", FALSE, "xml", NULL, 
                                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
                                    "Template (XML) files - *.xml)|*.xml|All Files (*.*)|*.*||");

                // Set up the title of the dialog
                dlgFile.m_ofn.lpstrTitle = "Save Template File";
                dlgFile.m_ofn.lpstrInitialDir = theApp.xml_dir_;

                if (dlgFile.DoModal() == IDOK)
                {
                    ptree_->Save(dlgFile.GetPathName());
                    theApp.GetXMLFileList();        // rebuild the list with the new file name in it
                    _splitpath(dlgFile.GetPathName(), NULL, NULL, fname, ext);
                    xml_file_num_ = theApp.FindXMLFile(fname);
                }
            }
        }
        else
        {
            switch (dlg.DoModal())  // IDOK = Save, IDYES = SaveAs, IDCANCEL = Don't save
            {
            case IDOK:
                ptree_->Save();
                break;
            case IDYES:
                CHexFileDialog dlgFile("TemplateFileDlg", FALSE, "xml", path_name, 
                                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
                                    "Template (XML) files - *.xml)|*.xml|All Files (*.*)|*.*||");

                // Set up the title of the dialog
                dlgFile.m_ofn.lpstrTitle = "Save Template File";

                if (dlgFile.DoModal() == IDOK)
                {
                    ptree_->Save(dlgFile.GetPathName());
                    theApp.GetXMLFileList();        // rebuild the list with the new file name in it
                    _splitpath(dlgFile.GetPathName(), NULL, NULL, fname, ext);
                    xml_file_num_ = theApp.FindXMLFile(fname);
                }
                break;
            }
        }
    }

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
    ASSERT(pthread_ == NULL);

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

    // Make sure file property page is updated
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    POSITION pos = GetFirstViewPosition();
    while (pos != NULL)
    {
        CView *pview = GetNextView(pos);
        if (pview->IsKindOf(RUNTIME_CLASS(CBCGTabView)))
            pview = ((CBCGTabView *)pview)->GetActiveView();
        if (pview->IsKindOf(RUNTIME_CLASS(CHexEditView)))
        {
            mm->m_wndProp.m_pSheet->Update((CHexEditView *)pview, -1);
            break;
        }
    }
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

    // BG search finished message may be lost if modeless dlg running
    ((CHexEditApp *)AfxGetApp())->CheckBGSearchFinished();
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
}

// Save As command
void CHexEditDoc::OnFileSaveAs()
{
    CDocument::OnFileSaveAs();
    ((CHexEditApp *)AfxGetApp())->SaveToMacro(km_saveas);
    CHECK_SECURITY(197);

    load_icon(GetPathName());
    show_icon();
}

// Called as part of Save and Save As commands
BOOL CHexEditDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
    BOOL retval = CDocument::DoSave(lpszPathName, bReplace);

    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    if (!retval && aa->mac_error_ < 2) aa->mac_error_ = 2;
    return retval;
}

// Called as part of Save and Save As commands
BOOL CHexEditDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
    last_view_ = NULL;
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    if (aa->recording_)
    {
        if (::HMessageBox("Saving file to disk will lose all \r"
                          "undo information during playback.\r\r"
                          "Do you want to continue?", MB_OKCANCEL) != IDOK)
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
            CFile::SetStatus(lpszPathName, saved_status_);
            if (!open_file(lpszPathName))
                return FALSE;                       // already done: mac_error_ = 10
        }
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
        if (pfile1_ != NULL)
        {
            pfile1_->Close();
            delete pfile1_;
            pfile1_ = NULL;
            if (pthread_ != NULL && pfile2_ != NULL)
            {
                pfile2_->Close();
                delete pfile2_;
                pfile2_ = NULL;
            }
        }
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
                    mess.Format("Creating backup file\r \"%s\"",
                        (const char *)backup_name);
                    if (::HMessageBox(mess, MB_OKCANCEL) == IDCANCEL)
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
                        mess.Format("Could not remove previous backup file\r \"%s\"",
                            (const char *)backup_name);
                        ::HMessageBox(mess);
                        aa->mac_error_ = 10;
                        return FALSE;
                    }

                // Rename the old file to the backup file name
                if (rename(lpszPathName, backup_name) == -1)
                {
                    (void)remove(temp_name);
                    (void)open_file(lpszPathName);              // Reopen orig. file

                    CString mess;
                    mess.Format("Could not create backup file\r \"%s\"",
                        (const char *)backup_name);
                    ::HMessageBox(mess);
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
                    ::HMessageBox(mess);
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
            mess.Format("File \"%s\" could not be renamed\r"
                        "  to \"%s\"", temp_name, lpszPathName);
            ::HMessageBox(mess);
            aa->mac_error_ = 10;
            return FALSE;
        }

		if (keep_times_ && !::IsDevice(lpszPathName))
            CFile::SetStatus(lpszPathName, saved_status_);

        base_type_ = 0;                         // Use new file as base for change tracking

        // Open the new file as the document file (pfile1_).
        if (!open_file(lpszPathName))
            return FALSE;                       // already done: mac_error_ = 10
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
            if (pthread_ != NULL)
            {
                ASSERT(data_file2_[ii] != NULL);
                data_file2_[ii]->Close();
                delete data_file2_[ii];
                data_file2_[ii] = NULL;
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

    // BG search finished message may be lost if modeless dlg running
    ((CHexEditApp *)AfxGetApp())->CheckBGSearchFinished();

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
        bm_index_.erase(pp);
        ASSERT(pp-bm_index_.begin() < (int)bm_posn_.size());
        FILE_ADDRESS address = bm_posn_[pp-bm_index_.begin()];
        bm_posn_.erase(bm_posn_.begin() + (pp-bm_index_.begin()));

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
    std::vector<FILE_ADDRESS>::iterator pp = std::find(bm_posn_.begin(), bm_posn_.end(), pos);
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

    if (theApp.bg_search_ && pthread_ != NULL && pfile2_ != NULL)
    {
        pfile2_->Close();
        delete pfile2_;
        pfile2_ = NULL;
    }
}

BOOL CHexEditDoc::open_file(LPCTSTR lpszPathName)
{
    ASSERT(pfile1_ == NULL);
	bool is_device = ::IsDevice(lpszPathName) == TRUE;
    if (is_device)
	{
#if defined(BG_DEVICE_SEARCH)
		// Make sure we don't try to open a read only device incorrectly (eg from recent file list)
		CSpecialList *psl = theApp.GetSpecialList();
		int idx = psl->find(lpszPathName);
		if (!readonly_ && idx != -1)
		{
			readonly_ = psl->read_only(idx);
		}
#endif
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
            mess.Format("%s could not be opened for exclusive access.\r\r"
                "It is opened shared (file contents may change).", lpszPathName);
            ::HMessageBox(mess);
            CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
            aa->mac_error_ = 1;
			shared_ = TRUE;
		}

		// xxx ask for notification if file changes xxx
    }
    else if (!shared_ && pfile1_->Open(lpszPathName,
                CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary, &fe) )
    {
		// Opened for read only
        if (!readonly_)
        {
            // Open for write failed but open for read was OK
            CString mess;
            mess.Format("%s is in use or is a read only file.\r\r"
                "It is opened for read only (no changes possible).", lpszPathName);
            ::HMessageBox(mess);
            CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
            aa->mac_error_ = 1;
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
            mess.Format("%s is in use or is a read only file.\r\r"
                "It is opened for read only (no changes possible)\r"
				"with shared access (file contents can change).", lpszPathName);
            ::HMessageBox(mess);
            CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
            aa->mac_error_ = 1;
		}
		readonly_ = TRUE;
        shared_ = TRUE;

        // xxx ask for notification if file changes xxx
    }
    else
    {
        // Display info about why the open failed
        CString mess(lpszPathName);
        CFileStatus fs;

        switch (fe.m_cause)
        {
        case CFileException::fileNotFound:
            mess += "\rdoes not exist";
            break;
        case CFileException::badPath:
            mess += "\ris an invalid file name or the drive/path does not exist";
            break;
        case CFileException::tooManyOpenFiles:
            mess += "\r- too many files already open";
            break;
        case CFileException::accessDenied:
            if (!CFile::GetStatus(lpszPathName, fs))
                mess += "\rdoes not exist";
            else
            {
                if (fs.m_attribute & CFile::directory)
                    mess += "\ris a directory";
                else if (fs.m_attribute & (CFile::volume|CFile::hidden|CFile::system))
                    mess += "\ris a special file";
                else
                    mess += "\rcannot be used (reason unknown)";
            }
            break;
        case CFileException::sharingViolation:
        case CFileException::lockViolation:
            mess += "\ris in use";
            break;
        case CFileException::hardIO:
            mess += "\r- hardware error";
            break;
		case 0xDEAD:
			mess += "\rUnrecognised or corrupt volume"
				    "\r(you may need to use the corresponding physical device)";
			break;
        default:
            mess += "\rcould not be opened (reason unknown)";
            break;
        }
        ::HMessageBox(mess);

        CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
        aa->mac_error_ = 10;
        return FALSE;
    }

    // If doing background searches and the newly opened file is not the same
    // as pfile2_ then close pfile2_ and open it as the new file.
    if (pthread_ != NULL && 
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
            TRACE1("File2 open failed for %p\n", this);
            return FALSE;
        }
    }

    return TRUE;
}

void CHexEditDoc::DeleteContents() 
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    // Close file if it was opened successfully
    if (pfile1_ != NULL)
    {
        pfile1_->Close();
        delete pfile1_;
        pfile1_ = NULL;
    }

    if (aa->bg_search_ && pthread_ != NULL)
        KillThread();

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
        ASSERT(data_file2_[ii] == NULL);  // should have been closed in KillThread() call above
    }

    // Reset change tracking
    replace_pair_.clear();
    insert_pair_.clear();
    delete_pair_.clear();
    need_change_track_ = false;            // Signal that rebuild not required
	base_type_ = 0;                        // Now we can use the saved file as base for compare

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
        ((CMainFrame *)AfxGetMainWnd())->StatusBarText("No fill data available");
        theApp.mac_error_ = 2;
		return -1;
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
        return 0;     // Note: old macro files that stored 0 as km_new parameter should end up here
    }

    // Work out if data is stored on disk (or in memory) and what the file name is
    bool write_file = false;        // Write to file (user wants to or too big for memory)
    CString file_name;

    if (pview == NULL)
	    base_type_ = 1;					// default to making initial memory block the base

    if (fill_bits.create_on_disk)
    {
        // Ask the user for the file name to use
        CHexFileDialog dlgFile("NewFileDlg", FALSE, NULL, NULL,
                            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
                            theApp.GetCurrentFilters(), AfxGetMainWnd());

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
        buf_len = (size_t)min(file_len, 16384); // Make sure we have a big enough buffer that disk writing is not slow
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
                memcpy(pout, psrc, min(data_len, buf_len - (pout - buf)));
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
            ::HMessageBox(::FileErrorMessage(&fe, CFile::modeWrite));
            theApp.mac_error_ = 10;
            return -1;
        }
        CWaitCursor wc;

        // Init progress bar
        CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
        mm->m_wndStatusBar.EnablePaneProgressBar(0, 100);
        clock_t last_checked = clock();

        // Write out the file
        FILE_ADDRESS num_out;
        FILE_ADDRESS towrite;
        for (num_out = 0; num_out < file_len; num_out += towrite)
        {
            towrite = min(buf_len, file_len - num_out);
            if (fill_bits.type == CNewFile::FILL_RANDOM)
                fill_rand(buf, size_t(towrite), rand_rs);
            pfile->Write(buf, size_t(towrite));

            // Update scan progress no more than once a second
            if ((clock() - last_checked)/CLOCKS_PER_SEC > 1)
            {
                mm->m_wndStatusBar.SetPaneProgress(0, long((num_out*100)/file_len));
                last_checked = clock();
            }
        }

        mm->m_wndStatusBar.EnablePaneProgressBar(0, -1);
        delete pfile;

        if (fill_bits.create_on_disk)
        {
            // Set up the file as the "original" file
            if (!open_file(file_name))
                return -1;              // open_file has already set mac_error_ = 10

            length_ = file_len;
            ASSERT(pthread_ == NULL);   // Must modify loc_ before creating thread (else docdata_ needs to be locked)
            loc_.push_back(doc_loc(FILE_ADDRESS(0), file_len));

            // Save original status
            VERIFY(pfile1_->GetStatus(saved_status_));
            saved_status_.m_size = 0;                   // We don't want to set the file length
            saved_status_.m_szFullName[0] = '\0';       // We don't want to change the file name

            load_icon(file_name);
            show_icon();
            SetPathName(file_name);
        }
        else
        {
            // Set up the file as a temp data file
            int idx = AddDataFile(file_name, TRUE);
            if (idx == -1)
            {
                ::HMessageBox("Too many temporary files\n"
                              "Try saving the file then try again.");
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

void CHexEditDoc::OnDffdRefresh() 
{
    CheckUpdate();
}

void CHexEditDoc::OnUpdateDffdRefresh(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(update_needed_);
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
        MSXML::IXMLDOMElementPtr pnew = xt.m_pdoc->createElement(_bstr_t("Elem1"));
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

void CHexEditDoc::CheckUpdate()
{
    if (update_needed_)
    {
		CSaveStateHint ssh;
        UpdateAllViews(NULL, 0, &ssh);
        ScanFile();  // rescan doc (for tree views) since it has changed
        //CRefreshHint rh;
        //UpdateAllViews(NULL, 0, &rh);
		CDFFDHint dffdh;
        UpdateAllViews(NULL, 0, &dffdh);
		CRestoreStateHint rsh;
        UpdateAllViews(NULL, 0, &rsh);
//        update_needed_ = false;   // This is done in ScanFile()
    }
}

// Opens the XML file asociated with this file
// The XML file describes the data contained within the file.
// It opens the passed file.  If NULL it opens the previously associated XML
// file, if any, then looks for a file based on the file extension.
void CHexEditDoc::OpenDataFormatFile(LPCTSTR data_file_name /*=NULL*/)
{
    int saved_file_num = xml_file_num_;
    xml_file_num_ = -1;  // Default to no file found

    // Make sure we are displaying a tree view
    if (theApp.tree_view_ == 0 || theApp.tree_view_ == -1)
    {
        ASSERT(data_file_name == NULL);
        return;
    }

    CString filename;

    if (data_file_name == NULL)
    {
        // Work out what data format (XML) file to use
        int recent_file_index = -1;
        CHexFileList *pfl = theApp.GetFileList();

        if (pfile1_ != NULL)
            recent_file_index = pfl->GetIndex(pfile1_->GetFilePath());

        if (recent_file_index != -1)
        {
            // Get data format file from file settings for this file
            filename = pfl->GetData(recent_file_index, CHexFileList::FORMAT);
//            if (!filename.IsEmpty() && _access(theApp.xml_dir_+filename, 0) == -1)
//                filename.Empty();

            // Find this filename in the list of current XML files
            if (filename.CompareNoCase("default") != 0)
                xml_file_num_ = theApp.FindXMLFile(filename);
        }

        // If none found above then use default for this file extension
        if (xml_file_num_ == -1 && pfile1_ != NULL)
        {
            // Get file extension and change "." to "_"
            CString ss = pfile1_->GetFileName();
            if (ss.ReverseFind('.') != -1)
            {
                filename = CString("_") + ss.Mid(ss.ReverseFind('.')+1);

                // Check that a file exists for this extension
//                if (_access(theApp.xml_dir_+filename, 0) == -1)
//                    filename.Empty();            // No file so don't use this file name
                xml_file_num_ = theApp.FindXMLFile(filename);
            }
        }

        // Lastly use "default" .XML if present
        if (xml_file_num_ == -1)
        {
            filename = "default";

            // Find it in the list
            xml_file_num_ = theApp.FindXMLFile(filename);
        }
    }
    else
    {
        ASSERT(data_file_name != NULL);
        // Check if specified file name is there
//        if (_access(theApp.xml_dir_+data_file_name, 0) != -1)
//            filename = data_file_name;
        xml_file_num_ = theApp.FindXMLFile(data_file_name);
        if (xml_file_num_ != -1)
            filename = data_file_name;
    }

    // If we got a file name to use then use it
    if (xml_file_num_ != -1)
    {
        ASSERT(!filename.IsEmpty());

        CXmlTree *saved_ptree = ptree_;

        CString ss;

        ptree_ = new CXmlTree(theApp.xml_dir_ + filename + CString(".xml"));
        if (ptree_->Error())
        {
            ss.Format("XML parse error in file \"%s.XML\"\nLine %ld:%s\n%s", 
                      filename, long(ptree_->ErrorLine()),
                      ptree_->ErrorLineText(), ptree_->ErrorMessage());
            AfxMessageBox(ss);
            delete ptree_;

            // Restore previous file
            ptree_ = saved_ptree;
            xml_file_num_ = saved_file_num;
        }
        else if (ptree_->GetDTDName() != "binary_file_format")
        {
            ss.Format("Invalid DTD used with XML file \"%s\"", ptree_->GetFileName());
            AfxMessageBox(ss);
            delete ptree_;

            // Restore previous file
            ptree_ = saved_ptree;
            xml_file_num_ = saved_file_num;
        }
        else if (saved_ptree != NULL)
        {
            // Destroy previous file
            delete saved_ptree;
        }
    }
}

// Called when views opened to make sure file has been scanned
BOOL CHexEditDoc::ScanInit()
{
    if (!df_init_)          // only scan once
        return ScanFile();
    else
        return !df_mess_.IsEmpty();
}

// This function scans the (data) file based on info from the (XML data format) file.
// It loads info. into member variables ready for display in a data format tree view.
// On successful analysis TRUE is returned. If the data does not match what is expected
// from data format file info then FALSE is returned (df_mess_ describes the error):

// Various error conditions are handled as:
// - if the data is supposed to be past the physical EOF then address == -1
// - if the data element is not present (empty for or false if) then size == 0
// - if a data element is incorrect (domain error) then its size is -ve

// Note: ScanFile() is not normally called immediately after OpenDataFormatFile() to
// give MSXML (via CXmlTree) a chance to load and process the XML file asynchronously.
BOOL CHexEditDoc::ScanFile()
{
    df_init_ = TRUE;

    if (ptree_ == NULL || ptree_->Error())
    {
        df_mess_ = "No data description file is available";
        return FALSE;
    }

    CWaitCursor wait;

    // Init progress bar
    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    mm->m_wndStatusBar.EnablePaneProgressBar(0);
    m_last_checked = clock();

    df_mess_.Empty();
    max_indent_ = 1;

    df_type_.clear();
    df_address_.clear();
    df_size_.clear();
    df_extra_.clear();
    df_indent_.clear();
    df_elt_.clear();
    df_info_.clear();
    df_enum_.clear();
    ASSERT(ptree_->GetRoot().GetName() == "binary_file_format");

    default_byte_order_ = ptree_->GetRoot().GetAttr("default_byte_order");
    default_read_only_ = ptree_->GetRoot().GetAttr("default_read_only");
    default_char_set_ = ptree_->GetRoot().GetAttr("default_char_set");

    // Add info for root element
    df_type_.push_back(DF_FILE);            // type representing all of file
    df_address_.push_back(0);               // address is start of file
    df_size_.push_back(0);                  // size is not yet known (filled in later)
    df_extra_.push_back(-1);                // arrays only
    df_indent_.push_back(1);                // represents root of tree view
    df_elt_.push_back(ptree_->GetRoot());   // root element in CXmlTree
    df_info_.push_back(ExprStringType());
    in_jump_ = 0;                           // we are not in any JUMPs
	bits_used_ = 0;                         // we have not seen a bitfield yet
	last_size_ = 0;                         // store 0 when bits_used_ == 0

    FILE_ADDRESS size_tmp;
    CHexExpr ee(this);
    try
    {
        add_branch(ptree_->GetRoot(), 0, 2, ee, size_tmp); // process whole tree (getting size)
		ASSERT(bits_used_ == 0);                           // Bitfields should have been terminated

        df_size_[0] = size_tmp;
        if (!df_mess_.IsEmpty())
        {
			// Display any error message
            AfxMessageBox(df_mess_);
            df_address_[0] = -1;
        }
        else
        {
            CString dispStr = ptree_->GetRoot().GetAttr("expr");
            if (!dispStr.IsEmpty())
            {
				// Generate display "expr" for the whole template (displayed next to root elt name)
                int expr_ac;                            // Last node accessed by expression

                if (dispStr.Find('{') == -1)
                    dispStr = CString("{") + dispStr + "}";  // This makes evaluate handle the errors

                CHexExpr::value_t tmp = Evaluate(dispStr, ee, 0, expr_ac);

                if (tmp.typ == CHexExpr::TYPE_STRING)
					df_info_[0] = *tmp.pstr;
            }
        }

        // Check that we're at EOF
        if (size_tmp < length_)
        {
            AfxMessageBox("Data past expected end of file");

            df_type_.push_back(DF_EXTRA);           // represents extra unexpected data
            df_address_.push_back(size_tmp);        // address is where EOF expected
            df_size_.push_back(length_ - size_tmp); // size is distance to real EOF
            df_extra_.push_back(-1);                // used for arrays only
            df_indent_.push_back(1);                // this is the only other tree element at root
            df_elt_.push_back(ptree_->GetRoot());   // what else can we use here?
            df_info_.push_back(ExprStringType("Expected EOF"));
        }
    }
    catch (const char *mess)
    {
        (void)mess;
        TRACE1("Caught %s in ScanFile\n", mess);
    }

    mm->m_wndStatusBar.EnablePaneProgressBar(0, -1);  // Turn off progress now
    update_needed_ = false;
    return TRUE;
}

// Adds a complete branch of the display tree and returns the size (bytes) of all the data of the tree.

// The return value indicates the last vector element accessed in expression used in this branch.
// This is used for arrays to determine if its elements are of fixed size or can vary.  If all
// nodes accessed within expressions of a branch are above the current element then the branch
// cannot vary in size.  Expressions occur in the "test" attribute of the "if" tag and the "count"
// and "stop_test" attributes of the "for" tag (arrays).

// If element sizes of an array can vary then we have to read and check each element of the array
// so that we know the total size of the array and the address of anything after the array.  If 
// element sizes are fixed we only display the first few elements of the array and can work out
// the array size as the numbers of elements times the element size.

// parent = the node of the parent whose child(ren) we are processing
// addr = current address in file or -1 if we are not processing data from the file
// ind = current indent level used in the tree view
// ee = expression evaluator including resolution of indentifier values
// returned_size = size passed back to caller so it knows the size of all children
// child_num = child to process or -1 to process all children (used for IF/ELSE processing)
// ok_bitfield_at_end = false to terminate bitfield or true if bitfield can continue (ie array of bitfields)

int CHexEditDoc::add_branch(CXmlTree::CElt parent, FILE_ADDRESS addr, unsigned char ind,
                            CHexExpr &ee, FILE_ADDRESS &returned_size,
							int child_num /* = -1*/, bool ok_bitfield_at_end /* = false */)
{
    ASSERT(DF_LAST < 128);

    // Keep track of the maximum indent
    if (ind > max_indent_)
    {
        ASSERT(ind < 255);
        max_indent_ = ind;
    }

    // Now do the subtree
    CXmlTree::CElt elt;
    if (child_num == -1)
        elt = parent.GetFirstChild();
    else
        elt = parent.GetChild(child_num); // Just do the specified child

    int last_ac = -1;                   // Default to no nodes accessed in expressions
    returned_size = 0;

	int ii;
    while (!elt.IsEmpty())
    {
        ii = df_address_.size();                // Index of entry we will now add

        // Add new entry in all arrays arrays (except df_type_) even if they are adjusted later
        // Note that one entry in these vectors corresp. to a row in the grid (tree display)
        df_address_.push_back(addr);
        df_size_.push_back(0);                  // Not yet known
        df_extra_.push_back(-1);                // Only used for array (below)
        df_indent_.push_back(ind);              // Indentation in tree
        df_elt_.push_back(elt);                 // Store a ptr to XML elt in case we need other stuff
        df_info_.push_back(ExprStringType());

        // Now also add the type (df_type_) and handle sub-elements (dep on the type)
        CString elt_type = elt.GetName();

		// First see if we need to terminate any preceding bitfield if the next field is not a bitfield
		if (bits_used_ > 0 && (elt_type != "data" || atoi(elt.GetAttr("bits")) == 0))
		{
			// Move to the end of this bitfield storage unit
			ASSERT(last_size_ != 0);
			returned_size += last_size_;
			if (addr != -1)
				addr += last_size_;
			df_address_[ii] = addr;
			// Indicate that there is now no bitfield in effect
			bits_used_ = 0;
			last_size_ = 0;
		}

        if (elt_type == "define_struct")
        {
            // If in edit mode show it in the tree so it can be edited
            if (theApp.tree_edit_)
            {
                // Add sub node but use address of -1 so that file is not accessed
                df_type_.push_back(DF_DEFINE_STRUCT);

                FILE_ADDRESS size_tmp;              // Ignored since we stay at the same address after return
                last_ac = _MAX(last_ac, add_branch(elt, -1, ind+1, ee, size_tmp));
                df_size_[ii] = size_tmp;            // Store size of struct
            }
            else
            {
                // Don't show struct definitions when not in edit mode
                //df_type_.pop_back(); - not pushed so don't pop it
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();
            }
        }
        else if (elt_type == "use_struct")
        {
            // Find in map and use it
            df_type_.push_back(DF_USE_STRUCT);

            // Find the define_struct that is to be used
            CString ss = elt.GetAttr("type_name");
            CXmlTree::CElt def_elt = elt.GetOwner()->GetRoot().GetFirstChild();
            for ( ; !def_elt.IsEmpty() && def_elt.GetName() == "define_struct"; ++def_elt)
                if (ss == def_elt.GetAttr("type_name"))
                    break;

            if (def_elt.IsEmpty())
            {
                CString ss;
                ss.Format("Structure definition for \"%s\" was not found", elt.GetAttr("type_name"));
                HandleError(ss);
            }
            else if (addr != -1)
            {
                // Add sub-elements of the STRUCT (only if present else we get inf. recursion)
                FILE_ADDRESS size_tmp;
                last_ac = _MAX(last_ac, add_branch(def_elt, addr, ind+1, ee, size_tmp));
                df_size_[ii] = size_tmp;            // size of struct is size of all contained elements
                returned_size += size_tmp;          // keep track of size of elts for our parent
                addr += size_tmp;                   // keep track of where we are now in the file

                CString dispStr = elt.GetAttr("expr");
                if (dispStr.IsEmpty())
                    dispStr = def_elt.GetAttr("expr");
                if (!dispStr.IsEmpty())
                {
                    int expr_ac;                            // Last node accessed by expression

                    if (dispStr.Find('{') == -1)
                        dispStr = CString("{") + dispStr + "}";  // This makes evaluate handle the errors

                    CHexExpr::value_t tmp = Evaluate(dispStr, ee, ii, expr_ac);
                    //if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

                    if (tmp.typ == CHexExpr::TYPE_STRING)
						df_info_[ii] = *tmp.pstr;
                }
            }
        }
        else if (elt_type == "struct")
        {
            df_type_.push_back(DF_STRUCT);

            // Add sub-elements of the STRUCT
            FILE_ADDRESS size_tmp;
            last_ac = _MAX(last_ac, add_branch(elt, addr, ind+1, ee, size_tmp));
            df_size_[ii] = size_tmp;            // size of struct is size of all contained elements
            returned_size += size_tmp;          // keep track of size of elts for our parent

            // If we have not passed EOF (addr of -1 indicates we have run out of data to match the template)
            if (addr != -1)
                addr += size_tmp;               // keep track of where we are now in the file

            // Only evaluate if present (since likely to eval members that are invalid)
            if (addr != -1)
            {
                CString dispStr = elt.GetAttr("expr");
                if (!dispStr.IsEmpty())
                {
                    int expr_ac;                            // Last node accessed by expression

                    if (dispStr.Find('{') == -1)
                        dispStr = CString("{") + dispStr + "}";  // This makes Evaluate handle the errors

                    CHexExpr::value_t tmp = Evaluate(dispStr, ee, ii, expr_ac);
                    //if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

                    if (tmp.typ == CHexExpr::TYPE_STRING)
						df_info_[ii] = *tmp.pstr;
                }
            }
        }
        else if (elt_type == "for")
        {
            df_type_.push_back(DF_FORF);        // Default to an array with fixed size elements (until we find different)

            CString strCount = elt.GetAttr("count");
            CString strTest = elt.GetAttr("stop_test");

            int elts_ac;                            // Last node accessed by elements
            int expr_ac;                            // Last node accessed by count or stop_test expression
            FILE_ADDRESS elt_size;                  // How big is one element of array
            int num_elts = INT_MAX;                 // Elts in the array (default to fill to EOF)
            if (addr == -1)
                num_elts = 0;
            else if (!strCount.IsEmpty())
            {
                CHexExpr::value_t tmp = ee.evaluate(strCount, ii, expr_ac);
                if (expr_ac > last_ac) last_ac = expr_ac;   // Keep track of what elts we access (in case parent is FOR)
                if (tmp.typ == CHexExpr::TYPE_NONE)
                {
                    df_size_[ii] = 0;

                    // Only show error message if we don't expect it (if past EOF we expect it)
                    ASSERT(addr != -1);
                    HandleError(CString(ee.get_error_message()) + "\nin \"count\" of \"for\".");
                    num_elts = 0;
                }
                else if (tmp.typ != CHexExpr::TYPE_INT || tmp.int64 < 0)
                {
                    df_size_[ii] = 0;

                    HandleError("The \"count\" attribute expression of the\n"
                                  "\"for\" tag must yield a +ve integer result.\n");
                    num_elts = 0;
                }
                else
                {
                    // We have found how many elts in this (instance of this) FOR
                    num_elts = int(tmp.int64);
                }
            }

            if (num_elts > 0)
            {
                ASSERT(addr != -1);

                // Now get the sub-element of the FOR
                elts_ac = add_branch(elt, addr, ind+1, ee, elt_size, -1, true);
                ASSERT(elt_size > 0 || bits_used_ > 0);

                if (elts_ac > ii || !strTest.IsEmpty())
                {
                    // Since the expression used within the FOR access data of the FOR (via "this")
                    // each element can vary in size since the data might vary
                    last_ac = ii+1;                 // Remember that array elts are accessed
                    df_type_[ii] = DF_FORV;         // Signal that array elts are var size
                }

                // One or more elts in the array so get them
                // Note: we do not display all elements for arrays with fixed size elements since
                // they can often be huge and we know where the array ends since we know the number
                // of elts and their size.  "display_elts" is the number of elts to show.
                int display_elts = df_type_[ii] == DF_FORV ? num_elts : min(theApp.max_fix_for_elts_, num_elts);
                FILE_ADDRESS array_size;        // Total size of this array

                array_size = elt_size;          // We have already added one elt (above)
                int jj = ii + 1;                // Start at first array elt

                // Create more array elts until we have displayed them all (or as many as
                // we are going to display for fixed elt arrays) or we are told to stop (strTest)
                for (int elt_num = 1; elt_num < display_elts; ++elt_num)
                {
					// Check if we need to stop at EOF
                    if (strCount.IsEmpty() &&
                        ( /* addr == -1 || */
                         (df_type_[ii] != DF_FORV && addr + array_size + elt_size + last_size_ > length_) ||    // xxx test this
                         (df_type_[ii] == DF_FORV && addr + array_size + last_size_ >= length_)
                        )
                       )
                    {
//                        ASSERT(strTest.IsEmpty());
                        // If no count and not test we stop at EOF
                        break;
                    }
                    // Check if we need to stop due to the stop_test string
                    else if (!strTest.IsEmpty())
                    {
                        // Evaluate the stop_test expression and handle any errors
                        CHexExpr::value_t tmp = ee.evaluate(strTest, jj, expr_ac);
                        if (expr_ac > last_ac) last_ac = expr_ac; // Keep track of what elts we access (in case parent is also FOR)
                        if (tmp.typ == CHexExpr::TYPE_NONE)
                        {
                            df_size_[ii] = 0;

                            // Only display error if not expected (ie not past EOF)
                            ASSERT(addr != -1);
                            HandleError(CString(ee.get_error_message()) + "\nin \"stop_test\" of \"for\".");
                            break;
                        }
                        else if (tmp.typ != CHexExpr::TYPE_BOOLEAN)
                        {
                            df_size_[ii] = 0;

                            HandleError("The \"stop_test\" attribute expression of the\n"
                                          "\"for\" tag must yield a boolean result.\n");
                            break;
                        }
                        else if (tmp.boolean)
                            break;              // stop_test returned true so break from loop
                    }

                    jj = df_address_.size();         // Move to start of next array elt

                     // Process next element of array (FOR)
                    ASSERT(addr != -1);
                    (void)add_branch(elt, addr + array_size, ind+1, ee, elt_size, -1, true);
                    ASSERT(elt_size > 0 || bits_used_ > 0);
                    array_size += elt_size;
                }

//                if (elts_ac <= ii && !strCount.IsEmpty() && elt_num < num_elts)
                // If this is a fixed size elt array and there are more elts (but not displayed)
                if (df_type_[ii] == DF_FORF && elt_num >= display_elts && elt_num < num_elts)
                {
                    ASSERT(elt_num == display_elts);

					int data_bits;

					// For array of bitfields "elt_size" is not valid (can actually contain zero or
					// size of bitfield storage unit) so we have to do calcs differently.
					if (elt.GetFirstChild().GetName() == "data" &&
						// elt.GetFirstChild().GetAttr("type") == "int" &&
						(data_bits = atoi(elt.GetFirstChild().GetAttr("bits"))) > 0)
					{
						// We need to work out the number of bitfield storage units needed for
						// the whole array and the number in the "elt_num" elts already done.
						// At this point jj is the last array element hence
						// df_size_[jj] contains the size of the bitfield storage unit,
						ASSERT(df_size_[jj] == 1 || df_size_[jj] == 2 || df_size_[jj] == 4 || df_size_[jj] == 8);
						int total_units, done_units;
						if (elt.GetFirstChild().GetAttr("straddle") == "true")
						{
							// In straddle mode we work in bits then convert back to bytes
							if (num_elts == INT_MAX)
							{
								total_units = int((length_ - addr)/df_size_[jj]);    // How many units to EOF
								num_elts = int((total_units*df_size_[jj]*8)/data_bits);
							}
							else
							{
								total_units = int((num_elts*data_bits - 1)/(df_size_[jj]*8)) + 1;
							}
							done_units = int((elt_num*data_bits - 1)/(df_size_[jj]*8)) + 1;
						}
						else
						{
							// If not straddling we work in storage units
							int elts_per_unit = int((df_size_[jj]*8)/data_bits);     // How many bitfields can we store in a unit
							if (num_elts == INT_MAX)
							{
								total_units = int((length_ - addr)/df_size_[jj]);    // How many units to EOF
								num_elts = total_units * elts_per_unit;
							}
							else
							{
								total_units = (num_elts-1)/elts_per_unit + 1;   // How many units needed for whole array
							}
							done_units = (elt_num-1)/elts_per_unit + 1;         // How many units already done
						}
						df_size_[ii] = total_units * df_size_[jj];              // Array size is no of units X unit size

						if (num_elts > elt_num)
						{
							// Add DF_MORE entry to show fixed size elts that have no entry
							df_type_.push_back(DF_MORE);
							ASSERT(addr != -1);
							df_address_.push_back(addr + done_units * df_size_[jj]);
							df_size_.push_back((total_units - done_units) * df_size_[jj]);
							df_extra_.push_back(num_elts-elt_num);
							df_indent_.push_back(ind+1);
							df_elt_.push_back(elt);
							df_info_.push_back(ExprStringType());
						}

						// Signal that we have handled bitfields (this avoids adding another storage unit to addr/returned_size below)
						bits_used_ = 0;
						last_size_ = 0;
					}
					else
					{
						// If we go all the way to EOF work out how many elts there are
						if (num_elts == INT_MAX)
						{
							ASSERT(strCount.IsEmpty());
							num_elts = int((length_ - addr)/elt_size);
						}
						// Set the size of the array to all elts (even those not displayed)
						df_size_[ii] = num_elts*elt_size;

						if (num_elts > elt_num)
						{
							// Add DF_MORE entry to show fixed size elts that have no entry
							df_type_.push_back(DF_MORE);
							ASSERT(addr != -1);
							df_address_.push_back(addr + elt_num*elt_size);
							df_size_.push_back((num_elts-elt_num)*elt_size);
							df_extra_.push_back(num_elts-elt_num);
							df_indent_.push_back(ind+1);
							df_elt_.push_back(elt);
							df_info_.push_back(ExprStringType());
						}
					}

                }
                else
                {
                    // All the element (array_size) have been displayed so use 
                    // their size as the total size for the array.
                    df_size_[ii] = array_size;
                }

                df_extra_[ii] = elt_num;          // No of elts actually read
                returned_size += df_size_[ii];
                addr += df_size_[ii];
            }
            else if (theApp.tree_edit_)
            {
                df_extra_[ii] = 0;

                // Get the branch for FOR so that it can be displayed and edited
                elts_ac = add_branch(elt, -1, ind+1, ee, elt_size);

                // xxx do we need to do anything with elts_ac here like set DF_FORV?

                for (int jj = ii; jj < (int)df_size_.size(); ++jj)
                {
                    df_size_[jj] = 0;           // signal that this elt is not present in file
                    df_address_[jj] = -1;       // make all sub-elts show problem
                }
            }
            else
            {
                // Remove this empty FOR when not in edit mode
                df_type_.pop_back();
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();
            }
        } // end array "for" processing
        else if (elt_type == "switch")
		{
            BOOL show_parent_row = theApp.tree_edit_;      // Only show IF row in edit mode
            unsigned char new_ind = ind+1;

            if (!show_parent_row)
            {
                // Don't show IF nodes at all (only contained branch if condition is TRUE
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();

                new_ind = ind;                      // Same indent as IF would have if shown
                ii--;                               // go back since we removed the IF
            }
            else
                df_type_.push_back(DF_SWITCH);

            // Check test expression
            CHexExpr::value_t tmp;
            bool expr_ok = true;                    // Valid integer expression?

            if (addr != -1)
            {
                int expr_ac;                            // Last node accessed by test expression

                tmp = ee.evaluate(elt.GetAttr("test"), ii, expr_ac);
                if (expr_ac > last_ac) last_ac = expr_ac;

                // Handle errors in test expression
				if (tmp.typ != CHexExpr::TYPE_INT)
                {
                    if (show_parent_row)
                        df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

                    // Display error message unless error is due to past EOF
                    if (tmp.typ != CHexExpr::TYPE_NONE)
                        HandleError("The \"test\" attribute expression of the\n"
                                    "\"switch\" tag must yield an integer result.\n");
                    else
                        HandleError(CString(ee.get_error_message()) + "\nin \"test\" of \"switch\".");

                    expr_ok = false;
                }
            }

			bool found_it = false;                 // Did we find a case that matches the switch expression value
            FILE_ADDRESS size_tmp = 0;
			for (CXmlTree::CElt case_elt = elt.GetFirstChild(); !case_elt.IsEmpty(); ++case_elt)
			{
				bool is_valid = false;    // Is this the "taken" case?
                ASSERT(case_elt.GetName() == "case" && case_elt.GetNumChildren() == 1);

				if (addr != -1 && expr_ok && !found_it)
				{
					// check if switch expression matches this case's range
					range_set<__int64> rs;
			        std::istringstream strstr((const char *)case_elt.GetAttr("range"));
					if (strstr >> rs && rs.find(tmp.int64) != rs.end())
						is_valid = true;
				}

				if (is_valid || theApp.tree_edit_)  // only show the taken case unless we are in edit mode
				{
					unsigned jj = df_address_.size();  // remember where we are up to
					last_ac = _MAX(last_ac, add_branch(case_elt, is_valid ? addr : -1, new_ind, ee, size_tmp));
					if (is_valid)
					{
						if (show_parent_row)
							df_size_[ii] = size_tmp;   // update size for IF (if present)

						returned_size += size_tmp;
						addr += size_tmp;

						found_it = true;
					}
					else
					{
						// Grey out this node and sub-nodes
						for ( ; jj < df_address_.size(); ++jj)
						{
							df_size_[jj] = 0;           // signal that this elt is not present in file
							//df_address_[jj] = -1;       // make all sub-elts show problem
						}
					}
				}
			}

			if (show_parent_row && !found_it)
			{
                df_size_[ii] = 0;           // signal that this elt is not present in file
                //df_address_[ii] = -1;       // make all sub-elts show problem
			}
		} // end "switch" processing
        else if (elt_type == "if")
        {
            ASSERT(elt.GetNumChildren() == 1 || elt.GetNumChildren() == 3);
            BOOL show_parent_row = theApp.tree_edit_;      // Only show IF row in edit mode
            unsigned char new_ind = ind+1;

            if (!show_parent_row)
            {
                // Don't show IF nodes at all (only contained branch if condition is TRUE
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();

                new_ind = ind;                      // Same indent as IF would have if shown
                ii--;                               // go back since we removed the IF
            }
            else
                df_type_.push_back(DF_IF);

            // Check test expression
            CHexExpr::value_t tmp;
            bool expr_ok = true;                    // Expression was boolean?

            if (addr != -1)
            {
                int expr_ac;                            // Last node accessed by test expression

                tmp = ee.evaluate(elt.GetAttr("test"), ii, expr_ac);
                if (expr_ac > last_ac) last_ac = expr_ac;

                // Handle errors in test expression
                if (tmp.typ != CHexExpr::TYPE_BOOLEAN)
                {
                    if (show_parent_row)
                        df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

                    // Display error message unless error is due to past EOF
                    if (tmp.typ != CHexExpr::TYPE_NONE)
                        HandleError("The \"test\" attribute expression of the\n"
                                    "\"if\" tag must yield a boolean result.\n");
                    else
                        HandleError(CString(ee.get_error_message()) + "\nin \"test\" of \"if\".");

                    expr_ok = false;
                }
            }

            FILE_ADDRESS size_tmp;

            // Do IF part
            if (addr != -1 && expr_ok && tmp.boolean)
            {
                // Now get the branch for true part
                last_ac = _MAX(last_ac, add_branch(elt, addr, new_ind, ee, size_tmp, 0));
                if (show_parent_row)
                    df_size_[ii] = size_tmp;   // update size for IF (if present)

                returned_size += size_tmp;
                addr += size_tmp;
            }
            else if (theApp.tree_edit_)
            {
				// Grey out sub-nodes that are not present
                unsigned curr_ii = ii;
                if (show_parent_row && addr != -1 && expr_ok && !tmp.boolean && elt.GetNumChildren() >= 3)
                    ++curr_ii;                  // Don't grey parent node if ELSE part is valid

                // Get the branch for if so that it can be displayed and edited
                last_ac = _MAX(last_ac, add_branch(elt, -1, new_ind, ee, size_tmp, 0));

                for (unsigned jj = curr_ii; jj < df_address_.size(); ++jj)
                {
                    df_size_[jj] = 0;           // signal that this elt is not present in file
                    //df_address_[jj] = -1;       // make all sub-elts show problem
                }
            }

            // Do ELSE part if it is present
            if (addr != -1 && expr_ok && !tmp.boolean && elt.GetNumChildren() >= 3)
            {
				ASSERT(elt.GetNumChildren() == 3);

                // Now get the branch for false part
                last_ac = _MAX(last_ac, add_branch(elt, addr, new_ind, ee, size_tmp, 2));
                if (show_parent_row)
                    df_size_[ii] = size_tmp;   // update size for containing IF (if present)

                returned_size += size_tmp;
                addr += size_tmp;
            }
            else if (theApp.tree_edit_ && elt.GetNumChildren() >= 3)
            {
                unsigned curr_ii = df_address_.size();  // remember where we were up to

                // Get the branch for IF so that it can be displayed and edited
                last_ac = _MAX(last_ac, add_branch(elt, -1, new_ind, ee, size_tmp, 2));

                // Mark all these sub-elements as not present (greyed)
                for (unsigned jj = curr_ii; jj < df_address_.size(); ++jj)
                {
                    df_size_[jj] = 0;           // signal that this elt is not present in file
                    //df_address_[jj] = -1;       // make all sub-elts show problem
                }
            }
        } // "if"
        else if (elt_type == "jump")
        {
            BOOL show_parent_row = theApp.tree_edit_;      // Only show row in edit mode
            unsigned char new_ind = ind+1;
            if (!show_parent_row)
            {
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();

                new_ind = ind;                      // Same indent as JUMP would have if shown
                ii--;                               // go back since we removed the last row
            }
            else
                df_type_.push_back(DF_JUMP);

            FILE_ADDRESS jump_addr = -1;

            if (addr != -1)
            {
                // Get offset for new address
                int expr_ac;                            // Last node accessed by offset expression

                CHexExpr::value_t tmp = ee.evaluate(elt.GetAttr("offset"), ii, expr_ac);
                if (expr_ac > last_ac) last_ac = expr_ac;

                // Expression must be an integer
                if (tmp.typ != CHexExpr::TYPE_INT)
                {
                    if (show_parent_row)
                        df_size_[ii] = 0;       // Signal error here in case exception thrown in HandleError and code below not reached

                    // Display error message unless error is due to past EOF
                    if (tmp.typ != CHexExpr::TYPE_NONE)
                        HandleError("The \"expr\" address expression of the\n"
                                    "\"jump\" tag must yield an integer result.\n");
                    else
                        HandleError(CString(ee.get_error_message()) + "\nin \"expr\" of \"jump\".");
                }
                else
                {
                    // Work out address jumped to
                    CString origin = elt.GetAttr("origin");
                    if (origin == "start")
                        jump_addr = tmp.int64;
                    else if (origin == "current")
                        jump_addr = addr + tmp.int64;
                    else if (origin == "end")
                        jump_addr = length_ + tmp.int64;
                    else
                        ASSERT(0);

                    if (jump_addr < 0)
                    {
                        HandleError("The jump address is before the start of file.\n");
                        jump_addr = -1;
                    }
                    else if (jump_addr >= length_)
                    {
                        HandleError("The jump address is past the end of file.\n");
                        jump_addr = -1;
                    }
                }
            }

            if (jump_addr != -1 || theApp.tree_edit_)
            {
                ++in_jump_;                     // Turn off progress in JUMP since it's based on file address
                // Add sub-element
                FILE_ADDRESS size_tmp;          // Ignored since we stay at the same address after return
                last_ac = _MAX(last_ac, add_branch(elt, jump_addr, new_ind, ee, size_tmp));
                in_jump_--;

                if (show_parent_row)
                {
                    df_address_[ii] = jump_addr;    // Store address & size of contained element
                    df_size_[ii] = size_tmp;

                    if (jump_addr != -1)
                    {
                        // ... but show address jumped to in the tree column
                        char buf[32];
                        sprintf(buf, "=> %I64d", jump_addr);   // CString::Format does not support %I64 (yet? - VS6)
						CString strAddr(buf);
                        AddCommas(strAddr);
                        if (jump_addr > 9)
                        {
                            sprintf(buf, "%I64X", jump_addr);
                            strAddr += CString(" (0X") + buf + CString(")");
                        }
                        df_info_[ii] = strAddr;
                    }
                    else
                        df_address_[ii] = -1;
                }
            }
        } // "jump"
        else if (elt_type == "eval")
        {
            df_type_.push_back(DF_EVAL);

            CHexExpr::value_t tmp;
            bool display_result = false;

            if (addr != -1)
            {
                // Evaluate the expression
                int expr_ac;                            // Last node accessed by expression

                // Evaluate a simple expression or a srting containing one or more {expr;format} specs
                tmp = Evaluate(elt.GetAttr("expr"), ee, ii, expr_ac);
                //if (expr_ac > last_ac) last_ac = expr_ac;  // Don't update last_ac as the value is only for display

                // Get options
                bool display_error = false;

                CString ss = elt.GetAttr("display_error");
                if (ss.CompareNoCase("true") == 0)
                    display_error = true;
                ss = elt.GetAttr("display_result");
                if (ss.CompareNoCase("true") == 0)
                    display_result = true;

                // Display an error message if the expression evaluated false
                if (tmp.typ == CHexExpr::TYPE_NONE)
                {
                    HandleError(CString(ee.get_error_message()) + "\nin EVAL \"expr\" attribute.");
                    display_result = true;
                }
                else if (display_error && tmp.typ != CHexExpr::TYPE_BOOLEAN && addr != -1)
                {
                    HandleError("The \"expr\" attribute expression of the \"EVAL\"\n"
                                "tag must be boolean if display_error is true.\n");

                    df_address_[ii] = -1;
                    // Always display the result if there is an error
                    display_result = true;
                }
                else if (display_error && !tmp.boolean)
                {
                    df_address_[ii] = -1;
                    // Always display the result if there is an error
                    display_result = true;
                }
            }

            // Display result if requested (always display in edit mode to allow editing of EVAL)
            if (display_result || theApp.tree_edit_)
            {
                // Save the result of the evaluation for later display
                switch (tmp.typ)
                {
                case CHexExpr::TYPE_BOOLEAN:
                    df_info_[ii] = tmp.boolean ? "TRUE" : "FALSE";
                    break;
                case CHexExpr::TYPE_INT:
                    //df_info_[ii].Format("%I64d", tmp.int64);    // CString::Format does not support %I64 yet
                    {
                        char buf[32];
                        sprintf(buf, "%I64d", tmp.int64);
						CString strAddr(buf);
                        AddCommas(strAddr);
                        df_info_[ii] = strAddr;
                    }
                    break;
                case CHexExpr::TYPE_DATE:
                    if (tmp.date > -1e30)
                    {
                        COleDateTime odt;
                        odt.m_dt = tmp.date;
                        odt.m_status = COleDateTime::valid;
                        df_info_[ii] = odt.Format("%#c");
                    }
                    else
                        df_info_[ii] = "##Invalid date##";
                    break;
                case CHexExpr::TYPE_REAL:
#ifdef UNICODE_TYPE_STRING
                    df_info_[ii].Format(L"%g", tmp.real64);
#else
                    df_info_[ii].Format("%g", tmp.real64);
#endif
                    break;
                case CHexExpr::TYPE_STRING:
                    df_info_[ii] = *tmp.pstr;
                    break;
                default:
                    ASSERT(tmp.typ == CHexExpr::TYPE_NONE);
                    if (addr != -1)
                    {
                        HandleError(CString(ee.get_error_message()) + "\nin \"expr\" of \"eval\".");
                        df_info_[ii] = CString("##") + ee.get_error_message();
                    }
                    df_address_[ii] = -1;
                    break;
                }
            }
            else
            {
                // Remove this entry (display of result not requested and not in edit mode)
                df_type_.pop_back();
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();
            }
        } // "eval"
        else
        {
            ASSERT(elt_type == "data");
            df_type_.push_back(DF_DATA);

//            ASSERT(addr == -1 || addr < length_);
            CString data_type = elt.GetAttr("type");
            data_type.MakeLower();
            CString data_format = elt.GetAttr("format");
            data_format.MakeLower();

			// Get default char set if nec.
            if (data_format == "default")
				data_format = ptree_->GetRoot().GetAttr("default_char_set");

			// Work out length of data or 0 (not present), -1 (not known)
            CString tmp_len = elt.GetAttr("len");
            CHexExpr::value_t data_len;
            if (addr == -1)
                data_len = CHexExpr::value_t(0);     // Use zero length if not present
            else if (tmp_len.IsEmpty())
                data_len = CHexExpr::value_t(-1);    // Length not given - eg, string/none to EOF (also fixed len flds that ignore data_len)
            else
            {
                int expr_ac;                            // Last node accessed by test expression

				// Get length (which may be an expression)
                data_len = ee.evaluate(tmp_len, ii, expr_ac);
                if (expr_ac > last_ac) last_ac = expr_ac;
                if (data_len.typ != CHexExpr::TYPE_INT)
                {
                    df_size_[ii] = 0;       // In case exception thrown in HandleError and code below not done

                    // Display appropriate error message unless past EOF
                    if (data_len.typ != CHexExpr::TYPE_NONE)
                        HandleError("The length (in \"format\" attribute) for data\n"
                                      "type \"none\" must yield an integer result.\n");
                    else
                        HandleError(CString(ee.get_error_message()) + "\nin \"length\" attribute.");

                    data_len = CHexExpr::value_t(-2);
                }
            }

			// Work out byte order - this is only used for numeric types longer than 1 byte plus Unicode text (DF_WCHAR/DF_WSTRING)
            CString byte_order = elt.GetAttr("byte_order");
            if (byte_order == "default")
                byte_order = default_byte_order_;


			// Work out if this is bitfield and get number of bits
			int data_bits = atoi(elt.GetAttr("bits"));

            if (data_type == "none")
            {
                df_type_[ii] = DF_NO_TYPE;

                if (data_len.int64 > -1)
                    df_size_[ii] = data_len.int64;
                //else if (addr == -1)
                //{
                //    df_size_[ii] = 0;          // signal that it's not present
                //    last_ac = ii;
                //}
                else
                {
                    df_size_[ii] = length_ - addr;
                    last_ac = ii;              // Just means that elt has variable length (dep. on file length)
                }
            }
#if _MSC_VER >= 1300
            else if (data_type.Left(6) == "string" && data_format == "unicode")
            {
                df_type_[ii] = DF_WSTRING;
                df_extra_[ii] = atoi(data_type.Mid(6));     // Terminator appended to data type eg "string13"
                if (data_len.int64 > -1)
                    df_size_[ii] = data_len.int64;
                else
                {
                    // Find the end of string (0) or stop at EOF
#ifdef _DEBUG
                    wchar_t buf[4];  // A small buffer is more likely to catch bugs during testing
#else
                    wchar_t buf[256];
#endif
                    const wchar_t *pp;
                    size_t got;
					wchar_t term = df_extra_[ii];
					// For big-endian Unicode strings we have to convert each wide character to little-endian
					// to compare again term, but it is simpler just to reverse bytes of term instead.
					if (byte_order == "big")
						flip_bytes((unsigned char *)&term, 2);

                    df_size_[ii] = 0;
					ASSERT(sizeof(buf)%2 == 0);   // Must be even length for wide chars
                    while ((got = GetData((unsigned char *)buf, sizeof(buf), addr + df_size_[ii])) > 0)
                    {
                        if ((pp = wmemchr(buf, term, got/2)) != NULL)
                        {
                            // End of string found
                            df_size_[ii] += ((pp - buf) + 1)*2;  // Add one to include terminating byte, mult by 2 for wide chars
                            break;
                        }

                        df_size_[ii] += got;
                    }
                    last_ac = ii;               // Indicate that we looked at the data (to find end of string)
                }
			}
#endif
            else if (data_type.Left(6) == "string")
            {
                if (data_format == "ascii")
                    df_type_[ii] = DF_STRINGA;
                else if (data_format == "ansi")
                    df_type_[ii] = DF_STRINGN;
                else if (data_format == "oem")
                    df_type_[ii] = DF_STRINGO;
                else if (data_format == "ebcdic")
                    df_type_[ii] = DF_STRINGE;
#if !(_MSC_VER >= 1300)  // handled separately above
                else if (data_format == "unicode")
                    df_type_[ii] = DF_WSTRING;
#endif
				else
				{
					ASSERT(0);
                    df_type_[ii] = DF_STRINGN;
				}

                df_extra_[ii] = atoi(data_type.Mid(6));     // Store string terminator

                if (data_len.int64 > -1)
                    df_size_[ii] = data_len.int64;
                //else if (addr == -1)
                //{
                //    df_size_[ii] = 0;
                //    last_ac = ii;
                //}
                else
                {
                    // Find the end of string (null byte) or stop at EOF
#ifdef _DEBUG
                    unsigned char buf[4];  // A small buffer is more likely to catch bugs during testing
#else
                    unsigned char buf[256];
#endif
                    unsigned char *pp;
                    size_t got;

                    df_size_[ii] = 0;
                    while ((got = GetData(buf, sizeof(buf), addr + df_size_[ii])) > 0)
                    {
                        if ((pp = (unsigned char *)memchr(buf, df_extra_[ii], got)) != NULL)
                        {
                            // End of string found
                            df_size_[ii] += (pp - buf) + 1;  // Add one to include terminating byte
                            break;
                        }

                        df_size_[ii] += got;
                    }
                    last_ac = ii;               // We had to access the data of this element to find end of string
                }
            }
            else if (data_type == "char")
            {
                df_size_[ii] = 1;
                if (data_format == "ascii")
                    df_type_[ii] = DF_CHARA;
                else if (data_format == "ansi")
                    df_type_[ii] = DF_CHARN;
                else if (data_format == "oem")
                    df_type_[ii] = DF_CHARO;
                else if (data_format == "ebcdic")
                    df_type_[ii] = DF_CHARE;
                else if (data_format == "unicode")
                {
                    df_type_[ii] = DF_WCHAR;
                    df_size_[ii] = 2;
                }
                else
				{
					ASSERT(0);
                    df_type_[ii] = DF_CHARN;
				}
            }
            else if (data_bits > 0)
			{
				ASSERT(data_type == "int");
				// Bitfield
                if (data_len.int64 == 1)
                {
                    df_type_[ii] = DF_BITFIELD8;
                    df_size_[ii] = 1;
                }
                else if (data_len.int64 == 2)
                {
                    df_type_[ii] = DF_BITFIELD16;
                    df_size_[ii] = 2;
                }
                else if (data_len.int64 == 8)
                {
                    df_type_[ii] = DF_BITFIELD64;
                    df_size_[ii] = 8;
                }
                else
                {
                    df_type_[ii] = DF_BITFIELD32;
                    df_size_[ii] = 4;
                }

				bool data_down = elt.GetAttr("direction") == "down";
				bool data_straddle = elt.GetAttr("straddle") == "true";

				// Check if we need to advance to the next bitfield storage unit
				if (bits_used_ > 0 && (df_size_[ii] != last_size_ || 
					                   data_down != last_down_ || 
									   (bits_used_ + data_bits) > int(df_size_[ii])*8 && !data_straddle))
				{
					// Move to the end of this bitfield storage unit
					ASSERT(last_size_ != 0);
					returned_size += last_size_;
					if (addr != -1)
						addr += last_size_;
					df_address_[ii] = addr;

					// Indicate that there is now no bitfield in effect
					bits_used_ = 0;
					last_size_ = 0;
				}
				// Store number of bits in 2nd byte and position (of bottom bit of bitfield) in lowest byte
				// Note: If straddle is on position+bits could be greater than storage units size (OR position 
				// could be -ve for data_down) in which case the extra bits are taken from the next storage unit.
				if (data_down)
					df_extra_[ii] = (data_bits<<8) | (int(df_size_[ii])*8 - (bits_used_+data_bits))&0xFF;
				else
					df_extra_[ii] = (data_bits<<8) | bits_used_;

				last_size_ = df_size_[ii];
				last_down_ = data_down;
				bits_used_ += data_bits;
				if (bits_used_ >= int(df_size_[ii])*8)
				{
					// Exactly filled one storage unit or overflowed it (if straddle allowed)
					ASSERT(bits_used_ == int(df_size_[ii])*8 || data_straddle);
					returned_size += last_size_ * (bits_used_ / int(df_size_[ii]*8));
					if (addr != -1)
						addr += last_size_ * (bits_used_ / int(df_size_[ii]*8));
					bits_used_ %= int(df_size_[ii])*8;
				}
			}
            else if (data_type == "int" && data_format.Left(1) == "u")
            {
                // Unsigned integer - now just get the size
                if (data_len.int64 == 1)
                {
                    df_type_[ii] = DF_UINT8;
                    df_size_[ii] = 1;
                }
                else if (data_len.int64 == 2)
                {
                    df_type_[ii] = DF_UINT16;
                    df_size_[ii] = 2;
                }
                else if (data_len.int64 == 8)
                {
                    df_type_[ii] = DF_UINT64;
                    df_size_[ii] = 8;
                }
                else
                {
                    df_type_[ii] = DF_UINT32;
                    df_size_[ii] = 4;
                }
            }
            else if (data_type == "int" && data_format.Left(1) == "m")
            {
                // Signed (sign & magnitude) integer - now get the size
                if (data_len.int64 == 1)
                {
                    df_type_[ii] = DF_MINT8;
                    df_size_[ii] = 1;
                }
                else if (data_len.int64 == 2)
                {
                    df_type_[ii] = DF_MINT16;
                    df_size_[ii] = 2;
                }
                else if (data_len.int64 == 8)
                {
                    df_type_[ii] = DF_MINT64;
                    df_size_[ii] = 8;
                }
                else
                {
                    df_type_[ii] = DF_MINT32;
                    df_size_[ii] = 4;
                }
            }
            else if (data_type == "int")
            {
                // Signed (2's complement) integer - now get the size
                if (data_len.int64 == 1)
                {
                    df_type_[ii] = DF_INT8;
                    df_size_[ii] = 1;
                }
                else if (data_len.int64 == 2)
                {
                    df_type_[ii] = DF_INT16;
                    df_size_[ii] = 2;
                }
                else if (data_len.int64 == 8)
                {
                    df_type_[ii] = DF_INT64;
                    df_size_[ii] = 8;
                }
                else
                {
                    df_type_[ii] = DF_INT32;
                    df_size_[ii] = 4;
                }
            }
            else if (data_type == "real" && data_format.Left(3) == "ibm")
            {
                // IBM floating point value - get whether 32 or 64 bit
                if (data_len.int64 == 4)
                {
                    df_type_[ii] = DF_IBMREAL32;
                    df_size_[ii] = 4;
                }
                else
                {
                    df_type_[ii] = DF_IBMREAL64;
                    df_size_[ii] = 8;
                }
            }
            else if (data_type == "real")
            {
                if (data_len.int64 == 4)
                {
                    df_type_[ii] = DF_REAL32;
                    df_size_[ii] = 4;
                }
                else
                {
                    df_type_[ii] = DF_REAL64;
                    df_size_[ii] = 8;
                }
            }
            else if (data_type == "date")
            {
                if (data_format == "c51")
                {
                    df_type_[ii] = DF_DATEC51;
                    df_size_[ii] = 4;
                }
                else if (data_format == "c7")
                {
                    df_type_[ii] = DF_DATEC7;
                    df_size_[ii] = 4;
                }
                else if (data_format == "cmin")
                {
                    df_type_[ii] = DF_DATECMIN;
                    df_size_[ii] = 4;
                }
                else if (data_format == "c64")
                {
                    df_type_[ii] = DF_DATEC64;
                    df_size_[ii] = 8;
                }
                else if (data_format == "ole")
                {
                    df_type_[ii] = DF_DATEOLE;
                    df_size_[ii] = 8;
                }
                else if (data_format == "systemtime")
                {
                    df_type_[ii] = DF_DATESYSTEMTIME;
                    df_size_[ii] = 16;
                }
                else if (data_format == "filetime")
                {
                    df_type_[ii] = DF_DATEFILETIME;
                    df_size_[ii] = 8;
                }
                else if (data_format == "msdos")
                {
                    df_type_[ii] = DF_DATEMSDOS;
                    df_size_[ii] = 4;
                }
                else
                {
                    ASSERT(data_format == "c");
                    df_type_[ii] = DF_DATEC;
                    df_size_[ii] = 4;
                }
            }

            // Make sure we can actually get the data from the file
            if (addr != -1 && df_address_[ii] + df_size_[ii] > length_)
            {
//                df_size_[ii] = length_ - df_address_[ii];
                if (df_mess_.IsEmpty()) df_mess_ = "Unexpected EOF";
//                HandleError("Unexpected end of file");
                addr = -1;
            }

            // Check and store byte order
            if (byte_order == "big" && (df_type_[ii] >= DF_INT8 || df_type_[ii] == DF_WCHAR || df_type_[ii] == DF_WSTRING))
                df_type_[ii] = -df_type_[ii];   // -ve indicates big-endian byte order

            // Only check value against domain if we can read it
            if (addr != -1)
            {
                // Check if the data is within its domain
                CString strDomain = df_elt_[ii].GetAttr("domain");

                if (!strDomain.IsEmpty() && strDomain[0] == '{')
                {
                    // Check the value is one of the enums
                    __int64 sym_size, sym_addr;  // not used

                    // Get the value of the data type and make sure it is integer data
                    CHexExpr::value_t tmp = ee.get_value(ii, sym_size, sym_addr);

                    if (tmp.typ != CHexExpr::TYPE_INT)
                    {
                        ASSERT(df_size_[ii] > 0);
                        df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid

                        if (tmp.typ == CHexExpr::TYPE_NONE)
                            HandleError(CString(ee.get_error_message()) + "\nin \"domain\" attribute.");
                        else
                            HandleError("An enum domain is only valid for integer data.\n");
                    }
                    else
                    {
                        if (!add_enum(df_elt_[ii], strDomain))
                        {
                            // Syntax error in enum string
                            ASSERT(df_size_[ii] > 0);
                            df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid
                            HandleError("Invalid enumeration in \"domain\" attribute.");
                        }
                        else
                        {
                            enum_t &ev = get_enum(df_elt_[ii]);
                            if (ev.find(tmp.int64) == ev.end())
                            {
                                // Value not found
                                ASSERT(df_size_[ii] > 0);
                                df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid
                            }
                        }
                    }
                }
                else if (!strDomain.IsEmpty())
                {
                    int expr_ac;                            // Last node accessed by test expression

                    CHexExpr::value_t tmp = ee.evaluate(strDomain, ii, expr_ac);
                    if (tmp.typ != CHexExpr::TYPE_BOOLEAN || !tmp.boolean)
                    {
                        ASSERT(df_size_[ii] > 0);
                        df_size_[ii] = -df_size_[ii];  // Make -ve to indicate that it's invalid

                        if (tmp.typ == CHexExpr::TYPE_NONE)
                            HandleError(CString(ee.get_error_message()) + "\nin \"domain\" attribute.");
                        else if (tmp.typ != CHexExpr::TYPE_BOOLEAN)
                            HandleError("The \"domain\" attribute expression of the\n"
                                          "\"data\" tag must yield a boolean result.\n");
                    }
                }
            }

			// Advance address unless it was a bitfield
            if (data_type != "int" || data_bits == 0)
			{
				returned_size += mac_abs(df_size_[ii]);
				if (addr != -1)
					addr += mac_abs(df_size_[ii]);
				else
					df_address_[ii] = -1;
			}

            ASSERT(df_type_[ii] != DF_DATA);  // Ensure a specific type has been assigned

            CString strName = df_elt_[ii].GetAttr("name");
            if (!theApp.tree_edit_ && strName.IsEmpty())
            {
                // Hide nameless data elements in view mode (but show in edit mode)
                df_type_.pop_back();
                df_address_.pop_back();
                df_size_.pop_back();
                df_extra_.pop_back();
                df_indent_.pop_back();
                df_elt_.pop_back();
                df_info_.pop_back();
            }

            // Update scan progress no more than once a second
            if (length_ > 0 && addr > 0 && (clock() - m_last_checked)/CLOCKS_PER_SEC > 1 && !in_jump_)
            {
                ((CMainFrame *)AfxGetMainWnd())->m_wndStatusBar.SetPaneProgress(0, addr < length_ ? long((addr*100)/length_) : 100);
                m_last_checked = clock();
            }
        } // end of "data" processing

        // Make sure array lengths are all the same
        ASSERT(df_address_.size() == df_size_.size());
        ASSERT(df_address_.size() == df_indent_.size());
        ASSERT(df_address_.size() == df_elt_.size());
        ASSERT(df_address_.size() == df_type_.size());
        ASSERT(df_address_.size() == df_extra_.size());
        ASSERT(df_address_.size() == df_info_.size());

#ifdef _DEBUG
        if (false)
            dump_tree();    // Set IP here to dump the tree
#endif

        if (child_num != -1)
            break;          // if not -1 we are only doing a single child

        // Move to next sibling
        ++elt;
    }

	// Terminate bitfield at end unless ok to have bitfield at end (ie we are doing an array of bitfields)
	if (!ok_bitfield_at_end && bits_used_ > 0)
	{
		// Move to the end of this bitfield storage unit
		ASSERT(last_size_ != 0);
		returned_size += last_size_;
		if (addr != -1)
			addr += last_size_;
		last_size_ = 0;
		bits_used_ = 0;                     // Indicate that there is now no bitfield in effect
	}

    return last_ac;
}

void CHexEditDoc::HandleError(const char *mess)
{
    CString ss = CString(mess) + "\n\nDo you want to continue?";

    if (AfxMessageBox(ss, MB_YESNO) != IDYES)
        throw mess;
}

// Handle storage of enums for the current template

// Adds an enum for a template element - if enum for this element exists it just return true
// ee = template element whose enum map we are storing
// pp = string containing enum defns to be parsed eg "{BLACK, RED=3}
// Returns false if there is som sort of parse error
bool CHexEditDoc::add_enum(CXmlTree::CElt &ee, LPCTSTR pp)
{
    // Make sure we are doing an integer data element and we got the right enum string
    ASSERT(ee.GetName() == "data");
    ASSERT(ee.GetAttr("type") == "char" || ee.GetAttr("type") == "int");
    ASSERT(ee.GetAttr("domain") == pp);

    if (df_enum_.find((MSXML::IXMLDOMElementPtr::Interface *)ee.m_pelt) != df_enum_.end())
        return true;                          // already added

    enum_t to_add;

    // Make sure the enum string starts with the flag character {
    if (pp[0] != '{')
	{
        // Add empty enum list so we don't get run-time errors
        df_enum_[(MSXML::IXMLDOMElementPtr::Interface *)ee.m_pelt] = to_add;
		return false;
	}

	bool retval = true;

    // Find the end so we know where to stop
    char *pend = strchr(pp, '}');
    if (pend == NULL)
	{
		pend = (char *)(pp + strlen(pp));
		retval = false;         // signal error but continue parsing
	}

    // Get string with just enums in it
    CString ss(pp+1, pend-pp-1);
    CString entry;

    __int64 enum_val = 0;
    for (int ii = 0; AfxExtractSubString(entry, ss, ii, ','); ++ii, ++enum_val)
    {
        entry.TrimLeft();
        entry.TrimRight();
        if (entry.IsEmpty())
			break;     // This is not an error as C syntax allows comma after enum list

        int eq_pos;                     // Posn of equals sign in the entry
        if ((eq_pos = entry.Find('=')) != -1)
        {
            // Separate value from the name
            enum_val = _atoi64(entry.Mid(eq_pos + 1));
            entry = entry.Left(eq_pos);
            entry.TrimRight();
        }
        if (!valid_id(entry))
		{
			retval = false;
			break;
		}

        to_add[enum_val] = entry;
        TRACE2("ENUM: %s=%ld\n", entry, long(enum_val));
    }

    df_enum_[(MSXML::IXMLDOMElementPtr::Interface *)ee.m_pelt] = to_add;
    return retval;
}

// Returns the a reference to the enum map for an element.
// The element must have already had its enums added using add_enum (above)
// ee = template element whose enum map we are getting
CHexEditDoc::enum_t &CHexEditDoc::get_enum(CXmlTree::CElt &ee)
{
    ASSERT(ee.GetName() == "data");
    ASSERT(ee.GetAttr("type") == "char" || ee.GetAttr("type") == "int");
    ASSERT(df_enum_.find((MSXML::IXMLDOMElementPtr::Interface *)ee.m_pelt) != df_enum_.end());

    enum_t &retval = df_enum_.find((MSXML::IXMLDOMElementPtr::Interface *)ee.m_pelt)->second;
    return retval;
}

#ifdef _DEBUG
void CHexEditDoc::dump_tree()
{
    CString dump, ss;

    for (int ii = 0; ii < (int)df_address_.size(); ++ii)
    {
        ss.Format("%*s%5ld %5ld %-20s\r\n",
                  int(df_indent_[ii]-1), "",
                  long(df_address_[ii]),
                  long(df_size_[ii]),
                  df_elt_[ii].GetAttr("name"));

        dump += ss;
    }
    AfxMessageBox(dump);
}
#endif

// Evaluates a simple expression OR parses a string containing {expr;format} specs and
// replaces the specs with the values and returns the total resulting string.
// ss = string containing expression OR text with spec string(s)
// ee = used for evaluating expressions containing variables from the file
// ref = which element we are accessing from (used for getting variables)
// ref_ac = returned value indicating the last element accessed in the file using var in the expression(s)
CHexExpr::value_t CHexEditDoc::Evaluate(CString ss, CHexExpr &ee, int ref, int &ref_ac)
{
    // First check if this string contains format specs
    if (ss.Find('{') == -1)
    {
        // No format specs so assume it is a simple expression
        return ee.evaluate(ss, ref, ref_ac);
    }
    else
    {
        ExprStringType retval;
        ref_ac = -1;
                    
        int start, end;     // Store { and } positions for current spec
        while ((start = ss.Find('{')) != -1 && (end = ss.Find('}',start)) != -1)
        {
            // We have another spec {expr;format}
            retval += ss.Left(start);       // put out the literal bit

            // Get the member name and a format string (if any)
            CString tmp = ss.Mid(start+1, end-start-1);
            CString exprStr, formatStr;
            AfxExtractSubString(exprStr,   tmp, 0, ';');
            AfxExtractSubString(formatStr, tmp, 1, ';');

            int expr_ac;                            // Last node accessed by expression

            expr_eval::value_t val = ee.evaluate(exprStr, ref, expr_ac);
            if (val.typ == CHexExpr::TYPE_NONE)
                retval += CString("##") + ee.get_error_message();
            else
                retval += GetDataString(val, formatStr);
            if (expr_ac > ref_ac) ref_ac = expr_ac;

            ss = ss.Mid(end+1);         // Continue on the next bit of the input string
        }

        retval += ss;                   // Add any left over literal stuff

        return CHexExpr::value_t(retval);
    }
}

// Formats a value as a string
// - val is the value
// - strFormat is the format string that determines formating depending on the type of val
// - size indicates 
ExprStringType CHexEditDoc::GetDataString(expr_eval::value_t val, CString strFormat, int size /* = -1 */, bool unsgned /* = false */)
{
    // Get default format string if none given
    if (strFormat.IsEmpty())
    {
        switch (val.typ)
        {
        case CHexExpr::TYPE_INT:
            strFormat = theApp.default_int_format_;
            break;
        case CHexExpr::TYPE_DATE:
            strFormat = theApp.default_date_format_;
            break;
        case CHexExpr::TYPE_REAL:
            strFormat = theApp.default_real_format_;
            break;
        case CHexExpr::TYPE_STRING:
            strFormat = theApp.default_string_format_;
            break;
        }
    }

    CString ss;
#ifdef UNICODE_TYPE_STRING
	CStringW sw;
#endif
    char disp[128];     // Holds output of sprintf for %I64 (CString::Format can't handle it)

    switch (val.typ)
    {
    case CHexExpr::TYPE_BOOLEAN:
        if (strFormat == "f")
            ss = val.boolean ? "t" : "f";
        else if (strFormat == "F")
            ss = val.boolean ? "T" : "F";
        else if (strFormat == "false")
            ss = val.boolean ? "true" : "false";
        else if (strFormat == "n")
            ss = val.boolean ? "y" : "n";
        else if (strFormat == "N")
            ss = val.boolean ? "Y" : "N";
        else if (strFormat == "no")
            ss = val.boolean ? "yes" : "no";
        else if (strFormat == "NO")
            ss = val.boolean ? "YES" : "NO";
        else if (strFormat == "0")
            ss = val.boolean ? "1" : "0";
        else
            ss = val.boolean ? "TRUE" : "FALSE";
        break;

    case CHexExpr::TYPE_INT:
		if (size == -1) size = 2;

		// We want to make sure we display the number of digits appropriate to the actual size of
		// the value (size of operands in expression?) but not lose bits if there was overflow.
		// So for each size make sure all unused bits are the same as the top-most used bit for
		// signed types OR unused bits are all zero for unsigned types.
		if (size == 1)
		{
			if (!unsgned && (val.int64 & ~0x7Fi64) != ~0x7Fi64 && (val.int64 & ~0x7Fi64) != 0 ||
			     unsgned && (val.int64 & ~0xFFi64) != 0)
			{
				size = 2;
			}
		}
		if (size == 2)
		{
			if (!unsgned && (val.int64 & ~0x7FFFi64) != ~0x7FFFi64 && (val.int64 & ~0x7FFFi64) != 0 ||
			     unsgned && (val.int64 & ~0xFFFFi64) != 0)
			{
				size = 4;
			}
		}
		if (size == 4)
		{
			if (!unsgned && (val.int64 & ~0x7fffFFFFi64) != ~0x7fffFFFFi64 && (val.int64 & ~0x7fffFFFFi64) != 0 ||
			     unsgned && (val.int64 & ~0xffffFFFFi64) != 0)
			{
				size = 8;
			}
		}

		ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
        if (strFormat.Left(3).CompareNoCase("hex") == 0)
        {
            if (theApp.hex_ucase_)
                sprintf(disp, "%*.*I64X", size*2, size*2, val.int64);  // pass number of digits (4-bit nybbles) to sprintf
            else
                sprintf(disp, "%*.*I64x", size*2, size*2, val.int64);
            ss = disp;
            AddSpaces(ss);
        }
        else if (strFormat.Left(3).CompareNoCase("bin") == 0)
        {
            ss = bin_str(val.int64, size*8);                           // pass the number of bits to bin_str
        }
        else if (strFormat.Left(3).CompareNoCase("oct") == 0)
        {
            sprintf(disp, "%I64o", val.int64);
            ss = disp;
        }
        else if (strFormat.Find('%') == -1)
        {
            sprintf(disp, "%I64d", val.int64);
            ss = disp;
            AddCommas(ss);
        }
        else
        {
            if (strchr("diouxX", *(const char *)strFormat.Right(1)) != NULL)
                strFormat.Insert(strFormat.GetLength()-1, "I64");
            sprintf(disp, strFormat, val.int64);
            ss = disp;
        }
        break;
    case CHexExpr::TYPE_DATE:
        if (val.date > -1e30)
        {
            COleDateTime odt;
            odt.m_dt = val.date;
            odt.m_status = COleDateTime::valid;
            ss = odt.Format(strFormat);
        }
        else
            ss = "##Invalid date##";
        break;

    case CHexExpr::TYPE_REAL:
        switch (_fpclass(val.real64))
        {
        case _FPCLASS_SNAN:
        case _FPCLASS_QNAN:
            ss = "NaN";
            break;
        case _FPCLASS_NINF:
            ss = "-Inf";
            break;
        case _FPCLASS_PINF:
            ss = "+Inf";
            break;
        default:
            if (strFormat.Find('%') != -1)
                ss.Format(strFormat, val.real64);
			else if (size == 8)
                ss.Format("%.15g", val.real64);
            else
                ss.Format("%.7g", val.real64);
            break;
        }
        break;

    case CHexExpr::TYPE_STRING:
#ifdef UNICODE_TYPE_STRING
        if (strFormat.Find('%') == -1)
            sw.Format(L"%s", *val.pstr);
        else
            sw.Format(CStringW(strFormat), *val.pstr);
#else
        if (strFormat.Find('%') == -1)
            ss.Format("%s", *val.pstr);
        else
            ss.Format(strFormat, *val.pstr);
#endif
        break;
    }

#ifdef UNICODE_TYPE_STRING
	if (!ss.IsEmpty())
	{
		ASSERT(sw.IsEmpty());
		sw = CStringW(ss);
	}
	return sw;
#else
    ASSERT(!ss.IsEmpty());
    return ss;
#endif
}

// Get symbol type/value.  sym is the name of the symbol to find, parent is the element
// whose sub-elements are to be searched.  If sym is NULL then parent must refer to a
// "for" element and index is the sub-element to access.  If sym is not NULL then parent
// is a "struct" or "binary_file_format" element.  If sym is "this" just return parent.
// pac returns the number of the latest symbol (sequentially within the file) accessed.
CHexExpr::value_t CHexExpr::find_symbol(const char *sym, value_t parent, size_t index, int *pac,
                                        __int64 &sym_size, __int64 &sym_address)
{
    ASSERT(pdoc->df_address_.size() == pdoc->df_size_.size());
    ASSERT(pdoc->df_address_.size() == pdoc->df_indent_.size());
    ASSERT(pdoc->df_address_.size() == pdoc->df_elt_.size());
    ASSERT(pdoc->df_address_.size() == pdoc->df_type_.size());
    ASSERT(pdoc->df_address_.size() == pdoc->df_extra_.size());
    ASSERT(pdoc->df_address_.size() == pdoc->df_info_.size());
    ASSERT(parent.typ == TYPE_NONE || parent.typ == TYPE_STRUCT || parent.typ == TYPE_ARRAY || parent.typ == TYPE_BLOB);

    CHexExpr::value_t retval;
    size_t ii = size_t(parent.int64);
    ASSERT(ii < pdoc->df_address_.size());

    retval.typ = TYPE_NONE;             // Default to symbol not found
    retval.error = false;
    retval.int64 = 0;
    sym_size = 0;
    sym_address = -1;

    if (parent.typ == TYPE_NONE && _stricmp(sym, "this") == 0)
    {
        if (parent.int64 > *pac) *pac = int(parent.int64);
        retval = get_value(int(parent.int64), sym_size, sym_address);
    }
    else if (parent.typ == TYPE_NONE)
    {
        // Search siblings, parent siblings etc up to top level
        int curr_indent = pdoc->df_indent_[ii];

//        for (int jj = ii - 1; jj > 0; jj--)
        for (int jj = ii; jj >= 0; jj--)
        {
            if (pdoc->df_indent_[jj] < curr_indent)
            {
                ASSERT(pdoc->df_indent_[jj] == curr_indent-1);
                curr_indent = pdoc->df_indent_[jj];
            }
            if (pdoc->df_indent_[jj] == curr_indent &&
                     sym_found(sym, jj, retval, pac, sym_size, sym_address))
            {
                break;
            }
        }
    }
    else if (parent.typ == TYPE_STRUCT)
    {
        ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_STRUCT || 
               pdoc->df_type_[ii] == CHexEditDoc::DF_USE_STRUCT || 
               pdoc->df_type_[ii] == CHexEditDoc::DF_FILE);

        // Search the sub-elements of this struct
        int curr_indent = pdoc->df_indent_[ii];
        ASSERT(ii < pdoc->df_address_.size() - 1);  // There must be at least one sub-elt

        for (int jj = ii + 1; jj < (int)pdoc->df_address_.size(); ++jj)
        {
            if (pdoc->df_indent_[jj] == curr_indent)
            {
                break;                  // End of sub-elements
            }
            else if (pdoc->df_indent_[jj] == curr_indent + 1 &&
                     sym_found(sym, jj, retval, pac, sym_size, sym_address))
            {
                break;
            }
        }
    }
    else if (parent.typ == TYPE_ARRAY)
    {
        ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_FORV ||
               pdoc->df_type_[ii] == CHexEditDoc::DF_FORF);
        ASSERT(sym == NULL);
        ASSERT(ii < pdoc->df_address_.size() - 1);  // There must be at least one sub-elt

        // Check that index is not out of range
        if (index < pdoc->df_extra_[ii])
        {
            int jj;
            int curr_indent = pdoc->df_indent_[ii];

            // Scan forward for the index'th array element
            for (jj = ii + 1; jj < (int)pdoc->df_address_.size(); ++jj)
            {
                ASSERT(pdoc->df_indent_[jj] > curr_indent); // lower indents mean we've gone past array end
                if (pdoc->df_indent_[jj] == curr_indent + 1)
                    if (index-- == 0)
                        break;
            }

            // Drill down to data if nec.
            while (jj < (int)pdoc->df_address_.size() && pdoc->df_type_[jj] == CHexEditDoc::DF_IF)
                ++jj;
            ASSERT(jj < (int)pdoc->df_address_.size());

            if (jj > *pac) *pac = jj;
            retval = get_value(jj, sym_size, sym_address);
        }
    }
    else if (parent.typ == TYPE_BLOB)
    {
        ASSERT(pdoc->df_type_[ii] == CHexEditDoc::DF_NO_TYPE);  // Make sure it is a blob
        ASSERT(sym == NULL);
        ASSERT(ii < pdoc->df_address_.size() - 1);
        unsigned char val;                                      // Byte obtained from the file
        retval.typ = TYPE_INT;

		// If data element does not exist, index is past end of BLOB or just couln't read it for some reason
		if (pdoc->df_address_[ii] == -1 ||
			index > pdoc->df_size_[ii] ||
			pdoc->GetData(&val, 1, pdoc->df_address_[ii] + index) != 1)
		{
			// Set flag to say there is a problem and use null data
			retval.error = true;
			retval.int64 = -1;
		}
		else
            retval.int64 = val;
    }
    else
    {
        ASSERT(0);
    }

    return retval;
}

// Returns TRUE if the symbol is found (even if of TYPE_NONE).
// sym = name of symbol to find
// ii = index where we are looking for the symbol
// val = returned symbol value (may be TYPE_NONE)
// pac = updated to reflect the last index accessed
// sym_size = size of returned symbol
// sym_address = address of returned symbol
BOOL CHexExpr::sym_found(const char * sym, int ii, CHexExpr::value_t &val, int *pac,
                         __int64 &sym_size, __int64 &sym_address)
{
    if (ii >= (int)pdoc->df_address_.size())
        return FALSE;

    if (pdoc->df_type_[ii] == CHexEditDoc::DF_JUMP)
        return sym_found(sym, ii+1, val, pac, sym_size, sym_address);
    else if (pdoc->df_type_[ii] == CHexEditDoc::DF_IF)
    {
        ++ii;                           // Move to sub-element of IF

        // Check IF part first
        BOOL found_in_if = sym_found(sym, ii, val, pac, sym_size, sym_address);

        if (found_in_if && val.typ != TYPE_NONE && !val.error)
        {
            return TRUE;               // Found in IF part and its not empty
        }

        // Skip forward to ELSE part which must have same indentation as main IF part
        int curr_indent = pdoc->df_indent_[ii];
        ++ii;
        while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] > curr_indent)
            ++ii;

        // If no ELSE part return what we found in IF part
        if (ii == (int)pdoc->df_address_.size() || pdoc->df_indent_[ii] < curr_indent)
            return found_in_if;

        CHexExpr::value_t val2;
        int ac2;
        __int64 sym_size2, sym_address2;
        if (!sym_found(sym, ii, val2, &ac2, sym_size2, sym_address2))
            return found_in_if;
        else if (val2.typ != TYPE_NONE || !found_in_if)
        {
            // Set returned values to what we found in ELSE part
            val = val2;
            *pac = ac2;
            sym_size = sym_size2;
            sym_address = sym_address2;
            return TRUE;
        }
        else
            return found_in_if;
    }
    else if (pdoc->df_type_[ii] == CHexEditDoc::DF_SWITCH)
	{
		++ii;                                    // First case in switch is next
        int curr_indent = pdoc->df_indent_[ii];  // indent of all cases in the switch

		BOOL found = FALSE;
		while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] == curr_indent)
		{
			// There are 3 situations:
			// 1. the case is the "taken" case and the symbol of the sub-element matches sym
			//   sym_found() returns TRUE and val is valid so just return TRUE
			// 2. the symbol matches but it is not the taken case or the location is invalid (eg past EOF)
			//   sym_found() returns TRUE but val is invalid so keep looking
			// 3. the symbol does not match
			if (sym_found(sym, ii, val, pac, sym_size, sym_address))
			{
				found = TRUE;
				if (val.typ != TYPE_NONE && !val.error)
					return TRUE;               // Found the case with the valid value

				// else we continue since a following case may have the same symbol name and actually be valid
			}

			// Skip children (ie higher indent level) until next case of switch or end of switch
			++ii;
			while (ii < (int)pdoc->df_address_.size() && pdoc->df_indent_[ii] > curr_indent)
				++ii;
		}
		return found;
	}
    else if (pdoc->df_elt_[ii].GetAttr("name") == sym)
    {
        val = get_value(ii, sym_size, sym_address);
        if (ii > *pac)                  // If this symbol is further forward than any seen before ...
            *pac = ii;                  // ... update the last accessed symbol ptr
        return TRUE;
    }
    else
        return FALSE;
}

// Gets the value of an element.  For data elements the type is TYPE_INT/TYPE_STRING/TYPE_REAL/
// TYPE_DATE and the value is returned in the appropriate value field. 
// If there is no actual associated data (eg if the data is in an "if" where the test failed or
// the data is past the EOF of the physical data file) then xxx.
// For a "for" or "struct" the return type is set to TYPE_ARRAY or TYPE_STRUCT and the int64 field
// is set to the vector index (into df_type_ etc).
CHexExpr::value_t CHexExpr::get_value(int ii, __int64 &sym_size, __int64 &sym_address)
{
    value_t retval;
    ASSERT(retval.error == false);  // xxx make sure it has been inited

    sym_size = mac_abs(pdoc->df_size_[ii]);
    sym_address = pdoc->df_address_[ii];

    if (pdoc->df_type_[ii] == CHexEditDoc::DF_STRUCT || 
        pdoc->df_type_[ii] == CHexEditDoc::DF_USE_STRUCT || 
        pdoc->df_type_[ii] == CHexEditDoc::DF_FILE)
    {
        retval.typ = TYPE_STRUCT;
        retval.int64 = ii;
    }
    else if (pdoc->df_type_[ii] == CHexEditDoc::DF_FORF ||
             pdoc->df_type_[ii] == CHexEditDoc::DF_FORV)
    {
        retval.typ = TYPE_ARRAY;
        retval.int64 = ii;
    }
    else if (abs(pdoc->df_type_[ii]) < CHexEditDoc::DF_DATA)
    {
        // No actual data (may be within non-taken IF, etc)
        retval.typ = TYPE_NONE;
    }
    else
    {
        signed char df_type = abs(pdoc->df_type_[ii]);      // Data types may be -ve to indicate big endian
		bool big_endian = pdoc->df_type_[ii] < 0;
        FILE_ADDRESS df_size = mac_abs(pdoc->df_size_[ii]);

        ASSERT(df_type >= CHexEditDoc::DF_DATA);
        unsigned char buf[128];

		// Get the bytes for the data (except for a BLOB which could be huge)
		if (df_type > CHexEditDoc::DF_NO_TYPE)
		{
			if (pdoc->df_address_[ii] == -1 ||
				pdoc->GetData(buf, size_t(__min(df_size, sizeof(buf))), pdoc->df_address_[ii]) != __min(df_size, sizeof(buf)))
			{
				// Set flag to say there is a problem and use null data
				retval.error = true;
				memset(buf, '\0', sizeof(buf));
				df_size = 0;
			}
			else if (big_endian && df_type == CHexEditDoc::DF_WSTRING)
				flip_each_word(buf, df_size);      // flip bytes in each word of big-endian Unicode string
			else if (big_endian && df_type == CHexEditDoc::DF_DATESYSTEMTIME)
				flip_each_word(buf, df_size);      // flip bytes of each (2 byte) field
			else if (big_endian)
				flip_bytes(buf, size_t(df_size));  // Convert big-endian to little-endian
		}

        switch (df_type)
        {
        case CHexEditDoc::DF_DATA:
            retval.typ = TYPE_NONE;
            break;
        case CHexEditDoc::DF_NO_TYPE:
			retval.typ = TYPE_BLOB;
			retval.int64 = ii;
			break;

        case CHexEditDoc::DF_CHAR:  // no longer used as a distinct type
            ASSERT(0);
			// fall through
        case CHexEditDoc::DF_CHARA:
        case CHexEditDoc::DF_CHARN:
        case CHexEditDoc::DF_CHARO:
        case CHexEditDoc::DF_CHARE:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned char *)(buf);
            break;
        case CHexEditDoc::DF_WCHAR:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned short *)(buf);
            break;
        case CHexEditDoc::DF_STRING:  // no longer used
            ASSERT(0);
			// fall through
        case CHexEditDoc::DF_STRINGA:
        case CHexEditDoc::DF_STRINGN:
        case CHexEditDoc::DF_STRINGO:
        case CHexEditDoc::DF_STRINGE:
            retval.typ = TYPE_STRING;
            {
                // Search for string terminator
                unsigned char *pp = (unsigned char *)memchr(buf, pdoc->df_extra_[ii], size_t(df_size));
			    if (pp != NULL)
				    retval.pstr = new ExprStringType((LPCTSTR)buf, pp-buf);
			    else
				    retval.pstr = new ExprStringType((LPCTSTR)buf, size_t(df_size));  // use full length of field
            }
            break;
        case CHexEditDoc::DF_WSTRING:
            retval.typ = TYPE_STRING;
            {
                // Search for string terminator
				ASSERT(df_size%2 == 0);  // Wide string must be even no of bytes
                const wchar_t *pp = wmemchr((wchar_t *)buf, (wchar_t)pdoc->df_extra_[ii], size_t(df_size/2));
			    if (pp != NULL)
				    retval.pstr = new ExprStringType((wchar_t *)buf, pp-(wchar_t *)buf);
			    else
				    retval.pstr = new ExprStringType((wchar_t *)buf, size_t(df_size));  // use full length of field
            }
            break;
        case CHexEditDoc::DF_INT8:
            retval.typ = TYPE_INT;
            retval.int64 = *(signed char *)(buf);
            break;
        case CHexEditDoc::DF_INT16:
            retval.typ = TYPE_INT;
            retval.int64 = *(short *)(buf);
            break;
        case CHexEditDoc::DF_INT32:
            retval.typ = TYPE_INT;
            retval.int64 = *(long *)(buf);
            break;
        case CHexEditDoc::DF_INT64:
            retval.typ = TYPE_INT;
            retval.int64 = *(__int64 *)(buf);
            break;
        case CHexEditDoc::DF_UINT8:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned char *)(buf);
            break;
        case CHexEditDoc::DF_UINT16:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned short *)(buf);
            break;
        case CHexEditDoc::DF_UINT32:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned long *)(buf);
            break;
        case CHexEditDoc::DF_UINT64:
            retval.typ = TYPE_INT;
            retval.int64 = *(unsigned __int64 *)(buf);
            break;
        case CHexEditDoc::DF_MINT8:
            retval.typ = TYPE_INT;
            retval.int64 = *(signed char *)(buf);
            if (retval.int64 < 0)
                retval.int64 = -(retval.int64&0x7F);
            break;
        case CHexEditDoc::DF_MINT16:
            retval.typ = TYPE_INT;
            retval.int64 = *(short *)(buf);
            if (retval.int64 < 0)
                retval.int64 = -(retval.int64&0x7FFF);
            break;
        case CHexEditDoc::DF_MINT32:
            retval.typ = TYPE_INT;
            retval.int64 = *(long *)(buf);
            if (retval.int64 < 0)
                retval.int64 = -(retval.int64&0x7fffFFFFL);
            break;
        case CHexEditDoc::DF_MINT64:
            retval.typ = TYPE_INT;
            retval.int64 = *(__int64 *)(buf);
            if (retval.int64 < 0)
                retval.int64 = -(retval.int64&0x7fffFFFFffffFFFFi64);
            break;

		// xxx bitfields do not handle straddle yet
        case CHexEditDoc::DF_BITFIELD8:
            retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 8);
            retval.int64 = (*((unsigned char *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
            break;
        case CHexEditDoc::DF_BITFIELD16:
            retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 16);
            retval.int64 = (*((unsigned short *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
            break;
        case CHexEditDoc::DF_BITFIELD32:
            retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 32);
            retval.int64 = (*((unsigned long *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
            break;
        case CHexEditDoc::DF_BITFIELD64:
            retval.typ = TYPE_INT;
			ASSERT(signed char(pdoc->df_extra_[ii]) >= 0 && (pdoc->df_extra_[ii]&0xFF)+(pdoc->df_extra_[ii]>>8) <= 64);
            retval.int64 = (*((unsigned __int64 *)buf)>>(pdoc->df_extra_[ii]&0xFF)) & ((1<<(pdoc->df_extra_[ii]>>8))-1);
            break;

        case CHexEditDoc::DF_REAL32:
            retval.typ = TYPE_REAL;
            retval.real64 = *(float *)(buf);
            break;
        case CHexEditDoc::DF_REAL64:
            retval.typ = TYPE_REAL;
            retval.real64 = *(double *)(buf);
            break;

        case CHexEditDoc::DF_IBMREAL32:
            retval.typ = TYPE_REAL;
            retval.real64 = ::ibm_fp32(buf);
            break;
        case CHexEditDoc::DF_IBMREAL64:
            retval.typ = TYPE_REAL;
            retval.real64 = ::ibm_fp64(buf);
            break;

        case CHexEditDoc::DF_DATEC:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(*((time_t *)buf));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATEC51:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(time_t(*((time_t *)buf) + (365*10 + 2)*24L*60L*60L));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATEC7:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(time_t(*((time_t *)buf) - (365*70 + 17 + 2)*24UL*60UL*60UL));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATECMIN:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(time_t(*((time_t *)buf) * 60));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATEOLE:
            retval.typ = TYPE_DATE;
            retval.date = *((DATE *)buf);
            break;
        case CHexEditDoc::DF_DATESYSTEMTIME:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(*((SYSTEMTIME *)buf));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATEFILETIME:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(*((FILETIME *)buf));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
        case CHexEditDoc::DF_DATEMSDOS:
            {
                retval.typ = TYPE_DATE;
                FILETIME ft;
                if (DosDateTimeToFileTime(*(LPWORD(buf+2)), *(LPWORD(buf)), &ft))
                {
                    COleDateTime odt(ft);
                    retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
                }
                else
                    retval.date = -1e30;
            }
            break;
#ifdef TIME64_T // Not implemented in VC6, but is in VC7
        case CHexEditDoc::DF_DATEC64:
            {
                retval.typ = TYPE_DATE;
                COleDateTime odt(*((__time64_t *)buf));
                retval.date = (odt.m_status == 0 ? odt.m_dt : -1e30);
            }
            break;
#endif
        default:
            ASSERT(0);
            break;
        }

        // If real check that its not invalid
        if (retval.typ == TYPE_REAL)
        {
            switch (_fpclass(retval.real64))
            {
            case _FPCLASS_SNAN:
            case _FPCLASS_QNAN:
                // df_mess_ = "Invalid floating point number";
                retval.error = true;
                break;
            case _FPCLASS_NINF:
            case _FPCLASS_PINF:
                // df_mess_ = "Infinite floating point number";
                retval.error = true;
                break;
            }
        }
    }

    return retval;
}

#if 0  // This was in the DTD but caused problems
/* 
<!--
General attributes
 name = uniquely identifies this data element -- a name is not required if an element is a child of a "for" tag
 comment = can be used to describe the element or attach information that cannot be placed elsewehere
 type_name = a name that may not be unique (eg. may be name of C/C++ struct, class or typedef)

Attributes of binary_file_format
 default_byte_order = default byte order of data elements unless otherwise specified with byte_order attribute
 default_read_only = read only attribute of all data elements unless otherwsie specified with read_only
 default_char_set = which is the default character set used to display char data

Struct attributes
 name = structure name -- members are referenced using a dot then the member name (eg name.member)

For attributes
 name = array (for) name -- sub-elements are referenced using square brackets and an integer index (eg name[2])
 count = an expression that evaluates to an integer specifying the number of elements (may be zero)
 stop_test = an expression that evaluates to a Boolean (true/false) value saying this is the last element of the array
 if a count and a stop_test are specified whichever occurs first determines the size
 if neither are specified then the array goes all the way to the end of file

Data attributes
 type = any supported primitive data type (see below)
 format = with the type. length and byte_order specifies exactly how the data is stored and how big it is
 length = specifies the size if the type can have more than one size
 byte_order = big- or little-endian -- if not specified or "default" then default_byte_order is used
 read_only = determines whether these bytes can be modified -- if not specified then default_read_only is used
 domain = determines the set of valid values -- this may be individual values or ranges
 units = for numeric values this says what units are used if any
 display = determines how the values are shown to the user (see below)

Note that C/C++ printf format specifies must use the dollar character (|) rather
than the percent character (%) as XML DTDs use % for entity parameters.

                  TYPE   FORMAT  LENGTH    DISPLAY
DF_NO_TYPE        none   -       <length>  none
DF_INT8           int    -       8         C/C++ printf format specifiers (%d, %x, %X etc)
DF_INT16          int    -       16
DF_INT32          int    -       -
DF_INT64          int    -       64
DF_UINT8          int    u       8
DF_UINT16         int    u       16
DF_UINT32         int    u       -
DF_UINT64         int    u       64
DF_MINT8          int    m       8
DF_MINT16         int    m       16
DF_MINT32         int    m       -
DF_MINT64         int    m       64
DF_CHAR           char   -
DF_CHARA          char   -       ascii
DF_CHARN          char   -       ansi
DF_CHARO          char   -       oem
DF_CHARE          char   -       ebcdic
DF_WCHAR          char   -       unicode
DF_STRING         string -       <length>
DF_STRINGA        string ascii   <length>
DF_STRINGN        string ansi    <length>
DF_STRINGO        string oem     <length>
DF_STRINGE        string ebcdic  <length>
DF_WSTRING        string unicode <length>
DF_REAL32         real   -       32        C/C++ printf format specifiers (%g, %e, %f etc)
DF_REAL64         real   -       -
DF_IBMREAL32      real   ibm     32
DF_IBMREAL64      real   ibm     -
DF_DATEC          date   -       c         C/C++ strftime format (%c, %#c, %#x etc)
DF_DATEC51        date   -       c51
DF_DATEC7         date   -       c7
DF_DATECMIN       date   -       cmin
DF_DATEOLE        date   -       ole
DF_DATESYSTEMTIME date   -       systemtime
DF_DATEFILETIME   date   -       filetime
DF_DATEMSDOS      date   -       msdos
DF_DATEC          date   -       c64
-->
*/
#endif




