#if !defined(AFX_DATAFORMATVIEW_H__AA218642_2DFC_4659_94FE_85AD21EDE726__INCLUDED_)
#define AFX_DATAFORMATVIEW_H__AA218642_2DFC_4659_94FE_85AD21EDE726__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "GridCtrl_src/GridCtrl.h"
#include "GridBtnCell_src/GridBtnCell.h"
#include "GridTreeBtnCell_src/GridTreeBtnCell.h"
#include "TreeColumn_src/TreeColumn.h"

// DataFormatView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGridCtrl2 - our class derived from CGridCtrl

class CGridCtrl2 : public CGridCtrl
{
    DECLARE_DYNCREATE(CGridCtrl2)
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
#if _MSC_VER >= 1300
    virtual void OnEditCell(int nRow, int nCol, CPoint point, UINT nChar);
    virtual void OnEndEditCell(int nRow, int nCol, CStringW str);
#endif

#ifndef GRIDCONTROL_NO_DRAGDROP
 public:
     CGridCtrl2() { pdoc_=NULL; pview_=NULL; VERIFY(m_cf_ours = (CLIPFORMAT)::RegisterClipboardFormat(_T("HexEdit.DFFD.DragAndDrop"))); }

    virtual void OnBeginDrag();
    virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
    virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
    virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
    CLIPFORMAT m_cf_ours;               // Custom Clip format to hold COM ptr (MSXML::IXMLDOMElementPtr)
#endif

protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnSelchange();
    DECLARE_MESSAGE_MAP()

	CString UniqueName(LPCTSTR base, const CXmlTree::CElt elt);

public:
    void FixHeading(int col);
    void SetDocView(CHexEditDoc *pdoc, CDataFormatView *pv) { pdoc_ = pdoc; pview_ = pv; }

private:
    CHexEditDoc *pdoc_;
    CDataFormatView *pview_;
    int sel_row_;                       // Row selected when template selection drop list clicked
    CStringW savedStr;
};

/////////////////////////////////////////////////////////////////////////////
// CDataFormatView view - displays "tree" view in a CGridCtrl

class CDataFormatView : public CView
{
public:
    // Number the different types of columns we can display
    enum
    {
        COL_TREE,                           // Tree column - always present
        COL_HEX_ADDRESS, COL_DEC_ADDRESS,   // hex/decimal address of start of elt in file
        COL_SIZE,                           // size of elt in bytes
        COL_TYPE,                           // decription of type (including endianness)
        COL_TYPE_NAME,                      // name of type from ext. source (eg typedef)
        COL_DATA,                           // the actual value from the file
        COL_UNITS,                          // the units for the data (if any)
        COL_DOMAIN,                         // describes the valid values
        COL_FLAGS,                          // flags (eg read-only)
        COL_COMMENT,                        // anything else decribing the data
		COL_LEVEL,                          // indent level in tree
        COL_LAST                            // leave at end (signals end of list)
    };

protected:
    CDataFormatView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CDataFormatView)

// Attributes
public:
    CHexEditView *phev_;                     // Ptr to sister hex view

