// Xmltree.h : wrapper for MS XML (DOM) objects
//
// Copyright (c) 1999 by Andrew W. Phillips.
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
#pragma once

#ifndef XMLTREE_INCLUDED
#define XMLTREE_INCLUDED

#include <afx.h>
#import <msxml.dll> named_guids

class CXmlTree
{
    friend class CElt;
    friend class CFrag;

public:
    CXmlTree(const LPCTSTR filename = NULL);
    MSXML::IXMLDOMDocumentPtr m_pdoc;   // COM Interface ptr to doc itself

	bool LoadFile(LPCTSTR filename);
	bool LoadString(LPCTSTR ss);
    bool Save(LPCTSTR filename = NULL);
    void SetFileName(const LPCTSTR filename) { m_filename = filename; }
    CString GetFileName() const { return m_filename; }
    CString GetDTDName() const;
    bool Error() { return m_error; }
    CString ErrorMessage() const;
    long ErrorLine() const;
    CString ErrorLineText() const;
    bool IsModified() const { return m_modified; }
    void SetModified(bool mm) { m_modified = mm; }

	class CFrag;
    class CElt
    {
        friend class CXmlTree;
		friend class CFrag;
    public:
        CElt() : m_pelt(NULL), m_powner(NULL) { }
        CElt(MSXML::IXMLDOMNodePtr pelt, CXmlTree *powner) : m_powner(powner), m_pelt(pelt)  { }
        CElt(const LPCTSTR name, CXmlTree *powner) : m_powner(powner)
        {
            m_pelt = m_powner->m_pdoc->createElement(_bstr_t(name));
        }

#ifdef _DEBUG
        CString Dump() const { return CString(LPCWSTR(m_pelt->xml)); }
#endif

        long GetNumChildren() const;
        long GetNumText() const;
        CXmlTree *GetOwner() const { return m_powner; }

        CString GetName() const { if (m_pelt != NULL) return CString(LPCWSTR(m_pelt->nodeName)); else return CString(""); }
//        void SetName(LPCTSTR name) { if (m_pelt != NULL) m_pelt->nodeName = _bstr_t(name); }

        // Get parent, children, text etc
        CElt GetParent() const;
        CElt GetChild(long num) const;          // get a child element by posn
        CElt GetFirstChild() const { return GetChild(long(0)); }
        CElt GetChild(const LPCTSTR name) const;// get a child element by name
        CString GetText(long num) const;        // get one text "section"
        CString GetText() const;                // get all text concatenated

        // Atributes
        CString GetAttr(const LPCTSTR attr_name) const;
        void SetAttr(const LPCTSTR attr_name, const LPCTSTR value);
        void RemoveAttr(const LPCTSTR attr_name);

        // Adding and deleting child elements
        CElt InsertNewChild(const LPCTSTR name, const CElt *before = NULL);
        CElt InsertChild(CElt elt, const CElt *before = NULL);
        CElt InsertClone(CElt elt, const CElt *before = NULL);   // clone (don't move) if already in the tree
        void ReplaceChild(CElt new_elt, CElt old_elt);
        CElt DeleteChild(CElt elt);
		void DeleteAllChildren();

        CElt Clone();                                            // create a (deep) clone of this element

        bool IsEmpty() const { return m_pelt == NULL; }
        CElt operator++() { if (m_pelt != NULL) m_pelt = m_pelt->nextSibling; return *this; }
        CElt operator++(int) { CElt tmp(*this); if (m_pelt != NULL) m_pelt = m_pelt->nextSibling; return tmp; }
        CElt operator--() { if (m_pelt != NULL) m_pelt = m_pelt->previousSibling; return *this; }
        CElt operator--(int) { CElt tmp(*this); if (m_pelt != NULL) m_pelt = m_pelt->previousSibling; return tmp; }

    protected:
        CXmlTree *m_powner;                     // CXmlTree class that this elt is associated with

    public:
// Using this operator causes compile error so make m_pelt public for direct access
//        operator MSXML::IXMLDOMElementPtr() { return m_pelt; }
    	MSXML::IXMLDOMElementPtr m_pelt;        // Interface ptr to the element node
//    	MSXML::IXMLDOMNodePtr m_pelt;           // Interface ptr to the element node
    };

    class CFrag
    {
        friend class CXmlTree;
    public:
        CFrag() : m_pfrag(NULL), m_powner(NULL) { }
        CFrag(CXmlTree *powner) : m_powner(powner) 
        {
            m_pfrag = m_powner->m_pdoc->createDocumentFragment();
        }

        void SaveKids(CElt *pelt);              // save children of elt to this fragment
        void InsertKids(CElt *pelt);            // add fragment as children of elt
		void InsertKids(CElt *pelt, const CElt *before);

		bool IsEmpty() const { return m_pfrag == NULL || m_pfrag->firstChild == NULL; }
        CElt GetFirstChild() const { return CElt(m_pfrag->firstChild, m_powner); }
        CElt AppendClone(CElt elt);
		CElt InsertClone(CElt elt, const CElt *before = NULL);

		CString DumpXML() const { return CString(LPCWSTR(m_pfrag->xml)); }

    protected:
        CXmlTree *m_powner;                     // CXmlTree class that this fragment is associated with
        MSXML::IXMLDOMDocumentFragmentPtr m_pfrag;
    };

    CString DumpXML() const { return CString(LPCWSTR(m_pdoc->xml)); }
    CElt GetRoot() { return CElt(m_pdoc->documentElement, this); }

protected:
    CString m_filename;                 // name of file to load from or save to
    bool m_error;                       // did the last load have a parse error?
    bool m_modified;                    // has the XML been modified
};
#endif
