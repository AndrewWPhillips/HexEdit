// CoordAp.h : coordinate classes with 64 bit vertical resolution
//
// Copyright (c) 2001 by Andrew W. Phillips.
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

/////////////////////////////////////////////////////////////////////////////
#ifndef COORDAP_INCLUDED_
#define COORDAP_INCLUDED_  1

#include <windef.h>

// This includes 3 classes CSizeAp, CPointAp and CRectAp that are drop-in
// replacements for the MFC CSize, CPoint and CRect classes.  These were
// created in 2.1 to enable large files to be edited.  Version 2.0 of HexEdit
// was limited to 2^31 pixels which limited (along with other things) the size
// of files that could be edited to a few Gbytes at most.

class CPointAp;
class CRectAp;

class CSizeAp
{
public:
    int cx;
    __int64 cy;

// Constructors
    CSizeAp() { ASSERT((cx = (int)0xCDCDCDCD, cy = 0xCDCDCDCDCDCDCDCD, TRUE)); }
    CSizeAp(int initCX, __int64 initCY) { cx = initCX, cy = initCY; }
    CSizeAp(const SIZE &initSize) { cx = initSize.cx; cy = __int64(initSize.cy); }
    CSizeAp(const POINT &initPt) { cx = initPt.x; cy = __int64(initPt.y); }
    CSizeAp(const CPointAp &initPt);
//    CSizeAp(const CSizeAp &initSize) { cx = initSize.cx; cy = initSize.cy; }

// Operations
    BOOL operator==(const CSizeAp &size) const { return (cx == size.cx && cy == size.cy); }
    BOOL operator!=(const CSizeAp &size) const { return (cx != size.cx || cy != size.cy); }
    void operator+=(const CSizeAp &size) { cx += size.cx; cy += size.cy; }
    void operator-=(const CSizeAp &size) { cx -= size.cx; cy -= size.cy; }

// Operators returning CSizeAp values
    CSizeAp operator+(const CSizeAp &size) const { return CSizeAp(cx + size.cx, cy + size.cy); }
    CSizeAp operator-(const CSizeAp &size) const { return CSizeAp(cx - size.cx, cy - size.cy); }
    CSizeAp operator-() const { return CSizeAp(-cx, -cy); }

// Operators returning CPointAp values
    CPointAp operator+(const CPointAp &point) const;
    CPointAp operator-(const CPointAp &point) const;

// Operators returning CRectAp values
    CRectAp operator+(const CRectAp &rect) const;
    CRectAp operator-(const CRectAp &rect) const;
};


class CPointAp
{
public:
    int x;
    __int64 y;

// Constructors
    CPointAp() { ASSERT((x = (int)0xCDCDCDCD, y = 0xCDCDCDCDCDCDCDCD, TRUE)); }
    CPointAp(int initX, __int64 initY) { x = initX; y = initY; }
    CPointAp(const POINT &initPt) { x = initPt.x; y = __int64(initPt.y); }
    CPointAp(const SIZE &initSize) { x = initSize.cx; y = __int64(initSize.cy); }
//  CPointAp(const CPointAp &initPt)  { x = initPt.x; y = initPt.y); }
    CPointAp(const CSizeAp &initSize) { x = initSize.cx; y = initSize.cy; }

// Operations
    void Offset(int xOffset, __int64 yOffset) { x += xOffset; y += yOffset; }
    void Offset(const CPointAp &point) { x += point.x; y += point.y; }
    void Offset(const SIZE &size) { x += size.cx; y += size.cy; }