// Operations
public:
    void InitColumnHeadings();
	CString GetColWidths();
    void InitTree();
    void save_tree_state();
    void restore_tree_state();
    void set_colours();
    void SelectAt(FILE_ADDRESS addr);
    BOOL ReadOnly(FILE_ADDRESS addr, FILE_ADDRESS end_addr = -1);
	bool has_children(int row);             // returns true if row below has indent one more
    void show_row(int row);                 // expands all ancestors so that a row is shown
    void expand_all(int row);               // expands all nodes below
	bool expand_one(int row);               // expands one level - returns false if already expanded
	bool collapse(int row);					// hides all sub-nodes - returns false if already collpased
	void goto_parent(int row);              // move to parent of specified row
	void goto_sibling(int row, int n);      // move to n'th child of parent - which may be same row
	void prev_error(int row);               // move to closest error row above this row
	void next_error(int row);               // move to first error row after this row

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDataFormatView)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL
		afx_msg void OnEditCopy();
		afx_msg void OnEditCut();
		afx_msg void OnUpdateEditCopyOrCut(CCmdUI* pCmdUI);
        afx_msg void OnFilePrintPreview();

        afx_msg void OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult);
        afx_msg void OnGridDoubleClick(NMHDR *pNotifyStruct, LRESULT* pResult);
        afx_msg void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* pResult);
        afx_msg void OnGridEndSelChange(NMHDR *pNotifyStruct, LRESULT* pResult);
        afx_msg void OnGridBeginLabelEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
        afx_msg void OnGridEndLabelEdit(NMHDR *pNotifyStruct, LRESULT* pResult);

        afx_msg void OnAddrToggle() { phev_->OnAddrToggle(); }
        afx_msg void OnUpdateAddrToggle(CCmdUI *pCmdUI) { phev_->OnUpdateAddrToggle(pCmdUI); }
        afx_msg void OnGraphicToggle() { phev_->OnGraphicToggle(); }
        afx_msg void OnUpdateGraphicToggle(CCmdUI *pCmdUI) { phev_->OnUpdateGraphicToggle(pCmdUI); }
        afx_msg void OnCharToggle() { phev_->OnCharToggle(); }
        afx_msg void OnUpdateCharToggle(CCmdUI *pCmdUI) { phev_->OnUpdateCharToggle(pCmdUI); }
        afx_msg void OnFont() { phev_->OnFont(); }
        afx_msg void OnAutoFit() { phev_->OnAutoFit(); }
        afx_msg void OnUpdateAutofit(CCmdUI *pCmdUI) { phev_->OnUpdateAutofit(pCmdUI); }
        afx_msg void OnAscEbc() { phev_->OnAscEbc(); }
        afx_msg void OnUpdateAscEbc(CCmdUI *pCmdUI) { phev_->OnUpdateAscEbc(pCmdUI); }
        afx_msg void OnControl() { phev_->OnControl(); }
        afx_msg void OnUpdateControl(CCmdUI *pCmdUI) { phev_->OnUpdateControl(pCmdUI); }
        afx_msg void OnDffdAutoSync() { phev_->OnDffdAutoSync(); }
        afx_msg void OnMark() { phev_->OnMark(); }
        afx_msg void OnGotoMark() { phev_->OnGotoMark(); }
        afx_msg void OnEditUndo() { phev_->OnEditUndo(); }
        afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI) { phev_->OnUpdateEditUndo(pCmdUI); }
        afx_msg void OnSearchHex() { phev_->OnSearchHex(); }
        afx_msg void OnSearchAscii() { phev_->OnSearchAscii(); }
//        afx_msg void OnSearchForw() { phev_->OnSearchForw(); }
//        afx_msg void OnSearchBack() { phev_->OnSearchBack(); }
//        afx_msg void OnSearchSel() { phev_->OnSearchSel(); }
        afx_msg void OnAllowMods() { phev_->OnAllowMods(); }
        afx_msg void OnUpdateAllowMods(CCmdUI *pCmdUI) { phev_->OnUpdateAllowMods(pCmdUI); }
        afx_msg void OnControlToggle() { phev_->OnControlToggle(); }
        afx_msg void OnInsert() { phev_->OnInsert(); }
        afx_msg void OnUpdateInsert(CCmdUI *pCmdUI) { phev_->OnUpdateInsert(pCmdUI); }
        afx_msg void OnUpdateDffdAutoSync(CCmdUI *pCmdUI) { phev_->OnUpdateDffdAutoSync(pCmdUI); }
        afx_msg void OnSearchIcase() { phev_->OnSearchIcase(); }
