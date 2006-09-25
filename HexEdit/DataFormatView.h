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
    CHexEditView *phev_;                     // Ptr to sister hex view

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

        void OnAddrToggle() { phev_->OnAddrToggle(); }
        void OnUpdateAddrToggle(CCmdUI *pCmdUI) { phev_->OnUpdateAddrToggle(pCmdUI); }
        void OnGraphicToggle() { phev_->OnGraphicToggle(); }
        void OnUpdateGraphicToggle(CCmdUI *pCmdUI) { phev_->OnUpdateGraphicToggle(pCmdUI); }
        void OnCharToggle() { phev_->OnCharToggle(); }
        void OnUpdateCharToggle(CCmdUI *pCmdUI) { phev_->OnUpdateCharToggle(pCmdUI); }
        void OnFont() { phev_->OnFont(); }
        void OnAutoFit() { phev_->OnAutoFit(); }
        void OnUpdateAutofit(CCmdUI *pCmdUI) { phev_->OnUpdateAutofit(pCmdUI); }
        void OnAscEbc() { phev_->OnAscEbc(); }
        void OnUpdateAscEbc(CCmdUI *pCmdUI) { phev_->OnUpdateAscEbc(pCmdUI); }
        void OnControl() { phev_->OnControl(); }
        void OnUpdateControl(CCmdUI *pCmdUI) { phev_->OnUpdateControl(pCmdUI); }
        void OnDffdAutoSync() { phev_->OnDffdAutoSync(); }
        void OnMark() { phev_->OnMark(); }
        void OnGotoMark() { phev_->OnGotoMark(); }
        void OnEditUndo() { phev_->OnEditUndo(); }
        void OnUpdateEditUndo(CCmdUI *pCmdUI) { phev_->OnUpdateEditUndo(pCmdUI); }
        void OnSearchHex() { phev_->OnSearchHex(); }
        void OnSearchAscii() { phev_->OnSearchAscii(); }
//        void OnSearchForw() { phev_->OnSearchForw(); }
//        void OnSearchBack() { phev_->OnSearchBack(); }
//        void OnSearchSel() { phev_->OnSearchSel(); }
        void OnAllowMods() { phev_->OnAllowMods(); }
        void OnUpdateAllowMods(CCmdUI *pCmdUI) { phev_->OnUpdateAllowMods(pCmdUI); }
        void OnControlToggle() { phev_->OnControlToggle(); }
        void OnInsert() { phev_->OnInsert(); }
        void OnUpdateInsert(CCmdUI *pCmdUI) { phev_->OnUpdateInsert(pCmdUI); }
        void OnUpdateDffdAutoSync(CCmdUI *pCmdUI) { phev_->OnUpdateDffdAutoSync(pCmdUI); }
        void OnSearchIcase() { phev_->OnSearchIcase(); }
//        void OnUpdateSearch(CCmdUI *pCmdUI) { phev_->OnUpdateSearch(pCmdUI); }
//        void OnEditCompare() { phev_->OnEditCompare(); }
//        void OnUpdateEditCompare(CCmdUI *pCmdUI) { phev_->OnUpdateEditCompare(pCmdUI); }
        void OnTrackChanges() { phev_->OnTrackChanges(); }
        void OnUpdateTrackChanges(CCmdUI *pCmdUI) { phev_->OnUpdateTrackChanges(pCmdUI); }

        void OnWindowNext() { phev_->OnWindowNext(); }
        void OnIncByte() { phev_->OnIncByte(); }
        void OnInc16bit() { phev_->OnInc16bit(); }
        void OnInc32bit() { phev_->OnInc32bit(); }
        void OnInc64bit() { phev_->OnInc64bit(); }
        void OnDecByte() { phev_->OnDecByte(); }
        void OnDec16bit() { phev_->OnDec16bit(); }
        void OnDec32bit() { phev_->OnDec32bit(); }
        void OnDec64bit() { phev_->OnDec64bit(); }
        void OnFlip16bit() { phev_->OnFlip16bit(); }
        void OnFlip32bit() { phev_->OnFlip32bit(); }
        void OnFlip64bit() { phev_->OnFlip64bit(); }
        void OnUpdateByte(CCmdUI *pCmdUI) { phev_->OnUpdateByte(pCmdUI); }
        void OnUpdate16bit(CCmdUI *pCmdUI) { phev_->OnUpdate16bit(pCmdUI); }
        void OnUpdate32bit(CCmdUI *pCmdUI) { phev_->OnUpdate32bit(pCmdUI); }
        void OnUpdate64bit(CCmdUI *pCmdUI) { phev_->OnUpdate64bit(pCmdUI); }
        void OnUpdateByteBinary(CCmdUI *pCmdUI) { phev_->OnUpdateByteBinary(pCmdUI); }
        void OnUpdate16bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate16bitBinary(pCmdUI); }
        void OnUpdate32bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate32bitBinary(pCmdUI); }
        void OnUpdate64bitBinary(CCmdUI *pCmdUI) { phev_->OnUpdate64bitBinary(pCmdUI); }
        void OnSelectAll() { phev_->OnSelectAll(); }
