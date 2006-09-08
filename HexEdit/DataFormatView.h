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
    CHexEditView *psis_;                     // Ptr to sister hex view

// Operations
public:
    void InitColumnHeadings();
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

        void OnAddrToggle() { psis_->OnAddrToggle(); }
        void OnUpdateAddrToggle(CCmdUI *pCmdUI) { psis_->OnUpdateAddrToggle(pCmdUI); }
        void OnGraphicToggle() { psis_->OnGraphicToggle(); }
        void OnUpdateGraphicToggle(CCmdUI *pCmdUI) { psis_->OnUpdateGraphicToggle(pCmdUI); }
        void OnCharToggle() { psis_->OnCharToggle(); }
        void OnUpdateCharToggle(CCmdUI *pCmdUI) { psis_->OnUpdateCharToggle(pCmdUI); }
        void OnFont() { psis_->OnFont(); }
        void OnAutoFit() { psis_->OnAutoFit(); }
        void OnUpdateAutofit(CCmdUI *pCmdUI) { psis_->OnUpdateAutofit(pCmdUI); }
        void OnAscEbc() { psis_->OnAscEbc(); }
        void OnUpdateAscEbc(CCmdUI *pCmdUI) { psis_->OnUpdateAscEbc(pCmdUI); }
        void OnControl() { psis_->OnControl(); }
        void OnUpdateControl(CCmdUI *pCmdUI) { psis_->OnUpdateControl(pCmdUI); }
        void OnDffdAutoSync() { psis_->OnDffdAutoSync(); }
        void OnMark() { psis_->OnMark(); }
        void OnGotoMark() { psis_->OnGotoMark(); }
        void OnEditUndo() { psis_->OnEditUndo(); }
        void OnUpdateEditUndo(CCmdUI *pCmdUI) { psis_->OnUpdateEditUndo(pCmdUI); }
        void OnSearchHex() { psis_->OnSearchHex(); }
        void OnSearchAscii() { psis_->OnSearchAscii(); }
//        void OnSearchForw() { psis_->OnSearchForw(); }
//        void OnSearchBack() { psis_->OnSearchBack(); }
//        void OnSearchSel() { psis_->OnSearchSel(); }
        void OnAllowMods() { psis_->OnAllowMods(); }
        void OnUpdateAllowMods(CCmdUI *pCmdUI) { psis_->OnUpdateAllowMods(pCmdUI); }
        void OnControlToggle() { psis_->OnControlToggle(); }
        void OnInsert() { psis_->OnInsert(); }
        void OnUpdateInsert(CCmdUI *pCmdUI) { psis_->OnUpdateInsert(pCmdUI); }
        void OnUpdateDffdAutoSync(CCmdUI *pCmdUI) { psis_->OnUpdateDffdAutoSync(pCmdUI); }
        void OnSearchIcase() { psis_->OnSearchIcase(); }
//        void OnUpdateSearch(CCmdUI *pCmdUI) { psis_->OnUpdateSearch(pCmdUI); }
//        void OnEditCompare() { psis_->OnEditCompare(); }
//        void OnUpdateEditCompare(CCmdUI *pCmdUI) { psis_->OnUpdateEditCompare(pCmdUI); }
        void OnTrackChanges() { psis_->OnTrackChanges(); }
        void OnUpdateTrackChanges(CCmdUI *pCmdUI) { psis_->OnUpdateTrackChanges(pCmdUI); }

        void OnWindowNext() { psis_->OnWindowNext(); }
        void OnIncByte() { psis_->OnIncByte(); }
        void OnInc16bit() { psis_->OnInc16bit(); }
        void OnInc32bit() { psis_->OnInc32bit(); }
        void OnInc64bit() { psis_->OnInc64bit(); }
        void OnDecByte() { psis_->OnDecByte(); }
        void OnDec16bit() { psis_->OnDec16bit(); }
        void OnDec32bit() { psis_->OnDec32bit(); }
        void OnDec64bit() { psis_->OnDec64bit(); }
        void OnFlip16bit() { psis_->OnFlip16bit(); }
        void OnFlip32bit() { psis_->OnFlip32bit(); }
        void OnFlip64bit() { psis_->OnFlip64bit(); }
        void OnUpdateByte(CCmdUI *pCmdUI) { psis_->OnUpdateByte(pCmdUI); }
        void OnUpdate16bit(CCmdUI *pCmdUI) { psis_->OnUpdate16bit(pCmdUI); }
        void OnUpdate32bit(CCmdUI *pCmdUI) { psis_->OnUpdate32bit(pCmdUI); }
        void OnUpdate64bit(CCmdUI *pCmdUI) { psis_->OnUpdate64bit(pCmdUI); }
        void OnUpdateByteBinary(CCmdUI *pCmdUI) { psis_->OnUpdateByteBinary(pCmdUI); }
        void OnUpdate16bitBinary(CCmdUI *pCmdUI) { psis_->OnUpdate16bitBinary(pCmdUI); }
        void OnUpdate32bitBinary(CCmdUI *pCmdUI) { psis_->OnUpdate32bitBinary(pCmdUI); }
        void OnUpdate64bitBinary(CCmdUI *pCmdUI) { psis_->OnUpdate64bitBinary(pCmdUI); }
        void OnSelectAll() { psis_->OnSelectAll(); }