//        afx_msg void OnUpdateSearch(CCmdUI *pCmdUI) { phev_->OnUpdateSearch(pCmdUI); }
//        afx_msg void OnEditCompare() { phev_->OnEditCompare(); }
//        afx_msg void OnUpdateEditCompare(CCmdUI *pCmdUI) { phev_->OnUpdateEditCompare(pCmdUI); }
        afx_msg void OnTrackChanges() { phev_->OnTrackChanges(); }
        afx_msg void OnUpdateTrackChanges(CCmdUI *pCmdUI) { phev_->OnUpdateTrackChanges(pCmdUI); }

        afx_msg void OnWindowNext() { phev_->OnWindowNext(); }
        afx_msg void OnIncByte() { phev_->OnIncByte(); }
        afx_msg void OnInc16bit() { phev_->OnInc16bit(); }
        afx_msg void OnInc32bit() { phev_->OnInc32bit(); }
        afx_msg void OnInc64bit() { phev_->OnInc64bit(); }
        afx_msg void OnDecByte() { phev_->OnDecByte(); }
        afx_msg void OnDec16bit() { phev_->OnDec16bit(); }
        afx_msg void OnDec32bit() { phev_->OnDec32bit(); }
        afx_msg void OnDec64bit() { phev_->OnDec64bit(); }
        afx_msg void OnFlip16bit() { phev_->OnFlip16bit(); }
        afx_msg void OnFlip32bit() { phev_->OnFlip32bit(); }
        afx_msg void OnFlip64bit() { phev_->OnFlip64bit(); }
        afx_msg void OnUpdateByte(CCmdUI *pCmdUI) { phev_->OnUpdateByte(pCmdUI); }
        afx_msg void OnUpdate16bit(CCmdUI *pCmdUI) { phev_->OnUpdate16bit(pCmdUI); }
        afx_msg void OnUpdate32bit(CCmdUI *pCmdUI) { phev_->OnUpdate32bit(pCmdUI); }
        afx_msg void OnUpdate64bit(CCmdUI *pCmdUI) { phev_->OnUpdate64bit(pCmdUI); }
        afx_msg void OnUpdateByteBinary(CCmdUI *pCmdUI) { phev_->OnUpdateByteBinary(pCmdUI); }
        afx_msg void OnUpdate16bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate16bitBinary(pCmdUI); }
        afx_msg void OnUpdate32bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate32bitBinary(pCmdUI); }
        afx_msg void OnUpdate64bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate64bitBinary(pCmdUI); }
        afx_msg void OnSelectAll() { phev_->OnSelectAll(); }
//        afx_msg void OnEditCopy() { phev_->OnEditCopy(); }
//        afx_msg void OnEditCut() { phev_->OnEditCut(); }
//        afx_msg void OnEditPaste() { phev_->OnEditPaste(); }
        afx_msg void OnUpdateTextPaste(CCmdUI *pCmdUI) { phev_->OnUpdateTextPaste(pCmdUI); }
        afx_msg void OnUpdateClipboard(CCmdUI *pCmdUI) { phev_->OnUpdateClipboard(pCmdUI); }
        afx_msg void OnUpdateUnicodePaste(CCmdUI *pCmdUI) { phev_->OnUpdateUnicodePaste(pCmdUI); }
        afx_msg void OnFontDec() { phev_->OnFontDec(); }
        afx_msg void OnFontInc() { phev_->OnFontInc(); }
        afx_msg void OnUpdateFontDec(CCmdUI *pCmdUI) { phev_->OnUpdateFontDec(pCmdUI); }
        afx_msg void OnUpdateFontInc(CCmdUI *pCmdUI) { phev_->OnUpdateFontInc(pCmdUI); }