//        void OnEditCopy() { phev_->OnEditCopy(); }
//        void OnEditCut() { phev_->OnEditCut(); }
//        void OnEditPaste() { phev_->OnEditPaste(); }
        void OnUpdateTextPaste(CCmdUI *pCmdUI) { phev_->OnUpdateTextPaste(pCmdUI); }
        void OnUpdateClipboard(CCmdUI *pCmdUI) { phev_->OnUpdateClipboard(pCmdUI); }
        void OnUpdateUnicodePaste(CCmdUI *pCmdUI) { phev_->OnUpdateUnicodePaste(pCmdUI); }
        void OnFontDec() { phev_->OnFontDec(); }
        void OnFontInc() { phev_->OnFontInc(); }
        void OnUpdateFontDec(CCmdUI *pCmdUI) { phev_->OnUpdateFontDec(pCmdUI); }
        void OnUpdateFontInc(CCmdUI *pCmdUI) { phev_->OnUpdateFontInc(pCmdUI); }
//        void OnUpdateEditCut(CCmdUI *pCmdUI) { phev_->OnUpdateEditCut(pCmdUI); }
        void OnPasteAscii() { phev_->OnPasteAscii(); }
        void OnPasteEbcdic() { phev_->OnPasteEbcdic(); }
        void OnPasteUnicode() { phev_->OnPasteUnicode(); }
        void OnCopyCchar() { phev_->OnCopyCchar(); }
        void OnCopyHex() { phev_->OnCopyHex(); }
        void OnEditWriteFile() { phev_->OnEditWriteFile(); }
        void OnUpdateReadFile(CCmdUI *pCmdUI) { phev_->OnUpdateReadFile(pCmdUI); }
        void OnReadFile() { phev_->OnReadFile(); }
        void OnExtendToMark() { phev_->OnExtendToMark(); }
        void OnSwapMark() { phev_->OnSwapMark(); }
        void OnRedraw() { phev_->OnRedraw(); }
        void OnScrollDown() { phev_->OnScrollDown(); }
        void OnScrollUp() { phev_->OnScrollUp(); }
        void OnSwap() { phev_->OnSwap(); }
        void OnStartLine() { phev_->OnStartLine(); }
        void OnDel() { phev_->OnDel(); }
        void OnUpdateSwap(CCmdUI *pCmdUI) { phev_->OnUpdateSwap(pCmdUI); }
        void OnOemToggle() { phev_->OnOemToggle(); }
        void OnUpdateOemToggle(CCmdUI *pCmdUI) { phev_->OnUpdateOemToggle(pCmdUI); }
        void OnInvert() { phev_->OnInvert(); }
        void OnNegByte() { phev_->OnNegByte(); }
        void OnNeg16bit() { phev_->OnNeg16bit(); }
        void OnNeg32bit() { phev_->OnNeg32bit(); }
        void OnNeg64bit() { phev_->OnNeg64bit(); }
        void OnHighlight() { phev_->OnHighlight(); }
        void OnUpdateHighlight(CCmdUI *pCmdUI) { phev_->OnUpdateHighlight(pCmdUI); }
        void OnHighlightClear() { phev_->OnHighlightClear(); }
        void OnHighlightPrev() { phev_->OnHighlightPrev(); }
        void OnHighlightNext() { phev_->OnHighlightNext(); }
        void OnUpdateHighlightPrev(CCmdUI *pCmdUI) { phev_->OnUpdateHighlightPrev(pCmdUI); }
        void OnUpdateHighlightNext(CCmdUI *pCmdUI) { phev_->OnUpdateHighlightNext(pCmdUI); }
        void OnEditGoto() { phev_->OnEditGoto(); }
        void OnEditFind() { phev_->OnEditFind(); }
        void OnEditReplace() { phev_->OnEditReplace(); }
    void OnAscii2Ebcdic() { phev_->OnAscii2Ebcdic(); }
    void OnUpdateConvert(CCmdUI *pCmdUI) { phev_->OnUpdateConvert(pCmdUI); }
    void OnEbcdic2Ascii() { phev_->OnEbcdic2Ascii(); }
    void OnAnsi2Ibm() { phev_->OnAnsi2Ibm(); }
    void OnIbm2Ansi() { phev_->OnIbm2Ansi(); }
    void OnEncrypt() { phev_->OnEncrypt(); }
    void OnDecrypt() { phev_->OnDecrypt(); }
    void OnEditAppendFile() { phev_->OnEditAppendFile(); }
    void OnEditAppendSameFile() { phev_->OnEditAppendSameFile(); }
    void OnUpdateEditAppendSameFile(CCmdUI *pCmdUI) { phev_->OnUpdateEditAppendSameFile(pCmdUI); }
    void OnUndoChanges() { phev_->OnUndoChanges(); }
    void OnUpdateUndoChanges(CCmdUI *pCmdUI) { phev_->OnUpdateUndoChanges(pCmdUI); }
    void OnCalcSel() { phev_->OnCalcSel(); }
    void OnUpdateCalcSel(CCmdUI *pCmdUI) { phev_->OnUpdateCalcSel(pCmdUI); }
    void OnDisplayReset() { phev_->OnDisplayReset(); }
    void OnUpdateEncrypt(CCmdUI *pCmdUI) { phev_->OnUpdateEncrypt(pCmdUI); }

        void OnXorByte() { phev_->OnXorByte(); }
        void OnXor16bit() { phev_->OnXor16bit(); }
        void OnXor32bit() { phev_->OnXor32bit(); }
        void OnXor64bit() { phev_->OnXor64bit(); }
        void OnAssignByte() { phev_->OnAssignByte(); }
        void OnAssign16bit() { phev_->OnAssign16bit(); }
        void OnAssign32bit() { phev_->OnAssign32bit(); }
        void OnAssign64bit() { phev_->OnAssign64bit(); }
        void OnRandByte() { phev_->OnRandByte(); }
        void OnRandFast() { phev_->OnRandFast(); }