//        void OnEditCopy() { psis_->OnEditCopy(); }
//        void OnEditCut() { psis_->OnEditCut(); }
//        void OnEditPaste() { psis_->OnEditPaste(); }
        void OnUpdateTextPaste(CCmdUI *pCmdUI) { psis_->OnUpdateTextPaste(pCmdUI); }
        void OnUpdateClipboard(CCmdUI *pCmdUI) { psis_->OnUpdateClipboard(pCmdUI); }
        void OnUpdateUnicodePaste(CCmdUI *pCmdUI) { psis_->OnUpdateUnicodePaste(pCmdUI); }
        void OnFontDec() { psis_->OnFontDec(); }
        void OnFontInc() { psis_->OnFontInc(); }
        void OnUpdateFontDec(CCmdUI *pCmdUI) { psis_->OnUpdateFontDec(pCmdUI); }
        void OnUpdateFontInc(CCmdUI *pCmdUI) { psis_->OnUpdateFontInc(pCmdUI); }
//        void OnUpdateEditCut(CCmdUI *pCmdUI) { psis_->OnUpdateEditCut(pCmdUI); }
        void OnPasteAscii() { psis_->OnPasteAscii(); }
        void OnPasteEbcdic() { psis_->OnPasteEbcdic(); }
        void OnPasteUnicode() { psis_->OnPasteUnicode(); }
        void OnCopyCchar() { psis_->OnCopyCchar(); }
        void OnCopyHex() { psis_->OnCopyHex(); }
        void OnEditWriteFile() { psis_->OnEditWriteFile(); }
        void OnUpdateReadFile(CCmdUI *pCmdUI) { psis_->OnUpdateReadFile(pCmdUI); }
        void OnReadFile() { psis_->OnReadFile(); }
        void OnExtendToMark() { psis_->OnExtendToMark(); }
        void OnSwapMark() { psis_->OnSwapMark(); }
        void OnRedraw() { psis_->OnRedraw(); }
        void OnScrollDown() { psis_->OnScrollDown(); }
        void OnScrollUp() { psis_->OnScrollUp(); }
        void OnSwap() { psis_->OnSwap(); }
        void OnStartLine() { psis_->OnStartLine(); }
        void OnDel() { psis_->OnDel(); }
        void OnUpdateSwap(CCmdUI *pCmdUI) { psis_->OnUpdateSwap(pCmdUI); }
        void OnOemToggle() { psis_->OnOemToggle(); }
        void OnUpdateOemToggle(CCmdUI *pCmdUI) { psis_->OnUpdateOemToggle(pCmdUI); }
        void OnInvert() { psis_->OnInvert(); }
        void OnNegByte() { psis_->OnNegByte(); }
        void OnNeg16bit() { psis_->OnNeg16bit(); }
        void OnNeg32bit() { psis_->OnNeg32bit(); }
        void OnNeg64bit() { psis_->OnNeg64bit(); }
        void OnHighlight() { psis_->OnHighlight(); }
        void OnUpdateHighlight(CCmdUI *pCmdUI) { psis_->OnUpdateHighlight(pCmdUI); }
        void OnHighlightClear() { psis_->OnHighlightClear(); }
        void OnHighlightPrev() { psis_->OnHighlightPrev(); }
        void OnHighlightNext() { psis_->OnHighlightNext(); }
        void OnUpdateHighlightPrev(CCmdUI *pCmdUI) { psis_->OnUpdateHighlightPrev(pCmdUI); }
        void OnUpdateHighlightNext(CCmdUI *pCmdUI) { psis_->OnUpdateHighlightNext(pCmdUI); }
        void OnEditGoto() { psis_->OnEditGoto(); }
        void OnEditFind() { psis_->OnEditFind(); }
        void OnEditReplace() { psis_->OnEditReplace(); }
    void OnAscii2Ebcdic() { psis_->OnAscii2Ebcdic(); }
    void OnUpdateConvert(CCmdUI *pCmdUI) { psis_->OnUpdateConvert(pCmdUI); }
    void OnEbcdic2Ascii() { psis_->OnEbcdic2Ascii(); }
    void OnAnsi2Ibm() { psis_->OnAnsi2Ibm(); }
    void OnIbm2Ansi() { psis_->OnIbm2Ansi(); }
    void OnEncrypt() { psis_->OnEncrypt(); }
    void OnDecrypt() { psis_->OnDecrypt(); }
    void OnEditAppendFile() { psis_->OnEditAppendFile(); }
    void OnEditAppendSameFile() { psis_->OnEditAppendSameFile(); }
    void OnUpdateEditAppendSameFile(CCmdUI *pCmdUI) { psis_->OnUpdateEditAppendSameFile(pCmdUI); }
    void OnUndoChanges() { psis_->OnUndoChanges(); }
    void OnUpdateUndoChanges(CCmdUI *pCmdUI) { psis_->OnUpdateUndoChanges(pCmdUI); }
    void OnCalcSel() { psis_->OnCalcSel(); }
    void OnUpdateCalcSel(CCmdUI *pCmdUI) { psis_->OnUpdateCalcSel(pCmdUI); }
    void OnDisplayReset() { psis_->OnDisplayReset(); }
    void OnUpdateEncrypt(CCmdUI *pCmdUI) { psis_->OnUpdateEncrypt(pCmdUI); }

        void OnXorByte() { psis_->OnXorByte(); }
        void OnXor16bit() { psis_->OnXor16bit(); }
        void OnXor32bit() { psis_->OnXor32bit(); }
        void OnXor64bit() { psis_->OnXor64bit(); }
        void OnAssignByte() { psis_->OnAssignByte(); }
        void OnAssign16bit() { psis_->OnAssign16bit(); }
        void OnAssign32bit() { psis_->OnAssign32bit(); }
        void OnAssign64bit() { psis_->OnAssign64bit(); }
        void OnRandByte() { psis_->OnRandByte(); }
        void OnRandFast() { psis_->OnRandFast(); }