    BOOL operator==(const CPointAp &point) const { return (x == point.x && y == point.y); }
    BOOL operator!=(const CPointAp &point) const { return (x != point.x || y != point.y); }
    void operator+=(const CSizeAp &size) { x += size.cx; y += size.cy; }
    void operator-=(const CSizeAp &size) { x -= size.cx; y -= size.cy; }
    void operator+=(const CPointAp &point) { x += point.x; y += point.y; }
    void operator-=(const CPointAp &point) { x -= point.x; y -= point.y; }

// Operators returning CPointAp values
    CPointAp operator+(const CSizeAp &size) const { return CPointAp(x + size.cx, y + size.cy); }
    CPointAp operator-(const CSizeAp &size) const { return CPointAp(x - size.cx, y - size.cy); }
    CPointAp operator-() const { return CPointAp(-x, -y); }
    CPointAp operator+(const CPointAp &point) const{ return CPointAp(x + point.x, y + point.y); }

// Operators returning CSizeAp values
    CSizeAp operator-(const CPointAp &point) const { return CSizeAp(x - point.x, y - point.y); }

// Operators returning CRectAp values
    CRectAp operator+(const CRectAp &rect) const;
    CRectAp operator-(const CRectAp &rect) const;
};

class CRectAp
{
public:
    LONG left, right;
    __int64 top, bottom;

// Constructors
    CRectAp() { ASSERT((left = right = (LONG)0xCDCDCDCD, top = bottom = 0xCDCDCDCDCDCDCDCD, TRUE)); }
    CRectAp(int l, __int64 t, int r, __int64 b) : left(l), right(r), top(t), bottom(b) { }
    CRectAp(const RECT &srcRect) { left = srcRect.left; top = __int64(srcRect.top); right = srcRect.right; bottom = __int64(srcRect.bottom); }
    CRectAp(const RECT *pr) { left = pr->left; top = __int64(pr->top); right = pr->right; bottom = __int64(pr->bottom); }
    CRectAp(const CPointAp &point, const CSizeAp &size) { right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
    CRectAp(const CPointAp &topLeft, const CPointAp &bottomRight) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }

// Attributes
    int Width() const { return right - left; }
    __int64 Height() const { return bottom - top; }
    CSizeAp Size() const { return CSizeAp(right - left, bottom - top); }
    CPointAp TopLeft() { return CPointAp(left, top); }
    CPointAp BottomRight() { return CPointAp(right, bottom); }
    CPointAp CenterPoint() const { return CPointAp((left+right)/2, (top+bottom)/2); }
    void SwapLeftRight() { int temp = left; left = right; right = temp; }
    void SwapTopBottom() { __int64 temp = top; top = bottom; bottom = temp; }

    BOOL IsRectEmpty() const { return (right <= left || bottom <= top); }
    BOOL IsRectNull() const { return (left == 0 && right == 0 && top == 0 && bottom == 0); }
    BOOL PtInRect(const CPointAp &point) const { return point.x >= left && point.x < right && point.y >= top && point.y < bottom; }

// Operations
    void SetRect(int xx1, __int64 yy1, int xx2, __int64 yy2) { left = xx1; top = yy1; right = xx2; bottom = yy2; }
    void SetRect(const CPointAp &topLeft, const CPointAp &bottomRight)
                { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }
    void SetRectEmpty() { left = right = 0; bottom = top = 0; }
    void CopyRect(const RECT *pr) { left = pr->left; top = __int64(pr->top); right = pr->right; bottom = __int64(pr->bottom); }
    BOOL EqualRect(const CRectAp &rect) const { return left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom; }

    // inflate rectangle's width and height
    void InflateRect(int x, __int64 y) { left -=x; top -= y; right += x; bottom += y; }
    void InflateRect(const CSizeAp &size) { left -= size.cx; top -= size.cy; right += size.cx; bottom += size.cy; }
    void InflateRect(const RECT *lpRect) { left -= lpRect->left; top -= __int64(lpRect->top); right += lpRect->right; bottom += __int64(lpRect->bottom); }
    void InflateRect(int l, __int64 t, int r, __int64 b) { left -= l; top -= t; right += r; bottom += b; }
    // deflate the rectangle's width and height
    void DeflateRect(int x, __int64 y) { left +=x; top += y; right -= x; bottom -= y; }
    void DeflateRect(const SIZE &size) { left += size.cx; top += size.cy; right -= size.cx; bottom -= size.cy; }
    void DeflateRect(const RECT *lpRect) { left += lpRect->left; top += __int64(lpRect->top); right -= lpRect->right; bottom -= __int64(lpRect->bottom); }
    void DeflateRect(int l, __int64 t, int r, __int64 b) { left += l; top += t; right -= r; bottom -= b; }