//        void OnRand16bit() { phev_->OnRand16bit(); }
//        void OnRand32bit() { phev_->OnRand32bit(); }
//        void OnRand64bit() { phev_->OnRand64bit(); }
        void OnAddByte() { phev_->OnAddByte(); }
        void OnAdd16bit() { phev_->OnAdd16bit(); }
        void OnAdd32bit() { phev_->OnAdd32bit(); }
        void OnAdd64bit() { phev_->OnAdd64bit(); }
        void OnSubtractByte() { phev_->OnSubtractByte(); }
        void OnSubtract16bit() { phev_->OnSubtract16bit(); }
        void OnSubtract32bit() { phev_->OnSubtract32bit(); }
        void OnSubtract64bit() { phev_->OnSubtract64bit(); }
        void OnAndByte() { phev_->OnAndByte(); }
        void OnAnd16bit() { phev_->OnAnd16bit(); }
        void OnAnd32bit() { phev_->OnAnd32bit(); }
        void OnAnd64bit() { phev_->OnAnd64bit(); }
        void OnOrByte() { phev_->OnOrByte(); }
        void OnOr16bit() { phev_->OnOr16bit(); }
        void OnOr32bit() { phev_->OnOr32bit(); }
        void OnOr64bit() { phev_->OnOr64bit(); }

        void OnMulByte() { phev_->OnMulByte(); }
        void OnMul16bit() { phev_->OnMul16bit(); }
        void OnMul32bit() { phev_->OnMul32bit(); }
        void OnMul64bit() { phev_->OnMul64bit(); }
        void OnDivByte() { phev_->OnDivByte(); }
        void OnDiv16bit() { phev_->OnDiv16bit(); }
        void OnDiv32bit() { phev_->OnDiv32bit(); }
        void OnDiv64bit() { phev_->OnDiv64bit(); }
        void OnModByte() { phev_->OnModByte(); }
        void OnMod16bit() { phev_->OnMod16bit(); }
        void OnMod32bit() { phev_->OnMod32bit(); }
        void OnMod64bit() { phev_->OnMod64bit(); }
        void OnRevByte() { phev_->OnRevByte(); }
        void OnRev16bit() { phev_->OnRev16bit(); }
        void OnRev32bit() { phev_->OnRev32bit(); }
        void OnRev64bit() { phev_->OnRev64bit(); }
        void OnSubtractXByte() { phev_->OnSubtractXByte(); }
        void OnSubtractX16bit() { phev_->OnSubtractX16bit(); }
        void OnSubtractX32bit() { phev_->OnSubtractX32bit(); }
        void OnSubtractX64bit() { phev_->OnSubtractX64bit(); }
        void OnDivXByte() { phev_->OnDivXByte(); }
        void OnDivX16bit() { phev_->OnDivX16bit(); }
        void OnDivX32bit() { phev_->OnDivX32bit(); }
        void OnDivX64bit() { phev_->OnDivX64bit(); }
        void OnModXByte() { phev_->OnModXByte(); }
        void OnModX16bit() { phev_->OnModX16bit(); }
        void OnModX32bit() { phev_->OnModX32bit(); }
        void OnModX64bit() { phev_->OnModX64bit(); }
        void OnGtrByte() { phev_->OnGtrByte(); }
        void OnGtr16bit() { phev_->OnGtr16bit(); }
        void OnGtr32bit() { phev_->OnGtr32bit(); }
        void OnGtr64bit() { phev_->OnGtr64bit(); }
        void OnLessByte() { phev_->OnLessByte(); }
        void OnLess16bit() { phev_->OnLess16bit(); }
        void OnLess32bit() { phev_->OnLess32bit(); }
        void OnLess64bit() { phev_->OnLess64bit(); }

        void OnGtrUByte() { phev_->OnGtrUByte(); }
        void OnGtrU16bit() { phev_->OnGtrU16bit(); }
        void OnGtrU32bit() { phev_->OnGtrU32bit(); }
        void OnGtrU64bit() { phev_->OnGtrU64bit(); }
        void OnLessUByte() { phev_->OnLessUByte(); }
        void OnLessU16bit() { phev_->OnLessU16bit(); }
        void OnLessU32bit() { phev_->OnLessU32bit(); }
        void OnLessU64bit() { phev_->OnLessU64bit(); }

        void OnRolByte()  { phev_->OnRolByte(); }
        void OnRol16bit() { phev_->OnRol16bit(); }
        void OnRol32bit() { phev_->OnRol32bit(); }
        void OnRol64bit() { phev_->OnRol64bit(); }
        void OnRorByte()  { phev_->OnRorByte(); }
        void OnRor16bit() { phev_->OnRor16bit(); }
        void OnRor32bit() { phev_->OnRor32bit(); }
        void OnRor64bit() { phev_->OnRor64bit(); }
        void OnLslByte()  { phev_->OnLslByte(); }
        void OnLsl16bit() { phev_->OnLsl16bit(); }
        void OnLsl32bit() { phev_->OnLsl32bit(); }
        void OnLsl64bit() { phev_->OnLsl64bit(); }
        void OnLsrByte()  { phev_->OnLsrByte(); }
        void OnLsr16bit() { phev_->OnLsr16bit(); }
        void OnLsr32bit() { phev_->OnLsr32bit(); }
        void OnLsr64bit() { phev_->OnLsr64bit(); }
        void OnAsrByte()  { phev_->OnAsrByte(); }
        void OnAsr16bit() { phev_->OnAsr16bit(); }
        void OnAsr32bit() { phev_->OnAsr32bit(); }
        void OnAsr64bit() { phev_->OnAsr64bit(); }

        void OnJumpHexAddr() { phev_->OnJumpHexAddr(); }
