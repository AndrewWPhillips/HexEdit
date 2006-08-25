// BCGMisc.h - for misc classes derived from BCG classes
//

#include <BCGCB.h>

// We just need to access some protected members
class CHexEditFontCombo : public CBCGToolbarFontCombo
{
	DECLARE_SERIAL(CHexEditFontCombo)
protected:
    static BYTE saved_charset;
    CHexEditFontCombo() : CBCGToolbarFontCombo() {}
public:
    CHexEditFontCombo(UINT uiID, int iImage,
						int nFontType = DEVICE_FONTTYPE | RASTER_FONTTYPE | TRUETYPE_FONTTYPE,
						BYTE nCharSet = DEFAULT_CHARSET,
						DWORD dwStyle = CBS_DROPDOWN, int iWidth = 0,
                        BYTE nPitchAndFamily = DEFAULT_PITCH) :
        CBCGToolbarFontCombo(uiID, iImage, nFontType, nCharSet, dwStyle, iWidth, nPitchAndFamily)
    {
    }

#if 0  // This is not necessary as the first time OnUpdateFont is called the charset will be fixed
    // This is needed when the base class changes the charset (eg after deserializing) and we have to keep
    // our saved copy up to date
    static void SaveCharSet()
    {
        CObList listButtons;
        POSITION posCombo;

        if (CBCGToolBar::GetCommandButtons(ID_FONTNAME, listButtons) > 0 &&
            (posCombo = listButtons.GetHeadPosition()) != NULL)
	    {
		    CHexEditFontCombo* pCombo = 
			    DYNAMIC_DOWNCAST(CHexEditFontCombo, listButtons.GetNext(posCombo));
            saved_charset = pCombo->m_nCharSet;
        }
    }
#endif

    // Is this control consistent with the static font list
    bool OKToRead()
    {
        return m_nCharSet == saved_charset;
    }

    // Required when we change between ANSI and OEM fonts
    void FixFontList(BYTE nCharSet)
    {
        bool fix_list = m_nCharSet != nCharSet;

        // Since the font list is static we only want to rebuild it once when charset changes
        if (saved_charset != nCharSet)
        {
            ClearFonts();
            m_nCharSet = nCharSet;
            saved_charset = nCharSet;
            RebuildFonts();
            fix_list = true;
        }
        if (fix_list)
        {
            m_nCharSet = nCharSet;
            RemoveAllItems();
            SetContext();
        }
    }
#ifdef _DEBUG
    void Check()
    {
    	POSITION pos;
        TRACE("***** %p %p\n", &m_lstItemData, &m_lstItems);
        TRACE("***** %d %d\n", m_lstItemData.GetCount(), m_lstItems.GetCount());

	    for (pos = m_lstItems.GetHeadPosition(); pos != NULL;)
        {
			CString strItem = m_lstItems.GetNext (pos);
            TRACE("*** NAME %s\n", strItem);
        }
	    for (pos = m_lstItemData.GetHeadPosition(); pos != NULL;)
        {
            CBCGFontDesc* pDesc = (CBCGFontDesc*) m_lstItemData.GetNext (pos);
            TRACE("*** name %s\n", pDesc->m_strName);
        }
    }
#endif
};

// We just need this to store the last font name used
class CHexEditFontSizeCombo : public CBCGToolbarFontSizeCombo
{
	DECLARE_SERIAL(CHexEditFontSizeCombo)
protected:
    CHexEditFontSizeCombo() : CBCGToolbarFontSizeCombo() {}
public:
	CHexEditFontSizeCombo(UINT uiID, int iImage, DWORD dwStyle = CBS_DROPDOWN, int iWidth = 0) :
        CBCGToolbarFontSizeCombo(uiID, iImage, dwStyle, iWidth)
    {
    }

    CString font_name_;                  // last font we got sizes for
};