//        afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI) { phev_->OnUpdateEditCut(pCmdUI); }
        afx_msg void OnPasteAscii() { phev_->OnPasteAscii(); }
        afx_msg void OnPasteEbcdic() { phev_->OnPasteEbcdic(); }
        afx_msg void OnPasteUnicode() { phev_->OnPasteUnicode(); }
        afx_msg void OnCopyCchar() { phev_->OnCopyCchar(); }
        afx_msg void OnCopyHex() { phev_->OnCopyHex(); }
        afx_msg void OnEditWriteFile() { phev_->OnEditWriteFile(); }
        afx_msg void OnUpdateReadFile(CCmdUI *pCmdUI) { phev_->OnUpdateReadFile(pCmdUI); }
        afx_msg void OnReadFile() { phev_->OnReadFile(); }
        afx_msg void OnExtendToMark() { phev_->OnExtendToMark(); }
        afx_msg void OnSwapMark() { phev_->OnSwapMark(); }
        afx_msg void OnRedraw() { phev_->OnRedraw(); }
        afx_msg void OnScrollDown() { phev_->OnScrollDown(); }
        afx_msg void OnScrollUp() { phev_->OnScrollUp(); }
        afx_msg void OnSwap() { phev_->OnSwap(); }
        afx_msg void OnStartLine() { phev_->OnStartLine(); }
        afx_msg void OnDel() { phev_->OnDel(); }
        afx_msg void OnUpdateSwap(CCmdUI *pCmdUI) { phev_->OnUpdateSwap(pCmdUI); }
        afx_msg void OnOemToggle() { phev_->OnOemToggle(); }
        afx_msg void OnUpdateOemToggle(CCmdUI *pCmdUI) { phev_->OnUpdateOemToggle(pCmdUI); }
        afx_msg void OnInvert() { phev_->OnInvert(); }
        afx_msg void OnNegByte() { phev_->OnNegByte(); }
        afx_msg void OnNeg16bit() { phev_->OnNeg16bit(); }
        afx_msg void OnNeg32bit() { phev_->OnNeg32bit(); }
        afx_msg void OnNeg64bit() { phev_->OnNeg64bit(); }
        afx_msg void OnHighlight() { phev_->OnHighlight(); }
        afx_msg void OnUpdateHighlight(CCmdUI *pCmdUI) { phev_->OnUpdateHighlight(pCmdUI); }
        afx_msg void OnHighlightClear() { phev_->OnHighlightClear(); }
        afx_msg void OnHighlightPrev() { phev_->OnHighlightPrev(); }
        afx_msg void OnHighlightNext() { phev_->OnHighlightNext(); }
        afx_msg void OnUpdateHighlightPrev(CCmdUI *pCmdUI) { phev_->OnUpdateHighlightPrev(pCmdUI); }
        afx_msg void OnUpdateHighlightNext(CCmdUI *pCmdUI) { phev_->OnUpdateHighlightNext(pCmdUI); }
        afx_msg void OnEditGoto() { phev_->OnEditGoto(); }
        afx_msg void OnEditFind() { phev_->OnEditFind(); }
        afx_msg void OnEditReplace() { phev_->OnEditReplace(); }
    afx_msg void OnAscii2Ebcdic() { phev_->OnAscii2Ebcdic(); }
    afx_msg void OnUpdateConvert(CCmdUI *pCmdUI) { phev_->OnUpdateConvert(pCmdUI); }
    afx_msg void OnEbcdic2Ascii() { phev_->OnEbcdic2Ascii(); }
    afx_msg void OnAnsi2Ibm() { phev_->OnAnsi2Ibm(); }
    afx_msg void OnIbm2Ansi() { phev_->OnIbm2Ansi(); }
    afx_msg void OnEncrypt() { phev_->OnEncrypt(); }
    afx_msg void OnDecrypt() { phev_->OnDecrypt(); }
    afx_msg void OnEditAppendFile() { phev_->OnEditAppendFile(); }
    afx_msg void OnEditAppendSameFile() { phev_->OnEditAppendSameFile(); }
    afx_msg void OnUpdateEditAppendSameFile(CCmdUI *pCmdUI) { phev_->OnUpdateEditAppendSameFile(pCmdUI); }
    afx_msg void OnUndoChanges() { phev_->OnUndoChanges(); }
    afx_msg void OnUpdateUndoChanges(CCmdUI *pCmdUI) { phev_->OnUpdateUndoChanges(pCmdUI); }
    afx_msg void OnCalcSel() { phev_->OnCalcSel(); }
    afx_msg void OnUpdateCalcSel(CCmdUI *pCmdUI) { phev_->OnUpdateCalcSel(pCmdUI); }
    afx_msg void OnDisplayReset() { phev_->OnDisplayReset(); }
    afx_msg void OnUpdateEncrypt(CCmdUI *pCmdUI) { phev_->OnUpdateEncrypt(pCmdUI); }

        afx_msg void OnXorByte() { phev_->OnXorByte(); }
        afx_msg void OnXor16bit() { phev_->OnXor16bit(); }
        afx_msg void OnXor32bit() { phev_->OnXor32bit(); }
        afx_msg void OnXor64bit() { phev_->OnXor64bit(); }
        afx_msg void OnAssignByte() { phev_->OnAssignByte(); }
        afx_msg void OnAssign16bit() { phev_->OnAssign16bit(); }
        afx_msg void OnAssign32bit() { phev_->OnAssign32bit(); }
        afx_msg void OnAssign64bit() { phev_->OnAssign64bit(); }
        afx_msg void OnRandByte() { phev_->OnRandByte(); }
        afx_msg void OnRandFast() { phev_->OnRandFast(); }
