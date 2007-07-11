// XMLTree.cpp : Wrapper of MS DOM implementation
//
// Copyright (c) 1999-2005 by Andrew W. Phillips.
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

#include "xmltree.h"

// Construct and load from file
CXmlTree::CXmlTree(const LPCTSTR filename /*=NULL*/)
{
    m_modified = false;

    m_pdoc.CreateInstance(MSXML::CLSID_DOMDocument);
    if (filename != NULL)
    {
        m_filename = filename;
        m_error = !m_pdoc->load(_bstr_t(m_filename));
    }
    else
        m_error = false;
}

// Load XML from a file
bool CXmlTree::LoadFile(LPCTSTR filename)
{
    m_modified = false;
    m_filename = filename;
    return !(m_error = !m_pdoc->load(_bstr_t(m_filename)));
}

// Load XML from a string
bool CXmlTree::LoadString(LPCTSTR ss)
{
    m_modified = true;      // Anything already there is overwritten
    return !(m_error = !m_pdoc->loadXML(_bstr_t(ss)));
}

bool CXmlTree::Save(LPCTSTR filename /*=NULL*/)
{
    if (filename == NULL)
	{
		if (m_filename.IsEmpty())
			return false;
		filename = m_filename;
	}
	try
	{
		HRESULT hr;
		hr = m_pdoc->save(_bstr_t(filename));

		m_modified = false;     // What's on disk now matches what's in memory
		return hr == S_OK;
	}
	catch (...)
	{
		return false;
	}
}

CString CXmlTree::GetDTDName() const
{
    return CString(LPCWSTR(m_pdoc->doctype->name));
}

CString CXmlTree::ErrorMessage() const
{
    return CString(LPCWSTR(m_pdoc->parseError->reason));
}

long CXmlTree::ErrorLine() const
{
    return m_pdoc->parseError->line;
}

CString CXmlTree::ErrorLineText() const
{
    return CString(LPCWSTR(m_pdoc->parseError->srcText));
}


//---------------------------------------------------------
// CXmlTree::CElt

long CXmlTree::CElt::GetNumChildren() const
{
    ASSERT(m_pelt != NULL);

    long count;

    MSXML::IXMLDOMNodePtr pnode = m_pelt->firstChild;

    for (count = 0; pnode != NULL; ++count)
    {
        pnode = pnode->nextSibling;
    }

    return count;
}

CXmlTree::CElt CXmlTree::CElt::GetParent() const
{
    ASSERT(m_powner != NULL && m_pelt != NULL);

    return CElt(m_pelt->parentNode, m_powner);
}

CXmlTree::CElt CXmlTree::CElt::GetChild(long num) const
{
	ASSERT(num >= 0);
    ASSERT(m_powner != NULL && m_pelt != NULL);

    MSXML::IXMLDOMNodePtr pnode = m_pelt->firstChild;

    long count = 0;

    while (pnode != NULL)
    {
        if (pnode->nodeType == MSXML::NODE_ELEMENT)
        {
            if (count++ == num)
                return CElt(pnode, m_powner);
        }
        pnode = pnode->nextSibling;
    }

    return CElt(MSXML::IXMLDOMNodePtr(0), m_powner);
}

CXmlTree::CElt CXmlTree::CElt::GetChild(const LPCTSTR name) const
{
    ASSERT(m_powner != NULL && m_pelt != NULL);

    MSXML::IXMLDOMNodePtr pnode = m_pelt->firstChild;

    while (pnode != NULL)
    {
        if (pnode->nodeType == MSXML::NODE_ELEMENT)
        {
            if (CString(LPCWSTR(pnode->nodeName)) == name)
            {
                return CElt(pnode, m_powner);
            }
        }
        pnode = pnode->nextSibling;
    }

    return CElt(MSXML::IXMLDOMNodePtr(0), m_powner);
}

CXmlTree::CElt CXmlTree::CElt::InsertNewChild(const LPCTSTR name, const CElt *before /*= NULL*/)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);
    ASSERT(before == NULL || before->m_powner == m_powner);

    MSXML::IXMLDOMElementPtr pnew = m_powner->m_pdoc->createElement(_bstr_t(name));

    m_powner->SetModified(true);
    if (before != NULL)
        return CElt(m_pelt->insertBefore(pnew, _variant_t((IDispatch *)(before->m_pelt))), m_powner);
    else
        return CElt(m_pelt->insertBefore(pnew, _variant_t()), m_powner);
}

CXmlTree::CElt CXmlTree::CElt::InsertChild(CElt elt, const CElt *before /*=NULL*/)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);
    ASSERT(before == NULL || before->m_powner == m_powner);
    ASSERT(elt.m_powner == m_powner);

    m_powner->SetModified(true);
    if (before != NULL)
        return CElt(m_pelt->insertBefore(elt.m_pelt, _variant_t((IDispatch *)(before->m_pelt))), m_powner);
    else
        return CElt(m_pelt->insertBefore(elt.m_pelt, _variant_t()), m_powner);
}

CXmlTree::CElt CXmlTree::CElt::InsertClone(CElt elt, const CElt *before /*=NULL*/)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);
    ASSERT(before == NULL || before->m_powner == m_powner);
    ASSERT(elt.m_powner == m_powner);

    MSXML::IXMLDOMElementPtr ee = elt.m_pelt->cloneNode(VARIANT_TRUE);   // TRUE = 0xFFFF not 1 !?!?!?

    m_powner->SetModified(true);
    if (before != NULL)
        return CElt(m_pelt->insertBefore(ee, _variant_t((IDispatch *)(before->m_pelt))), m_powner);
    else
        return CElt(m_pelt->insertBefore(ee, _variant_t()), m_powner);
}

