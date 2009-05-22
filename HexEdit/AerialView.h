#ifndef DATAFORMATVIEW_INCLUDED_ 
#define DATAFORMATVIEW_INCLUDED_  1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CAerialView view
#include "HexEditDoc.h"
#include "HexEditView.h"
#include "timer.h"  // xxx for testing

class CAerialView : public CView
{
    friend CHexEditView;
	DECLARE_DYNCREATE(CAerialView)

protected:
	CAerialView();           // protected constructor used by dynamic creation
	virtual ~CAerialView();

// Attributes
public:
    CHexEditView *phev_;                     // Ptr to sister hex view

public:
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view

    void SetScroll(int newpos);
    void SetScroll(FILE_ADDRESS newpos) { SetScroll(int(newpos/(GetDocument()->GetBpe() * cols_))); }
    void ShowPos(FILE_ADDRESS pos);
    void StoreOptions(CHexFileList *pfl, int idx);
    void InvalidateRange(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr);

    void StartTimer() { if (!timer_id_) VERIFY(timer_id_ = SetTimer(1, timer_msecs_, NULL)); }
    void StopTimer() { if (timer_id_) KillTimer(timer_id_); timer_id_ = 0; }

protected:
    virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);

    // message handlers
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDestroy();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg LRESULT OnMouseHover(WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnMouseLeave(WPARAM wp, LPARAM lp);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnViewTest();

    afx_msg void OnIndSelection() { disp_.draw_bdr_sel = !disp_.draw_bdr_sel; Invalidate(); }
    afx_msg void OnIndMark() { disp_.draw_bdr_mark = !disp_.draw_bdr_mark; Invalidate(); }
    afx_msg void OnIndHighlights() { disp_.draw_bdr_hl = !disp_.draw_bdr_hl; Invalidate(); }
    afx_msg void OnIndSearch() { disp_.draw_bdr_search = !disp_.draw_bdr_search; Invalidate(); }
    afx_msg void OnIndBookmarks() { disp_.draw_bdr_bm = !disp_.draw_bdr_bm; Invalidate(); }
    afx_msg void OnAntSelection() { disp_.draw_ants_sel = !disp_.draw_ants_sel; Invalidate(); }
    afx_msg void OnAntMark() { disp_.draw_ants_mark = !disp_.draw_ants_mark; Invalidate(); }
    afx_msg void OnAntHighlights() { disp_.draw_ants_hl = !disp_.draw_ants_hl; Invalidate(); }
    afx_msg void OnAntSearch() { disp_.draw_ants_search = !disp_.draw_ants_search; Invalidate(); }
    afx_msg void OnAntBookmarks() { disp_.draw_ants_bm = !disp_.draw_ants_bm; Invalidate(); }
    afx_msg void OnUpdateIndSelection(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_sel); }
    afx_msg void OnUpdateIndMark(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_mark); }
    afx_msg void OnUpdateIndHighlights(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_hl); }
    afx_msg void OnUpdateIndSearch(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_search); }
    afx_msg void OnUpdateIndBookmarks(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_bdr_bm); }
    afx_msg void OnUpdateAntSelection(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_sel); }
    afx_msg void OnUpdateAntMark(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_mark); }
    afx_msg void OnUpdateAntHighlights(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_hl); }
    afx_msg void OnUpdateAntSearch(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_search); }
    afx_msg void OnUpdateAntBookmarks(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.draw_ants_bm); }
    afx_msg void OnZoom1() { set_zoom(1); }
    afx_msg void OnZoom2() { set_zoom(2); }
    afx_msg void OnZoom3() { set_zoom(3); }
    afx_msg void OnZoom4() { set_zoom(4); }
    afx_msg void OnZoom5() { set_zoom(5); }
    afx_msg void OnZoom6() { set_zoom(6); }
    afx_msg void OnZoom7() { set_zoom(7); }
    afx_msg void OnZoom8() { set_zoom(8); }
    afx_msg void OnZoom9() { set_zoom(9); }
    afx_msg void OnZoom10() { set_zoom(10); }
    afx_msg void OnZoom11() { set_zoom(11); }
    afx_msg void OnZoom12() { set_zoom(12); }
    afx_msg void OnZoom13() { set_zoom(13); }
    afx_msg void OnZoom14() { set_zoom(14); }
    afx_msg void OnZoom15() { set_zoom(15); }
    afx_msg void OnZoom16() { set_zoom(16); }
    afx_msg void OnUpdateZoom1(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 1); }
    afx_msg void OnUpdateZoom2(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 2); }
    afx_msg void OnUpdateZoom3(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 3); }
    afx_msg void OnUpdateZoom4(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 4); }
    afx_msg void OnUpdateZoom5(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 5); }
    afx_msg void OnUpdateZoom6(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 6); }
    afx_msg void OnUpdateZoom7(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 7); }
    afx_msg void OnUpdateZoom8(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 8); }
    afx_msg void OnUpdateZoom9(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 9); }
    afx_msg void OnUpdateZoom10(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 10); }
    afx_msg void OnUpdateZoom11(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 11); }
    afx_msg void OnUpdateZoom12(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 12); }
    afx_msg void OnUpdateZoom13(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 13); }
    afx_msg void OnUpdateZoom14(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 14); }
    afx_msg void OnUpdateZoom15(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 15); }
    afx_msg void OnUpdateZoom16(CCmdUI *pCmdUI) { pCmdUI->SetCheck(disp_.dpix == 16); }

    // Commands handled by sister view
    afx_msg void OnAddrToggle() { phev_->OnAddrToggle(); }
    afx_msg void OnUpdateAddrToggle(CCmdUI *pCmdUI) { phev_->OnUpdateAddrToggle(pCmdUI); }
    afx_msg void OnAutoFit() { phev_->OnAutoFit(); }
    afx_msg void OnUpdateAutofit(CCmdUI *pCmdUI) { phev_->OnUpdateAutofit(pCmdUI); }
    afx_msg void OnAerialHide() { phev_->OnAerialHide(); }
    afx_msg void OnAerialSplit() { phev_->OnAerialSplit(); }
    afx_msg void OnAerialTab() { phev_->OnAerialTab(); }
    afx_msg void OnUpdateAerialHide(CCmdUI* pCmdUI) { phev_->OnUpdateAerialHide(pCmdUI); }
    afx_msg void OnUpdateAerialSplit(CCmdUI* pCmdUI) { phev_->OnUpdateAerialSplit(pCmdUI); }
    afx_msg void OnUpdateAerialTab(CCmdUI* pCmdUI) { phev_->OnUpdateAerialTab(pCmdUI); }
    //        afx_msg void OnEditCopy() { phev_->OnEditCopy(); }
    //        afx_msg void OnEditCut() { phev_->OnEditCut(); }
    //        afx_msg void OnEditPaste() { phev_->OnEditPaste(); }
    //        afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI) { phev_->OnUpdateEditCut(pCmdUI); }
    afx_msg void OnGraphicToggle() { phev_->OnGraphicToggle(); }
    afx_msg void OnUpdateGraphicToggle(CCmdUI *pCmdUI) { phev_->OnUpdateGraphicToggle(pCmdUI); }
    afx_msg void OnCharToggle() { phev_->OnCharToggle(); }
    afx_msg void OnUpdateCharToggle(CCmdUI *pCmdUI) { phev_->OnUpdateCharToggle(pCmdUI); }
    afx_msg void OnFont() { phev_->OnFont(); }
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
    afx_msg void OnAllowMods() { phev_->OnAllowMods(); }
    afx_msg void OnUpdateAllowMods(CCmdUI *pCmdUI) { phev_->OnUpdateAllowMods(pCmdUI); }
    afx_msg void OnControlToggle() { phev_->OnControlToggle(); }
    afx_msg void OnInsert() { phev_->OnInsert(); }
    afx_msg void OnUpdateInsert(CCmdUI *pCmdUI) { phev_->OnUpdateInsert(pCmdUI); }
    afx_msg void OnUpdateDffdAutoSync(CCmdUI *pCmdUI) { phev_->OnUpdateDffdAutoSync(pCmdUI); }
    afx_msg void OnSearchIcase() { phev_->OnSearchIcase(); }
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
    afx_msg void OnUpdateTextPaste(CCmdUI *pCmdUI) { phev_->OnUpdateTextPaste(pCmdUI); }
    afx_msg void OnUpdateClipboard(CCmdUI *pCmdUI) { phev_->OnUpdateClipboard(pCmdUI); }
    afx_msg void OnUpdateUnicodePaste(CCmdUI *pCmdUI) { phev_->OnUpdateUnicodePaste(pCmdUI); }
    afx_msg void OnFontDec() { phev_->OnFontDec(); }
    afx_msg void OnFontInc() { phev_->OnFontInc(); }
    afx_msg void OnUpdateFontDec(CCmdUI *pCmdUI) { phev_->OnUpdateFontDec(pCmdUI); }
    afx_msg void OnUpdateFontInc(CCmdUI *pCmdUI) { phev_->OnUpdateFontInc(pCmdUI); }
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

	DECLARE_MESSAGE_MAP()

