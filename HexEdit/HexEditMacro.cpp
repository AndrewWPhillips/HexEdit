// HexEditMacro.cpp : Stuff for keystroke macros - part of CHexEditApp
//
// Copyright (c) 2000-2010 by Andrew W. Phillips.
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

#include "MainFrm.h"
#include "ChildFrm.h"
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "DataFormatView.h"
#include "Bookmark.h"
#include "timer.h"
#include "SystemSound.h"
#include "misc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

/////////////////////////////////////////////////////////////////////////////

void CHexEditApp::macro_play(long play_times /*=1*/, const std::vector<key_macro> *pmac /*=NULL*/, int halt_lev/*=-1*/)
{
	TRACE0("============== macro_play ======================\n");
	// Fix default parameter values
	if (pmac == NULL) pmac = &mac_;
	if (halt_lev == -1) halt_lev = halt_level_;

	ASSERT(!recording_);                 // We probably don't want to record while playing
	ASSERT(pmac->size() > 0);
	ASSERT(km_last < 256);
	ASSERT(km_last_global <= km_calc);
	ASSERT(km_last_calc <= km_view);

	if (pmac->size() == 0) return;
	std::vector <key_macro, allocator<key_macro> >::const_iterator pk;

	++playing_;

	CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
	CChildFrame *pc;                    // Ptr to the child (view's) frame
	timer time(true);                   // Timer tracking time between refreshes
	long plays_done;                    // Number of times macro replayed so far
	long keys_done = 0;                 // Number of "keystrokes" replayed so far

	// Clear previous errors
	mac_error_ = 0;

	// There should be a better way to clear previous macro status bar messages
	bool bb = refresh_off_;             // Save current refresh status
	refresh_off_ = true;                // Turn off refresh so that we clear previous saved messages
	((CMainFrame *)AfxGetMainWnd())->StatusBarText("");
	refresh_off_ = bb;                  // Restore refresh status

	// Turn off refresh unless refreshing after every keystroke
	if (!(refresh_ == 2 && num_keys_ == 1))
	{
		refresh_off_ = true;
		disable_carets();
	}

	CWaitCursor wait;
	ASSERT(pv_ == NULL || playing_ > 1); // pv_ should always be NULL at start of macro
	CHexEditView *psaved_view = pv_;
	pv_ = GetView();                    // Ptr to the view (if any)
	CHexEditView *plast_view = pv_;     // Active view at start or last refresh

	for (plays_done = 0; plays_done < play_times; ++plays_done)
	{
		for (pk = pmac->begin(); pk != pmac->end(); ++pk, ++keys_done)
		{
			int operand_size;           // Size of operands for operations (0,1,2, or 3)

			if (AbortKeyPress() &&
				TaskMessageBox("Abort macro?", 
					"You have interrupted the running of the keystroke macro.  "
					"You may now stop the macro or continue running it.\n\n"
					"Do you want to stop the macro?",MB_YESNO) == IDYES)
			{
				((CMainFrame *)AfxGetMainWnd())->StatusBarText("Macro aborted");
				mac_error_ = 10;
				goto exit_play;
			}

			if ((*pk).ktype > km_view && pv_ == NULL)
			{
				TaskMessageBox("No File Open",
					"There are command(s) in the current macro that require a file to be "
					"currently open to execute.  The macro cannot continue.",
					0, 0, MAKEINTRESOURCE(IDI_CROSS));
				((CMainFrame *)AfxGetMainWnd())->StatusBarText("Macro terminated");
				mac_error_ = 10;
				goto exit_play;
			}

			TRACE("=== MACRO PLAY %d %d\r\n", (int)(*pk).ktype, (int)(*pk).v64);
			ASSERT(sizeof(*pk) == 9);
			switch ((*pk).ktype)
			{
			// Handled by the app
			case km_new:
				// Get options
				theApp.last_fill_state_ = (*pk).v64;
				theApp.last_fill_str_.Empty();
				{
					std::vector <key_macro, allocator<key_macro> >::const_iterator pknext = pk;
					++pknext;
					if ((*pknext).ktype == km_new_str)
					{
						theApp.last_fill_str_ = *(*pknext).pss;
						++pk;
					}
				}
				OnFileNew();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();
				break;
			case km_open:
				ASSERT(theApp.open_current_readonly_ == -1);
				theApp.OpenDocumentFile(*(*pk).pss);
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();
				break;
			case km_exit:
				OnAppExit();
				break;
			case km_print_setup:
				OnFilePrintSetup();
				break;
			case km_prop:
				OnProperties();
				break;
			case km_prop_file:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_file))
						pp->prop_file.UpdateWindow();
						// xxx else macro error? (page not available)
				}
				break;
			case km_prop_char:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_char))
						pp->prop_char.UpdateWindow();
				}
				break;
			case km_prop_dec:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_dec))
						pp->prop_dec.UpdateWindow();
				}
				break;
			case km_prop_float:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_real))
						pp->prop_real.UpdateWindow();
				}
				break;
			case km_prop_date:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_date))
						pp->prop_date.UpdateWindow();
				}
				break;
			case km_prop_stats:
				{
					OnProperties();
					CPropSheet *pp = &((CMainFrame *)AfxGetMainWnd())->m_wndProp;
					if (pp->SetActivePage(&pp->prop_graph))
						pp->prop_graph.UpdateWindow();
				}
				break;
			case km_prop_close:
				((CMainFrame *)AfxGetMainWnd())->m_wndProp.ShowWindow(SW_HIDE);
				break;
			case km_tips:
				ShowTipOfTheDay();
				break;
			case km_macro_message:
				if (AfxMessageBox(*(*pk).pss, MB_OKCANCEL) != IDOK)
					mac_error_ = 5;
				break;
			case km_macro_play:
				play_macro_file(*(*pk).pss);
				break;
			case km_bookmarks_goto:
				{
					CBookmarkList *pbl = theApp.GetBookmarkList();
					pbl->GoTo(*(*pk).pss);
					pv_ = NULL;    // make sure GetView gets real active view
					plast_view = pv_ = GetView();  // Bookmark may be in another file -> changes focus
				}
				break;

			// Handled by the main frame window
			case km_find_text:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.NewSearch(*(*pk).pss);
				((CMainFrame *)AfxGetMainWnd())->AddSearchHistory(*(*pk).pss);
				break;
			case km_replace_text:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.NewReplace(*(*pk).pss);
				((CMainFrame *)AfxGetMainWnd())->AddReplaceHistory(*(*pk).pss);
				break;
			case km_find_next:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnFindNext();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // May change active view (if search all open files is on)
				break;
			case km_bookmark_all:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnBookmarkAll(); // xxx needs to restore basename in use at recording
				break;
			case km_replace:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnReplace();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // May change active view (if search all open files is on)
				break;
			case km_replace_all:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnReplaceAll();
				break;
			case km_find_forw:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnSearchForw();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // May change active view (if search all open files is on)
				break;
			case km_find_back:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnSearchBack();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // May change active view (if search all open files is on)
				break;
			case km_find_sel:
				((CMainFrame *)AfxGetMainWnd())->m_wndFind.SetOptions((*pk).vv);
				((CMainFrame *)AfxGetMainWnd())->OnSearchSel();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // May change active view (if search all open files is on)
				break;
			case km_find_dlg:
				if ((*pk).vv == -1)
				{
					((CMainFrame *)AfxGetMainWnd())->m_paneFind.Hide();
				}
				else
				{
					((CMainFrame *)AfxGetMainWnd())->m_paneFind.ShowAndUnroll();
					((CMainFrame *)AfxGetMainWnd())->m_wndFind.ShowPage((*pk).vv);
				}
				break;
			case km_find_close:  // Replaced by km_find_dlg with -1 as param
				// Keep this for now for backward compatibility with old macro files
				((CMainFrame *)AfxGetMainWnd())->m_paneFind.Hide();
				break;

			case km_toolbar:
				if ((*pk).vv == 1)
					((CMainFrame *)AfxGetMainWnd())->OnViewViewbar();
				else if ((*pk).vv == 2)
					((CMainFrame *)AfxGetMainWnd())->OnViewEditbar();
				// Note: 3 was used for old toolbar (m_wndBarOld)
				else if ((*pk).vv == 4)
					((CMainFrame *)AfxGetMainWnd())->OnViewFormatbar();
				else if ((*pk).vv == 5)
					((CMainFrame *)AfxGetMainWnd())->OnViewNavbar();
				else if ((*pk).vv == 10)
					((CMainFrame *)AfxGetMainWnd())->OnViewCalculator();
				else if ((*pk).vv == 11)
					((CMainFrame *)AfxGetMainWnd())->OnViewBookmarks();
				else if ((*pk).vv == 12)
					((CMainFrame *)AfxGetMainWnd())->OnViewFind();
				else if ((*pk).vv == 13)
					((CMainFrame *)AfxGetMainWnd())->OnViewProperties();
				else if ((*pk).vv == 14)
					((CMainFrame *)AfxGetMainWnd())->OnViewExpl();
				else if ((*pk).vv == 15)
					((CMainFrame *)AfxGetMainWnd())->OnViewRuler();
				else if ((*pk).vv == 16)
					((CMainFrame *)AfxGetMainWnd())->OnViewHighlightCaret();
				else if ((*pk).vv == 17)
					((CMainFrame *)AfxGetMainWnd())->OnViewHighlightMouse();
				else
					ASSERT(0);
				break;
			case km_bar:
				((CMainFrame *)AfxGetMainWnd())->OnBarCheck((*pk).vv);
				break;
			case km_topics:
				((CMainFrame *)AfxGetMainWnd())->OnHelpFinder();
				break;
			case km_help:
				((CMainFrame *)AfxGetMainWnd())->OnHelp();
				break;
			case km_focus:
				{
					CChildFrame *nextc;        // Loops through all MDI child frames

					for (nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
						 nextc != NULL;
						 nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
					{
						CString ss;
						nextc->GetWindowText(ss);
						if (ss.Right(2) == " *")
							ss = ss.Left(ss.GetLength() - 2);
						if (ss == *(*pk).pss)
							break;
					}
					if (nextc == NULL)
					{
						CString ss;
						ss.Format("The window with title \"%s\" was not found.  "
							      "The window may have been closed.", (const char *)*(*pk).pss);
						TaskMessageBox("Window Not Found", ss);
						mac_error_ = 10;
					}
					else
					{
						//((CMainFrame *)AfxGetMainWnd())->MDIActivate(nextc);
						// Get the view associated with this child frame
						pv_ = nextc->GetHexEditView();
						ASSERT_KINDOF(CHexEditView, pv_);
					}

				}
				break;
			case km_mainsys:
				// Send message to main frame (see also km_childsys)
				((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_SYSCOMMAND, (*pk).vv, 0L);
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();  // Next window command may change active view
				break;

			// Handled by the view's frame (child frame window)
			case km_childsys:
				// Get active child frame
				pc = (CChildFrame *)((CMainFrame *)AfxGetMainWnd())->MDIGetActive();
				if (pc == NULL)
				{
					TaskMessageBox("No File Open",
						"There are command(s) in the current macro that require a file to be "
						"currently open to execute.  The macro cannot continue.");
					mac_error_ = 10;
					goto exit_play;
				}
				else
					pc->SendMessage(WM_SYSCOMMAND, (*pk).vv, 0L);
				break;

			case km_close:
				pv_->GetDocument()->OnFileClose();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();
				break;
			case km_save:
				pv_->GetDocument()->OnFileSave();
				break;
			case km_saveas:
				pv_->GetDocument()->OnFileSaveAs();
				break;
			case km_print:
				pv_->OnFilePrint();
				break;
			case km_preview:
				pv_->OnFilePrintPreview();
				break;

			case km_undo:
				pv_->OnEditUndo();
				break;
			case km_undo_changes:
				pv_->OnUndoChanges();
				break;
			case km_cut:
				pv_->OnEditCut();
				break;
			case km_copy:
				pv_->OnEditCopy();
				break;
			case km_paste:
				pv_->OnEditPaste();
				break;
			case km_del:
				pv_->OnDel();
				break;
			case km_copy_hex:
				pv_->OnCopyHex();
				break;
			case km_copy_cchar:
				pv_->do_copy_src((*pk).vv&0x07,
								((*pk).vv>>3)&0x07,
								((*pk).vv>>6)&0x07,
								((*pk).vv&0x200) != 0,
								((*pk).vv&0x400) == 0,
								int(((*pk).v64>>11)&0x7F) );
				break;
			case km_paste_ascii:
				pv_->OnPasteAscii();
				break;
			case km_paste_unicode:
				pv_->OnPasteUnicode();
				break;
			case km_paste_ebcdic:
				pv_->OnPasteEbcdic();
				break;
			case km_sel_all:
				pv_->OnSelectAll();
				break;
			case km_sel_line:
				pv_->OnSelectLine();
				break;
			case km_write_file:
				// 0 is used for writing binary data, 1 ... are used for export formats
				switch ((*pk).vv)
				{
				case 0:
					pv_->OnEditWriteFile();
					break;
				case 1:
					pv_->OnExportSRecord(ID_EXPORT_S1);
					break;
				case 2:
					pv_->OnExportSRecord(ID_EXPORT_S2);
					break;
				case 3:
					pv_->OnExportSRecord(ID_EXPORT_S3);
					break;
				case 7:
					pv_->OnExportIntel();
					break;
				case 10:
					pv_->OnExportHexText();
					break;
				default:
					ASSERT(0);
				}
				break;
			case km_append_file:
				pv_->OnEditAppendFile();
				break;
			case km_append_same_file:
				pv_->OnEditAppendSameFile();
				break;
			case km_read_file:
	//      pv_->OnReadFile();
				pv_->do_read(*(*pk).pss);
				break;
			case km_import_motorola:
				pv_->do_motorola(*(*pk).pss);
				break;
			case km_import_intel:
				pv_->do_intel(*(*pk).pss);
				break;
			case km_import_text:
				pv_->do_hex_text(*(*pk).pss);
				break;
			case km_insert:
				{
					std::vector <key_macro, allocator<key_macro> >::const_iterator pknext = pk;
					++pknext;
					if ((*pknext).ktype == km_insert_str)
					{
						pv_->do_insert_block((*pk).v64, *(*pknext).pss);
						++pk;
					}
				}
				break;
			case km_goto:
				((CMainFrame *)AfxGetMainWnd())->OnEditGoto((*pk).vv);
				break;
			case km_calc_dlg:
				if ((*pk).vv == -1)
				{
					// Hide it
					ASSERT(((CMainFrame *)AfxGetMainWnd())->m_paneCalc.m_hWnd != 0);
					((CMainFrame *)AfxGetMainWnd())->m_paneCalc.Hide();
				}
				else if ((*pk).vv == 0)
					((CMainFrame *)AfxGetMainWnd())->OnCalculator();
				else
					((CMainFrame *)AfxGetMainWnd())->OnCalcSel();
				break;
			case km_seed:
				if ((*pk).vv == 1)
					pv_->OnSeedCalc();
				else
					pv_->OnSeedRandom();
				break;
			case km_calc_close:     // Replaced with km_calc_dlg with param -1 (see above)
				// No longer generated but handle in case we run a really old macro file
				ASSERT(((CMainFrame *)AfxGetMainWnd())->m_paneCalc.m_hWnd != 0);
				((CMainFrame *)AfxGetMainWnd())->m_paneCalc.Hide();
				break;
			case km_ro_rw:
				pv_->OnAllowMods();
				break;
			case km_ovr_ins:
				pv_->OnInsert();
				break;
			case km_track_changes:
				pv_->OnTrackChanges();
				break;
			case km_big_endian:
				if ((*pk).vv == -1)
					pv_->OnToggleEndian();
				else if ((*pk).vv == 1)
					pv_->OnBigEndian();
				else
					pv_->OnLittleEndian();
				break;
			case km_mark_pos:
				pv_->OnMark();
				break;
			case km_goto_mark:
				pv_->OnGotoMark();
				break;
			case km_extendto_mark:
				pv_->OnExtendToMark();
				break;
			case km_swap_mark:
				pv_->OnSwapMark();
				break;
			case km_highlight:
				switch((*pk).vv)
				{
				case 0:
					pv_->OnHighlightClear();
					break;
				case 1:
					pv_->OnHighlight();
					break;
				case 2:
					pv_->OnHighlightPrev();
					break;
				case 3:
					pv_->OnHighlightNext();
					break;
				case 4:
					pv_->OnHighlightHide();
					break;
				case 5:
					pv_->OnHighlightSelect();
					break;
				}
				break;
			case km_bookmarks:
				switch((*pk).vv)
				{
				case 0:
					pv_->GetDocument()->ClearBookmarks();
					break;
				case 2:
					pv_->OnBookmarksPrev();
					break;
				case 3:
					pv_->OnBookmarksNext();
					break;
				case 4:
					pv_->OnBookmarksHide();
					break;
				case 5:  // show
					((CMainFrame *)AfxGetMainWnd())->m_paneBookmarks.ShowAndUnroll();
					break;
				case 6: // hide
					((CMainFrame *)AfxGetMainWnd())->m_paneBookmarks.Hide();
					break;
				case 7:
					pv_->OnBookmarkToggle();
					break;
				default:
					ASSERT(0);
				}
				break;
			case km_display_reset:
				pv_->OnDisplayReset();
				break;
			case km_autofit:
				pv_->do_autofit((*pk).vv);
				break;
			case km_font:
				pv_->do_font((*pk).plf);
				break;
			case km_inc_font:
				pv_->OnFontInc();
				break;
			case km_dec_font:
				pv_->OnFontDec();
				break;
			case km_addr:
				pv_->OnAddrToggle();
				break;
			case km_area:
				switch ((*pk).vv)
				{
				case 1:
					pv_->OnDisplayHex();
					break;
				case 2:
					pv_->OnDisplayChar();
					break;
				case 3:
					pv_->OnDisplayBoth();
					break;
				case 4:
					pv_->OnDisplayStacked();
					break;
				default:
					ASSERT(0);
				}
				break;
			case km_char_area:
				pv_->do_chartoggle((*pk).vv);
				break;
			case km_hex_area:
				pv_->do_hextoggle((*pk).vv);
				break;
			case km_display_state:
				pv_->do_change_state((*pk).vv);
				break;
			case km_charset:
				switch ((*pk).vv)
				{
				case 0:
					pv_->OnCharsetAscii();
					break;
				case 1:
					pv_->OnCharsetAnsi();
					break;
				case 2:
					pv_->OnCharsetOem();
					break;
				case 3:
					pv_->OnCharsetEbcdic();
					break;
				default:
					ASSERT(0);
				}
				break;
			case km_ebcdic:
				pv_->OnAscEbc();
				break;
			case km_control:
				switch ((*pk).vv)
				{
				case 1:
					pv_->OnControlNone();    // Off
					break;
				case 2:
					pv_->OnControlAlpha();   // Alpha control chars
					break;
				case 3:
					pv_->OnControlC();       // C escape seq chars
					break;
				case 99:
					pv_->OnControlToggle();  // Toggle on/off
					break;
				default:
					pv_->OnControl();        // 3-way toggle
					break;
				}
				break;
			case km_control2:
				pv_->OnControlToggle();      // 3-way toggle (phasing out)
				break;
			case km_graphic:
				pv_->OnGraphicToggle();
				break;
			case km_oem:
				pv_->OnOemToggle();
				break;
			case km_dffd_sync:
				switch ((*pk).vv)
				{
				case 2:
					pv_->OnDffdAutoSync();
					break;
				case 255:
					pv_->OnDffdSync();
					break;
				}
				break;

			case km_scheme:
				pv_->SetScheme(*(*pk).pss);
				break;
			case km_rowsize:
				if ((*pk).vv == -1)
					pv_->OnColumnDec();
				else if ((*pk).vv == -2)
					pv_->OnColumnInc();
				else
				{
					ASSERT((*pk).vv >= 4);
					pv_->change_rowsize((*pk).vv);
				}
				break;
			case km_group_by:
				pv_->change_group_by((*pk).vv);
				break;
			case km_offset:
				pv_->change_offset((*pk).vv);
				break;

			case km_win_next:
				//pv_->OnWindowNext(); -- this is now done here for speed (avoid actual focus change)
				{
					CHexEditView * pp = pv_->NextView();

					if (pp == NULL)
					{
						TaskMessageBox("No Other Windows", "There were no other non-minimized to switch to.  "
							           "The same window is active as before.");
						mac_error_ = 2;
					}
					else
						pv_ = pp;
				}
				break;
			case km_win_new:
				((CMainFrame *)AfxGetMainWnd())->OnWindowNew();
				pv_ = NULL;    // make sure GetView gets real active view
				plast_view = pv_ = GetView();
				break;
			case km_win_cmd:
				((CMainFrame *)AfxGetMainWnd())->OnMDIWindowCmd((*pk).vv);
				break;

			case km_redraw:
				pv_->OnRedraw();
				break;
			case km_scroll_up:
				pv_->OnScrollUp();
				break;
			case km_scroll_down:
				pv_->OnScrollDown();
				break;
			case km_swap_areas:
				pv_->OnSwap();
				break;
			case km_start_line:
				pv_->OnStartLine();
				break;

			case km_key:
				pv_->MovePos((*pk).vv & 0xFFFF, 1, (*pk).vv & 0x10000, (*pk).vv & 0x20000, TRUE);
				break;
			case km_char:
				pv_->do_char((*pk).vv);
				break;
			case km_mouse:
				pv_->do_mouse((*pk).pms->dev_down, (*pk).pms->doc_dist);
				break;
			case km_shift_mouse:
				pv_->do_shift_mouse((*pk).pms->dev_down, (*pk).pms->doc_dist);
				break;
			case km_hscroll:
				pv_->OnScroll(MAKEWORD((*pk).vv>>16, -1), (*pk).vv & 0xFFFF);
				break;
			case km_vscroll:
				pv_->OnScroll(MAKEWORD(-1, (*pk).vv>>16), (*pk).vv & 0xFFFF);
				break;

			case km_address_tool:
				// No longer created (replaced by km_address_hex && km_address_dec)
				// but we will leave in for now, in case old macro files use it
				pv_->MoveWithDesc("Jump Tool", (*pk).v64);
				break;
			case km_address_hex:
				((CMainFrame *)AfxGetMainWnd())->SetHexAddress(*(*pk).pss);
				pv_->OnJumpHex();
				break;
			case km_address_dec:
				((CMainFrame *)AfxGetMainWnd())->SetDecAddress(*(*pk).pss);
				pv_->OnJumpDec();
				break;

			case km_compare:
				pv_->OnEditCompare();
				break;

			// Operations
			case km_op_unary:
				operand_size = int((*pk).v64 & 0xFF);
				ASSERT(operand_size >= 0 && operand_size <= 3);
				switch ((*pk).v64>>8)
				{
				case unary_inc:
					switch (operand_size)
					{
					case 0:
						pv_->OnIncByte();
						break;
					case 1:
						pv_->OnInc16bit();
						break;
					case 2:
						pv_->OnInc32bit();
						break;
					case 3:
						pv_->OnInc64bit();
						break;
					}
					break;
				case unary_dec:
					switch (operand_size)
					{
					case 0:
						pv_->OnDecByte();
						break;
					case 1:
						pv_->OnDec16bit();
						break;
					case 2:
						pv_->OnDec32bit();
						break;
					case 3:
						pv_->OnDec64bit();
						break;
					}
					break;
				case unary_sign:
					switch (operand_size)
					{
					case 0:
						pv_->OnNegByte();
						break;
					case 1:
						pv_->OnNeg16bit();
						break;
					case 2:
						pv_->OnNeg32bit();
						break;
					case 3:
						pv_->OnNeg64bit();
						break;
					}
					break;
				case unary_flip:
					switch (operand_size)
					{
					case 0:
						ASSERT(0);  // There is no flip byte command
						break;
					case 1:
						pv_->OnFlip16bit();
						break;
					case 2:
						pv_->OnFlip32bit();
						break;
					case 3:
						pv_->OnFlip64bit();
						break;
					}
					break;
				case unary_not:
					pv_->OnInvert();
					break;

				case unary_at:    // Used for Operations/Assign
					switch (operand_size)
					{
					case 0:
						pv_->OnAssignByte();
						break;
					case 1:
						pv_->OnAssign16bit();
						break;
					case 2:
						pv_->OnAssign32bit();
						break;
					case 3:
						pv_->OnAssign64bit();
						break;
					}
					break;
				case unary_rand:   // Random number generation
					switch (operand_size)
					{
					case 0:
						pv_->OnRandByte();
						break;
					case 8:
						pv_->OnRandFast();  // for macros recorded before 3.0 (replaced with unary_rand_fast)
						break;
					}
					break;
				case unary_rand_fast:   // Fast random number generation
					pv_->OnRandFast();
					break;

				case unary_rev:
					switch (operand_size)
					{
					case 0:
						pv_->OnRevByte();
						break;
					case 1:
						pv_->OnRev16bit();
						break;
					case 2:
						pv_->OnRev32bit();
						break;
					case 3:
						pv_->OnRev64bit();
						break;
					}
					break;

				default:
					ASSERT(0);
				}
				break;

			case km_op_binary:
				operand_size = int((*pk).v64 & 0xFF);
				ASSERT(operand_size >= 0 && operand_size <= 3);
				switch ((*pk).v64>>8)
				{
				case binop_xor:
					switch (operand_size)
					{
					case 0:
						pv_->OnXorByte();
						break;
					case 1:
						pv_->OnXor16bit();
						break;
					case 2:
						pv_->OnXor32bit();
						break;
					case 3:
						pv_->OnXor64bit();
						break;
					}
					break;

				case binop_and:
					switch (operand_size)
					{
					case 0:
						pv_->OnAndByte();
						break;
					case 1:
						pv_->OnAnd16bit();
						break;
					case 2:
						pv_->OnAnd32bit();
						break;
					case 3:
						pv_->OnAnd64bit();
						break;
					}
					break;

				case binop_or:
					switch (operand_size)
					{
					case 0:
						pv_->OnOrByte();
						break;
					case 1:
						pv_->OnOr16bit();
						break;
					case 2:
						pv_->OnOr32bit();
						break;
					case 3:
						pv_->OnOr64bit();
						break;
					}
					break;

				case binop_add:
					switch (operand_size)
					{
					case 0:
						pv_->OnAddByte();
						break;
					case 1:
						pv_->OnAdd16bit();
						break;
					case 2:
						pv_->OnAdd32bit();
						break;
					case 3:
						pv_->OnAdd64bit();
						break;
					}
					break;

				case binop_subtract:
					switch (operand_size)
					{
					case 0:
						pv_->OnSubtractByte();
						break;
					case 1:
						pv_->OnSubtract16bit();
						break;
					case 2:
						pv_->OnSubtract32bit();
						break;
					case 3:
						pv_->OnSubtract64bit();
						break;
					}
					break;

				case binop_gtr:
					switch (operand_size)
					{
					case 0:
						pv_->OnGtrByte();
						break;
					case 1:
						pv_->OnGtr16bit();
						break;
					case 2:
						pv_->OnGtr32bit();
						break;
					case 3:
						pv_->OnGtr64bit();
						break;
					}
					break;

				case binop_less:
					switch (operand_size)
					{
					case 0:
						pv_->OnLessByte();
						break;
					case 1:
						pv_->OnLess16bit();
						break;
					case 2:
						pv_->OnLess32bit();
						break;
					case 3:
						pv_->OnLess64bit();
						break;
					}
					break;

				case binop_gtr_unsigned:
					switch (operand_size)
					{
					case 0:
						pv_->OnGtrUByte();
						break;
					case 1:
						pv_->OnGtrU16bit();
						break;
					case 2:
						pv_->OnGtrU32bit();
						break;
					case 3:
						pv_->OnGtrU64bit();
						break;
					}
					break;

				case binop_less_unsigned:
					switch (operand_size)
					{
					case 0:
						pv_->OnLessUByte();
						break;
					case 1:
						pv_->OnLessU16bit();
						break;
					case 2:
						pv_->OnLessU32bit();
						break;
					case 3:
						pv_->OnLessU64bit();
						break;
					}
					break;

				case binop_multiply:
					switch (operand_size)
					{
					case 0:
						pv_->OnMulByte();
						break;
					case 1:
						pv_->OnMul16bit();
						break;
					case 2:
						pv_->OnMul32bit();
						break;
					case 3:
						pv_->OnMul64bit();
						break;
					}
					break;

				case binop_divide:
					switch (operand_size)
					{
					case 0:
						pv_->OnDivByte();
						break;
					case 1:
						pv_->OnDiv16bit();
						break;
					case 2:
						pv_->OnDiv32bit();
						break;
					case 3:
						pv_->OnDiv64bit();
						break;
					}
					break;

				case binop_mod:
					switch (operand_size)
					{
					case 0:
						pv_->OnModByte();
						break;
					case 1:
						pv_->OnMod16bit();
						break;
					case 2:
						pv_->OnMod32bit();
						break;
					case 3:
						pv_->OnMod64bit();
						break;
					}
					break;

				case binop_subtract_x:
					switch (operand_size)
					{
					case 0:
						pv_->OnSubtractXByte();
						break;
					case 1:
						pv_->OnSubtractX16bit();
						break;
					case 2:
						pv_->OnSubtractX32bit();
						break;
					case 3:
						pv_->OnSubtractX64bit();
						break;
					}
					break;

				case binop_divide_x:
					switch (operand_size)
					{
					case 0:
						pv_->OnDivXByte();
						break;
					case 1:
						pv_->OnDivX16bit();
						break;
					case 2:
						pv_->OnDivX32bit();
						break;
					case 3:
						pv_->OnDivX64bit();
						break;
					}
					break;

				case binop_mod_x:
					switch (operand_size)
					{
					case 0:
						pv_->OnModXByte();
						break;
					case 1:
						pv_->OnModX16bit();
						break;
					case 2:
						pv_->OnModX32bit();
						break;
					case 3:
						pv_->OnModX64bit();
						break;
					}
					break;

				case binop_rol:
					switch (operand_size)
					{
					case 0:
						pv_->OnRolByte();
						break;
					case 1:
						pv_->OnRol16bit();
						break;
					case 2:
						pv_->OnRol32bit();
						break;
					case 3:
						pv_->OnRol64bit();
						break;
					}
					break;
				case binop_ror:
					switch (operand_size)
					{
					case 0:
						pv_->OnRorByte();
						break;
					case 1:
						pv_->OnRor16bit();
						break;
					case 2:
						pv_->OnRor32bit();
						break;
					case 3:
						pv_->OnRor64bit();
						break;
					}
					break;
				case binop_lsl:
					switch (operand_size)
					{
					case 0:
						pv_->OnLslByte();
						break;
					case 1:
						pv_->OnLsl16bit();
						break;
					case 2:
						pv_->OnLsl32bit();
						break;
					case 3:
						pv_->OnLsl64bit();
						break;
					}
					break;
				case binop_lsr:
					switch (operand_size)
					{
					case 0:
						pv_->OnLsrByte();
						break;
					case 1:
						pv_->OnLsr16bit();
						break;
					case 2:
						pv_->OnLsr32bit();
						break;
					case 3:
						pv_->OnLsr64bit();
						break;
					}
					break;
				case binop_asr:
					switch (operand_size)
					{
					case 0:
						pv_->OnAsrByte();
						break;
					case 1:
						pv_->OnAsr16bit();
						break;
					case 2:
						pv_->OnAsr32bit();
						break;
					case 3:
						pv_->OnAsr64bit();
						break;
					}
					break;

				default:
					ASSERT(0);
				}
				break;

			case km_checksum:
				switch ((*pk).v64)
				{
				case CHECKSUM_8:
					pv_->OnChecksum8();
					break;
				case CHECKSUM_16:
					pv_->OnChecksum16();
					break;
				case CHECKSUM_32:
					pv_->OnChecksum32();
					break;
				case CHECKSUM_64:
					pv_->OnChecksum64();
					break;

				case CHECKSUM_CRC16:
					pv_->OnCrc16();
					break;
				case CHECKSUM_CRC_CCITT:
					pv_->OnCrcCcitt();
					break;
				case CHECKSUM_CRC_CCITT_B:
					pv_->OnCrcCcittB();
					break;
				case CHECKSUM_CRC_XMODEM:
					pv_->OnCrcXmodem();
					break;
				case CHECKSUM_CRC32:
					pv_->OnCrc32();
					break;

				case CHECKSUM_MD5:
					pv_->OnMd5();
					break;
				case CHECKSUM_SHA1:
					pv_->OnSha1();
					break;
				default:
					ASSERT(0);
				}
				break;


			// Old macros commands - leave for now, so that old macro files still work
			case km_invert:
				pv_->OnInvert();
				break;
			case km_neg8:
				pv_->OnNegByte();
				break;
			case km_neg16:
				pv_->OnNeg16bit();
				break;
			case km_neg32:
				pv_->OnNeg32bit();
				break;
			case km_neg64:
				pv_->OnNeg64bit();
				break;
			case km_inc8:
				pv_->OnIncByte();
				break;
			case km_inc16:
				pv_->OnInc16bit();
				break;
			case km_inc32:
				pv_->OnInc32bit();
				break;
			case km_inc64:
				pv_->OnInc64bit();
				break;
			case km_dec8:
				pv_->OnDecByte();
				break;
			case km_dec16:
				pv_->OnDec16bit();
				break;
			case km_dec32:
				pv_->OnDec32bit();
				break;
			case km_dec64:
				pv_->OnDec64bit();
				break;
			case km_flip16:
				pv_->OnFlip16bit();
				break;
			case km_flip32:
				pv_->OnFlip32bit();
				break;
			case km_flip64:
				pv_->OnFlip64bit();
				break;

			case km_convert:
				switch ((*pk).vv)
				{
				case CONVERT_ASC2EBC: 
					pv_->OnAscii2Ebcdic();
					break;
				case CONVERT_EBC2ASC: 
					pv_->OnEbcdic2Ascii();
					break;
				case CONVERT_ANSI2IBM:
					pv_->OnAnsi2Ibm();
					break;
				case CONVERT_IBM2ANSI:
					pv_->OnIbm2Ansi();
					break;
				case CONVERT_UPPER:
					pv_->OnUppercase();
					break;
				case CONVERT_LOWER:
					pv_->OnLowercase();
					break;
				default:
					ASSERT(0);
				}
				break;

			// Encryption
			case km_encrypt_alg:
				((CHexEditApp *)AfxGetApp())->set_alg(*(*pk).pss);
				break;
			case km_encrypt_password:
				((CHexEditApp *)AfxGetApp())->set_password(*(*pk).pss);
				break;
			case km_encrypt:
				switch ((*pk).vv)
				{
				case 1:  // encrypt
					pv_->OnEncrypt();
					break;
				case 2:  // decrypt
					pv_->OnDecrypt();
					break;
				default:
					ASSERT(0);
				}
				break;
			case km_compress:
				switch ((*pk).vv)
				{
				case 0:
					((CHexEditApp *)AfxGetApp())->OnCompressionSettings();
					break;
				case 1:
					pv_->OnCompress();
					break;
				case 2:
					pv_->OnDecompress();
					break;
				default:
					ASSERT(0);
				}
				break;

			case km_bookmarks_add:
				{
					CBookmarkList *pbl = theApp.GetBookmarkList();
					int ii = pbl->AddBookmark(*(*pk).pss);
				}
				break;
			case km_clear:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnClear();
				break;
			case km_clear_entry:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnClearEntry();
				break;
			case km_equals:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnEquals();
				break;
			case km_endian:
				// This is just to allow old macros to run (used to call CalcDlg::toggle_endian())
				if (pv_ != NULL) // Since km_endian < km_view pv_ can be NULL
					pv_->OnToggleEndian();
				break;
			case km_change_bits:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_bits((*pk).vv);
				break;
			case km_change_base:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_base((*pk).vv);
				break;
			case km_binop:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.do_binop(binop_type((*pk).vv));
				break;
			case km_unaryop:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.do_unary(unary_type((*pk).vv));
				break;
			case km_user:
				{
					// User enetered number (up to 64 bits)
					mpz_class tmp;
					mpz_set_ui64(tmp.get_mpz_t(), (*pk).v64);
					((CMainFrame *)AfxGetMainWnd())->m_wndCalc.Set(tmp);
				}
				break;
			case km_user_str:
				{
					// User entered integer (unlimited bits)
					mpz_class tmp;
					mpz_set_str(tmp.get_mpz_t(), (*(*pk).pss), 10);     // stored in string using radix 10
					((CMainFrame *)AfxGetMainWnd())->m_wndCalc.Set(tmp);
				}
				break;
			case km_expression:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.SetStr((*(*pk).pss));
				break;
			case km_memget:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMemGet();
				break;
			case km_markget:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkGet();
				break;
			case km_markat:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkAt();
				break;
			case km_selget:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelGet();
				break;
			case km_selat:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelAt();
				break;
			case km_sellen:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelLen();
				break;
			case km_eofget:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnEofGet();
				break;
			case km_memstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMemStore();
				break;
			case km_memclear:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMemClear();
				break;
			case km_memadd:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMemAdd();
				break;
			case km_memsubtract:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMemSubtract();
				break;
			case km_markstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkStore();
				break;
			case km_markclear:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkClear();
				break;
			case km_markadd:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkAdd();
				break;
			case km_marksubtract:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkSubtract();
				break;
			case km_markatstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnMarkAtStore();
				break;
			case km_selstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelStore();
				break;
			case km_go:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnGo();
				break;
			case km_selatstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelAtStore();
				break;
			case km_sellenstore:
				((CMainFrame *)AfxGetMainWnd())->m_wndCalc.OnSelLenStore();
				break;

			default:
				ASSERT(0);
			} // switch

			// Check for termination here
			ASSERT(halt_lev == 0 || halt_lev == 1 || halt_lev == 2);
			if (mac_error_ > halt_lev)
			{
#ifdef SYS_SOUNDS
				CSystemSound::Play("Macro Error");
#endif
				// Display an appropriate error
				if (mac_error_ > 10)
					TaskMessageBox("Major Error", "A System Error was encountered during macro execution", 0, 0, MAKEINTRESOURCE(IDI_CROSS));
				else if (mac_error_  > 2)
				{
					// Just display mess in status bar as dlg box has already been seen
					TaskMessageBox("Macro Error", "An error was encountered during macro execution", 0, 0, MAKEINTRESOURCE(IDI_CROSS));
				}
				else if (mac_error_ > 1)
					TaskMessageBox("Minor Error", "A minor error was encountered during macro execution.");
				else if (mac_error_ > 0)
					TaskMessageBox("Macro Warning", "A warning was encountered during macro execution.");

				goto exit_play;
			}

			// Check if its time to refresh the display
			if (refresh_off_ &&
				((refresh_ == 1 && time.elapsed() > num_secs_) ||
				 (refresh_ == 2 && ((keys_done+1) % num_keys_) == 0) ||
				 (refresh_ == 3 && ((plays_done+1) % num_plays_) == 0 && pk == pmac->end()-1) ) )
			{
				if (pv_ != plast_view)
				{
					// Activate the current view
					pv_->GetFrame()->MDIActivate();
					pv_->GetFrame()->SetActiveView(pv_);
					plast_view = pv_;
					refresh_display(true);
				}
				else
					refresh_display(false);

				// If refreshing based on time: reset the timer for next period
				if (refresh_ == 1) time.reset();
			}
			else if (refresh_ == 2 && num_keys_ == 1 && refresh_bars_)
			{
				// If refresh every keystroke refresh is not turned off but
				// some things don't get updated till OnIdle called so force
				// it here (since we're not idle while playing).
				ASSERT(!refresh_off_);

				AfxGetApp()->OnIdle(0);

				// Find dialog
				mm->m_wndFind.UpdateWindow();
			}

			// Restore wait cursor if there is any chance a dialog was displayed etc
			if (mac_error_ > 0)
			{
				wait.Restore();         // Restore busy cursor in case a dialog has messed it up
				mac_error_ = 0;         // Reset error so we don't keep restoring after first warning
			}
		} // for keys
	} // for plays