void CXmlTree::CElt::ReplaceChild(CElt new_elt, CElt old_elt)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);
    ASSERT(old_elt.m_powner == m_powner);
    (void)m_pelt->replaceChild(new_elt.m_pelt, old_elt.m_pelt);
    m_powner->SetModified(true);
}

CXmlTree::CElt CXmlTree::CElt::DeleteChild(CElt elt)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);
    ASSERT(elt.m_powner == m_powner);
    MSXML::IXMLDOMElementPtr pdel = m_pelt->removeChild(elt.m_pelt);
    m_powner->SetModified(true);
    return CElt(pdel, m_powner);
}

void CXmlTree::CElt::DeleteAllChildren()
{
    CElt child, next_child;
    for (child = GetFirstChild(); !child.IsEmpty(); child = next_child)
    {
        next_child = child;
        ++next_child;

        (void)m_pelt->removeChild(child.m_pelt);
    }
    m_powner->SetModified(true);
}

CXmlTree::CElt CXmlTree::CElt::Clone()
{
    return CXmlTree::CElt(m_pelt->cloneNode(VARIANT_TRUE), m_powner);
}

CString CXmlTree::CElt::GetAttr(const LPCTSTR attr_name) const
{
    ASSERT(m_pelt != NULL);

    _variant_t retval = m_pelt->getAttribute(_bstr_t(attr_name));
    if (retval.vt == VT_NULL || retval.vt == VT_EMPTY)
        return CString("");
    else
    {
        retval.ChangeType(VT_BSTR);
        return CString(LPCWSTR(_bstr_t(retval)));
    }
}

void CXmlTree::CElt::SetAttr(const LPCTSTR attr_name, const LPCTSTR value)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);

    m_pelt->setAttribute(_bstr_t(attr_name), _variant_t(value));
    m_powner->SetModified(true);
}

void CXmlTree::CElt::RemoveAttr(const LPCTSTR attr_name)
{
    ASSERT(m_powner != NULL && m_pelt != NULL);

    m_pelt->removeAttribute(_bstr_t(attr_name));
    m_powner->SetModified(true);
}

// Save all the children of a node (pelt) into a doc fragment (CFrag)
void CXmlTree::CFrag::SaveKids(CElt *pelt)
{
    ASSERT(m_pfrag != NULL);
    CElt child;

//    for (child = pelt->GetFirstChild(); !child.IsEmpty(); ++child)
//        m_pfrag->appendChild(child.m_pelt->cloneNode(VARIANT_TRUE));

    // First move all the child nodes to the fragment
    for (child = pelt->GetFirstChild(); !child.IsEmpty(); child = pelt->GetFirstChild())
        m_pfrag->appendChild(child.m_pelt);

    // Now clone all the moved nodes and add them back
    MSXML::IXMLDOMNodePtr pnode;

    for (pnode = m_pfrag->firstChild; pnode != NULL; pnode = pnode->nextSibling)
        pelt->m_pelt->appendChild(pnode->cloneNode(VARIANT_TRUE));
}

void CXmlTree::CFrag::InsertKids(CElt *pelt)
{
    ASSERT(m_pfrag != NULL);
    pelt->m_pelt->appendChild(m_pfrag);
    pelt->GetOwner()->SetModified(true);
}

// We could have made this a superset of above by defaulting "before" to NULL,
// but we want to leave the above "InsertKids" function unchanged as we know it works.
void CXmlTree::CFrag::InsertKids(CElt *pelt, const CElt *before)
{
    ASSERT(m_powner != NULL && m_pfrag != NULL);
    ASSERT(before == NULL || before->m_powner == m_powner);
    if (before != NULL)
        pelt->m_pelt->insertBefore(m_pfrag, _variant_t((IDispatch *)(before->m_pelt)));
    else
        pelt->m_pelt->insertBefore(m_pfrag, _variant_t());
    pelt->GetOwner()->SetModified(true);
}

CXmlTree::CElt CXmlTree::CFrag::AppendClone(CXmlTree::CElt elt)
{
    ASSERT(m_powner != NULL && m_pfrag != NULL);
    ASSERT(elt.m_powner == m_powner);
    MSXML::IXMLDOMElementPtr ee = elt.m_pelt->cloneNode(VARIANT_TRUE);   // TRUE = 0xFFFF not 1 !?!?!?
    return CXmlTree::CElt(m_pfrag->insertBefore(ee, _variant_t()), m_powner);
}

CXmlTree::CElt CXmlTree::CFrag::InsertClone(CElt elt, const CElt *before /*=NULL*/)
{
    ASSERT(m_powner != NULL && m_pfrag != NULL);
    ASSERT(before == NULL || before->m_powner == m_powner);
    ASSERT(elt.m_powner == m_powner);

    MSXML::IXMLDOMElementPtr ee = elt.m_pelt->cloneNode(VARIANT_TRUE);   // TRUE = 0xFFFF not 1 !?!?!?
    if (before != NULL)
        return CElt(m_pfrag->insertBefore(ee, _variant_t((IDispatch *)(before->m_pelt))), m_powner);
    else
        return CElt(m_pfrag->insertBefore(ee, _variant_t()), m_powner);
}


