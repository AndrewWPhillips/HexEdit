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
	CLIPFORMAT m_cf_ours;               // Custom Clip format to hold COM ptr (MSXML2::IXMLDOMElementPtr)
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
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
	{
		// If dffd view can't handle it try "owner" hex view
		if (CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
		else if (phev_ != NULL)
			return phev_->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}
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

	// OnUpdateDisable is used to disable commands we don't want passed on to hex view
	afx_msg void OnUpdateDisable(CCmdUI* pCmdUI) { pCmdUI->Enable(FALSE); }

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
	IMAGE_DATA_GREEN,
	IMAGE_DATA_RED,
	IMAGE_DATA_BITFIELD,

	IMAGE_LAST   // leave this last
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAFORMATVIEW_H__AA218642_2DFC_4659_94FE_85AD21EDE726__INCLUDED_)