#ifdef SYS_SOUNDS
	CSystemSound::Play("Macro Finished");
#endif

	if (mac_error_ > 1)
		TaskMessageBox("Minor Error", "A minor error was encountered during macro execution.");
	else if (mac_error_ > 0)
		TaskMessageBox("Macro Warning", "A warning was encountered during macro execution.");//        ((CMainFrame *)AfxGetMainWnd())->

exit_play:
	// If finished playing and display refresh was off refresh the display
	if (playing_ <= 1 && refresh_off_)
	{
		if (pv_ != plast_view)
		{
			pv_->GetFrame()->MDIActivate();
			pv_->GetFrame()->SetActiveView(pv_);
		}
		refresh_display(true);
		refresh_off_ = bb;
		mm->m_wndCalc.UpdateData(FALSE);  // Update base/bits radio buttons etc
		// xxx Make sure current calc. value is displayed
		enable_carets();
	}

	// Keep track of play nesting
	playing_--;
	ASSERT(playing_ >= 0); // should never go -ve

	pv_ = psaved_view;
	ASSERT(pv_ == NULL || playing_ > 0);
}

void CHexEditApp::disable_carets()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	CChildFrame *nextc;        // Loops through all MDI child frames

	// Disable caret in all view windows
	for (nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
		 nextc != NULL;
		 nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
	{
		CHexEditView *pv = nextc->GetHexEditView();
		ASSERT(pv != NULL);
		ASSERT_KINDOF(CHexEditView, pv);
		pv->DisableCaret();
	}
}