    // translate the rectangle by moving its top and left
    void OffsetRect(int x, __int64 y) { left +=x; top += y; right += x; bottom += y; }
    void OffsetRect(const CSizeAp &size) { left += size.cx; top += size.cy; right += size.cx; bottom += size.cy; }
    void OffsetRect(const CPointAp &point)  { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void NormalizeRect() { if (right < left) SwapLeftRight();  if (bottom < top) SwapTopBottom(); }

    // set this rectangle to intersection of two others
    BOOL IntersectRect(const CRectAp &r1, const CRectAp &r2)
    {
        BOOL retval = r1.left < r2.right && r2.left < r1.right && r1.top < r2.bottom && r2.top < r1.bottom;
        if (retval)
        {
            left = max(r1.left, r2.left);
            right = min(r1.right, r2.right);
            ASSERT(left <= right);
            top = max(r1.top, r2.top);
            bottom = min(r1.bottom, r2.bottom);
            ASSERT(top <= bottom);
        }
        else
        {
            left = right = 0;
            bottom = top = 0;
        }
        return retval;
    }

    // set this rectangle to bounding union of two others
    BOOL UnionRect(const CRectAp &r1, const CRectAp &r2)
    {
        BOOL b1 = r1.IsRectEmpty();
        BOOL b2 = r2.IsRectEmpty();
        if (b1 && b2)
        {
            left = right = 0;
            bottom = top = 0;
            return FALSE;
        }
        else if (b1)
            *this = r2;
        else if (b2)
            *this = r1;
        else
        {
            left = min(r1.left, r2.left);
            right = max(r1.right, r2.right);
            top = min(r1.top, r2.top);
            bottom = max(r1.bottom, r2.bottom);
        }
        ASSERT(!IsRectEmpty());
        return TRUE;
    }

// Additional Operations
    void operator=(const RECT &srcRect) { left = srcRect.left; top = __int64(srcRect.top); right = srcRect.right; bottom = __int64(srcRect.bottom); }
    BOOL operator==(const CRectAp &r) const { return (left == r.left && top == r.top && right == r.right && bottom == r.bottom); }
    BOOL operator!=(const CRectAp &r) const { return !(left == r.left && top == r.top && right == r.right && bottom == r.bottom); }

    void operator+=(const CPointAp &point) { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator+=(const CSizeAp &size)  { left += size.cx; top += size.cy; right += size.cx; bottom += size.cy; }
    void operator-=(const CPointAp &point) { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }
    void operator-=(const CSizeAp &size)  { left -= size.cx; top -= size.cy; right -= size.cx; bottom -= size.cy; }

// Operators returning CRectAp values
    CRectAp operator+(const CPointAp &point) const { return CRectAp(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    CRectAp operator-(const CPointAp &point) const { return CRectAp(left - point.x, top - point.y, right - point.x, bottom - point.y); }
    CRectAp operator+(const CSizeAp &size) const  { return CRectAp(left + size.cx, top + size.cy, right + size.cx, bottom + size.cy); }
    CRectAp operator-(const CSizeAp &size) const  { return CRectAp(left - size.cx, top - size.cy, right - size.cx, bottom - size.cy); }
};

// These could not be added above due to circular references
inline CSizeAp::CSizeAp(const CPointAp &initPt)  { cx = initPt.x; cy = initPt.y; }
inline CPointAp CSizeAp::operator+(const CPointAp &point) const { return CPointAp(cx + point.x, cy + point.y); }
inline CPointAp CSizeAp::operator-(const CPointAp &point) const { return CPointAp(cx - point.x, cy - point.y); }
inline CRectAp CSizeAp::operator+(const CRectAp &rect) const { return CRectAp(rect) + *this; }
inline CRectAp CSizeAp::operator-(const CRectAp &rect) const { return CRectAp(rect) - *this; }

inline CRectAp CPointAp::operator+(const CRectAp &rect) const { return CRectAp(rect) + *this; }
inline CRectAp CPointAp::operator-(const CRectAp &rect) const { return CRectAp(rect) - *this; }

#endif