//        void OnSearch() { phev_->OnSearch(); }
        void OnJumpHex() { phev_->OnJumpHex(); }
        void OnJumpDec() { phev_->OnJumpDec(); }
        void OnSelectLine() { phev_->OnSelectLine(); }

        void OnDisplayHex() { phev_->OnDisplayHex(); }
        void OnUpdateDisplayHex(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayHex(pCmdUI); }
        void OnDisplayChar() { phev_->OnDisplayChar(); }
        void OnUpdateDisplayChar(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayChar(pCmdUI); }
        void OnDisplayBoth() { phev_->OnDisplayBoth(); }
        void OnUpdateDisplayBoth(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayBoth(pCmdUI); }
        void OnDisplayStacked() { phev_->OnDisplayStacked(); }
        void OnUpdateDisplayStacked(CCmdUI *pCmdUI) { phev_->OnUpdateDisplayStacked(pCmdUI); }
        void OnCharsetAscii() { phev_->OnCharsetAscii(); }
        void OnUpdateCharsetAscii(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetAscii(pCmdUI); }
        void OnCharsetAnsi() { phev_->OnCharsetAnsi(); }
        void OnUpdateCharsetAnsi(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetAnsi(pCmdUI); }
        void OnCharsetOem() { phev_->OnCharsetOem(); }
        void OnUpdateCharsetOem(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetOem(pCmdUI); }
        void OnCharsetEbcdic() { phev_->OnCharsetEbcdic(); }
        void OnUpdateCharsetEbcdic(CCmdUI *pCmdUI) { phev_->OnUpdateCharsetEbcdic(pCmdUI); }
        void OnControlNone() { phev_->OnControlNone(); }
        void OnUpdateControlNone(CCmdUI *pCmdUI) { phev_->OnUpdateControlNone(pCmdUI); }
        void OnControlAlpha() { phev_->OnControlAlpha(); }
        void OnUpdateControlAlpha(CCmdUI *pCmdUI) { phev_->OnUpdateControlAlpha(pCmdUI); }
        void OnControlC() { phev_->OnControlC(); }
        void OnUpdateControlC(CCmdUI *pCmdUI) { phev_->OnUpdateControlC(pCmdUI); }

        void OnChecksum8() { phev_->OnChecksum8(); }
        void OnChecksum16() { phev_->OnChecksum16(); }
        void OnChecksum32() { phev_->OnChecksum32(); }
        void OnChecksum64() { phev_->OnChecksum64(); }
        void OnCrcCcitt() { phev_->OnCrcCcitt(); }