void CHexEditApp::enable_carets()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	CChildFrame *nextc;        // Loops through all MDI child frames

	// Enable carets in all view windows
	for (nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
		 nextc != NULL;
		 nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
	{
		CHexEditView *pv = nextc->GetHexEditView();
		ASSERT(pv != NULL);
		ASSERT_KINDOF(CHexEditView, pv);
		pv->EnableCaret();
	}
}

void CHexEditApp::refresh_display(bool do_all /*=false*/)
{
	CHexEditView *pactive;              // Ptr to the active view (if any)
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	// Refresh status bar even if there are no windows open
	if (do_all || refresh_bars_)
	{
		mm->StatusBarText(NULL);                        // Show latest status bar message
#if 0 // BCG toolbar changes
		mm->m_wndStatusBar.OnUpdateCmdUI(mm, TRUE);     // Update other panes
		mm->m_wndStatusBar.UpdateWindow();              // Make sure window redrawn
#else
		AfxGetApp()->OnIdle(0);
#endif
	}

	// Update the calculator
	if ((do_all || refresh_bars_) && mm->m_paneCalc.IsWindowVisible())
	{
		//mm->m_wndCalc.UpdateData(FALSE);  // Update base/bits radio buttons etc
		//mm->m_wndCalc.edit_.Put();        // Make sure current calc. value is displayed
		mm->m_wndCalc.update_controls();
		mm->m_wndCalc.button_colour(mm->m_wndCalc.GetDlgItem(IDC_MEM_GET), mm->m_wndCalc.memory_ != 0, RGB(0x40, 0x40, 0x40));

		mm->m_wndCalc.UpdateWindow();     // Make sure all these nice updates are seen
	}

	// Refresh of other things only makes sense if there is a window open
	if ((pactive = GetView()) != NULL)
	{
		// Update properties dialog?
		if (do_all || refresh_props_)
		{
			pactive->show_prop(-2);
		}

		if (do_all || refresh_bars_)
		{
			// Update the hex/decimal address tools
			pactive->show_pos(-2);

			// Update the find dialog box
			mm->m_wndFind.UpdateWindow();
		}

		if (do_all)
		{
			CChildFrame *nextc;        // Loops through all MDI child frames

			// Invalidate all view windows
			for (nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
				 nextc != NULL;
				 nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
			{
//                CHexEditView *pv = (CHexEditView *)nextc->GetActiveView();
//                if (pv != NULL && pv->IsKindOf(RUNTIME_CLASS(CHexEditView)) && pv != pactive)
				CHexEditView *pv = nextc->GetHexEditView();
				if (pv != NULL && pv != pactive)
				{
					pv->EnableCaret();
					pv->DisplayCaret();
					pv->DoUpdate();
					pv->DisableCaret();
				}
			}
		}
		pactive->EnableCaret();
		pactive->DisplayCaret();
		pactive->DoUpdate();
		pactive->DisableCaret();
	}
}

struct file_header
{
	long plays;                         // Default number of plays
	long version;                       // Version of hexedit the macro was created in (see INTERNAL_VERSION)
	char dummy[16];                     // Unused (should be zero)
	short halt_lev;                     // Default halt level to use with the macro
	char comment[2];                    // First byte of description of this macro
};

BOOL CHexEditApp::macro_save(const char *filename, const std::vector<key_macro> *pmac /*=NULL*/,
							 const char *comment /*= NULL*/, int halt_lev /*=-1*/, long plays /*=1*/, int version /*=0*/)
{
	unsigned short magic, header_len;   // Magic number to check for valid files and header length

	// Fix default parameter values
	if (comment == NULL) comment = "";
	if (pmac == NULL) pmac = &mac_;
	if (halt_lev == -1) halt_lev = halt_level_;

	// Get memory for the header
	magic = 0xCDAB;
	header_len = sizeof(file_header) + strlen(comment);  // Includes 2 extra bytes for '\0' and '\xCD'
	char *pp = new char[header_len];
	file_header *ph = (file_header *)pp;

	// Build the header
	memset(pp, '\0', header_len);
	pp[header_len-1] = '\xCD';
	ph->halt_lev = halt_lev;
	ph->plays = plays;
	ph->version = version;
	strcpy(ph->comment, comment);
	ASSERT(pp[header_len-2] == '\0' && pp[header_len-1] == '\xCD');

	// Write the file
	try
	{
		short str_len;                  // Length of string for km_find_text etc
		ASSERT(pmac->size() > 0);
		std::vector <key_macro, allocator<key_macro> >::const_iterator pk;

		CFile ff(filename, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

		ff.Write(&magic, sizeof(magic));
		ff.Write(&header_len, sizeof(header_len));
		ff.Write(pp, header_len);              // Write header

		for (pk = pmac->begin(); pk != pmac->end(); ++pk)
		{
			ASSERT(sizeof((*pk)) == 9);
			ff.Write(&(*pk).ktype, sizeof((*pk).ktype));
			switch ((*pk).ktype)
			{
				// These "keystrokes" have an associated string
			case km_find_text:
			case km_replace_text:
			case km_open:
			case km_macro_message:
			case km_macro_play:
			case km_address_hex:
			case km_address_dec:
			case km_encrypt_alg:
			case km_encrypt_password:
			case km_read_file:
			case km_import_motorola:
			case km_import_intel:
			case km_import_text:
			case km_bookmarks_add:
			case km_bookmarks_goto:
			case km_new_str:
			case km_insert_str:
			case km_focus:
			case km_scheme:
			case km_expression:
				// Write the string length
				str_len = (*(*pk).pss).GetLength();
				ff.Write(&str_len, sizeof(str_len));
				ff.Write((const char *)(*(*pk).pss), str_len + 1);
				break;
			case km_mouse:
			case km_shift_mouse:
				ff.Write((*pk).pms, sizeof(*(*pk).pms));
				break;
			case km_font:
				ff.Write(&(*pk).plf->lfHeight, sizeof((*pk).plf->lfHeight));
				ff.Write(&(*pk).plf->lfCharSet, sizeof((*pk).plf->lfCharSet));
				str_len = strlen((*pk).plf->lfFaceName);
				ff.Write(&str_len, sizeof(str_len));
				ff.Write((*pk).plf->lfFaceName, str_len + 1);
				break;
			default:
				ff.Write(&(*pk).v64, 8);
				break;
			}
		}

	}
	catch (CFileException *pfe)
	{
		TaskMessageBox("Macro File Error", FileErrorMessage(pfe, CFile::modeWrite));
		pfe->Delete();
		mac_error_ = 10;
		delete[] pp;
		return FALSE;
	}

	delete[] pp;
	return TRUE;
}

BOOL CHexEditApp::macro_load(const char *filename, std::vector<key_macro> *pmac,
							 CString &comment, int &halt_lev, long &plays, int &version)
{
	pmac->clear();                      // Empty macro ready to build new one

	try
	{
		unsigned short magic;           // Special number to check file is what we expect
		unsigned short header_len;      // Length of header
		CFileException *pfe;            // This is thrown if we reach EOF before expected

		// Create the thing to be thrown if we reach EOF unexpectedly.
		// Note: the object must be created on the heap as CException::m_bAutoDelete
		// is always TRUE (default CException constructor is called which sets it TRUE).
		pfe = new CFileException(CFileException::endOfFile, -1, filename);

		CFile ff(filename, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary);

		if (ff.Read(&magic, sizeof(magic)) < sizeof(magic)) throw pfe;
		if (magic != 0xCDAB)
		{
				TaskMessageBox("Invalid Macro File",
					"This is not a HexEdit macro file or it has become corrupted.",
					0, 0, MAKEINTRESOURCE(IDI_CROSS));
			pfe->Delete();
			mac_error_ = 10;
			return FALSE;
		}

		// Read the header length
		if (ff.Read(&header_len, sizeof(header_len)) < sizeof(header_len)) throw pfe;


		// Now read the whole header
		char *pp = new char[header_len];       // Memory to hold header
		if (ff.Read(pp, header_len) < header_len) throw pfe;
		file_header *ph = (file_header *)pp;
		ASSERT(pp[header_len-2] == '\0' && pp[header_len-1] == '\xCD');

		comment = ph->comment;
		halt_lev = ph->halt_lev;
		plays = ph->plays;
		version = ph->version;

		if (version > INTERNAL_VERSION &&
			TaskMessageBox("Version Warning",
						   CString("This macro (") + filename + ")\n"
						  "was recorded in a later version of HexEdit "
						  "than you are currently using.  "
						  "It may not run as expected.\n\n"
						  "Do you want to continue?",
						  MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP) != IDYES)
		{
			return FALSE;
		}

		// Now read the "keystrokes" until EOF
		unsigned char kk;               // "key" type
		unsigned __int64 v64;           // Value associated with normal keys
		short str_len;                  // Length of string for km_find_text etc
		char *buf = NULL;               // Buffer for reading strings
		mouse_sel ms;                   // Info for km_mouse/km_shift_mouse
		LOGFONT lf;                     // Info use to create a new font for km_font

		memset(&lf, '\0', sizeof(lf));  // Make sure all fields are zero since we only set name and height

		while (ff.Read(&kk, sizeof(kk)) == sizeof(kk))
		{
			switch (km_type(kk))
			{
			case km_find_text:
			case km_replace_text:
			case km_open:
			case km_macro_message:
			case km_macro_play:
			case km_address_hex:
			case km_address_dec:
			case km_encrypt_alg:
			case km_encrypt_password:
			case km_read_file:
			case km_import_motorola:
			case km_import_intel:
			case km_import_text:
			case km_bookmarks_add:
			case km_bookmarks_goto:
			case km_new_str:
			case km_insert_str:
			case km_focus:
			case km_scheme:
			case km_expression:
				// Get the string length and allocate memory for the string
				if (ff.Read(&str_len, sizeof(str_len)) < sizeof(str_len)) throw pfe;
				buf = new char[str_len+1];

				// Read the string and add to the macro
				if (ff.Read(buf, str_len+1) < (UINT)str_len+1) throw pfe;
				ASSERT(buf[str_len] == '\0');
				pmac->push_back(key_macro(km_type(kk), CString(buf)));

				// Free the memory
				delete[] buf;
				buf = NULL;
				break;
			case km_mouse:
			case km_shift_mouse:
				// Get the mouse data and add to the macro
				if (ff.Read(&ms, sizeof(ms)) < sizeof(ms)) throw pfe;
				pmac->push_back(key_macro(km_type(kk), &ms));
				break;
			case km_font:
				// Read the font height and char set
				if (ff.Read(&lf.lfHeight, sizeof(lf.lfHeight)) < sizeof(lf.lfHeight)) throw pfe;
				if (ff.Read(&lf.lfCharSet, sizeof(lf.lfCharSet)) < sizeof(lf.lfCharSet)) throw pfe;

				// Read font name length, allocate memory for the string and read it in 
				if (ff.Read(&str_len, sizeof(str_len)) < sizeof(str_len)) throw pfe;
				buf = new char[str_len+1];
				if (ff.Read(buf, str_len+1) < (UINT)str_len+1) throw pfe;
				ASSERT(buf[str_len] == '\0');

				// Create font and add to macro
				if (lf.lfHeight < 2 || lf.lfHeight > 100)
					lf.lfHeight = 16;
				strncpy(lf.lfFaceName, buf, LF_FACESIZE-1);
				lf.lfFaceName[LF_FACESIZE-1] = '\0';
				pmac->push_back(key_macro(km_type(kk), &lf));

				// Free the memory
				delete[] buf;
				buf = NULL;
				break;
			default:
				ff.Read(&v64, 8);
				pmac->push_back(key_macro(km_type(kk), v64));
				break;
			}
		}

		delete[] pp;
		pfe->Delete();
	}
	catch (CFileException *pfe)
	{
		TaskMessageBox("Macro File Error", FileErrorMessage(pfe, CFile::modeRead));
		pfe->Delete();
		mac_error_ = 10;
		return FALSE;
	}

	return TRUE;
}
