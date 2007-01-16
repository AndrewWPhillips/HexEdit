// EnBitmap.h: interface for the CEnBitmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)
#define AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

////////////////////////////////////////////////////////////////////////////////////
// helper struct. equates to COLORREF

#pragma pack(push)
#pragma pack(1)

struct RGBX
{
public:
	RGBX() { btRed = btBlue = btGreen = btUnused = 0; }
	RGBX(BYTE red, BYTE green, BYTE blue) { btRed = red; btBlue = blue; btGreen = green; btUnused = 0; }
	RGBX(COLORREF color) { btRed = GetRValue(color); btBlue = GetBValue(color); btGreen = GetGValue(color); btUnused = 0; }

	BYTE btBlue;
	BYTE btGreen;
	BYTE btRed;

protected:
	BYTE btUnused;

public:
	inline BOOL Equals(const RGBX& rgb) { return (btRed == rgb.btRed && btGreen == rgb.btGreen && btBlue == rgb.btBlue); }
	inline BOOL IsGray() const { return (btRed == btGreen && btGreen == btBlue); }

	RGBX Gray() 
	{ 
		BYTE btGray = ((int)btBlue + (int)btGreen * 6 + (int)btRed * 3) / 10;
		return RGBX(btGray, btGray, btGray);
	}

};

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////////////////

class CEnBitmap : public CBitmap  
{
public:
	CEnBitmap(COLORREF crBkgnd = RGB(255, 255, 255));
	virtual ~CEnBitmap();

	BOOL LoadImage(LPCTSTR szImagePath, COLORREF crBack = -1);
	BOOL LoadImage(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst = NULL, COLORREF crBack = -1); 
	BOOL CopyImage(HBITMAP hBitmap);
	BOOL CopyImage(CBitmap* pBitmap);

	BOOL RotateImage(int nDegrees, BOOL bEnableWeighting = FALSE); // rotates about centrepoint, +ve (clockwise) or -ve (anticlockwise) from 12 o'clock
	BOOL ShearImage(int nHorz, int nVert, BOOL bEnableWeighting = FALSE); // shears +ve (right, down) or -ve (left, up)
	BOOL GrayImage();
	BOOL BlurImage(int nAmount = 5);
	BOOL SharpenImage(int nAmount = 5);
	BOOL ResizeImage(double dFactor);
	BOOL FlipImage(BOOL bHorz, BOOL bVert = 0);
	BOOL NegateImage();
	BOOL ReplaceColor(COLORREF crFrom, COLORREF crTo);

	// helpers
	static HBITMAP LoadImageFile(LPCTSTR szImagePath, COLORREF crBack = -1);
	static HBITMAP LoadImageResource(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst = NULL, COLORREF crBack = 0); 
	static BOOL GetResource(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst, void* pResource, int& nBufSize);
	static IPicture* LoadFromBuffer(BYTE* pBuff, int nSize);

protected:
	COLORREF m_crBkgnd;

protected:
	RGBX* GetDIBits32();
	BOOL PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap = NULL);

	static HBITMAP ExtractBitmap(IPicture* pPicture, COLORREF crBack);
	static int GetFileType(LPCTSTR szImagePath);
	static BOOL Fill(RGBX* pPixels, CSize size, COLORREF color);
};

#endif // !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)
