#if !defined(AFX_USERTOOL_H__E825A24A_BAE6_44EB_AE30_986F0355F2F3__INCLUDED_)
#define AFX_USERTOOL_H__E825A24A_BAE6_44EB_AE30_986F0355F2F3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserTool.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHexEditUserTool window

class CHexEditUserTool : public CBCGUserTool
{
    // Unless we add serialization we get the run-time error
    // "An unsupported operation was attempted", presumably
    // because BCG serializes objects of this class to the registry.
	DECLARE_SERIAL(CHexEditUserTool)

// Construction
public:
	CHexEditUserTool();
	virtual ~CHexEditUserTool();

// Attributes
public:

// Operations
public:

// Overrides
//	virtual void Serialize(CArchive& ar);  // Just use base class version as we added no extra members variables
	virtual BOOL Invoke();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERTOOL_H__E825A24A_BAE6_44EB_AE30_986F0355F2F3__INCLUDED_)