//        void OnRand16bit() { psis_->OnRand16bit(); }
//        void OnRand32bit() { psis_->OnRand32bit(); }
//        void OnRand64bit() { psis_->OnRand64bit(); }
        void OnAddByte() { psis_->OnAddByte(); }
        void OnAdd16bit() { psis_->OnAdd16bit(); }
        void OnAdd32bit() { psis_->OnAdd32bit(); }
        void OnAdd64bit() { psis_->OnAdd64bit(); }
        void OnSubtractByte() { psis_->OnSubtractByte(); }
        void OnSubtract16bit() { psis_->OnSubtract16bit(); }
        void OnSubtract32bit() { psis_->OnSubtract32bit(); }
        void OnSubtract64bit() { psis_->OnSubtract64bit(); }
        void OnAndByte() { psis_->OnAndByte(); }
        void OnAnd16bit() { psis_->OnAnd16bit(); }
        void OnAnd32bit() { psis_->OnAnd32bit(); }
        void OnAnd64bit() { psis_->OnAnd64bit(); }
        void OnOrByte() { psis_->OnOrByte(); }
        void OnOr16bit() { psis_->OnOr16bit(); }
        void OnOr32bit() { psis_->OnOr32bit(); }
        void OnOr64bit() { psis_->OnOr64bit(); }

        void OnMulByte() { psis_->OnMulByte(); }
        void OnMul16bit() { psis_->OnMul16bit(); }
        void OnMul32bit() { psis_->OnMul32bit(); }
        void OnMul64bit() { psis_->OnMul64bit(); }
        void OnDivByte() { psis_->OnDivByte(); }
        void OnDiv16bit() { psis_->OnDiv16bit(); }
        void OnDiv32bit() { psis_->OnDiv32bit(); }
        void OnDiv64bit() { psis_->OnDiv64bit(); }
        void OnModByte() { psis_->OnModByte(); }
        void OnMod16bit() { psis_->OnMod16bit(); }
        void OnMod32bit() { psis_->OnMod32bit(); }
        void OnMod64bit() { psis_->OnMod64bit(); }
        void OnRevByte() { psis_->OnRevByte(); }
        void OnRev16bit() { psis_->OnRev16bit(); }
        void OnRev32bit() { psis_->OnRev32bit(); }
        void OnRev64bit() { psis_->OnRev64bit(); }
        void OnSubtractXByte() { psis_->OnSubtractXByte(); }
        void OnSubtractX16bit() { psis_->OnSubtractX16bit(); }
        void OnSubtractX32bit() { psis_->OnSubtractX32bit(); }
        void OnSubtractX64bit() { psis_->OnSubtractX64bit(); }
        void OnDivXByte() { psis_->OnDivXByte(); }
        void OnDivX16bit() { psis_->OnDivX16bit(); }
        void OnDivX32bit() { psis_->OnDivX32bit(); }
        void OnDivX64bit() { psis_->OnDivX64bit(); }
        void OnModXByte() { psis_->OnModXByte(); }
        void OnModX16bit() { psis_->OnModX16bit(); }
        void OnModX32bit() { psis_->OnModX32bit(); }
        void OnModX64bit() { psis_->OnModX64bit(); }
        void OnGtrByte() { psis_->OnGtrByte(); }
        void OnGtr16bit() { psis_->OnGtr16bit(); }
        void OnGtr32bit() { psis_->OnGtr32bit(); }
        void OnGtr64bit() { psis_->OnGtr64bit(); }
        void OnLessByte() { psis_->OnLessByte(); }
        void OnLess16bit() { psis_->OnLess16bit(); }
        void OnLess32bit() { psis_->OnLess32bit(); }
        void OnLess64bit() { psis_->OnLess64bit(); }

        void OnGtrUByte() { psis_->OnGtrUByte(); }
        void OnGtrU16bit() { psis_->OnGtrU16bit(); }
        void OnGtrU32bit() { psis_->OnGtrU32bit(); }
        void OnGtrU64bit() { psis_->OnGtrU64bit(); }
        void OnLessUByte() { psis_->OnLessUByte(); }
        void OnLessU16bit() { psis_->OnLessU16bit(); }
        void OnLessU32bit() { psis_->OnLessU32bit(); }
        void OnLessU64bit() { psis_->OnLessU64bit(); }

        void OnRolByte()  { psis_->OnRolByte(); }
        void OnRol16bit() { psis_->OnRol16bit(); }
        void OnRol32bit() { psis_->OnRol32bit(); }
        void OnRol64bit() { psis_->OnRol64bit(); }
        void OnRorByte()  { psis_->OnRorByte(); }
        void OnRor16bit() { psis_->OnRor16bit(); }
        void OnRor32bit() { psis_->OnRor32bit(); }
        void OnRor64bit() { psis_->OnRor64bit(); }
        void OnLslByte()  { psis_->OnLslByte(); }
        void OnLsl16bit() { psis_->OnLsl16bit(); }
        void OnLsl32bit() { psis_->OnLsl32bit(); }
        void OnLsl64bit() { psis_->OnLsl64bit(); }
        void OnLsrByte()  { psis_->OnLsrByte(); }
        void OnLsr16bit() { psis_->OnLsr16bit(); }
        void OnLsr32bit() { psis_->OnLsr32bit(); }
        void OnLsr64bit() { psis_->OnLsr64bit(); }
        void OnAsrByte()  { psis_->OnAsrByte(); }
        void OnAsr16bit() { psis_->OnAsr16bit(); }
        void OnAsr32bit() { psis_->OnAsr32bit(); }
        void OnAsr64bit() { psis_->OnAsr64bit(); }

        void OnJumpHexAddr() { psis_->OnJumpHexAddr(); }