//        afx_msg void OnRand16bit() { phev_->OnRand16bit(); }
//        afx_msg void OnRand32bit() { phev_->OnRand32bit(); }
//        afx_msg void OnRand64bit() { phev_->OnRand64bit(); }
        afx_msg void OnAddByte() { phev_->OnAddByte(); }
        afx_msg void OnAdd16bit() { phev_->OnAdd16bit(); }
        afx_msg void OnAdd32bit() { phev_->OnAdd32bit(); }
        afx_msg void OnAdd64bit() { phev_->OnAdd64bit(); }
        afx_msg void OnSubtractByte() { phev_->OnSubtractByte(); }
        afx_msg void OnSubtract16bit() { phev_->OnSubtract16bit(); }
        afx_msg void OnSubtract32bit() { phev_->OnSubtract32bit(); }
        afx_msg void OnSubtract64bit() { phev_->OnSubtract64bit(); }
        afx_msg void OnAndByte() { phev_->OnAndByte(); }
        afx_msg void OnAnd16bit() { phev_->OnAnd16bit(); }
        afx_msg void OnAnd32bit() { phev_->OnAnd32bit(); }
        afx_msg void OnAnd64bit() { phev_->OnAnd64bit(); }
        afx_msg void OnOrByte() { phev_->OnOrByte(); }
        afx_msg void OnOr16bit() { phev_->OnOr16bit(); }
        afx_msg void OnOr32bit() { phev_->OnOr32bit(); }
        afx_msg void OnOr64bit() { phev_->OnOr64bit(); }

        afx_msg void OnMulByte() { phev_->OnMulByte(); }
        afx_msg void OnMul16bit() { phev_->OnMul16bit(); }
        afx_msg void OnMul32bit() { phev_->OnMul32bit(); }
        afx_msg void OnMul64bit() { phev_->OnMul64bit(); }
        afx_msg void OnDivByte() { phev_->OnDivByte(); }
        afx_msg void OnDiv16bit() { phev_->OnDiv16bit(); }
        afx_msg void OnDiv32bit() { phev_->OnDiv32bit(); }
        afx_msg void OnDiv64bit() { phev_->OnDiv64bit(); }
        afx_msg void OnModByte() { phev_->OnModByte(); }
        afx_msg void OnMod16bit() { phev_->OnMod16bit(); }
        afx_msg void OnMod32bit() { phev_->OnMod32bit(); }
        afx_msg void OnMod64bit() { phev_->OnMod64bit(); }
        afx_msg void OnRevByte() { phev_->OnRevByte(); }
        afx_msg void OnRev16bit() { phev_->OnRev16bit(); }
        afx_msg void OnRev32bit() { phev_->OnRev32bit(); }
        afx_msg void OnRev64bit() { phev_->OnRev64bit(); }
        afx_msg void OnSubtractXByte() { phev_->OnSubtractXByte(); }
        afx_msg void OnSubtractX16bit() { phev_->OnSubtractX16bit(); }
        afx_msg void OnSubtractX32bit() { phev_->OnSubtractX32bit(); }
        afx_msg void OnSubtractX64bit() { phev_->OnSubtractX64bit(); }
        afx_msg void OnDivXByte() { phev_->OnDivXByte(); }
        afx_msg void OnDivX16bit() { phev_->OnDivX16bit(); }
        afx_msg void OnDivX32bit() { phev_->OnDivX32bit(); }
        afx_msg void OnDivX64bit() { phev_->OnDivX64bit(); }
        afx_msg void OnModXByte() { phev_->OnModXByte(); }
        afx_msg void OnModX16bit() { phev_->OnModX16bit(); }
        afx_msg void OnModX32bit() { phev_->OnModX32bit(); }
        afx_msg void OnModX64bit() { phev_->OnModX64bit(); }
        afx_msg void OnGtrByte() { phev_->OnGtrByte(); }
        afx_msg void OnGtr16bit() { phev_->OnGtr16bit(); }
        afx_msg void OnGtr32bit() { phev_->OnGtr32bit(); }
        afx_msg void OnGtr64bit() { phev_->OnGtr64bit(); }
        afx_msg void OnLessByte() { phev_->OnLessByte(); }
        afx_msg void OnLess16bit() { phev_->OnLess16bit(); }
        afx_msg void OnLess32bit() { phev_->OnLess32bit(); }
        afx_msg void OnLess64bit() { phev_->OnLess64bit(); }

        afx_msg void OnGtrUByte() { phev_->OnGtrUByte(); }
        afx_msg void OnGtrU16bit() { phev_->OnGtrU16bit(); }
        afx_msg void OnGtrU32bit() { phev_->OnGtrU32bit(); }
        afx_msg void OnGtrU64bit() { phev_->OnGtrU64bit(); }
        afx_msg void OnLessUByte() { phev_->OnLessUByte(); }
        afx_msg void OnLessU16bit() { phev_->OnLessU16bit(); }
        afx_msg void OnLessU32bit() { phev_->OnLessU32bit(); }
        afx_msg void OnLessU64bit() { phev_->OnLessU64bit(); }

        afx_msg void OnRolByte()  { phev_->OnRolByte(); }
        afx_msg void OnRol16bit() { phev_->OnRol16bit(); }
        afx_msg void OnRol32bit() { phev_->OnRol32bit(); }
        afx_msg void OnRol64bit() { phev_->OnRol64bit(); }
        afx_msg void OnRorByte()  { phev_->OnRorByte(); }
        afx_msg void OnRor16bit() { phev_->OnRor16bit(); }
        afx_msg void OnRor32bit() { phev_->OnRor32bit(); }
        afx_msg void OnRor64bit() { phev_->OnRor64bit(); }
        afx_msg void OnLslByte()  { phev_->OnLslByte(); }
        afx_msg void OnLsl16bit() { phev_->OnLsl16bit(); }
        afx_msg void OnLsl32bit() { phev_->OnLsl32bit(); }
        afx_msg void OnLsl64bit() { phev_->OnLsl64bit(); }
        afx_msg void OnLsrByte()  { phev_->OnLsrByte(); }
        afx_msg void OnLsr16bit() { phev_->OnLsr16bit(); }
        afx_msg void OnLsr32bit() { phev_->OnLsr32bit(); }
        afx_msg void OnLsr64bit() { phev_->OnLsr64bit(); }
        afx_msg void OnAsrByte()  { phev_->OnAsrByte(); }
        afx_msg void OnAsr16bit() { phev_->OnAsr16bit(); }
        afx_msg void OnAsr32bit() { phev_->OnAsr32bit(); }
        afx_msg void OnAsr64bit() { phev_->OnAsr64bit(); }

        afx_msg void OnJumpHexAddr() { phev_->OnJumpHexAddr(); }
