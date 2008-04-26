/*
Module : SPLASHER.H
Purpose: A splash screen component for MFC 4.x which uses a DIB bitmap
         instead of a resource. Palette management code is also included
         so that the bitmap will display using its own optimized palette
Created: PJN / 15-11-1996

Copyright (c) 1996 - 2000 by PJ Naughter.  
All rights reserved.

*/


#ifndef __SPLASHER_H__
#define __SPLASHER_H__


///////////////// Classes //////////////////////////
class CSplashWnd : public CWnd
{
public:
  CSplashWnd();
  ~CSplashWnd();

// Operations
  void SetBitmapToUse(const CString& sFilename);
  void SetBitmapToUse(UINT nResourceID);
  void SetBitmapToUse(LPCTSTR pszResourceName); 
  BOOL Create();
  void SetOKToClose() { m_bOKToClose = TRUE; };  
  
protected:
  //{{AFX_VIRTUAL(CSplashWnd)
  //}}AFX_VIRTUAL

  //{{AFX_MSG(CSplashWnd)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaint();
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
  afx_msg BOOL OnQueryNewPalette();
  afx_msg void OnClose();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()
  
  BOOL SelRelPal(BOOL bForceBkgnd);
  BOOL LoadBitmap();
  void CreatePaletteFromBitmap();

  BOOL      m_bOKToClose;
  CBitmap   m_Bitmap;
  CPalette  m_Palette;
  int       m_nHeight;
  int       m_nWidth;
  CWnd      m_wndOwner;                   
  BOOL      m_bUseFile;
  LPCTSTR   m_pszResourceName;
  CString   m_sFilename;
};


class CSplashThread : public CWinThread
{
public:
  void HideSplash();
  void SetBitmapToUse(const CString& sFilename);
  void SetBitmapToUse(UINT nResourceID);
  void SetBitmapToUse(LPCTSTR pszResourceName); 

protected:
	CSplashThread();
  virtual ~CSplashThread();

	DECLARE_DYNCREATE(CSplashThread)

	//{{AFX_VIRTUAL(CSplashThread)
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CSplashThread)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

  CSplashWnd m_SplashScreen;
};


#endif //__SPLASHER_H__