//        void OnSearch() { psis_->OnSearch(); }
        void OnJumpHex() { psis_->OnJumpHex(); }
        void OnJumpDec() { psis_->OnJumpDec(); }
        void OnSelectLine() { psis_->OnSelectLine(); }

        void OnDisplayHex() { psis_->OnDisplayHex(); }
        void OnUpdateDisplayHex(CCmdUI *pCmdUI) { psis_->OnUpdateDisplayHex(pCmdUI); }
        void OnDisplayChar() { psis_->OnDisplayChar(); }
        void OnUpdateDisplayChar(CCmdUI *pCmdUI) { psis_->OnUpdateDisplayChar(pCmdUI); }
        void OnDisplayBoth() { psis_->OnDisplayBoth(); }
        void OnUpdateDisplayBoth(CCmdUI *pCmdUI) { psis_->OnUpdateDisplayBoth(pCmdUI); }
        void OnDisplayStacked() { psis_->OnDisplayStacked(); }
        void OnUpdateDisplayStacked(CCmdUI *pCmdUI) { psis_->OnUpdateDisplayStacked(pCmdUI); }
        void OnCharsetAscii() { psis_->OnCharsetAscii(); }
        void OnUpdateCharsetAscii(CCmdUI *pCmdUI) { psis_->OnUpdateCharsetAscii(pCmdUI); }
        void OnCharsetAnsi() { psis_->OnCharsetAnsi(); }
        void OnUpdateCharsetAnsi(CCmdUI *pCmdUI) { psis_->OnUpdateCharsetAnsi(pCmdUI); }
        void OnCharsetOem() { psis_->OnCharsetOem(); }
        void OnUpdateCharsetOem(CCmdUI *pCmdUI) { psis_->OnUpdateCharsetOem(pCmdUI); }
        void OnCharsetEbcdic() { psis_->OnCharsetEbcdic(); }
        void OnUpdateCharsetEbcdic(CCmdUI *pCmdUI) { psis_->OnUpdateCharsetEbcdic(pCmdUI); }
        void OnControlNone() { psis_->OnControlNone(); }
        void OnUpdateControlNone(CCmdUI *pCmdUI) { psis_->OnUpdateControlNone(pCmdUI); }
        void OnControlAlpha() { psis_->OnControlAlpha(); }
        void OnUpdateControlAlpha(CCmdUI *pCmdUI) { psis_->OnUpdateControlAlpha(pCmdUI); }
        void OnControlC() { psis_->OnControlC(); }
        void OnUpdateControlC(CCmdUI *pCmdUI) { psis_->OnUpdateControlC(pCmdUI); }

        void OnChecksum8() { psis_->OnChecksum8(); }
        void OnChecksum16() { psis_->OnChecksum16(); }
        void OnChecksum32() { psis_->OnChecksum32(); }
        void OnChecksum64() { psis_->OnChecksum64(); }
        void OnCrcCcitt() { psis_->OnCrcCcitt(); }