//        afx_msg void OnSearch() { phev_->OnSearch(); }
        afx_msg void OnJumpHex() { phev_->OnJumpHex(); }
        afx_msg void OnJumpDec() { phev_->OnJumpDec(); }
        afx_msg void OnSelectLine() { phev_->OnSelectLine(); }

        afx_msg void OnDisplayHex() { phev_->OnDisplayHex(); }
        afx_msg void OnUpdateDisplayHex(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayHex(pCmdUI); }
        afx_msg void OnDisplayChar() { phev_->OnDisplayChar(); }
        afx_msg void OnUpdateDisplayChar(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayChar(pCmdUI); }
        afx_msg void OnDisplayBoth() { phev_->OnDisplayBoth(); }
        afx_msg void OnUpdateDisplayBoth(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayBoth(pCmdUI); }
        afx_msg void OnDisplayStacked() { phev_->OnDisplayStacked(); }
        afx_msg void OnUpdateDisplayStacked(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayStacked(pCmdUI); }
        afx_msg void OnCharsetAscii() { phev_->OnCharsetAscii(); }
        afx_msg void OnUpdateCharsetAscii(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetAscii(pCmdUI); }
        afx_msg void OnCharsetAnsi() { phev_->OnCharsetAnsi(); }
        afx_msg void OnUpdateCharsetAnsi(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetAnsi(pCmdUI); }
        afx_msg void OnCharsetOem() { phev_->OnCharsetOem(); }
        afx_msg void OnUpdateCharsetOem(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetOem(pCmdUI); }
        afx_msg void OnCharsetEbcdic() { phev_->OnCharsetEbcdic(); }
        afx_msg void OnUpdateCharsetEbcdic(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetEbcdic(pCmdUI); }
        afx_msg void OnControlNone() { phev_->OnControlNone(); }
        afx_msg void OnUpdateControlNone(CCmdUI *pCmdUI) { phev_->OnUpdateControlNone(pCmdUI); }
        afx_msg void OnControlAlpha() { phev_->OnControlAlpha(); }
        afx_msg void OnUpdateControlAlpha(CCmdUI *pCmdUI) { phev_->OnUpdateControlAlpha(pCmdUI); }
        afx_msg void OnControlC() { phev_->OnControlC(); }
        afx_msg void OnUpdateControlC(CCmdUI *pCmdUI) { phev_->OnUpdateControlC(pCmdUI); }

        afx_msg void OnChecksum8() { phev_->OnChecksum8(); }
        afx_msg void OnChecksum16() { phev_->OnChecksum16(); }
        afx_msg void OnChecksum32() { phev_->OnChecksum32(); }
        afx_msg void OnChecksum64() { phev_->OnChecksum64(); }
        afx_msg void OnCrcCcitt() { phev_->OnCrcCcitt(); }