//        void OnCrc16() { phev_->OnCrc16(); }
        void OnCrc32() { phev_->OnCrc32(); }
        void OnUpdateByteNZ(CCmdUI *pCmdUI) { phev_->OnUpdateByteNZ(pCmdUI); }
        void OnUpdate16bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate16bitNZ(pCmdUI); }
        void OnUpdate32bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate32bitNZ(pCmdUI); }
        void OnUpdate64bitNZ(CCmdUI *pCmdUI) { phev_->OnUpdate64bitNZ(pCmdUI); }

        void OnToggleEndian() { phev_->OnToggleEndian(); }
        void OnBigEndian() { phev_->OnBigEndian(); }
        void OnLittleEndian() { phev_->OnLittleEndian(); }
        void OnUpdateToggleEndian(CCmdUI *pCmdUI) { phev_->OnUpdateToggleEndian(pCmdUI); }
        void OnUpdateBigEndian(CCmdUI *pCmdUI) { phev_->OnUpdateBigEndian(pCmdUI); }
        void OnUpdateLittleEndian(CCmdUI *pCmdUI) { phev_->OnUpdateLittleEndian(pCmdUI); }
        void OnUpdateInsertBlock(CCmdUI *pCmdUI) { phev_->OnUpdateInsertBlock(pCmdUI); }
        void OnInsertBlock() { phev_->OnInsertBlock(); }

        void OnCompress() { phev_->OnCompress(); }
        void OnDecompress() { phev_->OnDecompress(); }
        void OnUpdateSelNZ(CCmdUI *pCmdUI) { phev_->OnUpdateSelNZ(pCmdUI); }
        void OnMd5() { phev_->OnMd5(); }

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