//        void OnCrc16() { psis_->OnCrc16(); }
        void OnCrc32() { psis_->OnCrc32(); }
        void OnUpdateByteNZ(CCmdUI *pCmdUI) { psis_->OnUpdateByteNZ(pCmdUI); }
        void OnUpdate16bitNZ(CCmdUI *pCmdUI) { psis_->OnUpdate16bitNZ(pCmdUI); }
        void OnUpdate32bitNZ(CCmdUI *pCmdUI) { psis_->OnUpdate32bitNZ(pCmdUI); }
        void OnUpdate64bitNZ(CCmdUI *pCmdUI) { psis_->OnUpdate64bitNZ(pCmdUI); }

        void OnToggleEndian() { psis_->OnToggleEndian(); }
        void OnBigEndian() { psis_->OnBigEndian(); }
        void OnLittleEndian() { psis_->OnLittleEndian(); }
        void OnUpdateToggleEndian(CCmdUI *pCmdUI) { psis_->OnUpdateToggleEndian(pCmdUI); }
        void OnUpdateBigEndian(CCmdUI *pCmdUI) { psis_->OnUpdateBigEndian(pCmdUI); }
        void OnUpdateLittleEndian(CCmdUI *pCmdUI) { psis_->OnUpdateLittleEndian(pCmdUI); }
        void OnUpdateInsertBlock(CCmdUI *pCmdUI) { psis_->OnUpdateInsertBlock(pCmdUI); }
        void OnInsertBlock() { psis_->OnInsertBlock(); }

        void OnCompress() { psis_->OnCompress(); }
        void OnDecompress() { psis_->OnDecompress(); }
        void OnUpdateSelNZ(CCmdUI *pCmdUI) { psis_->OnUpdateSelNZ(pCmdUI); }
        void OnMd5() { psis_->OnMd5(); }

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