//        afx_msg void OnCrc16() { phev_->OnCrc16(); }
        afx_msg void OnCrc32() { phev_->OnCrc32(); }
        afx_msg void OnUpdateByteNZ(CCmdUI *pCmdUI) { phev_->OnUpdateByteNZ(pCmdUI); }
        afx_msg void OnUpdate16bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate16bitNZ(pCmdUI); }
        afx_msg void OnUpdate32bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate32bitNZ(pCmdUI); }
        afx_msg void OnUpdate64bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate64bitNZ(pCmdUI); }

        afx_msg void OnToggleEndian() { phev_->OnToggleEndian(); }
        afx_msg void OnBigEndian() { phev_->OnBigEndian(); }
        afx_msg void OnLittleEndian() { phev_->OnLittleEndian(); }
        afx_msg void OnUpdateToggleEndian(CCmdUI *pCmdUI) { phev_->OnUpdateToggleEndian(pCmdUI); }
        afx_msg void OnUpdateBigEndian(CCmdUI *pCmdUI) { phev_->OnUpdateBigEndian(pCmdUI); }
        afx_msg void OnUpdateLittleEndian(CCmdUI *pCmdUI) { phev_->OnUpdateLittleEndian(pCmdUI); }
        afx_msg void OnUpdateInsertBlock(CCmdUI *pCmdUI) { phev_->OnUpdateInsertBlock(pCmdUI); }
        afx_msg void OnInsertBlock() { phev_->OnInsertBlock(); }

        afx_msg void OnCompress() { phev_->OnCompress(); }
        afx_msg void OnDecompress() { phev_->OnDecompress(); }
        afx_msg void OnUpdateSelNZ(CCmdUI *pCmdUI) { phev_->OnUpdateSelNZ(pCmdUI); }
        afx_msg void OnMd5() { phev_->OnMd5(); }
		afx_msg void OnUppercase() { phev_->OnUppercase(); }
		afx_msg void OnLowercase() { phev_->OnLowercase(); }
		afx_msg void OnDffdHide() { phev_->OnDffdHide(); }
		afx_msg void OnDffdSplit() { phev_->OnDffdSplit(); }
		afx_msg void OnDffdTab() { phev_->OnDffdTab(); }
		afx_msg void OnUpdateDffdHide(CCmdUI* pCmdUI) { phev_->OnUpdateDffdHide(pCmdUI); }
		afx_msg void OnUpdateDffdSplit(CCmdUI* pCmdUI) { phev_->OnUpdateDffdSplit(pCmdUI); }
		afx_msg void OnUpdateDffdTab(CCmdUI* pCmdUI) { phev_->OnUpdateDffdTab(pCmdUI); }

