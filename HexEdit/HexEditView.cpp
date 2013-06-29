// HexEditView.cpp : implementation of the CHexEditView class
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
#include <boost/hash.hpp>  // This is for digests (SHA2 etc) see below for notes on where it came from
#include "HexEdit.h"
#include "HexFileList.h"
#include <imagehlp.h>       // For ::MakeSureDirectoryPathExists()

#include "HexEditDoc.h"
#include "HexEditView.h"
#include "DataFormatView.h"
#include "AerialView.h"
#include "CompareView.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "Dialog.h"
#include "Bookmark.h"
#include "NewFile.h"
#include "Password.h"
#include "GeneralCRC.h"

#include "Boyer.h"
#include "SystemSound.h"
#include "Misc.h"
#include "BCGMisc.h"
#include "SRecord.h"      // For import/export of Motorola S record files
#include "IntelHex.h"     // For import/export of Intel Hex files
#include "CopyCSrc.h"     // For Copy as C Source dialog
#include "zlib/zlib.h"    // For compression
#include "md5.h"          // For MD5 hash
#include "sha1.h"         // For SHA1 hash
#include "Bin2Src.h"      // For formatted clipboard text
#include "HelpID.hm"

#ifdef _DEBUG
#include "timer.h"
#endif

// This #define allows checking that the drawing code does clipping properly for
// efficiency.  For example for very long lines we don't want to draw a huge no
// of byte values to the right and/or left of the display area.  It does this by
// moving the borders in and turning off the clipping rectangle use in OnDraw
// (see GetDisplayRect) and also drawing red border lines in OnEraseBkgnd.
// Also note that when in vert_display (stacked) mode that up to 3 lines can be
// drawn past the top and bottom of the display area.
//#define TEST_CLIPPING  1

// A note on the selection:
// 1. The selection is a contiguous set of bytes with a start address and a length.
//    When a discontiguous "selection" is required we use the highlighter.
// 2. The actual selection is kept track of in the base class (ScrView) members
//    caretpos_ and selpos_.  SetCaret is used to set the caret with no selection
//    (ie selection length s zero) so caretpos_ == selpos_.  There is also basepos_
//    which is the start of the selection (ie either == caretpos_ or selpos_).
// 3. At the basic level the selection is set/got using CSrcView::GetSel and SetSel,
//    which are pixel based.  Since CScrView and its selection are text oriented there
//    is also GetTSel and SetTSel which are character based.
// 4. CScrView allows the caret to be at the start or end of the selection, which is
//    why SetSel takes start/end positions plus a bool to says which end has the caret.
// 5. SetSel calls ValidateCaret to find the closest valid caret position for the
//    start and end positions.
// 6. CHexEditView::GetSelAddr/SetSelAddr get/set the selection in terms of file
//    addresses, by using addr2pos/pos2addr to convert between pixels and file addresses
//    before calling CScrView::GetSel/SetSel.
//    Similarly GetPos is the equivalent of GetCaret but returns a file address.
// 7. To handle stacked mode addr2pos takes a row as well as an address.  The row
//    can be a value of 0, 1, or 2 for the 3 rows when in stacked mode.
// 8. At a higher level GoAddress will set the selection first making sure
//    its parameters are a valid range.
// 9. At an even higher level MoveToAddress allows setting the selection while
//    updating everything else for consistency including:
//   - scrolling the display to show the selection
//   - storing undo information so the move can be undone
//   - keeping track of nav points
//   - updating jump tools with new address
//   - updating properties dialog with value at caret, etc

#define MAX_CLIPBOARD    32000000L  // Just a rough guideline - may make an option later

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CHexEditApp theApp;

//BYTE CHexEditFontCombo::saved_charset = 147;  // (see BCGMisc.h) initially 147 to not match 
											  // anything -> force initial rebuild of the font list
static bool in_recalc_display = false;

/////////////////////////////////////////////////////////////////////////////
// CHexEditView

IMPLEMENT_DYNCREATE(CHexEditView, CScrView)

BEGIN_MESSAGE_MAP(CHexEditView, CScrView)

		//{{AFX_MSG_MAP(CHexEditView)
		ON_WM_SIZE()
		ON_COMMAND(ID_ADDR_TOGGLE, OnAddrToggle)
		ON_UPDATE_COMMAND_UI(ID_ADDR_TOGGLE, OnUpdateAddrToggle)
		ON_COMMAND(ID_GRAPHIC_TOGGLE, OnGraphicToggle)
		ON_UPDATE_COMMAND_UI(ID_GRAPHIC_TOGGLE, OnUpdateGraphicToggle)
		ON_COMMAND(ID_CHAR_TOGGLE, OnCharToggle)
		ON_UPDATE_COMMAND_UI(ID_CHAR_TOGGLE, OnUpdateCharToggle)
		ON_COMMAND(ID_FONT, OnFont)
		ON_COMMAND(ID_AUTOFIT, OnAutoFit)
		ON_UPDATE_COMMAND_UI(ID_AUTOFIT, OnUpdateAutofit)
		ON_COMMAND(ID_ASC_EBC, OnAscEbc)
		ON_UPDATE_COMMAND_UI(ID_ASC_EBC, OnUpdateAscEbc)
		ON_COMMAND(ID_CONTROL, OnControl)
		ON_UPDATE_COMMAND_UI(ID_CONTROL, OnUpdateControl)
		ON_WM_CHAR()
		ON_WM_SETFOCUS()
		ON_WM_KILLFOCUS()
		ON_WM_DESTROY()
		ON_WM_LBUTTONDOWN()
		ON_COMMAND(ID_MARK, OnMark)
		ON_COMMAND(ID_GOTO_MARK, OnGotoMark)
		ON_WM_LBUTTONDBLCLK()
		ON_WM_KEYDOWN()
		ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
		ON_COMMAND(ID_SEARCH_HEX, OnSearchHex)
		ON_COMMAND(ID_SEARCH_ASCII, OnSearchAscii)
		ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
		ON_COMMAND(ID_ALLOW_MODS, OnAllowMods)
		ON_UPDATE_COMMAND_UI(ID_ALLOW_MODS, OnUpdateAllowMods)
		ON_COMMAND(ID_CONTROL_TOGGLE, OnControlToggle)
		ON_COMMAND(ID_INSERT, OnInsert)
		ON_UPDATE_COMMAND_UI(ID_INSERT, OnUpdateInsert)
		ON_COMMAND(ID_SEARCH_ICASE, OnSearchIcase)
		ON_COMMAND(ID_EDIT_COMPARE, OnEditCompare)
		ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COMPARE, OnUpdateEditCompare)
		ON_WM_CONTEXTMENU()
		ON_WM_RBUTTONDOWN()
		ON_COMMAND(ID_INC_BYTE, OnIncByte)
		ON_COMMAND(ID_INC_16BIT, OnInc16bit)
		ON_COMMAND(ID_INC_32BIT, OnInc32bit)
		ON_COMMAND(ID_INC_64BIT, OnInc64bit)
		ON_COMMAND(ID_DEC_BYTE, OnDecByte)
		ON_COMMAND(ID_DEC_16BIT, OnDec16bit)
		ON_COMMAND(ID_DEC_32BIT, OnDec32bit)
		ON_COMMAND(ID_DEC_64BIT, OnDec64bit)
		ON_COMMAND(ID_FLIP_16BIT, OnFlip16bit)
		ON_COMMAND(ID_FLIP_32BIT, OnFlip32bit)
		ON_COMMAND(ID_FLIP_64BIT, OnFlip64bit)
		ON_UPDATE_COMMAND_UI(ID_INC_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_INC_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_INC_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_INC_64BIT, OnUpdate64bit)
		ON_WM_LBUTTONUP()
		ON_COMMAND(ID_SELECT_ALL, OnSelectAll)
		ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
		ON_COMMAND(ID_EDIT_CUT, OnEditCut)
		ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
		ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateTextPaste)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateClipboard)
		ON_UPDATE_COMMAND_UI(ID_PASTE_UNICODE, OnUpdateUnicodePaste)
		ON_WM_SETCURSOR()
		ON_COMMAND(ID_FONT_DEC, OnFontDec)
		ON_COMMAND(ID_FONT_INC, OnFontInc)
		ON_UPDATE_COMMAND_UI(ID_FONT_DEC, OnUpdateFontDec)
		ON_UPDATE_COMMAND_UI(ID_FONT_INC, OnUpdateFontInc)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
		ON_COMMAND(ID_PASTE_ASCII, OnPasteAscii)
		ON_COMMAND(ID_PASTE_EBCDIC, OnPasteEbcdic)
		ON_COMMAND(ID_PASTE_UNICODE, OnPasteUnicode)
		ON_COMMAND(ID_COPY_CCHAR, OnCopyCchar)
		ON_COMMAND(ID_COPY_HEX, OnCopyHex)
		ON_COMMAND(ID_EDIT_WRITEFILE, OnEditWriteFile)
		ON_UPDATE_COMMAND_UI(ID_EDIT_READFILE, OnUpdateReadFile)
		ON_COMMAND(ID_EDIT_READFILE, OnReadFile)
		ON_WM_HSCROLL()
		ON_WM_VSCROLL()
		ON_COMMAND(ID_EXTENDTO_MARK, OnExtendToMark)
		ON_COMMAND(ID_SWAP_MARK, OnSwapMark)
		ON_COMMAND(ID_REDRAW, OnRedraw)
		ON_COMMAND(ID_SCROLL_DOWN, OnScrollDown)
		ON_COMMAND(ID_SCROLL_UP, OnScrollUp)
		ON_COMMAND(ID_SWAP, OnSwap)
		ON_COMMAND(ID_START_LINE, OnStartLine)
		ON_COMMAND(ID_DEL, OnDel)
		ON_UPDATE_COMMAND_UI(ID_SWAP, OnUpdateSwap)
		ON_COMMAND(ID_OEM_TOGGLE, OnOemToggle)
		ON_UPDATE_COMMAND_UI(ID_OEM_TOGGLE, OnUpdateOemToggle)
		ON_WM_MOUSEWHEEL()
		ON_COMMAND(ID_INVERT, OnInvert)
		ON_COMMAND(ID_NEG_BYTE, OnNegByte)
		ON_COMMAND(ID_NEG_16BIT, OnNeg16bit)
		ON_COMMAND(ID_NEG_32BIT, OnNeg32bit)
		ON_COMMAND(ID_NEG_64BIT, OnNeg64bit)
		ON_WM_MOUSEMOVE()
		ON_WM_KEYUP()
		ON_COMMAND(ID_HIGHLIGHT, OnHighlight)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT, OnUpdateHighlight)
		ON_COMMAND(ID_HIGHLIGHT_CLEAR, OnHighlightClear)
		ON_COMMAND(ID_HIGHLIGHT_PREV, OnHighlightPrev)
		ON_COMMAND(ID_HIGHLIGHT_NEXT, OnHighlightNext)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_PREV, OnUpdateHighlightPrev)
		ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_NEXT, OnUpdateHighlightNext)
		ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
	ON_COMMAND(ID_ASC2EBC, OnAscii2Ebcdic)
	ON_UPDATE_COMMAND_UI(ID_ASC2EBC, OnUpdateConvert)
	ON_COMMAND(ID_EBC2ASC, OnEbcdic2Ascii)
	ON_COMMAND(ID_ANSI2IBM, OnAnsi2Ibm)
	ON_COMMAND(ID_IBM2ANSI, OnIbm2Ansi)
	ON_COMMAND(ID_ENCRYPT_ENCRYPT, OnEncrypt)
	ON_COMMAND(ID_ENCRYPT_DECRYPT, OnDecrypt)
	ON_UPDATE_COMMAND_UI(ID_ENCRYPT_ENCRYPT, OnUpdateEncrypt)
	ON_COMMAND(ID_EDIT_APPENDFILE, OnEditAppendFile)
	ON_COMMAND(ID_EDIT_APPENDSAMEFILE, OnEditAppendSameFile)
	ON_UPDATE_COMMAND_UI(ID_EDIT_APPENDSAMEFILE, OnUpdateEditAppendSameFile)
	ON_COMMAND(ID_UNDO_CHANGES, OnUndoChanges)
	ON_UPDATE_COMMAND_UI(ID_UNDO_CHANGES, OnUpdateUndoChanges)
	ON_COMMAND(ID_CALC_SEL, OnCalcSel)
	ON_UPDATE_COMMAND_UI(ID_CALC_SEL, OnUpdateCalcSel)
	ON_COMMAND(ID_DISPLAY_RESET, OnDisplayReset)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_IMPORT_MOTOROLA_S, OnImportMotorolaS)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_MOTOROLA_S, OnUpdateImportMotorolaS)
	ON_COMMAND(ID_IMPORT_INTEL, OnImportIntel)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_INTEL, OnUpdateImportIntel)
	ON_COMMAND(ID_EXPORT_INTEL, OnExportIntel)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_INTEL, OnUpdateExportIntel)
	ON_COMMAND(ID_IMPORT_HEX_TEXT, OnImportHexText)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_HEX_TEXT, OnUpdateImportHexText)
	ON_COMMAND(ID_EXPORT_HEX_TEXT, OnExportHexText)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_HEX_TEXT, OnUpdateExportHexText)
	ON_COMMAND(ID_CRC16, OnCrc16)
	ON_COMMAND(ID_CRC32, OnCrc32)
	ON_COMMAND(ID_CRC32_MPEG2, OnCrc32Mpeg2)
	ON_COMMAND(ID_CRC_GENERAL, OnCrcGeneral)
	ON_COMMAND(ID_CRC_CCITT_F, OnCrcCcittF)
	ON_COMMAND(ID_CRC_CCITT_AUG, OnCrcCcittAug)
	ON_COMMAND(ID_CRC_CCITT_T, OnCrcCcittT)
	ON_COMMAND(ID_CRC_XMODEM, OnCrcXmodem)
	ON_COMMAND(ID_BOOKMARKS_HIDE, OnBookmarksHide)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_HIDE, OnUpdateBookmarksHide)
	ON_COMMAND(ID_HIGHLIGHT_HIDE, OnHighlightHide)
	ON_UPDATE_COMMAND_UI(ID_HIGHLIGHT_HIDE, OnUpdateHighlightHide)
	ON_COMMAND(ID_BOOKMARKS_NEXT, OnBookmarksNext)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_NEXT, OnUpdateBookmarksNext)
	ON_COMMAND(ID_BOOKMARKS_PREV, OnBookmarksPrev)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_PREV, OnUpdateBookmarksPrev)
	ON_COMMAND(ID_BOOKMARKS_CLEAR, OnBookmarksClear)
	ON_UPDATE_COMMAND_UI(ID_BOOKMARKS_CLEAR, OnUpdateBookmarksClear)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_BOOKMARK_TOGGLE, OnBookmarkToggle)
	ON_COMMAND(ID_COLUMN_DEC, OnColumnDec)
	ON_UPDATE_COMMAND_UI(ID_COLUMN_DEC, OnUpdateColumnDec)
	ON_COMMAND(ID_COLUMN_INC, OnColumnInc)
	ON_UPDATE_COMMAND_UI(ID_COLUMN_INC, OnUpdateColumnInc)
	ON_COMMAND(ID_DFFD_AUTO_SYNC, OnDffdAutoSync)
	ON_UPDATE_COMMAND_UI(ID_DFFD_AUTO_SYNC, OnUpdateDffdAutoSync)
	ON_COMMAND(ID_DFFD_SYNC, OnDffdSync)
	ON_UPDATE_COMMAND_UI(ID_DFFD_SYNC, OnUpdateDffdSync)
	ON_COMMAND(ID_RAND_SEED_CALC, OnSeedCalc)
	ON_COMMAND(ID_RAND_SEED, OnSeedRandom)
		ON_UPDATE_COMMAND_UI(ID_CONTROL_TOGGLE, OnUpdateControl)
		ON_UPDATE_COMMAND_UI(ID_DEC_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_DEC_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_DEC_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_DEC_64BIT, OnUpdate64bit)
		ON_UPDATE_COMMAND_UI(ID_FLIP_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_FLIP_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_FLIP_64BIT, OnUpdate64bit)
		ON_UPDATE_COMMAND_UI(ID_SEARCH_SEL, OnUpdateClipboard)
		ON_UPDATE_COMMAND_UI(ID_PASTE_ASCII, OnUpdateTextPaste)
		ON_UPDATE_COMMAND_UI(ID_PASTE_EBCDIC, OnUpdateTextPaste)
		ON_UPDATE_COMMAND_UI(ID_EDIT_WRITEFILE, OnUpdateClipboard)
		ON_UPDATE_COMMAND_UI(ID_INVERT, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_NEG_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_NEG_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_NEG_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_NEG_64BIT, OnUpdate64bit)
	ON_UPDATE_COMMAND_UI(ID_EBC2ASC, OnUpdateConvert)
	ON_UPDATE_COMMAND_UI(ID_ANSI2IBM, OnUpdateConvert)
	ON_UPDATE_COMMAND_UI(ID_IBM2ANSI, OnUpdateConvert)
	ON_UPDATE_COMMAND_UI(ID_ENCRYPT_DECRYPT, OnUpdateEncrypt)
	ON_UPDATE_COMMAND_UI(ID_EDIT_APPENDFILE, OnUpdateClipboard)
	ON_UPDATE_COMMAND_UI(ID_CRC16, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC32, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC32_MPEG2, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC_GENERAL, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC_CCITT_F, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC_CCITT_AUG, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC_CCITT_T, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_CRC_XMODEM, OnUpdateByteNZ)
	ON_UPDATE_COMMAND_UI(ID_COPY_CCHAR, OnUpdateClipboard)
	ON_UPDATE_COMMAND_UI(ID_COPY_HEX, OnUpdateClipboard)
	ON_COMMAND(ID_TRACK_CHANGES, OnTrackChanges)
	ON_UPDATE_COMMAND_UI(ID_TRACK_CHANGES, OnUpdateTrackChanges)
	//}}AFX_MSG_MAP

		ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
		ON_COMMAND(ID_CHECKSUM8, OnChecksum8)
		ON_COMMAND(ID_CHECKSUM16, OnChecksum16)
		ON_COMMAND(ID_CHECKSUM32, OnChecksum32)
		ON_COMMAND(ID_CHECKSUM64, OnChecksum64)
		ON_UPDATE_COMMAND_UI(ID_CHECKSUM8, OnUpdateByteNZ)
		ON_UPDATE_COMMAND_UI(ID_CHECKSUM16, OnUpdate16bitNZ)
		ON_UPDATE_COMMAND_UI(ID_CHECKSUM32, OnUpdate32bitNZ)
		ON_UPDATE_COMMAND_UI(ID_CHECKSUM64, OnUpdate64bitNZ)

		ON_COMMAND(ID_XOR_BYTE, OnXorByte)
		ON_COMMAND(ID_XOR_16BIT, OnXor16bit)
		ON_COMMAND(ID_XOR_32BIT, OnXor32bit)
		ON_COMMAND(ID_XOR_64BIT, OnXor64bit)
		ON_UPDATE_COMMAND_UI(ID_XOR_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_XOR_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_XOR_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_XOR_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_ASSIGN_BYTE, OnAssignByte)
		ON_COMMAND(ID_ASSIGN_16BIT, OnAssign16bit)
		ON_COMMAND(ID_ASSIGN_32BIT, OnAssign32bit)
		ON_COMMAND(ID_ASSIGN_64BIT, OnAssign64bit)
		ON_UPDATE_COMMAND_UI(ID_ASSIGN_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_ASSIGN_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_ASSIGN_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_ASSIGN_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_RAND_BYTE, OnRandByte)
		ON_COMMAND(ID_RAND_FAST, OnRandFast)
//        ON_COMMAND(ID_RAND_16BIT, OnRand16bit)
//        ON_COMMAND(ID_RAND_32BIT, OnRand32bit)
//        ON_COMMAND(ID_RAND_64BIT, OnRand64bit)
		ON_UPDATE_COMMAND_UI(ID_RAND_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_RAND_FAST, OnUpdateByte)
//        ON_UPDATE_COMMAND_UI(ID_RAND_16BIT, OnUpdate16bit)
//        ON_UPDATE_COMMAND_UI(ID_RAND_32BIT, OnUpdate32bit)
//        ON_UPDATE_COMMAND_UI(ID_RAND_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_ADD_BYTE, OnAddByte)
		ON_COMMAND(ID_ADD_16BIT, OnAdd16bit)
		ON_COMMAND(ID_ADD_32BIT, OnAdd32bit)
		ON_COMMAND(ID_ADD_64BIT, OnAdd64bit)
		ON_UPDATE_COMMAND_UI(ID_ADD_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_ADD_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_ADD_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_ADD_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_SUBTRACT_BYTE, OnSubtractByte)
		ON_COMMAND(ID_SUBTRACT_16BIT, OnSubtract16bit)
		ON_COMMAND(ID_SUBTRACT_32BIT, OnSubtract32bit)
		ON_COMMAND(ID_SUBTRACT_64BIT, OnSubtract64bit)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_AND_BYTE, OnAndByte)
		ON_COMMAND(ID_AND_16BIT, OnAnd16bit)
		ON_COMMAND(ID_AND_32BIT, OnAnd32bit)
		ON_COMMAND(ID_AND_64BIT, OnAnd64bit)
		ON_UPDATE_COMMAND_UI(ID_AND_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_AND_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_AND_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_AND_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_OR_BYTE, OnOrByte)
		ON_COMMAND(ID_OR_16BIT, OnOr16bit)
		ON_COMMAND(ID_OR_32BIT, OnOr32bit)
		ON_COMMAND(ID_OR_64BIT, OnOr64bit)
		ON_UPDATE_COMMAND_UI(ID_OR_BYTE, OnUpdateByteBinary)
		ON_UPDATE_COMMAND_UI(ID_OR_16BIT, OnUpdate16bitBinary)
		ON_UPDATE_COMMAND_UI(ID_OR_32BIT, OnUpdate32bitBinary)
		ON_UPDATE_COMMAND_UI(ID_OR_64BIT, OnUpdate64bitBinary)

		ON_COMMAND(ID_MUL_BYTE, OnMulByte)
		ON_UPDATE_COMMAND_UI(ID_MUL_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_MUL_16BIT, OnMul16bit)
		ON_UPDATE_COMMAND_UI(ID_MUL_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_MUL_32BIT, OnMul32bit)
		ON_UPDATE_COMMAND_UI(ID_MUL_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_MUL_64BIT, OnMul64bit)
		ON_UPDATE_COMMAND_UI(ID_MUL_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_DIV_BYTE, OnDivByte)
		ON_UPDATE_COMMAND_UI(ID_DIV_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_DIV_16BIT, OnDiv16bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_DIV_32BIT, OnDiv32bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_DIV_64BIT, OnDiv64bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_MOD_BYTE, OnModByte)
		ON_UPDATE_COMMAND_UI(ID_MOD_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_MOD_16BIT, OnMod16bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_MOD_32BIT, OnMod32bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_MOD_64BIT, OnMod64bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_REV_BYTE, OnRevByte)
		ON_UPDATE_COMMAND_UI(ID_REV_BYTE, OnUpdateByte)
		ON_COMMAND(ID_REV_16BIT, OnRev16bit)
		ON_UPDATE_COMMAND_UI(ID_REV_16BIT, OnUpdate16bit)
		ON_COMMAND(ID_REV_32BIT, OnRev32bit)
		ON_UPDATE_COMMAND_UI(ID_REV_32BIT, OnUpdate32bit)
		ON_COMMAND(ID_REV_64BIT, OnRev64bit)
		ON_UPDATE_COMMAND_UI(ID_REV_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_SUBTRACT_X_BYTE, OnSubtractXByte)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_SUBTRACT_X_16BIT, OnSubtractX16bit)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_SUBTRACT_X_32BIT, OnSubtractX32bit)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_SUBTRACT_X_64BIT, OnSubtractX64bit)
		ON_UPDATE_COMMAND_UI(ID_SUBTRACT_X_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_DIV_X_BYTE, OnDivXByte)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_DIV_X_16BIT, OnDivX16bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_DIV_X_32BIT, OnDivX32bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_DIV_X_64BIT, OnDivX64bit)
		ON_UPDATE_COMMAND_UI(ID_DIV_X_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_MOD_X_BYTE, OnModXByte)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_MOD_X_16BIT, OnModX16bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_MOD_X_32BIT, OnModX32bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_MOD_X_64BIT, OnModX64bit)
		ON_UPDATE_COMMAND_UI(ID_MOD_X_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_GTR_BYTE, OnGtrByte)
		ON_UPDATE_COMMAND_UI(ID_GTR_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_GTR_16BIT, OnGtr16bit)
		ON_UPDATE_COMMAND_UI(ID_GTR_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_GTR_32BIT, OnGtr32bit)
		ON_UPDATE_COMMAND_UI(ID_GTR_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_GTR_64BIT, OnGtr64bit)
		ON_UPDATE_COMMAND_UI(ID_GTR_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_LESS_BYTE, OnLessByte)
		ON_UPDATE_COMMAND_UI(ID_LESS_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_LESS_16BIT, OnLess16bit)
		ON_UPDATE_COMMAND_UI(ID_LESS_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_LESS_32BIT, OnLess32bit)
		ON_UPDATE_COMMAND_UI(ID_LESS_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_LESS_64BIT, OnLess64bit)
		ON_UPDATE_COMMAND_UI(ID_LESS_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_GTRU_BYTE, OnGtrUByte)
		ON_UPDATE_COMMAND_UI(ID_GTRU_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_GTRU_16BIT, OnGtrU16bit)
		ON_UPDATE_COMMAND_UI(ID_GTRU_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_GTRU_32BIT, OnGtrU32bit)
		ON_UPDATE_COMMAND_UI(ID_GTRU_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_GTRU_64BIT, OnGtrU64bit)
		ON_UPDATE_COMMAND_UI(ID_GTRU_64BIT, OnUpdate64bitBinary)
		ON_COMMAND(ID_LESSU_BYTE, OnLessUByte)
		ON_UPDATE_COMMAND_UI(ID_LESSU_BYTE, OnUpdateByteBinary)
		ON_COMMAND(ID_LESSU_16BIT, OnLessU16bit)
		ON_UPDATE_COMMAND_UI(ID_LESSU_16BIT, OnUpdate16bitBinary)
		ON_COMMAND(ID_LESSU_32BIT, OnLessU32bit)
		ON_UPDATE_COMMAND_UI(ID_LESSU_32BIT, OnUpdate32bitBinary)
		ON_COMMAND(ID_LESSU_64BIT, OnLessU64bit)
		ON_UPDATE_COMMAND_UI(ID_LESSU_64BIT, OnUpdate64bitBinary)

		// Note shifts don't care about calc operand size (bits) since values are less than 64
		ON_COMMAND(ID_ROL_BYTE, OnRolByte)
		ON_COMMAND(ID_ROL_16BIT, OnRol16bit)
		ON_COMMAND(ID_ROL_32BIT, OnRol32bit)
		ON_COMMAND(ID_ROL_64BIT, OnRol64bit)
		ON_UPDATE_COMMAND_UI(ID_ROL_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_ROL_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_ROL_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_ROL_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_ROR_BYTE, OnRorByte)
		ON_COMMAND(ID_ROR_16BIT, OnRor16bit)
		ON_COMMAND(ID_ROR_32BIT, OnRor32bit)
		ON_COMMAND(ID_ROR_64BIT, OnRor64bit)
		ON_UPDATE_COMMAND_UI(ID_ROR_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_ROR_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_ROR_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_ROR_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_LSL_BYTE, OnLslByte)
		ON_COMMAND(ID_LSL_16BIT, OnLsl16bit)
		ON_COMMAND(ID_LSL_32BIT, OnLsl32bit)
		ON_COMMAND(ID_LSL_64BIT, OnLsl64bit)
		ON_UPDATE_COMMAND_UI(ID_LSL_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_LSL_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_LSL_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_LSL_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_LSR_BYTE, OnLsrByte)
		ON_COMMAND(ID_LSR_16BIT, OnLsr16bit)
		ON_COMMAND(ID_LSR_32BIT, OnLsr32bit)
		ON_COMMAND(ID_LSR_64BIT, OnLsr64bit)
		ON_UPDATE_COMMAND_UI(ID_LSR_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_LSR_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_LSR_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_LSR_64BIT, OnUpdate64bit)
		ON_COMMAND(ID_ASR_BYTE, OnAsrByte)
		ON_COMMAND(ID_ASR_16BIT, OnAsr16bit)
		ON_COMMAND(ID_ASR_32BIT, OnAsr32bit)
		ON_COMMAND(ID_ASR_64BIT, OnAsr64bit)
		ON_UPDATE_COMMAND_UI(ID_ASR_BYTE, OnUpdateByte)
		ON_UPDATE_COMMAND_UI(ID_ASR_16BIT, OnUpdate16bit)
		ON_UPDATE_COMMAND_UI(ID_ASR_32BIT, OnUpdate32bit)
		ON_UPDATE_COMMAND_UI(ID_ASR_64BIT, OnUpdate64bit)

	ON_COMMAND_RANGE(ID_EXPORT_S1, ID_EXPORT_S3, OnExportSRecord)
	ON_UPDATE_COMMAND_UI_RANGE(ID_EXPORT_S1, ID_EXPORT_S3, OnUpdateExportSRecord)

		ON_UPDATE_COMMAND_UI(ID_INSERT_BLOCK, OnUpdateInsertBlock)
		ON_COMMAND(ID_INSERT_BLOCK, OnInsertBlock)

		// New display handling commands
		ON_COMMAND(ID_DISPLAY_HEX, OnDisplayHex)
		ON_UPDATE_COMMAND_UI(ID_DISPLAY_HEX, OnUpdateDisplayHex)
		ON_COMMAND(ID_DISPLAY_CHAR, OnDisplayChar)
		ON_UPDATE_COMMAND_UI(ID_DISPLAY_CHAR, OnUpdateDisplayChar)
		ON_COMMAND(ID_DISPLAY_BOTH, OnDisplayBoth)
		ON_UPDATE_COMMAND_UI(ID_DISPLAY_BOTH, OnUpdateDisplayBoth)
		ON_COMMAND(ID_DISPLAY_STACKED, OnDisplayStacked)
		ON_UPDATE_COMMAND_UI(ID_DISPLAY_STACKED, OnUpdateDisplayStacked)
		ON_COMMAND(ID_CHARSET_ASCII, OnCharsetAscii)
		ON_UPDATE_COMMAND_UI(ID_CHARSET_ASCII, OnUpdateCharsetAscii)
		ON_COMMAND(ID_CHARSET_ANSI, OnCharsetAnsi)
		ON_UPDATE_COMMAND_UI(ID_CHARSET_ANSI, OnUpdateCharsetAnsi)
		ON_COMMAND(ID_CHARSET_OEM, OnCharsetOem)
		ON_UPDATE_COMMAND_UI(ID_CHARSET_OEM, OnUpdateCharsetOem)
		ON_COMMAND(ID_CHARSET_EBCDIC, OnCharsetEbcdic)
		ON_UPDATE_COMMAND_UI(ID_CHARSET_EBCDIC, OnUpdateCharsetEbcdic)
		ON_COMMAND(ID_CHARSET_CODEPAGE, OnCharsetCodepage)
		ON_UPDATE_COMMAND_UI(ID_CHARSET_CODEPAGE, OnUpdateCharsetCodepage)

		ON_COMMAND(ID_CODEPAGE_65001, OnCodepage65001)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_65001, OnUpdateCodepage65001)
		ON_COMMAND(ID_CODEPAGE_437, OnCodepage437)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_437, OnUpdateCodepage437)
		ON_COMMAND(ID_CODEPAGE_500, OnCodepage500)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_500, OnUpdateCodepage500)
		ON_COMMAND(ID_CODEPAGE_10000, OnCodepage10000)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_10000, OnUpdateCodepage10000)
		ON_COMMAND(ID_CODEPAGE_1250, OnCodepage1250)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1250, OnUpdateCodepage1250)
		ON_COMMAND(ID_CODEPAGE_1251, OnCodepage1251)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1251, OnUpdateCodepage1251)
		ON_COMMAND(ID_CODEPAGE_1252, OnCodepage1252)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1252, OnUpdateCodepage1252)
		ON_COMMAND(ID_CODEPAGE_1253, OnCodepage1253)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1253, OnUpdateCodepage1253)
		ON_COMMAND(ID_CODEPAGE_1254, OnCodepage1254)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1254, OnUpdateCodepage1254)
		ON_COMMAND(ID_CODEPAGE_1255, OnCodepage1255)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1255, OnUpdateCodepage1255)
		ON_COMMAND(ID_CODEPAGE_1256, OnCodepage1256)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1256, OnUpdateCodepage1256)
		ON_COMMAND(ID_CODEPAGE_1257, OnCodepage1257)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1257, OnUpdateCodepage1257)
		ON_COMMAND(ID_CODEPAGE_1258, OnCodepage1258)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_1258, OnUpdateCodepage1258)
		ON_COMMAND(ID_CODEPAGE_874, OnCodepage874)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_874, OnUpdateCodepage874)
		ON_COMMAND(ID_CODEPAGE_932, OnCodepage932)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_932, OnUpdateCodepage932)
		ON_COMMAND(ID_CODEPAGE_936, OnCodepage936)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_936, OnUpdateCodepage936)
		ON_COMMAND(ID_CODEPAGE_949, OnCodepage949)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_949, OnUpdateCodepage949)
		ON_COMMAND(ID_CODEPAGE_950, OnCodepage950)
		ON_UPDATE_COMMAND_UI(ID_CODEPAGE_950, OnUpdateCodepage950)

		ON_COMMAND(ID_CONTROL_NONE, OnControlNone)
		ON_UPDATE_COMMAND_UI(ID_CONTROL_NONE, OnUpdateControlNone)
		ON_COMMAND(ID_CONTROL_ALPHA, OnControlAlpha)
		ON_UPDATE_COMMAND_UI(ID_CONTROL_ALPHA, OnUpdateControlAlpha)
		ON_COMMAND(ID_CONTROL_C, OnControlC)
		ON_UPDATE_COMMAND_UI(ID_CONTROL_C, OnUpdateControlC)

		ON_COMMAND(ID_TOGGLE_ENDIAN, OnToggleEndian)
		ON_COMMAND(ID_BIG_ENDIAN, OnBigEndian)
		ON_COMMAND(ID_LITTLE_ENDIAN, OnLittleEndian)
		ON_UPDATE_COMMAND_UI(ID_TOGGLE_ENDIAN, OnUpdateToggleEndian)
		ON_UPDATE_COMMAND_UI(ID_BIG_ENDIAN, OnUpdateBigEndian)
		ON_UPDATE_COMMAND_UI(ID_LITTLE_ENDIAN, OnUpdateLittleEndian)

		// Standard printing commands
		ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
		ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
		ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
		ON_COMMAND(ID_JUMP_HEX_ADDR, OnJumpHexAddr)

//        ON_COMMAND(ID_SEARCH_START, OnSearch)     // Handle message from BCG search combo
		ON_COMMAND(ID_JUMP_HEX_START, OnJumpHex)  // Handle message from BCG hex combo
		ON_COMMAND(ID_JUMP_DEC_START, OnJumpDec)  // Handle message from BCG dec combo

		ON_COMMAND(ID_SELECT_LINE, OnSelectLine)
		ON_COMMAND(IDC_FONTNAME, OnFontName)
		ON_UPDATE_COMMAND_UI(IDC_FONTNAME, OnUpdateFontName)
		ON_COMMAND(IDC_FONTSIZE, OnFontSize)
		ON_UPDATE_COMMAND_UI(IDC_FONTSIZE, OnUpdateFontSize)
		ON_CBN_SELENDOK(IDC_FONTNAME, OnFontName)
		ON_CBN_SELENDOK(IDC_FONTSIZE, OnFontSize)

	ON_COMMAND(ID_ZLIB_COMPRESS, OnCompress)
	ON_UPDATE_COMMAND_UI(ID_ZLIB_COMPRESS, OnUpdateSelNZ)
	ON_COMMAND(ID_ZLIB_DECOMPRESS, OnDecompress)
	ON_UPDATE_COMMAND_UI(ID_ZLIB_DECOMPRESS, OnUpdateSelNZ)
	ON_COMMAND(ID_MD5, OnMd5)
	ON_UPDATE_COMMAND_UI(ID_MD5, OnUpdateByteNZ)
	ON_COMMAND(ID_SHA1, OnSha1)
	ON_UPDATE_COMMAND_UI(ID_SHA1, OnUpdateByteNZ)

	ON_COMMAND(ID_SHA224, OnSha2_224)
	ON_UPDATE_COMMAND_UI(ID_SHA224, OnUpdateByteNZ)
	ON_COMMAND(ID_SHA256, OnSha2_256)
	ON_UPDATE_COMMAND_UI(ID_SHA256, OnUpdateByteNZ)
	ON_COMMAND(ID_SHA384, OnSha2_384)
	ON_UPDATE_COMMAND_UI(ID_SHA384, OnUpdateByteNZ)
	ON_COMMAND(ID_SHA512, OnSha2_512)
	ON_UPDATE_COMMAND_UI(ID_SHA512, OnUpdateByteNZ)

	ON_COMMAND(ID_UPPERCASE, OnUppercase)
	ON_UPDATE_COMMAND_UI(ID_UPPERCASE, OnUpdateConvert)
	ON_COMMAND(ID_LOWERCASE, OnLowercase)
	ON_UPDATE_COMMAND_UI(ID_LOWERCASE, OnUpdateConvert)

	ON_COMMAND(ID_DFFD_HIDE, OnDffdHide)
	ON_UPDATE_COMMAND_UI(ID_DFFD_HIDE, OnUpdateDffdHide)
	ON_COMMAND(ID_DFFD_SPLIT, OnDffdSplit)
	ON_UPDATE_COMMAND_UI(ID_DFFD_SPLIT, OnUpdateDffdSplit)
	ON_COMMAND(ID_DFFD_TAB, OnDffdTab)
	ON_UPDATE_COMMAND_UI(ID_DFFD_TAB, OnUpdateDffdTab)

	ON_COMMAND(ID_AERIAL_HIDE, OnAerialHide)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_HIDE, OnUpdateAerialHide)
	ON_COMMAND(ID_AERIAL_SPLIT, OnAerialSplit)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_SPLIT, OnUpdateAerialSplit)
	ON_COMMAND(ID_AERIAL_TAB, OnAerialTab)
	ON_UPDATE_COMMAND_UI(ID_AERIAL_TAB, OnUpdateAerialTab)

	ON_COMMAND(ID_COMP_HIDE, OnCompHide)
	ON_UPDATE_COMMAND_UI(ID_COMP_HIDE, OnUpdateCompHide)
	ON_COMMAND(ID_COMP_SPLIT, OnCompSplit)
	ON_UPDATE_COMMAND_UI(ID_COMP_SPLIT, OnUpdateCompSplit)
	ON_COMMAND(ID_COMP_TAB, OnCompTab)
	ON_UPDATE_COMMAND_UI(ID_COMP_TAB, OnUpdateCompTab)

	ON_COMMAND(ID_HIGHLIGHT_SELECT, OnHighlightSelect)
	//ON_WM_TIMER()

	ON_COMMAND(ID_VIEWTEST, OnViewtest)
	ON_WM_SYSCOLORCHANGE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)

	ON_COMMAND(ID_SCHEME, OnOptScheme)
	ON_CBN_SELENDOK(ID_SCHEME, OnSelScheme)
	ON_UPDATE_COMMAND_UI(ID_SCHEME, OnUpdateScheme)
	ON_COMMAND(ID_SCHEME_US, OnOptScheme)
	ON_CBN_SELENDOK(ID_SCHEME_US, OnSelScheme)
	ON_UPDATE_COMMAND_UI(ID_SCHEME_US, OnUpdateScheme)

	ON_COMMAND(ID_COMP_FIRST, OnCompFirst)
	ON_COMMAND(ID_COMP_PREV, OnCompPrev)
	ON_COMMAND(ID_COMP_NEXT, OnCompNext)
	ON_COMMAND(ID_COMP_LAST, OnCompLast)
	ON_UPDATE_COMMAND_UI(ID_COMP_FIRST, OnUpdateCompFirst)
	ON_UPDATE_COMMAND_UI(ID_COMP_PREV, OnUpdateCompPrev)
	ON_UPDATE_COMMAND_UI(ID_COMP_NEXT, OnUpdateCompNext)
	ON_UPDATE_COMMAND_UI(ID_COMP_LAST, OnUpdateCompLast)
	ON_COMMAND(ID_COMP_ALL_FIRST, OnCompAllFirst)
	ON_COMMAND(ID_COMP_ALL_PREV, OnCompAllPrev)
	ON_COMMAND(ID_COMP_ALL_NEXT, OnCompAllNext)
	ON_COMMAND(ID_COMP_ALL_LAST, OnCompAllLast)
	ON_UPDATE_COMMAND_UI(ID_COMP_ALL_FIRST, OnUpdateCompAllFirst)
	ON_UPDATE_COMMAND_UI(ID_COMP_ALL_PREV, OnUpdateCompAllPrev)
	ON_UPDATE_COMMAND_UI(ID_COMP_ALL_NEXT, OnUpdateCompAllNext)
	ON_UPDATE_COMMAND_UI(ID_COMP_ALL_LAST, OnUpdateCompAllLast)
	ON_COMMAND(ID_COMP_AUTO_SYNC, OnCompAutoSync)
	ON_UPDATE_COMMAND_UI(ID_COMP_AUTO_SYNC, OnUpdateCompAutoSync)
	ON_COMMAND(ID_COMP_AUTO_SCROLL, OnCompAutoScroll)
	ON_UPDATE_COMMAND_UI(ID_COMP_AUTO_SCROLL, OnUpdateCompAutoScroll)

	END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHexEditView construction/destruction
CHexEditView::CHexEditView()
{
	nav_moves_ = -1;
	expr_.SetView(this);
	text_height_ = 0;     // While text_height_ == 0 none of the display settings have been calculated
	pdfv_ = NULL;
	pav_ = NULL;
	pcv_ = NULL;

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	disp_state_ = previous_state_ = 0;

	resize_start_addr_ = resize_curr_scroll_ = -1;

	pfont_ = NULL;
	pbrush_ = NULL;
	print_font_ = NULL;

	// Set up opening display based on global options
	rowsize_ = aa->open_rowsize_;
	real_offset_ = aa->open_offset_;
	offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;
	group_by_ = aa->open_group_by_;

	ASSERT(theApp.open_disp_state_ != -1);
	disp_state_ = theApp.open_disp_state_;

	bg_col_ = -1;

	hl_set_.clear();

	mark_ = 0L;                         // Mark initially at start of file
	if (!display_.hex_area)
		display_.edit_char = display_.mark_char = TRUE;  // No hex area so must use char
	else
		display_.edit_char = display_.mark_char = FALSE;    // Caret init. in hex not char area

	ASSERT(CP_ACP == 0);
	code_page_ = 0; max_cp_bytes_ = 1;  // default to ANSI code page

	mouse_addr_ = -1;           // Only used when theApp.hl_mouse_ is on

	mouse_down_ = false;
	drag_mark_ = false;	drag_bookmark_ = -1;
	needs_refresh_ = false;
	needs_hscroll_ = false;
	needs_vscroll_ = false;

	memset((void *)&lf_, '\0', sizeof(lf_));
	_tcscpy(lf_.lfFaceName, _T("Courier")); // A nice fixed (no-proportional) font
	lf_.lfHeight = 16;
	lf_.lfCharSet = ANSI_CHARSET;           // Only allow ANSI character set fonts
	oem_lf_ = lf_;
	_tcscpy(oem_lf_.lfFaceName, _T("Terminal")); // The only certain OEM font?
	oem_lf_.lfHeight = 18;
	oem_lf_.lfCharSet = OEM_CHARSET;         // Only allow OEM/IBM character set fonts
	mb_lf_ = lf_;
	_tcscpy(mb_lf_.lfFaceName, _T("Lucida Sans Unicode")); // Good Unicode font
	mb_lf_.lfCharSet = DEFAULT_CHARSET;

	search_length_ = 0;
	last_tip_addr_ = -1;
	tip_show_bookmark_ = true;
	tip_show_error_ = true;
	tip_last_locn_ = tip_.GetPos();

#ifndef NDEBUG
	// Make default capacity for undo_ vector small to force reallocation sooner.
	// This increases likelihood of catching bugs related to reallocation.
	undo_.reserve(4);
#else
	undo_.reserve(1024);
#endif

#ifdef RULER_ADJUST
	adjusting_rowsize_ = adjusting_offset_ = adjusting_group_by_ = -1;
#endif

	errors_mentioned_ = false;
}

CHexEditView::~CHexEditView()
{
	if (pfont_ != NULL)
		delete pfont_;
	if (pbrush_ != NULL)
		delete pbrush_;
} 

BOOL CHexEditView::PreCreateWindow(CREATESTRUCT& cs)
{
	BOOL retval = CScrView::PreCreateWindow(cs);

	// Get the create context so we can find out if this window is
	// being created via Window/New and the frame/view it's cloned from
	CCreateContext *pContext = (CCreateContext *)cs.lpCreateParams;

	if (pContext != NULL && pContext->m_pCurrentFrame != NULL)
	{
		// Must have been created via Window/New (ID_WINDOW_NEW)
		ASSERT_KINDOF(CMDIChildWnd, pContext->m_pCurrentFrame);

		// We have the frame so get the view within it
		CHexEditView *pView = (CHexEditView *)pContext->m_pCurrentFrame->GetActiveView();
		if (pView->IsKindOf(RUNTIME_CLASS(CCompareView)))
			pView = ((CCompareView *)pView)->phev_;
		else if (pView->IsKindOf(RUNTIME_CLASS(CAerialView)))
			pView = ((CAerialView *)pView)->phev_;
		else if (pView->IsKindOf(RUNTIME_CLASS(CDataFormatView)))
			pView = ((CDataFormatView *)pView)->phev_;
		ASSERT_KINDOF(CHexEditView, pView);

		// Make this view's undo stack the same as clone view
		undo_ = pView->undo_;

		// Flag any changes as not being made in this view
		std::vector <view_undo>::iterator pu;
		for (pu = undo_.begin(); pu != undo_.end(); ++pu)
		{
			if (pu->utype == undo_change)
				pu->flag = FALSE;
		}
	}

	return retval;
}

void CHexEditView::OnInitialUpdate()
{
	FILE_ADDRESS start_addr = 0;
	FILE_ADDRESS end_addr = 0;
	CHexEditDoc *pDoc = GetDocument();
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	print_map_mode_ = MM_HIENGLISH;
	split_width_d_ = split_width_a_ = split_width_c_ = -1;

	// Get options for the window from file settings in CHexFileList
	CHexFileList *pfl = aa->GetFileList();
	int recent_file_index = -1;
	if (pDoc->pfile1_ != NULL)
		recent_file_index = pfl->GetIndex(pDoc->pfile1_->GetFilePath());

#ifdef TEST_CLIPPING
	bdr_top_ = bdr_bottom_ = 80;
	bdr_left_ = 120;
	bdr_right_ = 40;
#endif

	if (recent_file_index != -1)
	{
		CString ss = pfl->GetData(recent_file_index, CHexFileList::DFFDVIEW);
		if (ss.IsEmpty())
			split_width_d_ = theApp.dffdview_;
		else
			split_width_d_ = atoi(ss);
		ss = pfl->GetData(recent_file_index, CHexFileList::AERIALVIEW);
		if (ss.IsEmpty())
			split_width_a_ = theApp.aerialview_;
		else
			split_width_a_ = atoi(ss);
		ss = pfl->GetData(recent_file_index, CHexFileList::COMPVIEW);
		if (ss.IsEmpty())
			split_width_c_ = theApp.compview_;
		else
			split_width_c_ = atoi(ss);

		disp_state_ = atoi(pfl->GetData(recent_file_index, CHexFileList::DISPLAY));
		if (disp_state_ == 0) disp_state_ = 3;  // just in case RecentFile is really corrupt
		SetVertBufferZone(atoi(pfl->GetData(recent_file_index, CHexFileList::VERT_BUFFER_ZONE)));

		SetCodePage(atoi(pfl->GetData(recent_file_index, CHexFileList::CODEPAGE)));

		// Get the colour scheme, if none try to find one based on file extension, otherwise
		// let set_colours() handle it using the default scheme for the current char set.
		scheme_name_ = pfl->GetData(recent_file_index, CHexFileList::SCHEME);
		if (scheme_name_.IsEmpty())
		{
			// Get file extension and change "." to "_"
			ss = pDoc->GetFileName();
			if (ss.ReverseFind('.') == -1)
				ss = "_";
			else
				ss = CString("_") + ss.Mid(ss.ReverseFind('.')+1);

			// If there is a scheme of this name make it the scheme to use
			std::vector<CScheme>::const_iterator ps;
			for (ps = theApp.scheme_.begin(); ps != theApp.scheme_.end(); ++ps)
			{
				if (ss.CompareNoCase(ps->name_) == 0)
				{
					scheme_name_ = ps->name_;
					break;
				}
			}
		}
		set_colours();

		rowsize_ = atoi(pfl->GetData(recent_file_index, CHexFileList::COLUMNS));
		if (rowsize_ < 4 || rowsize_ > 32767) rowsize_ = 4;   // just in case RecentFile is really corrupt
		real_offset_ = atoi(pfl->GetData(recent_file_index, CHexFileList::OFFSET));
		offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;
		group_by_ = atoi(pfl->GetData(recent_file_index, CHexFileList::GROUPING));
		if (group_by_ < 2) group_by_ = 2;  // just in case RecentFile is really corrupt

		start_addr = _atoi64(pfl->GetData(recent_file_index, CHexFileList::SELSTART));
		end_addr = _atoi64(pfl->GetData(recent_file_index, CHexFileList::SELEND));
		if (start_addr < 0 || start_addr > pDoc->length())
			start_addr = 0;
		if (end_addr < start_addr || end_addr > pDoc->length())
			end_addr = start_addr;

		mark_ = _atoi64(pfl->GetData(recent_file_index, CHexFileList::MARK));
		if (mark_ < 0 || mark_ > pDoc->length())
			mark_ = 0;
		ASSERT(display_.hex_area || display_.edit_char);  // if no hex area we must be editing chars

		// Get saved font info
		strncpy(lf_.lfFaceName, pfl->GetData(recent_file_index, CHexFileList::FONT), LF_FACESIZE-1);
		lf_.lfFaceName[LF_FACESIZE-1] = '\0';
		lf_.lfHeight = atoi(pfl->GetData(recent_file_index, CHexFileList::HEIGHT));
		strncpy(oem_lf_.lfFaceName, pfl->GetData(recent_file_index, CHexFileList::OEMFONT), LF_FACESIZE-1);
		oem_lf_.lfFaceName[LF_FACESIZE-1] = '\0';
		oem_lf_.lfHeight = atoi(pfl->GetData(recent_file_index, CHexFileList::OEMHEIGHT));
		strncpy(mb_lf_.lfFaceName, pfl->GetData(recent_file_index, CHexFileList::MBFONT), LF_FACESIZE-1);
		mb_lf_.lfFaceName[LF_FACESIZE-1] = '\0';
		mb_lf_.lfHeight = atoi(pfl->GetData(recent_file_index, CHexFileList::MBHEIGHT));

		std::istringstream strstr((const char *)pfl->GetData(recent_file_index, CHexFileList::HIGHLIGHTS));
		strstr >> hl_set_;

		CRect newpos;
		newpos.top =  atoi(pfl->GetData(recent_file_index, CHexFileList::TOP));
		newpos.left =  atoi(pfl->GetData(recent_file_index, CHexFileList::LEFT));
		newpos.bottom =  atoi(pfl->GetData(recent_file_index, CHexFileList::BOTTOM));
		newpos.right =  atoi(pfl->GetData(recent_file_index, CHexFileList::RIGHT));

		// Make sure that window size seems reasonable
		if (newpos.top != -30000 && newpos.left != -30000 &&
			newpos.top < newpos.bottom && newpos.left < newpos.right )
		{
			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			wp.flags = 0;
			wp.showCmd = atoi(pfl->GetData(recent_file_index, CHexFileList::CMD));
			wp.ptMinPosition = CPoint(-1,-1);
			wp.ptMaxPosition = CPoint(-1,-1);

			// If this window has sibling view this window was presumably created
			// using Window/New - make sure its not in the same place as its sibling(s).
			POSITION pos = pDoc->GetFirstViewPosition();
			while (pos != NULL)
			{
				WINDOWPLACEMENT frame_wp;
				frame_wp.length = sizeof(frame_wp);
				CView *pv = pDoc->GetNextView(pos);
				if (pv->IsKindOf(RUNTIME_CLASS(CHexEditView)))
				{
					ASSERT(pv != NULL && ((CHexEditView *)pv)->GetFrame() != NULL);
					if (pv != this && ((CHexEditView *)pv)->GetFrame()->GetWindowPlacement(&frame_wp))
					{
						// If the top left corners are about the same move the
						// new window down and right a bit.
						if (abs(newpos.top - frame_wp.rcNormalPosition.top) < 20 &&
							abs(newpos.left - frame_wp.rcNormalPosition.left) < 20)
						{
							newpos += CSize(30, 30);
						}
					}
				}
			}

			// Check that new position is not completely off the left, right or bottom,
			// and that the top title bar is still visible to allow dragging.
			CRect rr;
			ASSERT(GetFrame() != NULL && GetFrame()->GetParent() != NULL);
			GetFrame()->GetParent()->GetClientRect(&rr);
			if (newpos.left > rr.right-20)
			{
				newpos.left = rr.right - (newpos.right - newpos.left);
				newpos.right = rr.right;
			}
			if (newpos.right < 20)
			{
				newpos.right -= newpos.left;
				newpos.left = 0;
			}
			if (newpos.top > rr.bottom-20)
			{
				newpos.top = rr.bottom - (newpos.bottom - newpos.top);
				newpos.bottom = rr.bottom;
			}
			if (newpos.top < -20)
			{
				newpos.bottom -= newpos.top;
				newpos.top =0;
			}

			wp.rcNormalPosition = newpos;
			if (wp.showCmd == SW_SHOWMAXIMIZED)
				wp.flags = WPF_RESTORETOMAXIMIZED;
			GetFrame()->SetWindowPlacement(&wp);
		}
		else if (atoi(pfl->GetData(recent_file_index, CHexFileList::CMD)) == SW_SHOWMAXIMIZED)
		{
			WINDOWPLACEMENT wp;
			ASSERT(GetFrame() != NULL);
			GetFrame()->GetWindowPlacement(&wp);
			wp.showCmd = SW_SHOWMAXIMIZED;
			GetFrame()->SetWindowPlacement(&wp);
		}

		// Set horiz size, vert size, and scroll posn
		if (display_.vert_display || display_.char_area)
			SetSize(CSize(char_pos(rowsize_-1)+text_width_w_+text_width_w_/2+1, -1));
		else
		{
			ASSERT(display_.hex_area);
			display_.hex_area = TRUE; // defensive
			SetSize(CSize(hex_pos(rowsize_-1)+2*text_width_+text_width_/2+1, -1));
		}
		if (display_.vert_display)
			SetTSize(CSizeAp(-1, ((GetDocument()->length() + offset_)/rowsize_ + 1)*3));  // 3 rows of text
		else
			SetTSize(CSizeAp(-1, (GetDocument()->length() + offset_)/rowsize_ + 1));
		SetScroll(CPointAp(0, _atoi64(pfl->GetData(recent_file_index, CHexFileList::POS))));
	}
	else
	{
		ASSERT(pDoc->pfile1_ == NULL);   // we should only get here (now) if not yet saved to disk
		split_width_d_ = theApp.dffdview_;
		split_width_a_ = theApp.aerialview_;
		split_width_c_ = theApp.compview_;

		// Force colour scheme based on char set (not file extension as we don't have one)
		scheme_name_ = "";
		set_colours();

		if (aa->open_max_)
		{
			WINDOWPLACEMENT wp;
			ASSERT(GetFrame() != NULL);
			GetFrame()->GetWindowPlacement(&wp);
			wp.showCmd = SW_SHOWMAXIMIZED;
			GetFrame()->SetWindowPlacement(&wp);
		}

		// Set the fonts
		if (aa->open_plf_ != NULL)      lf_ = *aa->open_plf_;
		if (aa->open_oem_plf_ != NULL)  oem_lf_ = *aa->open_oem_plf_;
		if (aa->open_mb_plf_ != NULL)   mb_lf_ = *aa->open_mb_plf_;
	}

	// Make sure that something is displayed in the address area
	if (!display_.decimal_addr && !display_.hex_addr && !display_.line_nums)
	{
		display_.decimal_addr = display_.dec_addr;
		display_.hex_addr = !display_.dec_addr;
	}

	// In version 2 files there were 3 separate flags (eg when EBCDIC flag on other 2 flags ignored).
	// Now the 3 bits are used together to indicate char set but we allow for other bit patterns.
	if (display_.char_set == 2)
		display_.char_set = CHARSET_ASCII;       // ASCII:  0/2 -> 0

	// This is just here as a workaround for a small problem with MDItabs - when
	// the tabs are at the top then the tab bar gets slightly higher when the first
	// file is opened which mucks up a small part at the top of the MDI area.
	static bool first_time_here = true;
	if (first_time_here)
	{
		((CMainFrame *)AfxGetMainWnd())->redraw_background();
		first_time_here = false;
	}

	if (!display_.edit_char)
		SetHorzBufferZone(2);

	// Convert font sizes to logical units
	{
		CPoint convert(0, lf_.lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&convert);                    // Get screen font size in logical units
		lf_.lfHeight = convert.y;
	}
	{
		CPoint convert(0, oem_lf_.lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&convert);                    // Get screen font size in logical units
		oem_lf_.lfHeight = convert.y;
	}
	{
		CPoint convert(0, mb_lf_.lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&convert);                    // Get screen font size in logical units
		mb_lf_.lfHeight = convert.y;
	}

	// This can't be done till document available (ie. not in constructor)
	if (GetDocument()->read_only())
		display_.readonly = TRUE;

	// Set control bar buttons to display state of current options
//    load_bitmaps();

	CScrView::SetMapMode(MM_TEXT);

	ASSERT(pfont_ == NULL);
	pfont_ = new CFont;
	pfont_->CreateFontIndirect( display_.FontRequired() == FONT_OEM ? &oem_lf_
		                       : ( display_.FontRequired() == FONT_UCODE ? &mb_lf_
							       : &lf_ ) );
	SetFont(pfont_);

	// Create brush that is XORed with selection when focus lost.  (This
	// gives a more subtle grey selection) when window does not have focus.)
	pbrush_ = new CBrush(RGB(192, 192, 192));
	SetBrush(pbrush_);

	// Reopen template view
	ASSERT(pdfv_ == NULL);
	switch (split_width_d_)
	{
	case 0:        // When last open template view was hidden so do nothing
		split_width_d_ = -1;
		break;
	case 2:        // Last opened in tab view
		split_width_d_ = -1;
		DoDffdTab();
		break;
	default:       // Last opened in splitter
		DoDffdSplit();
		break;
	}

	// Reopen aerial view
	ASSERT(pav_ == NULL);
	switch (split_width_a_)
	{
	case 0:        // When last open template view was hidden so do nothing
		split_width_a_ = -1;
		break;
	case 2:        // Last opened in tab view
		split_width_a_ = -1;
		DoAerialTab(false);      // no init as that seems to be done by MFC
		break;
	default:       // Last opened in splitter
		DoAerialSplit(false);    // no init as that seems to be done by MFC
		break;
	}

	// Reopen compare view
	ASSERT(pcv_ == NULL);
	switch (split_width_c_)
	{
	case 0:        // When last open template view was hidden so do nothing
		split_width_c_ = -1;
		break;
	case 2:        // Last opened in tab view
		split_width_c_ = -1;
		DoCompTab(false);      // no WM_INITIALUPDATE needed here (done by MFC)
		break;
	default:       // Last opened in splitter
		DoCompSplit(false);    // no WM_INITIALUPDATE needed here (done by MFC)
		break;
	}

	CScrView::OnInitialUpdate();

#if 0
	if (recent_file_index != -1)
		SetScroll(CPointAp(0,pfl->scroll_[recent_file_index]));

	// What's the point of an empty read-only file
	if (GetDocument()->length() == 0 && !GetDocument()->read_only())
		display_.readonly = display_.overtype = FALSE;
#endif

	if (aa->large_cursor_)
		BlockCaret();
	else
		LineCaret();
	if (!display_.overtype || GetDocument()->length() > 0)
	{
		CaretMode();
		SetSel(addr2pos(start_addr), addr2pos(end_addr));
		show_prop();
		show_calc();
		show_pos();
	}

	ValidateScroll(GetScroll());

	// Save nav info in case we need to create a nav pt here
	nav_start_  = start_addr;
	nav_end_    = end_addr;
	nav_scroll_ = (GetScroll().y/line_height_)*rowsize_ - offset_;
	theApp.navman_.Add("Opened", get_info(), this, nav_start_, nav_end_, nav_scroll_);
	nav_moves_ = 0;
} // OnInitialUpdate

// Update our options to the CHexFileList 
void CHexEditView::StoreOptions()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CHexFileList *pfl = ((CHexEditApp *)AfxGetApp())->GetFileList();
	if (GetDocument()->pfile1_ == NULL)
		return;                                 // Not in the list if there's no file
	int ii = pfl->GetIndex(GetDocument()->pfile1_->GetFilePath());
	if (ii != -1)
	{
		// Found in the list so update all the values;
		WINDOWPLACEMENT wp;
		// This is a kludge because if you close more than one window at a time
		// and they are maximized MDI children then only one of them will have
		// wp.showCmd == SW_SHOWMAXIMIZED (3), others will be SW_SHOWNORMAL (1)
		ASSERT(GetFrame() != NULL && GetView()->GetFrame() != NULL);
		UINT activeShowCmd = SW_SHOWMAXIMIZED;
		if (GetView()->GetFrame()->GetWindowPlacement(&wp))
			activeShowCmd = wp.showCmd;

		ASSERT(GetFrame() != NULL);
		if (GetFrame()->GetWindowPlacement(&wp))
		{
			pfl->SetData(ii, CHexFileList::CMD, activeShowCmd);
			pfl->SetData(ii, CHexFileList::TOP, wp.rcNormalPosition.top);
			pfl->SetData(ii, CHexFileList::LEFT, wp.rcNormalPosition.left);
			pfl->SetData(ii, CHexFileList::BOTTOM, wp.rcNormalPosition.bottom);
			pfl->SetData(ii, CHexFileList::RIGHT, wp.rcNormalPosition.right);
		}
		else
		{
			pfl->SetData(ii, CHexFileList::CMD, SW_SHOWNORMAL);
			pfl->SetData(ii, CHexFileList::TOP, -30000);
			pfl->SetData(ii, CHexFileList::LEFT, -30000);
			pfl->SetData(ii, CHexFileList::BOTTOM, -30000);
			pfl->SetData(ii, CHexFileList::RIGHT, -30000);
		}

		pfl->SetData(ii, CHexFileList::COLUMNS, rowsize_);
		pfl->SetData(ii, CHexFileList::GROUPING, group_by_);
		pfl->SetData(ii, CHexFileList::OFFSET, real_offset_);

		pfl->SetData(ii, CHexFileList::SCHEME, scheme_name_);
		pfl->SetData(ii, CHexFileList::FONT, lf_.lfFaceName);
		pfl->SetData(ii, CHexFileList::HEIGHT, lf_.lfHeight);
		pfl->SetData(ii, CHexFileList::OEMFONT, oem_lf_.lfFaceName);
		pfl->SetData(ii, CHexFileList::OEMHEIGHT, oem_lf_.lfHeight);
		pfl->SetData(ii, CHexFileList::MBFONT, mb_lf_.lfFaceName);
		pfl->SetData(ii, CHexFileList::MBHEIGHT, mb_lf_.lfHeight);

		pfl->SetData(ii, CHexFileList::DISPLAY, disp_state_);
		if (code_page_ != 0) pfl->SetData(ii, CHexFileList::CODEPAGE, code_page_);

		pfl->SetData(ii, CHexFileList::DOC_FLAGS, GetDocument()->doc_flags());
		pfl->SetData(ii, CHexFileList::FORMAT, GetDocument()->GetFormatFileName());

		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		pfl->SetData(ii, CHexFileList::SELSTART, start_addr);
		pfl->SetData(ii, CHexFileList::SELEND, end_addr);
		pfl->SetData(ii, CHexFileList::POS, GetScroll().y);
		pfl->SetData(ii, CHexFileList::MARK, mark_);

		std::ostringstream strstr;
		strstr << hl_set_;
		pfl->SetData(ii, CHexFileList::HIGHLIGHTS, strstr.str().c_str());

		pfl->SetData(ii, CHexFileList::EDIT_TIME, __int64(GetDocument()->edit_time_.elapsed()));
		pfl->SetData(ii, CHexFileList::VIEW_TIME, __int64(GetDocument()->view_time_.elapsed()));
		pfl->SetData(ii, CHexFileList::VERT_BUFFER_ZONE, __int64(GetVertBufferZone()));

		if (pdfv_ == NULL)
		{
			pfl->SetData(ii, CHexFileList::DFFDVIEW, "0");
			pfl->SetData(ii, CHexFileList::DFFDWIDTHS, "");
		}
		else
		{
			int width = 2;         // assume tab view used
			ASSERT(GetFrame()->splitter_.m_hWnd != 0);
			int snum_d = GetFrame()->splitter_.FindViewColumn(pdfv_->GetSafeHwnd());
			int dummy;        // ignored since we don't care about the height
			if (snum_d > -1)
			{
				GetFrame()->splitter_.GetColumnInfo(snum_d, width, dummy);
				if (width < 10) width = 10;    // Make sure it is not too narrow and reserve values 0-9 (2 = tab view)
			}
			pfl->SetData(ii, CHexFileList::DFFDVIEW, __int64(width));
			pfl->SetData(ii, CHexFileList::DFFDWIDTHS, pdfv_->GetColWidths());
		}
		// DFFDVIEW data is now 0 (none), 2 (tab). or 10+ (splitter width)

		if (pav_ == NULL)
		{
			pfl->SetData(ii, CHexFileList::AERIALVIEW, "0");
		}
		else
		{
			int width = 2;         // assume tab view used
			ASSERT(GetFrame()->splitter_.m_hWnd != 0);
			int snum_a = GetFrame()->splitter_.FindViewColumn(pav_->GetSafeHwnd());
			int dummy;        // ignored since we don't care about the height
			if (snum_a > -1)
			{
				GetFrame()->splitter_.GetColumnInfo(snum_a, width, dummy);
				if (width < 10) width = 10;    // Make sure it is not too narrow and reserve values 0-9 (2 = tab view)
			}
			pfl->SetData(ii, CHexFileList::AERIALVIEW, __int64(width));
		}
		// AERIALVIEW data is now 0 (none), 2 (tab). or 10+ (splitter width)

		if (pav_ != NULL)
			pav_->StoreOptions(pfl, ii);

		if (pcv_ == NULL)
		{
			pfl->SetData(ii, CHexFileList::COMPVIEW, "0");
		}
		else
		{
			int width = 2;         // assume tab view used
			ASSERT(GetFrame()->splitter_.m_hWnd != 0);
			int snum_c = GetFrame()->splitter_.FindViewColumn(pcv_->GetSafeHwnd());
			int dummy;        // ignored since we don't care about the height
			if (snum_c > -1)
			{
				GetFrame()->splitter_.GetColumnInfo(snum_c, width, dummy);
				if (width < 10) width = 10;    // Make sure it is not too narrow and reserve values 0-9 (2 = tab view)
			}
			pfl->SetData(ii, CHexFileList::COMPVIEW, __int64(width));
			pfl->SetData(ii, CHexFileList::COMPFILENAME, GetDocument()->GetCompFileName());
		}
#ifdef FILE_PREVIEW
		if (theApp.thumbnail_)
		{
			CMainFrame * mm = (CMainFrame *)AfxGetMainWnd();
			CChildFrame * cc = GetFrame();

			// Make sure main window is visible
			if (mm->IsIconic() ||!mm->IsWindowVisible())
				mm->ShowWindow(SW_SHOWNORMAL);    // If we don't do this (and main window is minimized) we get black bitmaps

			CRect rct;
			mm->GetWindowRect(&rct);  // Rectangle of main window
			if (!mm->IsZoomed() && OutsideMonitor(rct))
			{
				CRect cont_rect = MonitorRect(rct);
				if (rct.left < cont_rect.left)
					rct.MoveToX(cont_rect.left);
				if (rct.right > cont_rect.right)
					rct.MoveToX(cont_rect.right - rct.Width());
				if (rct.top < cont_rect.top)
					rct.MoveToY(cont_rect.top);
				if (rct.bottom > cont_rect.bottom)
					rct.MoveToY(cont_rect.bottom - rct.Height());

				mm->MoveWindow(&rct);
			}
			//mm->ActivateFrame(SW_SHOW);

			// Make sure this view is visible
			if (cc->IsIconic())
				cc->MDIRestore();
			cc->MDIActivate();       // if we don't do this we get the wrong window contents
			//CView * pv = mm->GetActiveView();
			//if (pv != this)
			//{
			//	mm->SetActiveView(this);
			//	RedrawWindow();
			//}

			HWND hh;                         // Handle of window we are generating the thumbnail for
			if (theApp.thumb_frame_)
				hh = cc->m_hWnd;             // frame window
			else
				hh = m_hWnd;                 // client area of hex view
			HDC dc = ::GetDC(hh);
			VERIFY(::GetWindowRect(hh, &rct));
			ASSERT(rct.Width() > 0 && rct.Height() > 0);

			::SetWindowPos(mm->m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  // make topmost
			//Sleep(500);    // allow time to refresh
			mm->RedrawWindow();              // Gets rid of stuff like context menu

			// Prepare the DCs
			HDC dstDC = ::GetDC(NULL);
			HDC srcDC = ::GetWindowDC(hh);
			HDC memDC = ::CreateCompatibleDC(dstDC);

			// Copy the window to the bitmap
			HBITMAP hbmp = CreateCompatibleBitmap(dstDC, rct.Width(), rct.Height());
			HBITMAP oldbm = (HBITMAP)::SelectObject(memDC, hbmp);
			VERIFY(::BitBlt(memDC, 0, 0, rct.Width(), rct.Height(), srcDC, 0, 0, SRCCOPY));

			::SetWindowPos(mm->m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);  // NOT topmost

			///if (pv != this)
			///	mm->SetActiveView(pv);

			// Get information about the bitmap
			BITMAP bm_info;
			VERIFY(::GetObject(hbmp, sizeof(bm_info), &bm_info));

			// Create the FreeImage bitmap
			FIBITMAP * dib = FreeImage_AllocateT(FIT_BITMAP, (WORD)bm_info.bmWidth, (WORD)bm_info.bmHeight, (WORD)bm_info.bmBitsPixel);
			if (dib != NULL)
			{
				if (bm_info.bmBitsPixel <= 8)
				{
					// Create palette if bpp is small (indicates paletted bitmap)
					RGBQUAD * pal = FreeImage_GetPalette(dib);
					for (int jj = 0; jj < FreeImage_GetColorsUsed(dib); ++jj)
					{
						pal[jj].rgbRed = jj;
						pal[jj].rgbGreen = jj;
						pal[jj].rgbBlue = jj;
					}
				}
				int num = FreeImage_GetColorsUsed(dib);

				HDC dc = ::GetDC(NULL);

				// Copy the pixels to the FreeImage bitmap
				if (::GetDIBits(dc, hbmp, 0,
								FreeImage_GetHeight(dib),   // number of scan lines to copy
								FreeImage_GetBits(dib),     // array for bitmap bits
								FreeImage_GetInfo(dib),     // bitmap data buffer
								DIB_RGB_COLORS))
				{
					::ReleaseDC(NULL, dc);

					FreeImage_GetInfoHeader(dib)->biClrUsed = num;
					FreeImage_GetInfoHeader(dib)->biClrImportant = num;

					FIBITMAP * dib_thumb = FreeImage_MakeThumbnail(dib, theApp.thumb_size_);
					FreeImage_Unload(dib);

					FIBITMAP * dib_final;

					// I had trouble with trying to use 8-bit bitmaps. Eg: FreeImage_ColorQuantize()
					// always fails, writing 8-bit bitmap as PNG converts to 32-bit, etc, so gave up
					//if ( theApp.thumb_8bit_ && FreeImage_GetBPP(dib_thumb) == 8 ||
					//	!theApp.thumb_8bit_ && FreeImage_GetBPP(dib_thumb) == 24)
					//{
					//	dib_final = dib_thumb;
					//}
					//else if (theApp.thumb_8bit_)
					//{
					//	//dib_final = FreeImage_ConvertTo8Bits(dib_thumb);  // creates grey-scale image
					//	dib_final = FreeImage_ColorQuantize(dib_thumb, FIQ_NNQUANT);
					//	FreeImage_Unload(dib_thumb);
					//}
					//else
					{
						dib_final = FreeImage_ConvertTo24Bits(dib_thumb);
						FreeImage_Unload(dib_thumb);
					}

					CString fname;
					if (dib_final != NULL && ::GetDataPath(fname))
					{
						// Get firectory and prefix for the file name
						fname += DIRNAME_PREVIEW;
						fname += _T("HEPV");

						CString strPrevious = pfl->GetData(ii, CHexFileList::PREVIEWFILENAME);
						if (!strPrevious.IsEmpty())
						{
							strPrevious = fname + strPrevious;
							::DeleteFileA(strPrevious);
						}

						// Create the directory for the preview files if it does not exist yet
						static bool first = true;
						if (first)
						{
							MakeSureDirectoryPathExists(fname);
							first = false;
						}

						// Work out what sort of file to write
						FREE_IMAGE_FORMAT fmt;
						int flags;
						const char * ext;
						switch (theApp.thumb_type_)
						{
						case CHexEditApp::PNG:
							fmt = FIF_PNG;
							//flags = PNG_Z_BEST_SPEED;
							flags = PNG_DEFAULT;
							ext = ".PNG";
							break;
						case CHexEditApp::JPEG_GOOD:
							fmt = FIF_JPEG;
							flags = JPEG_QUALITYNORMAL;
							ext = ".JPG";
							break;
						case CHexEditApp::JPEG_AVERAGE:
							fmt = FIF_JPEG;
							flags = JPEG_QUALITYAVERAGE;
							ext = ".JPG";
							break;
						default:
							ASSERT(0);
							// fall through
						case CHexEditApp::JPEG_BAD:
							fmt = FIF_JPEG;
							flags = JPEG_QUALITYBAD;
							ext = ".JPG";
							break;
						}

						static int last_vacant = 0;  // remember the last vacant spot we found to save a bit of time

						// Try different file names till we find a vacant slot
						for (int jj = last_vacant + 1; jj < 1000000; ++jj)
						{
							CString tt;
							tt.Format("%04d", jj);
							if (_access(fname + tt + ext, 0) < 0 && errno == ENOENT)
							{
								// File does not exist so use this file name
								fname += tt;
								fname += ext;

								// Save the thumbnail to the file and remember in recent file list
								if (FreeImage_Save(fmt, dib_final, fname, flags))
									pfl->SetData(ii, CHexFileList::PREVIEWFILENAME, tt + ext);   // no need to save full path

								last_vacant = jj;
								break;
							}
						}

						FreeImage_Unload(dib_final);
					}
				}
				else
				{
					::ReleaseDC(NULL, dc);
					FreeImage_Unload(dib);
				}
			}

			(void)::SelectObject(memDC, oldbm);
			::DeleteObject(hbmp);
			::DeleteDC(memDC);
		}
#endif // FILE_PREVIEW
	}
}

void CHexEditView::SetScheme(const char *name)
{
	if (scheme_name_ == name) return;    // no change to the scheme

	CString previous_name = scheme_name_;
	scheme_name_ = name;
	if (set_colours())
	{
		// Scheme changed so save for undo/macros
		undo_.push_back(view_undo(undo_scheme));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = previous_name;
		theApp.SaveToMacro(km_scheme, name);
	}
	else
	{
		// Scheme was not found (presumably in macro playback)
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("The color scheme \"%s\" was not found.\n\n"
			        "This scheme existed when the macro was recorded but no longer exists.", name);
		TaskMessageBox("Unknown Color Scheme", mess);
		theApp.mac_error_ = 1;
		scheme_name_ = previous_name;
	}
	DoInvalidate();
	show_prop();      // Stats page depends on current colour scheme
}

// When docked vertically the colour scheme drop down combo (ID_SCHEME) becomes a command
// button which when clicked invokes this.  We just open the Colour page of the Options dialog.
void CHexEditView::OnOptScheme()
{
	theApp.display_options(COLOUR_OPTIONS_PAGE, TRUE);
}

// Handle selection of a new scheme from colour scheme drop down combo (ID_SCHEME).
void CHexEditView::OnSelScheme()
{
	CMFCToolBarComboBoxButton * ptbcbb = NULL;

	// Get and search all ID_SCHEME toolbar controls (there may be more than one on different toolbars)
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons(::IsUs() ? ID_SCHEME_US : ID_SCHEME, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition();
			 posCombo != NULL;)
		{
			CMFCToolBarComboBoxButton* pp = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, listButtons.GetNext(posCombo));

			if (pp != NULL && CMFCToolBar::IsLastCommandFromButton(pp))  // check that this one actually was the one used
			{
				ptbcbb = pp;
				break;
			}
		}
	}

	if (ptbcbb != NULL)
	{
		ASSERT_VALID(ptbcbb);

		CString name = CString(ptbcbb->GetItem());  // GetItem(-1) will return the selected one
		if (name.Right(3) == "...")
			theApp.display_options(COLOUR_OPTIONS_PAGE, TRUE);
		else if (!name.IsEmpty() && name[0] != '-')
			SetScheme(name);
	}
}

// Update the colour scheme combo (ID_SCHEME).  We need to check that the current list of schemes
// matches the combo list AND that the scheme used in this view is the selected one.
void CHexEditView::OnUpdateScheme(CCmdUI* pCmdUI)
{
	// First check that it is a scheme combo box
	if (pCmdUI->m_pOther != NULL && pCmdUI->m_pOther->GetDlgCtrlID() == (::IsUs() ? ID_SCHEME_US : ID_SCHEME))
	{
		// Find the owning CMFCToolBarComboBoxButton of the combo box
		CMFCToolBarComboBoxButton * ptbcbb = NULL;

		CObList listButtons;
		if (CMFCToolBar::GetCommandButtons(::IsUs() ? ID_SCHEME_US : ID_SCHEME, listButtons) > 0)
		{
			for (POSITION posCombo = listButtons.GetHeadPosition();
				 posCombo != NULL;)
			{
				CMFCToolBarComboBoxButton* pp = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, listButtons.GetNext(posCombo));
				ASSERT(pp != NULL && pp->GetComboBox() != NULL);

				if (pp != NULL && pp->GetComboBox() != NULL && pp->GetComboBox()->m_hWnd == pCmdUI->m_pOther->m_hWnd)
				{
					ptbcbb = pp;
					break;
				}
			}
		}

		// If we found it and it's not in a dropped state
		if (ptbcbb != NULL && !ptbcbb->GetComboBox()->GetDroppedState())
		{
			CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

			// Work out the current scheme of the active view and get vector of scheme names
			std::vector<CString> scheme_names;
			int current_scheme = -1;

			// Build list backwards as ComboNeedsUpdate() assumes top of list is at bottom of vector
			scheme_names.push_back(CString(::IsUs() ? "Modify Colors..." : "Modify Colours..."));
			scheme_names.push_back(CString('-', 50));  // Add a divider line above the "Modify Colours" line
			for (int ii = theApp.scheme_.size(); ii > 0; ii--)
			{
				scheme_names.push_back(theApp.scheme_[ii-1].name_);
				if (theApp.scheme_[ii-1].name_ == GetSchemeName())
					current_scheme = ii - 1;
			}

			// Make sure the list of schemes in the combo matches the current schemes
			if (mm->ComboNeedsUpdate(scheme_names, ptbcbb->GetComboBox()))
			{
				int max_str = 0;                // Max width of all the strings added so far
				CClientDC dc(ptbcbb->GetComboBox());
				int nSave = dc.SaveDC();
				dc.SelectObject(ptbcbb->GetComboBox()->GetFont());

				ptbcbb->RemoveAllItems();
				for (std::vector<CString>::reverse_iterator ps = scheme_names.rbegin();
					 ps != scheme_names.rend(); ++ps)
				{
					if ((*ps)[0] != '-')
						max_str = __max(max_str, dc.GetTextExtent(*ps).cx);

					// Add the string to the list
					ptbcbb->AddItem(*ps);
				}
				// Add space for margin and possible scrollbar
				//max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
				max_str += ::GetSystemMetrics(SM_CXVSCROLL);
				ptbcbb->GetComboBox()->SetDroppedWidth(__min(max_str, 640));

				dc.RestoreDC(nSave);
			}

			// Make sure the selected scheme in the combo matches the scheme in use in this view
			if (ptbcbb->GetCurSel() != current_scheme)
			{
				ptbcbb->SelectItem(current_scheme);

				// We need to invalidate the button so it show the correct scheme
				CMFCToolBar *ptb = DYNAMIC_DOWNCAST(CMFCToolBar, ptbcbb->GetComboBox()->GetParent());
				int idx = ptb->CommandToIndex(::IsUs() ? ID_SCHEME_US : ID_SCHEME);
				ptb->InvalidateButton(idx);
			}

		}
	}
}

// Gets all colour info from the app's scheme vector based on the current scheme_name_.
// For efficiency it builds the "kala" array which contains a COLORREF for every byte value.
BOOL CHexEditView::set_colours()
{
	if (scheme_name_.IsEmpty())
		scheme_name_ = theApp.open_scheme_name_;  // Use default

	BOOL retval = TRUE;
	std::vector<CScheme>::const_iterator ps;
	ASSERT(theApp.scheme_.size() > 3);

	// First find the scheme
	for (ps = theApp.scheme_.begin(); ps != theApp.scheme_.end(); ++ps)
	{
		if (ps->name_ == scheme_name_)
			break;
	}

	// If the scheme was not found use the standard one matching selected options
	if (ps == theApp.scheme_.end())
	{
		// Current scheme name is not found so set to something OK and return error
		if (display_.char_set == CHARSET_EBCDIC)
			ps = theApp.scheme_.begin()+3;
		else if (display_.char_set == CHARSET_OEM)
			ps = theApp.scheme_.begin()+2;
		else if (display_.char_set == CHARSET_ANSI)
			ps = theApp.scheme_.begin()+1;
		else
			ps = theApp.scheme_.begin();
		scheme_name_ = ps->name_;
		retval = FALSE;
	}

	// Get colours from scheme, where -1 means use default (Automatic) colour
	bg_col_        = ps->bg_col_ == -1       ? ::GetSysColor(COLOR_WINDOW) : ps->bg_col_;
	mark_col_      = ps->mark_col_ == -1     ? RGB(224, 224, 224) : ps->mark_col_;
	hi_col_        = ps->hi_col_ == -1       ? RGB(255, 255, 0) : ps->hi_col_;              // yellow
	bm_col_        = ps->bm_col_ == -1       ? RGB(160, 192, 255) : ps->bm_col_;
	search_col_    = ps->search_col_ == -1   ? RGB(160, 255, 224) : ps->search_col_;
	addr_bg_col_   = ps->addr_bg_col_ == -1  ? ::tone_down(::GetSysColor(COLOR_INACTIVEBORDER), bg_col_, 0.5) : ps->addr_bg_col_;
	// Getting change tracking colour and make a version closer to the background colour in luminance
	// trk_col_ is used to underline replacements, trk_bg_col_ is used as background for insertions
	trk_col_ = ps->trk_col_ == -1 ? RGB(255, 128, 0) : ps->trk_col_;              // orange-red
	trk_bg_col_    = ::tone_down(trk_col_, bg_col_);
	comp_col_ = ps->comp_col_ == -1 ? RGB(255, 0, 128) : ps->comp_col_;           // purple
	comp_bg_col_   = ::tone_down(comp_col_, bg_col_);

	sector_col_    = ps->sector_col_ == -1   ? RGB(255, 160, 128)   : ps->sector_col_;          // pinkish orange
	sector_bg_col_ = ::tone_down(sector_col_, bg_col_);
	hex_addr_col_  = ps->hex_addr_col_ == -1 ? ::GetSysColor(COLOR_WINDOWTEXT) : ps->hex_addr_col_;
	dec_addr_col_  = ps->dec_addr_col_ == -1 ? ::GetSysColor(COLOR_WINDOWTEXT) : ps->dec_addr_col_;

	// Set the default text colour (used for text not specific to a range)
	if (ps->range_col_.size() > CColourSchemes::INDEX_LAST)
		text_col_ = ps->range_col_[CColourSchemes::INDEX_LAST]; // was back()
	else
		text_col_ = ::GetSysColor(COLOR_WINDOWTEXT);  // no ranges at all so use windows default

	int ii;

	// Default colours to background (makes unspecified colours invisible)
	for (ii = 0; ii < 256; ++ii)
		kala[ii] = bg_col_;

	ASSERT(ps->range_val_.size() == ps->range_col_.size());
	for (ii = ps->range_val_.size(); ii > 0; ii--)
	{
		COLORREF colour = (ps->range_col_[ii-1] == -1 ? ::GetSysColor(COLOR_WINDOWTEXT) : ps->range_col_[ii-1]);

		// Set colour for all in this range
		for (range_set<int>::const_iterator rr = ps->range_val_[ii-1].begin(); rr != ps->range_val_[ii-1].end(); ++rr)
		{
			kala[*rr] = add_contrast(colour, bg_col_);
		}
	}

	// Keep data format view colours in sync
	if (pdfv_ != NULL)
	{
		ASSERT_KINDOF(CDataFormatView, pdfv_);
		pdfv_->set_colours();
	}

	if (pav_ != NULL)
	{
		ASSERT_KINDOF(CAerialView, pav_);
		GetDocument()->AerialChange(this);
	}

	return retval;
}

void CHexEditView::get_colours(std::vector<COLORREF> &kk)
{
	int ii;

	// First find the scheme in app's list of schemes
	std::vector<CScheme>::const_iterator ps;
	for (ps = theApp.scheme_.begin(); ps != theApp.scheme_.end(); ++ps)
		if (ps->name_ == scheme_name_)
			break;

	// If the scheme was not found use the standard one matching selected options
	if (ps == theApp.scheme_.end())
	{
		// Current scheme name is not found so set to something OK and return error
		if (display_.char_set == CHARSET_EBCDIC)
			ps = theApp.scheme_.begin()+3;
		else if (display_.char_set == CHARSET_OEM)
			ps = theApp.scheme_.begin()+2;
		else if (display_.char_set == CHARSET_ANSI)
			ps = theApp.scheme_.begin()+1;
		else
			ps = theApp.scheme_.begin();
		scheme_name_ = ps->name_;
	}

	// Default colours to background (ie, init. unspecified colours)
	kk.clear();
	kk.reserve(256);
	kk.resize(256, bg_col_);

	ASSERT(ps->range_val_.size() == ps->range_col_.size());
	for (ii = ps->range_val_.size(); ii > 0; ii--)
	{
		COLORREF colour = (ps->range_col_[ii-1] == -1 ? ::GetSysColor(COLOR_WINDOWTEXT) : ps->range_col_[ii-1]);

		// Set colour for all in this range
		for (range_set<int>::const_iterator rr = ps->range_val_[ii-1].begin(); rr != ps->range_val_[ii-1].end(); ++rr)
		{
			assert(*rr < 256);
			kk[*rr] = colour;
		}
	}
}

CFont *CHexEditView::SetFont(CFont *ff)
{
	CFont *retval = CScrView::SetFont(ff);

	// Work out size of printer font based on the new screen font
	TEXTMETRIC tm;
	CClientDC dc(this);
	OnPrepareDC(&dc);                   // Get screen map mode/font
	dc.GetTextMetrics(&tm);
	CPoint convert(0, tm.tmHeight);
	dc.LPtoDP(&convert);                // Convert screen font size to device units
	fontsize_.Format("%d", int(convert.y*72.0/dc.GetDeviceCaps(LOGPIXELSX) + 0.5));

	dc.SetMapMode(print_map_mode_);
	dc.DPtoLP(&convert);                // Get printer font size in logical units
	print_lfHeight_ = labs(convert.y);

	return retval;
}

// functor object used in searching hl_set_.range_.
struct segment_compare
{
	bool operator()(const range_set<FILE_ADDRESS>::segment &x, const range_set<FILE_ADDRESS>::segment &y) const
	{
		return x.sfirst < y.sfirst;
	}
};

// These update hints are handled:
// CRemoveHint: removes undo info (without undoing anything) - for when document saved
// CUndoHint: undo changes up to last doc change - called prior to undoing changes
// CHexHint: indicates the document has changed (including as a result of doc undo)
// CBGSearchHint: indicates a background search on the doc has finished

void CHexEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	// Check if this is a change caused by a view
	if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CRemoveHint)))
	{
		// Remove undo info (as opposed to actually undoing everything as below)
		num_entered_ = num_del_ = num_bs_ = 0;  // Changes now gone from doc undo
		CRemoveHint *prh = dynamic_cast<CRemoveHint *>(pHint);
		std::vector <view_undo, allocator<view_undo> >::iterator pu;
		for (pu = undo_.end(); pu != undo_.begin(); pu--)
			if ((pu-1)->utype == undo_change)
			{
				// Remove everything up to & including the last doc change undo
				ASSERT((pu-1)->index == prh->index);
				undo_.erase(undo_.begin(), pu);
				break;
			}
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CUndoHint)))
	{
		num_entered_ = num_del_ = num_bs_ = 0;

		// Undo all view changes up to last doc change (no window invalidate)
		CUndoHint *puh = dynamic_cast<CUndoHint *>(pHint);

		// If this is not the view that is undoing
		if (puh->pview != this)
		{
			// Undo all view moves etc up to this change
			ASSERT(undo_.size() > 0);
			while (undo_.size() > 0 && undo_.back().utype != undo_change)
				do_undo();
			ASSERT(undo_.size() > 0);
			ASSERT(undo_.back().index == puh->index);
		}
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CHexHint)))
	{
		// This hint informs the view that the document has changed with
		// an insertion, deletion or overtype.  This may be as a result of a
		// document undo (if is_undo is true)

		// Get current selection before recalc_display changes row offsets
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());
		int prev_addr_width = addr_width_;

		// Make sure CScrView knows about any size changes
		if (display_.vert_display)
			SetTSize(CSizeAp(-1, ((GetDocument()->length() + offset_)/rowsize_ + 1)*3));  // 3 rows of text
		else
			SetTSize(CSizeAp(-1, (GetDocument()->length() + offset_)/rowsize_ + 1));

		recalc_display();  // file length, hence addr_width_, may have changed

		CHexHint *phh = dynamic_cast<CHexHint *>(pHint);
		ASSERT(phh->address >= 0 && phh->address <= GetDocument()->length());

		// Is this the start of a doc modification?
		// (phh->index == -1 if this a continued modification)
		if (!phh->is_undo && phh->index > -1)
		{
			// New change - save mark, caret + change on undo stack
			// This could be optimized to save mark and caret
			// only if they are moved by the change.
			undo_.push_back(view_undo(undo_setmark, phh->ptoo));
			undo_.back().address = mark_;
			undo_.push_back(view_undo(undo_move, TRUE));
			undo_.back().address = start_addr | (FILE_ADDRESS(row)<<62);
			if (start_addr != end_addr)
			{
				undo_.push_back(view_undo(undo_sel, TRUE));
				undo_.back().address = end_addr;
			}
			undo_.push_back(view_undo(undo_change, TRUE));
			if (phh->pview == NULL || phh->pview == this)
				undo_.back().flag = TRUE;
			else
				undo_.back().flag = FALSE;
#ifndef NDEBUG
			undo_.back().index = phh->index;
#endif
		}

		// Move positions of the caret and the mark if address shifted
		if (!phh->is_undo && (phh->utype == mod_insert || phh->utype == mod_insert_file))
		{
			if (end_addr > phh->address)
			{
				end_addr += phh->len;
				if (start_addr >= phh->address)
					start_addr += phh->len;
				SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
			}

			if (mark_ >= phh->address)
				mark_ += phh->len;
		}
		else if (!phh->is_undo &&
				 (phh->utype == mod_delback || phh->utype == mod_delforw))
		{
			// Check if current selection and the deletion intersect
			if (start_addr < phh->address + phh->len && end_addr > phh->address)
			{
				if (start_addr >= phh->address)     // If sel start within deletion ...
					start_addr = phh->address;      // ... move it to where chars deleted
				if (end_addr <= phh->address + phh->len) // If sel end within deletion ...
					end_addr = phh->address;        // ... move it to where chars deleted
				else
					end_addr -= phh->len;           // past deletion so just move it back
				SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
			}
			else if (phh->address + phh->len <= start_addr)
			{
				// Deletion is before selection - just move selection backwards
				SetSel(addr2pos(start_addr - phh->len, row), addr2pos(end_addr - phh->len, row));
			}

			if (mark_ > phh->address + phh->len)
				mark_ -= phh->len;
			else if (mark_ > phh->address)
				mark_ = phh->address;
		}

		// Fix highlights
		if (phh->utype == mod_insert || phh->utype == mod_insert_file)
		{
			range_set<FILE_ADDRESS>::range_t::iterator pp =
				lower_bound(hl_set_.range_.begin(),
							hl_set_.range_.end(),
							range_set<FILE_ADDRESS>::segment(phh->address+1, phh->address+1),
							segment_compare());
			// If there is a highlight before insert posn check if insert is within it
			if (pp != hl_set_.range_.begin())
			{
				pp --;
				// If bytes inserted within highlight move end
				if (pp->slast > phh->address)
					pp->slast += phh->len;
				++pp;
			}
			// Move up all the following highlights
			for ( ; pp != hl_set_.range_.end(); ++pp)
			{
				ASSERT(pp->sfirst > phh->address);
				pp->sfirst += phh->len;
				pp->slast += phh->len;
			}
		}
		else if (phh->utype == mod_delback || phh->utype == mod_delforw)
		{
			// Remove highlights for deleted bytes (if any)
			hl_set_.erase_range(phh->address, phh->address+phh->len);
			range_set<FILE_ADDRESS>::range_t::iterator pp =
				lower_bound(hl_set_.range_.begin(),
							hl_set_.range_.end(),
							range_set<FILE_ADDRESS>::segment(phh->address, phh->address),
							segment_compare());
			if (pp != hl_set_.range_.begin() && pp != hl_set_.range_.end())
			{
				range_set<FILE_ADDRESS>::range_t::iterator tmp = pp;
				tmp--;
				// If previous abuts current then join them together
				if (pp->sfirst - phh->len <= tmp->slast)
				{
					ASSERT(pp->sfirst - phh->len == tmp->slast);
					tmp->slast = pp->slast - phh->len;
					// Remove extra list elt (and leave pp pointing to next)
					tmp = pp;
					pp++;
					hl_set_.range_.erase(tmp);
				}
			}
			// Move all the following highlights down
			for ( ; pp != hl_set_.range_.end(); ++pp)
			{
				ASSERT(pp->sfirst > phh->address);
				pp->sfirst -= phh->len;
				pp->slast -= phh->len;
			}
		}

		// Work out the addresses of the first and last line displayed
		CRect rct;
		CRectAp clip_rect;                // rectangle for calcs/clipping

		// Calculate where the display in document
		GetDisplayRect(&rct);      // First: get client rectangle
		clip_rect = ConvertFromDP(rct);

		// Calculate the address of the first byte of the top row of the
		// display and the first byte of the row just past bottom
		FILE_ADDRESS addr_top = (clip_rect.top/line_height_)*rowsize_ - offset_;
		FILE_ADDRESS addr_bot = (clip_rect.bottom/line_height_ + 1)*rowsize_ - offset_;

		if (addr_width_ != prev_addr_width)
		{
			// Addresses on left side are now different width so redraw everything
			DoInvalidate();
		}
		else if (phh->address >= addr_bot ||
			(phh->address + /*(signed)*/ phh->len < addr_top &&
			 (phh->utype == mod_replace || phh->utype == mod_repback)))
		{
			// Do nothing - changes after display OR before but with no address shift
			; /* null statement */
		}
		else if (phh->address < addr_top + rowsize_ &&
				 phh->utype != mod_replace && phh->utype != mod_repback)
		{
			// Whole display changes, due to address shift
			DoInvalidate();
		}
		else if (phh->utype == mod_replace || phh->utype == mod_repback)
		{
			// Replace starting within display - just invalidate changed area
			invalidate_addr_range(phh->address, phh->address + (phh->len<1?1:phh->len));
		}
		else if (GetDocument()->length() +
					((phh->utype == mod_insert || phh->utype == mod_insert_file) ? -phh->len : +phh->len) < addr_bot)
		{
			// Insertion/deletion and previous end was within display
			// Just invalidate from address to bottom of display
			CRectAp inv(0, ((phh->address + offset_)/rowsize_) * line_height_,
					  char_pos(rowsize_), ((addr_bot + offset_)/rowsize_) * line_height_);
			CRect dev = ConvertToDP(inv);
			DoInvalidateRect(&dev);
		}
		else
		{
			// Must be insert/delete starting within the displayed area
			// - invalidate from first changed line to end of display
			invalidate_addr_range(phh->address, addr_bot);
		}

		// Remove doc change undo from undo array & do any assoc undos
		if (phh->is_undo && phh->pview != this)
		{
			ASSERT(undo_.size() > 0);
			if (undo_.size() > 0)
			{
				BOOL ptoo = undo_.back().previous_too;

				undo_.pop_back();       // Change already made to doc

//                while (ptoo)

				// Note changes made to allow for "previous_too" file changes
				while (ptoo && undo_.back().utype != undo_change)
				{
					if (undo_.size() == 0)
						break;
					// Undo anything associated to change to doc
					ptoo = undo_.back().previous_too;
					do_undo();
				}
			}
		}
		show_prop();
		show_calc();

		// If mark moved and current search is relative to mark then restart bg search
		if (theApp.align_rel_ && mark_ != GetDocument()->base_addr_)
		{
			GetDocument()->StopSearch();
			GetDocument()->base_addr_ = GetSearchBase();
			GetDocument()->StartSearch();
		}
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CBGSearchHint)))
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		CBGSearchHint *ph = dynamic_cast<CBGSearchHint *>(pHint);
		if (ph->finished_ == -1)
		{
			// Occurrences in area ph->start_ to ph->end_ have been added or removed
			// probably due to bytes being inserted or deleted at caret.

			// Get latest set of search occurrences
			get_search_in_range(GetScroll());

			// Redraw area where occurrences added/removed
			invalidate_addr_range(ph->start_, ph->end_ + search_length_ - 1);
		}
		else if (ph->finished_)
		{
			// Background search finished

			// Get the display area occurrences
			get_search_in_range(GetScroll());

			// Invalidate any areas where new search string was found
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp, pend;
			for (pp = search_pair_.begin(), pend = search_pair_.end(); pp != pend; ++pp)
				invalidate_addr_range(pp->first, pp->second);
		}
		else
		{
			// Remove all displayed search strings (but save a copy in tmp for invalidating)
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > tmp;
			tmp.swap(search_pair_);    // Save search_pair_ in tmp (and make it empty)

			// Invalidate any areas where search string is currently displayed
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp, pend;
			for (pp = tmp.begin(), pend = tmp.end(); pp != pend; ++pp)
				invalidate_addr_range(pp->first, pp->second);
		}
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CDFFDHint)))
	{
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> > tmp;
		tmp.swap(dffd_bg_);   // save old bg areas to invalidate below
		get_dffd_in_range(GetScroll());

		// Invalidate old and new areas
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> >::reverse_iterator pdffd, penddffd;
		for (pdffd = tmp.rbegin(), penddffd = tmp.rend(); pdffd != penddffd; ++pdffd)
			invalidate_addr_range(pdffd->get<0>(), pdffd->get<1>());
		for (pdffd = dffd_bg_.rbegin(), penddffd = dffd_bg_.rend(); pdffd != penddffd; ++pdffd)
			invalidate_addr_range(pdffd->get<0>(), pdffd->get<1>());
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CSaveStateHint)))
	{
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CRestoreStateHint)))
	{
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CBookmarkHint)))
	{
		CBookmarkHint *pbmh = dynamic_cast<CBookmarkHint *>(pHint);
		invalidate_addr_range(pbmh->addr_, pbmh->addr_+1);
	}
	else if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CTrackHint)))
	{
		if (!display_.hide_replace || !display_.hide_insert || !display_.hide_delete)
		{
			CTrackHint *pth = dynamic_cast<CTrackHint *>(pHint);
			invalidate_addr_range(pth->start_, pth->end_);
		}
	}
	else
	{
		recalc_display();
		CScrView::OnUpdate(pSender, lHint, pHint);
	}
}

void CHexEditView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	if (bActivate)
		GetDocument()->SearchThreadPriority(THREAD_PRIORITY_LOWEST);
	else
		GetDocument()->SearchThreadPriority(THREAD_PRIORITY_IDLE);

	CScrView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

// Check if the file is in OVR mode, giving the user the option to switch to INS mode if possible
// Returns TRUE if still in OVR mode on return, or FALSE if in INS mode
BOOL CHexEditView::check_ovr(const char *desc)
{
	if (!display_.overtype)
		return FALSE;         // already in INS mode

	CString ss;
	ss.Format("You can't %s while in overtype mode.", desc);

	// First check if we can turn on INS mode
	if (display_.readonly)
	{
		TaskMessageBox("Not Allowed in OVR Mode",
			ss + "\n\nAlso note that you can't turn on insert mode when the file is read only.");
		theApp.mac_error_ = 10;
		return TRUE;
	}
	if (GetDocument()->IsDevice())
	{
		TaskMessageBox("Not Allowed in OVR Mode",
			ss + "\n\nAlso note that you can't turn on insert mode for devices (logical volumes and physical disks).");
		theApp.mac_error_ = 10;
		return TRUE;
	}

	if (CAvoidableDialog::Show(IDS_OVERTYPE,
							   ss + "\n\nDo you want to turn off overtype mode?",
							   "", 
							   MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
	{
		theApp.mac_error_ = 5;
		return TRUE;
	}
	else if (!do_insert())
		return TRUE;

	return FALSE;  // return FALSE to indicate we are no longer in OVR mode
}

// Checks if the document (file) or view (window) is read only.
// If the document is read only it just displays a message and returns.
// If the view is read only it gives the user the option to turn off RO.
// Returns TRUE if the file/view is read only, FALSE if modifiable.
BOOL CHexEditView::check_ro(const char *desc)
{
	CString ss;
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);

	if (display_.readonly && GetDocument()->read_only())
	{
		ss.Format("This file cannot be modified as it is open for read only.\n\n"
				  "For this reason you cannot %s.", desc);
		TaskMessageBox("File is read only", ss);
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		aa->mac_error_ = 10;
		return TRUE;
	}
	else if (pdfv_ != NULL && pdfv_->ReadOnly(start_addr, end_addr))
	{
		ss.Format("The selection contains one or "
				  "more read-only template fields, "
				  "hence you can't %s.", desc);
		TaskMessageBox("Read only field", ss);
		theApp.mac_error_ = 10;
		return TRUE;
	}
	else if (display_.readonly)
	{
		ss.Format("You can't %s while this window is read only.\n\n"
				  "Do you want to turn off read only mode?", desc);
		if (CAvoidableDialog::Show(IDS_READ_ONLY, ss, "", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
		{
			CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
			aa->mac_error_ = 10;
			return TRUE;
		}
		else
			allow_mods();
	}
	return FALSE;  // We were or now are in RW mode and there are no RO template fields in the selection
}

// Check if we have had read errors on the device and warn the user (but only once)
// Note that this will warn once per view not once per file but we don't care too much.
void CHexEditView::check_error()
{
	if (GetDocument()->HasSectorErrors())
	{
		// turn on sector display so user can see which sector(s) had errors
		display_.borders = 1;

		if (!errors_mentioned_)
		{
			TaskMessageBox("Read Error",
				"One or more bad sectors were reported when attempting to read from the device.");
			errors_mentioned_ = true;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditView drawing

// There are several different coordinate systems used.
//  - device: pixels on the physical device (screen or printer)
//  - logical: units set by windows mapping mode
//             for screen MM_TEXT is used (= device coords) with Y axis down
//             for printer MM_HIMETRIC is used with Y axis up
//  - normalised: this is the same as logical but the sign is changed so
//             that Y axis is always down (and X axis is always right)
//             in order to simplify comparisons etc
//  - document: this is the normalised logical coord system with the origin at
//             the very top of the document.
// Note that different mapping modes are used for screen and printer.
//  MM_TEXT is used for screen otherwise scrolling becomes slightly
//  out of whack resulting in missing or extra lines of pixels.
//  MM_TEXT cannot be used for the printer as it comes out tiny on lasers.
//

void CHexEditView::OnDraw(CDC* pDC)
{
	if (pfont_ == NULL) return;

	CHexEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	/* This was to allow print preview to be grey when active printer was
	 * monochrome, but somehow it's already OK.
	bool preview =  pDC->GetDeviceCaps(NUMCOLORS) == 2 &&
					pDC->m_hDC != pDC->m_hAttribDC;
	 */

	//CBrush backBrush;
	//backBrush.CreateSolidBrush(bg_col_);
	//backBrush.UnrealizeObject();

	pDC->SetBkMode(TRANSPARENT);

	// Are we in overtype mode and file is empty?
	if (display_.overtype && pDoc->length() == 0)
	{
		// Get the reason that there's no data from the document & display it
		CRect cli;
		GetDisplayRect(&cli);
		pDC->DPtoLP(&cli);
		if (pDC->IsPrinting())
		{
			if (cli.bottom < 0)
				cli.top = -2 * print_text_height_;
			else
				cli.top = 2 * print_text_height_;
		}
		const char *mm = pDoc->why0();
		pDC->SetTextColor(GetDefaultTextCol());
		pDC->DrawText(mm, -1, &cli, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		//::TextOutW(pDC->GetSafeHdc(), 0, 0, L"TEST", 4);  // this works!
		return;
	}

	const char *hex;
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	CRectAp doc_rect;                           // Display area relative to whole document
	CRect norm_rect;                            // Display area (norm. logical coords)
	// Note that norm_rect may extend slightly outside the physical view area, for example,
	// if only half a line is visible at the top of the screen.

	// These are the first and last "virtual" addresses of the top and (one past) the end
	// of the addresses within norm_rect.  Note that these are not necessarilly the first
	// and last real addresses.  For example, if offset_ != 0 and at the top of file then
	// first_virt will be -ve.  Similarly the file may not even be as long as last_virt.
	FILE_ADDRESS first_virt, last_virt;
	FILE_ADDRESS first_line;                    // First line that needs displaying
	FILE_ADDRESS last_line;                     // One past last line to display

	FILE_ADDRESS first_addr = 0;                // First address to actually display
	FILE_ADDRESS last_addr = pDoc->length();    // One past last address actually displayed

	FILE_ADDRESS line_inc;                      // 1 or -1 depending on draw dirn (up/down)
	CSize rect_inc;                             // How much to move norm_rect each time
	FILE_ADDRESS start_addr, end_addr;          // Address of current selection
	bool neg_x(false), neg_y(false);            // Does map mode have -ve to left or down
	bool has_focus;                             // Does this window have focus?
	int bitspixel = pDC->GetDeviceCaps(BITSPIXEL);
	if (bitspixel > 24) bitspixel = 24;         // 32 is really only 24 bits of colour
	long num_colours = 1L << (bitspixel*pDC->GetDeviceCaps(PLANES));

//    CBCGDrawManager bcgdm(*pDC);

	int line_height, char_width, char_width_w;  // Text sizes

	ASSERT(offset_ >= 0 && offset_ < rowsize_);
	if (offset_ < 0 || offset_ >= rowsize_) offset_ = 0; // xxx kludge - need to track down why offset is wrong
	ASSERT(rowsize_ > 0 && rowsize_ <= max_buf);
	ASSERT(group_by_ > 0);

	if (pDC->IsPrinting())
	{
		// Work out "client" rect of a printer page
		norm_rect.top = norm_rect.left = 0;
		norm_rect.bottom = pDC->GetDeviceCaps(VERTRES);
		norm_rect.right  = pDC->GetDeviceCaps(HORZRES);

		// Convert to logical units but with origin at top left of window
		// Note we can't use ConvertFromDP here as this is for printer not screen
		pDC->DPtoLP(&norm_rect);
		if (norm_rect.right < 0)
		{
			neg_x = true;
			norm_rect.right = -norm_rect.right;
		}
		if (norm_rect.bottom < 0)
		{
			neg_y = true;
			norm_rect.bottom = -norm_rect.bottom;
		}

		// Text-only printer drivers under Win98 return number of text lines not pixels for VERTRES
		if (norm_rect.bottom < norm_rect.right/5)
			norm_rect.bottom *= print_text_height_;

		// Since there is no translation (only scaling) between device and logical coords origin does not change
		ASSERT(norm_rect.top == 0 && norm_rect.left == 0);

		// Work out which part of document to display
		if (display_.vert_display)
			doc_rect = CRectAp(norm_rect) + 
				CSizeAp(-margin_size_.cx, curpage_ * FILE_ADDRESS(lines_per_page_) * print_text_height_*3 - margin_size_.cy);
		else
			doc_rect = CRectAp(norm_rect) + 
				CSizeAp(-margin_size_.cx, curpage_ * FILE_ADDRESS(lines_per_page_) * print_text_height_ - margin_size_.cy);

		line_height = print_text_height_;
		if (display_.vert_display) line_height *= 3;
		char_width = print_text_width_;
		char_width_w = print_text_width_w_;
	}
	else
	{
		has_focus = (GetFocus() == this);
		HideCaret();
		neg_x = negx(); neg_y = negy();

		// Get display rect in logical units but with origin at top left of display area in window
		CRect rct;
		GetDisplayRect(&rct);
		doc_rect = ConvertFromDP(rct);

		// Display = client rectangle translated to posn in document
//        norm_rect = doc_rect - GetScroll();
		norm_rect.left  = doc_rect.left  - GetScroll().x + bdr_left_;
		norm_rect.right = doc_rect.right - GetScroll().x + bdr_left_;
		norm_rect.top    = int(doc_rect.top    - GetScroll().y) + bdr_top_;
		norm_rect.bottom = int(doc_rect.bottom - GetScroll().y) + bdr_top_;

		// Get the current selection so that we can display it in reverse video
		GetSelAddr(start_addr, end_addr);

		line_height = line_height_;
		char_width = text_width_;
		char_width_w = text_width_w_;
	}

	// Get range of addresses that are visible the in window (overridden for printing below)
	first_virt = (doc_rect.top/line_height) * rowsize_ - offset_;
	last_virt  = (doc_rect.bottom/line_height + 1) * rowsize_ - offset_;

	// Work out which lines could possibly be in the display area
	if (pDC->IsPrinting() && print_sel_)
	{
		GetSelAddr(first_addr, last_addr);

		// Start drawing from start of selection (+ allow for subsequent pages)
		first_line = (first_addr+offset_)/rowsize_ + curpage_*FILE_ADDRESS(lines_per_page_);
		last_line = first_line + lines_per_page_;

		// Also if the selection ends on this page set the end
		if (last_line > (last_addr - 1 + offset_)/rowsize_ + 1)
			last_line = (last_addr - 1 + offset_)/rowsize_ + 1;

		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		first_virt = first_line * rowsize_ - offset_;
		last_virt  = first_virt + lines_per_page_*rowsize_;

		// Things are a bit different if merging duplicate lines
		if (dup_lines_)
		{
			if (curpage_ == 0)
				print_next_line_ = first_line;
			else
				first_line = print_next_line_;
			//  We don't know what the last line will be so set to end of selection
			last_line = (last_addr - 1 + offset_)/rowsize_ + 1;
			last_virt = last_line * rowsize_ - offset_;
		}

		/* Work out where to display the 1st line */
		norm_rect.top += margin_size_.cy;
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;

		doc_rect.top = first_line * line_height - margin_size_.cy;
		doc_rect.bottom = doc_rect.top + lines_per_page_*print_text_height_;
	}
	else if (pDC->IsPrinting())
	{
		// Draw just the lines on this page
		first_line = curpage_ * FILE_ADDRESS(lines_per_page_);
		last_line = first_line + lines_per_page_;
		first_addr = max(0, first_line*rowsize_ - offset_);
		last_addr = min(pDoc->length(), last_line*rowsize_ - offset_);

		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		first_virt = first_line * rowsize_ - offset_;
		last_virt  = first_virt + lines_per_page_*rowsize_;

		/* Work out where to display the 1st line */
//      norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.top += margin_size_.cy;
		norm_rect.bottom = norm_rect.top + line_height;
//        norm_rect.left +=  margin_size_.cx - doc_rect.left;
		norm_rect.left -= doc_rect.left;
	}
	else if (ScrollUp())
	{
		// Draw from bottom of window up since we're scrolling up (looks better)
		first_line = doc_rect.bottom/line_height;
		last_line = doc_rect.top/line_height - 1;
		line_inc = -1L;
		rect_inc = CSize(0, -line_height);

		/* Work out where to display the 1st line */
		norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;
	}
	else
	{
		// Draw from top of window down
		first_line = doc_rect.top/line_height;
		last_line = doc_rect.bottom/line_height + 1;
		line_inc = 1L;
		rect_inc = CSize(0, line_height);

		/* Work out where to display the 1st line */
		norm_rect.top -= int(doc_rect.top - first_line*line_height);
		norm_rect.bottom = norm_rect.top + line_height;
		norm_rect.left -= doc_rect.left;
	}

	if (first_addr < first_virt) first_addr = first_virt;
	if (last_addr > last_virt) last_addr = last_virt;

	// These are for drawing things on the screen
	CPoint pt;   // moved this here to avoid a spurious compiler error C2362
	CPen pen1(PS_SOLID, 0, same_hue(sector_col_, 100, 30));    // dark sector_col_
	CPen pen2(PS_SOLID, 0, same_hue(addr_bg_col_, 100, 30));   // dark addr_bg_col_
	CPen *psaved_pen;
	CBrush brush(sector_col_);

	// Skip background drawing in this case because it's too hard.
	// Note that this goto greatly simplifies the tests below.
	if (pDC->IsPrinting() && print_sel_ && dup_lines_)
		goto end_of_background_drawing;

	// Preread device blocks so we know if there are bad sectors
	if (GetDocument()->IsDevice())
	{
		// As long as we ensure that the CFileNC buffer is at least as big as the display area (the
		// distance from start of top line to end of bottom line) then this should read all
		// sectors to be dipslayed.
		unsigned char cc;
		TRACE("---- Display size is %ld\n", long(last_addr - first_addr));
		pDoc->GetData(&cc, 1, (first_addr + last_addr)/2);
	}

	if (display_.borders)
		psaved_pen = pDC->SelectObject(&pen1);
	else
		psaved_pen = pDC->SelectObject(&pen2);

	// Column related screen stuff (ruler, vertical lines etc)
	if (!pDC->IsPrinting())
	{
		ASSERT(!neg_y && !neg_x);       // This should be true when drawing on screen (uses MM_TEXT)
		// Vert. line between address and hex areas
		pt.y = bdr_top_ - 4;
		pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
		pDC->MoveTo(pt);
		pt.y = 30000;
		pDC->LineTo(pt);
		if (!display_.vert_display && display_.hex_area)
		{
			// Vert line to right of hex area
			pt.y = bdr_top_ - 4;
			pt.x = char_pos(0, char_width) - char_width_w/2 - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (display_.vert_display || display_.char_area)
		{
			// Vert line to right of char area
			pt.y = bdr_top_ - 4;
			pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
				   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.y = 30000;
			pDC->LineTo(pt);
		}
		if (theApp.ruler_)
		{
			int horz = bdr_left_ - GetScroll().x;       // Horiz. offset to window posn of left side

			ASSERT(bdr_top_ > 0);
			// Draw horiz line under ruler
			pt.y = bdr_top_ - 4;
			pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			pt.x = 30000;
			pDC->LineTo(pt);

			// Draw ticks using hex offsets for major ticks (if using hex addresses) or
			// decimal offsets (if using decimal addresses/line numbers and/or hex addresses)
			int major = 1;
			if (display_.decimal_addr || display_.line_nums)
				major = theApp.ruler_dec_ticks_;        // decimal ruler or both hex and decimal
			else
				major = theApp.ruler_hex_ticks_;        // only showing hex ruler
			// Hex area ticks
			if (!display_.vert_display && display_.hex_area)
				for (int column = 1; column < rowsize_; ++column)
				{
					if ((!display_.decimal_addr && !display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = bdr_top_ - 5;
					pt.x = hex_pos(column) - char_width/2 + horz;
					if (column%group_by_ == 0)
						pt.x -= char_width/2;
					pDC->MoveTo(pt);
					pt.y -= (column%major) ? 3 : 7;
					pDC->LineTo(pt);
				}
			// Char area or stacked display ticks
			if (display_.vert_display || display_.char_area)
				for (int column = 0; column <= rowsize_; ++column)
				{
					if ((!display_.decimal_addr && !display_.line_nums && theApp.ruler_hex_nums_ > 1 && column%theApp.ruler_hex_nums_ == 0) ||
						((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1 && column%theApp.ruler_dec_nums_ == 0) )
						continue;       // skip when displaying a number at this posn
					pt.y = bdr_top_ - 5;
					pt.x = char_pos(column) + horz;
					if (display_.vert_display && column%group_by_ == 0)
						if (column == rowsize_)    // skip last one
							break;
						else
							pt.x -= char_width/2;
					pDC->MoveTo(pt);
					pt.y -= (column%major) ? 2 : 5;
					pDC->LineTo(pt);
				}

			// Draw numbers in the ruler area
			// Note that if we are displaying hex and decimal addresses we show 2 rows
			//      - hex offsets at top (then after moving vert down) decimal offsets
			int vert = 0;                       // Screen y pixel to the row of nos at
			int hicol = -1;                     // Column with cursor is to be highlighted

			bool hl_box = true;               // Highlight mouse/cursor in the ruler using box around col number (or just a small line)
			if (display_.hex_addr && theApp.ruler_hex_nums_ > 1) hl_box = false;
			if ((display_.decimal_addr || display_.line_nums) && theApp.ruler_dec_nums_ > 1) hl_box = false;

			CRect hi_rect(-1, 0, -1, bdr_top_ - 4);
			if (!hl_box)
				hi_rect.top = hi_rect.bottom - 2;    // A flat rect just inside the ruler
			// Current caret position shown in the ruler
			if (theApp.hl_caret_ && !mouse_down_)
			{
				// Do highlighting with a background rectangle in ruler
				hicol = int((start_addr + offset_)%rowsize_);
				CBrush * psaved_brush = pDC->SelectObject(&brush);
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				if (!display_.vert_display && display_.hex_area)
				{
					hi_rect.left  = hex_pos(hicol) + horz;
					hi_rect.right = hi_rect.left + text_width_*2 + 1;
					pDC->Rectangle(&hi_rect);
				}
				if (display_.vert_display || display_.char_area)
				{
					hi_rect.left  = char_pos(hicol) + horz;
					hi_rect.right = hi_rect.left + text_width_ + 1;
					pDC->Rectangle(&hi_rect);
				}
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}
			// Current mouse position in the ruler
			if (theApp.hl_mouse_ && mouse_addr_ > -1)
			{
				int mousecol = int((mouse_addr_ + offset_)%rowsize_);   // Mouse column to be highlighted
				CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				if (!display_.vert_display && display_.hex_area)
				{
					hi_rect.left  = hex_pos(mousecol) + horz;
					hi_rect.right = hi_rect.left + text_width_*2 + 1;
					pDC->Rectangle(&hi_rect);
				}
				if (display_.vert_display || display_.char_area)
				{
					hi_rect.left  = char_pos(mousecol) + horz;
					hi_rect.right = hi_rect.left + text_width_ + 1;
					pDC->Rectangle(&hi_rect);
				}
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			// Get display rect for clipping at right and left
			CRect cli;
			GetDisplayRect(&cli);

			// Show hex offsets in the top border (ruler)
			if (display_.hex_addr)
			{
				bool between = theApp.ruler_hex_nums_ > 1;      // Only display numbers above cols if displaying for every column

				// Do hex numbers in ruler
				CRect rect(-1, vert, -1, vert + text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetHexAddrCol());   // Colour of hex addresses

				// Show hex offsets above hex area
				if (!display_.vert_display && display_.hex_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // usee doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;
						if (!between)
						{
							// Draw 2 digit number above every column
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", (column + display_.addrbase1)%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%16 == 0 && theApp.ruler_hex_nums_ > 3)
						{
							// Draw 2 digit numbers to mark end of 16 columns
							rect.left -= (char_width+1)/2;
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", column%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							// Draw single digit number in between columns
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", column%16);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				// Show hex offsets above char area or stacked display
				if (display_.vert_display || display_.char_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_hex_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (display_.vert_display && column > 0 && column%group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;

						if (!between)
						{
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", (column + display_.addrbase1)%16);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%16 == 0 && theApp.ruler_hex_nums_ > 3)
						{
							rect.left -= (char_width+1)/2;
							ss.Format(theApp.hex_ucase_ ? "%02X" : "%02x", column%256);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							ss.Format(theApp.hex_ucase_ ? "%1X" : "%1x", column%16);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				vert += text_height_;  // Move down for anything to be drawn underneath
			}
			// Show decimal offsets in the ruler
			if (display_.decimal_addr || display_.line_nums)
			{
				bool between = theApp.ruler_dec_nums_ > 1;      // Only display numbers above cols if displaying for every column

				CRect rect(-1, vert, -1, vert + text_height_ + 1);
				CString ss;
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses

				// Decimal offsets above hex area
				if (!display_.vert_display && display_.hex_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left  = hex_pos(column) + horz;
						if (between)
						{
							rect.left -= char_width;
							if (column > 0 && column%group_by_ == 0)
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;
						if (!between)
						{
							ss.Format("%02d", (column + display_.addrbase1)%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%10 == 0 && theApp.ruler_dec_nums_ > 4)
						{
							rect.left -= (char_width+1)/2;
							ss.Format("%02d", column%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							ss.Format("%1d", column%10);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				// Decimal offsets above char area or stacked display
				if (display_.vert_display || display_.char_area)
					for (int column = 0; column < rowsize_; ++column)
					{
						if (between && display_.addrbase1 && column == 0)
							continue;           // user doesn't like seeing zero
						if (column%theApp.ruler_dec_nums_ != 0)
							continue;
						rect.left = char_pos(column) + horz;
						if (between)
						{
							if (display_.vert_display && column > 0 && column%group_by_ == 0)
								rect.left -= char_width;
							else
								rect.left -= char_width/2;
						}
						if (rect.left < cli.left)
							continue;
						if (rect.left > cli.right)
							break;
						rect.right = rect.left + text_width_ + text_width_;

						if (!between)
						{
							ss.Format("%1d", (column + display_.addrbase1)%10);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else if (column%10 == 0 && theApp.ruler_dec_nums_ > 4)
						{
							// If displaying nums every 5 or 10 then display 2 digits fo tens column
							rect.left -= (char_width+1)/2;
							ss.Format("%02d", column%100);
							pDC->DrawText(ss, &rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
						else
						{
							// Display single dit between columns
							ss.Format("%1d", column%10);
							pDC->DrawText(ss, &rect, DT_BOTTOM | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				vert += text_height_;   // Move down for anything to be drawn underneath (currently nothing)
			}
#ifdef RULER_ADJUST
			draw_adjusters(pDC);           // Draw adjustment controls in the ruler
#endif
		} // end ruler drawing
		if (real_offset_ > 0 && real_offset_ >= first_virt && real_offset_ < last_virt)
		{
			// Horizontal line at offset
			pt.y = int(((real_offset_ - 1)/rowsize_ + 1) * line_height - doc_rect.top + bdr_top_);
			if (neg_y) pt.y = -pt.y;
			pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
			pDC->MoveTo(pt);
			//pt.x = 30000;
			if (!display_.vert_display && display_.hex_area && !display_.char_area)
				pt.x = char_pos(0, char_width) - char_width_w/2 - doc_rect.left + bdr_left_;
			else
				pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
					   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
			pDC->LineTo(pt);
		}
	}

	// Mask out the ruler so we don't get top of topmost line drawn into it.
	// Doing it this way allows the address of the top line to be drawn
	// higher (ie into ruler area) without being clipped.
	// Note that we currently only use bdr_top_ (for the ruler) but if
	// other borders are used similarly we would need to clip them too.
	// Note: This needs to be done after drawing the ruler.
#ifndef TEST_CLIPPING
	if (!pDC->IsPrinting())
	{
		CRect rct;
		GetDisplayRect(&rct);
		rct.bottom = rct.top;
		rct.top = 0;
		rct.left = (addr_width_ - 1)*char_width + norm_rect.left + bdr_left_;
		pDC->ExcludeClipRect(&rct);
	}
#endif

	// Note: We draw bad sectors, change-tracking deletions, etc first
	// as they are always drawn from the top of screen (or page) downwards.
	// This is so the user is less likely to notice the wrong direction.
	// (Major things are drawn from the bottom up when scrolling backwards.)

	// First decide is we need to draw sector borders
	int seclen = pDoc->GetSectorSize();
	bool draw_borders = pDoc->pfile1_ != NULL && display_.borders && seclen > 0;
	if (pDC->IsPrinting() && !theApp.print_sectors_)
		draw_borders = false;

	if (draw_borders)
	{
		ASSERT(seclen > 0);
		bool prev_bad = first_addr == 0;                  // don't display sector separator above top of file

		for (FILE_ADDRESS sector = (first_addr/seclen)*seclen; sector < last_addr; sector += seclen)
		{
			// Note that "sector" is the address of the start of the sector
			if (pDoc->HasSectorErrors() && pDoc->SectorError(sector/seclen) != NO_ERROR)
			{
				// Draw colour behind the bytes to indicate there is a problem with this sector
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, sector_bg_col_,
						max(sector, first_addr),
						min(sector + seclen, last_addr));
				prev_bad = true;
			}
			else if (!prev_bad && sector >= first_addr && sector < last_addr)
			{
				// Just draw a line above the top of the sector
				if (!display_.vert_display && display_.hex_area)
				{
					// Hex area
					pt.y = int(((sector + offset_)/rowsize_) * line_height -
							   doc_rect.top + bdr_top_);     // This is just above the first byte of the sector
					if (neg_y) pt.y = -pt.y;

					//pt.x = hex_pos(rowsize_ - 1, char_width) + 2*char_width - doc_rect.left + bdr_left_;
					pt.x = char_pos(0, char_width) - char_width/2 - doc_rect.left + bdr_left_;  // Right side of hex area
					if (neg_x) pt.x = - pt.x;
					pDC->MoveTo(pt);
					pt.x = hex_pos(int((sector + offset_)%rowsize_), char_width) - 
						   char_width/2 - doc_rect.left + bdr_left_;  // This is just to left of first byte of sector
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);

					if ((sector + offset_)%rowsize_ != 0)
					{
						// Draw on line below and vertical bit too
						pt.y = int(((sector + offset_)/rowsize_ + 1) * line_height - 
								   doc_rect.top + bdr_top_);
						if (neg_y) pt.y = -pt.y;
						pDC->LineTo(pt);
						pt.x = hex_pos(0, char_width) - doc_rect.left + bdr_left_;
						if (neg_x) pt.x = - pt.x;
						pDC->LineTo(pt);
					}
					pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
					pDC->LineTo(pt);
				}
				if (display_.vert_display || display_.char_area)
				{
					// Do char area (or stacked mode)
					pt.y = int(((sector + offset_)/rowsize_) * line_height -
							   doc_rect.top + bdr_top_);
					if (neg_y) pt.y = -pt.y;

					//pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + char_width_w - doc_rect.left + bdr_left_;
					pt.x = char_pos(rowsize_ - 1, char_width, char_width_w) + 
						   (3*char_width_w)/2 - doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->MoveTo(pt);
					pt.x = char_pos(int((sector + offset_)%rowsize_), char_width, char_width_w) -
						   doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);

					if ((sector + offset_)%rowsize_ != 0)
					{
						// Draw on line below and vertical bit too
						pt.y = int(((sector + offset_)/rowsize_ + 1) * line_height -
								   doc_rect.top + bdr_top_);
						if (neg_y) pt.y = -pt.y;
						pDC->LineTo(pt);
						pt.x = char_pos(0, char_width, char_width_w) - doc_rect.left + bdr_left_;
						if (neg_x) pt.x = - pt.x;
						pDC->LineTo(pt);
					}
					// Fill in a little bit to join with the vertical line on the left
					if (display_.vert_display || !display_.hex_area)
						pt.x = addr_width_*char_width - char_width - doc_rect.left + bdr_left_;
					else
						pt.x = char_pos(0, char_width, char_width_w) -
							   char_width_w/2 - doc_rect.left + bdr_left_;
					if (neg_x) pt.x = - pt.x;
					pDC->LineTo(pt);
				}
				// Draw a little bit more in the gap between address area and left side
				pt.x = addr_width_*char_width - char_width/2 - doc_rect.left + bdr_left_;
				if (neg_x) pt.x = - pt.x;
				pDC->MoveTo(pt);
				pt.x = addr_width_*char_width + char_width/8 -  // Extra char_width/8 due to one pixel out on screen (and many on printer)
					   doc_rect.left + bdr_left_;
				if (neg_x) pt.x = - pt.x;
				pDC->LineTo(pt);
			}
			else
			{
				prev_bad = false;  // don't draw border at bottom of previous block fill (just remember this one was OK)
			}
		}
	}
	pDC->SelectObject(psaved_pen);      // restore pen after drawing borders etc

	// Make sure background compare is on and finished and also print_compare_ is on (if printing)
	if (!((GetDocument()->CompareDifferences() <= 0) || pDC->IsPrinting() && !theApp.print_compare_))
	{
		// Draw differences with compare file
		CTime tnew = GetDocument()->ResultTime(0);         // time of most recent comparison
		// xxx TBD make "number of minutes before it completely disappears" into a parameter
		CTime tearliest = tnew - CTimeSpan(0, 0, 15, 0);   // older diffs are shown in lighter shades
		for (int rr = GetDocument()->ResultCount() - 1; rr >= 0; rr--)
		{
			COLORREF col, col2;   // colours for replace (underline) + delete, and also for inserts
			CTime tt = GetDocument()->ResultTime(rr);
			if (tt < tearliest)
				continue;
			// Tone down based on how long agoo the change was made (up to 0.8 since toning down more is not that visible)
			double amt = 0.8 - double((tt - tearliest).GetTotalSeconds()) / double((tnew - tearliest).GetTotalSeconds());
			col = ::tone_down(comp_col_, bg_col_, amt);
			col2 = ::tone_down(comp_bg_col_, bg_col_, amt);
			for (int dd = GetDocument()->FirstDiffAt(false, rr, first_virt); dd < GetDocument()->CompareDifferences(rr); ++dd)
			{
				FILE_ADDRESS addr;
				int len;

				GetDocument()->GetOrigDiff(rr, dd, addr, len);

				if (addr + len < first_virt)
					continue;         // before top of window
				else if (addr >= last_virt)
					break;            // after end of window

				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, col,
						max(addr, first_addr), 
						min(addr+len, last_addr),
						false,  // overwrite (not merge) since mutiple changes at the same address really mess up the colours
						(pDC->IsPrinting() ? print_text_height_ : text_height_)/8);
			}
		}
	}

	// Draw deletion marks always from top (shouldn't be too visible)
	if (!(display_.hide_delete || pDC->IsPrinting() && !theApp.print_change_))
	{
		COLORREF prev_col = pDC->SetTextColor(bg_col_);

		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > *ppr =
			GetDocument()->Deletions();
		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp;
		for (pp = ppr->begin(); pp != ppr->end(); ++pp)
		{
			// Check if its before or after the display area
			if (pp->first < first_virt)
				continue;
			else if (pp->first > last_virt)
				break;

			CRect draw_rect;

			draw_rect.top = int(((pp->first + offset_)/rowsize_) * line_height - 
								doc_rect.top + bdr_top_);
			draw_rect.bottom = draw_rect.top + line_height;
			if (neg_y)
			{
				draw_rect.top = -draw_rect.top;
				draw_rect.bottom = -draw_rect.bottom;
			}

			if (!display_.vert_display && display_.hex_area)
			{
				draw_rect.left = hex_pos(int((pp->first + offset_)%rowsize_), char_width) - 
									char_width - doc_rect.left + bdr_left_;
				draw_rect.right = draw_rect.left + char_width;
				if (neg_x)
				{
					draw_rect.left = -draw_rect.left;
					draw_rect.right = -draw_rect.right;
				}
				pDC->FillSolidRect(&draw_rect, trk_col_);
				char cc = (pp->second > 9 || !display_.delete_count) ? '*' : '0' + char(pp->second);
				pDC->DrawText(&cc, 1, &draw_rect, DT_CENTER | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
			}
			if (display_.vert_display || display_.char_area)
			{
				draw_rect.left = char_pos(int((pp->first + offset_)%rowsize_), char_width, char_width_w) - 
									doc_rect.left + bdr_left_ - 2;
				draw_rect.right = draw_rect.left + char_width_w/5+1;
				if (neg_x)
				{
					draw_rect.left = -draw_rect.left;
					draw_rect.right = -draw_rect.right;
				}
				pDC->FillSolidRect(&draw_rect, trk_col_);
			}
		}
		pDC->SetTextColor(prev_col);   // restore text colour
	}

	// Now draw other change tracking stuff top down OR bottom up depending on ScrollUp()
	if (!pDC->IsPrinting() && ScrollUp())
	{
		// Draw change tracking from bottom up
		if (!display_.hide_replace)
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > *ppr =
				GetDocument()->Replacements();
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::reverse_iterator pp;
			for (pp = ppr->rbegin(); pp != ppr->rend(); ++pp)
				if (pp->first < last_virt)
					break;
			for ( ; pp != ppr->rend(); ++pp)
			{
				if (pp->first + pp->second <= first_virt)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, trk_col_,
						max(pp->first, first_addr), 
						min(pp->first + pp->second, last_addr),
						true,
						(pDC->IsPrinting() ? print_text_height_ : text_height_)/8);
			}
		}
		if (!display_.hide_insert)
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > *ppr =
				GetDocument()->Insertions();
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::reverse_iterator pp;
			for (pp = ppr->rbegin(); pp != ppr->rend(); ++pp)
				if (pp->first < last_virt)
					break;
			for ( ; pp != ppr->rend(); ++pp)
			{
				if (pp->first +pp->second <= first_virt)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, trk_bg_col_,
						max(pp->first, first_addr), 
						min(pp->first + pp->second, last_addr));
			}
		}
	}
	else if (!pDC->IsPrinting() || theApp.print_change_)
	{
		// Draw change tracking from top down
		if (!display_.hide_replace)
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > *ppr =
				GetDocument()->Replacements();
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp;
			for (pp = ppr->begin(); pp != ppr->end(); ++pp)
				if (pp->first + pp->second > first_virt)
					break;
			for ( ; pp != ppr->end(); ++pp)
			{
				if (pp->first > last_virt)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, trk_col_,
						max(pp->first, first_addr), 
						min(pp->first + pp->second, last_addr),
						true,
						(pDC->IsPrinting() ? print_text_height_ : text_height_)/8);
			}
		}
		if (!display_.hide_insert)
		{
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > *ppr =
				GetDocument()->Insertions();
			std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp;
			for (pp = ppr->begin(); pp != ppr->end(); ++pp)
				if (pp->first + pp->second > first_virt)
					break;
			for ( ; pp != ppr->end(); ++pp)
			{
				if (pp->first > last_virt)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, trk_bg_col_,
						max(pp->first, first_addr), 
						min(pp->first + pp->second, last_addr));
			}
		}
	}

	// Don't print bookmarks if hide_bookmarks is on OR printing and print_bookmarks_ is off
	if (!(display_.hide_bookmarks || pDC->IsPrinting() && !theApp.print_bookmarks_))
	{
		// Draw bookmarks
		for (std::vector<FILE_ADDRESS>::const_iterator pbm = pDoc->bm_posn_.begin();
			 pbm != pDoc->bm_posn_.end(); ++pbm)
		{
			if (*pbm >= first_addr && *pbm <= last_addr)
			{
				CRect mark_rect;

				mark_rect.top = int(((*pbm + offset_)/rowsize_) * line_height - 
									doc_rect.top + bdr_top_ + 1);
//                mark_rect.bottom = mark_rect.top + line_height - 3;
				mark_rect.bottom = mark_rect.top + line_height - 1;
				if (neg_y)
				{
					mark_rect.top = -mark_rect.top;
					mark_rect.bottom = -mark_rect.bottom;
				}

				if (!display_.vert_display && display_.hex_area)
				{
					mark_rect.left = hex_pos(int((*pbm + offset_)%rowsize_), char_width) - 
									doc_rect.left + bdr_left_;
//                        mark_rect.right = mark_rect.left + 2*char_width;
					mark_rect.right = mark_rect.left + 2*char_width + 2;
					if (neg_x)
					{
						mark_rect.left = -mark_rect.left;
						mark_rect.right = -mark_rect.right;
					}

					pDC->FillSolidRect(&mark_rect, bm_col_);
				}

				if (display_.vert_display || display_.char_area)
				{
					mark_rect.left = char_pos(int((*pbm + offset_)%rowsize_), char_width, char_width_w) - 
									doc_rect.left  + bdr_left_ + 1;
//                        mark_rect.right = mark_rect.left + char_width_w - 2;
					mark_rect.right = mark_rect.left + char_width_w;
					if (neg_x)
					{
						mark_rect.left = -mark_rect.left;
						mark_rect.right = -mark_rect.right;
					}
					pDC->FillSolidRect(&mark_rect, bm_col_);
				}
			}
		}
	}

	// First work out if we draw the mark - in display and not turned off for printing
	bool draw_mark;
	if (pDC->IsPrinting())
		draw_mark = theApp.print_mark_ && mark_ >= first_virt && mark_ < last_virt;
	else
		draw_mark = mark_ >= first_addr && mark_ <= last_addr;
	if (draw_mark)
	{
		CRect mark_rect;                // Where mark is drawn in logical coords

		mark_rect.top = int(((mark_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
//        mark_rect.bottom = mark_rect.top + line_height - 3;
		mark_rect.bottom = mark_rect.top + line_height - 1;
		if (neg_y)
		{
			mark_rect.top = -mark_rect.top;
			mark_rect.bottom = -mark_rect.bottom;
		}

		if (!display_.vert_display && display_.hex_area)
		{
			mark_rect.left = hex_pos(int((mark_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
//            mark_rect.right = mark_rect.left + 2*char_width;
			mark_rect.right = mark_rect.left + 2*char_width + 2;
			if (neg_x)
			{
				mark_rect.left = -mark_rect.left;
				mark_rect.right = -mark_rect.right;
			}

			pDC->FillSolidRect(&mark_rect, mark_col_);
		}

		if (display_.vert_display || display_.char_area)
		{
			mark_rect.left = char_pos(int((mark_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
//            mark_rect.right = mark_rect.left + char_width_w - 2;
			mark_rect.right = mark_rect.left + char_width_w;
			if (neg_x)
			{
				mark_rect.left = -mark_rect.left;
				mark_rect.right = -mark_rect.right;
			}
			pDC->FillSolidRect(&mark_rect, mark_col_);
		}
	}

	// Draw indicator around byte that is used for info tips
	if (!pDC->IsPrinting() && last_tip_addr_ > -1 && last_tip_addr_ >= first_virt && last_tip_addr_ < last_virt)
	{
		CRect info_rect;                // Where mark is drawn in logical coords

		info_rect.top = int(((last_tip_addr_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
		info_rect.bottom = info_rect.top + line_height - 1;
		if (neg_y)
		{
			info_rect.top = -info_rect.top;
			info_rect.bottom = -info_rect.bottom;
		}

		CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
		CPen * psaved_pen = pDC->SelectObject(&pen1);
		if (!display_.vert_display && display_.hex_area)
		{
			info_rect.left = hex_pos(int((last_tip_addr_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
			info_rect.right = info_rect.left + 2*char_width + 1;
			if (neg_x)
			{
				info_rect.left = -info_rect.left;
				info_rect.right = -info_rect.right;
			}

			//pDC->FillSolidRect(&info_rect, sector_bg_col_);
			pDC->Rectangle(&info_rect);
			//pDC->MoveTo(info_rect.right, info_rect.bottom-1);
			//pDC->LineTo(info_rect.left, info_rect.bottom-1);
			//pDC->LineTo(info_rect.left, info_rect.top);
			//pDC->LineTo(info_rect.right, info_rect.top);
		}

		if (display_.vert_display || display_.char_area)
		{
			info_rect.left = char_pos(int((last_tip_addr_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
			info_rect.right = info_rect.left + char_width_w - 1;
			if (neg_x)
			{
				info_rect.left = -info_rect.left;
				info_rect.right = -info_rect.right;
			}
			//pDC->FillSolidRect(&info_rect, sector_bg_col_);
			pDC->Rectangle(&info_rect);
			//pDC->MoveTo(info_rect.right, info_rect.bottom-1);
			//pDC->LineTo(info_rect.left, info_rect.bottom-1);
			//pDC->LineTo(info_rect.left, info_rect.top);
			//pDC->LineTo(info_rect.right, info_rect.top);
		}
		(void)pDC->SelectObject(psaved_pen);
		(void)pDC->SelectObject(psaved_brush);
	}

	// Draw indicator around byte where bookmark is being moved to
	if (!pDC->IsPrinting() && mouse_down_ && (drag_mark_ || drag_bookmark_ > -1))
	{
		CRect drag_rect;                // Where mark is drawn in logical coords

		drag_rect.top = int(((drag_address_ + offset_)/rowsize_) * line_height - 
							doc_rect.top + bdr_top_ + 1);
		drag_rect.bottom = drag_rect.top + line_height - 2;
		if (neg_y)
		{
			drag_rect.top = -drag_rect.top;
			drag_rect.bottom = -drag_rect.bottom;
		}

		CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
		CPen * psaved_pen = pDC->SelectObject(&pen1);
		if (!display_.vert_display && display_.hex_area)
		{
			drag_rect.left = hex_pos(int((drag_address_ + offset_)%rowsize_), char_width) - 
							 doc_rect.left + bdr_left_;
			drag_rect.right = drag_rect.left + 2*char_width;
			if (neg_x)
			{
				drag_rect.left = -drag_rect.left;
				drag_rect.right = -drag_rect.right;
			}

			pDC->FillSolidRect(&drag_rect, drag_mark_ ? mark_col_ : bm_col_);
			pDC->Rectangle(&drag_rect);
		}

		if (display_.vert_display || display_.char_area)
		{
			drag_rect.left = char_pos(int((drag_address_ + offset_)%rowsize_), char_width, char_width_w) - 
							 doc_rect.left + bdr_left_ + 1;
			drag_rect.right = drag_rect.left + char_width_w - 1;
			if (neg_x)
			{
				drag_rect.left = -drag_rect.left;
				drag_rect.right = -drag_rect.right;
			}
			pDC->FillSolidRect(&drag_rect, drag_mark_ ? mark_col_ : bm_col_);
			pDC->Rectangle(&drag_rect);
		}
		(void)pDC->SelectObject(psaved_pen);
		(void)pDC->SelectObject(psaved_brush);
	}

	// Don't print highlights if hide_highlight is on OR printing and print_highlights_ is off
	if (!(display_.hide_highlight || pDC->IsPrinting() && !theApp.print_highlights_))
	{
		if (!pDC->IsPrinting() && ScrollUp())
		{
			// Draw highlighted areas bottom up
			range_set<FILE_ADDRESS>::range_t::reverse_iterator pr;
			for (pr = hl_set_.range_.rbegin(); pr != hl_set_.range_.rend(); ++pr)
				if (pr->sfirst < last_addr)
					break;
			for ( ; pr != hl_set_.range_.rend(); ++pr)
			{
				if (pr->slast <= first_addr)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, hi_col_,
						max(pr->sfirst, first_addr), 
						min(pr->slast, last_addr));
			}
		}
		else
		{
			// Draw highlighted areas top down
			range_set<FILE_ADDRESS>::range_t::const_iterator pr;
			for (pr = hl_set_.range_.begin(); pr != hl_set_.range_.end(); ++pr)
				if (pr->slast > first_addr)
					break;
			for ( ; pr != hl_set_.range_.end(); ++pr)
			{
				if (pr->sfirst > last_addr)
					break;
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, hi_col_,
						max(pr->sfirst, first_addr), 
						min(pr->slast, last_addr));
			}
		}
	}

	if (pDC->IsPrinting())
	{
		// Only print search occurrences if print_search_ is on
		if (theApp.print_search_)
		{
			// Draw search string occurrences
			// Note this goes through all search occurrences (since search_pair_ is
			// calculated for the current window) which may be slow but then so is printing.
			std::vector<FILE_ADDRESS> sf = GetDocument()->SearchAddresses(first_addr-search_length_, last_addr+search_length_);
			std::vector<FILE_ADDRESS>::const_iterator pp;

			for (pp = sf.begin(); pp != sf.end(); ++pp)
			{
				draw_bg(pDC, doc_rect, neg_x, neg_y,
						line_height, char_width, char_width_w, search_col_,
						max(*pp, first_addr), 
						min(*pp + search_length_, last_addr));
			}
		}
	}
	else if (ScrollUp())
	{
		// Draw search string occurrences bottom up
		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::reverse_iterator pp, pend;
		for (pp = search_pair_.rbegin(), pend = search_pair_.rend(); pp != pend; ++pp)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, search_col_,
					max(pp->first, first_addr), 
					min(pp->second, last_addr));
		}

		// Draw template field backgrounds bottom up
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> >::reverse_iterator pdffd, penddffd;
		for (pdffd = dffd_bg_.rbegin(), penddffd = dffd_bg_.rend(); pdffd != penddffd; ++pdffd)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
				line_height, char_width, char_width_w, pdffd->get<2>(),
				max(pdffd->get<0>(), first_addr), 
				min(pdffd->get<1>(), last_addr));
		}
	}
	else
	{
		// Draw search string occurrences from top down
		std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> >::const_iterator pp, pend;
		for (pp = search_pair_.begin(), pend = search_pair_.end(); pp != pend; ++pp)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
					line_height, char_width, char_width_w, search_col_,
					max(pp->first, first_addr), 
					min(pp->second, last_addr));
		}

		// Draw template field backgrounds from top down
		std::vector<boost::tuple<FILE_ADDRESS, FILE_ADDRESS, COLORREF> >::const_iterator pdffd, penddffd;
		for (pdffd = dffd_bg_.begin(), penddffd = dffd_bg_.end(); pdffd != penddffd; ++pdffd)
		{
			draw_bg(pDC, doc_rect, neg_x, neg_y,
				line_height, char_width, char_width_w, pdffd->get<2>(),
				max(pdffd->get<0>(), first_addr), 
				min(pdffd->get<1>(), last_addr));
		}
	}

end_of_background_drawing:

	unsigned char buf[max_buf];              // Holds bytes for current line being displayed
	size_t last_col = 0;                     // Number of bytes in buf to display
	unsigned char prev_buf[max_buf];         // Copy of last buf - used for merging repeated lines
	size_t prev_last_col;                    // Number of bytes in last buf that were displayed
	int repeat_count = 0;                    // Number of consec. duplicate lines found so far

	// Move declarations outside loop (faster?)
	CString ss(' ', 24);                     // Temp string for formatting
	CRect tt;                                // Temp rect
	CRect addr_rect;                         // Where address is drawn

	// This was added for vert_display
	int vert_offset = 0;
	if (display_.vert_display)
	{
		if (pDC->IsPrinting())
			vert_offset = print_text_height_;
		else
			vert_offset = text_height_;
		if (neg_y)
			vert_offset = - vert_offset;
//      vert_offset = (vert_offset*15)/16;  // Leave a gap between rows
	}

	// This was added for codepages
	std::vector<FILE_ADDRESS> cp_first;
	if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ > 1)
		cp_first = codepage_startchar(first_addr, last_addr);

	// THIS IS WHERE THE ACTUAL LINES ARE DRAWN
	// Note: we use != (line != last_line) since we may be drawing from bottom or top
	FILE_ADDRESS line;
	for (line = first_line; line != last_line;
							line += line_inc, norm_rect += rect_inc)
	{
		// Work out where to display line in logical coords (correct sign)
		tt = norm_rect;
		if (neg_x)
		{
			tt.left = -tt.left;
			tt.right = -tt.right;
		}
		if (neg_y)
		{
			tt.top = -tt.top;
			tt.bottom = -tt.bottom;
		}

		// No display needed if outside display area or past end of doc
		// Note: we don't break when past end since we may be drawing from bottom
		if (!pDC->RectVisible(&tt) || line*rowsize_ - offset_ > pDoc->length())
			continue;

		// Take a copy of the last line output to check for repeated lines
		if (pDC->IsPrinting() && print_sel_ && dup_lines_ && last_col > 0)
			memcpy(prev_buf, buf, last_col);
		prev_last_col = last_col;

		// Get the bytes to display
		size_t ii;                      // Column of first byte
		const int extra_bytes = 8;      // we need to read extra bytes past the end of the line for displaying MBCS characters

		if (line*rowsize_ - offset_ < first_addr)
		{
			last_col = pDoc->GetData(buf + offset_, rowsize_ - offset_ + extra_bytes, line*rowsize_) +
						offset_;
			ii = size_t(first_addr - (line*rowsize_ - offset_));
			ASSERT(int(ii) < rowsize_);
		}
		else
		{
			last_col = pDoc->GetData(buf, rowsize_ + extra_bytes, line*rowsize_ - offset_);
			ii = 0;
		}
		if (last_col > rowsize_) last_col = rowsize_;  // Don't let extra_bytes affect number of columns to display

		if (line*rowsize_ - offset_ + last_col - last_addr >= rowsize_)
			last_col = 0;
		else if (line*rowsize_ - offset_ + last_col > last_addr)
			last_col = size_t(last_addr - (line*rowsize_ - offset_));

//        TRACE("xxx line %ld rowsize_ %ld offset_ %ld last_col %ld first_addr %ld last_addr %ld\n",
//              long(line), long(rowsize_), long(offset_), long(last_col), long(first_addr), long(last_addr));

		if (pDC->IsPrinting() && print_sel_ && dup_lines_)
		{
			// Check if the line is the same as the last one
			// But NOT if very first line (line 0 on page 0) since we may not display the whole line
			if ((curpage_ > 0 || line > first_line+1) &&
				last_col == prev_last_col && memcmp(buf, prev_buf, last_col) == 0)
			{
				++repeat_count;
				norm_rect -= rect_inc;
				continue;
			}

			if (repeat_count > 0)
			{
				// Display how many times the line was repeated
				CString mess;
				if (repeat_count == 1)
					mess = "Repeated once";
				else
					mess.Format("Repeated %d more times", repeat_count);
				CRect mess_rect = tt;
				mess_rect.left += hex_pos(0, char_width);
				pDC->DrawText(mess, &mess_rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);

				// Move the current display line down one
				norm_rect += rect_inc;
				tt = norm_rect;
				if (neg_x)
				{
					tt.left = -tt.left;
					tt.right = -tt.right;
				}
				if (neg_y)
				{
					tt.top = -tt.top;
					tt.bottom = -tt.bottom;
				}

				// Reset the repeated line counter since this was a different line
				repeat_count = 0;
			}

			// See if we are at the bottom of the page yet
			if (norm_rect.bottom > margin_size_.cy + print_text_height_*lines_per_page_)
			{
				print_next_line_ = line;
				break;
			}
		}

		// Draw address if ...
		if ((addr_width_ - 1)*char_width + tt.left > 0 &&    // not off to the left
			(tt.top + text_height_/4 >= bdr_top_ || pDC->IsPrinting()))   // and does not encroach into ruler
		{
			addr_rect = tt;            // tt with right margin where addresses end
			addr_rect.right = addr_rect.left + addr_width_*char_width - char_width - 1;
			if (pDC->IsPrinting())
				if (neg_y)
					addr_rect.bottom = addr_rect.top - print_text_height_;
				else
					addr_rect.bottom = addr_rect.top + print_text_height_;
			else
				if (neg_y)
					addr_rect.bottom = addr_rect.top - text_height_;
				else
					addr_rect.bottom = addr_rect.top + text_height_;
			// Not highlighting when the mouse is down avoids a problem with invalidation of
			// the address area when autoscrolling (old highlights sometimes left behind).
			if (theApp.hl_caret_ && !pDC->IsPrinting() && !mouse_down_ && start_addr >= line*rowsize_ - offset_ && start_addr < (line+1)*rowsize_ - offset_)
			{
				CBrush * psaved_brush = pDC->SelectObject(&brush);
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				pDC->Rectangle(&addr_rect);
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			// Show address of current row with a different background colour
			if (theApp.hl_mouse_ && !pDC->IsPrinting() && mouse_addr_ >= line*rowsize_ - offset_ && mouse_addr_ < (line+1)*rowsize_ - offset_)
			{
				CBrush * psaved_brush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
				CPen * psaved_pen = pDC->SelectObject(&pen1);
				pDC->Rectangle(&addr_rect);
				(void)pDC->SelectObject(psaved_pen);
				(void)pDC->SelectObject(psaved_brush);
			}

			if (hex_width_ > 0)
			{
				int ww = hex_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
					theApp.hex_ucase_ ? "%0*I64X:" : "%0*I64x:",
						hex_width_,
						(line*rowsize_ - offset_ > first_addr ? line*rowsize_ - offset_ : first_addr) + display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddSpaces(ss);
					ww += (hex_width_-1)/4;
				}
				pDC->SetTextColor(GetHexAddrCol());   // Colour of hex addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}

			if (dec_width_ > 0)
			{
				int ww = dec_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
						"%*I64d:",
						dec_width_,
						(line*rowsize_ - offset_ > first_addr ? line*rowsize_ - offset_ : first_addr) + display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddCommas(ss);
					ww += (dec_width_-1)/3;
				}
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}

			if (num_width_ > 0)
			{
				int ww = num_width_ + 1;
				char *addr_buf = ss.GetBuffer(24);             // reserve space for 64 bit address
				sprintf(addr_buf,
						"%*I64d:",
						num_width_,
						line + display_.addrbase1);
				ss.ReleaseBuffer(-1);
				if (theApp.nice_addr_)
				{
					AddCommas(ss);
					ww += (num_width_-1)/3;
				}
				pDC->SetTextColor(GetDecAddrCol());   // Colour of dec addresses
				addr_rect.right = addr_rect.left + ww*char_width;
				pDC->DrawText(ss, &addr_rect, DT_TOP | DT_RIGHT | DT_NOPREFIX | DT_SINGLELINE);
				addr_rect.left = addr_rect.right;
			}
		}

		wchar_t dot = 0x00B7;    // Unicode char for middle dot
		int dot_pad = 0, cont_pad = 0, bad_pad = 0;
		if (text_width_w_ > text_width_)
		{
			CSize size;
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &dot, 1, &size);
			dot_pad = (char_width_w - size.cx)/2;

			wchar_t wc = *ContChar();
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
			cont_pad = (char_width_w - size.cx)/2;

			wc = *BadChar();
			::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
			bad_pad = (char_width_w - size.cx)/2;
		}
		int idx = -1;                      // Index into cp_first of the line we are doing
		int start_col = -1;                // Column of the next start char (not continuation char) in MBCS

		if (cp_first.size() > 0)
		{
			idx = int(line - doc_rect.top/line_height_);
			ASSERT(cp_first[idx] >= line*rowsize_ - offset_ && cp_first[idx] < (line+1)*rowsize_ - offset_ + 8 ||
			       idx == cp_first.size() - 1 && cp_first[idx] == -1);
			start_col = int(cp_first[idx] - line*rowsize_ - offset_);
		}

		// Keep track of the current colour so we only set it when it changes
		COLORREF current_colour = kala[buf[ii]];
		pDC->SetTextColor(current_colour);
		if (display_.vert_display || display_.hex_area)
		{
			int posx = tt.left + hex_pos(0, char_width);                 // Horiz pos of 1st hex column

			// Display each byte as hex (and char if nec.)
			for (size_t jj = ii ; jj < last_col; ++jj)
			{
				if (display_.vert_display)
				{
					if (posx + int(jj + 1 + jj/group_by_)*char_width_w < 0)
						continue;
					else if (posx + int(jj + jj/group_by_)*char_width_w >= tt.right)
						break;
				}
				else
				{
					if (posx + int((jj+1)*3 + jj/group_by_)*char_width < 0)
						continue;
					else if (posx + int(jj*3 + jj/group_by_)*char_width >= tt.right)
						break;
				}

				if (current_colour != kala[buf[jj]])
				{
					current_colour = kala[buf[jj]];
					pDC->SetTextColor(current_colour);
				}

				if (display_.vert_display)
				{
					// Display byte in char display area (as ASCII, EBCDIC etc)
					if (display_.char_set == CHARSET_CODEPAGE &&
						cp_first.size() > 0 &&
						jj < start_col)
					{
						// Continuation byte of MBCS character
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + cont_pad, tt.top, ContChar(), 1);
					}
					else if (buf[jj] < 32 && display_.char_set != CHARSET_EBCDIC && display_.char_set != CHARSET_OEM)
					{
						// Control characters are diplayed the same for all char sets except for 
						//  - EBCDIC (some chars < 32 may not be graphical) and 
						//  - OEM (there are graphic characters for chars < 32)
						if (display_.control == 0)
						{
							// Display control char and other chars as a dot (normally in red)
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}
						else if (display_.control == 1)
						{
							// Display control chars as red uppercase equiv.
							char cc = buf[jj] + 0x40;
							pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, &cc, 1);
						}
						else if (display_.control == 2)
						{
							// Display control chars as C escape code (in red)
							const char *check = "\a\b\f\n\r\t\v\0";
							const char *display = "abfnrtv0";
							const char *pp;
							if ((pp = strchr(check, buf[jj])) != NULL)
								pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, display + (pp-check), 1);
							else
								::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}

						// For MBCS char sets we need to keep track that we have done this character
						if (display_.char_set == CHARSET_CODEPAGE && cp_first.size() > 0)
							++start_col;
					}
					else if (display_.char_set == CHARSET_CODEPAGE && code_page_ == CP_UTF8)
					{
						// Handle UTF 8 specially since it is not like other Windows code pages
						ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 0);
						if ((buf[jj] & 0xC0) == 0x80)
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + cont_pad, tt.top, ContChar(), 1);
						else
						{
							// The top 2 bits determine how many following (continuation) bytes there are.  If top bit
							// is zero then len == 1 (no cont bytes) since it is ASCII
							size_t len;
							if ((buf[jj] & 0x80) == 0x00)
								len = 1;
							else if ((buf[jj] & 0xE0) == 0xC0)
								len = 2;
							else if ((buf[jj] & 0xF0) == 0xE0)
								len = 3;
							else if ((buf[jj] & 0xF8) == 0xF0)
								len = 4;
							else if ((buf[jj] & 0xFC) == 0xF8)
								len = 5;
							else if ((buf[jj] & 0xFE) == 0xFC)
								len = 6;
							else
								len = 0;   // invalid UTF-8

							wchar_t wc;                               // equivalent Unicode (UTF-16) character
							if (len == 0 || 
								MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], len, &wc, 1) == 0 ||
								wc >= 0xE000 && wc < 0xF900)
							{
								// Character translation returned no bytes or Unicode value in "Private Use Area"
								wc = *BadChar();
							}

							CSize size;
							::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
						}
					}
					else if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ <= 1)
					{
						// Handle single byte code page
						ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 1);
						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], 1, &wc, 1) != 1 ||
							wc >= 0xE000 && wc < 0xF900)
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + bad_pad, tt.top, BadChar(), 1);
							wc = *BadChar();
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
					else if (display_.char_set == CHARSET_CODEPAGE)
					{
						// Handle MBCS code pages
						ASSERT(cp_first.size() > 0 && jj == start_col);
						size_t len = CharNextExA(code_page_, (const char *)&buf[jj], 0) - (char *)&buf[jj];
						// Display the multibyte character that starts at this byte then find the start of the next one
						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (MultiByteToWideChar(code_page_, 0, (char *)&buf[jj], len, &wc, 1) != 1 ||
							wc >= 0xE000 && wc < 0xF900)
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							wc = *BadChar();
							++start_col;
						}
						else
						{
							// Character translation was good so ouput the character
							start_col += len;
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
					else if (display_.char_set == CHARSET_EBCDIC)
					{
						// Display EBCDIC (or red dot if not valid EBCDIC char)
						if (e2a_tab[buf[jj]] == '\0')
						{
							::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						}
						else
						{
							pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, (char *)&e2a_tab[buf[jj]], 1);
						}
					}
					else if (display_.char_set == CHARSET_ASCII && buf[jj] < 127 ||
							 display_.char_set != CHARSET_ASCII && buf[jj] >= first_char_ && buf[jj] <= last_char_)
					{
						// Display normal char or graphic char if in font
						pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top, (char *)&buf[jj], 1);
					}
					else
					{
						// Display chars "out of range" as dots
						::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + dot_pad, tt.top, &dot, 1);
						//::TextOutW(pDC->GetSafeHdc(), posx + (jj + jj/group_by_)*char_width_w + bad_pad, tt.top, BadChar(), 1);
  					}

					// Display the hex digits below that, one below the other
					pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top + vert_offset,   &hex[(buf[jj]>>4)&0xF], 1);
					pDC->TextOut(posx + (jj + jj/group_by_)*char_width_w, tt.top + vert_offset*2, &hex[buf[jj]&0xF], 1);
				}
				else
				{
					char hh[2];

					// Create hex digits and display them
					hh[0] = hex[(buf[jj]>>4)&0xF];
					hh[1] = hex[buf[jj]&0xF];

					// This actually displays the bytes (in hex)!
					// Note: removed calcs that were previously encapsulated in hex_pos
					pDC->TextOut(posx + (jj*3 + jj/group_by_)*char_width, tt.top, hh, 2);
				}
			}
		}

		if (!display_.vert_display && display_.char_area)
		{
			int posc = tt.left + char_pos(0, char_width, char_width_w);  // Horiz pos of 1st char column

			for (size_t kk = ii ; kk < last_col; ++kk)
			{
				if (posc + int(kk+1)*char_width_w < 0)
					continue;
				else if (posc + int(kk)*char_width_w >= tt.right)
					break;

				if (current_colour != kala[buf[kk]])
				{
					current_colour = kala[buf[kk]];
					pDC->SetTextColor(current_colour);
				}

				// Display byte in the char (right) area (as ASCII, EBCDIC etc)
				if (display_.char_set == CHARSET_CODEPAGE &&
					cp_first.size() > 0 &&
					kk < start_col)
				{
					// Continuation byte of MBCS character
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + cont_pad, tt.top, ContChar(), 1);
				}
				else if (buf[kk] < 32 && display_.char_set != CHARSET_EBCDIC && display_.char_set != CHARSET_OEM)
				{
					// Control characters are diplayed the same for all char sets except for
					//  - EBCDIC (some chars < 32 may not be graphical) and 
					//  - OEM (there are graphic characters for chars < 32)
					if (display_.control == 0)
					{
						// Display control char and other chars as a dot (normally in red)
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}
					else if (display_.control == 1)
					{
						// Display control chars as red uppercase equiv.
						char cc = buf[kk] + 0x40;
						pDC->TextOut(posc + kk*char_width_w, tt.top, &cc, 1);
					}
					else if (display_.control == 2)
					{
						// Display control chars as C escape code (in red)
						const char *check = "\a\b\f\n\r\t\v\0";
						const char *display = "abfnrtv0";
						const char *pp;
						if ((pp = strchr(check, buf[kk])) != NULL)
							pDC->TextOut(posc + kk*char_width_w, tt.top, display + (pp-check), 1);
						else
							::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}

					// For MBCS char sets we need to keep track that we have done this character
					if (display_.char_set == CHARSET_CODEPAGE && cp_first.size() > 0)
						++start_col;
				}
				else if (display_.char_set == CHARSET_CODEPAGE && code_page_ == CP_UTF8)
				{
					// Handle UTF 8 specially since it is not like other Windows code pages
					ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 0);
					if ((buf[kk] & 0xC0) == 0x80)
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + cont_pad, tt.top, ContChar(), 1);
					else
					{
						// The top 2 bits determine how many following (continuation) bytes there are.  If top bit
						// is zero then len == 1 (no cont bytes) since it is ASCII
						size_t len;
						if ((buf[kk] & 0x80) == 0x00)
							len = 1;
						else if ((buf[kk] & 0xE0) == 0xC0)
							len = 2;
						else if ((buf[kk] & 0xF0) == 0xE0)
							len = 3;
						else if ((buf[kk] & 0xF8) == 0xF0)
							len = 4;
						else if ((buf[kk] & 0xFC) == 0xF8)
							len = 5;
						else if ((buf[kk] & 0xFE) == 0xFC)
							len = 6;
						else
							len = 0;   // invalid UTF-8

						// MultiByteToWideChar (at least in XP) does not handle some invalid sequences properly,
						// eg C0 67 return "g" (the C0 appears to be ignored) - so we need to check ourselves
						bool isBad = false;
						for (int nn = 1; nn < len; ++nn)
							if ((buf[kk+nn] &0x80) != 0x80)
							{
								isBad = true;
								break;
							}

						wchar_t wc;                               // equivalent Unicode (UTF-16) character
						if (len == 0 ||                                                                // 1111111X is not a valid byte
							isBad ||                                                                   // not enough cont bytes
							MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], len, &wc, 1) == 0 ||  // could not convert for some other reason
							wc >= 0xE000 && wc < 0xF900)                                               // invalid byte sequence
						{
							// Character translation returned no bytes or Unicode value in "Private Use Area"
							wc = *BadChar();
						}

						CSize size;
						::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
					}
				}
				else if (display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ <= 1)
				{
					// Handle single byte code page
					ASSERT(cp_first.size() == 0 && max_cp_bytes_ == 1);
					wchar_t wc;                               // equivalent Unicode (UTF-16) character
					if (MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], 1, &wc, 1) != 1 ||
						wc >= 0xE000 && wc < 0xF900)
					{
						// Character translation returned no bytes or Unicode value in "Private Use Area"
						wc = *BadChar();
					}

					CSize size;
					::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
				}
				else if (display_.char_set == CHARSET_CODEPAGE)
				{
					// Handle MBCS code pages
					ASSERT(cp_first.size() > 0 && kk == start_col);
					size_t len = CharNextExA(code_page_, (const char *)&buf[kk], 0) - (char *)&buf[kk];
					// Display the multibyte character that starts at this byte then find the start of the next one
					wchar_t wc;                               // equivalent Unicode (UTF-16) character
					if (MultiByteToWideChar(code_page_, 0, (char *)&buf[kk], len, &wc, 1) != 1 ||
						wc >= 0xE000 && wc < 0xF900)
					{
						// Character translation returned no bytes or Unicode value in "Private Use Area"
						wc = *BadChar();
						++start_col;
					}
					else
					{
						// Character translation was good so ouput the character
						start_col += len;
					}

					CSize size;
					::GetTextExtentPoint32W(pDC->GetSafeHdc(), &wc, 1, &size);
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + (char_width_w-size.cx)/2, tt.top, &wc, 1);
				}
				else if (display_.char_set == CHARSET_EBCDIC)
				{
					// Display EBCDIC (or red dot if not valid EBCDIC char)
					if (e2a_tab[buf[kk]] == '\0')
					{
						::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + dot_pad, tt.top, &dot, 1);
					}
					else
					{
						pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&e2a_tab[buf[kk]], 1);
					}
				}
				else if (display_.char_set == CHARSET_ASCII && buf[kk] < 127 ||
					     display_.char_set != CHARSET_ASCII && buf[kk] >= first_char_ && buf[kk] <= last_char_)
				{
					// Display normal char or graphic char if in font
					pDC->TextOut(posc + kk*char_width_w, tt.top, (char *)&buf[kk], 1);
				}
				else
				{
					// Display chars "out of range" as dots
					::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + bad_pad, tt.top, &dot, 1);
					//::TextOutW(pDC->GetSafeHdc(), posc + kk*char_width_w + bad_pad, tt.top, BadChar(), 1);
  				}
			}
		}

		// If any part of the line is within the current selection
		if (!pDC->IsPrinting() && start_addr < end_addr &&
			end_addr > line*rowsize_ - offset_ && start_addr < (line+1)*rowsize_ - offset_)
		{
			FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
			FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);
//            ASSERT(end > start);

			ASSERT(display_.hex_area || display_.char_area);
			if (!display_.vert_display && display_.hex_area)
			{
				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_) - 1)) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				if (has_focus && !display_.edit_char || num_colours <= 256)
					pDC->InvertRect(&rev);  // Full contrast reverse video only if in editing in hex area
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}

			if (display_.vert_display || display_.char_area)
			{
				// Draw char selection in inverse
				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_) - 1)) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				if (num_colours <= 256 || has_focus && (display_.vert_display || display_.edit_char))
					pDC->InvertRect(&rev);
				else
					pDC->PatBlt(rev.left, rev.top,
								rev.right-rev.left, rev.bottom-rev.top,
								PATINVERT);
			}
		}
		else if (theApp.show_other_ && has_focus &&
				 !display_.vert_display &&
				 display_.char_area && display_.hex_area &&  // we can only display in the other area if both exist
				 !pDC->IsPrinting() &&
				 start_addr == end_addr && 
				 start_addr >= line*rowsize_ - offset_ && 
				 start_addr < (line+1)*rowsize_ - offset_)
		{
			// Draw "shadow" cursor in the other area
			FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
			FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);

			CRect rev(norm_rect);
			if (display_.edit_char)
			{
				ASSERT(display_.char_area);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_))) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
			}
			else
			{
				ASSERT(display_.hex_area);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_))) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
			}
			if (neg_x)
			{
				rev.left = -rev.left;
				rev.right = -rev.right;
			}
			if (neg_y)
			{
				rev.top = -rev.top;
				rev.bottom = -rev.bottom;
			}
			pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
		}
		else if (!has_focus &&
				 !pDC->IsPrinting() &&
				 start_addr == end_addr &&
				 start_addr >= line*rowsize_ - offset_ && 
				 start_addr < (line+1)*rowsize_ - offset_)
		{
			// Draw "shadow" for current byte when lost focus
			if (!display_.vert_display && display_.hex_area)
			{
				// Get rect for hex area
				FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
				FILE_ADDRESS end = min(end_addr, (line+1)*rowsize_ - offset_);

				CRect rev(norm_rect);
				rev.right = rev.left + hex_pos(int(end - (line*rowsize_ - offset_))) + 2*text_width_;
				rev.left += hex_pos(int(start - (line*rowsize_ - offset_)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
			}
			if (display_.vert_display || display_.char_area)
			{
				// Get rect for char area or stacked mode
				FILE_ADDRESS start = max(start_addr, line*rowsize_ - offset_);
				FILE_ADDRESS   end = min(end_addr, (line+1)*rowsize_ - offset_);

				CRect rev(norm_rect);
				rev.right = rev.left + char_pos(int(end - (line*rowsize_ - offset_))) + text_width_w_;
				rev.left += char_pos(int(start - (line*rowsize_ - offset_)));
				if (neg_x)
				{
					rev.left = -rev.left;
					rev.right = -rev.right;
				}
				if (neg_y)
				{
					rev.top = -rev.top;
					rev.bottom = -rev.bottom;
				}
				pDC->PatBlt(rev.left, rev.top, rev.right-rev.left, rev.bottom-rev.top, PATINVERT);
			}
		}
	} // for each display (text) line

	if (pDC->IsPrinting() && print_sel_ && dup_lines_)
	{
		if (line >= last_line)
			curpage_ = -1;  // signal that there is no more to print (so we can set m_bContinuePrinting to FALSE later)

		// Display any residual repeated lines count
		if (repeat_count > 0)
		{
			// Display how many times the line was repeated
			CString mess;
			if (repeat_count == 1)
				mess = "Repeated once";
			else
				mess.Format("Repeated %d more times", repeat_count);
			CRect mess_rect = norm_rect;
			if (neg_x)
			{
				mess_rect.left = -mess_rect.left;
				mess_rect.right = -mess_rect.right;
			}
			if (neg_y)
			{
				mess_rect.top = -mess_rect.top;
				mess_rect.bottom = -mess_rect.bottom;
			}
			mess_rect.left += hex_pos(0, char_width);
			pDC->DrawText(mess, &mess_rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
		}
	}

	if (!pDC->IsPrinting())
	{
		ShowCaret();
	}
//    move_dlgs();
}

#ifdef RULER_ADJUST
// Draws adjuster handles in the ruler
void CHexEditView::draw_adjusters(CDC* pDC)
{
	int xpos;

	// Set up pen and brush colours (all adjusters are the same colour)
	CPen pen(PS_SOLID, 0, RGB(0,0,0));          // black pen
	CPen pdash(PS_DOT, 0, RGB(0,0,0));          // for dashed black line
	CBrush bwhite(RGB(255,255,255));            // white brush
	CBrush bred(RGB(192,0,0));                  // red brush
	CPen * psp = pDC->SelectObject(&pen);       // ptr to saved pen
	CBrush * psb = pDC->SelectObject(&bwhite);  // ptr to saved brush

	// Show rowsize_ in the ruler
	ASSERT(rowsize_ > 3);
	if (!display_.vert_display && display_.hex_area)
	{
		if (adjusting_rowsize_ == -1 || adjusting_rowsize_ == rowsize_)
			xpos = char_pos(0) - text_width_w_/2 - scrollpos_.x;
		else
			xpos = hex_pos(adjusting_rowsize_) - scrollpos_.x;
		if (display_.autofit)
			pDC->SelectObject(&bred);
		draw_rowsize(pDC, xpos-1);
		if (display_.autofit)
			pDC->SelectObject(&bwhite);
		if (adjusting_rowsize_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}
	else if (display_.vert_display || display_.char_area)
	{
		if (adjusting_rowsize_ == -1 || adjusting_rowsize_ == rowsize_)
			xpos = char_pos(rowsize_ - 1) + (3*text_width_w_)/2 - scrollpos_.x;
		else
			xpos = char_pos(adjusting_rowsize_) - scrollpos_.x;
		if (display_.autofit)
			pDC->SelectObject(&bred);
		draw_rowsize(pDC, xpos-1);
		if (display_.autofit)
			pDC->SelectObject(&bwhite);
		if (adjusting_rowsize_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}

	// Show position of "offset_" in the ruler (in hex and/or char areas)
	ASSERT(offset_ < rowsize_);
	if (!display_.vert_display && display_.hex_area)
	{
		if (adjusting_offset_ > -1)
			xpos = hex_pos(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1) - 
			       scrollpos_.x;
		else
			xpos = hex_pos(offset_) - scrollpos_.x;
		draw_offset(pDC, xpos-1);
		if (adjusting_offset_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}
	if (display_.vert_display || display_.char_area)
	{
		if (adjusting_offset_ > -1)
			xpos = char_pos(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1) -
			       scrollpos_.x;
		else
			xpos = char_pos(offset_) - scrollpos_.x;
		if (display_.vert_display || !display_.hex_area)
			draw_offset(pDC, xpos-1);
		if (adjusting_offset_ > -1)
		{
			(void)pDC->SelectObject(pdash);
			pDC->MoveTo(xpos-1, bdr_top_);
			pDC->LineTo(xpos-1, 30000);
			(void)pDC->SelectObject(pen);
		}
	}

	// Show current grouping if not adjusting (adjusting_group_by_ == -1) OR
	// current adjust column if not dragged past the edge (adjusting_group_by_ < 9999)
	if (adjusting_group_by_ == -1 && group_by_ < rowsize_ ||
		adjusting_group_by_ > -1 && adjusting_group_by_ < rowsize_)
	{
		if (display_.vert_display)
		{
			if (adjusting_group_by_ > -1)
				xpos = char_pos(adjusting_group_by_) - scrollpos_.x;
			else
				xpos = char_pos(group_by_) - scrollpos_.x;
			if (display_.vert_display && 
				(adjusting_group_by_ == -1 || adjusting_group_by_%group_by_ == 0))
				xpos -= text_width_w_/2;
			draw_group_by(pDC, xpos);
			if (adjusting_group_by_ > -1)
			{
				// Draw vertical dashed line
				(void)pDC->SelectObject(pdash);
				pDC->MoveTo(xpos, bdr_top_);
				pDC->LineTo(xpos, 30000);
				(void)pDC->SelectObject(pen);
			}
		}
		else if (display_.hex_area)
		{
			if (adjusting_group_by_ > -1)
				xpos = hex_pos(adjusting_group_by_) - 2 - scrollpos_.x;
			else
				xpos = hex_pos(group_by_) - 2 - scrollpos_.x;
			if (adjusting_group_by_ == -1 || adjusting_group_by_%group_by_ == 0)
				xpos -= (text_width_ - 1);
			draw_group_by(pDC, xpos);
			if (adjusting_group_by_ > -1)
			{
				(void)pDC->SelectObject(pdash);
				pDC->MoveTo(xpos, bdr_top_);
				pDC->LineTo(xpos, 30000);
				(void)pDC->SelectObject(pen);
			}
		}
	}

	(void)pDC->SelectObject(psp);
	(void)pDC->SelectObject(psb);
}

// Draws row size adjustment handle in the ruler
void CHexEditView::draw_rowsize(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos + 2, bdr_top_ - 7);
	pDC->LineTo(xpos,     bdr_top_ - 7);
	pDC->LineTo(xpos - 3, bdr_top_ - 4);
	pDC->LineTo(xpos,     bdr_top_ - 1);
	pDC->LineTo(xpos + 2, bdr_top_ - 1);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}

// Draws offset handle in the ruler
void CHexEditView::draw_offset(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos - 1, bdr_top_ - 6);
	pDC->LineTo(xpos - 1, bdr_top_ - 2);
	pDC->LineTo(xpos,     bdr_top_ - 1);
	pDC->LineTo(xpos + 3, bdr_top_ - 4);
	pDC->LineTo(xpos,     bdr_top_ - 7);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}

// Draws group by handle in the ruler
void CHexEditView::draw_group_by(CDC* pDC, int xpos)
{
	pDC->BeginPath();
	pDC->MoveTo(xpos - 3, bdr_top_ - 7);
	pDC->LineTo(xpos - 3, bdr_top_ - 5);
	pDC->LineTo(xpos,     bdr_top_ - 2);
	pDC->LineTo(xpos + 3, bdr_top_ - 5);
	pDC->LineTo(xpos + 3, bdr_top_ - 7);
	pDC->EndPath();
	pDC->StrokeAndFillPath();
}
#endif

void CHexEditView::draw_bg(CDC* pDC, const CRectAp &doc_rect, bool neg_x, bool neg_y,
						   int line_height, int char_width, int char_width_w,
						   COLORREF clr, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr,
						   bool merge /*=true*/, int draw_height /*=-1*/)
{
	if (end_addr < start_addr) return;

	if (draw_height > -1 && draw_height < 2) draw_height = 2;  // make it at least 2 pixels (1 does not draw properly)

	int saved_rop = pDC->SetROP2(R2_NOTXORPEN);
	CPen pen(PS_SOLID, 0, clr);
	CPen * psaved_pen = pDC->SelectObject(&pen);
	CBrush brush(clr);
	CBrush * psaved_brush = pDC->SelectObject(&brush);

	FILE_ADDRESS start_line = (start_addr + offset_)/rowsize_;
	FILE_ADDRESS end_line = (end_addr + offset_)/rowsize_;
	int start_in_row = int((start_addr+offset_)%rowsize_);
	int end_in_row = int((end_addr+offset_)%rowsize_);

	CRect rct;

	if (start_line == end_line)
	{
		// Draw the block (all on one line)
		rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
		rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
		if (neg_y)
		{
			rct.top = -rct.top;
			rct.bottom = -rct.bottom;
		}

		if (!display_.vert_display && display_.hex_area)
		{
			rct.left = hex_pos(start_in_row, char_width) - 
					   doc_rect.left + bdr_left_;
			rct.right = hex_pos(end_in_row - 1, char_width) +
						2*char_width - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}

			if (neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right)
			{
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
			}
		}

		if (display_.vert_display || display_.char_area)
		{
			// rct.top = start_line * line_height;
			// rct.bottom = rct.top + line_height;
			rct.left = char_pos(start_in_row, char_width, char_width_w) -
					   doc_rect.left + bdr_left_;
			rct.right = char_pos(end_in_row - 1, char_width, char_width_w) +
						char_width_w - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}

		pDC->SetROP2(saved_rop);
		pDC->SelectObject(psaved_pen);
		pDC->SelectObject(psaved_brush);
		return;  // All on one line so that's it
	}

	// Block extends over (at least) 2 lines so draw the partial lines at each end
	rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
	rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
	if (neg_y)
	{
		rct.top = -rct.top;
		rct.bottom = -rct.bottom;
	}
	if (!display_.vert_display && display_.hex_area)
	{
		rct.left = hex_pos(start_in_row, char_width) -
				   doc_rect.left + bdr_left_;
		rct.right = hex_pos(rowsize_ - 1, char_width) +
					2*char_width - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
	}

	if (display_.vert_display || display_.char_area)
	{
		rct.left = char_pos(start_in_row, char_width, char_width_w) -
				   doc_rect.left + bdr_left_;
		rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
					char_width_w - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
	}

	// Last (partial) line
	rct.bottom = int((end_line+1) * line_height - doc_rect.top + bdr_top_);
	rct.top = rct.bottom - (draw_height > 0 ? draw_height : line_height);
	if (neg_y)
	{
		rct.top = -rct.top;
		rct.bottom = -rct.bottom;
	}
	if (!display_.vert_display && display_.hex_area)
	{
		rct.left = hex_pos(0, char_width) -
				   doc_rect.left + bdr_left_;
		rct.right = hex_pos(end_in_row - 1, char_width) +
					2*char_width - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		if (neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right)
		{
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}
	}

	if (display_.vert_display || display_.char_area)
	{
		rct.left = char_pos(0, char_width, char_width_w) -
				   doc_rect.left + bdr_left_;
		rct.right = char_pos(end_in_row - 1, char_width, char_width_w) +
					char_width_w - doc_rect.left + bdr_left_;
		if (neg_x)
		{
			rct.left = -rct.left;
			rct.right = -rct.right;
		}
		if (merge)
			pDC->Rectangle(&rct);
		else
			pDC->FillSolidRect(&rct, clr);
	}

	// Now draw all the full lines
	if (draw_height > 0)
	{
		// Since we ar not doing a complete fill of the lines (eg underline)
		// we have to do each line of text individually
		for (++start_line; start_line < end_line; ++start_line)
		{
			rct.bottom = int((start_line+1) * line_height - doc_rect.top + bdr_top_);
			rct.top = rct.bottom - draw_height;
			if (neg_y)
			{
				rct.top = -rct.top;
				rct.bottom = -rct.bottom;
			}
			if (!display_.vert_display && display_.hex_area)
			{
				rct.left = hex_pos(0, char_width) -
						   doc_rect.left + bdr_left_;
				rct.right = hex_pos(rowsize_ - 1, char_width) +
							2*char_width - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
				ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
			}

			if (display_.vert_display || display_.char_area)
			{
				rct.left = char_pos(0, char_width, char_width_w) -
						   doc_rect.left + bdr_left_;
				rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
							char_width_w - doc_rect.left + bdr_left_;
				if (neg_x)
				{
					rct.left = -rct.left;
					rct.right = -rct.right;
				}
				if (merge)
					pDC->Rectangle(&rct);
				else
					pDC->FillSolidRect(&rct, clr);
			}
		}
	}
	else if (start_line + 1 < end_line)
	{
		// Draw the complete lines as one block
		rct.top = int((start_line + 1) * line_height - doc_rect.top + bdr_top_);
		rct.bottom = int(end_line * line_height - doc_rect.top + bdr_top_);
		if (neg_y)
		{
			rct.top = -rct.top;
			rct.bottom = -rct.bottom;
		}

		if (!display_.vert_display && display_.hex_area)
		{
			rct.left = hex_pos(0, char_width) -
					   doc_rect.left + bdr_left_;
			rct.right = hex_pos(rowsize_ - 1, char_width) +
						2*char_width - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			ASSERT(neg_x && rct.left > rct.right || !neg_x && rct.left < rct.right);
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}

		if (display_.vert_display || display_.char_area)
		{
			rct.left = char_pos(0, char_width, char_width_w) -
					   doc_rect.left + bdr_left_;
			rct.right = char_pos(rowsize_ - 1, char_width, char_width_w) +
						char_width_w - doc_rect.left + bdr_left_;
			if (neg_x)
			{
				rct.left = -rct.left;
				rct.right = -rct.right;
			}
			if (merge)
				pDC->Rectangle(&rct);
			else
				pDC->FillSolidRect(&rct, clr);
		}
	}

	pDC->SetROP2(saved_rop);
	pDC->SelectObject(psaved_pen);
	pDC->SelectObject(psaved_brush);
	return;
}

// recalc_display() - recalculates everything to do with the display
// (and redraws it) if anything about how the window is drawn changes.
// This includes font changed, window resized, document changed, display
// options changed (address display, char display, autofit turned on etc).
void CHexEditView::recalc_display()
{
	// Stop re-entry (can cause inf. recursion)
	if (in_recalc_display) return;
	in_recalc_display = true;

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (GetScrollPastEnds() != theApp.scroll_past_ends_)
	{
		SetScrollPastEnds(theApp.scroll_past_ends_);
		SetScroll(GetScroll());
	}
	SetAutoscroll(theApp.autoscroll_accel_);

	// Save info on the current font
	{
		TEXTMETRIC tm;
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.GetTextMetrics(&tm);
		text_height_ = tm.tmHeight + tm.tmExternalLeading; // Height of font

		first_char_ = tm.tmFirstChar;                   // 1st valid char of font

		last_char_ = tm.tmLastChar;                     // last valid char of font
// This causes problems when in IBM/OEM mode since these fonts have characters right down to zero
//        if (first_char_ < 32) first_char_ = 32;         // Some fonts return 30 but 30,31 are nothing

// The max char width returned by many fonts is much too big (seems to be bigger than any character in font!?)
//      text_width_ = tm.tmMaxCharWidth;                // width of widest char in font
		CSize size;
		::GetTextExtentPoint32(dc.m_hDC, "D", 1, &size);
		text_width_ = size.cx;                  // width of "D"
		::GetTextExtentPoint32(dc.m_hDC, "W", 1, &size);
		text_width_w_ = size.cx;                  // width of "W"

		if (display_.vert_display)
			line_height_ = text_height_ * 3;
		else
			line_height_ = text_height_;
	}

	// Adjust border for ruler
	bdr_top_ = 0;
	if (theApp.ruler_)
	{
		if (display_.hex_addr)
			bdr_top_ += text_height_;  // one row of text for hex offsets
		if (display_.decimal_addr || display_.line_nums)
			bdr_top_ += text_height_;  // one row of text for dec offsets
		bdr_top_ += 5;                 // allow room for a thin line
	}
#ifdef TEST_CLIPPING
	bdr_top_ += 40;
#endif

	FILE_ADDRESS length = GetDocument()->length() + display_.addrbase1;

	hex_width_ = display_.hex_addr ? SigDigits(length, 16) : 0;
	dec_width_ = display_.decimal_addr ? SigDigits(length) : 0;

	num_width_ = 0;
	calc_addr_width();
	if (display_.autofit && display_.line_nums)
	{
		// If autofit is on then rowsize_ and num_width_ are mutually dependent so
		// we have to handle this carefully.
		int prev_rowsize = 4;

		// Loop a few timres and see if the value converges
		for (int ii = 0; ii < 10; ++ii)
		{
			num_width_ = SigDigits(length/prev_rowsize);
			calc_addr_width();
			calc_autofit();
			if (rowsize_ == prev_rowsize)
				break;
			prev_rowsize = rowsize_;
		}
		// If it didn't converge then favour the larger value
		// (I think non-convergence only occurs when the row
		// size oscillates between 2 adjacent integer values.)
		if (rowsize_ < prev_rowsize)
			rowsize_ = prev_rowsize;
	}
	else if (display_.autofit)
		calc_autofit();
	else if (display_.line_nums)
	{
		num_width_ = SigDigits(length/rowsize_);
		calc_addr_width();
	}

	// Ensure offset is within valid range after recalc of rowsize_
	offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;

	// Fit columns to window width?
	if (display_.autofit)
	{
		// Work out the current address of the caret/selection
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		// Set size before setting scroll/caret to avoid them being moved to "valid" pos
		if (display_.vert_display)
			SetTSize(CSizeAp(-1, ((GetDocument()->length() + offset_)/rowsize_ + 1)*3));  // 3 rows of text
		else
			SetTSize(CSizeAp(-1, (GetDocument()->length() + offset_)/rowsize_ + 1));

		// Make sure scroll position is at left side
//        SetScroll(CPointAp(0, topleft/rowsize_ * line_height_));
		SetScroll(CPointAp(0, GetScroll().y));

		// Move the caret/selection to the where the same byte(s) as before
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
	}
	else
	{
		if (display_.vert_display)
			SetTSize(CSizeAp(-1, ((GetDocument()->length() + offset_)/rowsize_ + 1)*3));  // 3 rows of text
		else
			SetTSize(CSizeAp(-1, (GetDocument()->length() + offset_)/rowsize_ + 1));
	}

	// Make sure we know the width of the display area
	if (display_.vert_display || display_.char_area)
		SetSize(CSize(char_pos(rowsize_-1)+text_width_w_+text_width_w_/2+1, -1));
	else
	{
		ASSERT(display_.hex_area);
		display_.hex_area = TRUE; // defensive
		SetSize(CSize(hex_pos(rowsize_-1)+2*text_width_+text_width_/2+1, -1));
	}

	if (pcv_ != NULL)
		pcv_->recalc_display();

	in_recalc_display = false;
} /* recalc_display() */

void CHexEditView::calc_addr_width()
{
	addr_width_ = hex_width_ + dec_width_ + num_width_;

	// Allow for separators (spaces and commas)
	if (theApp.nice_addr_)
		addr_width_ += (hex_width_-1)/4 + (dec_width_-1)/3 + (num_width_-1)/3;

	// Also add 1 for the colon
	addr_width_ += hex_width_ > 0 ? 1 : 0;
	addr_width_ += dec_width_ > 0 ? 1 : 0;
	addr_width_ += num_width_ > 0 ? 1 : 0;
	++addr_width_;
}

// This is just called from recalc_display
void CHexEditView::calc_autofit()
{
	CRect cli;
	CRectAp rect;           // Client rectangle in norm. coords
	GetDisplayRect(&cli);
	rect = ConvertFromDP(cli) - GetScroll();
	ASSERT(rect.left == 0 && rect.top == 0);

	// Work out how many columns we can display across the window
	// NOTE: These calcs are directly related to the calcs in hex_pos and char_pos

	// Work out width of display area (total minus address area width)
	int daw = rect.right - addr_width_*text_width_ - text_width_/2 - 1;
	if (display_.vert_display)
	{
		int group_offset = (daw/text_width_w_)%(group_by_ + 1);
		if (group_offset == 0) group_offset = group_by_;
		rowsize_ = ((daw/text_width_w_ - 1)/(group_by_ + 1)) * group_by_ + group_offset;

		// Make sure scroll bars are not shown
		ASSERT(rowsize_ < 5 || rect.right >= char_pos(rowsize_-1)+text_width_w_);
	}
	else if (display_.char_area && display_.hex_area)
	{
		int sec_len = group_by_*(3*text_width_ + text_width_w_) + text_width_;
		int group_offset = (daw % sec_len)/(3*text_width_ + text_width_w_);
		if (group_offset == group_by_) group_offset = 0;
		rowsize_ = ((daw + text_width_) / sec_len) * group_by_ + group_offset;
		ASSERT(rowsize_ < 5 || rect.right >= char_pos(rowsize_-1)+text_width_w_);
	}
	else if (display_.char_area)
	{
		// This is easy as char area has no grouping
		rowsize_ = daw/text_width_w_;
		ASSERT(rowsize_ < 5 || rect.right >= char_pos(rowsize_-1)+text_width_w_);
	}
	else
	{
		ASSERT(display_.hex_area);
		display_.hex_area = TRUE; // defensive
		//rowsize_ = (daw - daw/(3*group_by_+1))/(3*text_width_);
		int group_offset = (daw/text_width_ - 2)%(3*group_by_ + 1);
		if (group_offset == 3*group_by_) group_offset--;

		rowsize_ = ((daw/text_width_ - 2)/(3*group_by_ + 1))*group_by_ + group_offset/3 + 1;
		ASSERT(rowsize_ < 5 || rect.right >= hex_pos(rowsize_-1)+2*text_width_);
	}

	// Must display at least 4 columns & no more than buffer can hold
	if (rowsize_ < 4)
		rowsize_ = 4;
	else if (rowsize_ > max_buf)
		rowsize_ = max_buf;

#if 0 // now done after returns in recalc_display()
	offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;
#endif
}

// Return doc position given a hex area column number
// int CHexEditView::hex_pos(int column, int width) const;

// Return closest hex area column given x display position
// Inside determines the numbers returned for columns above rowsize_
//  1 (TRUE) will return a value from 0 to rowsize_ - 1
//  0 (FALSE) will return a value from 0 to rowsize_
//  -1 will return value possibly greater than rowsize_
int CHexEditView::pos_hex(int pos, int inside) const
{
	int col = pos - addr_width_*text_width_;
	col -= (col/(text_width_*(group_by_*3+1)))*text_width_;
	col = col/(3*text_width_);

	// Make sure col is within valid range
	if (col < 0) col = 0;
	else if (inside == 1 && col >= rowsize_) col = rowsize_ - 1;
	else if (inside == 0 && col > rowsize_) col = rowsize_;

	return col;
}

// Return display position given a char area column number
// int CHexEditView::char_pos(int column, int width /* = 0 */) const;

// Return closest char area column given display (X) coord
// Inside determines the numbers returned for columns above rowsize_
//  1 (TRUE) will return a value from 0 to rowsize_ - 1
//  0 (FALSE) will return a value from 0 to rowsize_
//  -1 will return value possibly greater than rowsize_
int CHexEditView::pos_char(int pos, int inside) const
{
	int col;
	if (display_.vert_display)
	{
		col = (pos - addr_width_*text_width_)/text_width_w_;
		col -= col/(group_by_+1);
	}
	else if (display_.hex_area)
		col = (pos - addr_width_*text_width_ -
			   rowsize_*3*text_width_ - ((rowsize_-1)/group_by_)*text_width_) / text_width_w_;
	else
		col = (pos - addr_width_*text_width_) / text_width_w_;

	// Make sure col is within valid range
	if (col < 0) col = 0;
	else if (inside == 1 && col >= rowsize_) col = rowsize_ - 1;
	else if (inside == 0 && col > rowsize_) col = rowsize_;

	return col;
}

CString CHexEditView::DescChar(int cc)
{
	CString retval;

	static char *ascii_control_char[] =
	{
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
		"space", "**ERROR"
	};

	static char *ebcdic_control_char[] =
	{
		"NUL", "SOH", "STX", "ETX", "PF",  "HT",  "LC",  "DEL",
		"GE",  "RLF", "SMM", "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "TM",  "RES", "NL",  "BS",  "IL",
		"CAN", "EM",  "CC",  "CU1", "IFS", "IGS", "IRS", "IUS",
		"DS",  "SOS", "FS",  "",    "BYP", "LF",  "ETB", "ESC",
		"",    "",    "SM",  "CU2", "",    "ENQ", "ACK", "BEL",
		"",    "",    "SYN", "",    "PN",  "RS",  "UC",  "EOT",
		"",    "",    "",    "CU3", "DC4", "NAK", "",    "SUB",
		"space","**ERROR"
	};

	if (EbcdicMode())
	{
		if (e2a_tab[cc] != '\0')
			retval.Format("'%c'", e2a_tab[cc]);
		else if (cc < sizeof(ebcdic_control_char)/sizeof(*ebcdic_control_char))
			retval = ebcdic_control_char[cc];
	}
	else if (cc >= 0x20 && cc < 0x7F || AnsiMode())
	{
		// Normal ASCII char
		retval.Format("'%c'", cc);
	}
	else if (OemMode())
	{
		// do nothing for other OEM chars
	}
	else if (cc < 0x20)
	{
		// ASCII Control char
		if (cc > 0 && cc < 27)
			retval.Format("%s [^%c]", ascii_control_char[cc], cc+0x40);
		else
			retval = ascii_control_char[cc];
	}
	else if (AsciiMode())
	{
		// do nothing for other ASCII chars
	}
	else if (code_page_ == CP_UTF8)
	{
		ASSERT(CodePageMode() && cc > 0x7F);
		if ((cc & 0xC0) == 0x80)
			retval = "cont";
		//else if ((cc & 0x0xE0) -- 0xC0)
		//	retval = "UTF2";
		//else if ((cc & 0x0xF0) -- 0xE0)
		//	retval = "UTF3";
		//else if ((cc & 0x0xF8) -- 0xF0)
		//	retval = "UTF4";
		//else if ((cc & 0x0xFC) -- 0xF8)
		//	retval = "UTF5";
		//else if ((cc & 0x0xFE) -- 0xFC)
		//	retval = "UTF6";
	}
	else
	{
		ASSERT(CodePageMode());
		if (::IsDBCSLeadByteEx(code_page_, cc))
			retval = "lead-in";
	}
	return retval;
}

// Set which code page is used when display_.char_set == CHARSET_CODEPAGE
void CHexEditView::SetCodePage(int cp)
{
	if (cp == code_page_)    // same as current
		return;

	code_page_ = cp;
	if (code_page_ == CP_UTF8)
	{
		max_cp_bytes_ = 0;  // UTF-8 is handled differently - set to zero to detect accidental use of normal CP code
		return;
	}

	CPINFOEX cpie;
	VERIFY(GetCPInfoEx(code_page_, 0, &cpie));
	max_cp_bytes_ = cpie.MaxCharSize;
}

// codepage_startchar scans the current display buffer to work out which byte is the first
// character start byte on each display line.  It is only used when displaying a codepage char
// set (display_.char_set == CHARSET_CODEPAGE).  It is also not used for:
// - single byte char sets (SBCS) since the first byte on the line is always the first char
// - UTF-8 (CP_UTF8) since it is easy to tell the difference between start bytes and continuation 
//          bytes (and Windows does not properly support UTF-8 like other code pages)
// Returns: an array of addresses which has an address for each row of text in the display.  These
//          are the addresses of the first "start" byte of the first complete character on the line.
//          That is, they are >= the start address of the line and < start address + max_cp_bytes.
// Note: We need this for 2 reasons:
//  1. To ensure that the character display does not change as the user scrolls up and down due
//     to some bytes being interpreted differently (either start or continuation bytes).
//     This requires scanning from before that start of the diplayed bytes to ensure we are in sync.
//  2. Allow draw from bottom and drwa from top of display to gice the same results
std::vector<FILE_ADDRESS> CHexEditView::codepage_startchar(FILE_ADDRESS fst, FILE_ADDRESS lst)
{
	ASSERT(display_.char_set == CHARSET_CODEPAGE && max_cp_bytes_ > 1 && code_page_ != CP_UTF8);
	std::vector<FILE_ADDRESS> retval;

	FILE_ADDRESS aa = fst - 16;  // We need to start well before the display top to make sure we are in sync
	if (aa < 0) aa = 0;                         // Don't go back before start of file

	// Get buffer of data to check
	size_t len = size_t(lst - aa) + max_cp_bytes_;  // this gets (fst-aa) extra bytes at the start AND an extra max_cp_bytes_ at the end
	char *buf = new char[len];
	size_t got = GetDocument()->GetData((unsigned char *)buf, len-1, aa);
	ASSERT(got >= size_t(lst - aa));
	buf[got] = '\0';                            // Add a string teminator just to make sure we don't run off the end of the data

	FILE_ADDRESS nn = fst;                      // nn is address of the start of the next line
	for ( const char *pp = buf; ; )
	{
		// Have we reached the end of this row yet?
		// Note that the number of bytes in a "char" may be 5 or more which is longer than the minimum row size (4)
		//      so some rows may have the same address (ie a row may be all leader bytes).
		while (aa >= nn && nn < lst)
		{
			retval.push_back(aa);               // save this address as the start of the first "char" in this row
			nn += rowsize_;                     // get start of next line
		}

		// Have we got to the end of all rows?
		if (aa >= lst && nn >= lst)
			break;

		// Find the start of the next character
		const char *prev = pp;
		pp = CharNextExA(code_page_, prev, 0);
		if (pp == prev) ++pp;                   // skip nul (and error?) bytes
		aa += pp - prev;                        // update the address to match
	}
	delete[] buf;

	// xxx can we resurrect these assertions (fail with short last line)
	//ASSERT(nn == lst);
	//ASSERT(retval.size() == (lst - fst)/rowsize_);  // must have an entry for all lines of the display
	retval.push_back(-1);   // We add an extra dummy entry to simplify calcs for byte past end of file
	return retval;
}

void CHexEditView::DoInvalidate()
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
		needs_refresh_ = true;
	else
	{
		if (pav_ != NULL)
			pav_->Invalidate();
		if (pcv_ != NULL)
			pcv_->Invalidate();

		CScrView::DoInvalidate();
	}
}

// Always call this virtual wrapper of CWnd::InvalidateRect
// necessary since InvalidateRect is not virtual.
void CHexEditView::DoInvalidateRect(LPCRECT lpRect)
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
		needs_refresh_ = true;
	else
		CScrView::DoInvalidateRect(lpRect);
}

void CHexEditView::DoInvalidateRgn(CRgn* pRgn)
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
		needs_refresh_ = true;
	else
		CScrView::DoInvalidateRgn(pRgn);
}

void CHexEditView::DoScrollWindow(int xx, int yy)
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
		needs_refresh_ = true;
	else
	{
		if (theApp.ruler_ && xx != 0)
		{
			// We need to scroll the ruler (as it's outside the scroll region)
			CRect rct;
			GetDisplayRect(&rct);
			rct.top = 0;
			rct.bottom = bdr_top_;
			ScrollWindow(xx, 0, &rct, &rct);
			// Also since we do not draw partial numbers at either end
			// we have to invalidate a bit more at either end than
			// is invalidated by ScrollWindow.
			if (xx > 0)
				rct.right = rct.left + xx + text_width_*3;
			else
				rct.left = rct.right + xx - text_width_*3;
			DoInvalidateRect(&rct);
		}
		CScrView::DoScrollWindow(xx, yy);
		if (yy < 0)
		{
			// We need to invalidate a bit of the address area near the top so that partial addresses are not drawn
			CRect rct;
			GetDisplayRect(&rct);
			rct.bottom = rct.top + line_height_;
			rct.top -= line_height_/4;
			rct.right = rct.left + addr_width_*text_width_;
			DoInvalidateRect(&rct);
		}
		else if (yy > 0)
		{
			// We need to invalidate a bit below the scrolled bit in the address area since
			// it may be blank when scrolling up (blank area avoids drawing partial address)
			CRect rct;
			GetDisplayRect(&rct);
			rct.top += yy;
			rct.bottom = rct.top + line_height_;
			rct.right = rct.left + addr_width_*text_width_;
			DoInvalidateRect(&rct);
		}
	}
}

void CHexEditView::AfterScroll(CPointAp newpos)
{
	// If we have a compare view and we have synchronised scrolling then scroll to match
	if (pcv_ != NULL && display_.auto_scroll_comp)
		pcv_->SetScroll(newpos);
}

void CHexEditView::DoUpdateWindow()
{
	if (!((CHexEditApp *)AfxGetApp())->refresh_off_)
		CScrView::DoUpdateWindow();
}

void CHexEditView::DoHScroll(int total, int page, int pos)
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
	{
		needs_hscroll_ = true;
		h_total_ = total;
		h_page_ = page;
		h_pos_ = pos;
	}
	else
		CScrView::DoHScroll(total, page, pos);
}

void CHexEditView::DoVScroll(int total, int page, int pos)
{
	if (((CHexEditApp *)AfxGetApp())->refresh_off_)
	{
		needs_vscroll_ = true;
		v_total_ = total;
		v_page_ = page;
		v_pos_ = pos;
	}
	else
		CScrView::DoVScroll(total, page, pos);
}

void CHexEditView::DoUpdate()
{
	if (needs_refresh_)
	{
		CScrView::DoInvalidate();
		if (pav_ != NULL)
			pav_->Invalidate();
		needs_refresh_ = false;
		CScrView::DoUpdateWindow();
	}

	if (needs_hscroll_)
	{
		CScrView::DoHScroll(h_total_, h_page_, h_pos_);
		needs_hscroll_ = false;
	}

	if (needs_vscroll_)
	{
		CScrView::DoVScroll(v_total_, v_page_, v_pos_);
		needs_vscroll_ = false;
	}
}

// InvalidateRange - virtual function called from base class (CScrView) to cause redrawing of
// the selection when it is changed due to mouse dragging or Shift+arrow keys.
// When dragging with the mouse this function is called twice:
// - once with f flag true and passing the whole selection
// - once with f flag false and passing only the change in the selection
void CHexEditView::InvalidateRange(CPointAp start, CPointAp end, bool f /*=false*/)
{
	BOOL saved_edit_char = display_.edit_char;          // Saved value of display_.edit_char
	FILE_ADDRESS start_addr, end_addr;          // Range of addresses to invalidate

	// Work out what we are invalidating (hex or char area)
	if (display_.vert_display)
		; // do nothing since edit_char does not affect pos2addr for vert_display
	else if (!display_.hex_area ||
		(display_.char_area && pos_hex(start.x) == rowsize_ && pos_hex(end.x) == rowsize_))
		display_.edit_char = TRUE;                      // Change display_.edit_char so pos2addr() works
	else
		display_.edit_char = FALSE;

	// Work out range to invalidate (WARNING: this relies on display_.edit_char)
	start_addr = pos2addr(start);
	end_addr = pos2addr(end);

	display_.edit_char = saved_edit_char;               // Restore display_.edit_char

	if (theApp.show_other_)
		++end_addr;
	else if (start_addr == end_addr)
		return;

	if (f)
	{
		// When dragging with the mouse the selected area in the hex view is only
		// invalidated in the part of the selection that actually changes - this
		// avoids the flickering which still occurs with kb (Shift+arrow) selection.
		// However, as the selectiop for the aerial view is drawn with a boundary
		// (marching ants) this leaves bits of the old boundary behind when increasing
		// the selection size by dragging the mouse.  The f flag signals that the
		// whole of the selection (start_addr to end_addr) should be invalidated
		// in the aerial view.
		if (pav_ != NULL)
			pav_->InvalidateRange(start_addr, end_addr);
	}
	else
	{
		// Note: We need to invalidate an extra char backwards because in the hex
		// display area the white space to the right of the last byte selected is
		// not shown in reverse video.  When the selection is extended towards the
		// end of file (causing InvalidateRange to be called) not only the newly
		// selected bytes need to be invalidated but also the previous one so that
		// the white area after the character is then drawn in reverse video.
		invalidate_hex_addr_range(start_addr-1, end_addr);

		// Also invalidate in aerial view so it can "undraw" the selection if it is smaller
		if (pav_ != NULL)
			pav_->InvalidateRange(start_addr, end_addr);
	}
}

// invalidate_addr_range is called when a part of the display may need redrawing:
// - selection changed -> called from InvalidateRange
// - focus lost/gained - so that selection can be drawn differently
// - replacement of bytes in the document, perhaps in a different view
// - insertion/deletion of bytes -> the changed bytes and those following need updating
// - undo of changes
// - background search finished -> occurrences need updating
// - bookmark added or deleted, or bookmarks hidden/shown
// - highlight added or highlights hidden/shown
// - mark moved (including swap with cursor) -> old and new address
// - undo of mark move, highlight etc

// Invalidate all addresses in the range that are displayed in the hex view
// and aerial view (if there is one).
void CHexEditView::invalidate_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr)
{
	if (pav_ != NULL)
		pav_->InvalidateRange(start_addr, end_addr);
	invalidate_hex_addr_range(start_addr, end_addr);
}

// Invalidate all of displayed addresses in hex view only
void CHexEditView::invalidate_hex_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr)
{
	CRect cli;                          // Client rectangle in device coords
	CRectAp inv;                        // The rectangle to actually invalidate (doc coords)
	CRectAp disp_rect;                  // Rectangle of display in our coords
	GetDisplayRect(&cli);
	disp_rect = ConvertFromDP(cli);

	// Work out the addresses of the first and (one past) the last byte in display
	FILE_ADDRESS start_disp = (disp_rect.top/line_height_)*rowsize_ - offset_;
	FILE_ADDRESS end_disp = (disp_rect.bottom/line_height_ + 1)*rowsize_ - offset_;

	// Reduce address range to relevant (displayed) area
	if (start_addr < start_disp) start_addr = start_disp;
	if (end_addr > end_disp) end_addr = end_disp;

	if (start_addr >= end_addr)
		return;                         // Nothing to invalidate or all outside display

	// Work out line range that needs invalidating
	FILE_ADDRESS start_line = (start_addr + offset_)/rowsize_;
	FILE_ADDRESS end_line = (end_addr + offset_)/rowsize_;

	// If start and end on the same line just invalidate between them
	if (start_line == end_line)
	{
		ASSERT(display_.hex_area || display_.char_area);
		if (!display_.vert_display && display_.hex_area)
		{
			// Note: (23/11/03) go back an extra text_width_ bytes to allow for 
			// deletion mark (tracking changes) before the byte
			inv = CRectAp(hex_pos(int((start_addr+offset_)%rowsize_)) - text_width_, start_line * line_height_,
						  hex_pos(int((end_addr+offset_)%rowsize_)) + 1, (start_line + 1) * line_height_ + 1);
			CRect dev = ConvertToDP(inv);
			DoInvalidateRect(&dev);
		}
		if (display_.vert_display || display_.char_area)
		{
			// Note: (23/11/03) go back an extra 2 pixels to allow for 
			// deletion mark (tracking changes) before the byte
			inv = CRectAp(char_pos(int((start_addr+offset_)%rowsize_)) - 2, start_line * line_height_,
						  char_pos(int((end_addr+offset_)%rowsize_)) + 1, (start_line + 1) * line_height_ + 1);
			CRect dev = ConvertToDP(inv);
			DoInvalidateRect(&dev);
		}
		return;
	}

	// Start line is before end line: invalidate partial lines at each end
	ASSERT(display_.hex_area || display_.char_area);
	if (!display_.vert_display && display_.hex_area)
	{
		// Hex area
		inv = CRectAp(hex_pos(int((start_addr+offset_)%rowsize_))-text_width_, start_line * line_height_,
					  hex_pos(int(rowsize_)) + 1, (start_line + 1) * line_height_ + 1);
		CRect dev = ConvertToDP(inv);
		DoInvalidateRect(&dev);

		inv = CRectAp(hex_pos(0)-text_width_, end_line * line_height_,
					  hex_pos(int((end_addr+offset_)%rowsize_)) + 1, (end_line + 1) * line_height_ + 1);
		dev = ConvertToDP(inv);
		DoInvalidateRect(&dev);
	}
	if (display_.vert_display || display_.char_area)
	{
		// Char area or vert_display
		inv = CRectAp(char_pos(int((start_addr+offset_)%rowsize_))-2, start_line * line_height_,
					  char_pos(int(rowsize_)), (start_line + 1) * line_height_);
		CRect dev = ConvertToDP(inv);
		DoInvalidateRect(&dev);

		inv = CRectAp(char_pos(0)-2, end_line * line_height_,
					  char_pos(int((end_addr+offset_)%rowsize_)), (end_line + 1) * line_height_);
		dev = ConvertToDP(inv);
		DoInvalidateRect(&dev);
	}

	// If more than one line between start and end then invalidate that block too
	if (start_line + 1 < end_line)
	{
		ASSERT(display_.hex_area || display_.char_area);
		if (!display_.vert_display && display_.hex_area)
		{
			inv = CRectAp(hex_pos(0)-text_width_, (start_line + 1) * line_height_,
						hex_pos(rowsize_) + 1, (end_line) * line_height_ + 1);
			CRect dev = ConvertToDP(inv);
			DoInvalidateRect(&dev);
		}
		if (display_.vert_display || display_.char_area)
		{
			inv = CRectAp(char_pos(0)-2, (start_line + 1) * line_height_,
						char_pos(rowsize_) + 1, (end_line) * line_height_ + 1);
			CRect dev = ConvertToDP(inv);
			DoInvalidateRect(&dev);
		}
	}
}

// Override CScrView::ValidateScroll()
void CHexEditView::ValidateScroll(CPointAp &pos, BOOL strict /* =FALSE */)
{
	CScrView::ValidateScroll(pos, strict);

	get_search_in_range(pos);
	get_dffd_in_range(pos);
}

void CHexEditView::get_search_in_range(CPointAp &pos)
{
	// Get search occurrences currently in display area whenever we change the scroll posn -
	// this saves checking all addresses (could be millions) in OnDraw
	search_pair_.clear();
	if (GetDocument()->CanDoSearch() && theApp.pboyer_ != NULL)
	{
		CHexEditDoc *pdoc = GetDocument();
		CRect cli;
		CRectAp rct;
		GetDisplayRect(&cli);
		rct = ConvertFromDP(cli);
		FILE_ADDRESS start, end;            // range of file addresses within display

		// Get all occurrences that are within the display - this includes those
		// that have an address before the start of display but extend into it.
		start = (pos.y/line_height_)*rowsize_ - offset_     // First addr in display
				- (theApp.pboyer_->length() - 1);              // Length of current search string - 1
		if (start < 0) start = 0;                           // Just in case (prob not nec.)
		end = ((pos.y+rct.Height())/line_height_ + 1)*rowsize_ - offset_;

		std::vector<FILE_ADDRESS> sf = pdoc->SearchAddresses(start, end);
		search_length_ = theApp.pboyer_->length();
		std::vector<FILE_ADDRESS>::const_iterator pp = sf.begin();
		std::vector<FILE_ADDRESS>::const_iterator pend = sf.end();
		pair<FILE_ADDRESS, FILE_ADDRESS> good_pair;

		if (pp != pend)
		{
			good_pair.first = *pp;
			good_pair.second = *pp + search_length_;
			while (++pp != pend)
			{
				if (*pp >= good_pair.second)
				{
					search_pair_.push_back(good_pair);
					good_pair.first = *pp;
				}
				good_pair.second = *pp + search_length_;
			}
			search_pair_.push_back(good_pair);
		}
	}
}

// Gets all the template fields that have addresses within the range of the displayed
// hex view (and their background colour).  This should be called when the view is
// scrolled/resized or when the template is opened/closed/changed.
void CHexEditView::get_dffd_in_range(CPointAp &pos)
{
	// Get template fields (address range and background colour) in the display area
	dffd_bg_.clear();
	if (pdfv_ != NULL)
	{
		CRect cli;
		CRectAp rct;
		GetDisplayRect(&cli);
		rct = ConvertFromDP(cli);

		FILE_ADDRESS start, end;            // range of file addresses within display
		start = (pos.y/line_height_)*rowsize_ - offset_;     // First addr in display
		if (start < 0) start = 0;                           // Just in case (prob not nec.)
		end = ((pos.y+rct.Height())/line_height_ + 1)*rowsize_ - offset_;

		GetDocument()->FindDffdEltsIn(start, end, dffd_bg_);
	}
}

// Override CScrView::ValidateCaret()
void CHexEditView::ValidateCaret(CPointAp &pos, BOOL inside /*=true*/)
{
	// Ensure pos is a valid caret position or move it to the closest such one
	FILE_ADDRESS address = pos2addr(pos, inside);
	if (address < 0)
		address = 0;
	else if (address > GetDocument()->length())
		address = GetDocument()->length();
	if (display_.vert_display)
	{
		pos = addr2pos(address, pos2row(pos));
		// All the following is to avoid problem of dragging a selection within the same "line" but
		// up a "row" and forward a column.  This caused no selection to be drawn and the selection
		// tip to show a negative selection length.
		CPointAp start, end;            // Current selection 
		CPointAp base;                  // Base of current selection (initial point clicked)
		if (GetSel(start, end))
			base = end;
		else
			base = start;
		// If we have a non-zero selection
		if (mouse_down_ && pos != base)
			pos = addr2pos(address, pos2row(base));  // Make end selection row same as start selection row
	}
	else
		pos = addr2pos(address);
}

void CHexEditView::DisplayCaret(int char_width /*= -1*/)
{
	// If no character width given default to width of hex or char area text
	if (char_width == -1 && (display_.edit_char || display_.vert_display))
		char_width = text_width_w_;
	else if (char_width == -1)
		char_width = text_width_;
	CScrView::DisplayCaret(char_width);

	// Since the window may be scrolled without the mouse even moving we
	// have to make sure that the byte addr/ruler highlight byte is updated.
	CPoint pt;
	::GetCursorPos(&pt);            // get mouse location (screen coords)
	ScreenToClient(&pt);
	set_mouse_addr(address_at(pt));

	move_dlgs();  // scrolling may also have moved caret under a dialog
}

// Move the modeless dialogs if visible and it/they obscure the caret
void CHexEditView::move_dlgs()
{
	// Get doc size so we know width of visible area
	CSizeAp tt, pp, ll;                 // Size of document total,page,line
	GetSize(tt, pp, ll);

	// Get the selection so we can work out a rectangle bounding the selection
	CPointAp start, end;                // Points of start/end of selection
	GetSel(start, end);

	CRectAp selrect;                    // Rect enclosing selection + a bit
	if (start.y == end.y)               // No selection or all on a single line?
		selrect = CRectAp(start.x - 2*ll.cx, start.y - ll.cy,
						  end.x + 3*ll.cx, end.y + 2*ll.cy);
	else
		selrect = CRectAp(0, start.y - ll.cy,  // multi-line selection
						  tt.cx, end.y + 2*ll.cy);

	// Get display rectangle (also in doc coords)
	CRect cli;
	GetDisplayRect(&cli);
	CRectAp doc_rect = ConvertFromDP(cli);

	// See if any of the selection is visible (intersect selection & display)
	CRectAp sd_rect;                    // selection that is visible
	if (!sd_rect.IntersectRect(doc_rect, selrect))
		return;                         // Selection not in display

	// Get rect (selection visible in display) in screen (device) coords
	CRect dev_rect = ConvertToDP(sd_rect);
	ClientToScreen(&dev_rect);

	HideCaret();
	// Tell mainframe to move all its dialog bars
	if (theApp.dlg_move_)
		((CMainFrame *)theApp.m_pMainWnd)->move_bars(dev_rect);
	ShowCaret();
}

// Move scroll or caret position in response to a key press.
// Note that this overrides CScrView::MovePos().
BOOL CHexEditView::MovePos(UINT nChar, UINT nRepCnt,
						 BOOL control_down, BOOL shift_down, BOOL caret_on)
{
	// CScrView::MovePos scrolling behaviour is OK
	if (!caret_on)
		return CScrView::MovePos(nChar, nRepCnt, control_down, shift_down, caret_on);

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (aa->recording_)
	{
		long vv = nChar;
		if (control_down) vv |= 0x10000;
		if (shift_down)   vv |= 0x20000;
		for (UINT ii = 0; ii < nRepCnt; ++ii)
			aa->SaveToMacro(km_key, vv);
	}

	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);   // Is selection base at end of selection?
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());
	FILE_ADDRESS new_address;
	CString desc("Cursor key");

	// Start with start of (or end of, if moving forwards) current selection
	if (shift_down)
	{
		// Work out which end of selection is being extended
		if (end_base)
			new_address = start_addr;
		else
			new_address = end_addr;
		++shift_moves_;
	}
	else if (start_addr == end_addr )
		new_address = start_addr;                       // No current selection
	else if (nChar == VK_DOWN || nChar == VK_NEXT)
		new_address = end_addr;                         // Move from char after selection
	else if (nChar == VK_RIGHT || nChar == VK_END)
		new_address = end_addr - 1;                     // Move from last char of selection
	else
		new_address = start_addr;                       // Move from start of selection

	CSizeAp tt, pp, ll;                   // Size of document total,page,line

	switch (nChar)
	{
	case VK_LEFT:
		if (control_down)
		{
			desc = "Ctrl + Left Arrow";
			// Work out how many groups there are to start of file
			long gpr = (rowsize_ - 1)/group_by_ + 1;    // groups per row
			FILE_ADDRESS groups = ((new_address+offset_)/rowsize_) * gpr +
						  ((new_address+offset_)%rowsize_ + group_by_ - 1)/group_by_;
			// Calculate the group to move to and address of 1st byte
			groups -= nRepCnt;
			new_address = (groups/gpr) * rowsize_ - offset_ + (groups%gpr) * group_by_;
		}
		else
		{
			desc = "Left Arrow";
			new_address -= nRepCnt;
		}
		break;
	case VK_RIGHT:
		if (control_down)
		{
			desc = "Ctrl + Right Arrow";
			// First work out how many groups there are to start of file
			long gpr = (rowsize_ - 1)/group_by_ + 1;    // groups per row
			FILE_ADDRESS groups = ((new_address+offset_)/rowsize_) * gpr +
						  ((new_address+offset_)%rowsize_)/group_by_;
			// Calculate the group to move to
			groups += nRepCnt;
			new_address = (groups/gpr) * rowsize_ - offset_ + (groups%gpr) * group_by_;
		}
		else
		{
			desc = "Right Arrow";
			new_address += nRepCnt;
		}
		break;
	case VK_UP:
		desc = "Up Arrow";
		if (display_.vert_display && !shift_down)
		{
			new_address -= rowsize_ * ((2 - row + nRepCnt)/3);
			row = (3333 + row - nRepCnt)%3;   // Add a large number div. by 3 to make sure % operand is +ve
		}
		else
			new_address -= rowsize_ * nRepCnt;
		break;
	case VK_DOWN:
		desc = "Down Arrow";
		if (display_.vert_display && !shift_down)
		{
			new_address += rowsize_ * ((row + nRepCnt)/3);
			row = (row + nRepCnt)%3;
		}
		else
			new_address += rowsize_ * nRepCnt;
		break;
	case VK_HOME:
		if (control_down)
		{
			desc = "Ctrl + Home key ";  // space at end means significant nav pt
			new_address = 0;
		}
		else
		{
			desc = "Home key";
			new_address = ((new_address+offset_)/rowsize_) * rowsize_ - offset_;
		}
		break;
	case VK_END:
		if (control_down)
		{
			desc = "Ctrl + End key ";  // space at end means significant nav pt
			new_address = GetDocument()->length();
		}
		else
		{
			desc = "End key";
			new_address = ((new_address+offset_)/rowsize_ + 1) * rowsize_ - offset_ - 
								(shift_down ? 0 : 1);
		}
		break;
	case VK_PRIOR:
		desc = "Page Up";
		GetSize(tt, pp, ll);
		new_address -= rowsize_ * (pp.cy/line_height_) * nRepCnt;
		break;
	case VK_NEXT:
		desc = "Page Down";
		GetSize(tt, pp, ll);
		new_address += rowsize_ * (pp.cy/line_height_) * nRepCnt;
		break;
	default:
		return CScrView::MovePos(nChar, nRepCnt, control_down, shift_down, caret_on);
	}

	if (new_address < 0)
	{
		new_address = 0;
		row = 0;
		aa->mac_error_ = 2;
	}
	else if (new_address > GetDocument()->length())
	{
		new_address = GetDocument()->length();
		if (display_.vert_display && !shift_down)
			row = 2;
		aa->mac_error_ = 2;
	}

	// Scroll addresses into view if moved to left column of hex area or 
	// left column of char area when no hex area displayed
	if ((new_address + offset_) % rowsize_ == 0 && (display_.vert_display || !display_.edit_char || !display_.hex_area))
		SetScroll(CPointAp(0,-1));

	if (shift_down && end_base)
	{
		MoveWithDesc("Shift + " + desc, end_addr, new_address);

		// Handle this when shift key released now (in OnKeyUp)
//        if (aa->highlight_)
//            add_highlight(new_address, end_addr, TRUE);
	}
	else if (shift_down)
	{
		MoveWithDesc("Shift + " + desc, start_addr, new_address);

		// Handle this when shift key released now (in OnKeyUp)
//        if (aa->highlight_)
//            add_highlight(start_addr, new_address, TRUE);
	}
	else
		MoveWithDesc(desc, new_address, -1, -1, -1, FALSE, FALSE, row);

	return TRUE;                // Indicate that keystroke used
}

// Move the caret to new position (or as close as possible).
// Returns the new position which may be diff to the requested position if
// the requested position was invalid (past EOF).
// Note: Does not update address in tool bar combo (use show_pos(GetPos()),
//       or make sure the caret within display, and does not save undo info.
FILE_ADDRESS CHexEditView::GoAddress(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr /*=-1*/)
{
	// Get row from top 2 bits of start_address (vert_display mode)
	int row = int(start_addr>>62) & 0x3;
	start_addr &= 0x3fffFFFFffffFFFF;

	if (end_addr < 0 || end_addr > GetDocument()->length())
		end_addr = start_addr;
	if (start_addr < 0 || start_addr > GetDocument()->length())
		start_addr = end_addr = GetDocument()->length();

	ASSERT(row == 0 || (row < 3 && display_.vert_display && start_addr == end_addr));
	SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));

	return start_addr;
}

// Move the caret to new position updating everything:
//  - saves previous position in undo array (if caret is actually moved)
//  - updates the display address in the tool bar address edit controls
//  - Makes sure the caret is visible within display
//  - astart/aend = new selection (if aend = -1 or aend=astart then just sets caret)
//  - pstart/pend = previous selection to be saved in undo list (if they are 
//    -1 it uses the current selection/caret)
//  - ptoo = when undo info saved combine it with previous operation (used if the move is part of a larger operation)
//  - no_dffd = don't sync DFFD view (if any) even in sync mode (avoids inf. mutually recursive calls)
//  - row = stacked mode row (0 to 2) - only important when setting caret not for block selection (includes all rows)
//  - desc = describes why a move was made (eg Bookmark, Search) so that info can be given in nav point list
void CHexEditView::MoveToAddress(FILE_ADDRESS astart, FILE_ADDRESS aend /*=-1*/,
								 FILE_ADDRESS pstart /*=-1*/, FILE_ADDRESS pend /*=-1*/,
								 BOOL ptoo /*=FALSE*/, BOOL no_dffd /*=FALSE*/,
								 int row /*=0*/, LPCTSTR desc /*=NULL*/)
{
	ASSERT((astart & ~0x3fffFFFFffffFFFF) == 0); // Make sure top 2 bits not on
	ASSERT(pstart == -1 || (pstart & ~0x3fffFFFFffffFFFF) == 0);

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing since caret moved

	if (astart < 0 || astart > GetDocument()->length())
	{
#ifdef SYS_SOUNDS
		if (!CSystemSound::Play("Invalid Address"))
#endif
			::Beep(5000,200);

		char buf[128];
		sprintf(buf, "Attempt to jump to invalid address %I64d\n\n"
				  "Jump to end of file instead and continue?", __int64(astart));
		if (CAvoidableDialog::Show(IDS_INVALID_JUMP, buf, "", 
			           MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
		{
			theApp.mac_error_ = 10;
			return;
		}
		else
			theApp.mac_error_ = 1;

		astart = GetDocument()->length();
	}
	if (aend < 0 || aend > GetDocument()->length())
		aend = astart;
	ASSERT(pstart >= -1 && pstart <= GetDocument()->length());
	ASSERT(pend >= pstart && pend <= GetDocument()->length());

	int prow = 0;   // Row of cursor if vert_display mode
	if (pstart < 0 || pstart > GetDocument()->length() ||
		pend < pstart || pend > GetDocument()->length())
	{
		GetSelAddr(pstart, pend);
		if (pstart == pend && display_.vert_display)
			prow = pos2row(GetCaret());
	}

	// Is the caret/selection now in a different position
	if (astart != pstart || aend != pend || row != prow)
	{
		// Move the caret/selection (THIS IS THE IMPORTANT BIT)
		SetSel(addr2pos(astart, row), addr2pos(aend, row), true);

		// Now check if we are just reversing an operation and we should just remove previous undo
		if (theApp.intelligent_undo_ &&         // intelligent undo is turned on
			astart == aend &&                   // not a selection
			undo_.size() > 0 &&                 // there is an undo op on the stack
			undo_.back().utype == undo_move &&  // previous op was a move
			!undo_.back().previous_too &&       // and not part of another operation
			undo_.back().address == (astart | (FILE_ADDRESS(row)<<62)))  // and same address/row
		{
			undo_.pop_back();
		}
		else
		{
			undo_.push_back(view_undo(undo_move, ptoo));  // Save move in undo array
			undo_.back().address = pstart | (FILE_ADDRESS(prow)<<62);
			if (pstart != pend)
			{
				undo_.push_back(view_undo(undo_sel, ptoo)); // Save selection end in undo array
				undo_.back().address = pend;
			}
		}

		nav_save(astart, aend, desc);

		show_prop();                            // Update prop modeless dlg
		show_calc();
		show_pos(-1, no_dffd);                  // Update tool bar
	}

	DisplayCaret();                             // Make sure caret is in the display
}

void CHexEditView::SetSel(CPointAp start, CPointAp end, bool base1 /*= false*/)
{
	if (theApp.hl_caret_)
	{
		FILE_ADDRESS old_addr, end_addr, new_addr;
		BOOL end_base = GetSelAddr(old_addr, end_addr);             // Get current caret before CScrView::SetSel moves it
		//if (end_base) old_addr = end_addr;

		CScrView::SetSel(start, end, base1);

		new_addr = pos2addr(start);
		if (old_addr != new_addr)
		{
			invalidate_addr(old_addr);
			invalidate_addr(new_addr);
			invalidate_ruler(old_addr);
			invalidate_ruler(new_addr);
		}
	}
	else
		CScrView::SetSel(start, end, base1);
}

void CHexEditView::set_mouse_addr(FILE_ADDRESS addr)
{
	if (!theApp.hl_mouse_ || addr == mouse_addr_)
		return;                 // no change so do nothing

	FILE_ADDRESS old_addr = mouse_addr_;
	mouse_addr_ = addr;
	invalidate_addr(old_addr);
	invalidate_ruler(old_addr);
	invalidate_addr(addr);
	invalidate_ruler(addr);

	if (addr > - 1)
		track_mouse(TME_LEAVE); // make sure we get a leave event when the mouse is moved outside
}

// Invalidate part of ruler related to an address
void CHexEditView::invalidate_ruler(FILE_ADDRESS addr)
{
	if (!theApp.ruler_)
		return;

	int horz = bdr_left_ - GetScroll().x;       // Offset of left side of doc from left side of window

	CRect rct;
	rct.top = 0;
	rct.bottom = bdr_top_;
	if (!display_.vert_display && display_.hex_area)
	{
		rct.left  = hex_pos(int((addr+offset_)%rowsize_)) + horz;
		rct.right = hex_pos(int((addr+offset_)%rowsize_)+1) + horz;
		DoInvalidateRect(&rct);
	}
	if (display_.vert_display || display_.char_area)
	{
		rct.left  = char_pos(int((addr+offset_)%rowsize_)) + horz;
		rct.right = char_pos(int((addr+offset_)%rowsize_)+1) + horz + 1;
		DoInvalidateRect(&rct);
	}
}

#ifdef RULER_ADJUST
// Invalidate part of ruler related to an address
void CHexEditView::invalidate_adjuster(int col)
{
	if (col < 0)
		return;
	CRect rct(-1, 0, -1, 30000);
	if (!display_.vert_display && display_.hex_area)
	{
		rct.left = hex_pos(col) - scrollpos_.x;
		rct.right = rct.left + 4;
		rct.left -= 7;
		if (col%group_by_ == 0)
			rct.left -= text_width_;
		InvalidateRect(&rct);   // DoInvalidateRect not nec. as we can't be in a macro??
	}
	if (display_.vert_display || display_.char_area)
	{
		rct.left = char_pos(col) - scrollpos_.x;
		rct.right = rct.left + 4;
		rct.left -= 7;
		if (display_.vert_display && col%group_by_ == 0)
			rct.left -= text_width_w_/2;
		InvalidateRect(&rct);
	}
}
#endif

// Invalidate address area based on an address
void CHexEditView::invalidate_addr(FILE_ADDRESS addr)
{
	CRect rct;
	GetDisplayRect(&rct);
	CRectAp doc_rect = ConvertFromDP(rct);
	CRect addr_rect;
	addr_rect.left  = bdr_left_;
	addr_rect.right = bdr_left_ - doc_rect.left + addr_width_*text_width_ - text_width_/2;

	// If moved and the address area is visible ...
	if (addr_rect.right <= addr_rect.left)
		return;         // Address area is off window to the left

	FILE_ADDRESS first_disp = (doc_rect.top/line_height_) * rowsize_ - offset_;
	FILE_ADDRESS last_disp = (doc_rect.bottom/line_height_ + 1) * rowsize_ - offset_;
	if (addr >= first_disp && addr < last_disp)
	{
		addr_rect.top    = int(bdr_top_ - doc_rect.top + addr2pos(addr).y);
		addr_rect.bottom = addr_rect.top + text_height_;
		DoInvalidateRect(&addr_rect);
	}
}

void CHexEditView::nav_save(FILE_ADDRESS astart, FILE_ADDRESS aend, LPCTSTR desc)
{
	if (theApp.navman_.InMove())
		return;

	// Check if we have moved enough to store a nav pt
	bool save_nav = false;      // Is this a significant enough move to record in nav stack?
	bool significant = desc != NULL && desc[strlen(desc)-1] == ' ';  // Important nav pts have space at end of desc
	++nav_moves_;

	// The number of rows to move before adding a new nav pt (for significant and insignificant events)
	FILE_ADDRESS significant_rows = 2;
	FILE_ADDRESS insignificant_rows = 20; //insignificant_rows = win_size_.cy / line_height_ / 2;

	// Note: The below checks if the caret has moved more than half a page AND the display has
	// scrolled.  BUT if desc is not NULL (a description was supplied) then this is some sort of
	// significant event (search, jump to bookmark etc) so keep even if just moved > 2 lines.

	// Check if the start of selection has moved significantly
	FILE_ADDRESS curr_line = (astart + offset_)/rowsize_;
	FILE_ADDRESS prev_line = (nav_start_ + offset_)/rowsize_;
	if (!significant && mac_abs(curr_line - prev_line) > insignificant_rows ||
		 significant && mac_abs(curr_line - prev_line) > significant_rows)
	{
		save_nav = true;
	}

	// Now do the same test on selection end address
	curr_line = (aend + offset_)/rowsize_;
	prev_line = (nav_end_ + offset_)/rowsize_;
	if (!significant && mac_abs(curr_line - prev_line) > insignificant_rows ||
		 significant && mac_abs(curr_line - prev_line) > significant_rows)
	{
		save_nav = true;
	}

	// If we haven't scrolled vertically yet don't add nav point (unless significant)
	if (!significant && nav_scroll_ == (GetScroll().y/line_height_)*rowsize_ - offset_)
		save_nav = false;

	if (save_nav)
	{
		nav_start_ = astart;
		nav_end_ = aend;
		nav_scroll_ = (GetScroll().y/line_height_)*rowsize_ - offset_;

		// If no descripition given work out something from the file
		if (desc == NULL)
			desc = "Move";

		// Save nav pt in the stack
		theApp.navman_.Add(desc, get_info(), this, nav_start_, nav_end_, nav_scroll_);
	}
}

// Get a bit of the file (as hex or text) to save with a nav point
CString CHexEditView::get_info()
{
	FILE_ADDRESS start = nav_start_;
	if (start >= GetDocument()->length())
	{
		return CString("End\nof\nfile");
	}

	if (display_.vert_display && pos2row(GetCaret()) == 0 || display_.edit_char)
	{
		// Take a sample of the file as text
		char buf[36];
		char *pp = buf;
		FILE_ADDRESS aa, end;
		unsigned char cc = '\0';
		end = start + 30;
		if (end > GetDocument()->length())
			end = GetDocument()->length();
		// Just use the selection if it is short enough
		if (start != nav_end_ && nav_end_ < end)
			end = nav_end_;
		for (aa = start; aa < end; ++aa)
		{
			GetDocument()->GetData(&cc, 1, aa);
			if (display_.char_set != CHARSET_EBCDIC)
			{
				if (isprint(cc))
					*pp++ = cc;
				else
					goto hex_info;
			}
			else
			{
				if (e2a_tab[cc] != '\0')
					*pp++ = e2a_tab[cc];
				else
					goto hex_info;
			}
		}
		*pp = '\0';
		if (aa < GetDocument()->length())
			strcat(pp, "...");
		return CString(buf);
	}

hex_info:
	{
		// Take a sample of the file as hex
		char buf[36];
		char *pp = buf;
		FILE_ADDRESS aa, end;
		unsigned char cc = '\0';
		end = start + 10;
		if (end > GetDocument()->length())
			end = GetDocument()->length();
		// Just use the selection if it is short enough
		if (start != nav_end_ && nav_end_ < end)
			end = nav_end_;
		for (aa = start; aa < end; ++aa)
		{
			GetDocument()->GetData(&cc, 1, aa);
			if (theApp.hex_ucase_)
				sprintf(pp, "%2.2X ", cc);
			else
				sprintf(pp, "%2.2x ", cc);
			pp += 3;
		}
		if (aa < GetDocument()->length())
			strcat(pp, "...");
		return CString(buf);
	}
}

#if 0
// This saves a move in the undo array for the caret position when focus
// was last lost.  This is used by the tool bar address edit controls to
// add to the undo list when they move the address of the caret.
void CHexEditView::SaveMove()
{
	undo_.push_back(view_undo(undo_move));
	undo_.back().address = saved_start_ | (FILE_ADDRESS(saved_row_)<<62);
	if (saved_start_ != saved_end_)
	{
		// Save the previous selection
		undo_.push_back(view_undo(undo_sel));
		undo_.back().address = saved_end_;
	}
}
#endif

// Convert an address to a document position
CPointAp CHexEditView::addr2pos(FILE_ADDRESS address, int row /*=0*/) const
{
	ASSERT(row == 0 || (display_.vert_display && row < 3));
	address += offset_;
	if (display_.vert_display)
		return CPointAp(char_pos(int(address%rowsize_)), (address/rowsize_) * line_height_ + row * text_height_);
	else if (display_.edit_char)
		return CPointAp(char_pos(int(address%rowsize_)), address/rowsize_ * line_height_);
	else if ((num_entered_ % 2) == 0)
		return CPointAp(hex_pos(int(address%rowsize_)), address/rowsize_ * line_height_);
	else
		return CPointAp(hex_pos(int(address%rowsize_)) + 2*text_width_,
					  address/rowsize_ * line_height_);
}

FILE_ADDRESS CHexEditView::pos2addr(CPointAp pos, BOOL inside /*=true*/) const
{
	FILE_ADDRESS address;
	address = (pos.y/line_height_)*rowsize_ - offset_;
	if (display_.vert_display || display_.edit_char)
		address += pos_char(pos.x, inside);
	else
		address += pos_hex(pos.x + text_width_/2, inside);
	return address;
}

// Given a point in doc coords returns which of the 3 rows of the line the point is in
// Note that in vert_display mode row 0 = char, row 1 = top nybble, row 2 = bottom nybble
int CHexEditView::pos2row(CPointAp pos)
{
	ASSERT(display_.vert_display);
	ASSERT((pos.y%line_height_)/text_height_ < 3);
	return int((pos.y%line_height_)/text_height_);
}

// Update property display if visible
// address = address of byte(s) to show properties of
//         = -1 to use the current cursor address
//         = -2 to update even when refresh off (using current cursor address)
void CHexEditView::show_prop(FILE_ADDRESS address /*=-1*/)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (address != -2 && aa->refresh_off_) return;

	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (address < 0) address = GetPos();
	mm->m_wndProp.Update(this, address);
}

void CHexEditView::show_calc()
{
	// xxx no longer needed?
}

// Update hex and decimal address tools in edit bar
// address = address to show in the address tools
//         = -1 to use the current cursor address
//         = -2 to update even when refresh off (using current cursor address)
void CHexEditView::show_pos(FILE_ADDRESS address /*=-1*/, BOOL no_dffd /*=FALSE*/)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (address != -2 && aa->refresh_off_) return;

	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	CString ss;         // String to hold text of edit box

	FILE_ADDRESS end;
	if (address < 0)
		GetSelAddr(address, end);
	else
		end = address;

#if 0 // BCG (this is handled by calls to AfxGetApp()->OnIdle(0) in searches/compares)
	// Set the hex address edit box
	if (aa->hex_ucase_)
		ss.Format("%lX", address);
	else
		ss.Format("%lx", address);
	mm->hec_hex_addr_.SetWindowText(ss);
	mm->hec_hex_addr_.add_spaces();
	mm->hec_hex_addr_.UpdateWindow();           // Force immed. redraw

	// Set the decimal address edit box 
	ss.Format("%lu", address);
	mm->dec_dec_addr_.SetWindowText(ss);
	mm->dec_dec_addr_.add_commas();
	mm->dec_dec_addr_.UpdateWindow();           // Force immed. redraw
#else
//    TRACE1("---- Address set to %ld\n", long(address));
	((CMainFrame *)AfxGetMainWnd())->SetAddress(address);  // for ON_UPDATE_COMMAND_UI to fix displayed addresses
#endif
	// Move to corresponding place in other views if sync on
	if (pdfv_ != NULL && !no_dffd && display_.auto_sync_dffd)
		pdfv_->SelectAt(address);
	if (pav_ != NULL && display_.auto_sync_aerial)
		pav_->ShowPos(address);
	if (pcv_ != NULL && display_.auto_sync_comp)
		pcv_->MoveToAddress(address, end);
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditView diagnostics

#ifdef _DEBUG
void CHexEditView::AssertValid() const
{
		CScrView::AssertValid();
}

void CHexEditView::Dump(CDumpContext& dc) const
{
	dc << "\nrowsize_   = " << rowsize_;
	dc << "\ntext_height_ = " << text_height_;
	dc << "\ntext_width_  = " << text_width_;
	dc << "\nmark_      = " << long(mark_);
	dc << "  mark_char_ = " << display_.mark_char;
	dc << "\nedit_char_ = " << display_.edit_char;
	dc << "\novertype_  = " << display_.overtype;
	dc << "\nreadonly_  = " << display_.readonly;
	dc << "\nsaved_start_/end_ = " << long(saved_start_) << " " << long(saved_end_);

	std::vector <view_undo, allocator<view_undo> >::const_iterator pu;
	for (pu = undo_.begin(); pu != undo_.end(); ++pu)
	{
		dc << "\nutype = " << (*pu).utype;
		switch (pu->utype)
		{
		case undo_move:
			dc << " MOVE from " << long((*pu).address);
			break;
		case undo_sel:
			dc << " SELECTION to " << long((*pu).address);
			break;
		case undo_change:
			dc << " DOC CHANGE this view = " << (*pu).flag;
			dc << " index = " << (*pu).index;
			break;
		case undo_font:
			dc << " FONT CHANGE ";
			break;
		case undo_setmark:
			dc << " MARK SET address = " << long((*pu).address);
			break;
		case undo_rowsize:
			dc << " ROW SIZE = " << (*pu).rowsize;
			break;
		case undo_group_by:
			dc << " GROUP BY = " << (*pu).rowsize;
			break;
		case undo_offset:
			dc << " OFFSET   = " << (*pu).rowsize;
			break;
		case undo_codepage:
			dc << " Code page = " << long((*pu).codepage);
			break;
		case undo_unknown:
			dc << " UNKNOWN";
			break;
		default:
			dc << " NOT RECOGNISED";
			break;
		}
	  }

	CScrView::Dump(dc);
}

CHexEditDoc* CHexEditView::GetDocument() // non-debug version is inline
{
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));
		return (CHexEditDoc*)m_pDocument;
}
#endif //_DEBUG

// Get child frame window of this view.  This may be the parent if there are no splitters,
// or it may be a more distant ancestor if the immediate parent is a splitter.
// This could possibly return NULL but I'm not sure why.
CChildFrame *CHexEditView::GetFrame() const
{
	CWnd *pp = GetParent();
	while (pp != NULL && !pp->IsKindOf(RUNTIME_CLASS(CChildFrame)))
		pp = pp->GetParent();
	ASSERT_KINDOF(CChildFrame, pp);

	return (CChildFrame *)pp;
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditView message handlers

BOOL CHexEditView::OnEraseBkgnd(CDC* pDC)
{
	if (bg_col_ == -1) return CScrView::OnEraseBkgnd(pDC);

	CRect rct;
	GetClientRect(rct);

	// Fill background with bg_col_
	CBrush backBrush;
	backBrush.CreateSolidBrush(bg_col_);
	backBrush.UnrealizeObject();
	pDC->FillRect(rct, &backBrush);

	// Get rect for address area
	rct.right = addr_width_*text_width_ - GetScroll().x - text_width_ + bdr_left_;

	// If address area is visible and address background is different to normal background ...
	if (rct.right > rct.left && addr_bg_col_ != bg_col_)
	{
		// Draw address background too
		CBrush addrBrush;
		addrBrush.CreateSolidBrush(addr_bg_col_);
		addrBrush.UnrealizeObject();
		pDC->FillRect(rct, &addrBrush);
	}
	if (theApp.ruler_ && addr_bg_col_ != bg_col_)
	{
		// Ruler background
		GetClientRect(rct);
		rct.bottom = bdr_top_ - 4;
		CBrush addrBrush;
		addrBrush.CreateSolidBrush(addr_bg_col_);
		addrBrush.UnrealizeObject();
		pDC->FillRect(rct, &addrBrush);
	}

#ifdef _DEBUG
	// Draw the location of the borders to make sure nothing's drawn outside
	GetClientRect(rct);
	CPen *psaved, pen(PS_SOLID, 1, RGB(255,0,0));
	psaved = pDC->SelectObject(&pen);

	CPoint pt;
	pt.x = rct.right - bdr_right_;pt.y = bdr_top_ - 1;
	pDC->MoveTo(pt);
	pt.y = rct.bottom - bdr_bottom_;
	pDC->LineTo(pt);
	pt.x = bdr_left_ - 1;
	pDC->LineTo(pt);
	pt.y = bdr_top_ - 1;
	pDC->LineTo(pt);
	pDC->SelectObject(psaved);
#endif

	return TRUE;
}

void CHexEditView::OnSize(UINT nType, int cx, int cy)
{
	if (cx == 0 && cy == 0 || rowsize_ == 0 || in_recalc_display)
	{
		CScrView::OnSize(nType, cx, cy);
		return;
	}
	num_entered_ = num_del_ = num_bs_ = 0;      // Can't be editing while mousing

	if (display_.autofit && text_height_ > 0)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		// This is to try to stay at the same part of the file when in
		// autofit mode and we get multiple consecutive resize events
		if (resize_start_addr_ == -1 || resize_curr_scroll_ != GetScroll().y)
			resize_start_addr_ = pos2addr(GetScroll());

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		recalc_display();
		CPointAp pt = addr2pos(resize_start_addr_);
		pt.x = 0;
		SetScroll(pt);

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));

		if (pcv_ != NULL)
			pcv_->end_change();
	}
	else
		recalc_display();

	CScrView::OnSize(nType, cx, cy);

	// Make sure we show all visible search occurrences for the new window size
	ValidateScroll(GetScroll());

	resize_curr_scroll_ = GetScroll().y;   // Save current pos so we can check if we are at the same place later
}

void CHexEditView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CScrView::OnHScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode != SB_THUMBTRACK)
		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_hscroll, (nSBCode << 16) | nPos);
}

void CHexEditView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CScrView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (nSBCode != SB_THUMBTRACK)
		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_vscroll, (nSBCode << 16) | nPos);
}

BOOL CHexEditView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	BOOL retval;

	// As the address under the mouse will probably change we need to get
	// rid of the tip window which shows info about the byte under the mouse.
	tip_.Hide(0);
	if (last_tip_addr_ != -1)
	{
		FILE_ADDRESS addr = last_tip_addr_;
		last_tip_addr_ = -1;
		invalidate_hex_addr_range(addr, addr+1);
	}
#ifdef RULER_ADJUST
	ruler_tip_.Hide(0);
#endif

	if ((nFlags & MK_CONTROL) != 0)
	{
		// Ctrl+ mouse wheel zooms in/out
		bool zoomIn = zDelta > 0;
		if (theApp.reverse_zoom_) zoomIn = !zoomIn;
		if (zoomIn)
			OnFontInc();
		else
			OnFontDec();
		retval = TRUE;
	}
	else
		retval = CScrView::OnMouseWheel(nFlags, zDelta, pt);

	if (theApp.hl_mouse_)
	{
		// Since the window may be scrolled without the mouse even moving we
		// have to make sure that the byte addr/ruler highlight byte is updated.
		CPoint point;
		::GetCursorPos(&point);         // get mouse location (screen coords)
		ScreenToClient(&point);
		set_mouse_addr(address_at(point));
	}

	return retval;
}

void CHexEditView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (mouse_down_ && (drag_mark_ || drag_bookmark_ > -1) && nChar == 27)
	{
		// Escape key aborts bookmark drag
		drag_mark_ = false; drag_bookmark_ = -1;
		mouse_down_ = false;
		invalidate_addr_range(drag_address_, drag_address_+1); // remove current drag position
		SendMessage(WM_SETCURSOR, WPARAM(m_hWnd), MAKELPARAM(HTCLIENT, 0));
	}
	else if ((nFlags & 0x2100) == 0)
	{
		for (UINT ii = 0; ii < nRepCnt; ++ii)
			do_char(nChar);
	}
	else
		CScrView::OnChar(nChar, nRepCnt, nFlags);
}

void CHexEditView::do_char(UINT nChar)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	// Test if edit key (not special key and not BS, ^\ or nul)
	// Note: nul byte is just ignored and ^\ is only used in debug version
	if (strchr("\b\034\0", nChar) == NULL)
	{
		unsigned char cc = '\0';
		num_del_ = num_bs_ = 0;         // We're not deleting

		// Warn of a number of different problems
		if (( display_.vert_display && row > 0 ||
			 !display_.vert_display && !display_.edit_char) &&
			!isxdigit(nChar))
		{
			// Non hex digit typed in hex area
#ifdef SYS_SOUNDS
			if (!CSystemSound::Play("Invalid Character"))
#endif
				::Beep(5000,200);
			CAvoidableDialog::Show(IDS_INVALID_HEX,
				"The only valid keys in the hex area are 0-9 and A-F.", "", 0, MAKEINTRESOURCE(IDI_CROSS));
			aa->mac_error_ = 5;
			return;
		}
		if (( display_.vert_display && row == 0 ||
			 !display_.vert_display && display_.edit_char) &&
			display_.char_set == CHARSET_EBCDIC && (nChar > 128 || a2e_tab[nChar] == 0))
		{
#ifdef SYS_SOUNDS
			if (!CSystemSound::Play("Invalid Character"))
#endif
				::Beep(5000,200);
			CAvoidableDialog::Show(IDS_INVALID_EBCDIC,
				"The key is not a valid EBCDIC character.");
			aa->mac_error_ = 10;
			return;
		}

		if (check_ro("edit it"))
			return;

		if (display_.overtype && start_addr != end_addr)
		{
			// xxx direct warning if GetDocument()->IsDevice()?
			if (CAvoidableDialog::Show(IDS_OVERTYPE,
				"This will delete the current selection which "
				"is not permitted in overtype mode.\n\n"
				"Do you want to turn off overtype mode?",
				"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
			{
				aa->mac_error_ = 10;
				return;
			}
			else if (!do_insert())
				return;
		}
		else if (display_.overtype && start_addr == GetDocument()->length())
		{
			// xxx direct warning if GetDocument()->IsDevice()?
			if (CAvoidableDialog::Show(IDS_OVERTYPE,
				"You can't extend this file while in overtype mode.\n\n"
				"Do you want to turn off overtype mode?",
				"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
			{
				aa->mac_error_ = 5;
				return;
			}
			else if (!do_insert())
				return;
		}

		if ((num_entered_ % 2) == 0)
			DisplayCaret();                 // Make sure start is visible

		// If there is a selection then delete it
		if (start_addr != end_addr)
		{
			ASSERT(start_addr >= 0 && end_addr <= GetDocument()->length() && start_addr < end_addr);
			GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr,
								  NULL, 0, this);
		}

		if ( display_.vert_display && row == 0 ||
			!display_.vert_display && display_.edit_char)
		{
			// In char area so insert/replace char at this curr locn
			if (display_.char_set == CHARSET_EBCDIC && nChar < 128)
				cc = a2e_tab[nChar];
			else if (display_.char_set == CHARSET_EBCDIC)
				cc = '\0';
			else
				cc = (unsigned char)nChar;
			GetDocument()->Change(
				display_.overtype ? mod_replace : mod_insert,
				start_addr, 1, &cc, num_entered_, this,
				start_addr != end_addr);
			// Move the caret to the next char
			++start_addr;
			num_entered_ += 2;  // One char == 2 nybbles
		}
		else if (display_.vert_display && row == 1)
		{
			ASSERT((num_entered_ % 2) == 0);
			// Convert to hex and store in top nybble
			cc = '\0';
			if (start_addr < GetDocument()->length())
			{
				// If not at end of doc then get nybble to shift
				size_t got = GetDocument()->GetData(&cc, 1, start_addr);
				ASSERT(got == 1);
			}
			cc = (cc & 0x0F) |
				((isdigit(nChar) ? nChar - '0' : toupper(nChar) - 'A' + 10) << 4);
			GetDocument()->Change(
				display_.overtype ? mod_replace : mod_insert,
				start_addr, 1, &cc, num_entered_, this,
				start_addr != end_addr);

			// Move to bottom nybble
			row = 2;
			++num_entered_;
		}
		else if (display_.vert_display && row == 2)
		{
			// Bottom nybble
			cc = '\0';
			if (start_addr < GetDocument()->length())
			{
				// If not at end of doc then get nybble to shift
				size_t got = GetDocument()->GetData(&cc, 1, start_addr);
				ASSERT(got == 1);
				// Check if started entering values on 2nd (bottom) nybble
				if (num_entered_ == 0)
				{
					// Fake entry of first (top nybble)
					GetDocument()->Change(
						display_.overtype ? mod_replace : mod_insert,
						start_addr, 1, &cc, num_entered_, this);
					++num_entered_;
				}

				ASSERT((num_entered_ % 2) != 0);
			}
			cc = (cc & 0xF0) |
				(isdigit(nChar) ? nChar - '0' : toupper(nChar) - 'A' + 10);
			GetDocument()->Change(
				display_.overtype ? mod_replace : mod_insert,
				start_addr, 1, &cc, num_entered_, this);

			// Move to first nybble of next byte
			start_addr++;
			row = 1;
			++num_entered_;
		}
		else if ((num_entered_ % 2) == 0)
		{
			//  First nybble entered - convert hex to low nybble
			cc = isdigit(nChar) ? nChar - '0' : toupper(nChar) - 'A' + 10;
			GetDocument()->Change(
				display_.overtype ? mod_replace : mod_insert,
				start_addr, 1, &cc, num_entered_, this,
				start_addr != end_addr);
			++num_entered_;
		}
		else
		{
			// 2nd nybble - shift over 1st nybble and add this nybble
			cc = '\0';
			if (start_addr < GetDocument()->length())
			{
				// If not at end of doc then get nybble to shift
				size_t got = GetDocument()->GetData(&cc, 1, start_addr);
				ASSERT(got == 1);
			}
			cc = (cc << 4) + 
				(isdigit(nChar) ? nChar - '0' : toupper(nChar) - 'A' + 10);
			GetDocument()->Change(
				display_.overtype ? mod_replace : mod_insert,
				start_addr, 1, &cc, num_entered_, this);
			// Move the caret to the next byte
			++start_addr;
			++num_entered_;
		}
		SetCaret(addr2pos(start_addr, row));
		show_prop();                            // Current char under caret may have changed so update prop dlg
		show_calc();
		show_pos();                             // Update tool bar
		if ((num_entered_ % 2) == 0)
			DisplayCaret();                     // Make sure end is visible
	}
#if 0
	else if (nChar == '\f')                             // ^L - centre caret and redraw
	{
		CRect cli;

		// Work out window height in logical units
		GetDisplayRect(&cli);
		CRectAp doc_rect = ConvertFromDP(cli);

		// Move display so that caret is centred vertically
		CPointAp pp = GetCaret();
		pp.x = 0;                       // Scroll to left side (but see DisplayCaret() below)
		if ((pp.y -= (doc_rect.bottom-doc_rect.top)/2) < 0)
			pp.y = 0;
		SetScroll(pp);

		DoInvalidate();                 // Also redraw display
		DisplayCaret();                 // Make sure caret not off right side
	}
	else if (nChar == '\t' && !display_.vert_display && display_.char_area && display_.hex_area)
	{
		// Allow tab between hex and char areas if they are both visible
		num_entered_ = num_del_ = num_bs_ = 0;  // Turn off any consec edits

		// TAB key swaps between hex and char areas
		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		undo_.push_back(view_undo(undo_state));
		undo_.back().disp_state = disp_state_;
		display_.edit_char = !display_.edit_char;
		if (end_base)
			SetSel(addr2pos(end_addr), addr2pos(start_addr), true);
		else
			SetSel(addr2pos(start_addr), addr2pos(end_addr));
		DisplayCaret();
	}
	else if (nChar == '\025')                   // ^U - scroll up
	{
		CPointAp pp = GetScroll();
		pp.y -= text_height_;
		SetScroll(pp);
	}
	else if (nChar == '\004')                   // ^D - scroll down
	{
		CPointAp pp = GetScroll();
		pp.y += text_height_;
		SetScroll(pp);
	}
	else if (nChar == '\r')
	{
		num_entered_ = num_del_ = num_bs_ = 0;  // Turn off any consec edits
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Check if offset has actually changed
		if (real_offset_ != (rowsize_ - start_addr%rowsize_)%rowsize_)
		{
			undo_.push_back(view_undo(undo_offset));
			undo_.back().rowsize = real_offset_;        // Save previous offset for undo
			real_offset_ = offset_ = (rowsize_ - start_addr%rowsize_)%rowsize_;
			SetSel(addr2pos(start_addr), addr2pos(end_addr));
			recalc_display();
			DoInvalidate();
		}
		else
		{
			((CMainFrame *)AfxGetMainWnd())->
				StatusBarText("Warning: offset not changed");
			aa->mac_error_ = 1;
		}
	}
#endif
	else if (nChar == '\b')                     // Back space
	{
		// Warn if view is read only
		if (check_ro("backspace in it"))
			return;

		if (start_addr != end_addr)
		{
			// There is a selection - back space just deletes it
			if (display_.overtype)
			{
				// xxx direct warning if GetDocument()->IsDevice()?
				if (CAvoidableDialog::Show(IDS_OVERTYPE,
					"This will delete the current selection which "
					"is not permitted in overtype mode.\n\n"
					"Do you want to turn off overtype mode?",
					"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
				{
					aa->mac_error_ = 5;
					return;
				}
				else if (!do_insert())
					return;
			}

			ASSERT(start_addr >= 0 && end_addr <= GetDocument()->length() && start_addr < end_addr);
			GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr,
								  NULL, 0, this);
		}
		else
		{
			// Back space deletes/replaces previous byte
			if ((num_entered_ % 2) != 0)        // Check if in middle of byte entry
				++start_addr;
			if (start_addr < 1)                 // We can't back space at the start of file
			{
				aa->mac_error_ = 2;
				return;
			}
			start_addr--;                       // Work out the address of the byte to delete
			if (!display_.overtype)
			{
				// Delete previous char
				GetDocument()->Change(mod_delback, start_addr, 1,
								  NULL, num_bs_, this);
				num_bs_ += 2;
			}
			else
			{
				// Replace previous char in OVR mode with space or nul byte
				unsigned char cc;

				if ( display_.vert_display && row == 0 || 
					!display_.vert_display && display_.edit_char)
				{
					// Use EBCDIC or ASCII space
					cc = display_.char_set == CHARSET_EBCDIC ? 0x40 : 0x20;
					GetDocument()->Change(mod_repback, start_addr, 1,
										  &cc, num_bs_, this);
				}
				else
				{
					ASSERT(display_.hex_area);
					cc = '\0';
					GetDocument()->Change(mod_repback, start_addr, 1,
										  &cc, num_bs_, this);
				}
				++num_bs_;
			}
		}
		num_del_ = num_entered_ = 0;                    // turn off any byte entry

		// Move the caret
		SetCaret(addr2pos(start_addr));
		show_prop();                            // New char - check props
		show_calc();
		show_pos();                             // Update tool bar
		DisplayCaret();                         // Make sure caret is visible
	}
#ifdef _DEBUG
	else if (nChar == '\034')
	{
		// Just delay for 3 seconds
		timer tt(true);
		// This loop is (hopefully) not optimised away in debug mode (optimisations off)
		while (tt.elapsed () < 2)
			;
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("Ctrl+\\");
	}
#endif
	aa->SaveToMacro(km_char, nChar);
}

void CHexEditView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= VK_PRIOR && nChar <= VK_DELETE)
	{
		// Cursor movement - reset consec. entered count
		num_entered_ = num_del_ = num_bs_ = 0;

		CPointAp start, end;
		if (GetSel(start, end))
			SetSel(end, start, true);
		else
			SetSel(start, end);     // Calls ValidateCaret - causing caret selection to be moved to valid locations
	}

	CScrView::OnKeyDown(nChar, nRepCnt, nFlags);

	if (nChar == VK_SHIFT && (nFlags & 0x4000) == 0)
	{
		// Key down and not autorepeat
		reset_tip();
		shift_moves_ = 0;
	}
	update_sel_tip();

	// Also make sure info tip is hidden
	if (nChar != VK_SHIFT)
	{
		tip_.Hide(0);  // hide immediately
		if (last_tip_addr_ != -1)
		{
			FILE_ADDRESS addr = last_tip_addr_;
			last_tip_addr_ = -1;
			invalidate_hex_addr_range(addr, addr+1);
		}
	}
#ifdef RULER_ADJUST
	ruler_tip_.Hide(0);
#endif
}

void CHexEditView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CScrView::OnKeyUp(nChar, nRepCnt, nFlags);

	if (nChar == VK_SHIFT)
	{
		update_sel_tip();       // Make sure window hidden when shift released

		if (theApp.highlight_ && shift_moves_)
		{
			FILE_ADDRESS start_addr, end_addr;
			GetSelAddr(start_addr, end_addr);

			add_highlight(start_addr, end_addr, TRUE);
		}
	}
}

void CHexEditView::OnKillFocus(CWnd* pNewWnd)
{
	if (pav_ != NULL && IsWindow(pav_->m_hWnd)) pav_->StopTimer();       // Turn off any animation in aerial view

	CScrView::OnKillFocus(pNewWnd);

	if (nav_moves_ < 0)
		return;

	// Save current address in case edit controls move caret
	GetSelAddr(saved_start_, saved_end_);

//    CScrView::OnKillFocus(pNewWnd);

	if (GetView() == NULL)
	{
		CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

		mm->m_wndProp.Update(NULL, -1);
	}

	// Invalidate the current selection so its drawn lighter in inactive window
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		++end_addr;   // if no selection invalidate current byte
	invalidate_hex_addr_range(start_addr, end_addr);
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing
}

void CHexEditView::OnSetFocus(CWnd* pOldWnd)
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	if (pav_ != NULL && IsWindow(pav_->m_hWnd)) pav_->StartTimer();       // Turn on any animation in aerial view

	CScrView::OnSetFocus(pOldWnd);

	if (nav_moves_ < 0)
		return;

#if 0 // Handled in CChildFrame::OnSetFocus now
	show_prop();
	show_calc();
	show_pos();
#endif
	move_dlgs();

	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		++end_addr;   // if no selection invalidate the current byte
	invalidate_hex_addr_range(start_addr, end_addr);

	// Save nav info in case we need to create a nav pt here
	if (theApp.navman_.LastView() != this && !theApp.navman_.InMove())
	{
		nav_start_  = start_addr;
		nav_end_    = end_addr;
		nav_scroll_ = (GetScroll().y/line_height_)*rowsize_ - offset_;
		theApp.navman_.Add("Change window", get_info(), this, nav_start_, nav_end_, nav_scroll_);
		nav_moves_ = 0;
	}

	// km_new now seems to get recorded before km_focus - so check if km_new_str is there
	// (which always follows km_new) and don't store km_focus
	if (!(theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_new_str) &&
		theApp.pview_ != this)
	{
		// We only want to store focus change in a macro, if the last CHexEditView that
		// got focus is different to this one.  This allows us to ignore focus changes
		// when other things (find dialog, toolbar buttons etc) temp. get focus.
		// Also this macro entry may be deleted if followed by km_new, km_open,
		// km_win_new, km_childsys (next/prev win or win close) or km_win_next
		// since these implicitly change the focus, and setting it explicitly before
		// to a certain window may ruin their effect.
		ASSERT(GetFrame() != NULL);
		CString ss;
		GetFrame()->GetWindowText(ss);
		if (ss.Right(2) == " *")
			ss = ss.Left(ss.GetLength() - 2);

		// Ignore setfocus before frame has name
		if (ss.GetLength() > 0)
			theApp.SaveToMacro(km_focus, ss);
	}
	theApp.pview_ = this;
}

void CHexEditView::OnDestroy()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing
	theApp.tip_offset_ += tip_.GetPos() - tip_last_locn_;  // in case moved but not redisplayed

	if (pav_ != NULL)
		GetDocument()->RemoveAerialView();
	if (pcv_ != NULL)
		GetDocument()->RemoveCompView();

	CScrView::OnDestroy();

	// If there are no more views active ...
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (GetView() == NULL)
	{
		// If this is the last window we need to make sure that all file
		// access buttons are disabled in the calculator (if visible)
		show_calc();
	}
}

// Point is in window coordinates
void CHexEditView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPointAp doc_pt = ConvertFromDP(point);

	if (( (display_.vert_display || display_.char_area) && doc_pt.x >= char_pos(rowsize_)+text_width_ ||
		 !(display_.vert_display || display_.char_area) && doc_pt.x >= hex_pos(rowsize_) )  )
	{
		// Don't do anything if off to right
		return;
	}

	if (point.y < bdr_top_)       // above top (ie in ruler)
	{
#ifdef RULER_ADJUST
		// Cancel any accidental adjustments
		adjusting_rowsize_ = adjusting_offset_ = adjusting_group_by_ = -1;
		// Ruler area - check for double click on any of the adjusters
		if (over_offset_adjuster(doc_pt))
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_OFFSET_HANDLE);
		else if (over_group_by_adjuster(doc_pt))
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_GROUP_BY_HANDLE);
		else if (over_rowsize_adjuster(doc_pt))
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_ROWSIZE_HANDLE);
		else
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_RULER);  // no handles clicked so just do ruler dclick
#endif
		return;
	}

	if (doc_pt.x < hex_pos(0))
		theApp.OnViewDoubleClick(this, IDR_CONTEXT_ADDRESS);
	else if (display_.vert_display)
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in char area
		FILE_ADDRESS addr = (doc_pt.y/line_height_)*rowsize_ - offset_ +
							pos_char(doc_pt.x);

		//if (addr >= start_addr && addr < end_addr)
		//    theApp.OnViewDoubleClick(this, IDR_CONTEXT_SELECTION);
		//else
		if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_BOOKMARKS);  // click on bookmark
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_HIGHLIGHT);  // click on highlight
		else if (pos2row(doc_pt) == 0)
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_CHAR); // click on top (char) row
		else
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_HEX);  // click on one of bottom 2 rows
	}
	else if (!display_.char_area || doc_pt.x < char_pos(0))
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in hex area
		FILE_ADDRESS addr = (doc_pt.y/text_height_)*rowsize_ - offset_ +
							pos_hex(doc_pt.x);

		//if (addr >= start_addr && addr < end_addr)
		//    theApp.OnViewDoubleClick(this, IDR_CONTEXT_SELECTION);
		//else
		if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_BOOKMARKS);
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_HIGHLIGHT);  // click on highlight
		else
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_HEX);
	}
	else
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in char area
		FILE_ADDRESS addr = (doc_pt.y/text_height_)*rowsize_ - offset_ +
							pos_char(doc_pt.x);

		//if (addr >= start_addr && addr < end_addr)
		//    theApp.OnViewDoubleClick(this, IDR_CONTEXT_SELECTION);
		//else
		if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_BOOKMARKS);
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_HIGHLIGHT);  // click on highlight
		else
			theApp.OnViewDoubleClick(this, IDR_CONTEXT_CHAR);
	}
}

#ifdef RULER_ADJUST
// Display a tip for an adjuster in the ruler
void CHexEditView::add_ruler_tip(CPoint pt, CString ss, COLORREF colour)
{
	// Change appearance
	ruler_tip_.SetWindowText(ss);
	ruler_tip_.SetBgCol(colour);

	// Change location and make sure it's visible
	CPoint tip_pt;
	tip_pt = pt + CSize(text_width_w_, text_height_/2);
	ClientToScreen(&tip_pt);
	ruler_tip_.Move(tip_pt, false);
	ruler_tip_.Show();
}

// Check if a mouse point is over an adjuster
bool CHexEditView::over_rowsize_adjuster(CPointAp pp)
{
	int xpos;

	if (!display_.vert_display && display_.hex_area)
	{
		xpos = char_pos(0) - text_width_w_/2;
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	else if (display_.vert_display || (display_.char_area && !display_.hex_area))
	{
		xpos = char_pos(rowsize_ - 1) + (3*text_width_w_)/2;
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	return false;
}

bool CHexEditView::over_offset_adjuster(CPointAp pp)
{
	int xpos;

	if (!display_.vert_display && display_.hex_area)
	{
		xpos = hex_pos(offset_);
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	if (display_.vert_display || display_.char_area && !display_.hex_area)
	{
		xpos = char_pos(offset_);
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	return false;
}

bool CHexEditView::over_group_by_adjuster(CPointAp pp)
{
	int xpos;

	if (display_.vert_display)
	{
		xpos = char_pos(group_by_) - text_width_w_/2;
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	else if (display_.hex_area)
	{
		xpos = hex_pos(group_by_) - text_width_;
		if (pp.x > xpos - 5 && pp.x < xpos + 5)
			return true;
	}
	return false;
}
#endif

void CHexEditView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPointAp pp = ConvertFromDP(point);          // Point in our coord system

	// Click to right of everything is ignored, but click on address area (left side)
	// is now allowed to allow double-click in address area event to be handled
	if ( (display_.vert_display || display_.char_area) && pp.x >= char_pos(rowsize_)+text_width_ ||
		!(display_.vert_display || display_.char_area) && pp.x >= hex_pos(rowsize_)  )
	{
		return;          // Do nothing if off to right
	}

	GetSelAddr(prev_start_, prev_end_);
	prev_row_ = 0;
	if (prev_start_ == prev_end_ && display_.vert_display)
		prev_row_ = pos2row(GetCaret());

	if (point.y < bdr_top_)       // above top (ie in ruler)
	{
#ifdef RULER_ADJUST
		// Ruler area - check if over any of the adjusters

		// Check if mouse down on row size adjuster
		adjusting_rowsize_ = -1;
		if (over_rowsize_adjuster(pp))
		{
			if (pcv_ != NULL)
				pcv_->begin_change();
			adjusting_rowsize_ = rowsize_;
			invalidate_adjuster(adjusting_rowsize_);
			SetCapture();
			return;
		}

		// Check if the offset adjuster has been clicked
		adjusting_offset_ = -1;
		if (over_offset_adjuster(pp))
		{
			if (pcv_ != NULL)
				pcv_->begin_change();
			adjusting_offset_ = real_offset_;
			invalidate_adjuster(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1);
			SetCapture();
			return;
		}

		adjusting_group_by_ = -1;
		if (over_group_by_adjuster(pp))
		{
			if (pcv_ != NULL)
				pcv_->begin_change();
			adjusting_group_by_ = group_by_;
			invalidate_adjuster(adjusting_group_by_);
			SetCapture();
			return;
		}
#endif
		return;         // Don't allow any other select for click in ruler
	}

	shift_down_ = shift_down();

	// Check if clicked on mark or a bookmark
	if (!shift_down_ && address_at(point) == mark_)
	{
		tip_.Hide(0);
		if (last_tip_addr_ != -1)
		{
			FILE_ADDRESS addr = last_tip_addr_;
			last_tip_addr_ = -1;
			invalidate_hex_addr_range(addr, addr+1);
		}
		drag_mark_ = true;
		drag_address_ = mark_;
		mouse_down_ = true;
		return;   // no selection wanted when dragging the mark
	}
	else if (!shift_down_ && (drag_bookmark_ = bookmark_at(address_at(point))) > -1)
	{
		tip_.Hide(0);
		if (last_tip_addr_ != -1)
		{
			FILE_ADDRESS addr = last_tip_addr_;
			last_tip_addr_ = -1;
			invalidate_hex_addr_range(addr, addr+1);
		}
		drag_address_ = GetDocument()->bm_posn_[drag_bookmark_];  // start at current bookmark addr
		mouse_down_ = true;
		return;   // no selection wanted when dragging a bookmark
	}

	saved_state_ = disp_state_;                 // Keep the current state for saving in undo stack

	// Save some info that may be needed for macro recording
	dev_down_ = point;                          // Mouse down posn (device coords)
	doc_down_ = pp;                             // Mouse down posn (doc coords)

	FILE_ADDRESS swap_addr = -1;                // caret addr before swap (or -1 if no swap)
	if (!shift_down_)
	{
		num_entered_ = num_del_ = num_bs_ = 0;  // Can't be editing while mousing
		if (!display_.vert_display && display_.char_area && display_.hex_area)
		{
			// Allow user to move caret between hex and char areas

			// Which area did the user click in?
			// Note display_.edit_char must be set before ValidateCaret() is called while the
			// mouse button is held down so that the dragged selection is drawn correctly.
			display_.edit_char = pos_hex(pp.x, FALSE) >= rowsize_;

			if (saved_state_ != disp_state_)
			{
				if (display_.edit_char)
					SetHorzBufferZone(1);
				else
					SetHorzBufferZone(2);

				swap_addr = prev_start_;
			}
		}

	}

	CScrView::OnLButtonDown(nFlags, point);

	if (theApp.hl_caret_ && swap_addr > -1)
		invalidate_ruler(swap_addr);
	reset_tip();
	show_prop();
	show_calc();
	show_pos();
	mouse_down_ = true;                         // We saw left button down event
}

void CHexEditView::OnLButtonUp(UINT nFlags, CPoint point)
{
#ifdef RULER_ADJUST
	if (adjusting_rowsize_ > -1)
	{
		if (adjusting_rowsize_ != rowsize_)
		{
			FILE_ADDRESS scroll_addr = pos2addr(GetScroll());

			undo_.push_back(view_undo(undo_rowsize));
			undo_.back().rowsize = rowsize_;
			if (display_.autofit)
			{
				undo_.push_back(view_undo(undo_autofit, TRUE));
				display_.autofit = 0;
			}
			rowsize_ = adjusting_rowsize_;
			recalc_display();

			// Fix scroll place so it's about the same even though the row length has changed
			CPointAp pt = addr2pos(scroll_addr);
			pt.x = 0;
			SetScroll(pt);

			DoInvalidate();
		}

		ReleaseCapture();
		adjusting_rowsize_ = -1;
		SetSel(addr2pos(prev_start_), addr2pos(prev_end_));
		return;
	}

	if (adjusting_offset_ > -1)
	{
		if (adjusting_offset_ != real_offset_)
		{
			undo_.push_back(view_undo(undo_offset));
			undo_.back().rowsize = real_offset_;        // Save previous offset for undo
			real_offset_ = adjusting_offset_;
			recalc_display();

			DoInvalidate();
		}

		ReleaseCapture();
		adjusting_offset_ = -1;
		SetSel(addr2pos(prev_start_), addr2pos(prev_end_));
		return;
	}

	if (adjusting_group_by_ > -1)
	{
		if (adjusting_group_by_ != group_by_)
		{
			undo_.push_back(view_undo(undo_group_by));
			undo_.back().rowsize = group_by_;              // Save previous group_by for undo
			group_by_ = adjusting_group_by_;
			recalc_display();

			DoInvalidate();
		}

		ReleaseCapture();
		adjusting_group_by_ = -1;
		SetSel(addr2pos(prev_start_), addr2pos(prev_end_));
		return;
	}
#endif

	if (mouse_down_ && drag_mark_)
	{
		//FILE_ADDRESS prev = mark_;

		if (drag_address_ == mark_)
			MoveToAddress(mark_);                     // Click (no drag) on mark - just move to address clicked on
		else
			SetMark(drag_address_);

		drag_mark_ = false;
		mouse_down_ = false;
		SendMessage(WM_SETCURSOR, WPARAM(m_hWnd), MAKELPARAM(HTCLIENT, 0));
	}
	else if (mouse_down_ && drag_bookmark_ > -1)
	{
		FILE_ADDRESS prev = GetDocument()->bm_posn_[drag_bookmark_];  // Previous bm position

		if (drag_bookmark_ == bookmark_at(drag_address_))
		{
			// Click on bookmark (or drag back to original location) - just set cursor address
			MoveToAddress(drag_address_);
		}
		else
		{
			// Move the bookmark to drag_address_
			CBookmarkList *pbl = theApp.GetBookmarkList();
			ASSERT(pbl != NULL);
			pbl->Move(GetDocument()->bm_index_[drag_bookmark_], int(drag_address_ - prev)); // move bm in global list
			GetDocument()->bm_posn_[drag_bookmark_] = drag_address_;   // move in doc's bm list
			((CMainFrame *)AfxGetMainWnd())->m_wndBookmarks.UpdateBookmark(GetDocument()->bm_index_[drag_bookmark_]);

			invalidate_addr_range(prev, prev+1);                     // force redraw to remove bookmark at old position
			invalidate_addr_range(drag_address_, drag_address_+1);   // redraw bookmark at new position
		}

		drag_bookmark_ = -1;  // signal that we are no longer dragging
		mouse_down_ = false;
		SendMessage(WM_SETCURSOR, WPARAM(m_hWnd), MAKELPARAM(HTCLIENT, 0));
	}
	else if (mouse_down_)
	{
		num_entered_ = num_del_ = num_bs_ = 0;  // Can't be editing while mousing

		CScrView::OnLButtonUp(nFlags, point);
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Save in undo array if position moved
		if (prev_start_ != start_addr || prev_end_ != end_addr || saved_state_ != disp_state_)
		{
			// Save the caret position (or start of selection)
			undo_.push_back(view_undo(undo_move));
			undo_.back().address = prev_start_ | (FILE_ADDRESS(prev_row_)<<62);
			if (prev_start_ != prev_end_)
			{
				// Save the previous selection
				undo_.push_back(view_undo(undo_sel));
				undo_.back().address = prev_end_;
			}
			if (saved_state_ != disp_state_)
			{
				// Save the fact that we swapped between areas
				undo_.push_back(view_undo(undo_state, TRUE));
				undo_.back().disp_state = saved_state_;
			}

			// Save info to keystroke macro (if recording)
			CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
			if (aa->recording_)
			{
				mouse_sel ms;                   // Info for km_mouse/km_shift_mouse
				ms.dev_down = dev_down_;        // Point in window where selection started
				CPointAp doc_up = ConvertFromDP(point); // Point where selection ended (doc coords)

//              ms.doc_dist = doc_up - doc_down_;
				// To avoid problems where a different number of rows are selected on play
				// for slightly different scroll positions we round up to mult. of text_height_.
				ms.doc_dist.cy = (doc_up.y/text_height_ - doc_down_.y/text_height_) * text_height_;
//                ms.doc_dist.cx = (doc_up.x/text_width_ - doc_down_.x/text_width_) * text_width_;
				if (shift_down_)
					aa->SaveToMacro(km_shift_mouse, &ms);
				else
					aa->SaveToMacro(km_mouse, &ms);
			}

			if (aa->highlight_)
				add_highlight(start_addr, end_addr, TRUE);

			nav_save(start_addr, end_addr, "Mouse Click ");
		}

		show_prop();
		show_calc();
		show_pos();
		mouse_down_ = false;
		update_sel_tip();           // Make sure window hidden when mouse button up
		if (theApp.hl_caret_)
		{
			invalidate_addr(start_addr);
			invalidate_ruler(start_addr);
		}
	}
}

void CHexEditView::OnMouseMove(UINT nFlags, CPoint point)
{
#ifdef RULER_ADJUST
	CPointAp doc_pt = ConvertFromDP(point);
	if (adjusting_rowsize_ > -1)        // Dragging row size adjuster?
	{
		CString ss;
		int old = adjusting_rowsize_;    // save current so we can "undraw" it

		if (!display_.vert_display && display_.hex_area)
		{
			if (doc_pt.x < hex_pos(4))
				adjusting_rowsize_ = 4;
			else
				adjusting_rowsize_ = pos_hex(doc_pt.x + text_width_, -1);
		}
		else if (display_.vert_display || display_.char_area)
		{
			if (doc_pt.x < char_pos(4))
				adjusting_rowsize_ = 4;
			else
				adjusting_rowsize_ = pos_char(doc_pt.x + text_width_w_/2, -1);
		}
		if (adjusting_rowsize_ != old)
		{
			invalidate_adjuster(old);
			invalidate_adjuster(adjusting_rowsize_);
		}
		ss.Format("Columns: %d", adjusting_rowsize_);
		add_ruler_tip(point, ss,
			adjusting_rowsize_ != rowsize_ ? RGB(224,255,224) : ::GetSysColor(COLOR_INFOBK));
		return;
	}
	if (adjusting_offset_ > -1)         // Dragging offset adjuster?
	{
		CString ss;
		int old = adjusting_offset_;    // save current so we can "undraw" it

		// Work out new offset from current dragged mouse posn
		if (display_.vert_display)
		{
			int left_side = char_pos(0);
			if (doc_pt.x >= char_pos(rowsize_))
				adjusting_offset_ = 0;    // Use 0 if dragged too far right
			else if (doc_pt.x >= char_pos(1))
				adjusting_offset_ = rowsize_ - pos_char(doc_pt.x + text_width_w_/2, TRUE);
			else
				adjusting_offset_ = (left_side - doc_pt.x)/text_width_;
		}
		else if (display_.char_area && display_.hex_area && doc_pt.x < char_pos(0))
		{
			int left_side = hex_pos(0);
			if (doc_pt.x >= hex_pos(rowsize_))
				adjusting_offset_ = 0;
			else if (doc_pt.x >= hex_pos(1))
				adjusting_offset_ = rowsize_ - pos_hex(doc_pt.x + text_width_, TRUE);
			else // drag to left of column zero is allowed
				adjusting_offset_ = (left_side - doc_pt.x)/text_width_;
		}
		else if (display_.char_area)
		{
			int left_side = char_pos(0);
			if (doc_pt.x >= char_pos(rowsize_))
				adjusting_offset_ = 0;
			else if (doc_pt.x >= char_pos(1))
				adjusting_offset_ = rowsize_ - pos_char(doc_pt.x + text_width_w_/2, TRUE);
			else
				adjusting_offset_ = (left_side - doc_pt.x)/text_width_;
		}
		else
		{
			int left_side = hex_pos(0);
			if (doc_pt.x >= hex_pos(rowsize_))
				adjusting_offset_ = 0;
			else if (doc_pt.x >= hex_pos(1))
				adjusting_offset_ = rowsize_ - pos_hex(doc_pt.x + text_width_, TRUE);
			else
				adjusting_offset_ = (left_side - doc_pt.x)/text_width_;
		}
		if (adjusting_offset_ < 0)
			adjusting_offset_ = 0;
		if (adjusting_offset_ != old)
		{
			invalidate_adjuster(rowsize_ - (rowsize_ + old - 1) % rowsize_ - 1);
			invalidate_adjuster(rowsize_ - (rowsize_ + adjusting_offset_ - 1) % rowsize_ - 1);
		}
		ss.Format("Offset: %d", adjusting_offset_);
		add_ruler_tip(point, ss,
			adjusting_offset_ != real_offset_ ? RGB(224,255,224) : ::GetSysColor(COLOR_INFOBK));
		return;
	}

	if (adjusting_group_by_ > -1)       // Dragging grouping adjuster?
	{
		CString ss;
		int old = adjusting_group_by_;    // save current so we can "undraw" it

		// Work out new group_by_ from current dragged mouse posn
		if (display_.vert_display)
		{
			if (doc_pt.x < char_pos(2))
				adjusting_group_by_ = 2;        // Minimum
			//else if (doc_pt.x >= char_pos(rowsize_))
			//    adjusting_group_by_ = 9999;
			else
				adjusting_group_by_ = pos_char(doc_pt.x + text_width_w_/2, -1);
		}
		else if (display_.hex_area)
		{
			if (doc_pt.x < hex_pos(2))
				adjusting_group_by_ = 2;
			//else if (doc_pt.x >= hex_pos(rowsize_))
			//    adjusting_group_by_ = 9999;
			else
				adjusting_group_by_ = pos_hex(doc_pt.x + text_width_, -1);
		}
		if (adjusting_group_by_ != old)
		{
			invalidate_adjuster(old);
			invalidate_adjuster(adjusting_group_by_);
		}
		ss.Format("Grouping: %d", adjusting_group_by_);
		add_ruler_tip(point, ss, 
			adjusting_group_by_ != group_by_ ? RGB(224,255,224) : ::GetSysColor(COLOR_INFOBK));
		return;
	}

	// Now check if we are hovering over a ruler handle.
	if (point.y < bdr_top_ &&
		(over_rowsize_adjuster(doc_pt) || 
		 over_offset_adjuster(doc_pt) || 
		 over_group_by_adjuster(doc_pt)) )
	{
		track_mouse(TME_HOVER);
		return;
	}
	if (!ruler_tip_.FadingOut())
		ruler_tip_.Hide(300);       // Not dragging or hovering over so hide it
#endif

	FILE_ADDRESS addr = address_at(point);      // Address of byte mouse is over (or -1)

	// If dragging a bookmark update the drag location
	if (mouse_down_ && (drag_mark_ || drag_bookmark_ > -1))
	{
		if (addr < 0 || addr > GetDocument()->length())
		{
			CScrView::OnMouseMove(nFlags, point);
			return; // don't move to invalid pos
		}
		FILE_ADDRESS prev = drag_address_;
		drag_address_ = addr;                                  // we must change this before redraw
		invalidate_addr_range(prev, prev+1);                   // force redraw at previous drag position
		invalidate_addr_range(drag_address_, drag_address_+1); // redraw at new position
		return;
	}

	FILE_ADDRESS old_addr, end_addr, new_addr;
	if (mouse_down_)
		GetSelAddr(old_addr, end_addr);             // Get current caret before moved
	CScrView::OnMouseMove(nFlags, point);
	if (mouse_down_)
		GetSelAddr(new_addr, end_addr);

	if (mouse_down_)
	{
		tip_.Hide(0);                           // Make sure there is no mouse tip while dragging
		move_dlgs();
		update_sel_tip(200);                    // Update selection tip
	}
	else if (!tip_.IsWindowVisible())
	{
		// Set a timer so that we can check if we want to display a tip window
		track_mouse(TME_HOVER);
		if (last_tip_addr_ != -1)
		{
			// Hide the box around the tip byte
			FILE_ADDRESS addr = last_tip_addr_;
			last_tip_addr_ = -1;
			invalidate_hex_addr_range(addr, addr+1);
		}
	}
	else if (addr != last_tip_addr_ && tip_.IsWindowVisible())
	{
		// Hide the tip window since the mouse has moved away
		if (!tip_.FadingOut())                  // Don't keep it alive if we have already told it to fade away
			tip_.Hide(300);
	}
	if (theApp.hl_mouse_)
		set_mouse_addr(addr);
}

/*
void CHexEditView::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == timer_id_)
	{
		VERIFY(KillTimer(timer_id_));
		timer_id_ = 0;

		CPoint pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		if (pt != last_mouse_)
			return;

		// Time to show a tip window if we are in the right place
		FILE_ADDRESS addr = address_at(pt);
		int bm;
		if (addr != -1 && (bm = bookmark_at(addr)) != -1)
		{
			last_tip_addr_ = addr;
			CBookmarkList *pbl = theApp.GetBookmarkList();
			tip_.SetWindowText(pbl->name_[GetDocument()->bm_index_[bm]]);
			CPoint point = last_mouse_ + CSize(text_width_w_, text_height_);
			ClientToScreen(&point);
			tip_.Move(point, false);
			tip_.Show();
		}
	}
	else
		CScrView::OnTimer(nIDEvent);
}
*/

LRESULT CHexEditView::OnMouseHover(WPARAM, LPARAM lp)
{
	CPoint pt(LOWORD(lp), HIWORD(lp));  // client window coords
	// Time to show a tip window if we are in the right place
	if (!mouse_down_ && !tip_.IsWindowVisible())
	{
		FILE_ADDRESS addr = address_at(pt);
		if (addr != -1 && addr <= GetDocument()->length() && update_tip(addr))
		{
			// Save which byte the tip is for and invalidate it so OnDraw can do it's thing
			last_tip_addr_ = addr;
			invalidate_hex_addr_range(addr, addr+1);

			// Work out the how far the user dragged the tip window last time it was visible
			theApp.tip_offset_ += tip_.GetPos() - tip_last_locn_;
			// But make sure it is not too far away
			if (theApp.tip_offset_.cx < -200)
				theApp.tip_offset_.cx = -200;
			else if (theApp.tip_offset_.cx > 200)
				theApp.tip_offset_.cx = 200;
			if (theApp.tip_offset_.cy < -100)
				theApp.tip_offset_.cy = -100;
			else if (theApp.tip_offset_.cy > 100)
				theApp.tip_offset_.cy = 100;

			// Work out the window display position and move the tip window there
			CPoint tip_pt = pt + theApp.tip_offset_;
			ClientToScreen(&tip_pt);
			tip_.Move(tip_pt, false);

			tip_last_locn_ = tip_.GetPos();   // remember where we left it
			tip_.Show();
			track_mouse(TME_LEAVE);
			return 0;
		}
	}
#ifdef RULER_ADJUST
	if (pt.y < bdr_top_ && !ruler_tip_.IsWindowVisible())
	{
		CPointAp pp = ConvertFromDP(pt);        // Point in our coord system
		CString ss;

		if (over_rowsize_adjuster(pp))
			ss.Format("Columns: %d%s", rowsize_, display_.autofit ? " (AutoFit)" : "");
		else if (over_offset_adjuster(pp))
			ss.Format("Offset: %d", real_offset_);
		else if (over_group_by_adjuster(pp))
			ss.Format("Grouping: %d", group_by_);

		if (!ss.IsEmpty())
		{
			add_ruler_tip(pt, ss, ::GetSysColor(COLOR_INFOBK));
			track_mouse(TME_LEAVE);
			return 0;
		}
	}
#endif

	return 1;
}

// Returns true if tip text was updated or false if there is nothing to show.
// The parameter (addr) if the address about which we show information
bool CHexEditView::update_tip(FILE_ADDRESS addr)
{
	size_t ii;
	tip_addr_ = addr;

	ASSERT(theApp.tip_name_.size() > 0); // we should at least have bookmarks
	ASSERT(theApp.tip_on_    .size() == theApp.tip_name_.size());
	ASSERT(theApp.tip_expr_  .size() == theApp.tip_name_.size());
	ASSERT(theApp.tip_format_.size() == theApp.tip_name_.size());

	tip_.Clear();
	// Do desciptions
	for (ii = 0; ii < theApp.tip_name_.size(); ++ii)
	{
		if (theApp.tip_on_[ii])
		{
			if (ii == 0)
			{
				// Special (hard-coded) bookmark tip
				ASSERT(theApp.tip_name_[ii] == "Bookmarks");
				if (bookmark_at(addr) != -1)
					tip_.AddString("Bookmark: ");
			}
			// Add any more "hard-coded" tip types here ...

			else
			{
				ASSERT(ii >= FIRST_USER_TIP);
				tip_.AddString(theApp.tip_name_[ii] + ": ");
			}
		}
	}

	CPoint pt(0,0);
	CRect rct = tip_.GetTotalRect();
	pt.x = rct.right;

	int idx = 0;
	for (ii = 0; ii < theApp.tip_name_.size(); ++ii)
	{
		if (theApp.tip_on_[ii])
		{
			if (ii == 0)
			{
				// Special (hard-coded) bookmark tip
				ASSERT(theApp.tip_name_[ii] == "Bookmarks");

				int bm;              // Index into doc's bookmark list of bookmark under cursor
				if ((bm = bookmark_at(addr)) != -1)
				{
					rct = tip_.GetRect(idx);
					pt.y = rct.top;
					tip_.AddString(theApp.GetBookmarkList()->name_[GetDocument()->bm_index_[bm]], -1, &pt);
					++idx;
				}
			}
			// Add any more "hard-coded" tip types here ...

			else
			{
				COLORREF col = -1;
				CString format = theApp.tip_format_[ii];

				// Work out the colour of the text
				if (theApp.tip_expr_[ii].Find("address") != -1 ||
					theApp.tip_expr_[ii].Find("cursor") != -1 ||
					theApp.tip_expr_[ii].Find("sel_len") != -1 ||
					theApp.tip_expr_[ii].Find("mark") != -1 ||
					theApp.tip_expr_[ii].Find("eof") != -1)
				{
					// If no format given display as current address format
					if (format.IsEmpty())
						format = DecAddresses() ? "dec" : "hex";

					// Display addresses in the appropriate colour
					if (format.Left(3).CompareNoCase("hex") == 0 ||
						format.Find('x') != -1 ||
						format.Find('X') != -1)
					{
						col = GetHexAddrCol();  // display in hex address colour
					}
					else if (format.Left(3).CompareNoCase("dec") == 0 ||
							 format.Find('u') != -1 ||
							 format.Find('d') != -1)
					{
						col = GetDecAddrCol();   // display in decimal address colour
					}
					// else other int formats like octal, or non-int format such as:
					// octal, %o, bin, false (boolean), no, off, %s (string) etc
				}

				// Work out where to put it
				rct = tip_.GetRect(idx);
				pt.y = rct.top;

				int dummy;
				expr_eval::value_t val = expr_.evaluate(theApp.tip_expr_[ii], 0, dummy);
				tip_.AddString(val.GetDataString(format, expr_.GetSize(), expr_.GetUnsigned()),
							   col,
							   &pt);
				++idx;
			}
		}
	}
	ASSERT(ii > 0); // This is mainly here for easier stepping in the debugger
	ASSERT(tip_.Count() == idx*2);  // should have added 2 entries for every tip

	tip_.SetAlpha(theApp.tip_transparency_);
	return idx > 0;
}

LRESULT CHexEditView::OnMouseLeave(WPARAM, LPARAM lp)
{
	tip_.Hide(300);
#ifdef RULER_ADJUST
	ruler_tip_.Hide(300);
#endif
	if (theApp.hl_mouse_)
		set_mouse_addr(-1);
	return 0;
}

void CHexEditView::OnSysColorChange()
{
	CScrView::OnSysColorChange();
	set_colours();
	Invalidate();
}

FILE_ADDRESS CHexEditView::address_at(CPoint pt)
{
	if (pt.y < bdr_top_)
		return -1;                           // Above top border (ie in ruler)

	CPointAp doc_pt = ConvertFromDP(pt);     // Mouse position (doc coords)

	if (doc_pt.x < hex_pos(0) || doc_pt.x > char_pos(rowsize_)+text_width_)
		return -1;                           // Left of hex area (ie in address area)
	else if ( (display_.vert_display || display_.char_area) && doc_pt.x >= char_pos(rowsize_)+text_width_ ||
			 !(display_.vert_display || display_.char_area) && doc_pt.x >= hex_pos(rowsize_) )
		return -1;                           // Right of areas (ie in blank area)
	else if (display_.vert_display)
		return (doc_pt.y/line_height_)*rowsize_ - offset_ + pos_char(doc_pt.x, TRUE);
	else if (!display_.char_area || doc_pt.x < char_pos(0))
		return (doc_pt.y/text_height_)*rowsize_ - offset_ + pos_hex(doc_pt.x, TRUE);
	else
		return (doc_pt.y/text_height_)*rowsize_ - offset_ + pos_char(doc_pt.x, TRUE);
}

// Returns the bookmark at file address or -1 if none
int CHexEditView::bookmark_at(FILE_ADDRESS addr)
{
	std::vector<FILE_ADDRESS>::const_iterator pp =
		std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr);
	if (pp != GetDocument()->bm_posn_.end())
		return pp - GetDocument()->bm_posn_.begin();
	else
		return -1;
}

void CHexEditView::OnHighlight()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);

	if (aa->highlight_)
		aa->highlight_ = FALSE;
	else if (start_addr != end_addr)
		add_highlight(start_addr, end_addr, FALSE);
	else
		aa->highlight_ = TRUE;
	aa->SaveToMacro(km_highlight, (unsigned __int64)1);
}

void CHexEditView::OnUpdateHighlight(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(dynamic_cast<CHexEditApp *>(AfxGetApp())->highlight_);
}

void CHexEditView::OnHighlightClear()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	aa->highlight_ = FALSE;
	if (!hl_set_.empty())
	{
		undo_.push_back(view_undo(undo_highlight));
		undo_.back().phl = new range_set<FILE_ADDRESS>(hl_set_);
		hl_set_.clear();
		DoInvalidate();
	}
	aa->SaveToMacro(km_highlight);
}

// Select the current highlight.
// This is designed for associating with double-click on a highlight event.
void CHexEditView::OnHighlightSelect()
{
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);
	if (hl_set_.range_.empty())
	{
		TaskMessageBox("Nothing selected",
			"There is nothing highlighted in this file.  "
			"The closest highlight could not be selected.");
		theApp.mac_error_ = 10;
		return;
	}
	else
	{
		range_set<FILE_ADDRESS>::range_t::const_iterator pp =
			lower_bound(hl_set_.range_.begin(),
						hl_set_.range_.end(),
						range_set<FILE_ADDRESS>::segment(start_addr+1, start_addr+1),
						segment_compare());
		if (pp != hl_set_.range_.begin())
			pp--;
		MoveWithDesc("Select Highlight ", pp->sfirst, pp->slast);
		theApp. SaveToMacro(km_highlight, (unsigned __int64)5);
	}
}

void CHexEditView::OnHighlightPrev()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);
	if (hl_set_.range_.empty())
	{
		TaskMessageBox("Nothing selected",
			"There is nothing highlighted in this file.  "
			"The previous highlight could not be selected.");
		aa->mac_error_ = 10;
		return;
	}
	else if (hl_set_.range_.front().sfirst >= start_addr)
	{
		TaskMessageBox("Nothing selected",
			"There is no previous highlight.");
		aa->mac_error_ = 10;
		return;
	}
	else
	{
		range_set<FILE_ADDRESS>::range_t::const_iterator pp =
			lower_bound(hl_set_.range_.begin(),
						hl_set_.range_.end(),
						range_set<FILE_ADDRESS>::segment(start_addr, end_addr),
						segment_compare());
		ASSERT(pp != hl_set_.range_.begin());
		pp--;           // Get the previous elt
		MoveWithDesc("Previous Highlight ", pp->sfirst, pp->slast);
		aa->SaveToMacro(km_highlight, (unsigned __int64)2);
	}
}

void CHexEditView::OnUpdateHighlightPrev(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	pCmdUI->Enable(!hl_set_.range_.empty() && hl_set_.range_.front().sfirst < start_addr);
}

void CHexEditView::OnHighlightNext()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);
	if (hl_set_.range_.empty())
	{
		TaskMessageBox("Nothing selected",
			"There is nothing highlighted in this file.  "
			"The next highlight could not be selected.");
		aa->mac_error_ = 10;
		return;
	}
	else if (hl_set_.range_.back().sfirst <= start_addr)
	{
		TaskMessageBox("Nothing selected",
			"There is no next highlight.");
		aa->mac_error_ = 10;
		return;
	}
	else
	{
		range_set<FILE_ADDRESS>::range_t::const_iterator pp =
			lower_bound(hl_set_.range_.begin(),
						hl_set_.range_.end(),
						range_set<FILE_ADDRESS>::segment(start_addr+1, end_addr+1),
						segment_compare());
		ASSERT(pp != hl_set_.range_.end());
		MoveWithDesc("Next Highlight ", pp->sfirst, pp->slast);
		aa->SaveToMacro(km_highlight, (unsigned __int64)3);
	}
}

void CHexEditView::OnUpdateHighlightNext(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	pCmdUI->Enable(!hl_set_.range_.empty() && hl_set_.range_.back().sfirst > start_addr);
}

void CHexEditView::add_highlight(FILE_ADDRESS start, FILE_ADDRESS end, BOOL ptoo /*=FALSE*/)
{
	if (start < end)
	{
		undo_.push_back(view_undo(undo_highlight, ptoo));
		undo_.back().phl = new range_set<FILE_ADDRESS>(hl_set_);

		range_set<FILE_ADDRESS> tt = hl_set_;
		tt.insert_range(start, end);
		// If selected area is already part of highlighted area
		if (tt == hl_set_)
			hl_set_.erase_range(start, end);    // Remove it from highlight
		else
			hl_set_.swap(tt);                   // else add it to highlight
		invalidate_addr_range(start, end);
	}

#ifdef _DEBUG
	for (range_set<FILE_ADDRESS>::range_t::iterator pr = hl_set_.range_.begin(); pr != hl_set_.range_.end(); ++pr)
	{
		FILE_ADDRESS fa = pr->sfirst;
		fa = pr->slast;
	}
#endif
}

void CHexEditView::OnHighlightHide()
{
	begin_change();
	display_.hide_highlight = !display_.hide_highlight;
	make_change();
	end_change();
	theApp.SaveToMacro(km_highlight, (unsigned __int64)4);
}

void CHexEditView::OnUpdateHighlightHide(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(display_.hide_highlight);
}

// We will try returning the bookmark above or -1 if there is none
int CHexEditView::ClosestBookmark(FILE_ADDRESS &diff)
{
	std::vector<FILE_ADDRESS>::const_iterator prev_bm = GetDocument()->bm_posn_.end();
	diff = GetDocument()->length()+1;  // Bigger than any possible difference between current & bookmark
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);

	// Find the bookmark that is closest but after the current address
	for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
		 pbm != GetDocument()->bm_posn_.end(); ++pbm)
	{
		if (*pbm <= start_addr && start_addr - *pbm < diff)
		{
			diff = start_addr - *pbm;
			prev_bm = pbm;
		}
	}
	if (prev_bm == GetDocument()->bm_posn_.end())
		return -1;
	else
		return GetDocument()->bm_index_[prev_bm - GetDocument()->bm_posn_.begin()];
}

void CHexEditView::OnBookmarksClear()
{
	GetDocument()->ClearBookmarks();
	theApp.SaveToMacro(km_bookmarks);
}

void CHexEditView::OnUpdateBookmarksClear(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->bm_posn_.empty());
}

void CHexEditView::OnBookmarksPrev()
{
	std::vector<FILE_ADDRESS>::const_iterator prev_bm = GetDocument()->bm_posn_.end();
	FILE_ADDRESS diff = GetDocument()->length()+1;  // Bigger than any possible difference between current & bookmark
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);

	// Find the bookmark that is closest but after the current address
	for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
		 pbm != GetDocument()->bm_posn_.end(); ++pbm)
	{
		if (*pbm < start_addr && start_addr - *pbm < diff)
		{
			diff = start_addr - *pbm;
			prev_bm = pbm;
		}
	}
	if (prev_bm == GetDocument()->bm_posn_.end())
	{
		// No bookmarks found (presumably in macro playback)
		TaskMessageBox("No Previous Bookmark", "There is no previous bookmark at this position.");
		theApp.mac_error_ = 10;
		return;
	}

	MoveWithDesc("Previous Bookmark ", *prev_bm, *prev_bm);
	theApp.SaveToMacro(km_bookmarks, (unsigned __int64)2);
}

void CHexEditView::OnUpdateBookmarksPrev(CCmdUI* pCmdUI)
{
	std::vector<FILE_ADDRESS>::const_iterator prev_bm = GetDocument()->bm_posn_.end();
	FILE_ADDRESS diff = GetDocument()->length()+1;  // Bigger than any possible difference between current & bookmark
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);

	// Find the bookmark that is closest but after the current address
	for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
		 pbm != GetDocument()->bm_posn_.end(); ++pbm)
	{
		if (*pbm < start_addr && start_addr - *pbm < diff)
		{
			diff = start_addr - *pbm;
			prev_bm = pbm;
		}
	}
	pCmdUI->Enable(prev_bm != GetDocument()->bm_posn_.end());
}

void CHexEditView::OnBookmarksNext()
{
	std::vector<FILE_ADDRESS>::const_iterator next_bm = GetDocument()->bm_posn_.end();
	FILE_ADDRESS diff = GetDocument()->length()+1;  // Bigger than any possible difference between current & bookmark
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);

	// Find the bookmark that is closest but after the current address
	for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
		 pbm != GetDocument()->bm_posn_.end(); ++pbm)
	{
		if (*pbm > start_addr && *pbm - start_addr < diff)
		{
			diff = *pbm - start_addr;
			next_bm = pbm;
		}
	}
	if (next_bm == GetDocument()->bm_posn_.end())
	{
		// No bookmarks found (presumably in macro playback)
		TaskMessageBox("No Next Bookmark", "There are no bookmarks after the current address.");
		theApp.mac_error_ = 10;
		return;
	}

	MoveWithDesc("Next Bookmark ", *next_bm, *next_bm);
	theApp.SaveToMacro(km_bookmarks, (unsigned __int64)3);
}

void CHexEditView::OnUpdateBookmarksNext(CCmdUI* pCmdUI)
{
	std::vector<FILE_ADDRESS>::const_iterator next_bm = GetDocument()->bm_posn_.end();
	FILE_ADDRESS diff = GetDocument()->length()+1;  // Bigger than any possible difference between current & bookmark
	FILE_ADDRESS start_addr, end_addr;

	GetSelAddr(start_addr, end_addr);

	// Find the bookmark that is closest but after the current address
	for (std::vector<FILE_ADDRESS>::const_iterator pbm = GetDocument()->bm_posn_.begin();
		 pbm != GetDocument()->bm_posn_.end(); ++pbm)
	{
		if (*pbm > end_addr && *pbm - end_addr < diff)
		{
			diff = *pbm - end_addr;
			next_bm = pbm;
		}
	}
	pCmdUI->Enable(next_bm != GetDocument()->bm_posn_.end());
}

void CHexEditView::OnBookmarksHide()
{
	begin_change();
	display_.hide_bookmarks = !display_.hide_bookmarks;
	make_change();
	end_change();
	theApp.SaveToMacro(km_bookmarks, (unsigned __int64)4);
}

void CHexEditView::OnUpdateBookmarksHide(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(display_.hide_bookmarks);
}

void CHexEditView::OnBookmarkToggle()
{
	if (GetDocument()->pfile1_ == NULL)
	{
		TaskMessageBox("Bookmark Requires Disk File",
			"A bookmark stores a disk file location.  Bookmarks can not "
			"be added to this file as it has not been saved to disk.\n\n"
			"Please save the file to disk and try again.", 0, 0, 0 /* no icon*/);
		((CHexEditApp *)AfxGetApp())->mac_error_ = 5;
		return;
	}

	int index;
	CBookmarkList *pbl = theApp.GetBookmarkList();

	// Test if there is already a bookmark at this location
	if ((index = GetDocument()->GetBookmarkAt(GetPos())) == -1)
	{
		// Add an unnamed bookmark here
		char *dv_name = "unnamed";
		int dummy;
		long last = pbl->GetSetLast(dv_name, dummy);

		CString bm_name;
		bm_name.Format("%s%05ld", dv_name, long(last));
		pbl->AddBookmark(bm_name, GetDocument()->pfile1_->GetFilePath(),
						 GetPos(), NULL, GetDocument());

	}
	else
		pbl->RemoveBookmark(index);    // remove the bookmark

	theApp.SaveToMacro(km_bookmarks, (unsigned __int64)7);
}


void CHexEditView::reset_tip()
{
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);

	CPointAp pt(addr2pos(start_addr));
	CPoint point = ConvertToDP(pt);
	point.x -= 24;
	ClientToScreen(&point);

	sel_tip_.Move(point);
	sel_tip_.Hide();
}

void CHexEditView::update_sel_tip(int delay /*=0*/)
{
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);

	if (theApp.sel_len_tip_ && (mouse_down_ || shift_down()) && start_addr != end_addr)
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
		CString ss;

		__int64 len = __int64(end_addr - start_addr);

		if (!display_.hex_addr)
		{
			char buf[22];                    // temp buf where we sprintf
			sprintf(buf, "%I64d", __int64(len));
			ss = buf;
			AddCommas(ss);
			sel_tip_.SetTextCol(dec_addr_col_);

			// If there is a fair amount of contrast with standard tip background
			// colour then use it else use something else.
			if (abs(BRIGHTNESS(dec_addr_col_) - BRIGHTNESS(::GetSysColor(COLOR_INFOBK))) > 30)
				sel_tip_.SetBgCol(::GetSysColor(COLOR_INFOBK));
			else if (BRIGHTNESS(dec_addr_col_) > 50)
				sel_tip_.SetBgCol(RGB(64,64,0));       // Light text so use dark "yellow" bg
			else
				sel_tip_.SetBgCol(RGB(255,255,224));   // Dark text so use light yellow bg
		}
		else
		{
			char buf[22];                    // temp buf where we sprintf
			if (len < 10)
				sprintf(buf, "%I64x", __int64(len));
			else if (aa->hex_ucase_)
				sprintf(buf, "%I64Xh", __int64(len));
			else
				sprintf(buf, "%I64xh", __int64(len));
			ss = buf;
			AddSpaces(ss);
			sel_tip_.SetTextCol(hex_addr_col_);

			// If there is a fair amount of contrast with standard tip background
			// colour then use it else use something else.
			if (abs(BRIGHTNESS(hex_addr_col_) - BRIGHTNESS(::GetSysColor(COLOR_INFOBK))) > 30)
				sel_tip_.SetBgCol(::GetSysColor(COLOR_INFOBK));
			else if (BRIGHTNESS(hex_addr_col_) > 50)
				sel_tip_.SetBgCol(RGB(64,64,0));       // Light text so use dark "yellow" bg
			else
				sel_tip_.SetBgCol(RGB(255,255,224));   // Dark text so use light yellow bg
		}

#if 0  // We used to indicate divisibility using colour
		if (len%8 == 0)
			sel_tip_.SetTextCol(RGB(0, 0, 128));
		else if (len%4 == 0)
			sel_tip_.SetTextCol(RGB(128, 0, 128));
		else if (len%2 == 0)
			sel_tip_.SetTextCol(RGB(128, 128, 0));
		else
			sel_tip_.SetTextCol(RGB(128, 128, 128));
#endif
		if (theApp.sel_len_div2_ && len > 10)  // User should know divisibility by 2,4 for numbers this small so don't show
		{
			int pow2;

			for (pow2 = 0; pow2 < 12; ++pow2)
				if (((len>>pow2) & 0x1) != 0)
					break;

			CString tmp;
			tmp.Format(" [%d]", 1<<pow2);

			ss += tmp;
		}

		// Don't update text if the string hasn't changed to avoid flicker
		CString strPrevious;
		sel_tip_.GetWindowText(strPrevious);
		if (strPrevious.Compare(ss) != 0)
			sel_tip_.SetWindowText(ss);

		// Check if tip window is too far from selection
		FILE_ADDRESS start_line = (start_addr + offset_)/rowsize_;
		FILE_ADDRESS end_line = (end_addr + offset_)/rowsize_ + 1;

		CRect rct;

		// Get bounding rectangle of selection (we only really need top and bottom)
		CRectAp bnd_rct;
		bnd_rct = CRectAp(0, start_line * line_height_,
						  char_pos(rowsize_), end_line * line_height_);

		sel_tip_.GetWindowRect(&rct);
		ScreenToClient(&rct);
		CRectAp tip_rct = ConvertFromDP(rct);

		GetDisplayRect(&rct);
		CRectAp cli_rct = ConvertFromDP(rct);

		bnd_rct.top -= tip_rct.Height()/2;
		bnd_rct.bottom += tip_rct.Height()/2;

		// Restrict bounds rect to the visible selection
		if (bnd_rct.top < cli_rct.top - 10)
		{
			bnd_rct.top = cli_rct.top - 10;
			if (bnd_rct.bottom < bnd_rct.top + tip_rct.Height())
				bnd_rct.bottom = bnd_rct.top + tip_rct.Height();
		}
		if (bnd_rct.bottom > cli_rct.bottom + 10)
		{
			bnd_rct.bottom = cli_rct.bottom + 10;
			if (bnd_rct.top > bnd_rct.bottom - tip_rct.Height())
				bnd_rct.top = bnd_rct.bottom - tip_rct.Height();
		}
		if (bnd_rct.left < cli_rct.left)
			bnd_rct.left = cli_rct.left;
		if (bnd_rct.right > cli_rct.right)
			bnd_rct.right = cli_rct.right;

		// Move the rectangle for tip within bounds
		if (tip_rct.top < bnd_rct.top)
		{
			tip_rct.bottom += bnd_rct.top - tip_rct.top;
			tip_rct.top    += bnd_rct.top - tip_rct.top;
		}
		else if (tip_rct.bottom > bnd_rct.bottom)
		{
			tip_rct.top    += bnd_rct.bottom - tip_rct.bottom;
			tip_rct.bottom += bnd_rct.bottom - tip_rct.bottom;
		}
		if (tip_rct.left < bnd_rct.left)
		{
			tip_rct.right += bnd_rct.left - tip_rct.left;
			tip_rct.left  += bnd_rct.left - tip_rct.left;
		}
		else if (tip_rct.right > bnd_rct.right)
		{
			tip_rct.left  += bnd_rct.right - tip_rct.right;
			tip_rct.right += bnd_rct.right - tip_rct.right;
		}

		// Calculate centre of rect and move the tip window
		CPointAp pt((tip_rct.left + tip_rct.right)/2, (tip_rct.top + tip_rct.bottom)/2);
		CPoint point = ConvertToDP(pt);
		ClientToScreen(&point);
		sel_tip_.Move(point);

		sel_tip_.Show(100, delay);
		SetFocus();
	}
	else
	{
		sel_tip_.Hide();
	}
}

// This is here to implement the macro command km_mouse
void CHexEditView::do_mouse(CPoint dev_down, CSizeAp doc_dist)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	CPointAp doc_down = ConvertFromDP(dev_down);  // Sel start point converted from device to doc coords
	CPointAp doc_up(doc_down);                    // Point where selection ends = start +
	doc_up += doc_dist;                         // ... distance of selection

	// Convert selection (doc coords) to addresses and make sure in valid range
	FILE_ADDRESS start_addr, end_addr;
	FILE_ADDRESS length = GetDocument()->length();
	start_addr = pos2addr(doc_down);
	end_addr = pos2addr(doc_up);

	if (start_addr < 0) start_addr = 0; else if (start_addr > length) start_addr = length;
	if (end_addr < 0) end_addr = 0; else if (end_addr > length) end_addr = length;

	// Save undo information
	FILE_ADDRESS prev_start, prev_end;
	GetSelAddr(prev_start, prev_end);
	int prev_row = 0;
	if (prev_start == prev_end && display_.vert_display)
		prev_row = pos2row(GetCaret());

	if (start_addr == prev_start && end_addr == prev_end)  // exactly the same selection
		return;

	BOOL saved_area = display_.edit_char;
	if (!display_.vert_display && display_.char_area && display_.hex_area) // If both areas displayed ...
		display_.edit_char = pos_hex(doc_down.x, FALSE) >= rowsize_;       // sel may swap between areas
	undo_.push_back(view_undo(undo_move));
	undo_.back().address = prev_start | (FILE_ADDRESS(prev_row)<<62);
	if (prev_start != prev_end)
	{
		// Save the previous selection
		undo_.push_back(view_undo(undo_sel));
		undo_.back().address = prev_end;
	}
	if (display_.edit_char != saved_area)
	{
		// Allow more buffer if editing hex
		if (display_.edit_char)
			SetHorzBufferZone(1);
		else
			SetHorzBufferZone(2);
		// Save the fact that we swapped between areas
		undo_.push_back(view_undo(undo_state, TRUE));
		undo_.back().disp_state = disp_state_;
	}

	SetSel(addr2pos(start_addr), addr2pos(end_addr), true);

	if (aa->highlight_)
		add_highlight(start_addr, end_addr, TRUE);

	nav_save(start_addr, end_addr, "Mouse Click (Play) ");
}

// This is here to implement the macro command km_shift_mouse
void CHexEditView::do_shift_mouse(CPoint dev_down, CSizeAp doc_dist)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	CPointAp doc_down = ConvertFromDP(dev_down); // Left down point converted from device to doc coords
	CPointAp doc_up(doc_down);                   // Point of left button up (doc coords) = ...
	doc_up += doc_dist;                          // ... doc_down + length of selection

	// Convert selection (doc coords) to addresses and make sure in valid range
	FILE_ADDRESS start_addr, end_addr;                  // New selection range
	FILE_ADDRESS prev_start, prev_end;                  // Previous selection range
	FILE_ADDRESS length = GetDocument()->length();
//    GetSelAddr(start_addr, prev_end);
	if (GetSelAddr(prev_start, prev_end))
		start_addr = prev_end;                  // Base of previous selection was end
	else
		start_addr = prev_start;
	end_addr = pos2addr(doc_up);

	if (start_addr < 0) start_addr = 0; else if (start_addr > length) start_addr = length;
	if (end_addr < 0) end_addr = 0; else if (end_addr > length) end_addr = length;

	if (end_addr != prev_end)
	{
		// Save the previous selection for undo
		undo_.push_back(view_undo(undo_move));
		undo_.back().address = start_addr;
		undo_.push_back(view_undo(undo_sel));
		undo_.back().address = prev_end;

		// Extend the selection
		SetSel(addr2pos(start_addr), addr2pos(end_addr), true);

		if (aa->highlight_)
			add_highlight(start_addr, end_addr, TRUE);
	}
	nav_save(start_addr, end_addr, "Mouse Click (Play) ");
}

void CHexEditView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// Move the caret to where the user clicked so that context menu items
	// that depend on the current address use the address that was clicked on
	CPointAp pp = ConvertFromDP(point);          // Point in our coord system

	if (( (display_.vert_display || display_.char_area) && pp.x >= char_pos(rowsize_)+text_width_ ||
		 !(display_.vert_display || display_.char_area) && pp.x >= hex_pos(rowsize_) )  )
	{
		// Don't do anything if off to right
		return;
	}

	// Save current state, selection etc for undo
	GetSelAddr(prev_start_, prev_end_);
	saved_state_ = disp_state_;                 // Keep the current state for saving in undo stack
	prev_row_ = 0;
	if (prev_start_ == prev_end_ && display_.vert_display)
		prev_row_ = pos2row(GetCaret());

	num_entered_ = num_del_ = num_bs_ = 0;  // Can't be editing while mousing
	if (!display_.vert_display && display_.char_area && display_.hex_area)
	{
		// Allow user to move caret between hex and char areas
		display_.edit_char = pos_hex(pp.x, FALSE) >= rowsize_;

		if (saved_state_ != disp_state_)
		{
			// Sett approp. buffering to allow user to see the whole byte that they are editing
			if (display_.edit_char)
				SetHorzBufferZone(1);
			else
				SetHorzBufferZone(2);
		}
	}

	// Note: this relies on display_.edit_char having been set correctly above
	FILE_ADDRESS addr = pos2addr(pp);
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = FALSE;

	// If we have not clicked inside the selection then ...
	if (addr < prev_start_ || addr >= prev_end_)
	{
		// Move the caret to where the user clicked
		ValidateCaret(pp, FALSE);
		SetCaret(pp);
		BOOL end_base = GetSelAddr(start_addr, end_addr);
	}
	else
	{
		// Don't move if clicked in selection
		start_addr = prev_start_;
		end_addr = prev_end_;
	}

	// Save in undo array if position moved
	if (prev_start_ != start_addr || prev_end_ != end_addr || saved_state_ != disp_state_)
	{
		// Save the caret position (or start of selection)
		undo_.push_back(view_undo(undo_move));
		undo_.back().address = prev_start_ | (FILE_ADDRESS(prev_row_)<<62);
		if (prev_start_ != prev_end_)
		{
			// Save the previous selection
			undo_.push_back(view_undo(undo_sel));
			undo_.back().address = prev_end_;
		}
		if (saved_state_ != disp_state_)
		{
			// Save the fact that we swapped between areas
			undo_.push_back(view_undo(undo_state, TRUE));
			undo_.back().disp_state = saved_state_;
		}

		if (end_base)
			SetSel(addr2pos(end_addr), addr2pos(start_addr), true);
		else
			SetSel(addr2pos(start_addr), addr2pos(end_addr));
		DisplayCaret();

		nav_save(start_addr, end_addr, "Right Mouse Click ");
	}

	// Update properties etc for new position
	show_prop();
	show_calc();
	show_pos();


#if 0
	else if (saved_state_ != disp_state_)
	{
		// Save the fact that we swapped between areas
		undo_.push_back(view_undo(undo_state, FALSE));
		undo_.back().disp_state = saved_state_;
	}
--------
	FILE_ADDRESS addr = pos2addr(pp);
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);

	// Make sure we have NOT clicked inside the selection
	if (addr < start_addr || addr > end_addr)
	{
		// Move the caret to where the user clicked
		ValidateCaret(pp, FALSE);
		SetCaret(pp);

		// Update properties etc for new position
		show_prop();
		show_calc();
		show_pos();
	}
#endif

	CScrView::OnRButtonDown(nFlags, point);
}

void CHexEditView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	view_context(point);
}

BOOL CHexEditView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// Only respond if in client area
	if (nHitTest == HTCLIENT)
	{
		// Get mouse location in window coords
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);

#ifdef RULER_ADJUST
		if (point.y < bdr_top_)
		{
			CPointAp pp = ConvertFromDP(point);          // Point in our coord system
			if (point.y > bdr_top_ - 7 &&
				(over_rowsize_adjuster(pp) || 
				 over_offset_adjuster(pp) || 
				 over_group_by_adjuster(pp)) )
			{
				::SetCursor(theApp.LoadStandardCursor(IDC_SIZEWE));
				return TRUE;
			}
			return CScrView::OnSetCursor(pWnd, nHitTest, message);
		}
#endif

		// Work out location of cursor in window and make sure it's
		// not over address area (on left) or past last chars (on right)
		CPointAp pt = ConvertFromDP(point);
		if (pt.x > (addr_width_ - 1)*text_width_ &&
			(display_.vert_display && pt.x < char_pos(rowsize_)+text_width_w_ ||
			 !display_.vert_display && display_.char_area && pt.x < char_pos(rowsize_)+text_width_w_ ||
			 !display_.vert_display && !display_.char_area && pt.x < hex_pos(rowsize_) )  )
		{
			// Over hex or char area
			if (drag_mark_ || drag_bookmark_ > -1)
				::SetCursor(theApp.LoadCursor(IDC_HANDGRAB));        // Bookmark drag
			else if (address_at(point) == mark_ || bookmark_at(address_at(point)) > -1)
				::SetCursor(theApp.LoadCursor(IDC_HANDOPEN));        // Indicate bookmark can be moved
			else if (theApp.highlight_)
				::SetCursor(theApp.LoadCursor(IDC_HIGHLIGHT));       // Highlighter cursor
			else
				::SetCursor(theApp.LoadStandardCursor(IDC_IBEAM));   // Text insertion cursor
			return TRUE;
		}
	}

	return CScrView::OnSetCursor(pWnd, nHitTest, message);
}

// Display views context menu
// point is in screen coords
void CHexEditView::view_context(CPoint point)
{
#if 0 // Changes for BCG context menus
	// Get the top level menu that contains the submenus used as popup menus
	CMenu top;
	BOOL ok = top.LoadMenu(IDR_CONTEXT);
	ASSERT(ok);
	if (!ok) return;

	CHECK_SECURITY(51);

	CMenu *ppop;
	if (point.x < hex_pos(0))
	{
		// Context menu for address area
		ppop = top.GetSubMenu(1);
	}
	else if (display_.char_area && point.x >= char_pos(0))
	{
		// Context menu for character area
		ppop = top.GetSubMenu(2);
	}
	else
	{
		// Context menu for hex area
		ppop = top.GetSubMenu(3);
	}
	ASSERT(ppop != NULL);
	if (ppop != NULL)
	{
		// Convert window coords to the required screen coords
		ClientToScreen(&point);
		VERIFY(ppop->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
									point.x, point.y, GetFrame()));
	}

	top.DestroyMenu();
#else
	CContextMenuManager *pCMM = theApp.GetContextMenuManager();

	CPoint cli_pt(point);              // The point clicked in doc coords
	ScreenToClient(&cli_pt);
	CPointAp doc_pt = ConvertFromDP(cli_pt);

	if (( (display_.vert_display || display_.char_area) && doc_pt.x >= char_pos(rowsize_)+text_width_ ||
		 !(display_.vert_display || display_.char_area) && doc_pt.x >= hex_pos(rowsize_) )  )
	{
		// Don't do anything if off to right
		return;
	}

	if (doc_pt.x < hex_pos(0))
		pCMM->ShowPopupMenu(IDR_CONTEXT_ADDRESS, point.x, point.y, this);
	else if (display_.vert_display)
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in char area
		FILE_ADDRESS addr = (doc_pt.y/line_height_)*rowsize_ - offset_ +
							pos_char(doc_pt.x);

		if (addr >= start_addr && addr < end_addr)
			pCMM->ShowPopupMenu(IDR_CONTEXT_SELECTION, point.x, point.y, this);
		else if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_BOOKMARKS, point.x, point.y, this);
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_HIGHLIGHT, point.x, point.y, this);
		else if (pos2row(doc_pt) == 0)
			pCMM->ShowPopupMenu(IDR_CONTEXT_CHAR, point.x, point.y, this);
		else
			pCMM->ShowPopupMenu(IDR_CONTEXT_HEX, point.x, point.y, this);
	}
	else if (!display_.char_area || doc_pt.x < char_pos(0))
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in hex area
		FILE_ADDRESS addr = (doc_pt.y/text_height_)*rowsize_ - offset_ +
							pos_hex(doc_pt.x);

		if (addr >= start_addr && addr < end_addr)
			pCMM->ShowPopupMenu(IDR_CONTEXT_SELECTION, point.x, point.y, this);
		else if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_BOOKMARKS, point.x, point.y, this);
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_HIGHLIGHT, point.x, point.y, this);
		else
			pCMM->ShowPopupMenu(IDR_CONTEXT_HEX, point.x, point.y, this);
	}
	else
	{
		// Get selection address
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);

		// Get address clicked on in char area
		FILE_ADDRESS addr = (doc_pt.y/text_height_)*rowsize_ - offset_ +
							pos_char(doc_pt.x);

		if (addr >= start_addr && addr < end_addr)
			pCMM->ShowPopupMenu(IDR_CONTEXT_SELECTION, point.x, point.y, this);
		else if (!display_.hide_bookmarks &&
				 std::find(GetDocument()->bm_posn_.begin(), GetDocument()->bm_posn_.end(), addr) != GetDocument()->bm_posn_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_BOOKMARKS, point.x, point.y, this);
		else if (!display_.hide_highlight && hl_set_.find(addr) != hl_set_.end())
			pCMM->ShowPopupMenu(IDR_CONTEXT_HIGHLIGHT, point.x, point.y, this);
		else
			pCMM->ShowPopupMenu(IDR_CONTEXT_CHAR, point.x, point.y, this);
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CHexEditView command handlers

void CHexEditView::OnRedraw()
{
	CRect cli;

	// Work out window height in logical units
	GetDisplayRect(&cli);
	CRectAp doc_rect = ConvertFromDP(cli);

	// Move display so that caret is centred vertically
	CPointAp pp = GetCaret();
	pp.x = 0;                       // Scroll to left side (but see DisplayCaret() below)
	if ((pp.y -= (doc_rect.bottom-doc_rect.top)/2) < 0)
		pp.y = 0;
	SetScroll(pp);

	DoInvalidate();                 // Also redraw display
	DisplayCaret();                 // Make sure caret not off right side

	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_redraw);
}

void CHexEditView::OnScrollDown()
{
	CPointAp pp = GetScroll();
	pp.y += text_height_;
	SetScroll(pp);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_scroll_down);
}

void CHexEditView::OnScrollUp()
{
	CPointAp pp = GetScroll();
	pp.y -= text_height_;
	SetScroll(pp);
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_scroll_up);
}

// Move current position to start of display line
void CHexEditView::OnStartLine()
{
	num_entered_ = num_del_ = num_bs_ = 0;  // Turn off any consec edits
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	// Check if offset has actually changed
	if (real_offset_ != start_addr)
	{
		undo_.push_back(view_undo(undo_offset));
		undo_.back().rowsize = real_offset_;        // Save previous offset for undo
		real_offset_ = (int)start_addr;
		//offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;

		recalc_display();
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		DoInvalidate();
		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_start_line);
	}
	else
	{
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: offset not changed");
		((CHexEditApp *)AfxGetApp())->mac_error_ = 1;
	}
}

void CHexEditView::OnSwap()
{
	if (display_.vert_display)
		return;

	CHECK_SECURITY(32);

	if (display_.char_area && display_.hex_area)
	{
		num_entered_ = num_del_ = num_bs_ = 0;  // Turn off any consec edits

		// Swap between hex and char areas
		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		undo_.push_back(view_undo(undo_state));
		undo_.back().disp_state = disp_state_;
		display_.edit_char = !display_.edit_char;
		if (display_.edit_char)
			SetHorzBufferZone(1);
		else
			SetHorzBufferZone(2);
		if (end_base)
			SetSel(addr2pos(end_addr), addr2pos(start_addr), true);
		else
			SetSel(addr2pos(start_addr), addr2pos(end_addr));
		DisplayCaret();

		((CHexEditApp *)AfxGetApp())->SaveToMacro(km_swap_areas);
	}
	else
	{
		TaskMessageBox(display_.char_area ? "No Hex Area" : "No Char Area",
				"You cannot swap between areas unless both areas are displayed.");
		((CHexEditApp *)AfxGetApp())->mac_error_ = 5;
	}
}

void CHexEditView::OnUpdateSwap(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(display_.char_area && display_.hex_area);      // disallow swap if both areas not displayed
}

void CHexEditView::OnDel()
{
	num_entered_ = 0;           // Not entering but deleting

	CPointAp pt_start, pt_end;
	if (GetSel(pt_start, pt_end))
		SetSel(pt_end, pt_start, true);
	else
		SetSel(pt_start, pt_end);     // Calls ValidateCaret - causing caret selection to be moved to valid locations

	// Ensure we are not in RO or INS modes
	if (check_ro("delete bytes") || check_ovr("delete"))
		return;

	// Handle deletion of chars at caret or selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
//      address = pos2addr(GetCaret());

	CHECK_SECURITY(190);

	if (start == end)
	{
//        if ((long)nRepCnt > GetDocument()->length() - start)
//            nRepCnt = (UINT)(GetDocument()->length() - start);
//        if (nRepCnt > 0)
//            GetDocument()->Change(mod_delforw, start, nRepCnt,
//                                  NULL, num_del_, this);
//        num_del_ += nRepCnt*2;
		if (start >= GetDocument()->length())
		{
			((CHexEditApp *)AfxGetApp())->mac_error_ = 2;
			return;
		}

		GetDocument()->Change(mod_delforw, start, 1, NULL, num_del_, this);
		num_del_ += 2;
	}
	else
	{
		GetDocument()->Change(mod_delforw, start, end-start,
								  NULL, 0, this);
		num_del_ = 0;               // Make sure this is separate deletion
	}
	DisplayCaret();                 // Make sure caret is visible

	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_del);
}

void CHexEditView::OnSelectAll()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Can't be editing while mousing
	GetSelAddr(prev_start_, prev_end_);
	prev_row_ = 0;
	if (prev_start_ == prev_end_ && display_.vert_display)
		prev_row_ = pos2row(GetCaret());
	SetSel(addr2pos(0), addr2pos(GetDocument()->length()));
	show_prop();
	show_calc();
	show_pos();

	// Save in undo array if position moved
	if (prev_start_ != 0 || prev_end_ != GetDocument()->length())
	{
		// Save the caret position (or start of selection)
		undo_.push_back(view_undo(undo_move));
		undo_.back().address = prev_start_ | (FILE_ADDRESS(prev_row_)<<62);
		if (prev_start_ != prev_end_)
		{
			// Save the previous selection
			undo_.push_back(view_undo(undo_sel));
			undo_.back().address = prev_end_;
		}

		nav_save(0, GetDocument()->length(), "Select All ");
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_sel_all);
}

void CHexEditView::OnSelectLine()
{
	num_entered_ = num_del_ = num_bs_ = 0;
	GetSelAddr(prev_start_, prev_end_);
	prev_row_ = 0;
	if (prev_start_ == prev_end_ && display_.vert_display)
		prev_row_ = pos2row(GetCaret());

	FILE_ADDRESS new_start = ((prev_start_+offset_)/rowsize_) * rowsize_ - offset_;
	FILE_ADDRESS new_end =   ((prev_end_+offset_)/rowsize_ + 1) * rowsize_ - offset_;
	ASSERT(new_end > new_start);
	SetSel(addr2pos(new_start), addr2pos(new_end));
	show_prop();
	show_calc();
	show_pos();

	// Save in undo array if position moved
	if (prev_start_ != new_start || prev_end_ != new_end)
	{
		// Save the caret position (or start of selection)
		undo_.push_back(view_undo(undo_move));
		undo_.back().address = prev_start_ | (FILE_ADDRESS(prev_row_)<<62);
		if (prev_start_ != prev_end_)
		{
			// Save the previous selection
			undo_.push_back(view_undo(undo_sel));
			undo_.back().address = prev_end_;
		}
		nav_save(new_start, new_end, "Select Line ");
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_sel_line);
}

void CHexEditView::OnInsertBlock()
{
	CNewFile dlg(this, true);

	if (dlg.DoModal() == IDOK)
	{
		const char *data_str = NULL;
		dlg.fill_.create_on_disk = 0;   // Make sure we are not creating a new file
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

		do_insert_block(dlg.fill_state_, data_str);
	}
}

void CHexEditView::do_insert_block(_int64 params, const char *data_str)
{
	// Can't insert a block if we are in RO or INS modes
	if (check_ro("insert a block") || check_ovr("insert a block"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing


	FILE_ADDRESS addr = GetPos();                       // Current caret (address to insert file)
	FILE_ADDRESS len = GetDocument()->insert_block(addr, params, data_str, this);
	if (len > -1)
	{
		SetSel(addr2pos(addr), addr2pos(addr+len));
		DisplayCaret();
		theApp.SaveToMacro(km_insert, params);
		theApp.SaveToMacro(km_insert_str, data_str);
	}
}

void CHexEditView::OnUpdateInsertBlock(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only()); // disallow insert if file is read only
}

// Get file name, insert into document and select inserted bytes
void CHexEditView::OnReadFile()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Get name of file to read
	CHexFileDialog dlgFile("ReadFileDlg", HIDD_FILE_READ, TRUE, NULL, aa->current_read_,
						   OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						   aa->GetCurrentFilters(), "Read", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Insert File";

	if (dlgFile.DoModal() == IDOK)
	{
		aa->current_read_ = dlgFile.GetPathName();

		CHECK_SECURITY(23);

		do_read(aa->current_read_);
	}
}

// Actually open and read the file and select the inserted bytes
void CHexEditView::do_read(CString file_name)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Can't modify the file if view is read only or in overtype mode
	if (check_ro("insert a file") || check_ovr("insert a file"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	FILE_ADDRESS addr = GetPos();                       // Current caret (address to insert file)
	FILE_ADDRESS data_len = CFile64::GetSize(file_name);
	if (data_len == 0)
	{
		// No need to insert a zero length file
		((CMainFrame *)AfxGetMainWnd())->StatusBarText("File is empty: nothing inserted");
		aa->mac_error_ = 1;
		return;
	}

	int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)
	if (data_len > (16*1024*1024) && (idx = GetDocument()->AddDataFile(file_name)) != -1)
	{
		// Use the file in situ
		GetDocument()->Change(mod_insert_file, addr, data_len, NULL, idx, this);
	}
	else
	{
		if (data_len > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Large File Error",
						  "HexEdit Pro is out of temporary files "
						  "and cannot open such a large file.  "
						  "Please save the file to free "
						  "temporary file handles and try again.",
						  MB_OK);
			return;
		}

		if (data_len > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Large File Warning",
							  "HexEdit Pro is out of temporary file "
							  "handles and opening such a large file may "
							  "cause memory exhaustion.  Please select "
							  "\"No\" and save the file to free handles. "
							  "Or, select \"Yes\" to continue.\n\n"
							  "Do you want to continue?", MB_YESNO) != IDYES)
				return;
		}

		// Open the file for reading
		CFile ff;                   // Don't make this CFile64 as we don't want to read > 2Gbytes into memory
		CFileException fe;          // Used to receive file error information

		if (!ff.Open(file_name,
			CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary, &fe) )
		{
			// Display info about why the open failed
			CString mess(file_name);
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
				if (!CFile::GetStatus(file_name, fs))
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
			default:
				mess += "\ncould not be opened (reason unknown)";
				break;
			}
			TaskMessageBox("File Read Error", mess);

			aa->mac_error_ = 10;
			return;
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		// If the user tries to insert a huge file we might run out of memory so catch this
		try
		{
			// Get memory to read all of the file
			unsigned char *file_data = new unsigned char[size_t(data_len)];

			// Read the file into memory
			if (ff.Read((void *)file_data, (UINT)data_len) != data_len)
			{
				TaskMessageBox("Read Error", "Some or all of the file could be read");
				aa->mac_error_ = 10;
				return;
			}
			ff.Close();

			// Make the memory part of the document
			GetDocument()->Change(mod_insert, addr, data_len, file_data, 0, this);
			delete[] file_data;
		}
		catch (std::bad_alloc)
		{
			TaskMessageBox("File Read", "Insufficient memory");
			aa->mac_error_ = 10;
			return;
		}
	}
	ASSERT(data_len > 0);
	SetSel(addr2pos(addr), addr2pos(addr+data_len));
	DisplayCaret();
	aa->SaveToMacro(km_read_file, file_name);
}

void CHexEditView::OnUpdateReadFile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only()); // disallow insert if file is read only
}

void CHexEditView::OnEditWriteFile()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());
	if (start == end)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(aa->playing_);
		TaskMessageBox("Nothing Selected",
			"This command writes the current selection to a file.  "
			"There is no selection so no file was written.  "
			"(An empty file is not created in this situation.)");
		aa->mac_error_ = 10;
		return;
	}

	// Get the file name to write the selection to
	CHexFileDialog dlgFile("WriteFileDlg", HIDD_FILE_WRITE, FALSE, NULL, aa->current_write_,
						   OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
						   aa->GetCurrentFilters(), "Write", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Save Selection As";

//    if (!aa->current_write_.IsEmpty())
//        dlgFile.m_ofn.lpstrInitialDir = aa->current_write_;

	if (dlgFile.DoModal() != IDOK)
	{
		aa->mac_error_ = 2;
		return;
	}

	aa->current_write_ = dlgFile.GetPathName();

	CHECK_SECURITY(25);

	// Write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	if (GetDocument()->WriteData(aa->current_write_, start, end))
		aa->SaveToMacro(km_write_file);
	else
		aa->current_write_.Empty();
}

void CHexEditView::OnEditAppendFile()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());
	if (start == end)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(aa->playing_);
		TaskMessageBox("Nothing selected",
			"This command uses the current selection.  As there is "
			"no selection, nothing was appended to the file!");
		aa->mac_error_ = 10;
		return;
	}

	// Get the file name to write the selection to
	CHexFileDialog dlgFile("WriteFileDlg", HIDD_FILE_APPEND, TRUE, NULL, aa->current_write_,
						OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						aa->GetCurrentFilters(), "Append", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Append Selection To";

	if (dlgFile.DoModal() != IDOK)
	{
		aa->mac_error_ = 2;
		return;
	}

	CHECK_SECURITY(15);

	aa->current_write_ = dlgFile.GetPathName();

	// Write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	if (GetDocument()->WriteData(aa->current_write_, start, end, TRUE))
		aa->SaveToMacro(km_append_file);
	else
		aa->current_write_.Empty();
}

void CHexEditView::OnEditAppendSameFile()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());
	CHECK_SECURITY(25);

	if (start == end)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(aa->playing_);
		TaskMessageBox("Nothing selected",
			"This command uses the current selection.  As there is "
			"no selection, nothing was appended to the file!");
		aa->mac_error_ = 10;
		return;
	}

	// Write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	if (GetDocument()->WriteData(aa->current_write_, start, end, TRUE))
		aa->SaveToMacro(km_append_same_file);
	else
		aa->current_write_.Empty();
}

void CHexEditView::OnUpdateEditAppendSameFile(CCmdUI* pCmdUI)
{
	if (theApp.current_write_.IsEmpty())
		pCmdUI->Enable(FALSE);
	else
		OnUpdateClipboard(pCmdUI);
}

void CHexEditView::OnExportSRecord(UINT nID)
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	// If no selection and no highlight there is nothing to do
	if (start == end && hl_set_.empty())
	{
		TaskMessageBox("Nothing to Export",
			"This command uses the current selection or highlights.  "
			"As nothing is selected or highlighted, nothing can be exported.");
		theApp.mac_error_ = 10;
		return;
	}

	// Get the file name to write to (plus discontinuous setting)
	CExportDialog dlgFile(theApp.current_export_, HIDD_FILE_EXPORT_MOTOROLA,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
						"Motorola S Files (*.s)|*.s|"+theApp.GetCurrentFilters(), this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Export S Records";

	if (dlgFile.DoModal() != IDOK)
	{
		theApp.mac_error_ = 2;
		return;
	}
	theApp.current_export_ = dlgFile.GetPathName();

	CHECK_SECURITY(25);

	if (theApp.import_discon_)
	{
		if (hl_set_.empty())
		{
			TaskMessageBox("Nothing to Export",
				"This command uses the current highlights.  "
				"As nothing is highlighted, nothing can be exported.");
			theApp.mac_error_ = 10;
			return;
		}

		// Set start and end to the range of highlighted bytes
		start = *hl_set_.begin();           // First highlighted byte
		range_set<FILE_ADDRESS>::iterator plast = hl_set_.end();
		plast--;
		end = *plast;            // Last highlighted byte
	}
	else
	{
		if (start == end)
		{
			// Nothing selected
			TaskMessageBox("Nothing selected",
				"Export uses the current selection.  As there is "
				"no selection, no file was written.");
			theApp.mac_error_ = 10;
			return;
		}
	}
	// Check the range of exported addresses to check that they are valid
	FILE_ADDRESS last_S_address;        // Byte past the last address to be written out
	int stype = 3;                          // Type of data records to write (S1, S2, or S3)
	if (theApp.export_base_addr_ < 0)
		last_S_address = end;
	else
		last_S_address = end - start + theApp.export_base_addr_;

	// Check that end address is not too big for S Record
	if (nID == ID_EXPORT_S1)
	{
		// Addresses must be within 16 bit range
		if (last_S_address > 0x10000)
		{
			TaskMessageBox("Address Too Big",
				"S1 addresses are 16 bits in size.  The export could not be performed "
				"as some or all export addresses are too large.");
			theApp.mac_error_ = 10;
			return;
		}
		stype = 1;
	}
	else if (nID == ID_EXPORT_S2)
	{
		// Addresses must be within 24 bit range
		if (last_S_address > 0x1000000)
		{
			TaskMessageBox("Address Too Big",
				"S2 addresses are 24 bits in size.  The export could not be performed "
				"as some or all export addresses are too large.");
			theApp.mac_error_ = 10;
			return;
		}
		stype = 2;
	}
	else if (nID == ID_EXPORT_S3)
	{
		// Addresses must be within 32 bit range
		if (last_S_address > 0x100000000)
		{
			TaskMessageBox("Address Too Big",
				"S3 addresses are 32 bits in size.  The export could not be performed "
				"as some or all export addresses are too large.");
			theApp.mac_error_ = 10;
			return;
		}
		ASSERT(stype == 3);
	}

	// Get ready to write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	CWriteSRecord wsr(theApp.current_export_,
					theApp.export_base_addr_ >= 0 ? theApp.export_base_addr_ : long(start),
					stype,
					theApp.export_line_len_);
	if (!wsr.Error().IsEmpty())
	{
		TaskMessageBox("Export Error", wsr.Error());
		theApp.mac_error_ = 10;
		theApp.current_export_.Empty();
		return;
	}

	unsigned char *buf = new unsigned char[4096];
	size_t len;
	if (theApp.import_discon_)
	{
		unsigned long base_address = 0;
		if (theApp.export_base_addr_ >= 0)
			base_address = unsigned long(theApp.export_base_addr_ - start);  // This puts 1st rec at export address of zero

		range_set<FILE_ADDRESS>::range_t::iterator pp;
		for (pp = hl_set_.range_.begin(); pp != hl_set_.range_.end(); ++pp)
			for (FILE_ADDRESS curr = pp->sfirst; curr < pp->slast; curr += len)
			{
				len = int(min(4096, pp->slast - curr));
				VERIFY(GetDocument()->GetData(buf, len, curr) == len);

				wsr.Put(buf, len, unsigned long(base_address + curr));
				if (!wsr.Error().IsEmpty())
				{
					TaskMessageBox("Export Error", wsr.Error());
					theApp.mac_error_ = 10;
					theApp.current_export_.Empty();
					goto export_end;
				}
			}
	}
	else
	{
		FILE_ADDRESS curr;
		for (curr = start; curr < end; curr += len)
		{
			len = min(4096, int(end - curr));
			VERIFY(GetDocument()->GetData(buf, len, curr) == len);

			wsr.Put(buf, len);
			if (!wsr.Error().IsEmpty())
			{
				TaskMessageBox("Export Error", wsr.Error());
				theApp.mac_error_ = 10;
				theApp.current_export_.Empty();
				goto export_end;
			}
		}
		ASSERT(curr == end);
	}
export_end:
	delete[] buf;

	if (theApp.mac_error_ < 10)
		theApp.SaveToMacro(km_write_file, stype);  // stype == 1,2, or 3
}

void CHexEditView::OnUpdateExportSRecord(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	if (start == end)
	{
		// We can still export based on highlights
		if (hl_set_.empty())
		{
			pCmdUI->Enable(FALSE);
			return;
		}
		FILE_ADDRESS hl_start, hl_end;
		hl_start = *hl_set_.begin();           // First highlighted byte
		range_set<FILE_ADDRESS>::iterator plast = hl_set_.end();
		plast--;
		hl_end = *plast;                       // Last highlighted byte

		// Not quite right but should prevent valid export cmds from being disabled
		if (hl_start > start) start = hl_start;
		if (hl_end < end) end = hl_end;
	}

	// Work out the last address that needs to be exported
	if (theApp.export_base_addr_ >= 0)
		end = end - start + theApp.export_base_addr_;

	if (pCmdUI->m_nID == ID_EXPORT_S1)
	{
		// Addresses must be within 16 bit range
		pCmdUI->Enable(end <= 0x10000);
	}
	else if (pCmdUI->m_nID == ID_EXPORT_S2)
	{
		// Addresses must be within 24 bit range
		pCmdUI->Enable(end <= 0x1000000);
	}
	else if (pCmdUI->m_nID == ID_EXPORT_S3)
	{
		// Addresses must be within 32 bit range
		pCmdUI->Enable(end <= 0x100000000);
	}
}

void CHexEditView::OnImportMotorolaS()
{
	// Get name of file to import
//    CFileDialog dlgFile(TRUE, NULL, theApp.current_import_,
//                        OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
//                        theApp.GetCurrentFilters(), this);
	CImportDialog dlgFile(theApp.current_import_, HIDD_FILE_IMPORT_MOTOROLA,
						OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						"Motorola S Files (*.s)|*.s|"+theApp.GetCurrentFilters(), this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Import Motorola S Records";

	if (dlgFile.DoModal() == IDOK)
	{
		theApp.current_import_ = dlgFile.GetPathName();

		do_motorola(theApp.current_import_);
	}
}

void CHexEditView::do_motorola(CString file_name)
{
	// Can't import we are in RO or INS modes
	if (check_ro("import a file") || !theApp.import_discon_ && check_ovr("import (non-adjoining)"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	CHECK_SECURITY(33);

	if (theApp.import_discon_)
	{
		BOOL ptoo = FALSE;              // First change is new undo op, thence merged with it
		if (theApp.import_highlight_ && !hl_set_.empty())
		{
			undo_.push_back(view_undo(undo_highlight));
			undo_.back().phl = new range_set<FILE_ADDRESS>(hl_set_);
			hl_set_.clear();
			DoInvalidate();
			ptoo = TRUE;
		}

		CReadSRecord rsr(file_name, TRUE);

		if (!rsr.Error().IsEmpty())
		{
			TaskMessageBox("Import Error", rsr.Error());
			theApp.mac_error_ = 10;
			return;
		}

		// Read 
		int count, skipped;             // Total records read and ignored
		size_t data_len = 1024;
		unsigned char *file_data = new unsigned char[data_len];  // one S record
		unsigned long address;
		int len;                        // length of data from one S record

		for (count = skipped = 0; (len = rsr.Get(file_data, data_len, address)) > 0; ++count)
		{
			if (address + len > GetDocument()->length())
			{
				++skipped;
				continue;
			}
			if (theApp.import_highlight_)
			{
				// Highlight the record
				add_highlight(address, address+len, ptoo);
				ptoo = TRUE;
			}
			GetDocument()->Change(mod_replace, FILE_ADDRESS(address), FILE_ADDRESS(len), file_data, 0, this, ptoo);
			ptoo = TRUE;  // force subsequent changes to be merged into same undo operation
		}
		delete[] file_data;

		if (!rsr.Error().IsEmpty())
			TaskMessageBox("Import Error", rsr.Error());
		CString mess;
		if (skipped > 0)
			mess.Format("Wrote %ld records\nIgnored %ld records\nimporting \"%s\"",
						long(count - skipped),
						long(skipped),
						file_name);
		else
			mess.Format("Wrote %ld records importing \"%s\"", long(count), file_name);
		CAvoidableDialog::Show(IDS_IMPORT_RESULT, mess, "", 0, MAKEINTRESOURCE(IDI_INFO));

		theApp.SaveToMacro(km_import_motorola, file_name);
	}
	else
	{
		CReadSRecord rsr(file_name);

		if (!rsr.Error().IsEmpty())
		{
			TaskMessageBox("Import Error", rsr.Error());
			theApp.mac_error_ = 10;
			return;
		}

		// We need to store the data in fairly large chunks otherwise the doc undo
		// stack becomes very full and slows things down
		size_t data_len = 32768;
		unsigned char *file_data = new unsigned char[data_len];  // work buffer
		unsigned char *pp = file_data;  // Ptr into file_data where we are currently storing
		unsigned long address;          // Import S record address (ignored)
		int len;                        // length of data from one S record
		bool first(true);               // is this the first S record read from the file
		FILE_ADDRESS start_addr, end_addr, curr_addr = -1;

		GetSelAddr(start_addr, end_addr);  // Get the current selection

		for (;;)
		{
			len = rsr.Get(pp, data_len - (pp - file_data), address);

			if (len <= 0 || (pp - file_data) + len > (int(data_len) - 256))
			{
				// The buffer is almost full or we're finished reading the file - so save buffer
				if (first)
				{
					curr_addr = start_addr;
					if (start_addr < end_addr)
					{
						// Delete current selection and insert new data
						GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
						GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this, TRUE);
					}
					else
						GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this);
					first = false;
				}
				else
				{
					ASSERT(curr_addr != -1);
					GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this, TRUE);
				}
				curr_addr += pp - file_data + len;
				pp = file_data;
			}
			else
				pp += len;

			if (len <= 0)
				break;
		}
		delete[] file_data;

		if (!rsr.Error().IsEmpty())
			TaskMessageBox("Import Error", rsr.Error());

		theApp.SaveToMacro(km_import_motorola, file_name);

		SetSel(addr2pos(start_addr), addr2pos(curr_addr));
		DisplayCaret();
	}
}

void CHexEditView::OnUpdateImportMotorolaS(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only()); // disallow insert if file is read only
}

void CHexEditView::OnImportIntel()
{
	// Get name of file to import
//    CFileDialog dlgFile(TRUE, NULL, theApp.current_import_,
//                        OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
//                        theApp.GetCurrentFilters(), this);
	CImportDialog dlgFile(theApp.current_import_, HIDD_FILE_IMPORT_INTEL,
						OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						"Intel Hex Files (*,hex, *.ihx)|*.hex;*.ihx|"+theApp.GetCurrentFilters(), this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Import Intel Hex File";

	if (dlgFile.DoModal() == IDOK)
	{
		theApp.current_import_ = dlgFile.GetPathName();

		do_intel(theApp.current_import_);
	}
}

void CHexEditView::do_intel(CString file_name)
{
	if (check_ro("import an Intel hex file") || !theApp.import_discon_ && check_ovr("import (non-adjoining)"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CHECK_SECURITY(32);
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	if (theApp.import_discon_)
	{
		BOOL ptoo = FALSE;              // First change is new undo op, thence merged with it
		if (theApp.import_highlight_ && !hl_set_.empty())
		{
			undo_.push_back(view_undo(undo_highlight));
			undo_.back().phl = new range_set<FILE_ADDRESS>(hl_set_);
			hl_set_.clear();
			DoInvalidate();
			ptoo = TRUE;
		}

		CReadIntelHex rih(file_name, TRUE);

		if (!rih.Error().IsEmpty())
		{
			TaskMessageBox("Import Error", rih.Error());
			theApp.mac_error_ = 10;
			return;
		}

		// Read 
		int count, skipped;             // Total records read and ignored
		size_t data_len = 1024;
		unsigned char *file_data = new unsigned char[data_len];  // one S record
		unsigned long address;
		int len;                        // length of data from one S record

		for (count = skipped = 0; (len = rih.Get(file_data, data_len, address)) > 0; ++count)
		{
			if (address + len > GetDocument()->length())
			{
				++skipped;
				continue;
			}
			if (theApp.import_highlight_)
			{
				// Highlight the record
				add_highlight(address, address+len, ptoo);
				ptoo = TRUE;
			}
			GetDocument()->Change(mod_replace, FILE_ADDRESS(address), FILE_ADDRESS(len), file_data, 0, this, ptoo);
			ptoo = TRUE;  // force subsequent changes to be merged into same undo operation
		}
		delete[] file_data;

		if (!rih.Error().IsEmpty())
			TaskMessageBox("Import Error", rih.Error());
		CString mess;
		if (skipped > 0)
			mess.Format("Wrote %ld records\nIgnored %ld records\nimporting \"%s\"",
						long(count - skipped),
						long(skipped),
						file_name);
		else
			mess.Format("Wrote %ld records importing \"%s\"", long(count), file_name);
		CAvoidableDialog::Show(IDS_IMPORT_RESULT, mess, NULL, 0, MAKEINTRESOURCE(IDI_INFO));

		theApp.SaveToMacro(km_import_intel, file_name);
	}
	else
	{
		CReadIntelHex rih(file_name);

		if (!rih.Error().IsEmpty())
		{
			TaskMessageBox("Import Error", rih.Error());
			theApp.mac_error_ = 10;
			return;
		}

		// We need to store the data in fairly large chunks otherwise the doc undo
		// stack becomes very full and slows things down
		size_t data_len = 32768;
		unsigned char *file_data = new unsigned char[data_len];  // work buffer
		unsigned char *pp = file_data;  // Ptr into file_data where we are currently storing
		int len;                        // length of data from one S record
		unsigned long address;          // Import record address (ignored)
		bool first(true);               // is this the first S record read from the file
		FILE_ADDRESS start_addr, end_addr, curr_addr = -1;

		GetSelAddr(start_addr, end_addr);  // Get the current selection

		for (;;)
		{
			len = rih.Get(pp, data_len - (pp - file_data), address);

			if (len <= 0 || (pp - file_data) + len > (int(data_len) - 256))
			{
				// The buffer is almost full or we're finished reading the file - so save buffer
				if (first)
				{
					curr_addr = start_addr;
					if (start_addr < end_addr)
					{
						// Delete current selection and insert new data
						GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
						GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this, TRUE);
					}
					else
						GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this);
					first = false;
				}
				else
				{
					ASSERT(curr_addr != -1);
					GetDocument()->Change(mod_insert, curr_addr, pp - file_data + len, file_data, 0, this, TRUE);
				}
				curr_addr += pp - file_data + len;
				pp = file_data;
			}
			else
				pp += len;

			if (len <= 0)
				break;
		}
		delete[] file_data;

		if (!rih.Error().IsEmpty())
			TaskMessageBox("Import Error", rih.Error());

		theApp.SaveToMacro(km_import_intel, file_name);

		SetSel(addr2pos(start_addr), addr2pos(curr_addr));
		DisplayCaret();
	}
}

void CHexEditView::OnUpdateImportIntel(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only()); // disallow insert if file is read only
}

void CHexEditView::OnExportIntel()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());
	if (start == end)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("Nothing selected",
			"Intel export uses the current selection.  As there is "
			"no selection, no file was written.");
		theApp.mac_error_ = 10;
		return;
	}

	FILE_ADDRESS last_address;        // Byte past the last address to be written out
	if (theApp.export_base_addr_ < 0)
		last_address = end;
	else
		last_address = end - start + theApp.export_base_addr_;

	// Check that end address is not too big for Intel Hex Record
	// Addresses must be within 16 bit range
	if (last_address > 0x10000)
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Address Too Big", "End address too big for Intel hex address field!");
		theApp.mac_error_ = 10;
		return;
	}

	// Get the file name to write the selection to
	CHexFileDialog dlgFile("ExportFileDlg", HIDD_FILE_EXPORT_INTEL, FALSE, NULL, theApp.current_export_,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
						theApp.GetCurrentFilters(), "Export", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Export Intel Hex";

	if (dlgFile.DoModal() != IDOK)
	{
		theApp.mac_error_ = 2;
		return;
	}

	theApp.current_export_ = dlgFile.GetPathName();

	// Write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	CWriteIntelHex wih(theApp.current_export_,
					  theApp.export_base_addr_ >= 0 ? theApp.export_base_addr_ : long(start), 
					  theApp.export_line_len_);
	if (!wih.Error().IsEmpty())
	{
		TaskMessageBox("Export Error", wih.Error());
		theApp.mac_error_ = 10;
		theApp.current_export_.Empty();
	}
	else
	{
		unsigned char *buf = new unsigned char[4096];
		size_t len;

		CHECK_SECURITY(16);

		FILE_ADDRESS curr;
		for (curr = start; curr < end; curr += len)
		{
			len = min(4096, int(end - curr));
			VERIFY(GetDocument()->GetData(buf, len, curr) == len);

			wih.Put(buf, len);
			if (!wih.Error().IsEmpty())
			{
				TaskMessageBox("Export Error", wih.Error());
				theApp.mac_error_ = 10;
				theApp.current_export_.Empty();
				break;
			}
		}
		delete[] buf;
		ASSERT(curr == end);

		if (theApp.mac_error_ < 10)
			theApp.SaveToMacro(km_write_file, 7);
	}
}

void CHexEditView::OnUpdateExportIntel(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	if (start >= end)
	{
		// We can't export if there is no selection at all
		pCmdUI->Enable(FALSE);
		return;
	}

	// Work out the last address that needs to be exported
	if (theApp.export_base_addr_ >= 0)
		end = end - start + theApp.export_base_addr_;

	// Addresses must be within 16 bit range
	pCmdUI->Enable(end <= 0x10000);
}

void CHexEditView::OnImportHexText()
{
	// Get name of file to import
	CHexFileDialog dlgFile("ImportFileDlg", HIDD_FILE_IMPORT_HEX, TRUE, NULL, theApp.current_import_,
						OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
						theApp.GetCurrentFilters(), "Import", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Import Hex Text File";

	if (dlgFile.DoModal() == IDOK)
	{
		theApp.current_import_ = dlgFile.GetPathName();

		do_hex_text(theApp.current_import_);
	}
}

void CHexEditView::do_hex_text(CString file_name)
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (check_ro("import an hex text file") || check_ovr("import a file"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CHECK_SECURITY(30);
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	CStdioFile ff;                      // Text file we are reading from
	CFileException fe;                  // Stores file exception info

	// Open the text (input) file
	if (!ff.Open(file_name, CFile::modeRead|CFile::shareDenyWrite|CFile::typeText, &fe))
	{
		TaskMessageBox("Import Error", ::FileErrorMessage(&fe, CFile::modeRead));
		theApp.mac_error_ = 10;
		return;
	}
	__int64 file_len = CFile64::GetSize(file_name);
	__int64 file_done = 0;              // amt of file read so far

	// Set up handling of a large input hex file by writing result to one of our temp files
	CFile64 fout;                      // output file (only used if input file is big)
	char temp_file[_MAX_PATH]; temp_file[0] = '\0';

	// Test if the file is very big
	if (file_len > (3*16*1024*1024) && GetDocument()->DataFileSlotFree())  // assuming 3 chars/byte ~= 16 Mb
	{
		// Create a file to store the bytes
		char temp_dir[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		if (!fout.Open(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary, &fe))
		{
			TaskMessageBox("Import Error", ::FileErrorMessage(&fe, CFile::modeWrite));
			theApp.mac_error_ = 10;
			return;
		}
	}
	else if (file_len > 3*128*1024*1024)
	{
		// Warn of possible memory shortage
		if (TaskMessageBox("Large File Warning", 
							"HexEdit Pro is out of temporary file "
							"handles and reading such a large file "
							"may cause memory exhaustion.  Please click "
							"\"No\" and save the file to free handles "
							"or click \"Yes\" to continue.\n\n"
							"Do you want to continue?",
							MB_YESNO) != IDYES)
		{
			theApp.mac_error_ = 5;
			return;
		}
	}

	// Input buffer (read from hex text file)
	char buffer[520];                   // Where read string is stored
	char *pp;                           // Ptr into read string
	long line_no = 0;                   // Current text line read

	// We need to store the data in fairly large chunks otherwise the doc undo
	// stack becomes very full and slows things down
	size_t data_len = 32768;
	unsigned char *file_data = NULL;    // work buffer
	try
	{
		file_data = new unsigned char[data_len];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		return;
	}
	unsigned char *pout = file_data;   // Ptr into file_data where we are currently storing

	bool first(true);               // is this the first line read from the file
	FILE_ADDRESS start_addr, end_addr, curr_addr = -1;

	GetSelAddr(start_addr, end_addr);  // Get the current selection

	buffer[0] = '\0'; pp = buffer;
	for (;;)  // loop on output bytes (one or two input hecx digits)
	{
		// Skip whitespace/unused characters etc
		while (*pp != '\0' && !isalpha(*pp) && !isdigit(*pp))
			++pp;

		// If at end of string get the next line from the file
		while (*pp == '\0')  // skip strings till we get a non-empty one
		{
			file_done += pp - buffer + 2;
			try
			{
				pp = ff.ReadString(buffer, sizeof(buffer)-1);
				++line_no;
			}
			catch (CFileException *pfe)
			{
				TaskMessageBox("File Read Error", ::FileErrorMessage(pfe, CFile::modeRead));
				pfe->Delete();
				goto error_return;
			}

			if (pp == NULL)
				goto end_file;                  // EOF

			// Skip leading whitespace/unused characters etc
			while (*pp != '\0' && !isalpha(*pp) && !isdigit(*pp))
				++pp;
		}

		if (!isxdigit(*pp))
		{
			CString ss;
			ss.Format("Unexpected alpha characters in hex text file at line %ld", long(line_no));
			TaskMessageBox("Import Hex Error", ss);
			goto error_return;
		}

		if (isdigit(*pp))
			*pout  = *pp - '0';
		else if (isupper(*pp))
			*pout  = *pp - 'A' + 10;
		else
			*pout  = *pp - 'a' + 10;
		++pp;

		// If pair of hex digits read as 2 nybbles of a byte
		if (isxdigit(*pp))
		{
			if (isdigit(*pp))
				*pout = (*pout << 4) | (*pp - '0');
			else if (isupper(*pp))
				*pout = (*pout << 4) | (*pp - 'A' + 10);
			else
				*pout = (*pout << 4) | (*pp - 'a' + 10);
			++pp;
		}

		// Check if the output buffer is full
		if (++pout >= file_data + data_len)
		{
			ASSERT(pout == file_data + data_len);

			// The buffer is almost full or we're finished reading the file - so save buffer
			if (fout.GetHandle() != INVALID_HANDLE_VALUE)
			{
				// Writing to temp file
				try
				{
					fout.Write(file_data, data_len);
				}
				catch (CFileException *pfe)
				{
					TaskMessageBox("Import Error", ::FileErrorMessage(pfe, CFile::modeWrite));
					pfe->Delete();
					fout.Close();
					remove(temp_file);
					goto error_return;
				}

				if (AbortKeyPress() &&
					TaskMessageBox("Abort import?", 
						"You have interrupted the hex text import.\n\n"
						"Do you want to stop the process?",MB_YESNO) == IDYES)
				{
					fout.Close();
					remove(temp_file);
					goto error_return;
				}

				mm->Progress(int((file_done*100)/file_len));
			}
			else if (first)
			{
				// First in memory block to be stored - may replace the current selection
				ASSERT(curr_addr == -1);
				curr_addr = start_addr;
				if (start_addr < end_addr)
				{
					// Delete current selection and insert new data
					GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
					GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this, TRUE);
				}
				else
					GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this);
				first = false;
			}
			else
			{
				// Next in memory block - just insert it
				ASSERT(curr_addr != -1);
				GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this, TRUE);
			}
			curr_addr += pout - file_data;

			pout = file_data;
		}
	}

end_file:
	// Write out any partial buffer at end
	if (pout > file_data)
	{
		if (fout.GetHandle() != INVALID_HANDLE_VALUE)
		{
			// Writing to temp file
			try
			{
				fout.Write(file_data, pout - file_data);
			}
			catch (CFileException *pfe)
			{
				TaskMessageBox("Read Error", ::FileErrorMessage(pfe, CFile::modeWrite));
				pfe->Delete();
				fout.Close();
				remove(temp_file);
				goto error_return;
			}
		}
		else if (first)
		{
			ASSERT(curr_addr == -1);
			curr_addr = start_addr;
			if (start_addr < end_addr)
			{
				// Delete current selection and insert new data
				GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
				GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this, TRUE);
			}
			else
				GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this);
			first = false;
		}
		else
		{
			ASSERT(curr_addr != -1);
			GetDocument()->Change(mod_insert, curr_addr, pout - file_data, file_data, 0, this, TRUE);
		}
		curr_addr += pout - file_data;
	}

	if (fout.GetHandle() != INVALID_HANDLE_VALUE)
	{
		FILE_ADDRESS fout_len = fout.GetLength();
		fout.Close();      // close the file so we can use it

		// Add the temp file to the document
		int idx = GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);

		if (start_addr < end_addr)
		{
			// Delete current selection and insert new data
			GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
			GetDocument()->Change(mod_insert_file, start_addr, fout_len, NULL, idx, this, TRUE);
		}
		else
			GetDocument()->Change(mod_insert_file, start_addr, fout_len, NULL, idx, this);

		// NOTE: curr_data is not used when writing to temp file except below (in selecting
		// the inserted data) - so we have to set it here.
		curr_addr = start_addr + fout_len;
	}

	delete[] file_data;
	mm->Progress(-1);  // disable progress bar
	SetSel(addr2pos(start_addr), addr2pos(curr_addr));
	DisplayCaret();

	theApp.SaveToMacro(km_import_text, file_name);
	return;

error_return:
	delete[] file_data;
	mm->Progress(-1);  // disable progress bar
	theApp.mac_error_ = 10;
}

void CHexEditView::OnUpdateImportHexText(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only()); // disallow insert if file is read only
}

void CHexEditView::OnExportHexText()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the selection
	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	ASSERT(start_addr >= 0 && start_addr <= end_addr && end_addr <= GetDocument()->length());
	if (start_addr >= end_addr)
	{
		// Nothing selected, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("Nothing selected",
			"The Export Hex Text commands uses the current selection.  "
			"As there is no selection, no file was written.");
		theApp.mac_error_ = 10;
		return;
	}

	// Get the file name to write the selection to
	CHexFileDialog dlgFile("ExportFileDlg", HIDD_FILE_EXPORT_HEX, FALSE, NULL, theApp.current_export_,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NOCHANGEDIR,
						theApp.GetCurrentFilters(), "Export", this);

	// Set up the title of the dialog
	dlgFile.m_ofn.lpstrTitle = "Export Hex Text";

	if (dlgFile.DoModal() != IDOK)
	{
		theApp.mac_error_ = 2;
		return;
	}

	theApp.current_export_ = dlgFile.GetPathName();

	// Now write to the file
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	CHECK_SECURITY(48);

	CFile64 ff;
	CFileException fe;                      // Stores file exception info

	// Open the file to export to
	if (!ff.Open(theApp.current_export_,
				 CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary,
				 &fe))
	{
		TaskMessageBox("Export Error", ::FileErrorMessage(&fe, CFile::modeWrite));
		theApp.mac_error_ = 10;
		theApp.current_export_.Empty();
		return;
	}

	// This could be perhaps 2 or 3 times faster by buffering more than a line of text at a time

	// Buffer used to hold bits of the binary file to convert
	unsigned char *buf = NULL;  // Buffer to hold some input
	char *out = NULL;           // Buffer for output of one line of text
	unsigned char *pin;
	FILE_ADDRESS curr;
	size_t len;

	try
	{
		buf = new unsigned char[theApp.export_line_len_];
		out = new char[3*theApp.export_line_len_ + 5];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		goto func_return;
	}

	const char *hex;
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	clock_t last_checked = clock();

	for (curr = start_addr; curr < end_addr; curr += len)
	{
		// Get the data bytes
		len = size_t(min(FILE_ADDRESS(theApp.export_line_len_), end_addr - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);

		// Convert to hex text
		char *pout = out;

		for(pin = buf; pin < buf+len; ++pin)
		{
			*pout++ = hex[(*pin>>4)&0xF];
			*pout++ = hex[*pin&0xF];
			*pout++ = ' ';
		}
		*pout++ = '\r';
		*pout++ = '\n';

		// Write the string to the file
		try
		{
			ff.Write(out, pout - out);
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("File Write Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();
			theApp.mac_error_ = 10;
			goto func_return;
		}

		if (AbortKeyPress() &&
			TaskMessageBox("Abort export?", 
				"You have interrupted the hex text export.\n\n"
				"Do you want to stop the process?",MB_YESNO) == IDYES)
		{
			theApp.mac_error_ = 10;
			goto func_return;
		}

		if (double(clock() - last_checked)/CLOCKS_PER_SEC > 3) // update every 3 secs
		{
			mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			last_checked = clock();
		}
	}
	ASSERT(curr == end_addr);

func_return:
	ff.Close();
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
	if (out != NULL)
		delete[] out;

	if (theApp.mac_error_ < 5)
		theApp.SaveToMacro(km_write_file, 10);
}

void CHexEditView::OnUpdateExportHexText(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	pCmdUI->Enable(start < end);
}

// This is here only so we can put it in context menu for view
void CHexEditView::OnEditFind()
{
	((CMainFrame *)AfxGetMainWnd())->OnEditFind();
}

// This is here only so we can put it in context menu for view
void CHexEditView::OnEditReplace()
{
	((CMainFrame *)AfxGetMainWnd())->OnEditReplace();
}

// This is here only so we can put it in context menu for view
void CHexEditView::OnEditGoto()
{
	((CMainFrame *)AfxGetMainWnd())->OnEditGoto();
}

void CHexEditView::OnCalcSel()
{
	((CMainFrame *)AfxGetMainWnd())->OnCalcSel();
}

void CHexEditView::OnUpdateCalcSel(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	pCmdUI->Enable(end_addr - start_addr < 9);
}

void CHexEditView::OnEditCut()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar

	// Can't delete if view is read only or in overtype mode
	if (check_ro("cut to the clipboard") || check_ovr("cut to the clipboard"))
		return;

	// Copy the selection to the clipboard and then delete it
	if (!CopyToClipboard())
	{
		ASSERT(aa->mac_error_ > 0);             // There must have been an error
		return;
	}

	// Handle deletion of chars at caret or selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start < end && end <= GetDocument()->length());
	GetDocument()->Change(mod_delforw, start, end-start,
							  NULL, 0, this);
	DisplayCaret();                     // Make sure caret is visible

	CHECK_SECURITY(101);

	aa->SaveToMacro(km_cut);
}

void CHexEditView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	if (GetDocument()->read_only())
		pCmdUI->Enable(FALSE);          // disallow cut if file is read only
	else
		OnUpdateClipboard(pCmdUI);
}

void CHexEditView::OnEditCopy()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar

	if (CopyToClipboard())
		aa->SaveToMacro(km_copy);
}

// Update handler that turns on certain user interface options (Copy etc) if there
// is a selection -- ie. there is something available to be placed on the clipboard
void CHexEditView::OnUpdateClipboard(CCmdUI* pCmdUI)
{
	// Is there any text selected?
	CPointAp start, end;
	GetSel(start, end);
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start != end ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(start != end);
}

bool CHexEditView::CopyToClipboard()
{
	// Get the addresses of the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	// Clipboard setup and error checking
	if (!copy2cb_init(start, end))
		return false;

	// Work out which format(s) to put on the clipboard.  Generally, we place
	// both binary data ("BinaryData") and text (either the text chars if valid
	// characters or binary data as hex digits), but if the data is too big
	// it may just be stored in a temp binary file ("HexEditLargeDataTempFile").
	CB_TEXT_TYPE cb_text = (CB_TEXT_TYPE)theApp.cb_text_type_;
	bool use_file = false;   // Use our own special format if too big for clipboard
	CString strTemp;         // name of temp file is used

	if (end - start > MAX_CLIPBOARD)
	{
		use_file = true;
	}
	else if (cb_text == CB_TEXT_AUTO)
	{
		// If binary data use hex text else use binary+chars
		cb_text = is_binary(start, end) ? CB_TEXT_HEXTEXT : CB_TEXT_BIN_CHARS;
	}
	else if (cb_text == CB_TEXT_AREA)
	{
		// If in hex area use binary+chars else hex text
		cb_text = display_.char_area && display_.edit_char ? CB_TEXT_BIN_CHARS : CB_TEXT_HEXTEXT;
	}

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	bool some_succeeded = false, some_failed = false;
	if (!use_file)
	{
		// Now put text data onto the clipboard
		if (cb_text == CB_TEXT_HEXTEXT)
		{
			// Text is hex text (eg 3 chars per byte "ABC" -> " 61 62 63")
			if (copy2cb_hextext(start, end) && copy2cb_flag_text_is_hextext())
				some_succeeded = true;
			else
				some_failed = true;
		}
		else
		{
			// Text as actual chars (if valid) including char set conversion
			if (copy2cb_text(start, end))
				some_succeeded = true;
			else
				some_failed = true;

			// And also as binary
			if (copy2cb_binary(start, end))
				some_succeeded = true;
			else
				use_file = true;   // Possibly too big error - so use temp file format
		}

	}
	if (use_file)
	{
		// Store all the data as binary in a (semi) temporary file.
		strTemp = copy2cb_file(start, end);
		if (!strTemp.IsEmpty())
			some_succeeded = true;
		else
			some_failed = true;
	}

	ASSERT(some_succeeded || some_failed);
	if (some_succeeded)
		theApp.ClipBoardAdd(end - start, strTemp);

	if (!::CloseClipboard() || some_failed)
	{
		theApp.mac_error_ = 20;
		return false;
	}

	return true;
}

// Copy to clipboard as hex text
void CHexEditView::OnCopyHex()
{
	// Get the addresses of the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	// Clipboard setup and error checking
	if (!copy2cb_init(start, end))
		return;

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	bool ok = copy2cb_hextext(start, end);

	if (::CloseClipboard() && ok)
	{
		theApp.ClipBoardAdd((end - start)*3);
		theApp.SaveToMacro(km_copy_hex);
	}
	else
		theApp.mac_error_ = 20;
}

// Set up clipboard ready for having data added to it.
// start,end = the part of the file to save (ie the selection)
// If there is any problem it returns false after
// displaying an error message and setting macro error level.
bool CHexEditView::copy2cb_init(FILE_ADDRESS start, FILE_ADDRESS end)
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Check for problems with selection size
	if (start == end)
	{
		ASSERT(theApp.playing_);  // Macro might have recorded Copy but when played there is no selection
		// Copy to clipboard while nothing selected, presumably in macro playback
		TaskMessageBox("Nothing selected",
			"Copying to the clipboard uses the current selection.  "
			"As there is no selection, nothing was copied.");
		theApp.mac_error_ = 10;
		return false;
	}
	if (end-start > 8000000L)
	{
		CString ss;
		ss.Format("Do you really want to put %sbytes on\n"
				  "the clipboard?  This may take some time.",
				  NumScale(double(end-start)));
		if (TaskMessageBox("Large Clipboard Size Warning", ss, MB_YESNO) != IDYES)
		{
			theApp.mac_error_ = 5;
			return false;
		}
	}
	// Now open and empty the clipboard ready for copying data into
	if (!OpenClipboard())
	{
		AfxMessageBox("The clipboard is in use!");
		theApp.mac_error_ = 10;
		return false;
	}
	if (!::EmptyClipboard())
	{
		AfxMessageBox("Could not delete previous contents of the clipboard!");
		::CloseClipboard();
		theApp.mac_error_ = 10;
		return false;
	}
	return true;
}

// Copy selection to clipboard as text which can be pasted into a text editor etc.
// Invalid text characters (eg nul byte) are not copied.
// If current display mode is EBCDIC then the characters are converted from
// EBCDIC to ASCII as they are added.
bool CHexEditView::copy2cb_text(FILE_ADDRESS start, FILE_ADDRESS end)
{
	// Get windows memory to allow data to be put on clipboard
	HANDLE hh;                                      // Windows handle for memory
	unsigned char *p_cb, *pp;                       // Ptrs to + within the clipboard mem
	if ((hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size_t(end-start+1))) == NULL ||
		(p_cb = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) == NULL)
	{
		AfxMessageBox("Not enough memory to add text to clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Copy the data from the document to the global memory
	unsigned char * buf = new unsigned char[clipboard_buf_len];
	size_t len;
	pp = p_cb;
	FILE_ADDRESS curr;
	for (curr = start; curr < end; curr += len)
	{
		len = min(clipboard_buf_len, int(end - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);
		// Copy all characters in buffer to clipboard memory (unless nul)
		unsigned char *end_buf = buf + len;
		if (display_.char_set != CHARSET_EBCDIC)
		{
			for (unsigned char *ss = buf; ss < end_buf; ++ss)
				if (*ss != '\0')
					*pp++ = *ss;
		}
		else
		{
			for (unsigned char *ss = buf; ss < end_buf; ++ss)
				if (e2a_tab[*ss] != '\0')
					*pp++ = e2a_tab[*ss];
		}
	}
	ASSERT(curr == end);
	delete[] buf;

	// If pp has not been incremented then no valid characters were copied
	if (pp == p_cb)
	{
		theApp.mac_error_ = 1;
		((CMainFrame *)AfxGetMainWnd())->
			StatusBarText("Warning: no valid text bytes - no text placed on clipboard");
	}
	*pp ='\0';

	if (::SetClipboardData(CF_TEXT, hh) == NULL)
	{
		::GlobalFree(hh);
		AfxMessageBox("Could not place text data on clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
	::GlobalUnlock(hh);

	return true;
}

// Add another format to clipboard that does not contain any data but just indi-
// cates that the current text (characters) are hex text that should paste as binary.
bool CHexEditView::copy2cb_flag_text_is_hextext()
{
	char *pp;
	HANDLE hh;
	UINT flag_format;
	if ((flag_format = ::RegisterClipboardFormat(CHexEditApp::flag_format_name)) == 0 ||
		(hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, 64)) == NULL ||
		(pp = reinterpret_cast<char *>(::GlobalLock(hh))) == NULL)
	{
		AfxMessageBox("Error adding flag data to clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Put some dummy data on the clipboard
	strncpy(pp, "Just indicates that the current text data (CF_TEXT) is hex text", 64);

	if (::SetClipboardData(flag_format, hh) == NULL)
	{
		::GlobalFree(hh);
		AfxMessageBox("Could not place flag data on clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
	::GlobalUnlock(hh);
	return true;
}

// Copy to the clipboard in our own custom format "HexEditLargeDataTempFile".
// This creates a Windows temporary file with all the data and just
// puts the file name onto the clipboard.  (The temp file is deleted
// later when the clipboard changes or when HexEdit exits.)
// It returns the name of the temp file or an empty string if there is
// some sort of error, the most likely being a file error.
CString CHexEditView::copy2cb_file(FILE_ADDRESS start, FILE_ADDRESS end)
{
	CString TempFileName;

	// Create a temp file and write the data to it.
	{
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);
		TempFileName = temp_file;
	}

	if (!GetDocument()->WriteData(TempFileName, start, end))
	{
		AfxMessageBox("Error writing clipboard to disk");
		theApp.mac_error_ = 10;
		return CString();
	}

	HANDLE hh;                                      // Windows handle for memory
	unsigned char *pp;                              // Actual pointer to the memory

	// Create the custom clipboard format (or get it if it already exists)
	// and get the memory to store the file name.
	UINT temp_format;
	if ((temp_format = ::RegisterClipboardFormat(CHexEditApp::temp_format_name)) == 0 ||
		(hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size_t(TempFileName.GetLength()+4))) == NULL ||
		(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) == NULL)
	{
		AfxMessageBox("Not enough memory to add temp data to clipboard");
		theApp.mac_error_ = 10;
		return CString();
	}

	// Put the length of the file name
	long *pl = reinterpret_cast<long *>(pp);
	*pl = TempFileName.GetLength();

	// Put the filename
	memcpy(pp+4, TempFileName.GetBuffer(), TempFileName.GetLength());

	if (::SetClipboardData(temp_format, hh) == NULL)
	{
		::GlobalFree(hh);
		AfxMessageBox("Could not place custom data on clipboard");
		theApp.mac_error_ = 10;
		return CString();
	}

	// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
	::GlobalUnlock(hh);

	return TempFileName;
}

// Copy the selection to the clipboard in custom "BinaryData" format.
// This is the same format as used by the Visual Studio hex editor,
// simply consisting of a length (32-bit integer) followed by the bytes.
// May return false due to errors such as insufficient memory, whence
// a message has been shown to the user and mac_error_ has been set.
bool CHexEditView::copy2cb_binary(FILE_ADDRESS start, FILE_ADDRESS end)
{
	HANDLE hh;                                      // Windows handle for memory
	unsigned char *pp;                              // Actual pointer to the memory

	// Create the "BinaryData" clipboard format (or get it if it already exists)
	// then get windows memory to allow binary data to be put on clipboard
	// then lock the memory
	UINT bin_format;
	if ((bin_format = ::RegisterClipboardFormat(CHexEditApp::bin_format_name)) == 0 ||
		(hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size_t(end-start+4))) == NULL ||
		(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) == NULL)
	{
		AfxMessageBox("Not enough memory to add binary data to clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Add the binary data length to first 4 bytes of BinaryData clipboard memory
	long *pl = reinterpret_cast<long *>(pp);
	*pl = long(end - start);

	// Copy the data from the document to the global memory
	VERIFY(GetDocument()->GetData(pp+4, size_t(end - start), start) == end - start);

	if (::SetClipboardData(bin_format, hh) == NULL)
	{
		::GlobalFree(hh);
		AfxMessageBox("Could not place binary data on clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
	::GlobalUnlock(hh);

	return true;
}

// Copy to clipboard as hex text, ie, each byte is stored as
// 2 hex digits + also includes spaces etc.
bool CHexEditView::copy2cb_hextext(FILE_ADDRESS start, FILE_ADDRESS end)
{
	// Work out the amount of memory needed (may be slightly more than needed).
	FILE_ADDRESS mem_needed = hex_text_size(start, end);

	// Get windows memory to allow data to be put on clipboard
	HANDLE hh;                              // Windows handle for memory
	char *p_cb, *pp;                        // Ptr to start and within the clipboard text

	if ((hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, DWORD(mem_needed))) == NULL ||
		(p_cb = reinterpret_cast<char *>(::GlobalLock(hh))) == NULL)
	{
		AfxMessageBox("Not enough memory to add hex text to clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	unsigned char cc;
	const char *hex;
	if (theApp.hex_ucase_)
		hex = "0123456789ABCDEF?";
	else
		hex = "0123456789abcdef?";

	pp = p_cb;

	FILE_ADDRESS curr;
	for (curr = start; curr < end; )
	{
		VERIFY(GetDocument()->GetData(&cc, 1, curr) == 1);
		*pp++ = hex[(cc>>4)&0xF];
		*pp++ = hex[cc&0xF];
		*pp++ = ' ';
		if ((++curr + offset_)%rowsize_ == 0)
		{
			*pp++ = '\r';
			*pp++ = '\n';
		}
	}
	if ((curr + offset_)%rowsize_ != 0)
	{
		// Add line termination at end
		*pp++ = '\r';
		*pp++ = '\n';
	}
	*pp ='\0';

	if (::SetClipboardData(CF_TEXT, hh) == NULL)
	{
		::GlobalFree(hh);
		AfxMessageBox("Could not place hex text data on clipboard");
		theApp.mac_error_ = 10;
		return false;
	}

	// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
	::GlobalUnlock(hh);

	return true;
}

bool CHexEditView::is_binary(FILE_ADDRESS start, FILE_ADDRESS end)
{
	unsigned char buf[8192];
	size_t len;
	for (FILE_ADDRESS curr = start; curr < end; curr += len)
	{
		// Get the next buffer full from the document
		len = size_t(min(sizeof(buf), end - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);

		// For now we only consider data with a null byte to be binary
		// since the clipboard can actually have any other character
		// added to it in text format.  However, it may make sense to be
		// more restrictive later.
		if (memchr(buf, '\0', len) != NULL)
			return true;
	}
	return false;
}

// Copy to the clipboard as C source or other formatted text
void CHexEditView::OnCopyCchar()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Get the addresses of the selection
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());
	if (start == end)
	{
		// Copy to clipboard while nothing selected, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("Nothing selected",
			"This command uses the current selection.  "
			"As there is no selection, nothing was copied.");
		theApp.mac_error_ = 10;
		return;
	}

	int addr_flags = Bin2Src::NONE;
	if (hex_width_ > 0)
		addr_flags |= Bin2Src::HEX_ADDRESS;
	if (dec_width_ > 0)
		addr_flags |= Bin2Src::DEC_ADDRESS;
	if (num_width_ > 0)
		addr_flags |= Bin2Src::LINE_NUM;
	if (display_.addrbase1)
		addr_flags |= Bin2Src::ADDR1;

	CCopyCSrc dlg(GetDocument(), start, end, rowsize_, offset_, addr_flags);
	if (dlg.DoModal() == IDOK)
	{
		do_copy_src(dlg.for_, dlg.type_,
			dlg.type_ == CCopyCSrc::FLOAT ? dlg.float_size_ : dlg.int_size_,
			dlg.int_type_,
			dlg.big_endian_,
			dlg.show_address_,
			dlg.align_cols_,
			dlg.indent_);
	}
}

// do_copy_src: creates text on the clipboard from the current selection based on parameters passed
//   src_for = 0 (General formatting), 1 (C/C++ formatting)
//   src_type = CCopyCSrc::STRING, CCopyCSrc::CHAR, CCopyCSrc::INT, or CCopyCSrc::FLOAT
//   src_size = 0,1,2,3 for 4 sizes of int, or 0,1 for 2 sizes of float
//   int_type = 0,1,2,3 for how the ints are to be output
//   big_endian = determines the byte order for ints and floats
void CHexEditView::do_copy_src(int src_for, int src_type, int src_size, int int_type, 
							   BOOL big_endian, BOOL show_address, BOOL align_cols, int indent)
{
	FILE_ADDRESS start, end;
	GetSelAddr(start, end);
	ASSERT(start >= 0 && start <= end && end <= GetDocument()->length());

	// Clipboard setup and error checking
	if (!copy2cb_init(start, end))
		return;

	Bin2Src formatter(GetDocument());
	Bin2Src::lang lang = Bin2Src::NOLANG;
	if (src_for == 1)
		lang = Bin2Src::C;

	Bin2Src::type typ;
	switch (src_type)
	{
	case CCopyCSrc::STRING:
		typ = Bin2Src::STRING;
		break;
	case CCopyCSrc::CHAR:
		typ = Bin2Src::CHAR;
		break;
	case CCopyCSrc::INT:
		typ = Bin2Src::INT;
		break;
	case CCopyCSrc::FLOAT:
		typ = Bin2Src::FLOAT;
		break;
	}
	int size = 1;
	if (src_type == CCopyCSrc::FLOAT)
	{
		switch(src_size)
		{
		case CCopyCSrc::FLOAT_32:
			size = 4;
			break;
		case CCopyCSrc::FLOAT_64:
			size = 8;
			break;
		case CCopyCSrc::REAL_48:
			size = 6;
			break;
		}
	}
	else if (src_type == CCopyCSrc::INT)
	{
		switch (src_size)
		{
		case CCopyCSrc::INT_8:
			size = 1;
			break;
		case CCopyCSrc::INT_16:
			size = 2;
			break;
		case CCopyCSrc::INT_32:
			size = 4;
			break;
		case CCopyCSrc::INT_64:
			size = 8;
			break;
		}
	}
	int base = 10;
	if (int_type == CCopyCSrc::INT_HEX)
		base = 16;
	else if (int_type == CCopyCSrc::INT_OCTAL)
		base = 8;
	int flags = Bin2Src::NONE;
	if (int_type == CCopyCSrc::INT_UNSIGNED || base != 10)
		flags |= Bin2Src::UNSIGNED;
	if (big_endian)
		flags |= Bin2Src::BIG_ENDIAN;
	if (align_cols)
		flags |= Bin2Src::ALIGN;

	if (show_address)
	{
		if (hex_width_ > 0)
			flags |= Bin2Src::HEX_ADDRESS;
		if (dec_width_ > 0)
			flags |= Bin2Src::DEC_ADDRESS;
		if (num_width_ > 0)
			flags |= Bin2Src::LINE_NUM;
		if (display_.addrbase1)
			flags |= Bin2Src::ADDR1;
	}

	formatter.Set(lang, typ, size, base, rowsize_, offset_, indent, flags);

	int mem_needed = formatter.GetLength(start, end);

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	{
		// Get windows memory to allow data to be put on clipboard
		HANDLE hh;                              // Windows handle for memory
		char *p_cb;                             // Ptr to start and within the clipboard text

		if ((hh = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, DWORD(mem_needed))) == NULL ||
			(p_cb = reinterpret_cast<char *>(::GlobalLock(hh))) == NULL)
		{
			AfxMessageBox("Insufficient memory for clipboard data");
			::CloseClipboard();
			theApp.mac_error_ = 10;
			return;
		}

		size_t len = formatter.GetString(start, end, p_cb, mem_needed);

		if (::SetClipboardData(CF_TEXT, hh) == NULL)
		{
			ASSERT(0);
			::GlobalFree(hh);
			AfxMessageBox("Could not place data on clipboard");
			::CloseClipboard();
			theApp.mac_error_ = 10;
			return;
		}
		theApp.ClipBoardAdd(len);

		// Note: the clipboard now owns the memory so ::GlobalFree(hh) should not be called
		::GlobalUnlock(hh);
	}

	if (!::CloseClipboard())
		theApp.mac_error_ = 20;
	else
	{
		union copy_cchar uu;
		uu.val = 0;
		uu.src_type = src_type;
		uu.src_size = src_size;
		uu.int_type = int_type;
		uu.big_endian = big_endian;
		uu.hide_address = !show_address;
		uu.indent = indent;
		uu.src_for = src_for;
		uu.align_cols = align_cols;
		theApp.SaveToMacro(km_copy_cchar, uu.val);
	}
}

// start+end define the bytes to be replaced
// pp points to the new bytes and len is the number of new bytes
void CHexEditView::do_replace(FILE_ADDRESS start, FILE_ADDRESS end, unsigned char *pp, size_t len)
{
	// Can't replace if view is read only
	if (check_ro("replace bytes"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	ASSERT(start < end && end <= GetDocument()->length());
	if (display_.overtype && end-start != len)
	{
		// xxx direct warning if GetDocument()->IsDevice()?
		if (CAvoidableDialog::Show(IDS_REPLACE_OVERTYPE,
								   "The Replace operation requires replacing bytes with data of a "
								   "different length.  This requires the window to be in insert mode."
								   "\n\nDo you want to turn off overtype mode?",
								   "", 
								   MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
		{
			theApp.mac_error_ = 10;
			return;
		}
		else if (!do_insert())
			return;
	}

	if (display_.overtype || end-start == len)
	{
		GetDocument()->Change(mod_replace, start, len, pp, 0, this);
	}
	else
	{
		GetDocument()->Change(mod_delforw, start, end-start, NULL, 0, this);
		if (len > 0)
			GetDocument()->Change(mod_insert, start, len, pp, 0, this, TRUE);
	}
	int row = 0;
	if (display_.vert_display) row = pos2row(GetCaret());
	SetSel(addr2pos(start+len, row), addr2pos(start+len, row));
	DisplayCaret();
	show_prop();                            // Make sure dialogs don't obscure our changes
	show_calc();
	show_pos();                             // Update tool bar
}

void CHexEditView::OnEditPaste()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Can't paste if view is read only
	if (check_ro("paste"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	UINT ff = 0;                                // Clipboard format number
	HANDLE hh;                                  // Handle to clipboard memory
	unsigned char *pp;                          // Pointer to actual clipboard data

	if (!OpenClipboard())
	{
		ASSERT(0);
		TaskMessageBox("Paste Error", "The clipboard is in use!");
		aa->mac_error_ = 10;
		return;
	}

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	// First check for a custom binary data format
	//  - "BinaryData" format [bin_format_name] - binary data added by us or DevStudio/VS hex editor
	//  - "HexEditLargeDataTempFile" [temp_format_name] - our binary data in a temp file
	while ((ff = EnumClipboardFormats(ff)) != 0)
	{
		CString tt;
		char name[16];
		size_t nlen = ::GetClipboardFormatName(ff, name, 15);
		name[nlen] = '\0';
		if (strcmp(name, CHexEditApp::bin_format_name) == 0)
		{
			// BINARY DATA
			if ((hh = ::GetClipboardData(ff)) != NULL &&
				(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
			{
				long *pl = reinterpret_cast<long *>(pp);
				FILE_ADDRESS start_addr, end_addr;
				GetSelAddr(start_addr, end_addr);
				int row = 0;
				if (start_addr == end_addr && display_.vert_display)
					row = pos2row(GetCaret());
				// FILE_ADDRESS addr = GetPos();
				if (display_.overtype && start_addr + *pl > GetDocument()->length())
				{
					if (CAvoidableDialog::Show(IDS_OVERTYPE,
						"This paste operation extends past the end of file.  "
						"The file cannot be extended in overtype mode.\n\n"
						"Do you want to turn off overtype mode?",
						"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
					{
						::CloseClipboard();
						aa->mac_error_ = 10;
						return;
					}
					else if (!do_insert())
					{
						::CloseClipboard();
						return;
					}
				}
				else if (display_.overtype && end_addr-start_addr != *pl)
				{
					switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
							"Pasting in overtype mode will overwrite data.  "
							"Select \"Yes\" to insert the data; select \"No\" to"
							"overwrite; or select \"Cancel\" to do nothing.\n\n"
							"Do you want to turn on insert mode?",
							"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
					{
					case IDYES:
						if (!do_insert())
						{
							::CloseClipboard();
							return;
						}
						break;
					case IDNO:
						break; // do nothing - bytes will be overwritten
					default:
						aa->mac_error_ = 5;
						::CloseClipboard();
						return;
					}
				}

				// OK make the change
				if (display_.overtype || end_addr-start_addr == *pl)
					GetDocument()->Change(mod_replace, start_addr, *pl, pp+4, 0, this);
				else if (start_addr == end_addr)
					GetDocument()->Change(mod_insert, start_addr, *pl, pp+4, 0, this);
				else
				{
					GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
					GetDocument()->Change(mod_insert, start_addr, *pl, pp+4, 0, this, TRUE);
				}
//                SetSel(addr2pos(start_addr), addr2pos(start_addr+*pl));
				SetSel(addr2pos(start_addr+*pl, row), addr2pos(start_addr+*pl, row));
				DisplayCaret();
				show_prop();                            // New char - check props
				show_calc();
				show_pos();                             // Update tool bar
				aa->SaveToMacro(km_paste);
			}
			else
				aa->mac_error_ = 20;    // It's there but couldn't get it
			::CloseClipboard();
			return;
		}
		else if (strcmp(name, CHexEditApp::temp_format_name) == 0)
		{
			// Binary data in temp file
			if ((hh = ::GetClipboardData(ff)) != NULL &&
				(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
			{
				// Get the temp file name
				CString strTemp;
				long *pl = reinterpret_cast<long *>(pp);
				memcpy(strTemp.GetBuffer(*pl), pp+4, *pl);
				strTemp.ReleaseBuffer(*pl);  // adds null byte at end
				::CloseClipboard();          // We have got everything from the cb memory

				// Make sure there is a temp file handle available. (We are going to
				// use mod_insert_file to access data directly from the temp file
				// as we don't want to read the whole (large} file into memory.)
				int idx = GetDocument()->AddDataFile(strTemp);
				if (idx == -1)
				{
					TaskMessageBox("Paste Error",
								  "HexEdit Pro is out of temporary files and "
								  "cannot paste from a temporary clipboard file.\n\n"
								  "Please save your file to free "
								  "temporary file handles and try again.");
					aa->mac_error_ = 10;
					return;
				}

				// Get the file's length so we know how much is being pasted
				CFileStatus fs;
				VERIFY(CFile64::GetStatus(strTemp, fs));

				// Get selection so we can replace it (and also to restore caret later)
				FILE_ADDRESS start_addr, end_addr;
				GetSelAddr(start_addr, end_addr);
				int row = 0;
				if (start_addr == end_addr && display_.vert_display)
					row = pos2row(GetCaret());

				// Do some file mode checks and fixes as specified by the user
				if (display_.overtype && start_addr + fs.m_size > GetDocument()->length())
				{
					if (CAvoidableDialog::Show(IDS_OVERTYPE,
						"This paste operation extends past the end of file.  "
						"The file cannot be extended in overtype mode.\n\n"
						"Do you want to turn off overtype mode?",
						"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
					{
						aa->mac_error_ = 10;
						return;
					}
					else if (!do_insert())
						return;
				}
				else if (display_.overtype && end_addr-start_addr != fs.m_size)
				{
					switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
						"Pasting in overtype mode will overwrite data.  "
						"Select \"Yes\" to insert the data; select \"No\" to"
						"overwrite; or select \"Cancel\" to do nothing.\n\n"
						"Do you want to turn on insert mode?",
						"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
					{
					case IDYES:
						if (!do_insert())
							return;
						break;
					case IDNO:
						break; /* do nothing */
					default:
						aa->mac_error_ = 5;
						return;
					}
				}

				// Insert/replace with temp data file
				if (display_.overtype)
				{
					// Effectively replace using the file length
					GetDocument()->Change(mod_delforw, start_addr, fs.m_size, NULL, 0, this);
					GetDocument()->Change(mod_insert_file, start_addr, fs.m_size, NULL, idx, this, TRUE);
				}
				else
				{
					// Wipe out any current selection then insert the data
					if (start_addr < end_addr)
						GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
					GetDocument()->Change(mod_insert_file, start_addr, fs.m_size, NULL, idx, this, TRUE);
				}
				// Restore caret and update everything due to possible new data at the caret
				SetSel(addr2pos(start_addr+fs.m_size, row), addr2pos(start_addr+fs.m_size, row));
				DisplayCaret();
				show_prop();                            // New current char - check props
				show_calc();
				show_pos();                             // Update tool bar
				aa->SaveToMacro(km_paste);
			}
			else
				aa->mac_error_ = 20;    // It's there but couldn't get it
			return;
		}
	}

	// This is here in case we need to get a buffer from and so we can delete[] it later (if not NULL)
	unsigned char *buf = NULL;

	// We did not get binary data so we are expecting text data ...
	if ((hh = ::GetClipboardData(CF_TEXT)) != NULL &&
		(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
	{
		// The text on the clipboard is handled in one of three ways:
		//  - if it is hex text we convert to binary and paste
		//  - if in EBCDIC mode we convert from ASCII to EBCDIC and paste
		//  - otherwise we just paste the ASCII bytes
		size_t len = strlen(reinterpret_cast<char *>(pp));
		UINT fmt = ::RegisterClipboardFormat(CHexEditApp::flag_format_name);  // Check for "flag" format that indicates we have hex text

		// Check if clipboard contains hex text
		bool isHexText = true;
		if (theApp.cb_text_type_ == CB_TEXT_BIN_CHARS)
			isHexText = false;                              // always treat text as bytes if using traditional clipboard format
		else if (::IsClipboardFormatAvailable(fmt) == 0)
		{
			// The clipboard just contains text but it could be hex text from somewhere so we better check
			for (int ii = 0; ii < len; ++ii)
				if (!isxdigit(pp[ii]) && !isspace(pp[ii]))
				{
					isHexText = false;
					break;
				}
		}

		if (len == 0)
		{
			TaskMessageBox("Paste Error", "The text on the clipboard is not valid ASCII text!");
			aa->mac_error_ = 10;    // Invalid text on clipboard?
			goto error_return;
		}
		else if (isHexText)
		{
			// Hex text
			buf = new unsigned char[len/2+1];
			unsigned char *pout;
			bool bFirst = true;
			for (pout = buf; *pp != '\0'; ++pp)
			{
				// Ignore anything but hex digits
				if (!isxdigit(*pp))
					continue;

				// Get the value of the digit as binary (one nybble)
				unsigned char nybble;
				if (isdigit(*pp))
					nybble = *pp - '0';
				else if (isupper(*pp))
					nybble = *pp - 'A' + 10;
				else
					nybble = *pp - 'a' + 10;

				// Each byte is created from 2 nybbles
				if (bFirst)
					*pout = nybble;
				else
				{
					*pout = (*pout << 4) | nybble;
					++pout;
				}
				bFirst = !bFirst;
			}
			if (!bFirst)
				++pout;  // lone digit is retained as low nybble of last byte

			// Set up data buffer for pasting below
			pp = buf;
			len = pout - buf;
		}
		else if (display_.char_set == CHARSET_EBCDIC)
		{
			// Copy from clipboard to temp buffer converting to EBCDIC
			buf = new unsigned char[len];
			size_t newlen = 0;
			for (size_t ii = 0; ii < len; ++ii)
				if (a2e_tab[pp[ii]] != '\0')
					buf[newlen++] = a2e_tab[pp[ii]];

			if (newlen == 0)
			{
				CAvoidableDialog::Show(IDS_INVALID_EBCDIC, "No valid EBCDIC characters were found to paste.");
				aa->mac_error_ = 2;
				goto error_return;
			}

			// Set up data buffer for adding to the file (below)
			pp = buf;
			len = newlen;
		}

		// Get current selection
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		// Check that we are not changing the file length when in overtype mode
		if (display_.overtype && start_addr + len > GetDocument()->length())
		{
			if (CAvoidableDialog::Show(IDS_OVERTYPE,
				"This paste operation extends past the end of file.  "
				"The file cannot be extended in overtype mode.\n\n"
				"Do you want to turn off overtype mode?",
				"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
			{
				aa->mac_error_ = 10;
				goto error_return;
			}
			else if (!do_insert())
				goto error_return;
		}
		else if (display_.overtype && end_addr-start_addr != len)
		{
			switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
				"Pasting in overtype mode will overwrite data.  "
				"Select \"Yes\" to insert the data; select \"No\" to"
				"overwrite; or select \"Cancel\" to do nothing.\n\n"
				"Do you want to turn on insert mode?",
				"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
			{
			case IDYES:
				if (!do_insert())
					goto error_return;
				break;
			case IDNO:
				break; /* do nothing */
			default:
				aa->mac_error_ = 5;
				goto error_return;
			}
		}

		// OK make the change
		if (display_.overtype || end_addr-start_addr == len)
			GetDocument()->Change(mod_replace, start_addr, len, pp, 0, this);
		else if (start_addr == end_addr)
			GetDocument()->Change(mod_insert, start_addr, len, pp, 0, this);
		else
		{
			GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
			GetDocument()->Change(mod_insert, start_addr, len, pp, 0, this, TRUE);
		}

//            SetSel(addr2pos(start_addr), addr2pos(start_addr+len));  // select pasted bytes
		SetSel(addr2pos(start_addr+len, row), addr2pos(start_addr+len, row));  // position caret ready to paste again
		DisplayCaret();
	}
	else
	{
		// Paste when nothing to paste, presumably in macro
		TaskMessageBox("Paste Error", "The clipboard is empty or there are no recognized clipboard formats available.");
		aa->mac_error_ = 10;
	}

error_return:
	if (buf != NULL)
		delete[] buf;
	CHECK_SECURITY(49);

	::CloseClipboard();
	// This actually records even when there were some errors & probably shouldn't
	show_prop();                            // New char - check props
	show_calc();
	show_pos();                             // Update tool bar
	aa->SaveToMacro(km_paste);
}

void CHexEditView::OnPasteAscii()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Can't insert text if view is read only or in overtype mode
	if (check_ro("paste (ASCII)"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	HANDLE hh;                                  // Handle to clipboard memory
	unsigned char *pp;                          // Pointer to actual data

	if (!OpenClipboard())
	{
		ASSERT(0);
		TaskMessageBox("Paste Error", "The clipboard is in use!");
		aa->mac_error_ = 10;
		return;
	}

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	if ((hh = ::GetClipboardData(CF_TEXT)) != NULL &&
		(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
	{
		size_t len = strlen(reinterpret_cast<char *>(pp));
		if (len > 0)                    // Bugs in other apps (eg Hedit) might cause len == 0
		{
			FILE_ADDRESS start_addr, end_addr;
			GetSelAddr(start_addr, end_addr);
			int row = 0;
			if (start_addr == end_addr && display_.vert_display)
				row = pos2row(GetCaret());
			// FILE_ADDRESS addr = GetPos();
			if (display_.overtype && start_addr + len > GetDocument()->length())
			{
				if (CAvoidableDialog::Show(IDS_OVERTYPE,
					"This paste operation extends past the end of file.  "
					"The file cannot be extended in overtype mode.\n\n"
					"Do you want to turn off overtype mode?",
					"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
				{
					::CloseClipboard();
					aa->mac_error_ = 10;
					return;
				}
				else if (!do_insert())
					return;
			}
			else if (display_.overtype && end_addr-start_addr != len)
			{
				switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
					"Pasting in overtype mode will overwrite data.  "
					"Select \"Yes\" to insert the data; select \"No\" to"
					"overwrite; or select \"Cancel\" to do nothing.\n\n"
					"Do you want to turn on insert mode?",
					"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
				{
				case IDYES:
					if (!do_insert())
						return;
					break;
				case IDNO:
					break; /* do nothing */
				default:
					aa->mac_error_ = 5;
					return;
				}
			}

			// OK make the change
			if (display_.overtype || end_addr-start_addr == len)
				GetDocument()->Change(mod_replace, start_addr, len, pp, 0, this);
			else if (start_addr == end_addr)
				GetDocument()->Change(mod_insert, start_addr, len, pp, 0, this);
			else
			{
				GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
				GetDocument()->Change(mod_insert, start_addr, len, pp, 0, this, TRUE);
			}
//            SetSel(addr2pos(start_addr), addr2pos(start_addr+len));
			SetSel(addr2pos(start_addr+len, row), addr2pos(start_addr+len, row));
			DisplayCaret();
		}
		else
			aa->mac_error_ = 20;
	}
	else
	{
		// Paste when nothing to paste, presumably in macro
		TaskMessageBox("Paste Error", "There is nothing on the clipboard to paste");
		aa->mac_error_ = 10;
	}

	::CloseClipboard();
  // This actually records even when there were some errors & probably shouldn't
	aa->SaveToMacro(km_paste_ascii);
}

void CHexEditView::OnPasteEbcdic()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Can't insert text if view is read only or in overtype mode
	if (check_ro("paste (EBCDIC)"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	HANDLE hh;                                  // Handle to clipboard memory
	unsigned char *pp;                          // Pointer to actual data

	if (!OpenClipboard())
	{
		ASSERT(0);
		AfxMessageBox("The clipboard is in use!");
		aa->mac_error_ = 10;
		return;
	}

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	if ((hh = ::GetClipboardData(CF_TEXT)) != NULL &&
		(pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
	{
		size_t len = strlen(reinterpret_cast<char *>(pp));
		if (len > 0)                    // Bugs in other apps (eg Hedit) might cause len == 0
		{
			// Copy from clipboard to temp buffer converting to EBCDIC
			unsigned char *buf = new unsigned char[len];
			size_t newlen = 0;
			for (size_t ii = 0; ii < len; ++ii)
				if (pp[ii] < 128 && a2e_tab[pp[ii]] != '\0')
					buf[newlen++] = a2e_tab[pp[ii]];
			if (newlen > 0)
			{
				// Insert the EBCDIC characters
				FILE_ADDRESS start_addr, end_addr;
				GetSelAddr(start_addr, end_addr);
				int row = 0;
				if (start_addr == end_addr && display_.vert_display)
					row = pos2row(GetCaret());
				// FILE_ADDRESS addr = GetPos();
				if (display_.overtype && start_addr + newlen > GetDocument()->length())
				{
					if (CAvoidableDialog::Show(IDS_OVERTYPE,
						"This paste operation extends past the end of file.  "
						"The file cannot be extended in overtype mode.\n\n"
						"Do you want to turn off overtype mode?",
						"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
					{
						::CloseClipboard();
						aa->mac_error_ = 10;
						return;
					}
					else if (!do_insert())
						return;
				}
				else if (display_.overtype && end_addr-start_addr != newlen)
				{
					switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
						"Pasting in overtype mode will overwrite data.  "
						"Select \"Yes\" to insert the data; select \"No\" to"
						"overwrite; or select \"Cancel\" to do nothing.\n\n"
						"Do you want to turn on insert mode?",
						"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
					{
					case IDYES:
						if (!do_insert())
							return;
						break;
					case IDNO:
						break; /* do nothing */
					default:
						aa->mac_error_ = 5;
						return;
					}
				}

				// OK make the change
				if (display_.overtype || end_addr-start_addr == newlen)
					GetDocument()->Change(mod_replace, start_addr, newlen, buf, 0, this);
				else if (start_addr == end_addr)
					GetDocument()->Change(mod_insert, start_addr, newlen, buf, 0, this);
				else
				{
					GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
					GetDocument()->Change(mod_insert, start_addr, newlen, buf, 0, this, TRUE);
				}
//                SetSel(addr2pos(start_addr), addr2pos(start_addr+newlen));
				SetSel(addr2pos(start_addr+newlen, row), addr2pos(start_addr+newlen, row));
				DisplayCaret();
			}
			delete[] buf;
		}
		else
			aa->mac_error_ = 20;
	}
	else
	{
		// Paste when nothing to paste, presumably in macro
		AfxMessageBox("There is nothing on the clipboard to paste");
		aa->mac_error_ = 10;
	}

	::CloseClipboard();
	aa->SaveToMacro(km_paste_ebcdic);  // Don't record if error (mac_error_ > 3)???
}

// Update handler that turns on user interface options (Paste etc) if there is
// text on the clipboard -- ie. there is text that can be pasted into the document
void CHexEditView::OnUpdateTextPaste(CCmdUI* pCmdUI)
{
	BOOL bEnable = !GetDocument()->read_only() && ::IsClipboardFormatAvailable(CF_TEXT);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (bEnable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(bEnable);

//    if (GetDocument()->read_only())
//        pCmdUI->Enable(FALSE);                  // Disallow paste if file is read only
//    else
//        pCmdUI->Enable(::IsClipboardFormatAvailable(CF_TEXT));
}

void CHexEditView::OnPasteUnicode()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	// Can't insert text if view is read only or in overtype mode
	if (check_ro("paste (Unicode)"))
		return;

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	HANDLE hh;                                  // Handle to clipboard memory
	wchar_t *pp;                                // Pointer to actual data

	if (!OpenClipboard())
	{
		ASSERT(0);
		AfxMessageBox("The clipboard is in use!");
		aa->mac_error_ = 10;
		return;
	}

	CWaitCursor wait;                           // Turn on wait cursor (hourglass)

	if ((hh = ::GetClipboardData(CF_UNICODETEXT)) != NULL &&
		(pp = reinterpret_cast<wchar_t *>(::GlobalLock(hh))) != NULL)
	{
		size_t len = wcslen(reinterpret_cast<wchar_t *>(pp));
		if (len > 0)                    // Bugs in other apps (eg Hedit) might cause len == 0
		{
			FILE_ADDRESS start_addr, end_addr;
			GetSelAddr(start_addr, end_addr);
			int row = 0;
			if (start_addr == end_addr && display_.vert_display)
				row = pos2row(GetCaret());
			// FILE_ADDRESS addr = GetPos();
			if (display_.overtype && start_addr + 2*len > GetDocument()->length())
			{
				if (CAvoidableDialog::Show(IDS_OVERTYPE,
					"This paste operation extends past the end of file.  "
					"The file cannot be extended in overtype mode.\n\n"
					"Do you want to turn off overtype mode?",
					"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
				{
					::CloseClipboard();
					aa->mac_error_ = 10;
					return;
				}
				else if (!do_insert())
					return;
			}
			else if (display_.overtype && end_addr-start_addr != 2*len)
			{
				switch (CAvoidableDialog::Show(IDS_PASTE_OVERTYPE,
					"Pasting in overtype mode will overwrite data.  "
					"Select \"Yes\" to insert the data; select \"No\" to"
					"overwrite; or select \"Cancel\" to do nothing.\n\n"
					"Do you want to turn on insert mode?",
					"", MLCBF_YES_BUTTON | MLCBF_NO_BUTTON | MLCBF_CANCEL_BUTTON))
				{
				case IDYES:
					if (!do_insert())
						return;
					break;
				case IDNO:
					break; /* do nothing */
				default:
					aa->mac_error_ = 5;
					return;
				}
			}

			// OK make the change
			if (display_.overtype || end_addr-start_addr == 2*len)
				GetDocument()->Change(mod_replace, start_addr, 2*len, (unsigned char *)pp, 0, this);
			else if (start_addr == end_addr)
				GetDocument()->Change(mod_insert, start_addr, 2*len, (unsigned char *)pp, 0, this);
			else
			{
				GetDocument()->Change(mod_delforw, start_addr, end_addr-start_addr, NULL, 0, this);
				GetDocument()->Change(mod_insert, start_addr, 2*len, (unsigned char *)pp, 0, this, TRUE);
			}
//            SetSel(addr2pos(start_addr), addr2pos(start_addr + 2*len));
			SetSel(addr2pos(start_addr + 2*len, row), addr2pos(start_addr + 2*len, row));
			DisplayCaret();
		}
		else
			aa->mac_error_ = 20;
	}
	else
	{
		// Paste when nothing to paste, presumably in macro
		AfxMessageBox("There is nothing on the clipboard to paste");
		aa->mac_error_ = 10;
	}

	::CloseClipboard();
	  // This actually records even when there were some errors & probably shouldn't
	aa->SaveToMacro(km_paste_unicode);
}

void CHexEditView::OnUpdateUnicodePaste(CCmdUI* pCmdUI)
{
	BOOL bEnable = !GetDocument()->read_only() && ::IsClipboardFormatAvailable(CF_UNICODETEXT);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (bEnable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(bEnable);

//    if (GetDocument()->read_only())
//        pCmdUI->Enable(FALSE);
//    else
//        pCmdUI->Enable(::IsClipboardFormatAvailable(CF_UNICODETEXT));
}

// Reset all options to the current defaults
void CHexEditView::OnDisplayReset()
{
	// Change search type in find modeless dlg if open
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (theApp.open_display_.char_set != display_.char_set)
	{
		// Character set is to be changed - so update find dlg to match
		if (theApp.open_display_.char_set == CHARSET_EBCDIC)
			mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_EBCDIC);
		else
			mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);
	}

	begin_change();

	// Change font
	undo_.push_back(view_undo(undo_font));
	// xxx TODO save any (or all) of the 3 fonts if different from default
	if (display_.FontRequired() == FONT_UCODE)
	{
		undo_.back().plf = new LOGFONT;
		*(undo_.back().plf) = mb_lf_;
		if (theApp.open_mb_plf_ != NULL)
			mb_lf_ = *theApp.open_mb_plf_;
		else
		{
			memset((void *)&mb_lf_, '\0', sizeof(mb_lf_));
			_tcscpy(mb_lf_.lfFaceName, _T("Lucida Sans Unicode"));
			mb_lf_.lfHeight = 16;
			mb_lf_.lfCharSet = DEFAULT_CHARSET;         // Only allow OEM/IBM character set fonts
		}
	}
	else if (display_.FontRequired() == FONT_OEM)
	{
		undo_.back().plf = new LOGFONT;
		*(undo_.back().plf) = oem_lf_;
		if (theApp.open_oem_plf_ != NULL)
			oem_lf_ = *theApp.open_oem_plf_;
		else
		{
			memset((void *)&oem_lf_, '\0', sizeof(oem_lf_));
			_tcscpy(oem_lf_.lfFaceName, _T("Terminal")); // The only certain OEM font?
			oem_lf_.lfHeight = 18;
			oem_lf_.lfCharSet = OEM_CHARSET;         // Only allow OEM/IBM character set fonts
		}
	}
	else
	{
		undo_.back().plf = new LOGFONT;
		*(undo_.back().plf) = lf_;
		if (theApp.open_plf_ != NULL)
			lf_ = *theApp.open_plf_;
		else
		{
			memset((void *)&lf_, '\0', sizeof(lf_));
			_tcscpy(lf_.lfFaceName, _T("Courier")); // A nice fixed (no-proportional) font
			lf_.lfHeight = 16;
			lf_.lfCharSet = ANSI_CHARSET;           // Only allow ANSI character set fonts
		}
	}

	// Change autofit
	undo_.push_back(view_undo(undo_autofit, TRUE));
	if (!display_.autofit)
		undo_.back().rowsize = rowsize_;
	else
		undo_.back().rowsize = 0;

	// Note: we don't use disp_state_ = theApp.disp_state_ as this loses edit_char and mark_char settings
	display_.hex_area = theApp.open_display_.hex_area;
	display_.char_area = theApp.open_display_.char_area;
	display_.char_set = theApp.open_display_.char_set;
	display_.control = theApp.open_display_.control;

	display_.autofit = theApp.open_display_.autofit;
	display_.dec_addr = theApp.open_display_.dec_addr;       // remove now or later?
	display_.decimal_addr = theApp.open_display_.decimal_addr;
	display_.hex_addr = theApp.open_display_.hex_area;
	display_.line_nums = theApp.open_display_.line_nums;
	display_.addrbase1 = theApp.open_display_.addrbase1;        // addresses start at 1 (not 0)

	display_.readonly = theApp.open_display_.readonly;
	display_.overtype = theApp.open_display_.overtype;

	display_.vert_display = theApp.open_display_.vert_display;
	display_.borders = theApp.open_display_.borders;

	if (GetDocument()->IsDevice())
	{
		display_.overtype = 1;  // INS not allowed
		display_.borders = 1;   // Always display borders for devices by default
	}

	// Make sure that caret and mark are not in hidden area
	ASSERT(display_.char_area || display_.hex_area);
	if (!display_.hex_area)
	{
		display_.edit_char = TRUE;
		display_.mark_char = TRUE;
	}
	else if (!display_.char_area)
	{
		display_.edit_char = FALSE;
		display_.mark_char = FALSE;
	}
	make_change(TRUE);

	if (code_page_ != theApp.open_code_page_)
	{
		undo_.push_back(view_undo(undo_codepage, TRUE));
		undo_.back().codepage = code_page_;
		SetCodePage(theApp.open_code_page_);
	}
	if (rowsize_ != theApp.open_rowsize_)
	{
		undo_.push_back(view_undo(undo_rowsize, TRUE));
		undo_.back().rowsize = rowsize_;            // Save previous rowsize for undo
		rowsize_ = theApp.open_rowsize_;
	}
	if (real_offset_ != theApp.open_offset_)
	{
		undo_.push_back(view_undo(undo_offset, TRUE));
		undo_.back().rowsize = real_offset_;        // Save previous offset for undo
		real_offset_ = theApp.open_offset_;
	}
	offset_ = rowsize_ - (rowsize_ + real_offset_ - 1) % rowsize_ - 1;
	undo_.push_back(view_undo(undo_group_by, TRUE));
	undo_.back().rowsize = group_by_;              // Save previous group_by for undo
	group_by_ = theApp.open_group_by_;

	undo_.push_back(view_undo(undo_scheme, TRUE));      // Allow undo of scheme change
	undo_.back().pscheme_name = new CString;
	*undo_.back().pscheme_name = scheme_name_;

	scheme_name_ = "";          // Force scheme reset
	set_colours();

	end_change();

	theApp.SaveToMacro(km_display_reset, unsigned __int64(0));  // Store zero to allow for future additions
}

void CHexEditView::OnEditUndo()
{
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar

	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	BOOL more = TRUE;

	while (more)
	{
		ASSERT(undo_.size() > 0 || theApp.playing_);
		more = undo_.back().previous_too;
		if (!do_undo())
		{
			theApp.mac_error_ = 10;
			return;
		}
	}

	theApp.SaveToMacro(km_undo);
}

void CHexEditView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(undo_.size() > 0);
}

void CHexEditView::OnUndoChanges()
{
	CWaitCursor wait;                           // Turn on wait cursor (hourglass)
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	BOOL more;
	BOOL last_change = is_last_change();

	do
	{
		ASSERT(undo_.size() > 0 || theApp.playing_);
		more = undo_.back().previous_too;
		if (!do_undo())
		{
			theApp.mac_error_ = 10;
			return;
		}
	} while (undo_.size() > 0 && (more || !(last_change || is_last_change())));
//    } while ((more || (last_change == is_last_change() && undo_.size() > 0));

	theApp.SaveToMacro(km_undo_changes);
}

// Check if the top "operation" on the undo stack is a file change.  Note that an operation
// consists of all elts on the stack back until one without the previous_too flag set.
BOOL CHexEditView::is_last_change()
{
#ifdef _DEBUG
	{
		// Why can't the debugger look into STL containers?
		int undo_size = undo_.size();
		for (std::vector<view_undo>::reverse_iterator rr = undo_.rbegin(); rr != undo_.rend(); ++rr)
		{
			view_undo undo_elt = *rr;
		}
	}
#endif

	// Check if the last thing on undo stack is a change
	for (std::vector<view_undo>::reverse_iterator pp = undo_.rbegin(); pp != undo_.rend(); ++pp)
	{
		if (pp->utype == undo_change)
			return TRUE;         // Top undo op is a change

		if (!pp->previous_too)
			break;               // End of top undo operation
	}
	return FALSE;
}

void CHexEditView::OnUpdateUndoChanges(CCmdUI* pCmdUI)
{
	BOOL change_present = FALSE;

	// Check if the last thing on undo stack is a change
	for (std::vector<view_undo>::iterator pp = undo_.begin(); pp != undo_.end(); ++pp)
	{
		if (pp->utype == undo_change)
		{
			change_present = TRUE;    // There is a change
			break;
		}
	}

	pCmdUI->Enable(change_present);
}

BOOL CHexEditView::do_undo()
{
	// The string 'mess' is used to display a message in the status
	// bar if it is not obvious what has been undone.
	CString mess, tmp;
	CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());

	BOOL caret_displayed = CaretDisplayed();
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	CHECK_SECURITY(191);
	if (undo_.size() == 0)
	{
		// This can only (presumably) happen during a macro
		TaskMessageBox("Can't Undo", "There is nothing to undo.");
		return FALSE;
	}

	switch (undo_.back().utype)
	{
	case undo_move:
		GoAddress(undo_.back().address);
		show_prop();
		show_calc();
		show_pos();
		DisplayCaret();         // Make sure move visible
		break;
	case undo_sel:
		end_addr = undo_.back().address;

		// Now go back to previous undo (which should be undo_move)
		undo_.pop_back();
		ASSERT(undo_.size() > 0);
		ASSERT(undo_.back().utype == undo_move);
		GoAddress(undo_.back().address, end_addr);
		show_prop();
		show_calc();
		show_pos();
		DisplayCaret();         // Make sure move visible
		break;
	case undo_change:
#if 0
		if (display_.readonly)
		{
			if (AfxMessageBox("You can't undo changes while the window is read only.\r"
							  "Do you want to turn off read only mode?",
							  MB_OKCANCEL) == IDCANCEL)
				return FALSE;
			else
				allow_mods();
		}
#endif
		// Note: flag == TRUE if this view originally made the change.
		if (!undo_.back().flag)
			mess += "Undo: changes made in different window undone ";
#ifndef NDEBUG
		if (!GetDocument()->Undo(this, undo_.back().index,
								 undo_.back().flag))
			return FALSE;
#else
		if (!GetDocument()->Undo(this, 0, undo_.back().flag))
			return FALSE;
#endif
		break;

	case undo_state:
		disp_state_ = undo_.back().disp_state;
		redo_font();
		recalc_display();
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (caret_displayed)
			DisplayCaret();                     // Keep caret within display
		DoInvalidate();

		break;

	case undo_overtype:
		ASSERT(!GetDocument()->IsDevice());
		display_.overtype = undo_.back().flag;
		if (GetDocument()->length() == 0)
			DoInvalidate();
		if (display_.overtype)
			mess += "Undo: overtype now ON ";
		else
			mess += "Undo: overtype now OFF ";
		break;

	case undo_readonly:
		display_.readonly = undo_.back().flag;
		if (display_.readonly)
			mess += "Undo: read only now ON ";
		else
			mess += "Undo: read only now OFF ";
		break;

	case undo_font:
		if (display_.FontRequired() == FONT_UCODE)
			mb_lf_ = *(undo_.back().plf);
		else if (display_.FontRequired() == FONT_OEM)
			oem_lf_ = *(undo_.back().plf);
		else
			lf_ = *(undo_.back().plf);

		if (pcv_ != NULL)
			pcv_->begin_change();

		redo_font();

		// Calculate new position based on new font size
		recalc_display();
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (caret_displayed)
			DisplayCaret();                     // Keep caret within display

		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
		mess += "Undo: font restored ";
		break;

	case undo_codepage:
		SetCodePage(undo_.back().codepage);
		DoInvalidate();
		mess += "Undo: code page restored ";
		break;

	case undo_scheme:
		tmp = scheme_name_;
		scheme_name_ = *(undo_.back()).pscheme_name;
		if (set_colours())
		{
			DoInvalidate();
			mess += "Undo: color scheme restored ";
		}
		else
		{
			scheme_name_ = tmp;
			if (::IsUs())
				TaskMessageBox("Can't Undo", "The previous color scheme was "
							  "not found.  The operation could not be undone.");
			else
				TaskMessageBox("Can't Undo", "The previous colour scheme was "
							  "not found.  The operation could not be undone.");
		}
		break;

	case undo_rowsize:
	{
		FILE_ADDRESS scroll_addr = pos2addr(GetScroll());

		if (pcv_ != NULL)
			pcv_->begin_change();

		rowsize_ = undo_.back().rowsize;
		recalc_display();
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));

		// Fix scroll place so it's about the same even though the row length has changed
		CPointAp pt = addr2pos(scroll_addr);
		pt.x = 0;
		SetScroll(pt);

		if (caret_displayed)
			DisplayCaret();     // Keep caret within display
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
	}
	break;

	case undo_group_by:
		if (pcv_ != NULL)
			pcv_->begin_change();

		group_by_ = undo_.back().rowsize;
		recalc_display();
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (caret_displayed)
			DisplayCaret();     // Keep caret within display
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
		break;

	case undo_offset:
		if (pcv_ != NULL)
			pcv_->begin_change();

		real_offset_ = undo_.back().rowsize;
		recalc_display();
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (caret_displayed)
			DisplayCaret();     // Keep caret within display
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
		break;

	case undo_autofit:
	{
		FILE_ADDRESS scroll_addr = pos2addr(GetScroll());
		if (pcv_ != NULL)
			pcv_->begin_change();

		// If rowsize has been specified then autofit is now off (undo turn on)
		display_.autofit = undo_.back().rowsize == 0;
		if (!display_.autofit)
		{
			mess += "Undo: auto fit now OFF ";
			rowsize_ = undo_.back().rowsize;
		}
		recalc_display();

		// Fix scroll place so it's about the same even though the row length has changed
		CPointAp pt = addr2pos(scroll_addr);
		pt.x = 0;
		SetScroll(pt);

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (caret_displayed)
			DisplayCaret();     // Keep caret within display
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
	}
	break;

	case undo_setmark:
		invalidate_addr_range(mark_, mark_ + 1);
		mark_ = undo_.back().address;
		if (theApp.align_rel_ && mark_ != GetDocument()->base_addr_)
		{
			GetDocument()->StopSearch();
			GetDocument()->base_addr_ = GetSearchBase();
			GetDocument()->StartSearch();
		}
		invalidate_addr_range(mark_, mark_ + 1);
		show_calc();                    // Status of some buttons may have changed when mark_ moves
		break;

	case undo_highlight:
		hl_set_ = *(undo_.back().phl);
		DoInvalidate();
		break;
	case undo_unknown:
	default:
		ASSERT(0);
		mess += " Unknown undo! ";
	}
	undo_.pop_back();
	mm->StatusBarText(mess);
	return TRUE;
}

void CHexEditView::OnAddrToggle()
{
	begin_change();
	display_.decimal_addr = display_.hex_addr;  // Display decimal addr if currently displaying hex or both
	display_.hex_addr = !display_.decimal_addr;
	make_change();
	end_change();

	theApp.SaveToMacro(km_addr);
}

void CHexEditView::OnUpdateAddrToggle(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!display_.hex_addr);
}

void CHexEditView::OnGraphicToggle()
{
	if (!display_.char_area || display_.char_set == CHARSET_EBCDIC)
	{
		CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

		// Can't toggle graphic chars, presumably in macro playback
		ASSERT(aa->playing_);
		if (!display_.char_area)
			TaskMessageBox("Graphic Chars", "You can't display graphic characters without the char area.");
		else
			TaskMessageBox("Graphic Chars", "Graphic characters are not supported for EBCDIC.");
		aa->mac_error_ = 2;
		return;
	}

	begin_change();
	if (display_.char_set == CHARSET_ASCII)
		display_.char_set = CHARSET_ANSI;
	else
		display_.char_set = CHARSET_ASCII;
	make_change();
	end_change();

	CHECK_SECURITY(22);
	theApp.SaveToMacro(km_graphic);
}

void CHexEditView::OnUpdateGraphicToggle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(display_.char_area && display_.char_set != CHARSET_EBCDIC);
	pCmdUI->SetCheck(display_.char_set == CHARSET_ANSI || display_.char_set == CHARSET_OEM);
}

void CHexEditView::OnCharToggle()
{
	do_chartoggle();
}

void CHexEditView::do_chartoggle(int state /*=-1*/)
{
	if (display_.vert_display)
	{
		TaskMessageBox("Char Area", "You can't toggle char area in stacked mode");
		theApp.mac_error_ = 2;
		return;
	}

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	begin_change();

	// Change state of char area flag
	if (state == -1)
		display_.char_area = !display_.char_area;
	else if (state != display_.char_area)
		display_.char_area = state;

	// Make sure things are kept consistent
	if (!display_.char_area && display_.edit_char)
	{
		display_.edit_char = FALSE;
		SetHorzBufferZone(2);                      // Allow more room in hex area
	}

	if (!display_.char_area && !display_.hex_area)
	{
		display_.hex_area = TRUE;
	}

	make_change();
	end_change();

	aa->SaveToMacro(km_char_area, state);
}

void CHexEditView::OnUpdateCharToggle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!display_.vert_display);
	pCmdUI->SetCheck(display_.char_area);
}

void CHexEditView::do_hextoggle(int state /*=-1*/)
{
	if (display_.vert_display)
	{
		TaskMessageBox("Hex Area", "You can't toggle hex area in stacked mode");
		theApp.mac_error_ = 2;
		return;
	}

	begin_change();
	if (state == -1)
		display_.hex_area = !display_.hex_area;
	else if (state != display_.hex_area)
		display_.hex_area = state;

	// Save previous value of display hex for undo and also swap edit area if nec.
	if (!display_.hex_area && !display_.edit_char)
	{
		display_.edit_char = TRUE;
		SetHorzBufferZone(1);
	}

	if (!display_.hex_area && !display_.char_area)
	{
		display_.char_area = TRUE;
	}

	make_change();
	end_change();

	theApp.SaveToMacro(km_hex_area, state);
}

void CHexEditView::OnOemToggle()
{
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		// Can't toggle OEM chars, presumably in macro playback
		ASSERT(theApp.playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Display Error", "You can't display OEM/ANSI graphic characters without the char area.");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Display Error", "Graphic characters are not supported for EBCDIC.");
		theApp.mac_error_ = 2;
		return;
	}

	begin_change();
	if (display_.char_set == CHARSET_OEM)
		display_.char_set = CHARSET_ANSI;
	else
		display_.char_set = CHARSET_OEM;
	make_change();
	end_change();

	theApp.SaveToMacro(km_oem);
}

void CHexEditView::OnUpdateOemToggle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((display_.vert_display || display_.char_area) && display_.char_set != CHARSET_EBCDIC);
	pCmdUI->SetCheck(display_.char_set == CHARSET_OEM);
}

void CHexEditView::OnFontInc()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	if (pcv_ != NULL)
		pcv_->begin_change();

	// Save font for undo
	LOGFONT *plf = new LOGFONT;
	*plf = display_.FontRequired() == FONT_OEM ? oem_lf_
		   : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	for ( ; ; )
	{
		// Increase font size by one pixel
		CPoint convert(plf->lfWidth, plf->lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.LPtoDP(&convert);
		if (convert.y < max_font_size)
			++convert.y;
		else
		{
			((CMainFrame *)AfxGetMainWnd())->
				StatusBarText("Warning: Font size too big - not increased");
			aa->mac_error_ = 2;
			return;
		}
		dc.DPtoLP(&convert);
		plf->lfHeight = convert.y;
		plf->lfWidth = 0;            // Calced from height

		CFont font;
		font.CreateFontIndirect(plf);
//        if (display_.char_area && !display_.ebcdic && display_.graphic && display_.oem)
//        {
//            oem_lf_.lfHeight = convert.y;
//            oem_lf_.lfWidth = 0;                        // Calced from height
//            font.CreateFontIndirect(&oem_lf_);
//        }
//        else
//        {
//            lf_.lfHeight = convert.y;
//            lf_.lfWidth = 0;                        // Calced from height
//            font.CreateFontIndirect(&lf_);
//        }
		TEXTMETRIC tm;
		CFont *tf = dc.SelectObject(&font);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(tf);                        // Deselect font before it is destroyed
		if (tm.tmHeight + tm.tmExternalLeading > text_height_)
		{
			if (display_.FontRequired() == FONT_UCODE)
				mb_lf_.lfHeight = convert.y;
			else if (display_.FontRequired() == FONT_OEM)
				oem_lf_.lfHeight = convert.y;
			else
				lf_.lfHeight = convert.y;
			break;
		}
	}

	BOOL caret_displayed = CaretDisplayed();
//      FILE_ADDRESS address = pos2addr(GetCaret());
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	// Create and install the new font
	redo_font();

	// Calculate new position (and new total size) based on change in font size
	recalc_display();
	if (end_base)
		SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
	else
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
	if (caret_displayed)
		DisplayCaret();                 // Keep caret within display
	if (pcv_ != NULL)
		pcv_->end_change();

	DoInvalidate();
	undo_.push_back(view_undo(undo_font));      // Allow undo of font change
	undo_.back().plf = plf;
	aa->SaveToMacro(km_inc_font);
}

void CHexEditView::OnUpdateFontInc(CCmdUI* pCmdUI)
{
	// Create a large (max_font_size) font and see if the current font is
	// displayed at the same size on screen.  If so we can't increase the size
	LOGFONT logfont;			// Get font the same as current ...
	logfont = display_.FontRequired() == FONT_OEM ? oem_lf_
		   : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	{
		CPoint convert(0, max_font_size);       // ... but very big
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&convert);
		logfont.lfHeight = convert.y;
	}
	logfont.lfWidth = 0;                        // Width calced from height

	// Create font, put into DC, and see what size it would be on screen
	CFont font;
	font.CreateFontIndirect(&logfont);
	{
		TEXTMETRIC tm;
		CClientDC dc(this);
		OnPrepareDC(&dc);
		CFont *tf = dc.SelectObject(&font);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(tf);
		if (tm.tmHeight + tm.tmExternalLeading == text_height_)
			pCmdUI->Enable(FALSE);              // Already at smallest displayable font
	}
}

void CHexEditView::OnFontDec()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	if (pcv_ != NULL)
		pcv_->begin_change();

	// Save font for undo
	LOGFONT *plf = new LOGFONT;
	*plf = display_.FontRequired() == FONT_OEM ? oem_lf_
		   : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );
	for ( ; ; )
	{
		// Decrease font size by one pixel
		CPoint convert(plf->lfWidth, plf->lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.LPtoDP(&convert);
		if (convert.y > 1)
			convert.y--;
		else
		{
			((CMainFrame *)AfxGetMainWnd())->
				StatusBarText("Warning: Font size already at minimum - not decreased");
			aa->mac_error_ = 2;
			return;
		}
		dc.DPtoLP(&convert);
		plf->lfHeight = convert.y;
		plf->lfWidth = 0;            // Calced from height

		CFont font;
		font.CreateFontIndirect(plf);
//        if (display_.char_area && !display_.ebcdic && display_.graphic && display_.oem)
//        {
//            oem_lf_.lfHeight = convert.y;
//            oem_lf_.lfWidth = 0;                        // Calced from height
//            font.CreateFontIndirect(&oem_lf_);
//        }
//        else
//        {
//            lf_.lfHeight = convert.y;
//            lf_.lfWidth = 0;                        // Calced from height
//            font.CreateFontIndirect(&lf_);
//        }
		TEXTMETRIC tm;
		CFont *tf = dc.SelectObject(&font);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(tf);                        // Deselect font before it is destroyed
		if (tm.tmHeight + tm.tmExternalLeading < text_height_)
		{
			if (display_.FontRequired() == FONT_UCODE)
				mb_lf_.lfHeight = convert.y;
			else if (display_.FontRequired() == FONT_OEM)
				oem_lf_.lfHeight = convert.y;
			else
				lf_.lfHeight = convert.y;
			break;
		}
	}

	BOOL caret_displayed = CaretDisplayed();
//      FILE_ADDRESS address = pos2addr(GetCaret());
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	// Create and install the new font
	redo_font();

	// Calculate new position (and new total size) based on change in font size
	recalc_display();
	if (end_base)
		SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
	else
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
	if (caret_displayed)
		DisplayCaret();                 // Keep caret within display
	if (pcv_ != NULL)
		pcv_->end_change();

	DoInvalidate();
	undo_.push_back(view_undo(undo_font));      // Allow undo of font change
	undo_.back().plf = plf;
	aa->SaveToMacro(km_dec_font);
}

void CHexEditView::OnUpdateFontDec(CCmdUI* pCmdUI)
{
	// If we create a very small font then see what the text height is when that
	// font is used.  If this height is the same as the current text height then the
	// font size can not be decreased any more.
	LOGFONT logfont;			// Get font the same as current ...
	logfont = display_.FontRequired() == FONT_OEM ? oem_lf_
		      : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	{
		CPoint convert(0, 1);                   // ... but just one pixel high
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&convert);
		logfont.lfHeight = convert.y;
	}
	logfont.lfWidth = 0;                        // Width calced from height

	// Create font, put into DC, and see what size it would be on screen
	CFont font;
	font.CreateFontIndirect(&logfont);
	{
		TEXTMETRIC tm;
		CClientDC dc(this);
		OnPrepareDC(&dc);
		CFont *tf = dc.SelectObject(&font);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(tf);
		if (tm.tmHeight + tm.tmExternalLeading == text_height_)
			pCmdUI->Enable(FALSE);              // Already at smallest displayable font
	}
}

void CHexEditView::OnFont()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	LOGFONT logfont = display_.FontRequired() == FONT_OEM ? oem_lf_
		              : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	// Convert font size to units that user can relate to
	{
		CPoint convert(logfont.lfWidth, logfont.lfHeight);
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.LPtoDP(&convert);
		logfont.lfWidth = convert.x;
		logfont.lfHeight = convert.y;
	}

	CFontDialog dlg;
	dlg.m_cf.lpLogFont = &logfont;
	dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT | CF_SHOWHELP;
	dlg.m_cf.Flags &= ~(CF_EFFECTS);              // Disable selection of strikethrough, colours etc
	if (dlg.DoModal() == IDOK)
	{
		// Convert font size back to logical units
		dlg.GetCurrentFont(&logfont);
		if (logfont.lfHeight < 0)
			logfont.lfHeight = -logfont.lfHeight;

		{
			CPoint convert(logfont.lfWidth, logfont.lfHeight);
			CClientDC dc(this);
			OnPrepareDC(&dc);
			dc.DPtoLP(&convert);
			logfont.lfWidth = convert.x;
			logfont.lfHeight = convert.y;
		}

		do_font(&logfont);
	}
	else
	{
		((CHexEditApp *)AfxGetApp())->mac_error_ = 2;
	}
}

void CHexEditView::OnFontName()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CHexEditFontCombo* pSrcCombo = 
		(CHexEditFontCombo*)CMFCToolBarComboBoxButton::GetByCmd(IDC_FONTNAME, TRUE);
	if (pSrcCombo == NULL)
	{
		OnFont();
		return;
	}

	LOGFONT logfont = display_.FontRequired() == FONT_OEM ? oem_lf_
		              : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	CString str = pSrcCombo->GetText();
	if (pSrcCombo->SetFont(str))
	{
		const CMFCFontInfo *pDesc = pSrcCombo->GetFontDesc();
		ASSERT_VALID(pDesc);
		ASSERT(pDesc->m_strName.GetLength() < LF_FACESIZE);

		strncpy(logfont.lfFaceName, pDesc->m_strName, LF_FACESIZE);
		logfont.lfCharSet = pDesc->m_nCharSet;
		logfont.lfPitchAndFamily = pDesc->m_nPitchAndFamily;

		do_font(&logfont);
	}
}

void CHexEditView::OnUpdateFontName(CCmdUI* pCmdUI)
{
	CObList listButtons;
	static int last_font_required = -1;
	bool font_list_changed = last_font_required != display_.FontRequired();
	if (CMFCToolBar::GetCommandButtons (IDC_FONTNAME, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition (); posCombo != NULL; )
		{
			CHexEditFontCombo * pCombo = DYNAMIC_DOWNCAST(CHexEditFontCombo, listButtons.GetNext (posCombo));

			if (pCombo != NULL && !pCombo->HasFocus ())
			{
				if (display_.FontRequired() == FONT_UCODE)
				{
					if (font_list_changed)
						pCombo->FixFontList(DEFAULT_CHARSET);
					CString ss = pCombo->GetText();
					if (ss != mb_lf_.lfFaceName)
						pCombo->SetText(mb_lf_.lfFaceName);
				}
				else if (display_.FontRequired() == FONT_OEM)
				{
					if (font_list_changed)
						pCombo->FixFontList(OEM_CHARSET);
					CString ss = pCombo->GetText();
					if (ss != oem_lf_.lfFaceName)
						pCombo->SetText(oem_lf_.lfFaceName);
				}
				else
				{
					if (font_list_changed)
						pCombo->FixFontList(ANSI_CHARSET);
					CString ss = pCombo->GetText();
					if (ss != lf_.lfFaceName)
						pCombo->SetText(lf_.lfFaceName);
				}
			}
		}
	}
	last_font_required = display_.FontRequired();  // remeber the font set we are now using
}

void CHexEditView::OnFontSize()
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CHexEditFontSizeCombo* pSrcCombo = 
		(CHexEditFontSizeCombo*)CMFCToolBarComboBoxButton::GetByCmd(IDC_FONTSIZE, TRUE);
	if (pSrcCombo == NULL)
	{
		OnFont();
		return;
	}

	LOGFONT logfont = display_.FontRequired() == FONT_OEM ? oem_lf_
		              : ( display_.FontRequired() == FONT_UCODE ? mb_lf_ : lf_ );

	int nSize = pSrcCombo->GetTwipSize();
	if (nSize == -2 || (nSize >= 0 && nSize < 20) || nSize > 32760)
	{
		AfxMessageBox("Invalid font size.");
	}
	else if (nSize > 0)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		logfont.lfHeight = int((nSize * dc.GetDeviceCaps(LOGPIXELSX)) / 1440.0 + 0.5);

		do_font(&logfont);

		// Store the exact font size (due to rounding probs do_font (SetFont) calc may be slightly off)
		fontsize_.Format("%d", nSize/20);
	}
}

void CHexEditView::OnUpdateFontSize(CCmdUI* pCmdUI)
{
	CObList listButtons;
	if (CMFCToolBar::GetCommandButtons (IDC_FONTSIZE, listButtons) > 0)
	{
		for (POSITION posCombo = listButtons.GetHeadPosition (); posCombo != NULL;)
		{
			CMFCToolBarFontSizeComboBox * pCombo = DYNAMIC_DOWNCAST(CMFCToolBarFontSizeComboBox, listButtons.GetNext (posCombo));

			if (pCombo != NULL && !pCombo->HasFocus ())
			{
				static CString savedFontName;
				CString fontName;
				if (display_.FontRequired() == FONT_UCODE)
					fontName = mb_lf_.lfFaceName;
				else if (display_.FontRequired() == FONT_OEM)
					fontName = oem_lf_.lfFaceName;
				else
					fontName = lf_.lfFaceName;

				if (!fontName.IsEmpty() &&
					(pCombo->GetCount() == 0 || fontName != savedFontName))
				{
					savedFontName = fontName;
					pCombo->RebuildFontSizes(fontName);
				}

				int nSize = atoi(fontsize_)*20;
				if (nSize == -2 || (nSize >= 0 && nSize < 20) || nSize > 32760)
					nSize = 20*12;
				pCombo->SetTwipSize(nSize);

				// Store the exact font size (due to rounding probs do_font (SetFont) calc may be slightly off)
				fontsize_.Format("%d", nSize/20);
				CString ss = pCombo->GetText();
				if (ss != fontsize_)
					pCombo->SetText(fontsize_);
			}
		}
	}
}

void CHexEditView::do_font(LOGFONT *plf)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	if (pcv_ != NULL)
		pcv_->begin_change();

	// Save current LOGFONT for undo
	LOGFONT *prev_lf = new LOGFONT;
	if (display_.FontRequired() == FONT_UCODE)
	{
		*prev_lf = mb_lf_;
		mb_lf_ = *plf;
	}
	else if (display_.FontRequired() == FONT_OEM)
	{
		// We can't switch to an ANSI char set because we are displaying OEM graphics
		if (plf->lfCharSet != OEM_CHARSET)
		{
			mm->StatusBarText("Can't switch to this font when displaying IBM/OEM graphics chars");
			aa->mac_error_ = 2;
			return;
		}
		*prev_lf = oem_lf_;
		oem_lf_ = *plf;
	}
	else
	{
		// We can't switch to an OEM char set
		if (plf->lfCharSet != ANSI_CHARSET)
		{
			mm->StatusBarText("Can't switch to this font when displaying normal characters");
			aa->mac_error_ = 2;
			return;
		}
		*prev_lf = lf_;
		lf_ = *plf;
	}

	// Set new LOGFONT

	BOOL caret_displayed = CaretDisplayed();
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	CHECK_SECURITY(9);

	// Create and install the new font
	redo_font();

	// Calculate new position (and new total size) based on change in font size
	recalc_display();
	if (end_base)
		SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
	else
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
	if (caret_displayed)
		DisplayCaret();                 // Keep caret within display
	if (pcv_ != NULL)
		pcv_->end_change();

	DoInvalidate();
	undo_.push_back(view_undo(undo_font));      // Allow undo of font change
	undo_.back().plf = prev_lf;

	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_font, 
		              display_.FontRequired() == FONT_OEM ? &oem_lf_
		              : ( display_.FontRequired() == FONT_UCODE ? &mb_lf_ : &lf_ ) );
}

void CHexEditView::change_rowsize(int rowsize)
{
	if (rowsize != rowsize_)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		FILE_ADDRESS scroll_addr = pos2addr(GetScroll());

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		undo_.push_back(view_undo(undo_rowsize));
		undo_.back().rowsize = rowsize_;            // Save previous rowsize for undo
		rowsize_ = rowsize;
		recalc_display();

		// Fix scroll place so it's about the same even though the row length has changed
		CPointAp pt = addr2pos(scroll_addr);
		pt.x = 0;
		SetScroll(pt);

		// Fix selection
		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_rowsize, rowsize);
}

void CHexEditView::change_group_by(int group_by)
{
	if (group_by != group_by_)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		undo_.push_back(view_undo(undo_group_by));
		undo_.back().rowsize = group_by_;           // Save previous group_by for undo
		group_by_ = group_by;
		recalc_display();

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_group_by, group_by);
}

void CHexEditView::change_offset(int new_offset)
{
	if (new_offset != real_offset_)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		undo_.push_back(view_undo(undo_offset));
		undo_.back().rowsize = real_offset_;        // Save previous offset for undo
		real_offset_ = new_offset;
		recalc_display();

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_offset, new_offset);
}

void CHexEditView::OnAutoFit()
{
	do_autofit();
}

// Change autofit mode to state (0 or 1).  If state is -1 then toggle autofit.
void CHexEditView::do_autofit(int state /*=-1*/)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	if (pcv_ != NULL)
		pcv_->begin_change();

//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	BOOL caret_displayed = CaretDisplayed();
	FILE_ADDRESS scroll_addr = pos2addr(GetScroll());

//    FILE_ADDRESS address = pos2addr(GetCaret());
	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	if (state == -1)
		display_.autofit = !display_.autofit;
	else if ((BOOL)state == display_.autofit)
		return;                                 // No change - do nothing
	else
		display_.autofit = (BOOL)state;
	undo_.push_back(view_undo(undo_autofit));
	if (display_.autofit)
		undo_.back().rowsize = rowsize_;
	else
		undo_.back().rowsize = 0;
	recalc_display();
	// Fix scroll place so it's about the same even though the row length has changed
	CPointAp pt = addr2pos(scroll_addr);
	pt.x = 0;
	SetScroll(pt);
	if (end_base)
		SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
	else
		SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
	if (caret_displayed)
		DisplayCaret(); // Keep caret within display
	if (pcv_ != NULL)
		pcv_->end_change();

	DoInvalidate();

	aa->SaveToMacro(km_autofit, state);

	CHECK_SECURITY(19);
}

void CHexEditView::OnUpdateAutofit(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(display_.autofit);
}

void CHexEditView::OnColumnDec()
{
	if (rowsize_ > 4)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		// Try to stay at the same place when multiple column adjustments are made
		if (resize_start_addr_ == -1 || resize_curr_scroll_ != GetScroll().y)
			resize_start_addr_ = pos2addr(GetScroll());

		undo_.push_back(view_undo(undo_rowsize));
		undo_.back().rowsize = rowsize_;            // Save previous rowsize for undo
		rowsize_ = rowsize_ - 1;
		recalc_display();

		// Adjust scroll so that about the same row is visible
		CPointAp pt = addr2pos(resize_start_addr_);
		pt.x = 0;
		SetScroll(pt);

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();

		resize_curr_scroll_ = GetScroll().y;   // Save current pos so we can check if we are at the same place later
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_rowsize, -1);
}

void CHexEditView::OnUpdateColumnDec(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!display_.autofit && rowsize_ > 4);
}

void CHexEditView::OnColumnInc()
{
	if (rowsize_ < max_buf)
	{
		if (pcv_ != NULL)
			pcv_->begin_change();

		FILE_ADDRESS start_addr, end_addr;
		BOOL end_base = GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		// Try to stay at the same place when multiple column adjustments are made
		if (resize_start_addr_ == -1 || resize_curr_scroll_ != GetScroll().y)
			resize_start_addr_ = pos2addr(GetScroll());

		undo_.push_back(view_undo(undo_rowsize));
		undo_.back().rowsize = rowsize_;            // Save previous rowsize for undo
		rowsize_ = rowsize_ + 1;
		recalc_display();

		// Adjust scroll so that about the same row is visible
		CPointAp pt = addr2pos(resize_start_addr_);
		pt.x = 0;
		SetScroll(pt);

		if (end_base)
			SetSel(addr2pos(end_addr, row), addr2pos(start_addr, row), true);
		else
			SetSel(addr2pos(start_addr, row), addr2pos(end_addr, row));
		if (pcv_ != NULL)
			pcv_->end_change();

		DoInvalidate();

		resize_curr_scroll_ = GetScroll().y;   // Save current pos so we can check if we are at the same place later
	}
	((CHexEditApp *)AfxGetApp())->SaveToMacro(km_rowsize, -2);
}

void CHexEditView::OnUpdateColumnInc(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!display_.autofit && rowsize_ < max_buf);
}

void CHexEditView::redo_font()
{
	CFont *tf = pfont_;
	pfont_ = new CFont;
	pfont_->CreateFontIndirect(display_.FontRequired() == FONT_OEM ? &oem_lf_
		                       : ( display_.FontRequired() == FONT_UCODE ? &mb_lf_ : &lf_ ));
	SetFont(pfont_);
	if (pcv_ != NULL)
		pcv_->SetFont(pfont_);
	if (tf != NULL)
		delete tf;                      // Delete old font after it's deselected
}

void CHexEditView::begin_change()
{
	ASSERT(previous_state_ == 0);
	previous_caret_displayed_ = CaretDisplayed();
	previous_end_base_ = GetSelAddr(previous_start_addr_, previous_end_addr_);
	previous_row_ = 0;
	if (previous_start_addr_ == previous_end_addr_ && display_.vert_display)
		previous_row_ = pos2row(GetCaret());
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	CHECK_SECURITY(21);

	previous_state_ = disp_state_;

	if (pcv_ != NULL)
		pcv_->begin_change();
}

// Return ptoo or TRUE if it added to the undo stack
BOOL CHexEditView::make_change(BOOL ptoo /*=FALSE*/)
{
	if (previous_state_ != disp_state_)
	{
		if (theApp.intelligent_undo_ &&         // intelligent undo is turned on
			undo_.size() > 0 &&                 // there is an undo op on the stack
			undo_.back().utype == undo_state && // previous op was a state change
			!undo_.back().previous_too &&       // and not part of another operation
			undo_.back().disp_state == disp_state_) // and same state
		{
			ASSERT(!ptoo);     // if part of larger op then previous utype should not have been undo_state
			undo_.pop_back();
		}
		else
		{
			undo_.push_back(view_undo(undo_state, ptoo));
			undo_.back().disp_state = previous_state_;
			ptoo = TRUE;
		}
	}

	return ptoo;
}

void CHexEditView::end_change()
{
	redo_font();
	recalc_display();
	if (!display_.vert_display) previous_row_ = 0;  // If vert mode turned off make sure row is zero
	if (previous_end_base_)
		SetSel(addr2pos(previous_end_addr_, previous_row_), addr2pos(previous_start_addr_, previous_row_), true);
	else
		SetSel(addr2pos(previous_start_addr_, previous_row_), addr2pos(previous_end_addr_, previous_row_));
	if (previous_caret_displayed_)
		DisplayCaret();                 // Keep caret within display
	DoInvalidate();

	if (pcv_ != NULL)
		pcv_->end_change();

	previous_state_ = 0;  // Used to make sure begin_change/end_change are in pairs
}

void CHexEditView::OnDisplayHex()
{
	// Change the current state (storing previous state in undo vector if changed)
	begin_change();
	display_.hex_area = TRUE;
	display_.char_area = FALSE;
	display_.edit_char = FALSE;
	display_.vert_display = FALSE;
	SetHorzBufferZone(2);
	make_change();
	end_change();

	theApp.SaveToMacro(km_area, 1);
}

void CHexEditView::OnUpdateDisplayHex(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!display_.vert_display && display_.hex_area && !display_.char_area);
}

void CHexEditView::OnDisplayChar()
{
	// Change the current state (storing previous state in undo vector if changed)
	begin_change();
	display_.hex_area = FALSE;
	display_.char_area = TRUE;
	display_.edit_char = TRUE;
	display_.vert_display = FALSE;
	SetHorzBufferZone(1);
	make_change();
	end_change();

	theApp.SaveToMacro(km_area, 2);
}

void CHexEditView::OnUpdateDisplayChar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!display_.vert_display && !display_.hex_area && display_.char_area);
}

void CHexEditView::OnDisplayBoth()
{
	// Change the current state (storing previous state in undo vector if changed)
	begin_change();
	CHECK_SECURITY(50);
	display_.hex_area = TRUE;
	display_.char_area = TRUE;
	display_.vert_display = FALSE;
	if (display_.edit_char)
		SetHorzBufferZone(1);
	else
		SetHorzBufferZone(2);
	make_change();
	end_change();

	theApp.SaveToMacro(km_area, 3);
}

void CHexEditView::OnUpdateDisplayBoth(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!display_.vert_display && display_.hex_area && display_.char_area);
}

void CHexEditView::OnDisplayStacked()
{
	// Change the current state (storing previous state in undo vector if changed)
	begin_change();
	display_.vert_display = TRUE;
	if (GetVertBufferZone() < 2)
		SetVertBufferZone(2);   // Make sure we can always see the other 2 rows at the same address
	SetHorzBufferZone(1);
	make_change();
	end_change();
	CHECK_SECURITY(30);

	theApp.SaveToMacro(km_area, 4);
}

void CHexEditView::OnUpdateDisplayStacked(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(display_.vert_display);
}

void CHexEditView::OnCharsetAscii()
{
	bool std_scheme = false;

	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change character sets without the char area");
		theApp.mac_error_ = 2;
		return;
	}

	// Change search type in find modeless dlg
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (display_.char_set == CHARSET_EBCDIC)
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);

	if (scheme_name_ == ANSI_NAME     && display_.char_set == CHARSET_ANSI   ||
		scheme_name_ == OEM_NAME      && display_.char_set == CHARSET_OEM    ||
		scheme_name_ == EBCDIC_NAME   && display_.char_set == CHARSET_EBCDIC ||
		scheme_name_ == UNICODE_NAME  && (display_.char_set == CHARSET_UCODE_EVEN || display_.char_set == CHARSET_UCODE_ODD) ||
		scheme_name_ == CODEPAGE_NAME && display_.char_set == CHARSET_CODEPAGE )
	{
		std_scheme = true;
	}

	begin_change();
	display_.char_set = CHARSET_ASCII;
	BOOL ptoo = make_change();
	if (std_scheme)
	{
		undo_.push_back(view_undo(undo_scheme, ptoo));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = scheme_name_;
		scheme_name_ = ASCII_NAME;
		set_colours();
	}
	end_change();

	CHECK_SECURITY(51);
	theApp.SaveToMacro(km_charset, unsigned __int64(0));
}

void CHexEditView::OnUpdateCharsetAscii(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_ASCII);
	}
}

void CHexEditView::OnCharsetAnsi()
{
	bool std_scheme = false;

	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change character sets without the char area.");
		theApp.mac_error_ = 2;
		return;
	}

	// Change search type in find modeless dlg
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	ASSERT(mm != NULL);
	if (display_.char_set == CHARSET_EBCDIC)
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);

	if (scheme_name_ == ASCII_NAME    && display_.char_set == CHARSET_ASCII ||
		scheme_name_ == OEM_NAME      && display_.char_set == CHARSET_OEM   ||
		scheme_name_ == EBCDIC_NAME   && display_.char_set == CHARSET_EBCDIC ||
		scheme_name_ == UNICODE_NAME  && (display_.char_set == CHARSET_UCODE_EVEN || display_.char_set == CHARSET_UCODE_ODD) ||
		scheme_name_ == CODEPAGE_NAME && display_.char_set == CHARSET_CODEPAGE )
	{
		std_scheme = true;
	}

	begin_change();
	display_.char_set = CHARSET_ANSI;
	BOOL ptoo = make_change();
	if (std_scheme)
	{
		undo_.push_back(view_undo(undo_scheme, ptoo));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = scheme_name_;
		scheme_name_ = ANSI_NAME;
		set_colours();
	}
	end_change();

	theApp.SaveToMacro(km_charset, 1);
}

void CHexEditView::OnUpdateCharsetAnsi(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_ANSI);
	}
}

void CHexEditView::OnCharsetOem()
{
	bool std_scheme = false;

	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change character sets without the char area.");
		theApp.mac_error_ = 2;
		return;
	}

	// Change search type in find modeless dlg
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	ASSERT(mm != NULL);
	if (display_.char_set == CHARSET_EBCDIC)
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);

	if (scheme_name_ == ASCII_NAME    && display_.char_set == CHARSET_ASCII  ||
		scheme_name_ == ANSI_NAME     && display_.char_set == CHARSET_ANSI   ||
		scheme_name_ == EBCDIC_NAME   && display_.char_set == CHARSET_EBCDIC ||
		scheme_name_ == UNICODE_NAME  && (display_.char_set == CHARSET_UCODE_EVEN || display_.char_set == CHARSET_UCODE_ODD) ||
		scheme_name_ == CODEPAGE_NAME && display_.char_set == CHARSET_CODEPAGE )
	{
		std_scheme = true;
	}

	begin_change();
	display_.char_set = CHARSET_OEM;
	BOOL ptoo = make_change();
	if (std_scheme)
	{
		undo_.push_back(view_undo(undo_scheme, ptoo));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = scheme_name_;
		scheme_name_ = OEM_NAME;
		set_colours();
	}
	end_change();

	theApp.SaveToMacro(km_charset, 2);
}

void CHexEditView::OnUpdateCharsetOem(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_OEM);
	}
}

void CHexEditView::OnCharsetEbcdic()
{
	bool std_scheme = false;

	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change character sets without the char area.");
		theApp.mac_error_ = 2;
		return;
	}

	// Change search type in find modeless dlg
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	ASSERT(mm != NULL);
	if (display_.char_set != CHARSET_EBCDIC)
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_EBCDIC);

	if (scheme_name_ == ASCII_NAME    && display_.char_set == CHARSET_ASCII ||
		scheme_name_ == ANSI_NAME     && display_.char_set == CHARSET_ANSI  ||
		scheme_name_ == OEM_NAME      && display_.char_set == CHARSET_OEM   ||
		scheme_name_ == UNICODE_NAME  && (display_.char_set == CHARSET_UCODE_EVEN || display_.char_set == CHARSET_UCODE_ODD) ||
		scheme_name_ == CODEPAGE_NAME && display_.char_set == CHARSET_CODEPAGE )
	{
		std_scheme = true;
	}

	begin_change();
	display_.char_set = CHARSET_EBCDIC;
	BOOL ptoo = make_change();
	if (std_scheme)
	{
		undo_.push_back(view_undo(undo_scheme, ptoo));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = scheme_name_;
		scheme_name_ = EBCDIC_NAME;
		set_colours();
	}
	end_change();

	theApp.SaveToMacro(km_charset, 3);
}

void CHexEditView::OnUpdateCharsetEbcdic(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_EBCDIC);
	}
}

void CHexEditView::OnCharsetCodepage()
{
	bool std_scheme = false;

	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change characters set without the char area.");
		theApp.mac_error_ = 2;
		return;
	}

	if (scheme_name_ == ASCII_NAME    && display_.char_set == CHARSET_ASCII ||
		scheme_name_ == ANSI_NAME     && display_.char_set == CHARSET_ANSI  ||
		scheme_name_ == OEM_NAME      && display_.char_set == CHARSET_OEM   ||
		scheme_name_ == UNICODE_NAME  && (display_.char_set == CHARSET_UCODE_EVEN || display_.char_set == CHARSET_UCODE_ODD) ||
		scheme_name_ == EBCDIC_NAME   && display_.char_set == CHARSET_EBCDIC )
	{
		std_scheme = true;
	}

	begin_change();
	display_.char_set = CHARSET_CODEPAGE;
	BOOL ptoo = make_change();
	if (std_scheme)
	{
		undo_.push_back(view_undo(undo_scheme, ptoo));      // Allow undo of scheme change
		undo_.back().pscheme_name = new CString;
		*undo_.back().pscheme_name = scheme_name_;
		scheme_name_ = CODEPAGE_NAME;
		set_colours();
	}
	end_change();

	theApp.SaveToMacro(km_charset, 4);
}

void CHexEditView::OnUpdateCharsetCodepage(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_CODEPAGE);
	}
}

void CHexEditView::OnCodepage(int cp)
{
	if (!(display_.vert_display || display_.char_area))
	{
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Set Error", "You can't change character set code page without the char area.");
		theApp.mac_error_ = 2;
		return;
	}

	begin_change();
	display_.char_set = CHARSET_CODEPAGE;
	BOOL ptoo = make_change();
	undo_.push_back(view_undo(undo_codepage, ptoo));
	undo_.back().codepage = code_page_;
	SetCodePage(cp);
	end_change();

	theApp.SaveToMacro(km_charset, cp);
}

void CHexEditView::OnUpdateCodepage(CCmdUI *pCmdUI, int cp)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | ((display_.vert_display || display_.char_area) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area));
		pCmdUI->SetCheck(display_.char_set == CHARSET_CODEPAGE && code_page_ == cp);
	}
}

void CHexEditView::OnCodepage65001()
{
	OnCodepage(65001);
}

void CHexEditView::OnUpdateCodepage65001(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 65001);
}

void CHexEditView::OnCodepage437()
{
	OnCodepage(437);
}

void CHexEditView::OnUpdateCodepage437(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 437);
}

void CHexEditView::OnCodepage500()
{
	OnCodepage(500);
}

void CHexEditView::OnUpdateCodepage500(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 500);
}

void CHexEditView::OnCodepage10000()
{
	OnCodepage(10000);
}

void CHexEditView::OnUpdateCodepage10000(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 10000);
}

void CHexEditView::OnCodepage1250()
{
	OnCodepage(1250);
}

void CHexEditView::OnUpdateCodepage1250(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1250);
}

void CHexEditView::OnCodepage1251()
{
	OnCodepage(1251);
}

void CHexEditView::OnUpdateCodepage1251(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1251);
}

void CHexEditView::OnCodepage1252()
{
	OnCodepage(1252);
}

void CHexEditView::OnUpdateCodepage1252(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1252);
}

void CHexEditView::OnCodepage1253()
{
	OnCodepage(1253);
}

void CHexEditView::OnUpdateCodepage1253(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1253);
}

void CHexEditView::OnCodepage1254()
{
	OnCodepage(1254);
}

void CHexEditView::OnUpdateCodepage1254(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1254);
}

void CHexEditView::OnCodepage1255()
{
	OnCodepage(1255);
}

void CHexEditView::OnUpdateCodepage1255(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1255);
}

void CHexEditView::OnCodepage1256()
{
	OnCodepage(1256);
}

void CHexEditView::OnUpdateCodepage1256(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1256);
}

void CHexEditView::OnCodepage1257()
{
	OnCodepage(1257);
}

void CHexEditView::OnUpdateCodepage1257(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1257);
}

void CHexEditView::OnCodepage1258()
{
	OnCodepage(1258);
}

void CHexEditView::OnUpdateCodepage1258(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 1258);
}

void CHexEditView::OnCodepage874()
{
	OnCodepage(874);
}

void CHexEditView::OnUpdateCodepage874(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 874);
}

void CHexEditView::OnCodepage932()
{
	OnCodepage(932);
}

void CHexEditView::OnUpdateCodepage932(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 932);
}

void CHexEditView::OnCodepage936()
{
	OnCodepage(936);
}

void CHexEditView::OnUpdateCodepage936(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 936);
}

void CHexEditView::OnCodepage949()
{
	OnCodepage(949);
}

void CHexEditView::OnUpdateCodepage949(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 949);
}

void CHexEditView::OnCodepage950()
{
	OnCodepage(950);
}

void CHexEditView::OnUpdateCodepage950(CCmdUI *pCmdUI)
{
	OnUpdateCodepage(pCmdUI, 950);
}

void CHexEditView::OnControlNone()
{
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		ASSERT(theApp.playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Area", "You can't display control characters without the char area");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Set Error", "You can't display control characters in EBCDIC");
		theApp.mac_error_ = 2;
		return;
	}

	// Change the current state (storing previous state in undo vector if changed)
	begin_change();
	display_.control = 0;
	make_change();
	end_change();

	theApp.SaveToMacro(km_control, 1);
}

void CHexEditView::OnUpdateControlNone(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | 
			((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI) ?
					MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI));
		pCmdUI->SetCheck(display_.control == 0);
	}
}

void CHexEditView::OnControlAlpha()
{
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		ASSERT(theApp.playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Area", "You can't display control characters without the char area");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Set Error", "You can't display control characters in EBCDIC");
		theApp.mac_error_ = 2;
		return;
	}

	begin_change();
	display_.control = 1;
	make_change();
	end_change();

	theApp.SaveToMacro(km_control, 2);
}

void CHexEditView::OnUpdateControlAlpha(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | 
			((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI));
		pCmdUI->SetCheck(display_.control == 1);
	}
}

void CHexEditView::OnControlC()
{
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		ASSERT(theApp.playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Area", "You can't display control characters without the char area");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Set Error", "You can't display control characters in EBCDIC");
		theApp.mac_error_ = 2;
		return;
	}

	begin_change();
	display_.control = 2;
	make_change();
	end_change();

	theApp.SaveToMacro(km_control, 3);
}

void CHexEditView::OnUpdateControlC(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | 
			((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI) ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
	{
		pCmdUI->Enable((display_.vert_display || display_.char_area) && (display_.char_set == CHARSET_ASCII || display_.char_set == CHARSET_ANSI));
		pCmdUI->SetCheck(display_.control == 2);
	}
}

void CHexEditView::OnAscEbc()
{
	if (!(display_.vert_display || display_.char_area))
	{
		// Can't display EBCDIC, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("Char Area", "You can't display EBCDIC without the char area");
		theApp.mac_error_ = 2;
		return;
	}

	// Change search type in find modeless dlg
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	ASSERT(mm != NULL);
	if (display_.char_set == CHARSET_EBCDIC)
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_ASCII);
	else
		mm->m_wndFind.SetCharSet(CFindSheet::RB_CHARSET_EBCDIC);

	begin_change();
	if (display_.char_set == CHARSET_EBCDIC)
		display_.char_set = CHARSET_ANSI;
	else
		display_.char_set = CHARSET_EBCDIC;
	make_change();
	end_change();

	theApp.SaveToMacro(km_ebcdic);
}

void CHexEditView::OnUpdateAscEbc(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((display_.vert_display || display_.char_area));
	pCmdUI->SetCheck(display_.char_set == CHARSET_EBCDIC);
}

void CHexEditView::OnControl()
{
	CHECK_SECURITY(18);

	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		// Can't toggle control chars, presumably in macro playback
		ASSERT(aa->playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Area", "You can't display control characters without the char area");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Set Error", "You can't display control characters in EBCDIC");
		aa->mac_error_ = 2;
		return;
	}

	begin_change();
	display_.control = (display_.control + 1)%3;
	make_change();
	end_change();

	aa->SaveToMacro(km_control, unsigned __int64(0));
}

void CHexEditView::OnControlToggle()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	if (!(display_.vert_display || display_.char_area) || display_.char_set == CHARSET_EBCDIC)
	{
		// Can't toggle control chars, presumably in macro playback
		ASSERT(aa->playing_);
		if (!(display_.vert_display || display_.char_area))
			TaskMessageBox("Char Area", "You can't display control characters without the char area");
		else if (display_.char_set == CHARSET_EBCDIC)
			TaskMessageBox("Char Set Error", "You can't display control characters in EBCDIC");
		aa->mac_error_ = 2;
		return;
	}

	begin_change();
	// This is called as a result of menu item which has only 2 states (unlike
	// dialog bar button handled by OnControl() above which has 3)
	if (display_.control > 0)
		display_.control = 0;
	else
		display_.control = 1;
	make_change();
	end_change();

	aa->SaveToMacro(km_control, 99);
}

void CHexEditView::OnUpdateControl(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((display_.vert_display || display_.char_area) && display_.char_set != CHARSET_EBCDIC);
	pCmdUI->SetCheck(display_.control != 0);
}

void CHexEditView::OnMark()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar

	SetMark(GetPos());

	// Keep track whether mark is in hex or char area (if char area on)
	if (display_.vert_display)
	{
		// If cursor is in char row set mark to be in char area
		if (pos2row(GetCaret()) == 0)
			display_.mark_char = TRUE;
		else
			display_.mark_char = FALSE;
	}
	else if (display_.char_area && display_.hex_area)
		display_.mark_char = display_.edit_char;
	else if (display_.char_area)
		display_.mark_char = TRUE;
	else if (display_.hex_area)
		display_.mark_char = FALSE;

	aa->SaveToMacro(km_mark_pos);
}

void CHexEditView::SetMark(FILE_ADDRESS new_mark)
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	// Invalidate where mark was to change colours
	invalidate_addr_range(mark_, mark_ + 1);

	// Save current mark and move mark_
	undo_.push_back(view_undo(undo_setmark));
	undo_.back().address = mark_;
	undo_.push_back(view_undo(undo_state, TRUE));
	undo_.back().disp_state = disp_state_;
	mark_ = new_mark;
	if (theApp.align_rel_ && mark_ != GetDocument()->base_addr_)
	{
		GetDocument()->StopSearch();
		GetDocument()->base_addr_ = GetSearchBase();
		GetDocument()->StartSearch();
	}

	// Invalidate where mark now is to change background colour
	invalidate_addr_range(mark_, mark_ + 1);

	CHECK_SECURITY(41);

	show_calc();                        // Some button enablement depends on mark_ position (eg. @ Mark)
}

void CHexEditView::OnGotoMark()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	if (!display_.vert_display && 
		display_.char_area && display_.hex_area && 
		display_.edit_char != display_.mark_char)
	{
		// Going to mark also entails swapping between hex and char areas
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		int row = 0;
		if (start_addr == end_addr && display_.vert_display)
			row = pos2row(GetCaret());

		undo_.push_back(view_undo(undo_state));
		undo_.back().disp_state = disp_state_;

		display_.edit_char = display_.mark_char;
		if (display_.edit_char)
			SetHorzBufferZone(1);
		else
			SetHorzBufferZone(2);
		// We need to specify the previous address since otherwise MoveToAddress
		// will use the current selection which is now in the wrong area.
		MoveWithDesc("Jump to Mark ", mark_, mark_, start_addr, end_addr, TRUE);  // space at end means significant nav pt

		// Need to call SetSel in case MoveToAddress did not, due to no move (only area swap)
		SetSel(addr2pos(mark_, row), addr2pos(mark_, row), true);
	}
	else
		MoveWithDesc("Jump to Mark ", mark_);  // space at end means significant nav pt

	aa->SaveToMacro(km_goto_mark);
}

void CHexEditView::OnExtendToMark()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	if ((end_base && mark_ == end_addr) ||
		(!end_base && mark_ == start_addr))
	{
		// There is nothing to do
		aa->mac_error_ = 1;
		return;
	}

	// Move the non-base end of the selection to the mark (MoveToAddress saves undo info)
	if (end_base)
		MoveToAddress(mark_, end_addr);
	else
		MoveWithDesc("Extend to Mark ", start_addr, mark_);  // space at end means significant nav pt

	aa->SaveToMacro(km_extendto_mark);
}

// Swap the current caret position with the mark
void CHexEditView::OnSwapMark()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing

	FILE_ADDRESS start_addr, end_addr;
	BOOL end_base = GetSelAddr(start_addr, end_addr);
	int row = 0;
	if (start_addr == end_addr && display_.vert_display)
		row = pos2row(GetCaret());

	// If same address and mark/caret are in same areas (both in hex or both in char area) return
	if (mark_ == start_addr && start_addr == end_addr && 
		(display_.vert_display || !display_.char_area || !display_.hex_area || display_.edit_char == display_.mark_char))
	{
		// There is nothing to do
		aa->mac_error_ = 1;
		return;
	}

	// If the caret and the mark are in different areas we have to swap them
	if (!display_.vert_display && 
		display_.char_area && display_.hex_area && 
		display_.edit_char != display_.mark_char)
	{
		undo_.push_back(view_undo(undo_state));        // save undo for edit_char_ and mark_char_
		undo_.back().disp_state = disp_state_;
		display_.edit_char = !display_.edit_char;
		display_.mark_char = !display_.mark_char;
		if (display_.edit_char)
			SetHorzBufferZone(1);
		else
			SetHorzBufferZone(2);

		// We need to specify the previous address since otherwise MoveToAddress
		// will use the current selection which is now in the wrong area since
		// display_.edit_char has changed.
		MoveWithDesc("Swap Cursor with Mark ", mark_, mark_, start_addr, end_addr, TRUE);

		// Need to call SetSel in case MoveToAddress did not due to no move (only area swap)
		SetSel(addr2pos(mark_), addr2pos(mark_), true);
	}
	else
		MoveWithDesc("Swap Cursor with Mark ", mark_, -1, -1, -1, FALSE, FALSE, row);

	// Move the mark
	undo_.push_back(view_undo(undo_setmark, TRUE));     // save undo for move mark
	undo_.back().address = mark_;
	invalidate_addr_range(mark_, mark_ + 1);            // force undraw of mark
	mark_ = start_addr;
	invalidate_addr_range(mark_, mark_ + 1);            // force draw of new mark
	if (theApp.align_rel_ && mark_ != GetDocument()->base_addr_)
	{
		GetDocument()->StopSearch();
		GetDocument()->base_addr_ = GetSearchBase();
		GetDocument()->StartSearch();
	}
	show_calc();

	aa->SaveToMacro(km_swap_mark);
}

void CHexEditView::OnJumpDec()           // message from BCG edit bar combo
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	CString addr_str, err_str;
	FILE_ADDRESS address = mm->GetDecAddress(addr_str, err_str);

	if (address == -1)
	{
		CString ss;
		ss.Format("The value below is an invalid integer expression"
			      "\n\n%s\n\n%s", addr_str, err_str);
		TaskMessageBox("Invalid address", ss);
		return;
	}

	// If recording macro indicate jump & store address jumped to
	aa->SaveToMacro(km_address_dec, addr_str);

#if 0
	// Try to go to the address requested
	FILE_ADDRESS actual;
	if ((actual = GoAddress(address)) != address)
	{
		// Could not go to the requested address - tell user
		AfxMessageBox("Invalid address entered. Address set to EOF");
	}
	SaveMove();          // Save prev pos in undo array
	DisplayCaret();
#else
	MoveWithDesc("Jump (Decimal Jump Tool) ", address);  // space at end means significant nav pt
#endif
}

void CHexEditView::OnJumpHex()           // message from BCG edit bar combo
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();

	CString addr_str, err_str;
	FILE_ADDRESS address = mm->GetHexAddress(addr_str, err_str);

	if (address == -1)
	{
		CString ss;
		ss.Format("The value is an invalid hex address or expression"
			"\n\n%s\n\n%s", addr_str, err_str);
		TaskMessageBox("Invalid address", ss);
		return;
	}

	// If recording macro indicate jump & store address jumped to
	theApp.SaveToMacro(km_address_hex, addr_str);
	MoveWithDesc("Jump (Hex Jump Tool) ", address);  // space at end means significant nav pt
}

void CHexEditView::OnJumpHexAddr()      // Alt-J
{
	num_entered_ = num_del_ = num_bs_ = 0;      // Stop any editing
	CHexEditControl::BeginJump();
}

void CHexEditView::OnInsert()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;

	if (!do_insert())
		return;

	aa->SaveToMacro(km_ovr_ins);
}

bool CHexEditView::do_insert()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (display_.readonly)
	{
		TaskMessageBox("INS not allowed for Read Only Files",
			"You can't turn on insert mode when the file is read only.");
		aa->mac_error_ = 10;
		return false;
	}
	if (GetDocument()->IsDevice())
	{
		TaskMessageBox("INS Illegal for Devices",
			"You cannot use insert mode for devices (logical volumes and physical disks)");
		aa->mac_error_ = 10;
		return false;
	}
	display_.overtype = !display_.overtype;
	undo_.push_back(view_undo(undo_overtype));
	undo_.back().flag = !display_.overtype;
	if (GetDocument()->length() == 0)
	{
		DoInvalidate();
		if (display_.overtype)
			ScrollMode();
		else
		{
			CaretMode();
			SetCaret(addr2pos(0));  // very start of file
		}
	}
	return true;
}

void CHexEditView::OnUpdateInsert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!display_.readonly && !GetDocument()->IsDevice());
	pCmdUI->SetCheck(!display_.overtype);
}

void CHexEditView::OnAllowMods()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
//    if (!aa->playing_ && GetFocus() != this) SetFocus(); // Ensure focus does not stay in DlgBar
	num_entered_ = num_del_ = num_bs_ = 0;

	allow_mods();

	aa->SaveToMacro(km_ro_rw);
}

void CHexEditView::allow_mods()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

	if (display_.readonly && GetDocument()->read_only())
	{
		TaskMessageBox("Read Only", "This file cannot be modified as "
			           "it was opened in read only mode.");
		aa->mac_error_ = 10;
		return;
	}
	display_.readonly = !display_.readonly;
	undo_.push_back(view_undo(undo_readonly));
	undo_.back().flag = !display_.readonly;
	show_prop();  // things may be changeable now
}

void CHexEditView::OnUpdateAllowMods(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!GetDocument()->read_only());
	pCmdUI->SetCheck(!display_.readonly);
}

void CHexEditView::OnToggleEndian()
{
	begin_change();
	display_.big_endian = !display_.big_endian;
	make_change();
	end_change();

	theApp.SaveToMacro(km_big_endian, (__int64)-1);  // -1 = toggle
}

void CHexEditView::OnUpdateToggleEndian(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(display_.big_endian);
}

void CHexEditView::OnBigEndian()
{
	begin_change();
	display_.big_endian = 1;
	make_change();
	end_change();

	theApp.SaveToMacro(km_big_endian, (__int64)1);  // 1 = big endian on
}

void CHexEditView::OnUpdateBigEndian(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(display_.big_endian);
}

void CHexEditView::OnLittleEndian()
{
	begin_change();
	display_.big_endian = 0;
	make_change();
	end_change();

	theApp.SaveToMacro(km_big_endian, unsigned __int64(0));  // 0 = big endian off (ie little-endian)
}

void CHexEditView::OnUpdateLittleEndian(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!display_.big_endian);
}

void CHexEditView::OnTrackChanges()
{
	begin_change();
	if (display_.hide_replace && display_.hide_insert && display_.hide_delete)
	{
		// All off - so turn them all on
		display_.hide_replace = display_.hide_insert = display_.hide_delete = 0;
	}
	else
	{
		// Turn them all off
		display_.hide_replace = display_.hide_insert = display_.hide_delete = 1;
	}
	make_change();
	end_change();
	theApp.SaveToMacro(km_track_changes, (__int64)-1);  // -1 = toggle
}

void CHexEditView::OnUpdateTrackChanges(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!(display_.hide_replace && display_.hide_insert && display_.hide_delete));
}

void CHexEditView::OnDffdAutoSync()
{
	if (pdfv_ == NULL)
	{
		TaskMessageBox("No Template", "No template is open to sync with.");
		theApp.mac_error_ = 10;
		return;
	}
	begin_change();
	display_.auto_sync_dffd = !display_.auto_sync_dffd;
	make_change();
	end_change();

	// If has been turned on then sync now
	if (display_.auto_sync_dffd)
	{
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		pdfv_->SelectAt(start_addr);
	}

	theApp.SaveToMacro(km_dffd_sync, 2);
}

void CHexEditView::OnUpdateDffdAutoSync(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pdfv_ != NULL);
	pCmdUI->SetCheck(display_.auto_sync_dffd);
}

void CHexEditView::OnDffdSync()
{
	if (pdfv_ == NULL)
	{
		TaskMessageBox("No Template", "No template is open to sync with.");
		theApp.mac_error_ = 10;
		return;
	}

	FILE_ADDRESS start_addr, end_addr;
	GetSelAddr(start_addr, end_addr);
	pdfv_->SelectAt(start_addr);
	pdfv_->SetFocus();
	theApp.SaveToMacro(km_dffd_sync, 255);
}

void CHexEditView::OnUpdateDffdSync(CCmdUI* pCmdUI)
{
	// Don't allow manual sync if auto sync is on
	pCmdUI->Enable(pdfv_ != NULL && !display_.auto_sync_dffd);
}

void CHexEditView::OnSearchHex()        // Alt-L, F6
{
	CSearchEditControl::BeginSearch(CSearchEditControl::mode_hex);
}

void CHexEditView::OnSearchAscii()      // F5
{
	CSearchEditControl::BeginSearch(CSearchEditControl::mode_char);
}

void CHexEditView::OnSearchIcase()      // F4
{
	CSearchEditControl::BeginSearch(CSearchEditControl::mode_icase);
}

// 
CChildFrame *CHexEditView::comp_window()
{
	CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
	CChildFrame *nextc;         // Loops through all MDI child frames
	CChildFrame *compc = NULL;  // Frame of view available to compare with
	BOOL got_one;               // Have we gound an appropriate view?

	// Get the currently active child MDI window
	nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
	ASSERT(nextc != NULL);
	ASSERT(nextc->GetHexEditView() == this);

	// Search for another (non-iconized) window
	for (got_one = FALSE, nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT));
		 nextc != NULL; nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
	{
		if (!nextc->IsIconic())
		{
			// More than one found - use which one?
			if (got_one)
			{
// Can't display message when used in OnUpdateEditCompare call
//              AfxMessageBox("Comparison not performed\n"
//                              "- more than two (non-iconized) windows");
				return NULL;
			}
			got_one = TRUE;
			compc = nextc;
		}
	}

	// If we didn't find a non-iconized window search for iconized one
	if (compc == NULL)
	{
		nextc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());

		// Search for an iconized window
		for (got_one = FALSE, nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT));
			 nextc != NULL; nextc = dynamic_cast<CChildFrame *>(nextc->GetWindow(GW_HWNDNEXT)) )
		{
			ASSERT(nextc->IsIconic());          // else compc != NULL
			// If more than one window - use which one?
			if (got_one)
			{
// Can't display message when used in OnUpdateEditCompare call
//              AfxMessageBox("Comparison not performed\n"
//                              "- more than two windows found");
				return NULL;
			}
			got_one = TRUE;
			compc = nextc;
		}
	}
	return compc;
}

void CHexEditView::OnEditCompare()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = dynamic_cast<CMainFrame *>(AfxGetMainWnd());
	CChildFrame *origc;        // Pointer to MDI child frame of current view

	// Save the currently active child MDI window
	origc = dynamic_cast<CChildFrame *>(mm->MDIGetActive());
	ASSERT(origc != NULL);
	ASSERT(origc->GetHexEditView() == this);

	// Restore current window if iconized
	if (origc->IsIconic())
	{
		WINDOWPLACEMENT wp;
		origc->GetWindowPlacement(&wp);
		wp.showCmd = SW_RESTORE;
		origc->SetWindowPlacement(&wp);
	}

	// Get MDI child frame of compare window
	CChildFrame *compc = comp_window();

	// If we found nothing to compare with, display message and return
	if (compc == NULL)
	{
		CAvoidableDialog::Show(IDS_CANT_COMPARE, "Comparison not performed "
						"- two non-minimized windows required.");
		aa->mac_error_ = 10;
		return;
	}

	// Get view to compare with - active view of compc.  (Actually the
	// active view should be the only view since we don't have splitters.)
	CHexEditView *compv = dynamic_cast<CHexEditView *>(compc->GetHexEditView());
	if (compv == NULL)
	{
		CAvoidableDialog::Show(IDS_CANT_COMPARE, "Cannot compare with this type of window.");
		aa->mac_error_ = 10;
		return;
	}
	ASSERT_KINDOF(CHexEditView, compv);

	CString orig_title, comp_title;     // Title of the windows to be compared
	origc->GetWindowText(orig_title);
	compc->GetWindowText(comp_title);

	CHexEditDoc *origd, *compd;         // Documents of the compared views
	origd = GetDocument();
	compd = compv->GetDocument();

	// Now compare the data from each view starting at END of current selection
	CString mess;                       // Message for user when problem encountered

	FILE_ADDRESS dummy;                         // Not used - start address of selection
	FILE_ADDRESS start_addr;                    // Start address in current view
	FILE_ADDRESS orig_addr, comp_addr;          // Current comp location in both views
	GetSelAddr(dummy, start_addr);
	orig_addr = start_addr;
	compv->GetSelAddr(dummy, comp_addr);
//    start_addr = orig_addr = GetPos();
//    comp_addr = compv->pos2addr(compv->GetCaret());

#ifndef _DEBUG  // Allow self-compare for testing purposes
	// If same doc and same address then we aren't doing anything useful
	if (origd == compd && orig_addr == comp_addr)
	{
		mess.Format("Comparing data with itself in windows:\n\n%s and %s",
						(const char *)orig_title, (const char *)comp_title);
		CAvoidableDialog::Show(IDS_CANT_COMPARE, mess);
		aa->mac_error_ = 10;
		return;
	}
#endif

	size_t orig_got, comp_got;          // How many bytes obtained from docs

	FILE_ADDRESS show_inc = 0x80000;            // How far between showing addresses
	FILE_ADDRESS next_show = (orig_addr/show_inc + 1)*show_inc; // Next address to show
	FILE_ADDRESS slow_show = ((orig_addr+0x800000)/0x800000 + 1)*0x800000;      // When we slow showing
	FILE_ADDRESS comp_length = min(origd->length() - orig_addr, compd->length() - comp_addr);

	// Get memory for compare buffers
	unsigned char *orig_buf = new unsigned char[compare_buf_len];
	unsigned char *comp_buf = new unsigned char[compare_buf_len];

	CWaitCursor wait;
	aa->SaveToMacro(km_compare);

	while (1)
	{
		if (orig_addr > next_show)
		{
			if (AbortKeyPress() &&
				TaskMessageBox("Abort comparison?", 
				    "You have interrupted the comparison.\n\n"
				    "Do you want to stop the process?",MB_YESNO) == IDYES)
			{
				delete[] orig_buf;
				delete[] comp_buf;
				mm->Progress(-1);
				((CMainFrame *)AfxGetMainWnd())
					->StatusBarText("Comparison aborted");
				aa->mac_error_ = 10;
				show_pos();
				return;
			}

			show_pos(next_show);

			// If we've been showing the current address for awhile then show
			// less often (and show progress) to avoid slowing down the actual compare
			if (next_show >= slow_show)
			{
				mm->Progress(int(((next_show-start_addr)*100)/comp_length));
				show_inc = 0x800000;
			}
			AfxGetApp()->OnIdle(0);         // Force display of updated address
			next_show += show_inc;
		}

		orig_got = origd->GetData(orig_buf, compare_buf_len, orig_addr);
		comp_got = compd->GetData(comp_buf, compare_buf_len, comp_addr);

		size_t comp_len = min(orig_got, comp_got);
		if (comp_len == 0)              // EOF of one or both files
			break;

		if (memcmp(orig_buf, comp_buf, comp_len) != 0)
		{
			// Difference found
			size_t pos;
			for (pos = 0; pos < comp_len; ++pos)
				if (orig_buf[pos] != comp_buf[pos])
					break;
			ASSERT(pos < comp_len);

#ifdef SYS_SOUNDS
			CSystemSound::Play("Comparison Difference Found");
#endif

//          mess.Format("Difference found after $%I64X (decimal %I64d) bytes\n"
//                      "%s at address $%I64X (decimal %I64d)\n%s at address $%I64X (decimal %I64d)",
//                      __int64(orig_addr + pos - start_addr), __int64(orig_addr + pos - start_addr),
//                      orig_title, __int64(orig_addr + pos), __int64(orig_addr + pos),
//                      comp_title, __int64(comp_addr + pos), __int64(comp_addr + pos));
//          AfxMessageBox(mess);
			mm->Progress(-1);
			char buf[32];
			sprintf(buf, "%I64d", __int64(orig_addr + pos - start_addr));
			mess = CString("Difference found after ") + buf + CString(" bytes");
			mm->StatusBarText(mess);
			if (!aa->playing_)
				CAvoidableDialog::Show(IDS_COMPARE_DIFF, mess, NULL, 0, MAKEINTRESOURCE(IDI_INFO));

			// Move to where diff found and select that byte in both views
			// (Selecting the byte allows the differences to be seen in both
			// windows without flipping between them, and allows another
			// compare immediately starting at the byte after.)
			compv->MoveWithDesc("Comparison Difference Found ", comp_addr + pos, comp_addr + pos + 1);
			MoveWithDesc("Comparison Difference Found ", orig_addr + pos, orig_addr + pos + 1);
			if (aa->highlight_)
			{
				compv->add_highlight(comp_addr + pos, comp_addr + pos + 1, TRUE);
				add_highlight(orig_addr + pos, orig_addr + pos + 1, TRUE);
			}
			delete[] orig_buf;
			delete[] comp_buf;
			return;
		}

		if (orig_got != comp_got)
			break;

		orig_addr += orig_got;
		comp_addr += comp_got;
	}

	mm->Progress(-1);

	// If we got here then we hit EOF on one or both files
	if (orig_got == comp_got && orig_addr + orig_got - start_addr == 0)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Comparison Difference Not Found");
#endif
		mess = "Both files already at EOF.";
		mm->StatusBarText(mess);
		if (!aa->playing_)
			CAvoidableDialog::Show(IDS_COMPARE_NO_DIFF, mess);
		aa->mac_error_ = 1;
	}
	else if (orig_got == comp_got)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Comparison Difference Not Found");
#endif
		// EOF (both files) encountered
		ASSERT(orig_got == 0);
		show_pos(orig_addr);

		// Display message box when no differences found so that user sees that
		// something happened -- the caret is not moved and they may not notice
		// a message in the status bar (or the status bar may be invisible).
		// (On the other hand if a difference is found then the caret of both views
		// is moved and we just display a message in the status bar -- putting up 
		// a dialog would just be annoying.)

		CString sdec, shex;
		char buf[22];                    // temp buf where we sprintf
		sprintf(buf, "%I64d", __int64(orig_addr + orig_got - start_addr));
		sdec = buf;
		AddCommas(sdec);

		if (aa->hex_ucase_)
			sprintf(buf, "%I64X", __int64(orig_addr + orig_got - start_addr));
		else
			sprintf(buf, "%I64x", __int64(orig_addr + orig_got - start_addr));
		shex = buf;
		AddSpaces(shex);

		mess.Format("No differences found after\n"
					"%s (%sh) bytes.", sdec, shex);
//        mess.Format("No differences found\n"
//                    "after %lX (hex) or\n"
//                    "%ld (decimal) bytes.",
//                    orig_addr + orig_got - start_addr,
//                    orig_addr + orig_got - start_addr);
		mm->StatusBarText(mess);
		if (!aa->playing_)
			CAvoidableDialog::Show(IDS_COMPARE_NO_DIFF, mess);
		aa->mac_error_ = 1;
	}
	else if (orig_got < comp_got)
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Comparison Difference Found");
#endif
		// EOF on orig file before EOF on comp file
//      mess.Format("Difference found after $%I64X (decimal %I64d) bytes\n"
//                  "%s at EOF - address $%I64X (decimal %I64d)\n%s at address $%I64X (decimal %I64d)",
//                  __int64(orig_addr + orig_got - start_addr), __int64(orig_addr + orig_got - start_addr),
//                  orig_title, __int64(orig_addr + orig_got), __int64(orig_addr + orig_got),
//                  comp_title, __int64(comp_addr + orig_got), __int64(comp_addr + orig_got));
//      AfxMessageBox(mess);
		char buf[32];
		sprintf(buf, "%I64d", __int64(orig_addr + orig_got - start_addr));
		mess.Format("EOF on \"%s\" after %s bytes", orig_title, buf);
		mm->StatusBarText(mess);
		if (!aa->playing_)
			CAvoidableDialog::Show(IDS_COMPARE_DIFF, mess, NULL, 0, MAKEINTRESOURCE(IDI_INFO));

		compv->MoveWithDesc("Comparison Difference Found ", comp_addr + orig_got, comp_addr + orig_got + 1);
		if (aa->highlight_)
			compv->add_highlight(comp_addr + orig_got, comp_addr + orig_got + 1, TRUE);
		MoveWithDesc("EOF in Comparison ", orig_addr + orig_got);
	}
	else
	{
#ifdef SYS_SOUNDS
		CSystemSound::Play("Comparison Difference Found");
#endif
		// EOF on comp file before EOF on orig file
//      mess.Format("Difference found after $%lX (decimal %ld) bytes\n"
//                  "%s at address $%lX (decimal %ld)\n%s at EOF - address $%lX (decimal %ld)",
//                  orig_addr + comp_got - start_addr, orig_addr + comp_got - start_addr,
//                  orig_title, orig_addr + comp_got, orig_addr + comp_got,
//                  comp_title, comp_addr + comp_got, comp_addr + comp_got);
//      AfxMessageBox(mess);
		char buf[32];
		sprintf(buf, "%I64d", __int64(orig_addr + comp_got - start_addr));
		mess.Format("EOF on \"%s\" after %s bytes", comp_title, buf);
		mm->StatusBarText(mess);
		if (!aa->playing_)
			CAvoidableDialog::Show(IDS_COMPARE_DIFF, mess, NULL, 0, MAKEINTRESOURCE(IDI_INFO));

		compv->MoveWithDesc("EOF in Comparison ", comp_addr + comp_got);
		MoveWithDesc("Comparison Difference Found ", orig_addr + comp_got, orig_addr + comp_got + 1);
		if (aa->highlight_)
			add_highlight(orig_addr + comp_got, orig_addr + comp_got + 1, TRUE);
	}
	delete[] orig_buf;
	delete[] comp_buf;
}

void CHexEditView::OnUpdateEditCompare(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(comp_window() != NULL);
}

// Activate next window
void CHexEditView::OnWindowNext()
{
	// Get the next window
	CHexEditView *pnext = NextView();

	if (pnext == NULL)
	{
		TaskMessageBox("Warning", "No other non-minimized windows found");
		theApp.mac_error_ = 2;
		return;
	}

	// Make new view active
	pnext->GetFrame()->MDIActivate();           // Activate next frame
	pnext->GetFrame()->SetActiveView(pnext);    // Make sure active view is normal (hex) view

	ASSERT(pnext == GetView());       // Make sure it is now the active view
	if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_focus)
	{
		// We don't want focus change recorded (see CHexEditView::OnSetFocus)
		theApp.mac_.pop_back();
	}
	theApp.SaveToMacro(km_win_next);
}

// Return next view in list non-minimised view or NULL if none.
// Cycles through the list in reverse order so repeated calls
// and focus changes still return all the non-minimised frames.
CHexEditView * CHexEditView::NextView() const
{
	CChildFrame *currc = dynamic_cast<CChildFrame *>(GetFrame());
	ASSERT(currc != NULL);
	CChildFrame *nextc = currc;

	do
	{
		// Get previous sibling window (which are the child frames of other views)
		CWnd * pw = nextc->GetWindow(GW_HWNDPREV);

		// If reached the top of the list go to the end
		if (pw == NULL)
			pw = nextc->GetWindow(GW_HWNDLAST);

		// Skip any windows that are not child frames (eg MFC 10 has added a CMFCTabCtrl window at this level)
		while (!pw->IsKindOf(RUNTIME_CLASS(CChildFrame)))
		{
			pw = pw->GetWindow(GW_HWNDPREV);
			if (pw == NULL)
				pw = nextc->GetWindow(GW_HWNDLAST);
		}

		// Get the window as a child frame
		nextc = dynamic_cast<CChildFrame *>(pw);
	} while (nextc != NULL && nextc != currc && nextc->IsIconic());

	if (nextc == NULL || nextc == currc)
		return NULL;
	else
		return nextc->GetHexEditView();
}
// Same as NextView but returns the previous view.  This will be the previous most recently
// viewed window - ie, when called for the active window it will be the previous active window.
CHexEditView * CHexEditView::PrevView() const
{
	CChildFrame *currc = dynamic_cast<CChildFrame *>(GetFrame());
	ASSERT(currc != NULL);
	CChildFrame *prevc = currc;

	do
	{
		// Get next sibling window (which are the child frames of other views)
		CWnd * pw = prevc->GetWindow(GW_HWNDNEXT);

		// If reached the end of the list go back around
		if (pw == NULL)
			pw = prevc->GetWindow(GW_HWNDFIRST);

		// Skip any windows that are not child frames (eg MFC 10 has added a CMFCTabCtrl window at this level)
		while (!pw->IsKindOf(RUNTIME_CLASS(CChildFrame)))
		{
			pw = pw->GetWindow(GW_HWNDNEXT);
			if (pw == NULL)
				pw = prevc->GetWindow(GW_HWNDFIRST);
		}

		// Get the window as a child frame
		prevc = dynamic_cast<CChildFrame *>(pw);
	} while (prevc != NULL && prevc != currc && prevc->IsIconic());

	if (prevc == NULL || prevc == currc)
		return NULL;
	else
		return prevc->GetHexEditView();
}

void CHexEditView::OnAscii2Ebcdic()
{
	DoConversion(CONVERT_ASC2EBC, "convert from ASCII to EBCDIC");
}

void CHexEditView::OnEbcdic2Ascii()
{
	DoConversion(CONVERT_EBC2ASC, "convert from EBCDIC to ASCII");
}

void CHexEditView::OnAnsi2Ibm()
{
	DoConversion(CONVERT_ANSI2IBM, "convert from ANSI to IBM/OEM");
}

void CHexEditView::OnIbm2Ansi()
{
	DoConversion(CONVERT_IBM2ANSI, "convert from IBM/OEM to ANSI");
}

void CHexEditView::OnUppercase()
{
	DoConversion(CONVERT_UPPER, "convert to upper case");
}

void CHexEditView::OnLowercase()
{
	DoConversion(CONVERT_LOWER, "convert to lower case");
}

// Performs particular conversions on a memory buffer
void CHexEditView::ProcConversion(unsigned char *buf, size_t count, convert_type op)
{
	// Operate on all the selected values
	for (size_t ii = 0; ii < count; ++ii)
	{
		switch (op)
		{
		case CONVERT_ASC2EBC:
			if (buf[ii] < 128)
				buf[ii] = a2e_tab[buf[ii]];
			else
				buf[ii] = '\0';
			break;
		case CONVERT_EBC2ASC:
			buf[ii] = e2a_tab[buf[ii]];
			break;
		case CONVERT_ANSI2IBM:
			if (buf[ii] > 128)
				buf[ii] = a2i_tab[buf[ii]&0x7F];
			break;
		case CONVERT_IBM2ANSI:
			if (buf[ii] > 128)
				buf[ii] = i2a_tab[buf[ii]&0x7F];
			break;
		case CONVERT_UPPER:
			if (EbcdicMode())
			{
				if (buf[ii] >= 0x81 && buf[ii] <= 0x89 ||
					buf[ii] >= 0x91 && buf[ii] <= 0x99 ||
					buf[ii] >= 0xA2 && buf[ii] <= 0xA9 )
				{
					buf[ii] += 0x40;
				}
			}
			else if (AnsiMode() || buf[ii] < 128)
				buf[ii] = toupper(buf[ii]);
			break;
		case CONVERT_LOWER:
			if (EbcdicMode())
			{
				if (buf[ii] >= 0xC1 && buf[ii] <= 0xC9 ||
					buf[ii] >= 0xD1 && buf[ii] <= 0xD9 ||
					buf[ii] >= 0xE2 && buf[ii] <= 0xE9 )
				{
				buf[ii] -= 0x40;
				}
			}
			else if (AnsiMode() || buf[ii] < 128)
				buf[ii] = tolower(buf[ii]);
			break;
		default:
			ASSERT(0);
		}
	}
}

// This handles conversion operations (everything except for the actual data
// convert operation - see ProcConversion) including getting the selection
// and allocating memory buffers to store the result (or temp file for
// really big selections) and updating the document.
// It also updates progress in the status bar and allows user abort.
void CHexEditView::DoConversion(convert_type op, LPCSTR desc)
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	if (check_ro(desc))
		return;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	// Make sure there is a selection
	if (start_addr >= end_addr)
	{
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("There is nothing selected to %s", desc);
		TaskMessageBox("No Selection", mess);
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < end_addr && end_addr <= GetDocument()->length());

	// Test if selection is too big to do in memory
	if (end_addr - start_addr > (16*1024*1024) && GetDocument()->DataFileSlotFree())
	{
		int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)

		// Create a file to store the resultant data
		// (Temp file used by the document until it is closed or written to disk.)
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		// Get data buffer
		size_t len, buflen = size_t(min(4096, end_addr - start_addr));
		try
		{
			buf = new unsigned char[buflen];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}

		try
		{
			CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

			for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
			{
				// Get the next buffer full from the document
				len = size_t(min(buflen, end_addr - curr));
				if (op != unary_at)      // We don't need to get old value if assigning
					VERIFY(GetDocument()->GetData(buf, len, curr) == len);

				ProcConversion(buf, len, op);

				ff.Write(buf, len);

				if (AbortKeyPress())
				{
					CString mess;
					mess.Format("Abort %s?", desc);
					if (TaskMessageBox(mess, 
						"Do you want to stop the process?",MB_YESNO) == IDYES)
					{
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}
				}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			}
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();

			remove(temp_file);
			theApp.mac_error_ = 10;
			goto func_return;
		}

		// Delete the unprocessed block that is to be replaced
		// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
		GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, this);

		// Add the temp file to the document
		idx = GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);
		GetDocument()->Change(mod_insert_file, start_addr, end_addr - start_addr, NULL, idx, this, TRUE);
	}
	else
	{
		// Allocate memory block and process it

		// Error if ran out of temp file handles and file is too big
		if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Conversion Error",
						  "HexEdit Pro is out of temporary files and "
						  "cannot convert such a large selection.\n\n"
						  "Please save the file to deallocate "
						  "temporary file handles and try again.");
			theApp.mac_error_ = 10;
			return;
		}

		// Warn if we might cause memory exhaustion
		if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Conversion Warning",
				"HexEdit Pro is out of temporary file "
				"handles and converting such a large selection "
				"may cause memory exhaustion.  Please click "
				"\"No\" and save the file to free handles "
				"or click \"Yes\" to continue.\n\n"
				"Do you want to continue?",
				MB_YESNO) != IDYES)
			{
				theApp.mac_error_ = 5;
				return;
			}
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		try
		{
			buf = new unsigned char[size_t(end_addr - start_addr)];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}
		size_t got = GetDocument()->GetData(buf, size_t(end_addr - start_addr), start_addr);
		ASSERT(got == size_t(end_addr - start_addr));

		ProcConversion(buf, got, op);

		GetDocument()->Change(mod_replace, start_addr, got, (unsigned char *)buf, 0, this);
	}

	DisplayCaret();
	theApp.SaveToMacro(km_convert, op);

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

// Make sure there is a selection before doing conversion
void CHexEditView::OnUpdateConvert(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start_addr < end_addr ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(start_addr < end_addr);
}

void CHexEditView::OnEncrypt()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (check_ro("encrypt"))
		return;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	// Make sure there is a selection
	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(aa->playing_);
		TaskMessageBox("No Selection", "There is nothing selected to encrypt.");
		aa->mac_error_ = 10;
		return;
	}

	if (aa->password_.IsEmpty())
	{
		CPassword dlg;
		dlg.m_password = aa->password_;
		if (dlg.DoModal() == IDOK)
		{
			aa->password_ = dlg.m_password;
			// Create encryption key with new password
			if (aa->algorithm_ > 0)
			{
				ASSERT(aa->algorithm_ - 1 < (int)aa->crypto_.GetNum());
				aa->crypto_.SetPassword(aa->algorithm_-1, aa->password_);
			}
		}
		else
			return;
	}
	else if (aa->algorithm_ > 0 && aa->crypto_.NeedsPassword(aa->algorithm_-1))
	{
		// We have a password but somehow it has not been set for this alg
		ASSERT(aa->algorithm_ - 1 < (int)aa->crypto_.GetNum());
		aa->crypto_.SetPassword(aa->algorithm_-1, aa->password_);
	}

	unsigned char *buf = NULL;

	if (aa->algorithm_ > 0)
	{
		if (display_.overtype && aa->crypto_.needed(aa->algorithm_-1, 256) != 256)
		{
			if (CAvoidableDialog::Show(IDS_OVERTYPE,
				"Encrypting with this algorithm will increase the length "
				"of the current (unencrypted) selection and increase the "
				"length of the file.  This is not permitted in OVR mode.\n\n."
				"Do you want to turn off overtype mode?",
				"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
			{
				aa->mac_error_ = 10;
				return;
			}
			else if (!do_insert())
				return;
		}

		// Test if selection is too big to do in memory
		if (end_addr - start_addr > (16*1024*1024) && GetDocument()->DataFileSlotFree())
		{
			CWaitCursor wait;                                  // Turn on wait cursor (hourglass)

			// Get input and output buffers
			size_t inbuflen = 32768;
			size_t outbuflen = aa->crypto_.needed(aa->algorithm_-1, inbuflen);

			FILE_ADDRESS total_out = 0;
			try
			{
				buf = new unsigned char[outbuflen];
			}
			catch (std::bad_alloc)
			{
				AfxMessageBox("Insufficient memory");
				theApp.mac_error_ = 10;
				return;
			}

			// Create a "temp" file to store the encrypted data
			// (This file stores the data until the document is closed or written to disk.)
			char temp_dir[_MAX_PATH];
			char temp_file[_MAX_PATH];
			::GetTempPath(sizeof(temp_dir), temp_dir);
			::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

			try
			{
				CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

				size_t len;                                    // size of data to be encrypted
				for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
				{
					// Get the next buffer full from the document
					len = size_t(min(inbuflen, end_addr - curr));
					VERIFY(GetDocument()->GetData(buf, len, curr) == len);

					// Encrypt and write it
					size_t outlen = aa->crypto_.encrypt(aa->algorithm_-1, buf, len, outbuflen, curr + len == end_addr);
					ff.Write(buf, outlen);
					total_out += outlen;

					if (AbortKeyPress() &&
						TaskMessageBox("Abort encryption?", 
						    "You have interrupted the encryption.\n\n"
						    "Do you want to stop the process?",MB_YESNO) == IDYES)
					{
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
				}
			}
			catch (CFileException *pfe)
			{
				TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
				pfe->Delete();

				remove(temp_file);
				theApp.mac_error_ = 10;
				goto func_return;
			}

			// Delete the unencrypted block that is to be replaced
			// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
			GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, this);

			// Add the temp file to the document
			int idx = GetDocument()->AddDataFile(temp_file, TRUE);
			ASSERT(idx != -1);
			GetDocument()->Change(mod_insert_file, start_addr, total_out, NULL, idx, this, TRUE);

			// If length changed update the selection and makde sure it is undoable
			if (total_out != end_addr - start_addr)
			{
				// Select encrypted data
				SetSel(addr2pos(start_addr), addr2pos(start_addr + total_out));

				// Inform the user in case they don't notice the selection has been increased
				CString mess;
				mess.Format("Encryption increased the selection length by %ld bytes", long(total_out - (end_addr - start_addr)));
				((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
			}
		}
		else
		{
			// Error if ran out of temp file handles and file is too big
			if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
			{
				TaskMessageBox("Encryption Error",
					"HexEdit Pro is out of temporary files and "
					"cannot encrypt such a large selection.\n\n"
					"Please save the file to deallocate "
					"temporary file handles and try again.");
				theApp.mac_error_ = 10;
				return;
			}

			// Warn if we might cause memory exhaustion
			if (end_addr - start_addr > 128*1024*1024)  // More than 128 Mb may be too big
			{
				if (TaskMessageBox("Encryption Warning",
					"HexEdit Pro is out of temporary file "
					"handles and encrypting such a large selection "
					"may cause memory exhaustion.  Please click "
					"\"No\" and save the file to free handles "
					"or click \"Yes\" to continue.\n\n"
					"Do you want to continue?",
					MB_YESNO) != IDYES)
				{
					theApp.mac_error_ = 5;
					return;
				}
			}

			CWaitCursor wait;                           // Turn on wait cursor (hourglass)

			size_t len = size_t(end_addr - start_addr); // Length of selection
			size_t outlen = aa->crypto_.needed(aa->algorithm_-1, len);

			// Get memory for selection and read it from the document
			try
			{
				buf = new unsigned char[outlen];
			}
			catch (std::bad_alloc)
			{
				AfxMessageBox("Insufficient memory");
				aa->mac_error_ = 10;
				return;
			}

			VERIFY(GetDocument()->GetData(buf, len, start_addr) == len);

			if (aa->crypto_.encrypt(aa->algorithm_-1, buf, len, outlen) != outlen)
			{
				ASSERT(0);
				AfxMessageBox("Encryption error");
				aa->mac_error_ = 10;
				goto func_return;
			}

			if (len != outlen)
			{
				ASSERT(outlen > len);

				// Since the encrypted text is bigger we must delete then insert
				GetDocument()->Change(mod_delforw, start_addr, len, NULL, 0, this);
				GetDocument()->Change(mod_insert, start_addr, outlen, buf, 0, this, TRUE);
				ASSERT(undo_.back().utype == undo_change);
				undo_.back().previous_too = true;    // Merge changes (at least in this view)

				// Select encrypted data and save undo of selection
				SetSel(addr2pos(start_addr), addr2pos(start_addr+outlen));

				// Inform the user in case they don't notice the selection has been increased
				CString mess;
				mess.Format("Encryption increased the selection length by %ld bytes", long(outlen-len));
				((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
			}
			else
				GetDocument()->Change(mod_replace, start_addr, len, buf, 0, this);
		}
	}
	else
	{
		// Note that CryptoAPI algs (above) the key is set when password
		// or alg is changed (more efficient) but for internal we set it here
		// in case other internal use (security) has changed the current key
		::set_key(aa->password_, aa->password_.GetLength());

		// Make sure selection is right size
		if ((end_addr-start_addr)%8 != 0)
		{
			// Presumably in macro playback
			ASSERT(aa->playing_);
			CAvoidableDialog::Show(IDS_SEL_LEN,
				"The selection length is not a multiple of 8.  "
				"For this encryption algorithm the length of the "
				"selection must be a multiple of the encryption block length.");
			aa->mac_error_ = 10;
			return;
		}

		size_t len; // Length of selection

		// Note: for internal alg. we don't break the encryption up and write
		// to temp file in order to handle huge selections as above (yet?)

		// Get memory for selection and read it from the document
		try
		{
			if (end_addr - start_addr > UINT_MAX)
				throw std::bad_alloc();
			len = size_t(end_addr - start_addr);
			buf = new unsigned char[len];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory - selection too large");
			aa->mac_error_ = 10;
			return;
		}

		VERIFY(GetDocument()->GetData(buf, len, start_addr) == len);

		::encrypt(buf, len);

		GetDocument()->Change(mod_replace, start_addr, len, buf, 0, this);
	}
	DisplayCaret();
	aa->SaveToMacro(km_encrypt, 1);  // 1 == encrypt (2 == decrypt)

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnDecrypt()
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	if (aa->algorithm_ < 0 || check_ro("decrypt"))
		return;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	// Make sure there is a selection
	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(aa->playing_);
		TaskMessageBox("No Selection", "There is no selection to decrypt.");
		aa->mac_error_ = 10;
		return;
	}

	if (aa->password_.IsEmpty())
	{
		CPassword dlg;
		dlg.m_password = aa->password_;
		if (dlg.DoModal() == IDOK)
		{
			aa->password_ = dlg.m_password;
			// Create encryption key with new password
			if (aa->algorithm_ > 0)
			{
				ASSERT(aa->algorithm_ - 1 < (int)aa->crypto_.GetNum());
				aa->crypto_.SetPassword(aa->algorithm_-1, aa->password_);
			}
		}
		else
			return;
	}
	else if (aa->algorithm_ > 0 && aa->crypto_.NeedsPassword(aa->algorithm_-1))
	{
		// We have a password but somehow it has not been set for this alg
		ASSERT(aa->algorithm_ - 1 < (int)aa->crypto_.GetNum());
		aa->crypto_.SetPassword(aa->algorithm_-1, aa->password_);
	}

	unsigned char *buf = NULL;

	if (aa->algorithm_ > 0)
	{
		int blen = aa->crypto_.GetBlockLength(aa->algorithm_-1);
		if (blen == -1)
		{
			AfxMessageBox("Encryption block length error");
			aa->mac_error_ = 10;
			return;
		}

		if (blen != 0 && display_.overtype)
		{
			if (CAvoidableDialog::Show(IDS_OVERTYPE,
				"Decrypting with this algorithm may reduce increase the length "
				"of the current (encrypted) selection and hence decrease the "
				"length of the file.  This is not permitted in OVR mode.\n\n."
				"Do you want to turn off overtype mode?",
				"", MLCBF_OK_BUTTON | MLCBF_CANCEL_BUTTON) != IDOK)
			{
				aa->mac_error_ = 10;
				return;
			}
			else if (!do_insert())
				return;
		}

		ASSERT(blen%8 == 0); // Should be multiple of number of bits/byte
		if (blen == 0)
			blen = 1;       // Stream cipher - don't restrict selection length
		else
			blen = blen/8;  // Convert bits to bytes

		// Make sure selection is right size
		if ((end_addr-start_addr)%blen != 0)
		{
			// Presumably in macro playback
			ASSERT(aa->playing_);
			CString ss;
			ss.Format("%d", blen);
			CAvoidableDialog::Show(IDS_SEL_LEN,
				"The selection length is not a multiple of " + ss + ".  "
				"For this encryption algorithm the length of the "
				"selection must be a multiple of the encryption block length.");
			aa->mac_error_ = 10;
			return;
		}

		// Test if selection is too big to do in memory
		if (end_addr - start_addr > (16*1024*1024) && GetDocument()->DataFileSlotFree())
		{
			CWaitCursor wait;                           // Turn on wait cursor (hourglass)

			size_t len, buflen = 32768;
			ASSERT(buflen % blen == 0);    // buffer length must be multiple of cipher block length

			FILE_ADDRESS total_out = 0;
			try
			{
				buf = new unsigned char[buflen];
			}
			catch (std::bad_alloc)
			{
				AfxMessageBox("Insufficient memory");
				theApp.mac_error_ = 10;
				return;
			}

			// Create a "temp" file to store the decrypted data
			// (This file stores the data until the document is closed or written to disk.)
			char temp_dir[_MAX_PATH];
			char temp_file[_MAX_PATH];
			::GetTempPath(sizeof(temp_dir), temp_dir);
			::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

			try
			{
				CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

				for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
				{
					// Get the next buffer full from the document
					len = size_t(min(buflen, end_addr - curr));
					VERIFY(GetDocument()->GetData(buf, len, curr) == len);

					size_t outlen = aa->crypto_.decrypt(aa->algorithm_-1, buf, len, curr + len == end_addr);

					if (outlen == size_t(-1))
					{
						AfxMessageBox(CString("Could not decrypt using:\n") + aa->crypto_.GetName(aa->algorithm_-1));
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}

					ff.Write(buf, outlen);
					total_out += outlen;

					if (AbortKeyPress() &&
						TaskMessageBox("Abort decryption?", 
						    "You have interrupted the decryption.\n\n"
						    "Do you want to stop the process?",MB_YESNO) == IDYES)
					{
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
				}
			}
			catch (CFileException *pfe)
			{
				TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
				pfe->Delete();

				remove(temp_file);
				theApp.mac_error_ = 10;
				goto func_return;
			}

			// Delete the original encrypted block that is to be replaced
			// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
			GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, this);

			// Add the temp file to the document
			int idx = GetDocument()->AddDataFile(temp_file, TRUE);
			ASSERT(idx != -1);
			GetDocument()->Change(mod_insert_file, start_addr, total_out, NULL, idx, this, TRUE);

			if (total_out != end_addr - start_addr)
			{
				// Select the decrypted data
				SetSel(addr2pos(start_addr), addr2pos(start_addr + total_out));

				// Inform the user in case they don't notice the selection has been increased
				CString mess;
				mess.Format("Decryption decreased the selection length by %ld bytes", long(end_addr - start_addr - total_out));
				((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
			}
		}
		else
		{
			// Error if ran out of temp file handles and file is too big
			if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
			{
				TaskMessageBox("Decryption Error",
					"HexEdit Pro is out of temporary files and "
					"cannot decrypt such a large selection.\n\n"
					"Please save the file to deallocate "
					"temporary file handles and try again.");
				theApp.mac_error_ = 10;
				return;
			}

			// Warn if we might cause memory exhaustion
			if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
			{
				if (TaskMessageBox("Decryption Warning",
					"HexEdit Pro is out of temporary file "
					"handles and decrypting such a large selection "
					"may cause memory exhaustion.  Please click "
					"\"No\" and save the file to free handles "
					"or click \"Yes\" to continue.\n\n"
					"Do you want to continue?",
					MB_YESNO) != IDYES)
				{
					theApp.mac_error_ = 5;
					return;
				}
			}

			size_t len = size_t(end_addr - start_addr); // Length of selection
			size_t outlen;          // Size of decrypted text (may be less than encrypted)

			// Get memory for selection and read it from the document
			try
			{
				buf = new unsigned char[len];
			}
			catch (std::bad_alloc)
			{
				AfxMessageBox("Insufficient memory");
				aa->mac_error_ = 10;
				return;
			}
			VERIFY(GetDocument()->GetData(buf, len, start_addr) == len);

			outlen = aa->crypto_.decrypt(aa->algorithm_-1, buf, len);
			if (outlen == size_t(-1))
			{
				AfxMessageBox(CString("Could not decrypt using:\n") + aa->crypto_.GetName(aa->algorithm_-1));
				aa->mac_error_ = 10;
				goto func_return;
			}

			if (len != outlen)
			{
				ASSERT(outlen < len);

				// Since the decrypted text is smaller we must delete then insert
				GetDocument()->Change(mod_delforw, start_addr, len, NULL, 0, this);
				GetDocument()->Change(mod_insert, start_addr, outlen, buf, 0, this, TRUE);
				ASSERT(undo_.back().utype == undo_change);
				undo_.back().previous_too = true;    // Merge changes (at least in this view)

				// Select encrypted data and save undo of selection
				SetSel(addr2pos(start_addr), addr2pos(start_addr+outlen));

				// Inform the user in case they don't notice the selection has been decreased
				CString mess;
				mess.Format("Decryption decreased the selection length by %ld bytes", long(len-outlen));
				((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
			}
			else
				GetDocument()->Change(mod_replace, start_addr, len, buf, 0, this);
		}
	}
	else
	{
		// Note that CryptoAPI algs (above) the key is set when password
		// or alg is changed (more efficient) but for internal we set it here
		// in case other internal use (security) has changed the current key
		::set_key(aa->password_, aa->password_.GetLength());

		// Make sure selection is right size
		if ((end_addr-start_addr)%8 != 0)
		{
			// Presumably in macro playback
			ASSERT(aa->playing_);
			CAvoidableDialog::Show(IDS_SEL_LEN,
				"The selection length is not a multiple of 8.  "
				"For this encryption algorithm the length of the "
				"selection must be a multiple of the encryption block length.");
			aa->mac_error_ = 10;
			return;
		}

		CWaitCursor wait;                       // Turn on wait cursor (hourglass)

		size_t len;                             // Length of selection

		// Note: for internal alg. we don't break the decryption up and write
		// to temp file to handle huge selections as above (yet?)

		// Get memory for selection and read it from the document
		try
		{
			if (end_addr - start_addr > UINT_MAX)
				throw std::bad_alloc();
			len = size_t(end_addr - start_addr);
			buf = new unsigned char[len];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			aa->mac_error_ = 10;
			return;
		}

		VERIFY(GetDocument()->GetData(buf, len, start_addr) == len);

		::decrypt(buf, len);

		GetDocument()->Change(mod_replace, start_addr, len, buf, 0, this);
	}
	DisplayCaret();
	aa->SaveToMacro(km_encrypt, 2);  // 2 == decrypt (1 == encrypt)

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnUpdateEncrypt(CCmdUI* pCmdUI)
{
	CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		pCmdUI->Enable(FALSE);
	else if (aa->algorithm_ == 0)
	{
		// Internal algorithm must encrypt mult. of 8 bytes (Blow Fish has 64 bit block length)
		pCmdUI->Enable((end_addr-start_addr)%8 == 0);
	}
}

void CHexEditView::OnCompress()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *in_data = NULL;
	unsigned char *out_data = NULL;

	// Check if file is read-only and ask user if they want to turn this off
	if (check_ro("compress") || check_ovr("compress"))
		return;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	// Make sure there is a selection
	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("No Selection", "There is no selection to compress.");
		theApp.mac_error_ = 10;
		return;
	}

	// Initialise structure used to communicate with the zlib routines
	z_stream zs;
	int level      = Z_DEFAULT_COMPRESSION;
	if (theApp.GetProfileInt("Options", "ZlibCompressionDVLevel", 1) == 0)
		level = theApp.GetProfileInt("Options", "ZlibCompressionLevel", level);
	ASSERT(level == Z_DEFAULT_COMPRESSION || (level >= 1 && level <= 9));
	int windowBits = 15;
	if (theApp.GetProfileInt("Options", "ZlibCompressionDVWindow", 1) == 0)
		windowBits = theApp.GetProfileInt("Options", "ZlibCompressionWindow", windowBits);
	ASSERT(windowBits >= 8 && windowBits <= 15);
	switch (theApp.GetProfileInt("Options", "ZlibCompressionHeaderType", 1))
	{
	case 0: // raw (no header)
		windowBits = - windowBits;
		break;
	case 2: // gzip header
		windowBits += 16;
		break;
	default:
		ASSERT(0);
	case 1:
		break;
	}
	int memLevel   = 8;
	if (theApp.GetProfileInt("Options", "ZlibCompressionDVMemory", 1) == 0)
		memLevel = theApp.GetProfileInt("Options", "ZlibCompressionMemory", memLevel);
	ASSERT(memLevel >= 1 && memLevel <= 9);
	int strategy   = theApp.GetProfileInt("Options", "ZlibCompressionStrategy", Z_DEFAULT_STRATEGY);
	ASSERT( strategy == Z_DEFAULT_STRATEGY ||
			strategy == Z_FILTERED ||
			strategy == Z_HUFFMAN_ONLY ||
			strategy == Z_RLE ||
			strategy == Z_FIXED
		  );

	zs.zalloc = (alloc_func)0;
	zs.zfree = (free_func)0;
	zs.opaque = (voidpf)0;
//	VERIFY(deflateInit(&zs, Z_DEFAULT_COMPRESSION) == Z_OK);
	VERIFY(deflateInit2(&zs,
						level,
						Z_DEFLATED,  // only method currently supported by zlib
						windowBits,
						memLevel,
						strategy
					   ) == Z_OK);

	// Test if selection is too big to do in memory
	if (end_addr - start_addr > (16*1024*1024) && GetDocument()->DataFileSlotFree())
	{
		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)

		// Create a "temp" file to store the compressed data
		// (This file stores the compressed data until the document is closed or written to disk.)
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		// Get input and output buffers
		size_t len, inbuflen = size_t(min(32768, end_addr - start_addr)), outbuflen = max(inbuflen/2, 256);

		// Work out no of input blocks per sync
		int sync_every = (theApp.GetProfileInt("Options", "ZlibCompressionSync", 0)*1024)/inbuflen;
		FILE_ADDRESS total_out = 0;
		int err;
		try
		{
			in_data = new unsigned char[inbuflen];
			out_data = new unsigned char[outbuflen];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}

		try
		{
			CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

			for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
			{
				int flush;
				if (sync_every > 0 && (curr - start_addr)/inbuflen % sync_every == sync_every - 1)  // time to sync?
					flush = Z_FULL_FLUSH;
				else
					flush = Z_NO_FLUSH;

				// Get the next buffer full from the document
				len = size_t(min(inbuflen, end_addr - curr));
				VERIFY(GetDocument()->GetData(in_data, len, curr) == len);

				// Compress this buffer
				zs.next_in = in_data;
				zs.avail_in = len;
				do
				{
					zs.next_out = out_data;
					zs.avail_out = outbuflen;
					err = deflate(&zs, flush);
					ASSERT(err == Z_OK);

					// Write to "temp" file
					ff.Write(out_data, outbuflen - zs.avail_out);

					total_out += outbuflen - zs.avail_out;  // we need to keep track of this ourselves since zs.total_out can overflow
				} while (zs.avail_out == 0);
				ASSERT(zs.avail_in == 0);

				if (AbortKeyPress() &&
					TaskMessageBox("Abort compression?", 
					    "You have interrupted the compression.\n\n"
					    "Do you want to stop the process?",MB_YESNO) == IDYES)
				{
					ff.Close();
					remove(temp_file);
					theApp.mac_error_ = 10;
					goto func_return;
				}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			}

			// Process any residual data
			do
			{
				zs.next_out = out_data;
				zs.avail_out = outbuflen;
				err = deflate(&zs, Z_FINISH);
				ASSERT(err == Z_OK || err == Z_STREAM_END);

				ff.Write(out_data, outbuflen - zs.avail_out);
				total_out += outbuflen - zs.avail_out;
			} while (err == Z_OK);
			ASSERT(err == Z_STREAM_END);
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();

			remove(temp_file);
			theApp.mac_error_ = 10;
			goto func_return;
		}

		// Delete the uncompressed block that is to be replaced
		// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
		GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, this);

		// Add the temp file to the document
		idx = GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);
		GetDocument()->Change(mod_insert_file, start_addr, total_out, NULL, idx, this, TRUE);

		// Select compressed data
		SetSel(addr2pos(start_addr), addr2pos(start_addr + total_out));

		// Inform the user about the amount of compression
		CString mess;
		mess.Format("Compressed %d%%", int((1.0 - double(total_out)/(end_addr - start_addr))*100.0 + 0.5));
		mm->StatusBarText(mess);
	}
	else
	{
		// Allocate memory block and compress into it

		// Error if ran out of temp file handles and file is too big
		if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Compression Error",
				"HexEdit Pro is out of temporary files and "
				"cannot compress such a large selection.\n\n"
				"Please save the file to deallocate "
				"temporary file handles and try again.");
			theApp.mac_error_ = 10;
			return;
		}

		// Warn if we might cause memory exhaustion
		if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Compression Warning",
				"HexEdit Pro is out of temporary file "
				"handles and compressing such a large selection "
				"may cause memory exhaustion.  Please click "
				"\"No\" and save the file to free handles "
				"or click \"Yes\" to continue.\n\n"
				"Do you want to continue?",
				MB_YESNO) != IDYES)
			{
				theApp.mac_error_ = 5;
				return;
			}
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		try
		{
			// Get buffer for data, get the data, and get buffer for compressed data
			in_data = new unsigned char[size_t(end_addr - start_addr)];
			size_t got = GetDocument()->GetData(in_data, size_t(end_addr - start_addr), start_addr);
			ASSERT(got == size_t(end_addr - start_addr));

			// Remove uncompressed data from the document
			GetDocument()->Change(mod_delforw, start_addr, got, NULL, 0, this);

			size_t outbuflen = max(got/2, 256);
			out_data = new unsigned char[outbuflen];
			FILE_ADDRESS curr = start_addr;

			int err;    // return value from inflate (normally Z_OK or Z_STREAM_END)

			zs.next_in = in_data;
			zs.avail_in = got;
			do
			{
				zs.next_out = out_data;
				zs.avail_out = outbuflen;
				err = deflate(&zs, Z_FINISH);
				ASSERT(err == Z_OK || err == Z_STREAM_END);

				// Replace the selection with the compressed data
				if (outbuflen - zs.avail_out > 0)
					GetDocument()->Change(mod_insert, curr, outbuflen - zs.avail_out, out_data, 0, this, TRUE);

				curr += outbuflen - zs.avail_out;
			} while (err == Z_OK);
			ASSERT(err == Z_STREAM_END);

			// Select compressed block
			SetSel(addr2pos(start_addr), addr2pos(start_addr+zs.total_out));

			// Inform the user about the amount of compression
			CString mess;
			mess.Format("Compressed %d%%", int((1.0 - double(zs.total_out)/got)*100.0 + 0.5));
			mm->StatusBarText(mess);
		}
		catch (std::bad_alloc)
		{
			// Note: this exception may come from ZLIB
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;

			// Undo changes already made (but make sure we don't record undo in macros)
			OnEditUndo();
			if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_undo)
				theApp.mac_.pop_back();

			goto func_return;
		}
	}
	VERIFY(deflateEnd(&zs) == Z_OK);

	DisplayCaret();
	theApp.SaveToMacro(km_compress, 1);  // 1 = compress (2 = decompress)

func_return:
	mm->Progress(-1);  // disable progress bar

	if (in_data != NULL)
		delete[] in_data;
	if (out_data != NULL)
		delete[] out_data;
}

void CHexEditView::OnDecompress()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *in_data = NULL;
	unsigned char *out_data = NULL;

	// Check if file is read-only and ask user if they want to turn this off
	if (check_ro("decompress") || check_ovr("decompress"))
		return;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	// Make sure there is a selection
	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("No Selection", "There is no selection to decompress");
		theApp.mac_error_ = 10;
		return;
	}

	// Initialise structure used to communicate with the zlib routines
	z_stream zs;
	zs.zalloc = (alloc_func)0;
	zs.zfree = (free_func)0;
	zs.opaque = (voidpf)0;
	zs.next_in = (Bytef*)0;
	zs.avail_in = 0;
//	VERIFY(inflateInit(&zs) == Z_OK);
	int windowBits = 15;
	if (theApp.GetProfileInt("Options", "ZlibCompressionDVWindow", 1) == 0)
		windowBits = theApp.GetProfileInt("Options", "ZlibCompressionWindow", windowBits);
	ASSERT(windowBits >= 8 && windowBits <= 15);
	switch (theApp.GetProfileInt("Options", "ZlibCompressionHeaderType", 1))
	{
	case 0: // raw (no header)
		windowBits = - windowBits;  // assume no header
		break;
	default:
		ASSERT(0);
	case 1:
	case 2:
		windowBits += 32;  // auto detect header type
		break;
	}
	VERIFY(inflateInit2(&zs,
						windowBits
					   ) == Z_OK);

	// Test if selection is too big to do in memory (Note that even a small selection can expand greatly)
	if (end_addr - start_addr > (2*1024*1024) && GetDocument()->DataFileSlotFree())
	{
		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)

		// Create a "temp" file to store the compressed data
		// (This file stores the compressed data until the document is closed or written to disk.)
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		// Get input and output buffers
		size_t len, outbuflen = size_t(min(32768, end_addr - start_addr)), inbuflen = max(outbuflen/4, 256);
		FILE_ADDRESS total_out = 0;
		int err = Z_OK;
		try
		{
			in_data = new unsigned char[inbuflen];
			out_data = new unsigned char[outbuflen];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}

		try
		{
			CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

			// For each chunk of the currently selected block
			for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
			{
				// Get the next buffer full from the document
				len = size_t(min(inbuflen, end_addr - curr));
				VERIFY(GetDocument()->GetData(in_data, len, curr) == len);

				zs.next_in = in_data;
				zs.avail_in = len;
				do
				{
					// Prepare output buffer
					zs.next_out = out_data;
					zs.avail_out = outbuflen;
					if (err == Z_DATA_ERROR)
					{
						// We must still be trying to sync from previously encountered data error
						err = inflateSync(&zs);
					}
					else
					{
						err = inflate(&zs, Z_NO_FLUSH);
						ASSERT(err == Z_OK || err == Z_STREAM_END || err == Z_DATA_ERROR || err == Z_BUF_ERROR);
						// Note Z_BUF_ERROR can occur when zs.avail_out == 0 (and zs.avail_in after previous call was zero).
						// This is not a fatal error and according to the docs we should continue looping while zs.avail_in == 0.

						if (err == Z_DATA_ERROR)
						{
							theApp.mac_error_ = 5;
							if (TaskMessageBox("Bad Compressed Data",
											  "The compression data is corrupted. "
											  "HexEdit Pro can save the data decompressed "
											  "so far and attempt to recover from "
											  "this error but some data will be lost.\n\n"
											  "Do you want to continue?", MB_YESNO, 0,
											  MAKEINTRESOURCE(IDI_CROSS)) != IDYES)
							{
								ff.Close();
								remove(temp_file);
								theApp.mac_error_ = 10;
								goto func_return;
							}
							// Continue reading the data checking for sync point (see inflateSync call above)
						}

						ff.Write(out_data, outbuflen - zs.avail_out);  // write all that we got
						total_out += outbuflen - zs.avail_out;  // we need to keep track of this ourselves since zs.total_out can overflow
					}
				} while (zs.avail_out == 0 || zs.avail_in != 0);  // if output buffer is full there may be more

				if (AbortKeyPress() &&
					TaskMessageBox("Abort decompression?", 
					    "You have interrupted the decompression.\n\n"
					    "Do you want to stop the process?",MB_YESNO) == IDYES)
				{
					ff.Close();
					remove(temp_file);
					theApp.mac_error_ = 10;
					goto func_return;
				}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			}

			ASSERT(err == Z_STREAM_END || err == Z_DATA_ERROR);
			ASSERT(ff.GetLength() == total_out);
			if (err == Z_DATA_ERROR)
			{
				AfxMessageBox("HexEdit Pro did not recover from \n"
								"the compression data error. ");
			}
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();

			remove(temp_file);
			theApp.mac_error_ = 10;
			goto func_return;
		}

		if (total_out == 0)
		{
			// This could happen if there were data errors
			remove(temp_file);
			theApp.mac_error_ = 5;
			goto func_return;
		}

		// Delete the input (compressed) block that is to be replaced
		// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
		GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, this);

		// Add the temp file to the document
		idx = GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);
		GetDocument()->Change(mod_insert_file, start_addr, total_out, NULL, idx, this, TRUE);

		// Select uncompressed block
		SetSel(addr2pos(start_addr), addr2pos(start_addr + total_out));

		// Inform the user about the amount of compression
		CString mess;
		mess.Format("Expanded by %d%%", int((double(total_out)/(end_addr - start_addr) - 1.0)*100.0 + 0.5));
		mm->StatusBarText(mess);
	}
	else
	{
		// Allocate memory block and decompress into it

		// Error if ran out of temp file handles and file is too big
		if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Decompression Error",
				"HexEdit Pro is out of temporary files and "
				"cannot decompress such a large selection.\n\n"
				"Please save the file to deallocate "
				"temporary file handles and try again.");
			theApp.mac_error_ = 10;
			return;
		}

		// Warn if we might cause memory exhaustion
		if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Decompression Warning",
				"HexEdit Pro is out of temporary file "
				"handles and decompressing such a large selection "
				"may cause memory exhaustion.  Please click "
				"\"No\" and save the file to free handles "
				"or click \"Yes\" to continue.\n\n"
				"Do you want to continue?",
				MB_YESNO) != IDYES)
			{
				theApp.mac_error_ = 5;
				return;
			}
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		try
		{
			// Get buffer for input data, get the data, and get buffer for output (decompressed) data
			in_data = new unsigned char[size_t(end_addr - start_addr)];
			size_t got = GetDocument()->GetData(in_data, size_t(end_addr - start_addr), start_addr);
			ASSERT(got == size_t(end_addr - start_addr));

			// Remove input (compressed) block from the document
			GetDocument()->Change(mod_delforw, start_addr, got, NULL, 0, this);

			size_t outbuflen = max(4*got, 256);
			out_data = new unsigned char[outbuflen];
			FILE_ADDRESS curr = start_addr;   // Current output address

			int err = Z_OK;    // return value from inflate or inflateSync

			zs.next_in = in_data;
			zs.avail_in = got;
			do
			{
				zs.next_out = out_data;
				zs.avail_out = outbuflen;
				if (err == Z_DATA_ERROR)
				{
					// We must still be trying to sync from previously encountered data error
					err = inflateSync(&zs);
				}
				if (err != Z_DATA_ERROR)
				{
					err = inflate(&zs, Z_NO_FLUSH);
					ASSERT(err == Z_OK || err == Z_STREAM_END || err == Z_BUF_ERROR || err == Z_DATA_ERROR);
					if (err == Z_DATA_ERROR)
					{
						if (TaskMessageBox("Bad Compressed Data",
										  "The compression data is corrupted. "
										  "HexEdit Pro can save the data decompressed "
										  "so far and attempt to recover from "
										  "this error but some data will be lost.\n\n"
										  "Do you want to continue?", MB_YESNO, 0,
										  MAKEINTRESOURCE(IDI_CROSS)) != IDYES)
						{
							theApp.mac_error_ = 10;

							// Undo changes already (but make sure we don't record undo in macros)
							OnEditUndo();
							if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_undo)
								theApp.mac_.pop_back();

							goto func_return;
						}
					}

					// Add the decompressed data block to the file
					if (outbuflen - zs.avail_out > 0)
						GetDocument()->Change(mod_insert, curr, outbuflen - zs.avail_out, out_data, 0, this, TRUE);

					curr += outbuflen - zs.avail_out;
				}
			} while (zs.avail_out == 0 || zs.avail_in != 0);
			ASSERT(err == Z_STREAM_END);

			// Select uncompressed block
			SetSel(addr2pos(start_addr), addr2pos(start_addr+zs.total_out));

			// Inform the user about the amount of compression
			CString mess;
			mess.Format("Expanded by %d%%", int((double(zs.total_out)/got - 1.0)*100.0 + 0.5));
			mm->StatusBarText(mess);
		}
		catch (std::bad_alloc)
		{
			// Note this exception may come from ZLIB
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;

			// Undo changes already made (but make sure we don't record undo in macros)
			OnEditUndo();
			if (theApp.recording_ && theApp.mac_.size() > 0 && (theApp.mac_.back()).ktype == km_undo)
				theApp.mac_.pop_back();

			goto func_return;
		}
	}
	VERIFY(inflateEnd(&zs) == Z_OK);

	DisplayCaret();
	theApp.SaveToMacro(km_compress, 2);  // 2 = decompress (1 = compress)

func_return:
	mm->Progress(-1);  // disable progress bar

	if (in_data != NULL)
		delete[] in_data;
	if (out_data != NULL)
		delete[] out_data;
}

// Enable commands that depend simply on a non-zero selection
void CHexEditView::OnUpdateSelNZ(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	pCmdUI->Enable(start_addr < end_addr);
}

// The following functions are used as update handlers
// They enable their corresponding UI handlers if there are
// enough bytes between the current address and the end of file
void CHexEditView::OnUpdateByte(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetPos() < GetDocument()->length());
}

void CHexEditView::OnUpdate16bit(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		// No selection so only enable if enough before EOF (2 bytes)
		pCmdUI->Enable(start_addr + 1 < GetDocument()->length());
	else
		// Else make sure selection is multiple of 2
		pCmdUI->Enable((end_addr - start_addr)%2 == 0);
}

void CHexEditView::OnUpdate32bit(CCmdUI* pCmdUI)
{
//    pCmdUI->Enable(GetPos() + 3 < GetDocument()->length());
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		// No selection so only enable if enough before EOF (4 bytes)
		pCmdUI->Enable(start_addr + 3 < GetDocument()->length());
	else
		// Else make sure selection is multiple of 4
		pCmdUI->Enable((end_addr - start_addr)%4 == 0);
}

void CHexEditView::OnUpdate64bit(CCmdUI* pCmdUI)
{
//    pCmdUI->Enable(GetPos() + 7 < GetDocument()->length());
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);
	if (start_addr == end_addr)
		// No selection so only enable if enough before EOF (8 bytes)
		pCmdUI->Enable(start_addr + 7 < GetDocument()->length());
	else
		// Else make sure selection is multiple of 8
		pCmdUI->Enable((end_addr - start_addr)%8 == 0);
}

// These update handlers are similar to the above but for binary operations
// As binary operations use the current value from the calculator we also need
// to ensure that the current operand size in the calculator is not too big.
void CHexEditView::OnUpdateByteBinary(CCmdUI* pCmdUI)
{
	if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.ByteSize() > 1)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	OnUpdateByte(pCmdUI);
}

void CHexEditView::OnUpdate16bitBinary(CCmdUI* pCmdUI)
{
	if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.ByteSize() > 2)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	OnUpdate16bit(pCmdUI);
}

void CHexEditView::OnUpdate32bitBinary(CCmdUI* pCmdUI)
{
	if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.ByteSize() > 4)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	OnUpdate32bit(pCmdUI);
}

void CHexEditView::OnUpdate64bitBinary(CCmdUI* pCmdUI)
{
	if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.ByteSize() > 8)
	{
		ASSERT(0);                     // Max size is 8 at present
		pCmdUI->Enable(FALSE);
		return;
	}
	OnUpdate64bit(pCmdUI);
}

// Note: DoChecksum is not a member template (not supported in VC++ when written) so a ptr to the view is passed in pv
template<class T> void DoChecksum(CHexEditView *pv, checksum_type op, LPCSTR desc)
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	pv->GetSelAddr(start_addr, end_addr);

	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("There is nothing selected to calculate %s on", desc);
		TaskMessageBox("No Selection", mess);
		theApp.mac_error_ = 10;
		return;
	}
	if (op >= CHECKSUM_8 && op < 10 && (end_addr - start_addr)%sizeof(T) != 0)
	{
		// Selection is wrong length, presumably in macro playback
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("The selection must be a multiple of %d for %s", sizeof(T), desc);
		CAvoidableDialog::Show(IDS_SEL_LEN, mess);
		((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < pv->GetDocument()->length());

	// Get a buffer - fairly large for efficiency
	size_t len, buflen = size_t(min(4096, end_addr - start_addr));
	try
	{
		buf = new unsigned char[buflen];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(buf != NULL);

	T val;
	void * hh;
	switch (op)
	{
	case CHECKSUM_8:
	case CHECKSUM_16:
	case CHECKSUM_32:
	case CHECKSUM_64:
		val = 0;
		break;

	case CHECKSUM_CRC16:
		hh = crc_16_init();
		break;
	case CHECKSUM_CRC_CCITT_F:
		hh = crc_ccitt_f_init();
		break;
	case CHECKSUM_CRC_CCITT_AUG:
		hh = crc_ccitt_aug_init();
		break;
	case CHECKSUM_CRC_CCITT_T:
		hh = crc_ccitt_t_init();
		break;
	case CHECKSUM_CRC_XMODEM:
		hh = crc_xmodem_init();
		break;
	case CHECKSUM_CRC32:
		hh = crc_32_init();
		break;
	case CHECKSUM_CRC32_MPEG2:
		hh = crc_32_mpeg2_init();
		break;

	case CHECKSUM_CRC_4BIT:
		hh = crc_4bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_8BIT:
		hh = crc_8bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_10BIT:
		hh = crc_10bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_12BIT:
		hh = crc_12bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_16BIT:
		hh = crc_16bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_32BIT:
		hh = crc_32bit_init(&pv->crc_params_);
		break;
	case CHECKSUM_CRC_64BIT:
		hh = crc_64bit_init(&pv->crc_params_);
		break;

	default:
		ASSERT(0);
	}
	for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
	{
		// Get the next buffer full from the document
		len = size_t(min(buflen, end_addr - curr));
		VERIFY(pv->GetDocument()->GetData(buf, len, curr) == len);

		switch (op)
		{
		case CHECKSUM_8:
		case CHECKSUM_16:
		case CHECKSUM_32:
		case CHECKSUM_64:
			ASSERT(len % sizeof(T) == 0);
			{
				T *pp, *endp = (T *)(buf + len);
				for (pp = (T *)buf; pp < endp; ++pp)
					val += *pp;
			}
			break;

		case CHECKSUM_CRC16:
			crc_16_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_CCITT_F:
			crc_ccitt_f_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_CCITT_AUG:
			crc_ccitt_aug_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_CCITT_T:
			crc_ccitt_t_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_XMODEM:
			crc_xmodem_update(hh, buf, len);
			break;
		case CHECKSUM_CRC32:
			crc_32_update(hh, buf, len);
			break;
		case CHECKSUM_CRC32_MPEG2:
			crc_32_mpeg2_update(hh, buf, len);
			break;

		case CHECKSUM_CRC_4BIT:
			crc_4bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_8BIT:
			crc_8bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_10BIT:
			crc_10bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_12BIT:
			crc_12bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_16BIT:
			crc_16bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_32BIT:
			crc_32bit_update(hh, buf, len);
			break;
		case CHECKSUM_CRC_64BIT:
			crc_64bit_update(hh, buf, len);
			break;
		}

		if (AbortKeyPress() &&
			TaskMessageBox("Abort calculation?", 
			    "You have interrupted the calculation.\n\n"
			    "Do you want to stop the process?",MB_YESNO) == IDYES)
		{
			theApp.mac_error_ = 10;
			goto func_return;
		}

		mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
	}
	switch (op)
	{
	case CHECKSUM_CRC16:
		val = T(crc_16_final(hh));
		break;
	case CHECKSUM_CRC_CCITT_F:
		val = T(crc_ccitt_f_final(hh));
		break;
	case CHECKSUM_CRC_CCITT_AUG:
		val = T(crc_ccitt_aug_final(hh));
		break;
	case CHECKSUM_CRC_CCITT_T:
		val = T(crc_ccitt_t_final(hh));
		break;
	case CHECKSUM_CRC_XMODEM:
		val = T(crc_xmodem_final(hh));
		break;
	case CHECKSUM_CRC32:
		val = T(crc_32_final(hh));
		break;
	case CHECKSUM_CRC32_MPEG2:
		val = T(crc_32_mpeg2_final(hh));
		break;

	case CHECKSUM_CRC_4BIT:
		val = T(crc_4bit_final(hh));
		break;
	case CHECKSUM_CRC_8BIT:
		val = T(crc_8bit_final(hh));
		break;
	case CHECKSUM_CRC_10BIT:
		val = T(crc_10bit_final(hh));
		break;
	case CHECKSUM_CRC_12BIT:
		val = T(crc_12bit_final(hh));
		break;
	case CHECKSUM_CRC_16BIT:
		val = T(crc_16bit_final(hh));
		break;
	case CHECKSUM_CRC_32BIT:
		val = T(crc_32bit_final(hh));
		break;
	case CHECKSUM_CRC_64BIT:
		val = T(crc_64bit_final(hh));
		break;
	}

	// Get final CRC and store it in the calculator
	if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.get_bits() < sizeof(T)*8)
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_bits(sizeof(T)*8);
	((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_signed(false);  // avoid overflow if top bit is on
	//((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_base(16);
	((CMainFrame *)AfxGetMainWnd())->m_wndCalc.Set(val);

	dynamic_cast<CMainFrame *>(::AfxGetMainWnd())->show_calc();          // make sure calc is displayed

	theApp.SaveToMacro(km_checksum, op);

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnChecksum8()
{
	DoChecksum<unsigned char>(this, CHECKSUM_8, "8 bit Checksum");
}

void CHexEditView::OnChecksum16()
{
	DoChecksum<unsigned short>(this, CHECKSUM_16, "16 bit Checksum");
}

void CHexEditView::OnChecksum32()
{
	DoChecksum<unsigned long>(this, CHECKSUM_32, "32 bit Checksum");
}

void CHexEditView::OnChecksum64()
{
	DoChecksum<unsigned __int64>(this, CHECKSUM_64, "64 bit Checksum");
}

void CHexEditView::OnCrc16()
{
	DoChecksum<unsigned short>(this, CHECKSUM_CRC16, "CRC 16");
}

void CHexEditView::OnCrcCcittF()
{
	DoChecksum<unsigned short>(this, CHECKSUM_CRC_CCITT_F, "CRC CCITT F");
}

void CHexEditView::OnCrcCcittAug()
{
	DoChecksum<unsigned short>(this, CHECKSUM_CRC_CCITT_AUG, "CRC CCITT AUG");
}

void CHexEditView::OnCrcCcittT()
{
	DoChecksum<unsigned short>(this, CHECKSUM_CRC_CCITT_T, "CRC CCITT");
}

void CHexEditView::OnCrcXmodem()
{
	DoChecksum<unsigned short>(this, CHECKSUM_CRC_XMODEM, "CRC XMODEM");
}

void CHexEditView::OnCrc32()
{
	DoChecksum<DWORD>(this, CHECKSUM_CRC32, "CRC 32");
}

void CHexEditView::OnCrc32Mpeg2()
{
	DoChecksum<DWORD>(this, CHECKSUM_CRC32_MPEG2, "CRC 32 MPEG-2");
}

void CHexEditView::OnCrcGeneral()
{
	CGeneralCRC dlg;
	if (dlg.DoModal() == IDOK)
	{
		CString params = dlg.SaveParams();
		::load_crc_params(&crc_params_, params);
		switch (crc_params_.bits)
		{
		case 4:
			DoChecksum<char>(this, CHECKSUM_CRC_4BIT, "CRC (4 BIT)");
			break;
		case 8:
			DoChecksum<unsigned char>(this, CHECKSUM_CRC_8BIT, "CRC (8 BIT)");
			break;
		case 10:
			DoChecksum<unsigned short>(this, CHECKSUM_CRC_10BIT, "CRC (10 BIT)");
			break;
		case 12:
			DoChecksum<unsigned short>(this, CHECKSUM_CRC_12BIT, "CRC (12 BIT)");
			break;
		case 16:
			DoChecksum<unsigned short>(this, CHECKSUM_CRC_16BIT, "CRC (16 BIT)");
			break;
		case 32:
			DoChecksum<unsigned int>(this, CHECKSUM_CRC_32BIT, "CRC (32 BIT)");
			break;
		case 64:
			DoChecksum<unsigned int>(this, CHECKSUM_CRC_64BIT, "CRC (64 BIT)");
			break;
		default:
			assert(0); // The code should prevent this from happening
			TaskMessageBox("Invalid CRC Bits", "CRC calculations do not support this number of bits");
			break;
		}
	}
}

#if 0  // Replace old MD5 with Boost version
void CHexEditView::OnMd5()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("No Selection", "There is no selection to calculate MD5 on.");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < GetDocument()->length());

	// Get a buffer - fairly large for efficiency
	size_t len, buflen = size_t(min(4096, end_addr - start_addr));
	try
	{
		buf = new unsigned char[buflen];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(buf != NULL);

	struct MD5Context ctx;
	MD5Init(&ctx);
	for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
	{
		// Get the next buffer full from the document
		len = size_t(min(buflen, end_addr - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);

		MD5Update(&ctx, buf, len);

		if (AbortKeyPress() &&
			TaskMessageBox("Abort MD5 calculation?", 
			    "You have interrupted the MD5 calculation.\n\n"
			    "Do you want to stop the process?",MB_YESNO) == IDYES)
		{
			theApp.mac_error_ = 10;
			goto func_return;
		}

		mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
	}

	unsigned char digest[16];
	MD5Final(digest, &ctx);

	// Display the value in (avoidable) message box and then load it into the calculator
	{
		// First get the result as text (hex digits)
		CString ss;
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		ss.Format(fmt,
				digest[0], digest[1], digest[2], digest[3],
				digest[4], digest[5], digest[6], digest[7],
				digest[8], digest[9], digest[10], digest[11],
				digest[12], digest[13], digest[14], digest[15]);

		// Load the value into the calculator ensuring bits is at least 160 and radix is hex
		if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.get_bits() < 128)
			((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_bits(128);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_signed(false);  // avoid overflow if top bit is on
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_base(16);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.SetStr(ss);
		dynamic_cast<CMainFrame *>(::AfxGetMainWnd())->show_calc();          // make sure calc is displayed

		// Display the result in an (avoidable) dialog
		CString mess;
		AddSpaces(ss);
		mess = "The calculator value has been set to the result of the MD5 calculation "
		       "which is 128 bits in length and displayed in hex (radix 16).\n\n" + 
		       ss;
		CAvoidableDialog::Show(IDS_MD5, mess);
	}

	// Record in macro since we did it successfully
	theApp.SaveToMacro(km_checksum, CHECKSUM_MD5);

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}
#else
void CHexEditView::OnMd5()
{
	typedef boost::hashes::md5 alg;
	DoDigest<alg>("MD5", CHECKSUM_MD5);
}
#endif

#if 0  // Don't use Boost SHA1 (yet) as it is 4-5 times slower than our old one
void CHexEditView::OnSha1()
{
	typedef boost::hashes::sha1 alg;
	DoDigest<alg>("SHA1", CHECKSUM_SHA1);
}
#else
void CHexEditView::OnSha1()
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("No Selection", "There is no selection to calculate SHA1 on");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < GetDocument()->length());

	// Get a buffer - fairly large for efficiency
	size_t len, buflen = size_t(min(4096, end_addr - start_addr));
	try
	{
		buf = new unsigned char[buflen];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(buf != NULL);

	sha1_context ctx;
	sha1_starts(&ctx);
	for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
	{
		// Get the next buffer full from the document
		len = size_t(min(buflen, end_addr - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);

		sha1_update(&ctx, buf, len);

		if (AbortKeyPress() &&
			TaskMessageBox("Abort SHA1 calculation?", 
			    "You have interrupted the SHA1 calculation.\n\n"
			    "Do you want to stop the process?",MB_YESNO) == IDYES)
		{
			theApp.mac_error_ = 10;
			goto func_return;
		}

		mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
	}

	unsigned char digest[20];
	sha1_finish(&ctx, digest);

	mm->Progress(-1);  // disable progress bar
	// Display the value in (avoidable) message box and then load it into the calculator
	{
		// First get the result as text (hex digits)
		CString ss;
		const char * fmt;
		if (theApp.hex_ucase_)
			fmt = "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X";
		else
			fmt = "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x";
		ss.Format(fmt,
				digest[0],  digest[1],  digest[2],  digest[3],
				digest[4],  digest[5],  digest[6],  digest[7],
				digest[8],  digest[9],  digest[10], digest[11],
				digest[12], digest[13], digest[14], digest[15],
				digest[16], digest[17], digest[18], digest[19]);

		// Load the value into the calculator ensuring bits is at least 160 and radix is hex
		if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.get_bits() < 160)
			((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_bits(160);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_signed(false);  // avoid overflow if top bit is on
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_base(16);

		//((CMainFrame *)AfxGetMainWnd())->m_wndCalc.SetStr(ss);
		mpz_class tmp;
		mpz_set_str(tmp.get_mpz_t(), ss, 16);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.Set(tmp);

		dynamic_cast<CMainFrame *>(::AfxGetMainWnd())->show_calc();          // make sure calc is displayed

		// Display the result in an (avoidable) dialog
		CString mess;
		AddSpaces(ss);
		mess = "The calculator value has been set to the result of the SHA1 calculation "
		       "which is 160 bits in length and displayed in hex (radix 16).\n\n" + 
		       ss;
		CAvoidableDialog::Show(IDS_SHA1, mess);
	}

	// Record in macro since we did it successfully
	theApp.SaveToMacro(km_checksum, CHECKSUM_SHA1);

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}
#endif

void CHexEditView::OnSha2_224()
{
	DoDigest<boost::hashes::sha2<224> >("SHA-224", CHECKSUM_SHA224);
}

void CHexEditView::OnSha2_256()
{
	DoDigest<boost::hashes::sha2<256> >("SHA-256", CHECKSUM_SHA256);
}

void CHexEditView::OnSha2_384()
{
	DoDigest<boost::hashes::sha2<384> >("SHA-384", CHECKSUM_SHA384);
}

void CHexEditView::OnSha2_512()
{
	DoDigest<boost::hashes::sha2<512> >("SHA-512", CHECKSUM_SHA512);
}

// This is a template member function for generating digest based on the proposed
// Boost "hash" library.  This is actually not (yet) part of Boost as at ver 1.47
// but I have installed the hash.hpp header (and the files int the hash sub-directory)
// into the Boost include directory (currently using Boost 1.47).  Note that the
// file, directory and namespace (hashes) may all change if/when it is added to
// Boost (possibly as "digest") to avoid confusion with functional/hash.
// Also note that I had to comment out the Adler and CRC includes in hash.hpp as
// this was causing build errors (and I don't need those).
// Anyway, I added this for the SHA2 digests (SHA-256 etc) but I have also changed
// the MD5 to use it.  (I have not changed SHA1 (yet) as the old code is faster.) 
template<class T> void CHexEditView::DoDigest(LPCSTR desc, int mac_id)
{
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	GetSelAddr(start_addr, end_addr);

	if (start_addr >= end_addr)
	{
		// No selection, presumably in macro playback
		ASSERT(theApp.playing_);
		TaskMessageBox("No Selection", "There is no selection to calculate " + CString(desc));
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < GetDocument()->length());

	// Get a buffer - fairly large for efficiency
	size_t len, buflen = size_t(min(4096, end_addr - start_addr));
	try
	{
		buf = new unsigned char[buflen];
	}
	catch (std::bad_alloc)
	{
		AfxMessageBox("Insufficient memory");
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(buf != NULL);

	T::stream_hash<8>::type ctx;
	T::digest_type digest;

	for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
	{
		// Get the next buffer full from the document
		len = size_t(min(buflen, end_addr - curr));
		VERIFY(GetDocument()->GetData(buf, len, curr) == len);

		ctx.update_n(buf, len);

		if (AbortKeyPress() &&
			TaskMessageBox("Abort " + CString(desc) + " calculation?", 
			    "You have interrupted the " + CString(desc) + " calculation.\n\n"
			    "Do you want to stop the process?",MB_YESNO) == IDYES)
		{
			theApp.mac_error_ = 10;
			mm->Progress(-1);  // disable progress bar
			goto func_return;
		}

		mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
	}
	digest = ctx.end_message();

	mm->Progress(-1);  // disable progress bar

	// Display the value in (avoidable) message box and then load it into the calculator
	{
		CString ss;
		const char * fmt = theApp.hex_ucase_ ? "%2.2X" : "%2.2x";
		for (int ii = 0; ii < T::digest_type::digest_bits/8; ++ii)
		{
			char buf[8];
			sprintf(buf, fmt, digest[ii]);
			ss += buf;
		}

		// Load the value into the calculator ensuring bits is at least 160 and radix is hex
		if (((CMainFrame *)AfxGetMainWnd())->m_wndCalc.get_bits() < T::digest_type::digest_bits)
			((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_bits(T::digest_type::digest_bits);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_signed(false);  // avoid overflow if top bit is on
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.change_base(16);       // Digests are traditionally shown in hex

		//((CMainFrame *)AfxGetMainWnd())->m_wndCalc.SetStr(ss);
		mpz_class tmp;
		mpz_set_str(tmp.get_mpz_t(), ss, 16);
		((CMainFrame *)AfxGetMainWnd())->m_wndCalc.Set(tmp);

		dynamic_cast<CMainFrame *>(::AfxGetMainWnd())->show_calc();          // make sure calc is displayed

		// Display the result in an (avoidable) dialog
		AddSpaces(ss);
		CString mess;
		mess.Format("%s\n\n"
		            "The calculator value has been set to the result of the %s calculation "
		            "which is %d bits in length and displayed in hex (radix 16).\n\n"
		            "%s", desc, desc, T::digest_type::digest_bits, ss);
		CAvoidableDialog::Show(IDS_DIGEST, mess);
	}

	// Record in macro since we did it successfully
	theApp.SaveToMacro(km_checksum, mac_id);

func_return:
	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnUpdateByteNZ(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start_addr < end_addr ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		// ??? is start_addr < end_addr then pos must be less than EOF ???
		pCmdUI->Enable(start_addr < end_addr && GetPos() < GetDocument()->length());
}

void CHexEditView::OnUpdate16bitNZ(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start_addr < end_addr ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(end_addr > start_addr && (end_addr - start_addr)%2 == 0);
}

void CHexEditView::OnUpdate32bitNZ(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start_addr < end_addr ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(end_addr > start_addr && (end_addr - start_addr)%4 == 0);
}

void CHexEditView::OnUpdate64bitNZ(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start_addr, end_addr;              // Current selection
	GetSelAddr(start_addr, end_addr);

	if (pCmdUI->m_pSubMenu != NULL)
	{
		// This happens when popup menu itself is drawn
		pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex,
			MF_BYPOSITION | (start_addr < end_addr ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}
	else
		pCmdUI->Enable(end_addr > start_addr && (end_addr - start_addr)%8 == 0);
}

template<class T> void ProcBinary(CHexEditView *pv, T val, T *buf, size_t count, binop_type op, int &div0)
{
	for (size_t ii = 0; ii < count; ++ii)
	{
		// Reverse byte order if using big-endian
		if (pv->BigEndian())
			::flip_bytes((unsigned char *)&buf[ii], sizeof(T));
		switch (op)
		{
		case binop_add:
			buf[ii] += val;
			break;
		case binop_subtract:
			buf[ii] -= val;
			break;
		case binop_subtract_x:
			buf[ii] = val - buf[ii];
			break;
		case binop_multiply:
			buf[ii] *= val;
			break;
		case binop_divide:
			ASSERT(val != 0);
			buf[ii] /= val;
			break;
		case binop_divide_x:
			if (buf[ii] != 0)
				buf[ii] = val / buf[ii];
			else
				++div0;
			break;
		case binop_mod:
			ASSERT(val != 0);
			buf[ii] %= val;
			break;
		case binop_mod_x:
			if (buf[ii] != 0)
				buf[ii] = val % buf[ii];
			else
				++div0;
			break;
		case binop_and:
			buf[ii] &= val;
			break;
		case binop_or:
			buf[ii] |= val;
			break;
		case binop_xor:
			buf[ii] ^= val;
			break;

		// Signed/unsigned comparisons are differentiated by type of T passed to the template function
		case binop_gtr:
		case binop_gtr_unsigned:
			if (val > buf[ii])
				buf[ii] = val;
			break;
		case binop_less:
		case binop_less_unsigned:
			if (val < buf[ii])
				buf[ii] = val;
			break;

		case binop_rol:  // T must be unsigned type for zero fill in >>
			{
				int tmp = int(val) % (sizeof(T)*8);
				buf[ii] = (buf[ii] << tmp) | (buf[ii] >> (sizeof(T)*8 - tmp));
			}
			break;
		case binop_ror:  // T must be unsigned type for zero fill in >>
			{
				int tmp = int(val) % (sizeof(T)*8);
				buf[ii] = (buf[ii] >> tmp) | (buf[ii] << (sizeof(T)*8 - tmp));
			}
			break;
		case binop_lsl:
			buf[ii] <<= int(val);
			break;

		case binop_lsr:  // T must be unsigned type for zero fill
		case binop_asr:  // T must be signed type for sign to be extended
			buf[ii] >>= int(val);
			break;
		default:
			ASSERT(0);
		}
		// Reverse byte order back again before putting back to the file
		if (pv->BigEndian())
			::flip_bytes((unsigned char *)&buf[ii], sizeof(T));
	}
}

// Perform binary operations (where the 2nd operand is taken from the calculator).
// [This is used to replace numerous other functions (OnAddByte, OnXor64Bit etc).]
// T = template parameter specifying size of operand (1,2,4 or 8 byte integer)
// pv = pointer to the view (we had to do this as VC6 does not support member templates)
// op = operation to perform
// desc = describes the operations and operand size for use in error messages
// dummy = determines template operand type (should not be nec. except for VC6 template bugs)
template<class T> void OnOperateBinary(CHexEditView *pv, binop_type op, LPCSTR desc, T dummy)
{
	(void)dummy;  // The dummy param. makes sure we get the right template (VC++ 6 bug)

	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	ASSERT(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
	if (pv->check_ro(desc))
		return;

	if (mm->m_wndCalc.ByteSize() > sizeof(T))
	{
		ASSERT(theApp.playing_);  // This should only happen during playback since commands are disabled otherwise
		CString mess;
		mess.Format("You can't %s unless second operand is %d bytes or less.  "
			"Please adjust the \"Bits\" setting in the calculator.", desc, sizeof(T));
		TaskMessageBox("Operand Too Big", mess);
		theApp.mac_error_ = 10;
		return;
	}

	// Get current address of selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	pv->GetSelAddr(start_addr, end_addr);

	// If no selection just do one
	if (start_addr >= end_addr)
	{
		end_addr = start_addr + sizeof(T);
		if (end_addr > pv->GetDocument()->length())
		{
			// Not enough bytes before EOF, presumably in macro playback
			ASSERT(theApp.playing_);
			CString mess;
			mess.Format("Too Few Bytes for %s", desc);
			TaskMessageBox(mess,  "There are not enough bytes before end of file to perform this "
				"operation.  Please add some dummy bytes at the end of file if you want to extend the file.");
			theApp.mac_error_ = 10;
			return;
		}
	}

	// Make sure selection is a multiple of the data type length
	if ((end_addr - start_addr) % sizeof(T) != 0)
	{
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("The selection must be a multiple of %d bytes for %s", sizeof(T), desc);
		CAvoidableDialog::Show(IDS_SEL_LEN, mess);
		((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < end_addr && end_addr <= pv->GetDocument()->length());

	// Get the other operand from the calculator
	T val = T(mm->m_wndCalc.GetValue());

	// Check for calculator val of zero causing divide by zero
	if (val == 0 && (op == binop_divide || op == binop_mod))
	{
		CString mess;
		mess.Format("The calculator value is zero (causing divide by 0) while performing %s.", desc);
		TaskMessageBox("Divide by Zero", mess);
		theApp.mac_error_ = 5;
		return;
	}

	int div0 = 0;   // Count of divide by zero errors (for binop_divide_x and binop_mod_x)

	// Test if selection is too big to do in memory
	if (end_addr - start_addr > (16*1024*1024) && pv->GetDocument()->DataFileSlotFree())
	{
		int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)

		// Create a file to store the resultant data
		// (Temp file used by the document until it is closed or written to disk.)
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		// Get data buffer
		size_t len, buflen = size_t(min(4096, end_addr - start_addr));
		try
		{
			buf = new unsigned char[buflen];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}

		try
		{
			CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

			for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
			{
				// Get the next buffer full from the document
				len = size_t(min(buflen, end_addr - curr));
				VERIFY(pv->GetDocument()->GetData(buf, len, curr) == len);

				ProcBinary(pv, val, (T *)buf, len/sizeof(T), op, div0);

				ff.Write(buf, len);

				if (AbortKeyPress())
				{
					CString mess;
					mess.Format("Abort %s operation?", desc);
					if (TaskMessageBox(mess, 
						"You have interrupted the calculation.\n\n"
						"Do you want to stop the process?",MB_YESNO) == IDYES)
					{
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}
				}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			}
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();

			remove(temp_file);
			theApp.mac_error_ = 10;
			goto func_return;
		}

		// Delete the unprocessed block that is to be replaced
		// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
		pv->GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, pv);

		// Add the temp file to the document
		idx = pv->GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);
		pv->GetDocument()->Change(mod_insert_file, start_addr, end_addr - start_addr, NULL, idx, pv, TRUE);
	}
	else
	{
		// Allocate memory block and process it

		// Error if ran out of temp file handles and file is too big
		if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Large Selection Error",
				"HexEdit Pro is out of temporary files and "
				"cannot operate on such a large selection.\n\n"
				"Please save the file to deallocate "
				"temporary file handles and try again.");
			theApp.mac_error_ = 10;
			return;
		}

		// Warn if we might cause memory exhaustion
		if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Warning",
				"HexEdit Pro is out of temporary file "
				"handles and operating on such a large selection "
				"may cause memory exhaustion.  Please click "
				"\"No\" and save the file to free handles "
				"or click \"Yes\" to continue.\n\n"
				"Do you want to continue?",
				MB_YESNO) != IDYES)
			{
				theApp.mac_error_ = 5;
				return;
			}
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		try
		{
			buf = new unsigned char[size_t(end_addr - start_addr)];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}
		size_t got = pv->GetDocument()->GetData(buf, size_t(end_addr - start_addr), start_addr);
		ASSERT(got == size_t(end_addr - start_addr));

		ProcBinary(pv, val, (T *)buf, got/sizeof(T), op, div0);

		pv->GetDocument()->Change(mod_replace, start_addr, got, (unsigned char *)buf, 0, pv);
	}

	if (div0 > 0)
	{
		// Tell the user about divide by zero errors
		CString mess;
		mess.Format("There were %d divide by zero errors while performing %s.  "
			        "The values at those locations were not changed.", int(div0), desc);
		TaskMessageBox("Divide by Zero", mess);
		theApp.mac_error_ = 5;
	}

	pv->DisplayCaret();

	{
		static int bit_pos[8] = {0, 1, -1, 2, -1, -1, -1, 3};
		ASSERT(sizeof(T)-1 < 8 && bit_pos[sizeof(T)-1] != -1);  // size must be 1,2,4,8 giving bit_pos of 0,1,2,3
		theApp.SaveToMacro(km_op_binary, (op<<8)|bit_pos[sizeof(T)-1]);
	}

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnAddByte()
{
	::OnOperateBinary<char>(this, binop_add, "Add bytes", char(0));
}

void CHexEditView::OnAdd16bit()
{
	::OnOperateBinary<short>(this, binop_add, "Add words", short(0));
}

void CHexEditView::OnAdd32bit()
{
	::OnOperateBinary<long>(this, binop_add, "Add double words", long(0));
}

void CHexEditView::OnAdd64bit()
{
	::OnOperateBinary<__int64>(this, binop_add, "Add quad words", __int64(0));
}

void CHexEditView::OnSubtractByte()
{
	::OnOperateBinary<char>(this, binop_subtract, "Subtract bytes", char(0));
}

void CHexEditView::OnSubtract16bit()
{
	::OnOperateBinary<short>(this, binop_subtract, "Subtract words", short(0));
}

void CHexEditView::OnSubtract32bit()
{
	::OnOperateBinary<long>(this, binop_subtract, "Subtract double words", long(0));
}

void CHexEditView::OnSubtract64bit()
{
	::OnOperateBinary<__int64>(this, binop_subtract, "Subtract quad words", __int64(0));
}

void CHexEditView::OnAndByte()
{
	::OnOperateBinary<char>(this, binop_and, "AND bytes", char(0));
}

void CHexEditView::OnAnd16bit()
{
	::OnOperateBinary<short>(this, binop_and, "AND words", short(0));
}

void CHexEditView::OnAnd32bit()
{
	::OnOperateBinary<long>(this, binop_and, "AND double words", long(0));
}

void CHexEditView::OnAnd64bit()
{
	::OnOperateBinary<__int64>(this, binop_and, "AND quad words", __int64(0));
}

void CHexEditView::OnOrByte()
{
	::OnOperateBinary<char>(this, binop_or, "OR bytes", char(0));
}

void CHexEditView::OnOr16bit()
{
	::OnOperateBinary<short>(this, binop_or, "OR words", short(0));
}

void CHexEditView::OnOr32bit()
{
	::OnOperateBinary<long>(this, binop_or, "OR double words", long(0));
}

void CHexEditView::OnOr64bit()
{
	::OnOperateBinary<__int64>(this, binop_or, "OR quad words", __int64(0));
}

void CHexEditView::OnXorByte()
{
	::OnOperateBinary<char>(this, binop_xor, "XOR bytes", char(0));
}

void CHexEditView::OnXor16bit()
{
	::OnOperateBinary<short>(this, binop_xor, "XOR words", short(0));
}

void CHexEditView::OnXor32bit()
{
	::OnOperateBinary<long>(this, binop_xor, "XOR double words", long(0));
}

void CHexEditView::OnXor64bit()
{
	::OnOperateBinary<__int64>(this, binop_xor, "XOR quad words", __int64(0));
}

void CHexEditView::OnMulByte()
{
	::OnOperateBinary<unsigned char>(this, binop_multiply, "multiply bytes", unsigned char(0));
}

void CHexEditView::OnMul16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_multiply, "multiply words", unsigned short(0));
}

void CHexEditView::OnMul32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_multiply, "multiply double words", unsigned long(0));
}

void CHexEditView::OnMul64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_multiply, "multiply quad words", unsigned __int64(0));
}

void CHexEditView::OnDivByte()
{
	::OnOperateBinary<unsigned char>(this, binop_divide, "divide bytes", unsigned char(0));
}

void CHexEditView::OnDiv16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_divide, "divide words", unsigned short(0));
}

void CHexEditView::OnDiv32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_divide, "divide double words", unsigned long(0));
}

void CHexEditView::OnDiv64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_divide, "divide quad words", unsigned __int64(0));
}

void CHexEditView::OnModByte()
{
	::OnOperateBinary<unsigned char>(this, binop_mod, "modulus bytes", unsigned char(0));
}

void CHexEditView::OnMod16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_mod, "modulus words", unsigned short(0));
}

void CHexEditView::OnMod32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_mod, "modulus double words", unsigned long(0));
}

void CHexEditView::OnMod64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_mod, "modulus quad words", unsigned __int64(0));
}

void CHexEditView::OnSubtractXByte()
{
	::OnOperateBinary<char>(this, binop_subtract_x, "Subtract bytes", char(0));
}

void CHexEditView::OnSubtractX16bit()
{
	::OnOperateBinary<short>(this, binop_subtract_x, "Subtract words", short(0));
}

void CHexEditView::OnSubtractX32bit()
{
	::OnOperateBinary<long>(this, binop_subtract_x, "Subtract double words", long(0));
}

void CHexEditView::OnSubtractX64bit()
{
	::OnOperateBinary<__int64>(this, binop_subtract_x, "Subtract quad words", __int64(0));
}

void CHexEditView::OnDivXByte()
{
	::OnOperateBinary<unsigned char>(this, binop_divide_x, "divide bytes", unsigned char(0));
}

void CHexEditView::OnDivX16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_divide_x, "divide words", unsigned short(0));
}

void CHexEditView::OnDivX32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_divide_x, "divide double words", unsigned long(0));
}

void CHexEditView::OnDivX64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_divide_x, "divide quad words", unsigned __int64(0));
}

void CHexEditView::OnModXByte()
{
	::OnOperateBinary<unsigned char>(this, binop_mod_x, "modulus bytes", unsigned char(0));
}

void CHexEditView::OnModX16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_mod_x, "modulus words", unsigned short(0));
}

void CHexEditView::OnModX32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_mod_x, "modulus double words", unsigned long(0));
}

void CHexEditView::OnModX64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_mod_x, "modulus quad words", unsigned __int64(0));
}

void CHexEditView::OnGtrByte()
{
	::OnOperateBinary<signed char>(this, binop_gtr, "finding greater of bytes", signed char(0));
}

void CHexEditView::OnGtr16bit()
{
	::OnOperateBinary<short>(this, binop_gtr, "finding greater of words", short(0));
}

void CHexEditView::OnGtr32bit()
{
	::OnOperateBinary<long>(this, binop_gtr, "finding greater of double words", long(0));
}

void CHexEditView::OnGtr64bit()
{
	::OnOperateBinary<__int64>(this, binop_gtr, "finding greater of quad words", __int64(0));
}

void CHexEditView::OnLessByte()
{
	::OnOperateBinary<signed char>(this, binop_less, "finding lesser of bytes", signed char(0));
}

void CHexEditView::OnLess16bit()
{
	::OnOperateBinary<short>(this, binop_less, "finding lesser of words", short(0));
}

void CHexEditView::OnLess32bit()
{
	::OnOperateBinary<long>(this, binop_less, "finding lesser of double words", long(0));
}

void CHexEditView::OnLess64bit()
{
	::OnOperateBinary<__int64>(this, binop_less, "finding lesser of quad words", __int64(0));
}

void CHexEditView::OnGtrUByte()
{
	::OnOperateBinary<unsigned char>(this, binop_gtr_unsigned, "finding unsigned greater of bytes", unsigned char(0));
}

void CHexEditView::OnGtrU16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_gtr_unsigned, "finding unsigned greater of words", unsigned short(0));
}

void CHexEditView::OnGtrU32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_gtr_unsigned, "finding unsigned greater of double words", unsigned long(0));
}

void CHexEditView::OnGtrU64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_gtr_unsigned, "finding unsigned greater of quad words", unsigned __int64(0));
}

void CHexEditView::OnLessUByte()
{
	::OnOperateBinary<unsigned char>(this, binop_less_unsigned, "finding unsigned lesser of bytes", unsigned char(0));
}

void CHexEditView::OnLessU16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_less_unsigned, "finding unsigned lesser of words", unsigned short(0));
}

void CHexEditView::OnLessU32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_less_unsigned, "finding unsigned lesser of double words", unsigned long(0));
}

void CHexEditView::OnLessU64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_less_unsigned, "finding unsigned lesser of quad words", unsigned __int64(0));
}

void CHexEditView::OnRolByte()
{
	::OnOperateBinary<unsigned char>(this, binop_rol, "rotate left bytes", unsigned char(0));
}

void CHexEditView::OnRol16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_rol, "rotate left words", unsigned short(0));
}

void CHexEditView::OnRol32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_rol, "rotate left double words", unsigned long(0));
}

void CHexEditView::OnRol64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_rol, "rotate left quad words", unsigned __int64(0));
}

void CHexEditView::OnRorByte()
{
	::OnOperateBinary<unsigned char>(this, binop_ror, "rotate right bytes", unsigned char(0));
}

void CHexEditView::OnRor16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_ror, "rotate right words", unsigned short(0));
}

void CHexEditView::OnRor32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_ror, "rotate right double words", unsigned long(0));
}

void CHexEditView::OnRor64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_ror, "rotate right quad words", unsigned __int64(0));
}

void CHexEditView::OnLslByte()
{
	::OnOperateBinary<char>(this, binop_lsl, "shift left bytes", char(0));
}

void CHexEditView::OnLsl16bit()
{
	::OnOperateBinary<short>(this, binop_lsl, "shift left words", short(0));
}

void CHexEditView::OnLsl32bit()
{
	::OnOperateBinary<long>(this, binop_lsl, "shift left double words", long(0));
}

void CHexEditView::OnLsl64bit()
{
	::OnOperateBinary<__int64>(this, binop_lsl, "shift left quad words", __int64(0));
}

void CHexEditView::OnLsrByte()
{
	::OnOperateBinary<unsigned char>(this, binop_lsr, "shift right bytes", unsigned char(0));
}

void CHexEditView::OnLsr16bit()
{
	::OnOperateBinary<unsigned short>(this, binop_lsr, "shift right words", unsigned short(0));
}

void CHexEditView::OnLsr32bit()
{
	::OnOperateBinary<unsigned long>(this, binop_lsr, "shift right double words", unsigned long(0));
}

void CHexEditView::OnLsr64bit()
{
	::OnOperateBinary<unsigned __int64>(this, binop_lsr, "shift right quad words", unsigned __int64(0));
}

void CHexEditView::OnAsrByte()
{
	::OnOperateBinary<signed char>(this, binop_asr, "arithmetic shift right bytes", signed char(0));
}

void CHexEditView::OnAsr16bit()
{
	::OnOperateBinary<short>(this, binop_asr, "arithmetic shift right words", short(0));
}

void CHexEditView::OnAsr32bit()
{
	::OnOperateBinary<long>(this, binop_asr, "arithmetic shift right double words", long(0));
}

void CHexEditView::OnAsr64bit()
{
	::OnOperateBinary<__int64>(this, binop_asr, "arithmetic shift right quad words", __int64(0));
}

template<class T> void ProcUnary(CHexEditView *pv, T val, T *buf, size_t count, unary_type op)
{
	// Operate on all the selected values
	for (size_t ii = 0; ii < count; ++ii)
	{
		// Reverse if big endian and the operation calls for it.
		if (pv->BigEndian() && op != unary_flip)
			::flip_bytes((unsigned char *)&buf[ii], sizeof(T));
		switch (op)
		{
		case unary_at:
			buf[ii] = val;
			break;
		case unary_sign:
			buf[ii] = -buf[ii];
			break;
		case unary_inc:
			++buf[ii];
			if (buf[ii] == 0)
			{
				((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: Increment wrapped around to zero");
				theApp.mac_error_ = 1;            // Signal warning on wrap
			}
			break;
		case unary_dec:
			buf[ii]--;
			if (buf[ii] == -1)
			{
				((CMainFrame *)AfxGetMainWnd())->StatusBarText("Warning: Decrement wrapped past zero");
				theApp.mac_error_ = 1;            // Signal warning on wrap
			}
			break;
		case unary_not:
			ASSERT(sizeof(T) == 1);  // Only need to perform NOT on bytes
			buf[ii] = ~buf[ii];
			break;
		case unary_flip:
			ASSERT(sizeof(T) > 1);   // No point in flipping a single byte
			// Do nothing here (let ::flip_bytes below handle it)
			break;
		case unary_rev:
			{
				T result = 0;
				T tmp = buf[ii];
				for (int jj = 0; jj < sizeof(T)*8; ++jj)
				{
					result <<= 1;                      // Make room for the next bit
					if (tmp & 0x1)
						result |= 0x1;
					tmp >>= 1;                         // Move bits down to test the next
				}
				buf[ii] = result;
			}
			break;
		case unary_rand:
			ASSERT(sizeof(T) == 1);  // Only really need random bytes
			buf[ii] = T(::rand_good()>>16);
			break;
		case unary_rand_fast:
			ASSERT(sizeof(T) == 1);  // Only really need random bytes
			buf[ii] = T(::rand()>>7);
			break;
		default:
			ASSERT(0);
		}
		if (pv->BigEndian() || op == unary_flip)
			::flip_bytes((unsigned char *)&buf[ii], sizeof(T));
	}
}

// Perform unary operations on the selection
// [This is used to replace numerous other functions (OnIncByte, OnFlip64Bit etc).]
// T = template parameter specifying size of operand (1,2,4 or 8 byte integer)
// op = operation to perform
// desc = describes the operations and operand size for use in error messages
template<class T> void OnOperateUnary(CHexEditView *pv, unary_type op, LPCSTR desc, T dummy)
{
	(void)dummy;  // The dummy param. makes sure we get the right template (VC++ 6 bug)
	CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
	unsigned char *buf = NULL;

	ASSERT(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
	if (pv->check_ro(desc))
		return;

	T val = 0;
	// For assign operation we get the value from calc
	if (op == unary_at)
	{
		val = T(mm->m_wndCalc.GetValue());  // prefetch the value

		// Make sure the operand is not too big
		if (mm->m_wndCalc.ByteSize() > sizeof(T))
		{
			ASSERT(theApp.playing_);  // This should only happen during playback since commands are disabled otherwise
			CString mess;
			mess.Format("You can't %s unless the operand is %d bytes or less.  "
				"Please adjust the \"Bits\" setting in the calculator.", desc, sizeof(T));
			TaskMessageBox("Operand Too Big", mess);
			theApp.mac_error_ = 10;
			return;
		}
	}

	// Get current address or selection
	FILE_ADDRESS start_addr, end_addr;          // Start and end of selection
	pv->GetSelAddr(start_addr, end_addr);

	// If no selection just do one
	if (start_addr >= end_addr)
	{
		end_addr = start_addr + sizeof(T);
		if (end_addr > pv->GetDocument()->length())
		{
			// Not enough bytes before EOF, presumably in macro playback
			ASSERT(theApp.playing_);
			CString mess;
			mess.Format("Too Few Bytes for %s", desc);
			TaskMessageBox(mess,  "There are not enough bytes before end of file to perform this "
				"operation.  Please add some dummy bytes at the end of file if you want to extend the file.");
			theApp.mac_error_ = 10;
			return;
		}
	}

	// Make sure selection is a multiple of the data type length
	if ((end_addr - start_addr) % sizeof(T) != 0)
	{
		ASSERT(theApp.playing_);
		CString mess;
		mess.Format("The selection must be a multiple of %d bytes for %s", sizeof(T), desc);
		CAvoidableDialog::Show(IDS_SEL_LEN, mess);
		((CMainFrame *)AfxGetMainWnd())->StatusBarText(mess);
		theApp.mac_error_ = 10;
		return;
	}
	ASSERT(start_addr < end_addr && end_addr <= pv->GetDocument()->length());

	// Test if selection is too big to do in memory
	if (end_addr - start_addr > (16*1024*1024) && pv->GetDocument()->DataFileSlotFree())
	{
		int idx = -1;                       // Index into docs data_file_ array (or -1 if no slots avail.)

		// Create a file to store the resultant data
		// (Temp file used by the document until it is closed or written to disk.)
		char temp_dir[_MAX_PATH];
		char temp_file[_MAX_PATH];
		::GetTempPath(sizeof(temp_dir), temp_dir);
		::GetTempFileName(temp_dir, _T("_HE"), 0, temp_file);

		// Get data buffer
		size_t len, buflen = size_t(min(4096, end_addr - start_addr));
		try
		{
			buf = new unsigned char[buflen];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}

		try
		{
			CFile64 ff(temp_file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary);

			for (FILE_ADDRESS curr = start_addr; curr < end_addr; curr += len)
			{
				// Get the next buffer full from the document
				len = size_t(min(buflen, end_addr - curr));
				if (op != unary_at)      // We don't need to get old value if assigning
					VERIFY(pv->GetDocument()->GetData(buf, len, curr) == len);

				ProcUnary(pv, val, (T *)buf, len/sizeof(T), op);

				ff.Write(buf, len);

				if (AbortKeyPress())
				{
					CString mess;
					mess.Format("Abort %s operation?", desc);
					if (TaskMessageBox(mess, 
						"You have interrupted the calculation.\n\n"
						"Do you want to stop the process?",MB_YESNO) == IDYES)
					{
						ff.Close();
						remove(temp_file);
						theApp.mac_error_ = 10;
						goto func_return;
					}
				}

				mm->Progress(int(((curr - start_addr)*100)/(end_addr - start_addr)));
			}
		}
		catch (CFileException *pfe)
		{
			TaskMessageBox("Temp File Error", ::FileErrorMessage(pfe, CFile::modeWrite));
			pfe->Delete();

			remove(temp_file);
			theApp.mac_error_ = 10;
			goto func_return;
		}

		// Delete the unprocessed block that is to be replaced
		// Note: this must be done before AddDataFile otherwise Change() (via regenerate()) will delete the temp file.
		pv->GetDocument()->Change(mod_delforw, start_addr, end_addr - start_addr, NULL, 0, pv);

		// Add the temp file to the document
		idx = pv->GetDocument()->AddDataFile(temp_file, TRUE);
		ASSERT(idx != -1);
		pv->GetDocument()->Change(mod_insert_file, start_addr, end_addr - start_addr, NULL, idx, pv, TRUE);
	}
	else
	{
		// Allocate memory block and process it

		// Error if ran out of temp file handles and file is too big
		if (end_addr - start_addr > UINT_MAX)  // why is there no SIZE_T_MAX?
		{
			TaskMessageBox("Large Selection Error",
				"HexEdit Pro is out of temporary files and "
				"cannot operate on such a large selection.\n\n"
				"Please save the file to deallocate "
				"temporary file handles and try again.");
			theApp.mac_error_ = 10;
			return;
		}

		// Warn if we might cause memory exhaustion
		if (end_addr - start_addr > 128*1024*1024)  // 128 Mb file may be too big
		{
			if (TaskMessageBox("Warning",
				"HexEdit Pro is out of temporary file "
				"handles and operating on such a large selection "
				"may cause memory exhaustion.  Please click "
				"\"No\" and save the file to free handles "
				"or click \"Yes\" to continue.\n\n"
				"Do you want to continue?",
				MB_YESNO) != IDYES)
			{
				theApp.mac_error_ = 5;
				return;
			}
		}

		CWaitCursor wait;                           // Turn on wait cursor (hourglass)

		try
		{
			buf = new unsigned char[size_t(end_addr - start_addr)];
		}
		catch (std::bad_alloc)
		{
			AfxMessageBox("Insufficient memory");
			theApp.mac_error_ = 10;
			goto func_return;
		}
		size_t got;
		if (op != unary_at)      // We don't need to get old value if assigning
		{
			got = pv->GetDocument()->GetData(buf, size_t(end_addr - start_addr), start_addr);
			ASSERT(got == size_t(end_addr - start_addr));
		}
		else
			got = size_t(end_addr - start_addr);

		ProcUnary(pv, val, (T *)buf, got/sizeof(T), op);

		pv->GetDocument()->Change(mod_replace, start_addr, got, (unsigned char *)buf, 0, pv);
	}

	pv->DisplayCaret();

	{
		static int bit_pos[8] = {0, 1, -1, 2, -1, -1, -1, 3};
		ASSERT(sizeof(T)-1 < 8 && bit_pos[sizeof(T)-1] != -1);  // size must be 1,2,4,8 giving bit_pos of 0,1,2,3
		theApp.SaveToMacro(km_op_unary, (op<<8)|bit_pos[sizeof(T)-1]);
	}

func_return:
	mm->Progress(-1);  // disable progress bar

	if (buf != NULL)
		delete[] buf;
}

void CHexEditView::OnIncByte()
{
	::OnOperateUnary<char>(this, unary_inc, "increment bytes", char(0));
}

void CHexEditView::OnInc16bit()
{
	::OnOperateUnary<short>(this, unary_inc, "increment words", short(0));
}

void CHexEditView::OnInc32bit()
{
	::OnOperateUnary<long>(this, unary_inc, "increment double words", long(0));
}

void CHexEditView::OnInc64bit()
{
	::OnOperateUnary<__int64>(this, unary_inc, "increment quad words", __int64(0));
}

void CHexEditView::OnDecByte()
{
	::OnOperateUnary<char>(this, unary_dec, "decrement bytes", char(0));
}

void CHexEditView::OnDec16bit()
{
	::OnOperateUnary<short>(this, unary_dec, "decrement words", short(0));
}

void CHexEditView::OnDec32bit()
{
	::OnOperateUnary<long>(this, unary_dec, "decrement double words", long(0));
}

void CHexEditView::OnDec64bit()
{
	::OnOperateUnary<__int64>(this, unary_dec, "decrement quad words", __int64(0));
}

void CHexEditView::OnRevByte()
{
	::OnOperateUnary<char>(this, unary_rev, "reverse bits of bytes", char(0));
}

void CHexEditView::OnRev16bit()
{
	::OnOperateUnary<short>(this, unary_rev, "reverse bits of words", short(0));
}

void CHexEditView::OnRev32bit()
{
	::OnOperateUnary<long>(this, unary_rev, "reverse bits of double words", long(0));
}

void CHexEditView::OnRev64bit()
{
	::OnOperateUnary<__int64>(this, unary_rev, "reverse bits of quad words", __int64(0));
}

// There is NO OnFlipByte (since it would not do anything)

void CHexEditView::OnFlip16bit()
{
	::OnOperateUnary<short>(this, unary_flip, "flip bytes of words", short(0));
}

void CHexEditView::OnFlip32bit()
{
	::OnOperateUnary<long>(this, unary_flip, "flip bytes of double words", long(0));
}

void CHexEditView::OnFlip64bit()
{
	::OnOperateUnary<__int64>(this, unary_flip, "flip bytes of quad words", __int64(0));
}

void CHexEditView::OnInvert()
{
	::OnOperateUnary<char>(this, unary_not, "invert bits", char(0));
}

void CHexEditView::OnNegByte()
{
	::OnOperateUnary<signed char>(this, unary_sign, "negate bytes", signed char(0));
}

void CHexEditView::OnNeg16bit()
{
	::OnOperateUnary<short>(this, unary_sign, "negate words", short(0));
}

void CHexEditView::OnNeg32bit()
{
	::OnOperateUnary<long>(this, unary_sign, "negate double words", long(0));
}

void CHexEditView::OnNeg64bit()
{
	::OnOperateUnary<__int64>(this, unary_sign, "negate quad words", __int64(0));
}

void CHexEditView::OnAssignByte()
{
	::OnOperateUnary<char>(this, unary_at, "assign bytes", char(0));
}

void CHexEditView::OnAssign16bit()
{
	::OnOperateUnary<short>(this, unary_at, "assign words", short(0));
}

void CHexEditView::OnAssign32bit()
{
	::OnOperateUnary<long>(this, unary_at, "assign double words", long(0));
}

void CHexEditView::OnAssign64bit()
{
	::OnOperateUnary<__int64>(this, unary_at, "assign quad words", __int64(0));
}

void CHexEditView::OnSeedCalc()
{
	unsigned long seed = unsigned long(((CMainFrame *)AfxGetMainWnd())->m_wndCalc.GetValue());
	srand(seed);                        // Seed compiler PRNG (simple one)
	rand_good_seed(seed);               // Seed our own PRNG
	theApp.SaveToMacro(km_seed, 1);
}

void CHexEditView::OnSeedRandom()
{
	unsigned long seed = ::GetTickCount();
	srand(seed);                        // Seed compiler PRNG (simple fast one)
	rand_good_seed(seed);               // Seed our own PRNG
	theApp.SaveToMacro(km_seed);
}

void CHexEditView::OnRandByte()
{
	::OnOperateUnary<char>(this, unary_rand, "randomize bytes", char(0));
}

void CHexEditView::OnRandFast()
{
	::OnOperateUnary<char>(this, unary_rand_fast, "fast randomize bytes", char(0));
}

// The display of views in tabs and split windows is complicated, mainly because the BCG tab view class
// behaves very differently from a splitter (eg is derived from CView).
// Currently we need to show 3 types of views: CHexEditView (normal hex view), CDataFormatView (template),
// and CAerialView (bitmap overview).  There is also a CHexTabView that just contains one or more other views.
// * The child frame has a splitter called "splitter_"
//   - there is always at least one pane so at least one view
//   - if there is only one pane then it contains the tab view
// * One of the splits is always a tab view called "ptv_" in the child frame
//   - if the window has not been split this is the only view
//   - this may be the 2nd split (since the first one can be template view)
// * The tab view always contains the hex view but may also have tabs for the other views
//   - the left most tab is always the hex view
//   - the tabs are not shown if there is just one view (the hex view)
// * The template and aerial views are either not shown, in a pane of the splitter or in a tab view
//
// In the code below we often store the index of a view in the splitter or in the tab view using this convention:
//  snum_t = index of tab view in the splitter (currently only 0 or 1)
//  snum_d = index of DFFD (template) view in the left splitter (currently only 0 or -1)
//  snum_a = index of aerial view in the right splitter (currently only 1, 2 or -1)
//  tnum_d = index of DFFD (template) view in the tab view (currently -1, 1 or 2)
//  tnum_a = index of aerial view in the tab view (currently -1, 1 or 2)
//  where -1 indicates the view is not present
//  Note tnum_h is not required as the hex view is always the left (0) view in the tab view

void CHexEditView::ShowDffd()
{
	if (pdfv_ == NULL)
		OnDffdSplit();
}

void CHexEditView::OnDffdHide()
{
	if (pdfv_ == NULL)
		return;   // already hidden

	int snum_d = GetFrame()->splitter_.FindViewColumn(pdfv_->GetSafeHwnd());
	int tnum_d = GetFrame()->ptv_->FindTab(pdfv_->GetSafeHwnd());

	ASSERT(snum_d > -1 || tnum_d > -1);  // It must be open in tab or splitter

	pdfv_ = NULL;
	get_dffd_in_range(GetScroll());  // clear any field bg colours

	if (snum_d > -1)  // splitter?
	{
		int dummy;
		GetFrame()->splitter_.GetColumnInfo(snum_d, split_width_d_, dummy);  // save split window width in case it is reopened
		VERIFY(GetFrame()->splitter_.DelColumn(snum_d, TRUE));
		GetFrame()->splitter_.RecalcLayout();
	}
	else if (tnum_d > -1)  // tab?
	{
		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing tree view
		VERIFY(GetFrame()->ptv_->RemoveView(tnum_d));
	}
}
void CHexEditView::OnUpdateDffdHide(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(TemplateViewType() == 0);
}

void CHexEditView::OnDffdSplit()
{
	if (DoDffdSplit())
		pdfv_->SendMessage(WM_INITIALUPDATE);
}
bool CHexEditView::DoDffdSplit()
{
	//if (pdfv_ == GetFrame()->splitter_.GetPane(0, 0))
	if (TemplateViewType() == 1)
		return false;   // already open in splitter

	// If open then it is open in a tab - close it
	if (pdfv_ != NULL)
	{
		int tnum_d = GetFrame()->ptv_->FindTab(pdfv_->GetSafeHwnd());
		ASSERT(tnum_d > -1);
		pdfv_ = NULL;

		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing tree view
		if (tnum_d > -1)
			VERIFY(GetFrame()->ptv_->RemoveView(tnum_d));
	}

	// Reopen in the splitter
	CCreateContext ctxt;
	ctxt.m_pNewViewClass = RUNTIME_CLASS(CDataFormatView);
	ctxt.m_pCurrentDoc = GetDocument();
	ctxt.m_pLastView = this;
	ctxt.m_pCurrentFrame = GetFrame();

	CHexEditSplitter *psplitter = &(GetFrame()->splitter_);
	ASSERT(psplitter->m_hWnd != 0);

	// Make sure width of splitter window is OK
	CRect rr;
	GetFrame()->GetClientRect(&rr);
	if (split_width_d_ < 10 || split_width_d_ > rr.Width())
	{
		split_width_d_ = rr.Width()/3;
		if (split_width_d_ < 10)
			split_width_d_ = 10;
	}

	// Add tree view in left splitter column
	VERIFY(psplitter->InsColumn(0, split_width_d_, 10, RUNTIME_CLASS(CDataFormatView), &ctxt));
	psplitter->SetActivePane(0, 1);   // make hex (tab) view active = now in col 1

	pdfv_ = (CDataFormatView *)psplitter->GetPane(0, 0);
	ASSERT_KINDOF(CDataFormatView, pdfv_);

	// Make sure dataformat view knows which hex view it is assoc. with
	pdfv_->phev_ = this;
	get_dffd_in_range(GetScroll());

	psplitter->RecalcLayout();
	AdjustColumns();
	return true;
}
void CHexEditView::OnUpdateDffdSplit(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(TemplateViewType() == 1);
	pCmdUI->Enable(GetDocument()->ptree_ != NULL);
}

void CHexEditView::OnDffdTab()
{
	if (DoDffdTab())
		pdfv_->SendMessage(WM_INITIALUPDATE);
}
bool CHexEditView::DoDffdTab()
{
	if (TemplateViewType() == 2)
		return false;  // already open in tab

	// Close DFFD view in split window if there is one
	if (pdfv_ != NULL)
	{
		int snum_d = GetFrame()->splitter_.FindViewColumn(pdfv_->GetSafeHwnd());
		ASSERT(snum_d > -1);

		pdfv_ = NULL;
		if (snum_d > -1)
		{
			int dummy;
			GetFrame()->splitter_.GetColumnInfo(snum_d, split_width_d_, dummy);  // save split window width in case it is reopened
			VERIFY(GetFrame()->splitter_.DelColumn(snum_d, TRUE));
			GetFrame()->splitter_.RecalcLayout();
		}
	}

	// Reopen in the tab
	CHexTabView *ptv = GetFrame()->ptv_;
	int tnum_d = ptv->AddView(RUNTIME_CLASS (CDataFormatView), _T("Template (Tree) View"));

	ptv->SetActiveView(tnum_d);
	pdfv_ = (CDataFormatView *)ptv->GetActiveView();
	ASSERT_KINDOF(CDataFormatView, pdfv_);

	// Make sure dataformat view knows which hex view it is assoc. with
	pdfv_->phev_ = this;
	get_dffd_in_range(GetScroll());

	ptv->SetActiveView(0);
	return true;
}
void CHexEditView::OnUpdateDffdTab(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(TemplateViewType() == 2);
	pCmdUI->Enable(GetDocument()->ptree_ != NULL);
}

// public function that just says how the template is displayed
int CHexEditView::TemplateViewType() const
{
	if (pdfv_ == NULL)
		return 0;
	else if (GetFrame()->splitter_.FindViewColumn(pdfv_->GetSafeHwnd()) > -1)
		return 1;
	else if (GetFrame()->ptv_->FindTab(pdfv_->GetSafeHwnd()) > -1)
		return 2;
	else
	{
		ASSERT(FALSE);
		return 0;
	}
}

// Aerial View commands
void CHexEditView::OnAerialHide()
{
	if (pav_ == NULL)
		return;   // already hidden

	int snum_a = GetFrame()->splitter_.FindViewColumn(pav_->GetSafeHwnd());
	int tnum_a = GetFrame()->ptv_->FindTab(pav_->GetSafeHwnd());
	ASSERT(snum_a > -1 || tnum_a > -1);  // It must be open in tab or splitter

	// Set pav_ to NULL now to avoid the view getting messages while invalid
	pav_ = NULL;
	GetDocument()->RemoveAerialView();

	if (snum_a > -1)  // splitter?
	{
		int dummy;
		GetFrame()->splitter_.GetColumnInfo(snum_a, split_width_a_, dummy);  // save split window width in case it is reopened
		VERIFY(GetFrame()->splitter_.DelColumn(snum_a, TRUE));
		GetFrame()->splitter_.RecalcLayout();
	}
	else if (tnum_a > -1)  // tab?
	{
		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing aerial view
		VERIFY(GetFrame()->ptv_->RemoveView(tnum_a));
	}
}

void CHexEditView::OnUpdateAerialHide(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(AerialViewType() == 0);
}

void CHexEditView::OnAerialSplit()
{
	DoAerialSplit();
}

bool CHexEditView::DoAerialSplit(bool init /*=true*/)
{
	int snum_a;
	if (pav_ != NULL && (snum_a = GetFrame()->splitter_.FindViewColumn(pav_->GetSafeHwnd())) > -1)
	{
		if (GetFrame()->splitter_.ColWidth(snum_a) < 8) AdjustColumns();
		return false;   // already open in splitter
	}

	CAerialView * pSaved  = pav_;    // Save this so we can delete tab view after open in split view
	pav_ = NULL;

	// Reopen in the splitter
	CCreateContext ctxt;
	ctxt.m_pNewViewClass = RUNTIME_CLASS(CAerialView);
	ctxt.m_pCurrentDoc = GetDocument();
	ctxt.m_pLastView = this;
	ctxt.m_pCurrentFrame = GetFrame();

	CHexEditSplitter *psplitter = &(GetFrame()->splitter_);
	ASSERT(psplitter->m_hWnd != 0);

	// Make sure width of splitter window is OK
	CRect rr;
	GetFrame()->GetClientRect(&rr);
	if (split_width_a_ < 8 || split_width_a_ > rr.Width())
	{
		split_width_a_ = rr.Width()/3;
		if (split_width_a_ < 8)
			split_width_a_ = 8;
	}

	// Add aerial view column to the right of hex (tab) view column
	int snum_t = GetFrame()->splitter_.FindViewColumn(GetFrame()->ptv_->GetSafeHwnd());
	VERIFY(psplitter->InsColumn(snum_t + 1, split_width_a_, 8, RUNTIME_CLASS(CAerialView), &ctxt));
	psplitter->SetActivePane(0, snum_t);   // make hex view active

	pav_ = (CAerialView *)psplitter->GetPane(0, snum_t + 1);
	ASSERT_KINDOF(CAerialView, pav_);

	// Make sure aerial view knows which hex view it is assoc. with
	pav_->phev_ = this;

	psplitter->SetColumnInfo(snum_t, rr.Width() - split_width_a_, 10);
	psplitter->RecalcLayout();
	AdjustColumns();

	if (init)
		pav_->SendMessage(WM_INITIALUPDATE);

	// If open then it is open in a tab - close it
	if (pSaved != NULL)
	{
		int tnum_a = GetFrame()->ptv_->FindTab(pSaved->GetSafeHwnd());
		ASSERT(tnum_a > -1);

		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing aerial view
		if (tnum_a > -1)
			VERIFY(GetFrame()->ptv_->RemoveView(tnum_a));
		GetDocument()->RemoveAerialView();
	}

	return true;
}

void CHexEditView::OnUpdateAerialSplit(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AerialViewType() == 1);
}

void CHexEditView::OnAerialTab()
{
	DoAerialTab();
}

bool CHexEditView::DoAerialTab(bool init /*=true*/)
{
	if (pav_ != NULL && GetFrame()->ptv_->FindTab(pav_->GetSafeHwnd()) > -1)
		return false;

	CAerialView *pSaved  = pav_;    // Save this so we can delete the tab view after open in split view
	pav_ = NULL;

	// Reopen in the tab
	CHexTabView *ptv = GetFrame()->ptv_;
	int tnum_a = ptv->AddView(RUNTIME_CLASS (CAerialView), _T("Aerial View"));
	ASSERT(tnum_a > 0);
	if (tnum_a == -1)
		return false;

	ptv->SetActiveView(tnum_a);
	pav_ = (CAerialView *)ptv->GetActiveView();
	ASSERT_KINDOF(CAerialView, pav_);

	// Make sure aerial view knows which hex view it is assoc. with
	pav_->phev_ = this;

	ptv->SetActiveView(0);
	if (init)
		pav_->SendMessage(WM_INITIALUPDATE);

	// Close Aerial view in split window if there is one
	if (pSaved != NULL)
	{
		int snum_a = GetFrame()->splitter_.FindViewColumn(pSaved->GetSafeHwnd());
		ASSERT(snum_a > 0);       // Should be there (not -1) and not the left one (not 0)

		if (snum_a > -1)
		{
			int dummy;
			GetFrame()->splitter_.GetColumnInfo(snum_a, split_width_a_, dummy);  // save split window width in case it is reopened
			VERIFY(GetFrame()->splitter_.DelColumn(snum_a, TRUE));
			GetFrame()->splitter_.RecalcLayout();
			GetDocument()->RemoveAerialView();
		}
	}
	return true;
}

void CHexEditView::OnUpdateAerialTab(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AerialViewType() == 2);
}

// public functiom that just says how we are displaying the aerial view
int CHexEditView::AerialViewType() const
{
	if (pav_ == NULL)
		return 0;
	else if (GetFrame()->splitter_.FindViewColumn(pav_->GetSafeHwnd()) > -1)
		return 1;
	else if (GetFrame()->ptv_->FindTab(pav_->GetSafeHwnd()) > -1)
		return 2;
	else
	{
		ASSERT(FALSE);
		return 0;
	}
}

BOOL CHexEditView::CompareWithSelf()
{
	return GetDocument()->bCompSelf_;
}

// Compare View commands
void CHexEditView::OnCompHide()
{
	if (pcv_ == NULL)
		return;   // already hidden

	int snum_c = GetFrame()->splitter_.FindViewColumn(pcv_->GetSafeHwnd());
	int tnum_c = GetFrame()->ptv_->FindTab(pcv_->GetSafeHwnd());

	pcv_ = NULL;  // Clear it now to avoid getting errors by trying to use it when dead
	GetDocument()->RemoveCompView();  // compare window is detroyed directly (no WM_CLOSE) so we need this here

	ASSERT(snum_c > -1 || tnum_c > -1);  // It must be open in tab or splitter

	if (snum_c > -1)  // splitter?
	{
		int dummy;
		GetFrame()->splitter_.GetColumnInfo(snum_c, split_width_c_, dummy);  // save split window width in case it is reopened
		VERIFY(GetFrame()->splitter_.DelColumn(snum_c, TRUE));
		GetFrame()->splitter_.RecalcLayout();
	}
	else if (tnum_c > -1)  // tab?
	{
		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing compare view
		VERIFY(GetFrame()->ptv_->RemoveView(tnum_c));
	}
}

void CHexEditView::OnUpdateCompHide(CCmdUI* pCmdUI)
{
	//pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(CompViewType() == 0);
}

void CHexEditView::OnCompSplit()
{
	DoCompSplit();
}

bool CHexEditView::DoCompSplit(bool init /*=true*/)
{
	int snum_c;
	if (pcv_ != NULL && (snum_c = GetFrame()->splitter_.FindViewColumn(pcv_->GetSafeHwnd())) > -1)
	{
		if (GetFrame()->splitter_.ColWidth(snum_c) < 8) AdjustColumns();
		return false;   // already open in splitter
	}

	// Make sure we have the name of the compare file to open
	if (!GetDocument()->GetCompareFile())
		return false;

	// Save current view so we can close it later
	CCompareView * pSaved = pcv_;
	pcv_ = NULL;

	// Reopen in the splitter
	CCreateContext ctxt;
	ctxt.m_pNewViewClass = RUNTIME_CLASS(CCompareView);
	ctxt.m_pCurrentDoc = GetDocument();
	ctxt.m_pLastView = this;
	ctxt.m_pCurrentFrame = GetFrame();

	CHexEditSplitter *psplitter = &(GetFrame()->splitter_);
	ASSERT(psplitter->m_hWnd != 0);

	// Make sure width of splitter window is OK
	CRect rr;
	GetFrame()->GetClientRect(&rr);
	if (split_width_c_ < 8 || split_width_c_ > rr.Width())
	{
		split_width_c_ = rr.Width()/3;
		if (split_width_c_ < 8)
			split_width_c_ = 8;
	}

	// Add compare view column to the right of hex (tab) view column
	int snum_t = GetFrame()->splitter_.FindViewColumn(GetFrame()->ptv_->GetSafeHwnd());
	VERIFY(psplitter->InsColumn(snum_t + 1, split_width_c_, 8, RUNTIME_CLASS(CCompareView), &ctxt));
	psplitter->SetActivePane(0, snum_t);   // make hex view active

	pcv_ = (CCompareView *)psplitter->GetPane(0, snum_t + 1);
	ASSERT_KINDOF(CCompareView, pcv_);
	pcv_->phev_ = this;

	psplitter->SetColumnInfo(snum_t, rr.Width() - split_width_c_, 10);
	psplitter->RecalcLayout();
	AdjustColumns();

	// Make sure compare view knows which hex view it is assoc. with
	if (init)
		pcv_->SendMessage(WM_INITIALUPDATE);

	// If it was open in the tabbed view close that view
	if (pSaved != NULL)
	{
		int tnum_c = GetFrame()->ptv_->FindTab(pSaved->GetSafeHwnd());
		ASSERT(tnum_c > -1);

		GetFrame()->ptv_->SetActiveView(0);  // Make sure hex view (always view 0) is active before removing compare view
		if (tnum_c > -1)
			VERIFY(GetFrame()->ptv_->RemoveView(tnum_c));
		GetDocument()->RemoveCompView();    // compare window is detroyed directly (no WM_CLOSE) so we need this here
	}

	return true;
}

void CHexEditView::OnUpdateCompSplit(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(CompViewType() == 1);
}

void CHexEditView::OnCompTab()
{
	DoCompTab();
}

bool CHexEditView::DoCompTab(bool init /*=true*/)
{
	if (pcv_ != NULL && GetFrame()->ptv_->FindTab(pcv_->GetSafeHwnd()) > -1)
		return false;  // already open in tab

	// Save current view so we can close it later
	CCompareView * pSaved = pcv_;
	pcv_ = NULL;

	// Make sure we have the name of the compare file to open
	if (!GetDocument()->GetCompareFile())
		return false;

	// Reopen in the tab
	CHexTabView *ptv = GetFrame()->ptv_;
	int tnum_c = ptv->AddView(RUNTIME_CLASS (CCompareView), _T("Compare View"));
	ASSERT(tnum_c > 0);
	if (tnum_c == -1)
		return false;

	ptv->SetActiveView(tnum_c);
	pcv_ = (CCompareView *)ptv->GetActiveView();
	ASSERT_KINDOF(CCompareView, pcv_);

	// Make sure compare view knows which hex view it is assoc. with
	pcv_->phev_ = this;

	ptv->SetActiveView(0);
	if (init)
		pcv_->SendMessage(WM_INITIALUPDATE);

	// Close Compare view in split window if there is one
	if (pSaved != NULL)
	{
		int snum_c = GetFrame()->splitter_.FindViewColumn(pSaved->GetSafeHwnd());
		ASSERT(snum_c > 0);       // Should be there (not -1) and not the left one (not 0)

		if (snum_c > -1)
		{
			int dummy;
			GetFrame()->splitter_.GetColumnInfo(snum_c, split_width_c_, dummy);  // save split window width in case it is reopened
			VERIFY(GetFrame()->splitter_.DelColumn(snum_c, TRUE));
			GetFrame()->splitter_.RecalcLayout();
			GetDocument()->RemoveCompView();    // compare window is detroyed directly (no WM_CLOSE) so we need this here
		}
	}
	return true;
}

void CHexEditView::OnUpdateCompTab(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(CompViewType() == 2);
}

int CHexEditView::CompViewType() const
{
	if (pcv_ == NULL)
		return 0;
	else if (GetFrame()->splitter_.FindViewColumn(pcv_->GetSafeHwnd()) > -1)
		return 1;
	else if (GetFrame()->ptv_->FindTab(pcv_->GetSafeHwnd()) > -1)
		return 2;
	else
	{
		ASSERT(FALSE);
		return 0;
	}
}

// private function which hopefully makes sure all the splitter columns are obvious (ie a min width)
void CHexEditView::AdjustColumns()
{
	int d, t, a, c, min;        // Current width of dffd, tab, aerial, and compare columns
	d = t = a = c = -1;
	int snum_d, snum_t, snum_a, snum_c;
	snum_d = snum_t = snum_a = snum_c = -1;

	// Get current column widths
	CHexEditSplitter *psplitter = &(GetFrame()->splitter_);
	if (pdfv_ != NULL) snum_d = psplitter->FindViewColumn(pdfv_->GetSafeHwnd());
	snum_t = psplitter->FindViewColumn(GetFrame()->ptv_->GetSafeHwnd());        // We always have a tab column since it contains the hex view
	if (pav_ != NULL) snum_a = psplitter->FindViewColumn(pav_->GetSafeHwnd());
	if (pcv_ != NULL) snum_c = psplitter->FindViewColumn(pcv_->GetSafeHwnd());

	if (snum_d > -1) psplitter->GetColumnInfo(snum_d, d, min);
	if (snum_t > -1) psplitter->GetColumnInfo(snum_t, t, min);
	if (snum_a > -1) psplitter->GetColumnInfo(snum_a, a, min);
	if (snum_c > -1) psplitter->GetColumnInfo(snum_c, c, min);

	// Make ideal widths slightly smaller but not less than a minimum
	bool adjust = false;
	d -= 20; if (snum_d > -1 && d < 20) { d = 20; adjust = true; }
	t -= 30; if (snum_t > -1 && t < 30) { t = 30; adjust = true; }
	a -= 10; if (snum_a > -1 && a < 10) { a = 10; adjust = true; }
	c -= 10; if (snum_c > -1 && c < 10) { c = 10; adjust = true; }
	if (adjust)
	{
		if (snum_d > -1) psplitter->SetColumnInfo(snum_d, d, 10);
		if (snum_t > -1) psplitter->SetColumnInfo(snum_t, t, 10);
		if (snum_a > -1) psplitter->SetColumnInfo(snum_a, a, 8);
		if (snum_c > -1) psplitter->SetColumnInfo(snum_c, c, 8);
		psplitter->RecalcLayout();
	}
	if (snum_d > -1) psplitter->GetColumnInfo(snum_d, split_width_d_, min);
	if (snum_a > -1) psplitter->GetColumnInfo(snum_a, split_width_a_, min);
	if (snum_c > -1) psplitter->GetColumnInfo(snum_c, split_width_c_, min);
}

// Command to go to first recent difference in compare view
void CHexEditView::OnCompFirst()
{
	if (GetDocument()->CompareDifferences() > 0)
	{
		FILE_ADDRESS addr;
		int len;
		GetDocument()->GetOrigDiff(0, 0, addr, len);
		MoveToAddress(addr, addr+len);
	}
}

void CHexEditView::OnUpdateCompFirst(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->CompareDifferences() > 0);
}

// Command to go to previous recent difference in compare view
void CHexEditView::OnCompPrev()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	int idx = GetDocument()->FirstDiffAt(false, 0, start - 1);
	if (idx > 0)
	{
		FILE_ADDRESS addr;
		int len;
		GetDocument()->GetOrigDiff(0, idx - 1, addr, len);
		MoveToAddress(addr, addr+len);
	}
}

void CHexEditView::OnUpdateCompPrev(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);
	pCmdUI->Enable(GetDocument()->CompareDifferences() >= 0 &&
				   GetDocument()->FirstDiffAt(false, 0, start - 1) > 0);
}

// Command to go to next recent difference in compare view
void CHexEditView::OnCompNext()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	int idx = GetDocument()->FirstDiffAt(false, 0, start);
	if (idx < GetDocument()->CompareDifferences(0))
	{
		FILE_ADDRESS addr;
		int len;
		GetDocument()->GetOrigDiff(0, idx, addr, len);
		MoveToAddress(addr, addr+len);
	}
}

void CHexEditView::OnUpdateCompNext(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);
	pCmdUI->Enable(GetDocument()->CompareDifferences() >= 0 &&
				   GetDocument()->FirstDiffAt(false, 0, start) < 
							   GetDocument()->CompareDifferences());
}

// Command to go to last recent difference in compare view
void CHexEditView::OnCompLast()
{
	int count = GetDocument()->CompareDifferences();
	if (count > 0)
	{
		FILE_ADDRESS addr;
		int len;
		GetDocument()->GetOrigDiff(0, count - 1, addr, len);
		MoveToAddress(addr, addr+len);
	}
}

void CHexEditView::OnUpdateCompLast(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->CompareDifferences() > 0);
}

// Command to go to first of all differences (not just recent) in compare view
void CHexEditView::OnCompAllFirst()
{
	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetFirstDiffAll(addr, len);
	if (addr > -1)
		MoveToAddress(addr, addr+len);
}

void CHexEditView::OnUpdateCompAllFirst(CCmdUI* pCmdUI)
{
	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetFirstDiffAll(addr, len);
	pCmdUI->Enable(addr > -1);
}

void CHexEditView::OnCompAllPrev()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetPrevDiffAll(start - 1, addr, len);
	if (addr > -1)
		MoveToAddress(addr, addr+len);
}

void CHexEditView::OnUpdateCompAllPrev(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetPrevDiffAll(start - 1, addr, len);
	pCmdUI->Enable(addr > -1);
}

void CHexEditView::OnCompAllNext()
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetNextDiffAll(end, addr, len);
	if (addr > -1)
		MoveToAddress(addr, addr+len);
}

void CHexEditView::OnUpdateCompAllNext(CCmdUI* pCmdUI)
{
	FILE_ADDRESS start, end;  // current selection
	GetSelAddr(start, end);

	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetNextDiffAll(end, addr, len);
	pCmdUI->Enable(addr > -1);
}

void CHexEditView::OnCompAllLast()
{
	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetLastDiffAll(addr, len);
	if (addr > -1)
		MoveToAddress(addr, addr+len);
}

void CHexEditView::OnUpdateCompAllLast(CCmdUI* pCmdUI)
{
	FILE_ADDRESS addr;
	int len;
	GetDocument()->GetLastDiffAll(addr, len);
	pCmdUI->Enable(addr > -1);
}

void CHexEditView::OnCompAutoSync()
{
	if (pcv_ == NULL)
	{
		AfxMessageBox("No Compare is available");
		theApp.mac_error_ = 10;
		return;
	}
	begin_change();
	display_.auto_sync_comp = !display_.auto_sync_comp;
	make_change();
	end_change();

	// If has been turned on then sync now
	if (display_.auto_sync_comp)
	{
		FILE_ADDRESS start_addr, end_addr;
		GetSelAddr(start_addr, end_addr);
		pcv_->MoveToAddress(start_addr, end_addr);
	}

	theApp.SaveToMacro(km_comp_sync, 2);
}

void CHexEditView::OnUpdateCompAutoSync(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pcv_ != NULL);
	pCmdUI->SetCheck(display_.auto_sync_comp);
}

void CHexEditView::OnCompAutoScroll()
{
	if (pcv_ == NULL)
	{
		AfxMessageBox("No Compare is available");
		theApp.mac_error_ = 10;
		return;
	}
	begin_change();
	display_.auto_scroll_comp = !display_.auto_scroll_comp;
	make_change();
	end_change();

	// If has been turned on then scroll now
	if (display_.auto_scroll_comp)
		pcv_->SetScroll(GetScroll());

	theApp.SaveToMacro(km_comp_sync, 3);
}

void CHexEditView::OnUpdateCompAutoScroll(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pcv_ != NULL);
	pCmdUI->SetCheck(display_.auto_scroll_comp);
}

int CHexEditView::CurrentSearchOccurrence()
{
	FILE_ADDRESS start, end;
	(void)GetSelAddr(start, end);
	std::vector<FILE_ADDRESS> sf = GetDocument()->SearchAddresses(0, start+1);
	return sf.size();
}

// This is connected to Ctrl+T and is used for testing new dialogs etc
void CHexEditView::OnViewtest()
{
	// for testing new commands etc
#ifdef _DEBUG
	test_misc();
#endif
}

CTipExpr::value_t CTipExpr::find_symbol(const char *sym, value_t parent, size_t index, int *pac,
		__int64 &sym_size, __int64 &sym_address, CString &)
{
	ASSERT(pview_ != NULL);
	ASSERT(parent.typ == TYPE_NONE);    // Parent is not used here
	value_t retval;
	retval.typ = TYPE_NONE;             // Default to symbol not found
	retval.error = false;
	retval.int64 = 0;
	sym_address = pview_->tip_addr_;
	unsigned_ = false;

	CString sym_str(sym);
	if (sym_str.CompareNoCase("address") == 0)
	{
		retval.typ = TYPE_INT;          // Just return the address
		retval.int64 = sym_address;
		sym_size = 4;                   // Addresses can be up to 8 but this restricts display to 32-bits (and if bigger then extra bits are shown)
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("cursor") == 0)
	{
		FILE_ADDRESS start, end;
		pview_->GetSelAddr(start, end);
		retval.typ = TYPE_INT;
		retval.int64 = start;
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("sel_len") == 0)
	{
		FILE_ADDRESS start, end;
		pview_->GetSelAddr(start, end);
		retval.typ = TYPE_INT;
		retval.int64 = end - start;
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("mark") == 0)
	{
		retval.typ = TYPE_INT;
		retval.int64 = pview_->GetMark();
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("eof") == 0)
	{
		retval.typ = TYPE_INT;
		retval.int64 = pview_->GetDocument()->length();
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("sector") == 0)
	{
		retval.typ = TYPE_INT;          // current sector
		int seclen = pview_->GetDocument()->GetSectorSize();
		if (seclen > 0)
			retval.int64 = sym_address / seclen;
		else
			retval.int64 = 0;
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("offset") == 0)
	{
		retval.typ = TYPE_INT;
		retval.int64 = pview_->real_offset_;
		sym_size = 4;
		unsigned_ = true;
	}
	else if (sym_str.CompareNoCase("sbyte") == 0)
	{
		signed char val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			retval.typ = TYPE_INT;          // Just return the current byte
			retval.int64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("byte") == 0)
	{
		unsigned char val;
		if (pview_->GetDocument()->GetData(&val, sizeof(val), sym_address) == sizeof(val))
		{
			retval.typ = TYPE_INT;          // Just return the current byte
			retval.int64 = val;
			sym_size = sizeof(val);
			unsigned_ = true;
		}
	}
	else if (sym_str.CompareNoCase("word") == 0)
	{
		short val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("uword") == 0)
	{
		unsigned short val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
			unsigned_ = true;
		}
	}
	else if (sym_str.CompareNoCase("dword") == 0)
	{
		long val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("udword") == 0)
	{
		unsigned long val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
			unsigned_ = true;
		}
	}
	else if (sym_str.CompareNoCase("qword") == 0)
	{
		__int64 val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("uqword") == 0)
	{
		unsigned __int64 val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_INT;
			retval.int64 = val;
			sym_size = sizeof(val);
			unsigned_ = true;
		}
	}
	else if (sym_str.CompareNoCase("ieee32") == 0)
	{
		float val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_REAL;
			retval.real64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("ieee64") == 0)
	{
		double val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_REAL;
			retval.real64 = val;
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("ibm32") == 0)
	{
		unsigned char val[4];
		if (pview_->GetDocument()->GetData(val, sizeof(val), sym_address) == sizeof(val))
		{
			retval.typ = TYPE_REAL;
			retval.real64 = ::ibm_fp32(val, NULL, NULL, !pview_->BigEndian());
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("ibm64") == 0)
	{
		unsigned char val[8];
		if (pview_->GetDocument()->GetData(val, sizeof(val), sym_address) == sizeof(val))
		{
			retval.typ = TYPE_REAL;
			retval.real64 = ::ibm_fp64(val, NULL, NULL, !pview_->BigEndian());
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("real48") == 0)
	{
		unsigned char val[6];
		if (pview_->GetDocument()->GetData(val, sizeof(val), sym_address) == sizeof(val))
		{
			retval.typ = TYPE_REAL;
			retval.real64 = ::real48(val, NULL, NULL, pview_->BigEndian() == TRUE);
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("time_t") == 0)
	{
		time_t val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_DATE;
			retval.date = FromTime_t(val);
			sym_size = sizeof(val);
		}
	}
#ifdef TIME64_T
	else if (sym_str.CompareNoCase("time64_t") == 0)
	{
		__int64 val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_DATE;
			retval.date = FromTime_t(val);
			sym_size = sizeof(val);
		}
	}
#endif
	else if (sym_str.CompareNoCase("time_t_80") == 0)  // MSC 5.1 time_t
	{
		long val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_DATE;
			retval.date = FromTime_t_80(val);
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("time_t_1899") == 0)  // MSC 7 time_t
	{
		unsigned long val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_DATE;
			retval.date = FromTime_t_1899(val);
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("time_t_mins") == 0)  // MSC 7 time_t
	{
		long val;
		if (pview_->GetDocument()->GetData((unsigned char *)&val, sizeof(val), sym_address) == sizeof(val))
		{
			if (pview_->BigEndian())
				flip_bytes((unsigned char *)&val, sizeof(val));
			retval.typ = TYPE_DATE;
			retval.date = FromTime_t_mins(val);
			sym_size = sizeof(val);
		}
	}
	else if (sym_str.CompareNoCase("astring") == 0)
	{
		char buf[256];
		size_t len = pview_->GetDocument()->GetData((unsigned char *)buf, sizeof(buf)-1, sym_address);
		buf[len] = '\0';
		retval.typ = TYPE_STRING;
		retval.pstr = new ExprStringType((LPCTSTR)buf);
	}

	size_ = int(sym_size);  // Remember size of the symbol we encountered
	return retval;
}