#ifdef _DEBUG
    CHexEditDoc *GetDocument()
    {
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CHexEditDoc)));
        return (CHexEditDoc*)m_pDocument;
    }
#endif

private:
    void invalidate_addr_range(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, bool no_border = false);
    void invalidate_addr_range_boundary(FILE_ADDRESS start_addr, FILE_ADDRESS end_addr);
    void draw_bitmap(CDC* pDC);
    void draw_ants(CDC* pDC, FILE_ADDRESS start, FILE_ADDRESS end, COLORREF);
    void draw_bounds(CDC* pDC, FILE_ADDRESS start_addr, FILE_ADDRESS end_addr, COLORREF clr1, COLORREF clr2);
    void draw_lines(CDC* pDC, FILE_ADDRESS start, FILE_ADDRESS end, COLORREF clr1, COLORREF clr2);
    void draw_pixel(CDC* pDC, int pnum, int x, int y, COLORREF clr1, COLORREF clr2);
    void get_colours(COLORREF clr, COLORREF & clr1, COLORREF & clr2);
    void draw_left_border(CDC* pDC, int left, int right, int row, COLORREF clr);
    void draw_top_border(CDC* pDC, int x, int ncols, COLORREF clr);
    void set_zoom(int z, bool scroll = true);

    union
    {
        // Note as this state is written to RecentFileList you must preserve the position of bits
        // in the union below
        unsigned long disp_state_;
        struct
        {
            unsigned int draw_ants_sel: 1;
            unsigned int draw_ants_mark: 1;
            unsigned int draw_ants_hl: 1;
            unsigned int draw_ants_search: 1;
            unsigned int draw_ants_bm: 1;
            unsigned int res1: 3;               // reserve 3 more bits for future ant options

            unsigned int draw_bdr_sel: 1;
            unsigned int draw_bdr_mark: 1;
            unsigned int draw_bdr_hl: 1;
            unsigned int draw_bdr_search: 1;
            unsigned int draw_bdr_bm: 1;
            unsigned int res2: 3;               // reserve 3 more bits for future border options

            unsigned int dpix: 6;               // Currently only the bottom 4 bits are used
        } disp_;
    };

    bool mouse_down_;                           // Is the left mouse button currently down?

    // The following are used to display a popup tip window
	CTipWnd tip_;       // Tip window shown when mouse hovers
    int tip_elt_;       // The elt we display info about
    int last_tip_elt_;  // The elt we last showed a tip about
    int elt_at(CPoint pt);                      // Given a mouse posn returns the elt under it
    void track_mouse(unsigned long);            // Turns on receipt of mouse hover/leave messages
	bool update_tip(int elt);                   // Refresh tip text
    void hex_addr_tip(FILE_ADDRESS addr);       // Adds a hex address to the tip window
    void dec_addr_tip(FILE_ADDRESS addr);       // Adds decimal address to the tip window

    // We use a timer to draw marching ants
    UINT timer_id_;     // Timer for selection flashing/marching ants
    UINT timer_msecs_;  // Number of millisecs between timer calls for timer_id_
    timer t00_;         // This is a "timer" that times how long it takes to process one "timer" event
    int timer_count_;   // Cycles between 0 and 7 for each timer call
    double bfactor_;    // Stretches the cycle when we are drawing a line that is not a multiple of 8 pixels
    FILE_ADDRESS prev_start_addr_, prev_end_addr_; // Previous selection in case we need to erase it

    // The following are used in drawing the window
    int bdr_left_, bdr_right_, bdr_top_, bdr_bottom_;       // Allow for borders around the "bitmap"
    FILE_ADDRESS scrollpos_;            // file address of the top-left displayed elt.
    int rows_, cols_;                   // Current number of rows and cols of elts shown in the window

    // Note: There are 2 different "zooms" which may be a bit confusing.
    // 1. BPE = number of bytes that contributes to an "elt" (ie, a "pixel" of the FreeImage bitmap).
    //    This allows the user to "zoom" in and out on large files to see more or less of the file.
    //    User can choose from between 1 and 65536, (restricted to the 16 values that are powers of 2?)
    //    For large files there is a limit on the smallest BPE allowed,
    //    otherwise a 1 TByte file would require 3 TBytes of memory at a BPE of 1.
    //    xxx need user option to specify largest amount of memory to use (min 16Mb = 1 TByte file @ BPE 65536)
    //    BPE says how big the FreeImage bitmap is so (like the bitmap) it is part of the 
    //    document and when it changes the FreeImage bitmap needs to be completely recalculated.
    // 2. DPIX = number of screen pixels required to display one "elt".
    //    Values are 1,2,3...8 for a 1x1, 2x2, 3x3, ... 8x8 square.
    //    Larger values allow individual elts to be visible on a high resolution display.
    //    For a small file in a large aerial view the "actual" value may be increased from the
    //    user specified value in order to make use of the extra screen real estate.
    //    When this value changes the FreeImage bitmap does not need to be recalculated (only
    //    reshaped) hence this can vary between views and is stored here.
    int actual_dpix_;   // Actual display pixel size - may be larger than dpix_ if the whole file fits in the window
    void get_disp_params(int &rows, int &cols, int &actual_dpix);

    // We cache the search occurrences that are currently drawn in the window.
    // This speeds up redraws espe the marching ants when there are millions of
    // occurrences since we don't have to rescan CHexEditDoc::found_.
    std::vector<pair<FILE_ADDRESS, FILE_ADDRESS> > search_pair_;  // Areas that need to be drawn to indicate found occurrences

    // Scrollbars
    int sbfact_;        // This is a scaling factor for the scrollbar as for large files 32-bit ints are not enough
    void update_bars();         // Make sure scroll bars reflect the part of the bitmap displayed
    void update_display();      // Recalc display params when it changes to speed OnDraw

    timer t0_, t1_, t2_, t3_, t4_, t5_, t6_, t7_, t8_, t9_, ta_, tb_, tc_, td_, te_;  // xxx for timing tests
};
#endif  // DATAFORMATVIEW_INCLUDED_