// Implementation
protected:
    virtual ~CDataFormatView();

#if _MSC_VER >= 1300
	bool InitTreeCol(int ii, GV_ITEMW & item); // set up tree column field (image, name etc)
	void InitDataCol(int ii, GV_ITEMW & item); // set up field in data column
#else
	bool InitTreeCol(int ii, GV_ITEM & item);
	void InitDataCol(int ii, GV_ITEM & item);
#endif
	COleDateTime GetDate(int ii); // get date (assumes data row containing date)
    size_t find_address(FILE_ADDRESS addr);
    CString hex_format(size_t count);
    void calc_colours(COLORREF &bg1, COLORREF &bg2, COLORREF &bg3);
    CGridCtrl2 grid_;                       // MFC grid control
    CImageList imagelist_;                  // Needed for tree icons
    CBtnDataBase btn_db_;                   // Needed for button cells (tree)
    CTreeColumn tree_col_;                  // Provides tree column support to MFC grid control
    bool tree_init_;                        // Has the tree column been initialised?
	int default_height_;                    // Default cell height
    int edit_row_type_changed_;             // Row where we have set a new cell type for data column when editing

    std::vector<CString> tree_state_;
    CString              get_full_name(std::vector<int> &curr_state, bool use_comma = false);

    void do_insert(int item, int index, int parent, signed char parent_type);
    void do_enclose(int item, int index, int parent, signed char parent_type);
    void do_edit(int index, signed char parent_type);

#ifdef _DEBUG
    CString get_name(int ii) ;
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CDataFormatView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDestroy();
    afx_msg void OnViewtest();
    afx_msg void OnDffdSync();
    afx_msg void OnUpdateDffdSync(CCmdUI* pCmdUI);
    afx_msg void OnDffdWeb();
    afx_msg void OnUpdateDffdWeb(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CHexEditDoc *GetDocument()
    {
            ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));
            return (CHexEditDoc*)m_pDocument;
    }
};

enum  // Each index into imagelist_
{
    IMAGE_FILE,
    IMAGE_JUMP,
    IMAGE_EVAL,
    IMAGE_IF,
    IMAGE_DATA,
    IMAGE_SWITCH,
    IMAGE_FOR,
    IMAGE_STRUCT,
    IMAGE_DEFINE_STRUCT,

    IMAGE_JUMP_GREY,
    IMAGE_EVAL_GREY,
    IMAGE_IF_GREY,
    IMAGE_DATA_GREY,
    IMAGE_EXTRA,
    IMAGE_FILE_GREY,
    IMAGE_FOR_GREY,
    IMAGE_STRUCT_GREY,
    IMAGE_SWITCH_GREY,

    IMAGE_MORE,
    IMAGE_MORE_GREY,
    IMAGE_NO_TYPE,
    IMAGE_NO_TYPE_GREY,

    IMAGE_DATA_DATE,
    IMAGE_DATA_STRING,
    IMAGE_DATA_REAL,

    IMAGE_LAST   // leave this last
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAFORMATVIEW_H__AA218642_2DFC_4659_94FE_85AD21EDE726__INCLUDED_)
