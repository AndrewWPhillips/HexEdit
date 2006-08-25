// FindDlg.cpp : implementation file
//

#include "stdafx.h"
#include <MultiMon.h>
#include "hexedit.h"
#include "mainfrm.h"
#include "FindDlg.h"
#include "Control.h"
#include "HexEditView.h"

#ifdef USE_HTML_HELP
#include <HtmlHelp.h>
#endif

#include "resource.hm"
#include "HelpID.hm"            // For help IDs for the pages

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindSheet

IMPLEMENT_DYNAMIC(CFindSheet, CPropertySheet)

CFindSheet::CFindSheet(UINT iSelectPage /*=0*/)
        :CPropertySheet(_T("Find"), AfxGetMainWnd(), iSelectPage)
{
    prefix_entered_ = FALSE;

    search_buf_ = mask_buf_ = replace_buf_ = NULL;

    // Set up the pages
    p_page_simple_ = new CSimplePage;
    p_page_hex_ = new CHexPage;
    p_page_text_ = new CTextPage;
    p_page_number_ = new CNumberPage;
    p_page_replace_ = new CReplacePage;

    AddPage(p_page_simple_);
    AddPage(p_page_hex_);
    AddPage(p_page_text_);
    AddPage(p_page_number_);
    AddPage(p_page_replace_);

    // Restore settings from the registry
	whole_word_ = theApp.GetProfileInt("Find-Settings", "WholeWord", FALSE);
	match_case_ = theApp.GetProfileInt("Find-Settings", "MatchCase", FALSE);
    direction_ = dirn_t(theApp.GetProfileInt("Find-Settings", "Direction", DIRN_DOWN));
	scope_ = scope_t(theApp.GetProfileInt("Find-Settings", "Scope", SCOPE_EOF));

    alignment_ = ALIGN_BYTE;  // May be confusing to save/restore this - default to byte (all found)
	offset_ = 0;
//    use_mask_ = theApp.GetProfileInt("Find-Settings", "UseMask", FALSE);

    wildcards_allowed_ = theApp.GetProfileInt("Find-Settings", "UseWildcards", FALSE);;
    charset_ = RB_CHARSET_ASCII;    // Changes to match char set of active window
    char buf[2];
    buf[0] = theApp.GetProfileInt("Find-Settings", "Wildcard", '?');
    buf[1] = '\0';
    wildcard_char_ = buf;

    big_endian_ = FALSE;  // May be confusing to save/restore this - default to little-endian (Intel)
    number_format_ = theApp.GetProfileInt("Find-Settings", "NumberFormat", 0);
    number_size_ = theApp.GetProfileInt("Find-Settings", "NumberSize", 2);;
}

BOOL CFindSheet::Create(CWnd* pParentWnd, DWORD dwStyle) 
{
    // Create the property sheet
	if (!CPropertySheet::Create(pParentWnd, dwStyle))
		return FALSE;

	return TRUE;
}

CFindSheet::~CFindSheet()
{
	theApp.WriteProfileInt("Find-Settings", "WholeWord", whole_word_);
	theApp.WriteProfileInt("Find-Settings", "MatchCase", match_case_);
    theApp.WriteProfileInt("Find-Settings", "Direction", direction_);
	theApp.WriteProfileInt("Find-Settings", "Scope", scope_);

//    theApp.WriteProfileInt("Find-Settings", "UseMask", use_mask_);

    theApp.WriteProfileInt("Find-Settings", "UseWildcards", wildcards_allowed_);;
    ASSERT(wildcard_char_.GetLength() == 1);
    theApp.WriteProfileInt("Find-Settings", "Wildcard", wildcard_char_[0]);

    theApp.WriteProfileInt("Find-Settings", "NumberFormat", number_format_);
    theApp.WriteProfileInt("Find-Settings", "NumberSize", number_size_);;
}

BEGIN_MESSAGE_MAP(CFindSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CFindSheet)
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

// Determine if the simple string search is for a text or hex search
CFindSheet::string_t CFindSheet::StringType(LPCTSTR ss)
{
    string_t retval = STRING_UNKNOWN;
    int consec_count = 0;               // number of consecutive hex digits
    const char *pp;

    for (pp = ss; *pp != '\0'; ++pp)
    {
        if (isspace(*pp))
        {
            if ((consec_count % 2) != 0)
                return STRING_TEXT;
            consec_count = 0;
        }
        else if (isxdigit(*pp))
        {
            retval = STRING_HEX;
            ++consec_count;
        }
        else
            return STRING_TEXT;
    }

    if ((consec_count % 2) != 0)
        return STRING_TEXT;
    else
        return retval;
}

// Change current search using encoded string as in Find Tool
void CFindSheet::NewSearch(LPCTSTR ss)
{
    if (strlen(ss) == 0)
        return;
    else if (ss[0] == '~')
    {
        SetMatchCase(FALSE);
        NewText(ss+1);
    }
    else if (ss[0] == '=')
    {
        SetMatchCase(TRUE);
        NewText(ss+1);
    }
    else
    {
        // Must be a hex search
        ASSERT(strspn(ss, "0123456789abcdefABCDEF ?") == strlen(ss));
        NewHex(ss);
    }
}

// Change current search text from outside the dialog
void CFindSheet::NewText(LPCTSTR ss)
{
    CWnd *pwnd = GetFocus();
    if (pwnd->IsKindOf(RUNTIME_CLASS(CEdit)))
    {
        // If the Find Tool is active we must preserve focus and selection
        int start, end;
        ((CEdit *)pwnd)->GetSel(start, end);
        SetActivePage(p_page_text_);        // Show text search page
        pwnd->SetFocus();
        ((CEdit *)pwnd)->SetSel(start, end);
    }
    else
    {
        SetActivePage(p_page_text_);        // Show text search page
        pwnd->SetFocus();
    }

    p_page_text_->UpdateData();
    combined_string_ = text_string_ = ss;
    HexFromText(ss);                    // Convert text to hex using current char set
    p_page_text_->UpdateData(FALSE);
    p_page_text_->ctl_text_string_.SetWindowText(ss);

    // Set current search string so Find Tool(s) match
    CString temp;
    if (GetMatchCase())
        temp = CString("=") + ss;
    else
        temp = CString("~") + ss;
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
}

// Change current hex search from outside the dialog
void CFindSheet::NewHex(LPCTSTR ss)
{
    CWnd *pwnd = GetFocus();
    if (pwnd->IsKindOf(RUNTIME_CLASS(CEdit)))
    {
        // If the Find Tool is active we must preserve focus and selection
        int start, end;
        ((CEdit *)pwnd)->GetSel(start, end);
        SetActivePage(p_page_hex_);
        pwnd->SetFocus();
        ((CEdit *)pwnd)->SetSel(start, end);
    }
    else
    {
        SetActivePage(p_page_hex_);
        pwnd->SetFocus();
    }

    p_page_hex_->UpdateData();          // Get current value before changing some of them

    combined_string_ = hex_string_ = ss;

    // hex_string_ has not had the case of the last entered character fixed yet
    if (theApp.hex_ucase_)
        combined_string_.MakeUpper();
    else
        combined_string_.MakeLower();

    // We need to call FixHexString to get the updated mask but not change hex_string_
    FixHexString();
    TextFromHex(hex_string_, mask_string_);
//    if (p_page_hex_->m_hWnd != 0)
    ASSERT(p_page_hex_->GetDlgItem(IDC_FIND_MASK) != NULL);
    p_page_hex_->GetDlgItem(IDC_FIND_MASK)->SetWindowText(mask_string_);
    p_page_hex_->UpdateData(FALSE);     // Update the dialog to match
    p_page_hex_->ctl_hex_string_.SetWindowText(hex_string_);

    // Set current search string so Find Tool(s) match Find Dialog
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(ss);
}

void CFindSheet::NewReplace(LPCTSTR ss)
{
    SetActivePage(p_page_replace_);
//    p_page_replace_->UpdateData();
    replace_string_ = ss;
    ASSERT(p_page_replace_->GetDlgItem(IDC_FIND_REPLACE_STRING) != NULL);
    p_page_replace_->GetDlgItem(IDC_FIND_REPLACE_STRING)->SetWindowText(ss);
    ((CMainFrame *)AfxGetMainWnd())->SetReplaceString(ss);
}

// Convert hex bytes as string to text using current char set (some byte values may be lost eg 00)
void CFindSheet::TextFromHex(LPCTSTR ss, LPCTSTR mask)
{
    if (strlen(ss) == 0)
    {
        text_string_.Empty();
        return;
    }

    BOOL bad_chars = FALSE;             // Any invalid ASCII/EBCDIC/Unicode chars?
    const char *pp;                     // Ptr to input (hex digit) string

    char *out = new char[strlen(ss)/2+3]; // Where output chars are stored
    size_t length;                      // Number of hex output nybbles generated
    unsigned int curr;                  // Output character value
    char *dd = out;                     // Ptr to current output char

    for (pp = ss, length = 0, curr = 0; *pp != '\0'; ++pp)
    {
        // Ignore spaces (and anything else)
        if (!isxdigit(*pp))
            continue;

        curr = (curr<<4) + (isdigit(*pp) ? *pp - '0' : toupper(*pp) - 'A' + 10);

        length++;                       // Got one more hex digit
        if (charset_ == RB_CHARSET_ASCII && (length % 2) == 0)
        {
            // Allow any byte value above CR
            if (curr > '\r')
            {
                *dd++ = curr;           // Printable so move to next byte
            }
            else
            {
                length -= 2;            // Forget byte (2 hex digits) just seen
                bad_chars = TRUE;
            }
            curr = 0;                   // Start next char
        }
        else if (charset_ == RB_CHARSET_UNICODE && (length % 4) == 0)  // Get 2 bytes for Unicode
        {
            // Swap bytes and convert wide character to MBCS
            if (wctomb(dd, (wchar_t)((curr>>8)&0xFF|(curr<<8)&0xFF00)) == 1)
                ++dd;                   // Keep 1 byte MBCS (= ASCII char)
            else
            {
                length -= 4;
                bad_chars = TRUE;       // Not translateable or > 1 byte MBCS char
            }
            curr = 0;                   // Start next wide char
        }
        else if (charset_ == RB_CHARSET_EBCDIC && (length % 2) == 0)
        {
            // Only allow valid EBCDIC characters
            if ((*dd = e2a_tab[curr]) != '\0') 
                ++dd;
            else
            {
                length -= 2;    // Forget byte (2 hex digits) just seen
                bad_chars = TRUE;
            }
            curr = 0;                   // Start next char
        }
    }
    if (charset_ == RB_CHARSET_UNICODE)
    {
        if ((length%4) != 0) bad_chars = TRUE;
        length = length/4;
    }
    else
    {
        if ((length%2) != 0) bad_chars = TRUE;
        length = length/2;
    }
    out[length] = '\0';

    text_string_ = out;
    delete[] out;
#if 0
    if (bad_chars && charset_ == RB_CHARSET_ASCII)
        AfxMessageBox("ASCII control character(s) ignored");
    else if (bad_chars && charset_ == RB_CHARSET_UNICODE)
        AfxMessageBox("Undisplayable Unicode character(s) ignored");
    else if (bad_chars && charset_ == RB_CHARSET_EBCDIC)
        AfxMessageBox("Invalid EBCDIC character(s) ignored");
#endif
}

// Convert text to hex digits using current char set
void CFindSheet::HexFromText(const char *ss, size_t len /*=-1*/)
{
    size_t length = len == -1 ?  strlen(ss) : len;

    if (length == 0)
    {
        hex_string_.Empty();
        mask_string_.Empty();
        return;
    }

    char *out_hex = new char[length*6+1];  // where hex string is built
    char *out_msk = new char[length*6+1];  // where mask is built
    char *po1 = out_hex;
    char *po2 = out_msk;
    const char *pp;                         // Ptr into input (ASCII char) string
    BOOL bad_chars = FALSE;                 // Any unrecognised characters for the char set

    // Display hex in upper or lower case?
    const char *hex_fmt;
    if (theApp.hex_ucase_)
        hex_fmt = "%02X ";
    else
        hex_fmt = "%02x ";

    for (pp = ss; *pp != '\0'; ++pp)
    {
        int out_count = 0;

        if (charset_ == RB_CHARSET_ASCII)
        {
            sprintf(po1, hex_fmt, (unsigned char)*pp);
            po1 += 3;
            out_count = 1;
        }
        else if (charset_ == RB_CHARSET_UNICODE)
        {
            wchar_t ww;

            // Convert ASCII character to wide char then to hex digits
            if (mbtowc(&ww, pp, 1) == 1)
            {
                sprintf(po1, hex_fmt, ww & 0xFF);
                po1 += 3;
                sprintf(po1, hex_fmt, (ww>>8) & 0xFF);
                po1 += 3;
                out_count = 2;
            }
        }
        else if (charset_ == RB_CHARSET_EBCDIC)
        {
            if (*pp < 128 && *pp >= 0 && a2e_tab[*pp] != '\0')
            {
                sprintf(po1, hex_fmt, a2e_tab[(unsigned char)*pp]);
                po1 += 3;
                out_count = 1;
            }
        }
        else
            ASSERT(0);

        // Add the mask characters
        while (out_count--)
        {
            if (wildcards_allowed_ && *pp == wildcard_char_[0])
                strcpy(po2, "00 ");
            else if (theApp.hex_ucase_)
                strcpy(po2, "FF ");
            else
                strcpy(po2, "FF ");
            po2 += 3;
        }
    }
    if (po1 > out_hex)
    {
        // Forget trailing space
        po1--;
        po2--;
    }
    *po1 = *po2 = '\0';                 // Terminate string

    hex_string_ = out_hex;
    mask_string_ = out_msk;
    delete[] out_hex;
    delete[] out_msk;
}

// Make sure the hex string and the associated mask string are nicely formatted
void CFindSheet::FixHexString()
{
    int count;
    char *out_hex = new char[hex_string_.GetLength()*3/2+2];
    char *out_msk = new char[__max(hex_string_.GetLength(), mask_string_.GetLength())*3/2+2];
    char *po1 = out_hex;
    char *po2 = out_msk;

    const char *p1 = hex_string_;
    const char *p2 = mask_string_;

    for (count = 0; ; ++count)
    {
        // Skip unrecognised characters in both strings
        while (*p1 != '\0' && !isxdigit(*p1) && *p1 != '?')
            ++p1;
        while (*p2 != '\0' && !isxdigit(*p2))
            ++p2;

        // Finished if end of hex string and rest of mask contains nothing but F's
        if (*p1 == '\0' && (*p2 == '\0' || strcspn(p2, "0123456789abcdefABCDEF") == strlen(p2)))
            break;

        // If we haven't reached the end of the hex string put out the digit in correct case
        if (*p1 != '\0')
        {
            if (theApp.hex_ucase_)
                *po1++ = toupper(*p1);
            else
                *po1++ = tolower(*p1);

            // Put a space between every pair of digits
            if (count%2 == 1)
                *po1++ = ' ';
        }

        if (*p1 == '?')                             // ? matches any digit so use mask of zero
        {
            wildcards_allowed_ = TRUE;
            *po2++ = '0';
        }
        else if (*p1 != '\0' && *p2 == '\0')        // End of mask but not end of search so pad with F's
            *po2++ = theApp.hex_ucase_ ? 'F' : 'f';
        else if (theApp.hex_ucase_)
            *po2++ = toupper(*p2);
        else
            *po2++ = tolower(*p2);

        if (count%2 == 1)
            *po2++ = ' ';

        // Move the ptr forward in both strings
        if (*p1 != '\0')
            ++p1;
        if (*p2 != '\0')
            ++p2;
    }

    *po1 = *po2 = '\0';

    hex_string_ = out_hex;
    mask_string_ = out_msk;

    // If a question mark was typed we want to turn on wildcard searches
    if (wildcards_allowed_ && GetActivePage() == p_page_hex_)
    {
        ASSERT(p_page_hex_->GetDlgItem(IDC_FIND_USE_MASK) != NULL);
        ((CButton *)p_page_hex_->GetDlgItem(IDC_FIND_USE_MASK))->SetCheck(1);
    }
//    GetActivePage()->UpdateData(FALSE);
}

void CFindSheet::AdjustPrefix(LPCTSTR pin, BOOL underscore_space /*=FALSE*/)
{
    if (prefix_entered_) return;        // Don't override what the user has entered

    char buf[16];
    char *pout;

    for (pout = buf; *pin != '\0' && pout < buf+10; ++pin)
    {
        if (isalnum(*pin) || strchr("+-_$@.", *pin) != NULL)
            *pout++ = *pin;
        else if (underscore_space && isspace(*pin) && pout > buf && isalnum(*(pout-1)))
            *pout++ = '_';
    }
    if (pout > buf && isdigit(*(pout-1)))
        *pout++ = '_';
    *pout = '\0';

    if (pout == buf)
        bookmark_prefix_ = "Search";
    else
        bookmark_prefix_ = buf;

    CPropertyPage *pp = GetActivePage();
#if 0
    if (pp->GetDlgItem(IDC_FIND_BOOKMARK_PREFIX) != NULL)
        pp->GetDlgItem(IDC_FIND_BOOKMARK_PREFIX)->SetWindowText(bookmark_prefix_);
#else
    pp->UpdateData(FALSE);              // Put the new prefix string into the control
#endif
}

bool CFindSheet::HexReplace() const
{
    return StringType(combined_string_) != STRING_TEXT &&
           StringType(replace_string_)  != STRING_TEXT;
}

void CFindSheet::GetReplace(unsigned char **pps, size_t *plen)
{
    CPropertyPage *pp = GetActivePage();

    // We must be displaying the replace page otherwise the type of search is confiusing.
    // Eg if find string (combined_string_) could be for hex search but replace_ string is text then we have
    // to do a text (not hex) search but on the Simple page we should do a hex search.
    ASSERT(pp != NULL && pp == p_page_replace_);
    pp->UpdateData();

    if (replace_buf_ != NULL)
    {
        delete replace_buf_;
        replace_buf_ = NULL;
    }

    if (HexReplace())
    {
        // Hex search (no mask)
        replace_buf_ = new unsigned char[replace_string_.GetLength()/2+2];

        size_t length;
        const char *pp;
        unsigned char *dd;
        for (pp = replace_string_, dd = replace_buf_, length = 0, *dd = '\0'; *pp != '\0'; ++pp)
        {
            // Ignore spaces (and anything except hex digits)
            if (!isxdigit(*pp))
                continue;

            *dd = (*dd<<4) + (isdigit(*pp) ? *pp - '0' : toupper(*pp) - 'A' + 10);

            if ((++length % 2) == 0)
                *(++dd) = '\0';         // Move to next byte and zero it
        }

        *pps = replace_buf_;
        *plen = length/2;
    }
    else
    {
        // Not hex mode search as at least one string is not hex
        *pps = (unsigned char *)((const char *)replace_string_);
        *plen = replace_string_.GetLength();
    }
}

void CFindSheet::GetSearch(const unsigned char **pps, const unsigned char **ppmask, size_t *plen)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    if (search_buf_ != NULL)
    {
        delete search_buf_;
        search_buf_ = NULL;
    }
    if (mask_buf_ != NULL)
    {
        delete mask_buf_;
        mask_buf_ = NULL;
    }

    if (pp == p_page_simple_ && StringType(combined_string_) != STRING_HEX ||
        pp == p_page_replace_ && !HexReplace())
    {
        CHexEditView *pview = GetView();

        if (pview == NULL || !pview->EbcdicMode())
        {
            // Text search (no wildcards)
            *pps = (const unsigned char *)((const char *)combined_string_);
            *ppmask = NULL;
            *plen = combined_string_.GetLength();
        }
        else
        {
            search_buf_ = new unsigned char[combined_string_.GetLength()+1];
            unsigned char *po = search_buf_;

            // Convert text to EBCDIC bytes
            for (const char *pp = combined_string_; *pp != '\0'; ++pp)
                if (*pp < 128 && *pp >= 0 && a2e_tab[*pp] != '\0')
                    *po++ = a2e_tab[*pp];
            *pps = search_buf_;
            *ppmask = NULL;
            *plen = po - search_buf_;
        }
    }
    else if (pp == p_page_simple_ || pp == p_page_replace_)
    {
        // Hex search (no mask)
        search_buf_ = new unsigned char[combined_string_.GetLength()/2+2];

        size_t length;
        const char *pp;
        unsigned char *dd;
        for (pp = combined_string_, dd = search_buf_, length = 0, *dd = '\0'; *pp != '\0'; ++pp)
        {
            // Ignore spaces (and anything else)
            if (!isxdigit(*pp))
                continue;

            *dd = (*dd<<4) + (isdigit(*pp) ? *pp - '0' : toupper(*pp) - 'A' + 10);

            if ((++length % 2) == 0)
                *(++dd) = '\0';         // Move to next byte and zero it
        }

        *pps = search_buf_;
        *ppmask = NULL;
        *plen = length/2;
    }
    else if (pp == p_page_hex_ || pp == p_page_number_)
    {
        search_buf_ = new unsigned char[hex_string_.GetLength()/2+2];
        if (pp != p_page_number_ && 
            wildcards_allowed_ && 
            strcspn(mask_string_, "0123456789abcdefABCDEF") < mask_string_.GetLength())
        {
            mask_buf_ = new unsigned char[hex_string_.GetLength()/2+2];
        }

        size_t length;
        const char *p1 = hex_string_;
        const char *p2 = mask_string_;
        unsigned char *po1 = search_buf_;
        unsigned char *po2 = mask_buf_;

        *po1 = '\0';
        if (mask_buf_ != NULL) *po2 = '\0';

        for (length = 0; *p1 != '\0'; ++p1)
        {
            // Skip any rubbish in each string
            while (*p1 != '\0' && !(isxdigit(*p1) || *p1 == '?' && mask_buf_ != NULL))
                ++p1;
            while (*p2 != '\0' && !isxdigit(*p2))
                ++p2;

            if (*p1 == '\0')
            {
                break;
            }
            else if (*p1 == '?')
            {
                // Set nybble to match anything
                ASSERT(mask_buf_ != NULL);
                *po1 = (*po1 << 4);
                *po2 = (*po2 << 4);
            }
            else
            {
                // Get hex digit
                *po1 = (*po1<<4) + (isdigit(*p1) ? *p1 - '0' : toupper(*p1) - 'A' + 10);
                if (mask_buf_ != NULL)
                {
                    if (isxdigit(*p2))
                        *po2 = (*po2<<4) + (isdigit(*p2) ? *p2 - '0' : toupper(*p2) - 'A' + 10);
                    else
                        *po2 = (*po2<<4) + 0xF;
                }
            }

            // Next output nybble
            if ((++length%2) == 0)
            {
                *(++po1) = '\0';    // Move to next byte and zero it
                if (mask_buf_ != NULL)
                    *(++po2) = '\0';
            }

            if (mask_buf_ != NULL && *p2 != '\0') ++p2;
        }
        *pps = search_buf_;
        *ppmask = mask_buf_;
        *plen = ++length/2;
    }
    else if (pp == p_page_text_ && (!wildcards_allowed_ || strchr(text_string_, wildcard_char_[0]) == NULL))
    {
        // Text search (no wildcards)
        if (charset_ == RB_CHARSET_ASCII)
        {
            *pps = (const unsigned char *)((const char *)text_string_);
            *ppmask = NULL;
            *plen = text_string_.GetLength();
        }
        else if (charset_ == RB_CHARSET_UNICODE)
        {
            search_buf_ = new unsigned char[text_string_.GetLength()*2+2];
            unsigned char *po = search_buf_;

            // Convert ASCII characters to wide chars
            for (const char *pp = text_string_; *pp != '\0'; ++pp)
            {
                wchar_t ww;

                if (mbtowc(&ww, pp, 1) == 1)
                {
                    *po++ = ww & 0xFF;
                    *po++ = (ww>>8) & 0xFF;
                }
            }
            *pps = search_buf_;
            *ppmask = NULL;
            *plen = po - search_buf_;

        }
        else if (charset_ == RB_CHARSET_EBCDIC)
        {
            search_buf_ = new unsigned char[text_string_.GetLength()+1];
            unsigned char *po = search_buf_;

            // Convert text to EBCDIC bytes
            for (const char *pp = text_string_; *pp != '\0'; ++pp)
                if (*pp < 128 && *pp >= 0 && a2e_tab[*pp] != '\0')
                    *po++ = a2e_tab[*pp];
            *pps = search_buf_;
            *ppmask = NULL;
            *plen = po - search_buf_;
        }
    }
    else
    {
        ASSERT(pp == p_page_text_);
        if (charset_ == RB_CHARSET_ASCII)
        {
            mask_buf_ = new unsigned char[text_string_.GetLength()];
            memset(mask_buf_, '\xFF', text_string_.GetLength());

            for (const char *pp = text_string_; (pp = strchr(pp, wildcard_char_[0])) != NULL; ++pp)
                mask_buf_[pp - (const char *)text_string_] = '\0';

            *pps = (const unsigned char *)((const char *)text_string_);
            *ppmask = mask_buf_;
            *plen = text_string_.GetLength();
        }
        else if (charset_ == RB_CHARSET_UNICODE)
        {
            search_buf_ = new unsigned char[text_string_.GetLength()*2+2];
            mask_buf_   = new unsigned char[text_string_.GetLength()*2+2];
            unsigned char *po = search_buf_;
            unsigned char *pom = mask_buf_;

            // Convert ASCII characters to wide chars
            for (const char *pp = text_string_; *pp != '\0'; ++pp)
            {
                wchar_t ww;

                if (mbtowc(&ww, pp, 1) == 1)
                {
                    *po++ = ww & 0xFF;
                    *po++ = (ww>>8) & 0xFF;
                    if (*pp == wildcard_char_[0])
                    {
                        *pom++ = '\0';
                        *pom++ = '\0';
                    }
                    else
                    {
                        *pom++ = '\xFF';
                        *pom++ = '\xFF';
                    }
                }
            }
            *pps = search_buf_;
            *ppmask = mask_buf_;
            *plen = po - search_buf_;

        }
        else if (charset_ == RB_CHARSET_EBCDIC)
        {
            search_buf_ = new unsigned char[text_string_.GetLength()+1];
            mask_buf_   = new unsigned char[text_string_.GetLength()+1];
            unsigned char *po = search_buf_;
            unsigned char *pom = mask_buf_;

            // Convert text to EBCDIC bytes
            for (const char *pp = text_string_; *pp != '\0'; ++pp)
            {
                if (*pp < 128 && *pp >= 0 && a2e_tab[*pp] != '\0')
                {
                    *po++ = a2e_tab[*pp];

                    if (*pp == wildcard_char_[0])
                        *pom++ = '\0';
                    else
                        *pom++ = '\xFF';
                }
            }
            *pps = search_buf_;
            *ppmask = mask_buf_;
            *plen = po - search_buf_;
        }
    }
}

static union
{
    _int64 as_int;
    struct
    {
        unsigned int scope :3;          // Only need 4 now, but allow for 8 (2^3) just in case
        unsigned int whole_word :1;
        unsigned int match_case :1;
        unsigned int direction :1;
        unsigned int charset :2;

        unsigned int wildcards_allowed :1;
        unsigned int big_endian :1;
        unsigned int number_format :3;
        unsigned int number_size :3;

        unsigned int wildcard_char :8;

        unsigned int alignment :7;      // Currently only 1, 2, 4, or 8
		unsigned int rel_mark :1;       // Alignment is relative to mark in active view
        unsigned int offset :7;         // Only in range [0, alignment)
		// one spare bit!
    };
} all_options;

__int64 CFindSheet::GetOptions()
{
    ASSERT(sizeof(all_options) == 8);
    all_options.as_int = 0;

    all_options.scope = scope_;
    all_options.whole_word = whole_word_;
    all_options.match_case = match_case_;
    all_options.direction = direction_;
    ASSERT(charset_ != RB_CHARSET_UNKNOWN);
    all_options.charset = charset_;

    all_options.wildcards_allowed = wildcards_allowed_;
    all_options.wildcard_char = wildcard_char_[0];

    all_options.big_endian = big_endian_;
    all_options.number_format = number_format_;
    all_options.number_size = number_size_;

    // Store alignment as actual value for possible future enhancement
    switch (alignment_)
    {
    default:
        ASSERT(0);
        // fall through
    case ALIGN_BYTE:
        all_options.alignment = 1;
        break;
    case ALIGN_WORD:
        all_options.alignment = 2;
        break;
    case ALIGN_DWORD:
        all_options.alignment = 4;
        break;
    case ALIGN_QWORD:
        all_options.alignment = 8;
        break;
    }
	ASSERT(offset_ < all_options.alignment);
	all_options.offset = offset_;

    return all_options.as_int;
}

void CFindSheet::SetOptions(__int64 val)
{
    ASSERT(sizeof(all_options) == 8);

    all_options.as_int = val;
    ASSERT(all_options.offset == 0);     // not yet used

    scope_ = scope_t(all_options.scope);
    whole_word_ = all_options.whole_word;
    match_case_ = all_options.match_case;
    direction_ = dirn_t(all_options.direction);
    charset_ = charset_t(all_options.charset);  // xxx handle unknown ?

    wildcards_allowed_ = all_options.wildcards_allowed;
    wildcard_char_ = CString(TCHAR(all_options.wildcard_char));

    big_endian_ = all_options.big_endian;
    number_format_ = all_options.number_format;
    number_size_ = all_options.number_size;

	offset_ = all_options.offset;
	if (offset_ >= all_options.alignment)
	{
		ASSERT(0);
		offset_ = 0;
	}
    switch (all_options.alignment)
    {
    default:
		offset_ = 0;
        ASSERT(0);
        // fall through
    case 1:
        alignment_ = ALIGN_BYTE;
        break;
    case 2:
        alignment_ = ALIGN_WORD;
        break;
    case 4:
        alignment_ = ALIGN_DWORD;
        break;
    case 8:
        alignment_ = ALIGN_QWORD;
        break;
    }

    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData(FALSE);
}

BOOL CFindSheet::GetWholeWord() const
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    // No whole word matching if not supported by the page
    if (pp != p_page_simple_ && pp != p_page_text_ && pp != p_page_replace_)
        return FALSE;

    // No whole word matching if in hex mode
    if (pp == p_page_simple_ && StringType(combined_string_) != STRING_TEXT ||
        pp == p_page_replace_ && HexReplace())
        return FALSE;

    return whole_word_;
}

BOOL CFindSheet::GetMatchCase() const
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    // Exact compare if case-insensitive compare is not supported by the page
    if (pp != p_page_simple_ && pp != p_page_text_ && pp != p_page_replace_)
        return TRUE;

    // Exact compare if in hex mode
    if (pp == p_page_simple_ && StringType(combined_string_) != STRING_TEXT ||
        pp == p_page_replace_ && HexReplace())
        return TRUE;

    return match_case_;
}

BOOL CFindSheet::GetWildcardsAllowed() const
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    if (pp != p_page_hex_ && pp != p_page_text_)
        return FALSE;
    
    return wildcards_allowed_;
}

CFindSheet::charset_t CFindSheet::GetCharSet() const
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

#ifndef DIALOG_BAR
    if (!((CMainFrame *)AfxGetMainWnd())->m_wndFind.visible_ || pp != p_page_text_)
#else
    if ((((CMainFrame *)AfxGetMainWnd())->m_wndFind.GetStyle() & WS_VISIBLE) != 0 || 
		pp != p_page_text_)
#endif
    {
        CHexEditView *pview = GetView();
        if (pview == NULL || !pview->EbcdicMode())
            return RB_CHARSET_ASCII;
        else
            return RB_CHARSET_EBCDIC;
    }
    
    return charset_;
}

int CFindSheet::GetAlignment()
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

#ifndef DIALOG_BAR
    if (((CMainFrame *)AfxGetMainWnd())->m_wndFind.visible_ && (pp == p_page_hex_ || pp == p_page_number_))
#else
    if ((((CMainFrame *)AfxGetMainWnd())->m_wndFind.GetStyle() & WS_VISIBLE) != 0 && 
        (pp == p_page_hex_ || pp == p_page_number_))
#endif
    {
        switch (alignment_)
        {
        case ALIGN_BYTE:
            return 1;
        case ALIGN_WORD:
            return 2;
        case ALIGN_DWORD:
            return 4;
        case ALIGN_QWORD:
            return 8;
        }
    }
    return 1;
}

int CFindSheet::GetOffset()
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

#ifndef DIALOG_BAR
    if (((CMainFrame *)AfxGetMainWnd())->m_wndFind.visible_ && (pp == p_page_hex_ || pp == p_page_number_))
#else
    if ((((CMainFrame *)AfxGetMainWnd())->m_wndFind.GetStyle() & WS_VISIBLE) != 0 && 
        (pp == p_page_hex_ || pp == p_page_number_))
#endif
    {
		// xxx
    }
#ifdef _DEBUG  // hard code value till we add dialog control
	return 1; // xxx
#else
	return 0;
#endif
    return offset_;
}

bool CFindSheet::AlignMark()
{
#ifdef _DEBUG  // hard code value till we add dialog control
	return true; // xxx
#else
	return false;
#endif
}

void CFindSheet::Redisplay()            // Make sure hex digits case OK etc
{
    // Get current values of controls so we don't lose anything when we update the text box
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    if (theApp.hex_ucase_)
    {
#if 0 // we don't worry about case of hex digits in these
        if (pp == p_page_replace_ && HexReplace() ||
            StringType(combined_string_) == STRING_HEX)
        {
            combined_string_.MakeUpper();
            replace_string_.MakeUpper();
        }
#endif
        hex_string_.MakeUpper();
        mask_string_.MakeUpper();
    }
    else
    {
#if 0 // we don't worry about case of hex digits in these
        if (pp == p_page_replace_ && HexReplace() ||
            StringType(combined_string_) == STRING_HEX)
        {
            combined_string_.MakeLower();
            replace_string_.MakeLower();
        }
#endif
        hex_string_.MakeLower();
        mask_string_.MakeLower();
    }

    // Update all combo controls that don't have DDX and can contain hex strings
    p_page_hex_->ctl_hex_string_.SetWindowText(hex_string_);
#if 0 // we don't worry about case of hex digits in these
    p_page_simple_->ctl_string_.SetWindowText(combined_string_);
    p_page_replace_->ctl_string_.SetWindowText(combined_string_);
    p_page_replace_->ctl_replace_string_.SetWindowText(replace_string_);
#endif
    // Fix controls (mask?) in the active page (controls in other pages are fixed in CPropertyPage::OnSetActive)
    pp->UpdateData(FALSE);
}

// Add to text search histories
void CFindSheet::AddText(LPCTSTR txt)
{
    // Only add to ctl if the page has been set up (OnInitDialog), otherwise we
    // assume that the history will be set up when OnInitDialog is actually called.
    if (p_page_text_->m_hWnd != HWND(0))
        p_page_text_->ctl_text_string_.AddString(txt);

    if (p_page_simple_->m_hWnd != HWND(0))
        p_page_simple_->ctl_string_.AddString(txt);
}

// Add to hex search histories
void CFindSheet::AddHex(LPCTSTR txt)
{
    // Only add to ctl if the page has been set up (OnInitDialog), otherwise we
    // assume that the history will be set up when OnInitDialog is actually called.
    if (p_page_hex_->m_hWnd != HWND(0))
        p_page_hex_->ctl_hex_string_.AddString(txt);

    if (p_page_simple_->m_hWnd != HWND(0))
        p_page_simple_->ctl_string_.AddString(txt);
}

void CFindSheet::SetWholeWord(BOOL ww)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    whole_word_ = ww;

    // Fix controls in the active page (controls in other pages are fixed in CPropertyPage::OnSetActive)
    pp->UpdateData(FALSE);
}

void CFindSheet::SetMatchCase(BOOL match)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    match_case_ = match;

    // Fix controls in the active page (controls in other pages are fixed in CPropertyPage::OnSetActive)
    pp->UpdateData(FALSE);
}

void CFindSheet::SetWildcardsAllowed(BOOL wa)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    wildcards_allowed_ = wa;

    pp->UpdateData(FALSE);
}

void CFindSheet::SetDirn(dirn_t dirn)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    direction_ = dirn;

    // Fix controls in the active page
    pp->UpdateData(FALSE);

    // If this page has scope controls (all currently do) then update the "to EOF" radio button text
    if (pp->GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL)
        pp->GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(direction_ == DIRN_UP ? "To start of file" : "To end of file");
}

void CFindSheet::SetScope(scope_t scope)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    scope_ = scope;

    // Fix controls in the active page
    pp->UpdateData(FALSE);
}

void CFindSheet::SetCharSet(charset_t cs)
{
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    charset_ = cs;

    // Fix controls in the active page (may do nothing)
    pp->UpdateData(FALSE);
}

void CFindSheet::SetAlignment(int aa)
{
    switch (aa)
    {
    default:
        ASSERT(0);
        // fall through
    case 1:
        alignment_ = ALIGN_BYTE;
        break;
    case 2:
        alignment_ = ALIGN_WORD;
        break;
    case 4:
        alignment_ = ALIGN_DWORD;
        break;
    case 8:
        alignment_ = ALIGN_QWORD;
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CFindSheet message handlers

BOOL CFindSheet::OnNcCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (!CPropertySheet::OnNcCreate(lpCreateStruct))
		return FALSE;
	
    ModifyStyleEx(0, WS_EX_CONTEXTHELP);

	return TRUE;
}

LRESULT CFindSheet::OnKickIdle(WPARAM, LPARAM lCount)
{
    CHexEditView *pview;
    CPropertyPage *pp = GetActivePage();
    ASSERT(pp != NULL);
    pp->UpdateData();

    CMainFrame *mm = (CMainFrame *)AfxGetMainWnd();
    std::vector<CString>::const_iterator ph;
    std::vector<CString> hex_hist;
    std::vector<CString> text_hist;
    std::vector<CString> both_hist;

    // Build list of hex strings from history
    for (ph = mm->search_hist_.begin(); ph != mm->search_hist_.end(); ++ph)
    {
        if (ph->IsEmpty())
        {
            ASSERT(0);
        }
        else if ((*ph)[0] == '~' || (*ph)[0] == '=')
        {
            CString temp = ph->Mid(1);
            text_hist.push_back(temp);
            both_hist.push_back(temp);
        }
        else
        {
            hex_hist.push_back(*ph);
            both_hist.push_back(*ph);
        }
    }

    if (pp == p_page_simple_)
    {
        // Update history list with all searches if nec.
        if (mm->ComboNeedsUpdate(both_hist, &p_page_simple_->ctl_string_))
        {
            // Save current edit control text/selection
            CString strCurr;
            p_page_simple_->ctl_string_.GetWindowText(strCurr);
            DWORD sel = p_page_simple_->ctl_string_.GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far

            // Get a DC to work out max width of drop window
            CClientDC dc(&p_page_simple_->ctl_string_);
            int nSave = dc.SaveDC();
            dc.SelectObject(p_page_simple_->ctl_string_.GetFont());

            // Update the strings in the list (and work out the width of the longest)
            p_page_simple_->ctl_string_.ResetContent();
            for (ph = both_hist.begin(); ph != both_hist.end(); ++ph)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ph).cx);

                // Add the string to the list
                p_page_simple_->ctl_string_.InsertString(0, *ph);
            }

            // Restore edit control text and selection
            p_page_simple_->ctl_string_.SetWindowText(strCurr);
            p_page_simple_->ctl_string_.SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin (and possible scrollbar) and set dropped window width
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            p_page_simple_->ctl_string_.SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }
    }
    else if (pp == p_page_hex_)
    {
        if (mm->ComboNeedsUpdate(hex_hist, &p_page_hex_->ctl_hex_string_))
        {
            // Save current edit control text/selection
            CString strCurr;
            p_page_hex_->ctl_hex_string_.GetWindowText(strCurr);
            DWORD sel = p_page_hex_->ctl_hex_string_.GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far

            // Get a DC to work out max width of drop window
            CClientDC dc(&p_page_hex_->ctl_hex_string_);
            int nSave = dc.SaveDC();
            dc.SelectObject(p_page_hex_->ctl_hex_string_.GetFont());

            // Update the strings in the list (and work out the width of the longest)
            p_page_hex_->ctl_hex_string_.ResetContent();
            for (ph = hex_hist.begin(); ph != hex_hist.end(); ++ph)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ph).cx);

                // Add the string to the list
                p_page_hex_->ctl_hex_string_.InsertString(0, *ph);
            }

            // Restore edit control text and selection
            p_page_hex_->ctl_hex_string_.SetWindowText(strCurr);
            p_page_hex_->ctl_hex_string_.SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin (and possible scrollbar) and set dropped window width
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            p_page_hex_->ctl_hex_string_.SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }
    }
    else if (pp == p_page_text_)
    {
        if (mm->ComboNeedsUpdate(text_hist, &p_page_text_->ctl_text_string_))
        {
            // Save current edit control text/selection
            CString strCurr;
            p_page_text_->ctl_text_string_.GetWindowText(strCurr);
            DWORD sel = p_page_text_->ctl_text_string_.GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far

            // Get a DC to work out max width of drop window
            CClientDC dc(&p_page_text_->ctl_text_string_);
            int nSave = dc.SaveDC();
            dc.SelectObject(p_page_text_->ctl_text_string_.GetFont());

            // Update the strings in the list (and work out the width of the longest)
            p_page_text_->ctl_text_string_.ResetContent();
            for (ph = text_hist.begin(); ph != text_hist.end(); ++ph)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ph).cx);

                // Add the string to the list
                p_page_text_->ctl_text_string_.InsertString(0, *ph);
            }

            // Restore edit control text and selection
            p_page_text_->ctl_text_string_.SetWindowText(strCurr);
            p_page_text_->ctl_text_string_.SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin (and possible scrollbar) and set dropped window width
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            p_page_text_->ctl_text_string_.SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }
    }
    else if (pp == p_page_replace_)
    {
        if (mm->ComboNeedsUpdate(both_hist, &p_page_replace_->ctl_string_))
        {
            // Save current edit control text/selection
            CString strCurr;
            p_page_replace_->ctl_string_.GetWindowText(strCurr);
            DWORD sel = p_page_replace_->ctl_string_.GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far

            // Get a DC to work out max width of drop window
            CClientDC dc(&p_page_replace_->ctl_string_);
            int nSave = dc.SaveDC();
            dc.SelectObject(p_page_replace_->ctl_string_.GetFont());

            // Update the strings in the list (and work out the width of the longest)
            p_page_replace_->ctl_string_.ResetContent();
            for (ph = both_hist.begin(); ph != both_hist.end(); ++ph)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ph).cx);

                // Add the string to the list
                p_page_replace_->ctl_string_.InsertString(0, *ph);
            }

            // Restore edit control text and selection
            p_page_replace_->ctl_string_.SetWindowText(strCurr);
            p_page_replace_->ctl_string_.SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin (and possible scrollbar) and set dropped window width
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            p_page_replace_->ctl_string_.SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }

        if (mm->ComboNeedsUpdate(mm->replace_hist_, &p_page_replace_->ctl_replace_string_))
        {
            // Save current edit control text/selection
            CString strCurr;
            p_page_replace_->ctl_replace_string_.GetWindowText(strCurr);
            DWORD sel = p_page_replace_->ctl_replace_string_.GetEditSel();
            int max_str = 0;                // Max width of all the strings added so far

            // Get a DC to work out max width of drop window
            CClientDC dc(&p_page_replace_->ctl_replace_string_);
            int nSave = dc.SaveDC();
            dc.SelectObject(p_page_replace_->ctl_replace_string_.GetFont());

            // Update the strings in the list (and work out the width of the longest)
            p_page_replace_->ctl_replace_string_.ResetContent();
            for (ph = mm->replace_hist_.begin(); ph != mm->replace_hist_.end(); ++ph)
            {
                max_str = __max(max_str, dc.GetTextExtent(*ph).cx);

                // Add the string to the list
                p_page_replace_->ctl_replace_string_.InsertString(0, *ph);
            }

            // Restore edit control text and selection
            p_page_replace_->ctl_replace_string_.SetWindowText(strCurr);
            p_page_replace_->ctl_replace_string_.SetEditSel(LOWORD(sel), HIWORD(sel));

            // Add space for margin (and possible scrollbar) and set dropped window width
            max_str += dc.GetTextExtent("0").cx + ::GetSystemMetrics(SM_CXVSCROLL);
            p_page_replace_->ctl_replace_string_.SetDroppedWidth(__min(max_str, 780));

            dc.RestoreDC(nSave);
        }
    }

#if 0
    // Enable/disable selection (scope) option depending on whether there is a selection in active view
    if (pp->GetDlgItem(IDC_FIND_SCOPE_SELECTION) != NULL &&
        visible_ &&
        (pview = GetView()) != NULL)
    {
        FILE_ADDRESS start_addr, end_addr;              // Current selection
        pview->GetSelAddr(start_addr, end_addr);
        if (start_addr < end_addr)
            pp->GetDlgItem(IDC_FIND_SCOPE_SELECTION)->EnableWindow(TRUE);
        else
        {
            if (scope_ == SCOPE_SEL)
            {
                scope_ = SCOPE_EOF;
                pp->UpdateData(FALSE);
            }
            pp->GetDlgItem(IDC_FIND_SCOPE_SELECTION)->EnableWindow(FALSE);
        }
    }
#endif
    // Enable/disable search to mark (scope) option depending on positions of caret and mark
    if (pp->GetDlgItem(IDC_FIND_SCOPE_TOMARK) != NULL &&
        (pview = GetView()) != NULL)
    {
        FILE_ADDRESS start_addr, end_addr;              // Current selection
        pview->GetSelAddr(start_addr, end_addr);
        FILE_ADDRESS mark_addr = pview->GetMark();

        if (direction_ == DIRN_UP && mark_addr < end_addr ||
            direction_ == DIRN_DOWN && mark_addr > start_addr)
        {
            pp->GetDlgItem(IDC_FIND_SCOPE_TOMARK)->EnableWindow(TRUE);
        }
        else
        {
            if (scope_ == SCOPE_TOMARK)
            {
                scope_ = SCOPE_EOF;
                pp->UpdateData(FALSE);
            }
            pp->GetDlgItem(IDC_FIND_SCOPE_TOMARK)->EnableWindow(FALSE);
        }
    }

    if (pp->GetDlgItem(IDC_FIND_NEXT) != NULL)
        pp->GetDlgItem(IDC_FIND_NEXT)->EnableWindow(GetView() != NULL);
    if (pp->GetDlgItem(IDC_FIND_REPLACE) != NULL)
        pp->GetDlgItem(IDC_FIND_REPLACE)->EnableWindow(GetView() != NULL);
    if (pp->GetDlgItem(IDC_FIND_REPLACE_ALL) != NULL)
        pp->GetDlgItem(IDC_FIND_REPLACE_ALL)->EnableWindow(GetView() != NULL);

    if (pp->GetDlgItem(IDC_FIND_BOOKMARK_ALL) != NULL && pp->GetDlgItem(IDC_FIND_BOOKMARK_PREFIX) != NULL)
    {
        // Enable Bookmark All button if bookmark prefix is not empty
        CString ss;
        pp->GetDlgItem(IDC_FIND_BOOKMARK_PREFIX)->GetWindowText(ss);
        pp->GetDlgItem(IDC_FIND_BOOKMARK_ALL)->EnableWindow(!ss.IsEmpty());
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CSimplePage property page

IMPLEMENT_DYNCREATE(CSimplePage, CPropertyPage)

CSimplePage::CSimplePage() : CPropertyPage(CSimplePage::IDD)
{
	//{{AFX_DATA_INIT(CSimplePage)
	//}}AFX_DATA_INIT
}

CSimplePage::~CSimplePage()
{
}

void CSimplePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimplePage)
	DDX_Control(pDX, IDC_FIND_COMBINED_STRING, ctl_string_);
	//}}AFX_DATA_MAP
//	DDX_CBString(pDX, IDC_FIND_COMBINED_STRING, pparent_->combined_string_);
	DDX_Radio(pDX, IDC_FIND_DIRN_UP, *(int *)(&pparent_->direction_));
	DDX_Check(pDX, IDC_FIND_WHOLE_WORD, pparent_->whole_word_);
	DDX_Check(pDX, IDC_FIND_MATCH_CASE, pparent_->match_case_);
//	DDX_Radio(pDX, IDC_FIND_SCOPE_SELECTION, *(int *)(&pparent_->scope_));
	DDX_Radio(pDX, IDC_FIND_SCOPE_TOMARK, *(int *)(&pparent_->scope_));
}

BEGIN_MESSAGE_MAP(CSimplePage, CPropertyPage)
	//{{AFX_MSG_MAP(CSimplePage)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_FIND_HELP, OnHelp)
	ON_BN_CLICKED(IDC_FIND_DIRN_DOWN, OnChangeDirn)
	ON_CBN_EDITCHANGE(IDC_FIND_COMBINED_STRING, OnChangeString)
	ON_CBN_SELCHANGE(IDC_FIND_COMBINED_STRING, OnSelchangeString)
	ON_BN_CLICKED(IDC_FIND_MATCH_CASE, OnChangeMatchCase)
	ON_BN_CLICKED(IDC_FIND_DIRN_UP, OnChangeDirn)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

// Determine whether to do a hex or text search by looking at the string entered.
// Then set the comments and controls appropriately.
void CSimplePage::FixType(BOOL fix_strings /*=FALSE*/)
{
    ASSERT(GetDlgItem(IDC_FIND_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_FIND_MESSAGE) != NULL);
    ASSERT(GetDlgItem(IDC_FIND_WHOLE_WORD) != NULL);
    ASSERT(GetDlgItem(IDC_FIND_MATCH_CASE) != NULL);

    switch (pparent_->StringType(pparent_->combined_string_))
    {
    case CFindSheet::STRING_TEXT:
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Text find:");
        GetDlgItem(IDC_FIND_MESSAGE)->SetWindowText(
            "Press Enter or click the \"Find Next\" button to search for the string.\n"
            "Use the \"Text\" page for advanced text search options.");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(TRUE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(TRUE);

        if (fix_strings)
        {
            // Set search strings in other pages
            pparent_->text_string_ = pparent_->combined_string_;
            pparent_->HexFromText(pparent_->combined_string_);   // Convert text to hex using current char set

            CString temp;
            if (pparent_->GetMatchCase())
                temp = CString("=") + pparent_->combined_string_;
            else
                temp = CString("~") + pparent_->combined_string_;
            ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
        }
        break;
    case CFindSheet::STRING_HEX:
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Hex find:");
        GetDlgItem(IDC_FIND_MESSAGE)->SetWindowText(
            "Press Enter or click the \"Find Next\" button to search for the bytes.\n"
            "Select the \"Hex\" page for advanced hex search options.");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(FALSE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(FALSE);

        if (fix_strings)
        {
            // Set search strings in other pages
            pparent_->hex_string_ = pparent_->combined_string_;
            pparent_->FixHexString();
            pparent_->TextFromHex(pparent_->combined_string_);  // Extract text from hex bytes using current char set

            ((CMainFrame *)AfxGetMainWnd())->SetSearchString(pparent_->hex_string_);
        }
        break;
    default:
        ASSERT(0);
        /* fall through */
    case CFindSheet::STRING_UNKNOWN:
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Find:");
        GetDlgItem(IDC_FIND_MESSAGE)->SetWindowText("Type pairs of hex digits or some text to search for.\n");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(FALSE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(FALSE);

        if (fix_strings)
        {
            // Clear strings
            pparent_->hex_string_.Empty();
            pparent_->FixHexString();
            pparent_->text_string_.Empty();

            ((CMainFrame *)AfxGetMainWnd())->SetSearchString("");
        }
        break;
    }
}

void CSimplePage::FixDirn()
{
    ASSERT(GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL);
    GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(pparent_->direction_ == 0 ? "To start of file" : "To end of file");
}

/////////////////////////////////////////////////////////////////////////////
// CSimplePage message handlers

BOOL CSimplePage::OnInitDialog() 
{
    pparent_ = (CFindSheet *)GetParent();
	CPropertyPage::OnInitDialog();

	return TRUE;
}

BOOL CSimplePage::OnSetActive() 
{
    ctl_string_.SetWindowText(pparent_->combined_string_);
    FixDirn();
    FixType();

	return CPropertyPage::OnSetActive();
}

void CSimplePage::OnFindNext() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_FIND_NEXT);
}

static DWORD id_pairs1[] = { 
    IDC_FIND_DESC, HIDC_FIND_COMBINED_STRING,
    IDC_FIND_COMBINED_STRING, HIDC_FIND_COMBINED_STRING,
    IDC_FIND_MESSAGE, HIDC_FIND_MESSAGE,
    IDC_FIND_NEXT, HIDC_FIND_NEXT,
    IDC_FIND_HELP, HIDC_FIND_HELP,
    IDC_FIND_DIRN_UP, HIDC_FIND_DIRN_UP,
    IDC_FIND_DIRN_DOWN, HIDC_FIND_DIRN_UP,
    IDC_FIND_SCOPE_TOMARK, HIDC_FIND_SCOPE_TOMARK,
    IDC_FIND_SCOPE_TOEND, HIDC_FIND_SCOPE_TOEND,
    IDC_FIND_SCOPE_FILE, HIDC_FIND_SCOPE_FILE,
    IDC_FIND_SCOPE_ALL, HIDC_FIND_SCOPE_ALL,
    IDC_FIND_MATCH_CASE, HIDC_FIND_MATCH_CASE,
    IDC_FIND_WHOLE_WORD, HIDC_FIND_WHOLE_WORD,
    0,0 
};

void CSimplePage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs1))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CSimplePage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
//	return CPropertyPage::OnHelpInfo(pHelpInfo);
 
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);
    ASSERT(pApp->m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, pApp->m_pszHelpFilePath, HELP_WM_HELP, (DWORD) (LPSTR) id_pairs1))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CSimplePage::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_FIND_SIMPLE_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_FIND_SIMPLE_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CSimplePage::OnChangeString() 
{
    ctl_string_.GetWindowText(pparent_->combined_string_);
    pparent_->AdjustPrefix(pparent_->combined_string_);
    FixType(TRUE);
}

void CSimplePage::OnSelchangeString() 
{
    ctl_string_.GetLBText(ctl_string_.GetCurSel(), pparent_->combined_string_);
    pparent_->AdjustPrefix(pparent_->combined_string_);
    FixType(TRUE);
}

void CSimplePage::OnChangeDirn() 
{
    UpdateData();
    FixDirn();
}

void CSimplePage::OnChangeMatchCase() 
{
    UpdateData();

    // Fix current search string so it has correct case-sensitivity (~ or =)
    CString temp;
    if (pparent_->GetMatchCase())
        temp = CString("=") + pparent_->combined_string_;
    else
        temp = CString("~") + pparent_->combined_string_;
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
}

/////////////////////////////////////////////////////////////////////////////
// CHexPage property page

IMPLEMENT_DYNCREATE(CHexPage, CPropertyPage)

CHexPage::CHexPage() : CPropertyPage(CHexPage::IDD)
{
	//{{AFX_DATA_INIT(CHexPage)
	//}}AFX_DATA_INIT
    phex_ = NULL;
    pmask_ = NULL;
}

CHexPage::~CHexPage()
{
}

void CHexPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHexPage)
	DDX_Control(pDX, IDC_FIND_BOOKMARK_PREFIX, ctl_bookmark_prefix_);
	DDX_Control(pDX, IDC_FIND_HEX_STRING, ctl_hex_string_);
	//}}AFX_DATA_MAP
//	DDX_CBString(pDX, IDC_FIND_HEX_STRING, pparent_->hex_string_);
	DDX_Text(pDX, IDC_FIND_MASK, pparent_->mask_string_);
//	DDX_Check(pDX, IDC_FIND_USE_MASK, pparent_->use_mask_);
	DDX_Check(pDX, IDC_FIND_USE_MASK, pparent_->wildcards_allowed_);
    DDX_Radio(pDX, IDC_FIND_ALIGN1, *(int *)(&pparent_->alignment_));
	DDX_Radio(pDX, IDC_FIND_DIRN_UP, *(int *)(&pparent_->direction_));
//	DDX_Radio(pDX, IDC_FIND_SCOPE_SELECTION, *(int *)(&pparent_->scope_));
	DDX_Radio(pDX, IDC_FIND_SCOPE_TOMARK, *(int *)(&pparent_->scope_));
	DDX_Text(pDX, IDC_FIND_BOOKMARK_PREFIX, pparent_->bookmark_prefix_);
}

BEGIN_MESSAGE_MAP(CHexPage, CPropertyPage)
	//{{AFX_MSG_MAP(CHexPage)
	ON_BN_CLICKED(IDC_FIND_USE_MASK, OnUseMask)
	ON_BN_CLICKED(IDC_FIND_DIRN_DOWN, OnChangeDirn)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_FIND_HELP, OnHelp)
	ON_CBN_EDITCHANGE(IDC_FIND_HEX_STRING, OnChangeString)
	ON_EN_CHANGE(IDC_FIND_MASK, OnChangeMask)
	ON_CBN_SELCHANGE(IDC_FIND_HEX_STRING, OnSelchangeString)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_FIND_BOOKMARK_ALL, OnBookmarkAll)
	ON_BN_CLICKED(IDC_FIND_DIRN_UP, OnChangeDirn)
	ON_EN_CHANGE(IDC_FIND_BOOKMARK_PREFIX, OnChangePrefix)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CHexPage::FixDirn()
{
    ASSERT(GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL);
    GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(pparent_->direction_ == 0 ? "To start of file" : "To end of file");
}

void CHexPage::FixStrings()
{
    pparent_->combined_string_ = pparent_->hex_string_;

    // hex_string_ has not had the case of the last entered character fixed yet
    if (theApp.hex_ucase_)
        pparent_->combined_string_.MakeUpper();
    else
        pparent_->combined_string_.MakeLower();

    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(pparent_->combined_string_);

    // We need to call FixHexString to get the updated mask but not change hex_string_
    CString saved = pparent_->hex_string_;
    pparent_->FixHexString();
    ASSERT(GetDlgItem(IDC_FIND_MASK) != NULL);
    GetDlgItem(IDC_FIND_MASK)->SetWindowText(pparent_->mask_string_);
    pparent_->hex_string_ = saved;

    pparent_->TextFromHex(pparent_->hex_string_, pparent_->mask_string_);

    pparent_->AdjustPrefix(pparent_->hex_string_);
}

void CHexPage::FixMask()
{
    ASSERT(GetDlgItem(IDC_FIND_MASK) != NULL);
//    GetDlgItem(IDC_FIND_MASK)->EnableWindow(pparent_->use_mask_);
    GetDlgItem(IDC_FIND_MASK)->EnableWindow(pparent_->wildcards_allowed_);
    ASSERT(GetDlgItem(IDC_FIND_MASK_DESC) != NULL);
//    GetDlgItem(IDC_FIND_MASK_DESC)->EnableWindow(pparent_->use_mask_);
    GetDlgItem(IDC_FIND_MASK_DESC)->EnableWindow(pparent_->wildcards_allowed_);
}

/////////////////////////////////////////////////////////////////////////////
// CHexPage message handlers

BOOL CHexPage::OnInitDialog() 
{
    pparent_ = (CFindSheet *)GetParent();
	CPropertyPage::OnInitDialog();

    // Subclass the edit control part of the combo box
    CWnd *pwnd;

    if ((pwnd = ctl_hex_string_.GetWindow(GW_CHILD)) != NULL)
    {
        phex_ = new CHexEdit;
        phex_->set_group_by(2);
        phex_->set_allow_qmark();

        if (!phex_->SubclassWindow(pwnd->m_hWnd))
        {
            TRACE0("Failed to subclass hex text edit control in search hex page\n");
            delete phex_;
            phex_ = NULL;
        }
    }

    // Subclass the mask edit control
    pmask_ = new CHexEdit;
    pmask_->set_group_by(2);

    if (!pmask_->SubclassWindow(GetDlgItem(IDC_FIND_MASK)->m_hWnd))
    {
        TRACE0("Failed to subclass mask edit control in search hex page\n");
        delete pmask_;
        pmask_ = NULL;
    }

	return TRUE;
}

void CHexPage::OnCancel() 
{
    if (phex_ != NULL)
    {
        if (phex_->m_hWnd != 0)
            phex_->UnsubclassWindow();
        delete phex_;
        phex_ = NULL;
    }
    if (pmask_ != NULL)
    {
        if (pmask_->m_hWnd != 0)
            pmask_->UnsubclassWindow();
        delete pmask_;
        pmask_ = NULL;
    }

	CPropertyPage::OnCancel();
}

void CHexPage::OnOK() 
{
    if (phex_ != NULL)
    {
        if (phex_->m_hWnd != 0)
            phex_->UnsubclassWindow();
        delete phex_;
        phex_ = NULL;
    }
    if (pmask_ != NULL)
    {
        if (pmask_->m_hWnd != 0)
            pmask_->UnsubclassWindow();
        delete pmask_;
        pmask_ = NULL;
    }

	CPropertyPage::OnOK();
}

BOOL CHexPage::OnSetActive() 
{
    ctl_hex_string_.SetWindowText(pparent_->hex_string_);
    FixDirn();
    FixMask();

	return CPropertyPage::OnSetActive();
}

void CHexPage::OnFindNext() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_FIND_NEXT);
}

void CHexPage::OnBookmarkAll() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_BOOKMARK_ALL);
}

static DWORD id_pairs2[] = { 
    IDC_FIND_HEX_STRING, HIDC_FIND_HEX_STRING,
    IDC_FIND_MASK_DESC, HIDC_FIND_MASK,
    IDC_FIND_MASK, HIDC_FIND_MASK,
    IDC_FIND_USE_MASK, HIDC_FIND_USE_MASK,
    IDC_FIND_NEXT, HIDC_FIND_NEXT,
    IDC_FIND_BOOKMARK_ALL, HIDC_FIND_BOOKMARK_ALL,
    IDC_FIND_BOOKMARK_PREFIX, HIDC_FIND_BOOKMARK_PREFIX,
    IDC_FIND_HELP, HIDC_FIND_HELP,
    IDC_FIND_DIRN_UP, HIDC_FIND_DIRN_UP,
    IDC_FIND_DIRN_DOWN, HIDC_FIND_DIRN_UP,
    IDC_FIND_SCOPE_TOMARK, HIDC_FIND_SCOPE_TOMARK,
    IDC_FIND_SCOPE_TOEND, HIDC_FIND_SCOPE_TOEND,
    IDC_FIND_SCOPE_FILE, HIDC_FIND_SCOPE_FILE,
    IDC_FIND_SCOPE_ALL, HIDC_FIND_SCOPE_ALL,
    IDC_FIND_ALIGN1, HIDC_FIND_ALIGN1,
    IDC_FIND_ALIGN2, HIDC_FIND_ALIGN2,
    IDC_FIND_ALIGN4, HIDC_FIND_ALIGN4,
    IDC_FIND_ALIGN8, HIDC_FIND_ALIGN8,
    0,0 
};

void CHexPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs2))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CHexPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
//	return CPropertyPage::OnHelpInfo(pHelpInfo);
 
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);
    ASSERT(pApp->m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, pApp->m_pszHelpFilePath, HELP_WM_HELP, (DWORD) (LPSTR) id_pairs2))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CHexPage::OnHelp()
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_FIND_HEX_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_FIND_HEX_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CHexPage::OnChangeString()
{
    ctl_hex_string_.GetWindowText(pparent_->hex_string_);
    FixStrings();
    FixMask();
}

void CHexPage::OnSelchangeString() 
{
    ctl_hex_string_.GetLBText(ctl_hex_string_.GetCurSel(), pparent_->hex_string_);
    FixStrings();
    FixMask();
}

void CHexPage::OnChangeMask() 
{
	// xxx TODO: Add your control notification handler code here
}

void CHexPage::OnUseMask() 
{
    ASSERT(GetDlgItem(IDC_FIND_MASK) != NULL);
    UpdateData();
    FixMask();
    if (pparent_->wildcards_allowed_)
    {
        GetDlgItem(IDC_FIND_MASK)->SetFocus();
        ((CEdit *)GetDlgItem(IDC_FIND_MASK))->SetSel(0, -1, FALSE);
    }
}

void CHexPage::OnChangeDirn() 
{
    UpdateData();
    FixDirn();
}

void CHexPage::OnChangePrefix() 
{
    UpdateData();

    // Remember that prefix has been entered by user (unless cleared)
    pparent_->prefix_entered_ = !pparent_->bookmark_prefix_.IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////
// CTextPage property page

IMPLEMENT_DYNCREATE(CTextPage, CPropertyPage)

CTextPage::CTextPage() : CPropertyPage(CTextPage::IDD)
{
	//{{AFX_DATA_INIT(CTextPage)
	//}}AFX_DATA_INIT
}

CTextPage::~CTextPage()
{
}

void CTextPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextPage)
	DDX_Control(pDX, IDC_FIND_BOOKMARK_PREFIX, ctl_bookmark_prefix_);
	DDX_Control(pDX, IDC_FIND_TEXT_STRING, ctl_text_string_);
	//}}AFX_DATA_MAP
//	DDX_CBString(pDX, IDC_FIND_TEXT_STRING, pparent_->text_string_);
	DDX_Radio(pDX, IDC_FIND_DIRN_UP, *(int *)(&pparent_->direction_));
	DDX_Check(pDX, IDC_FIND_WHOLE_WORD, pparent_->whole_word_);
	DDX_Check(pDX, IDC_FIND_MATCH_CASE, pparent_->match_case_);
//	DDX_Radio(pDX, IDC_FIND_SCOPE_SELECTION, *(int *)(&pparent_->scope_));
	DDX_Radio(pDX, IDC_FIND_SCOPE_TOMARK, *(int *)(&pparent_->scope_));
	DDX_Check(pDX, IDC_FIND_ALLOW_WILDCARD, pparent_->wildcards_allowed_);
	DDX_Text(pDX, IDC_FIND_WILDCARD_CHAR, pparent_->wildcard_char_);
	DDX_Radio(pDX, IDC_FIND_TYPE_ASCII, *(int *)(&pparent_->charset_));
	DDX_Text(pDX, IDC_FIND_BOOKMARK_PREFIX, pparent_->bookmark_prefix_);
}

BEGIN_MESSAGE_MAP(CTextPage, CPropertyPage)
	//{{AFX_MSG_MAP(CTextPage)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_FIND_ALLOW_WILDCARD, OnAllowWildcard)
	ON_BN_CLICKED(IDC_FIND_DIRN_DOWN, OnChangeDirn)
	ON_BN_CLICKED(IDC_FIND_TYPE_ASCII, OnChangeType)
	ON_BN_CLICKED(IDC_FIND_HELP, OnHelp)
	ON_CBN_EDITCHANGE(IDC_FIND_TEXT_STRING, OnChangeString)
	ON_CBN_SELCHANGE(IDC_FIND_TEXT_STRING, OnSelchangeString)
	ON_BN_CLICKED(IDC_FIND_MATCH_CASE, OnChangeMatchCase)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_FIND_BOOKMARK_ALL, OnBookmarkAll)
	ON_BN_CLICKED(IDC_FIND_DIRN_UP, OnChangeDirn)
	ON_BN_CLICKED(IDC_FIND_TYPE_EBCDIC, OnChangeType)
	ON_BN_CLICKED(IDC_FIND_TYPE_UNICODE, OnChangeType)
	ON_EN_CHANGE(IDC_FIND_BOOKMARK_PREFIX, OnChangePrefix)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CTextPage::FixDirn()
{
    ASSERT(GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL);
    GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(pparent_->direction_ == 0 ? "To start of file" : "To end of file");
}

void CTextPage::FixStrings()
{
    pparent_->combined_string_ = pparent_->text_string_;
    pparent_->HexFromText(pparent_->text_string_);   // Convert text to hex using current char set

    CString temp;
    if (pparent_->GetMatchCase())
        temp = CString("=") + pparent_->text_string_;
    else
        temp = CString("~") + pparent_->text_string_;
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);

    pparent_->AdjustPrefix(pparent_->text_string_, TRUE);
}

void CTextPage::FixWildcard()
{
    ASSERT(GetDlgItem(IDC_FIND_WILDCARD_CHAR) != NULL);
    GetDlgItem(IDC_FIND_WILDCARD_CHAR)->EnableWindow(pparent_->wildcards_allowed_);
}

/////////////////////////////////////////////////////////////////////////////
// CTextPage message handlers

BOOL CTextPage::OnInitDialog() 
{
    pparent_ = (CFindSheet *)GetParent();
	CPropertyPage::OnInitDialog();

	return TRUE;
}

BOOL CTextPage::OnSetActive() 
{
    ctl_text_string_.SetWindowText(pparent_->text_string_);
    FixDirn();
    FixWildcard();

	return CPropertyPage::OnSetActive();
}

void CTextPage::OnFindNext() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_FIND_NEXT);
}

void CTextPage::OnBookmarkAll() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_BOOKMARK_ALL);
}

static DWORD id_pairs3[] = { 
    IDC_FIND_TEXT_STRING, HIDC_FIND_TEXT_STRING,
    IDC_FIND_ALLOW_WILDCARD, HIDC_FIND_ALLOW_WILDCARD,
    IDC_FIND_WILDCARD_CHAR, HIDC_FIND_ALLOW_WILDCARD,
    IDC_FIND_NEXT, HIDC_FIND_NEXT,
    IDC_FIND_BOOKMARK_ALL, HIDC_FIND_BOOKMARK_ALL,
    IDC_FIND_BOOKMARK_PREFIX, HIDC_FIND_BOOKMARK_PREFIX,
    IDC_FIND_HELP, HIDC_FIND_HELP,
    IDC_FIND_DIRN_UP, HIDC_FIND_DIRN_UP,
    IDC_FIND_DIRN_DOWN, HIDC_FIND_DIRN_UP,
    IDC_FIND_SCOPE_TOMARK, HIDC_FIND_SCOPE_TOMARK,
    IDC_FIND_SCOPE_TOEND, HIDC_FIND_SCOPE_TOEND,
    IDC_FIND_SCOPE_FILE, HIDC_FIND_SCOPE_FILE,
    IDC_FIND_SCOPE_ALL, HIDC_FIND_SCOPE_ALL,
    IDC_FIND_MATCH_CASE, HIDC_FIND_MATCH_CASE,
    IDC_FIND_WHOLE_WORD, HIDC_FIND_WHOLE_WORD,
    IDC_FIND_TYPE_ASCII, HIDC_FIND_TYPE_ASCII,
    IDC_FIND_TYPE_UNICODE, HIDC_FIND_TYPE_UNICODE,
    IDC_FIND_TYPE_EBCDIC, HIDC_FIND_TYPE_EBCDIC,
    0,0 
};

void CTextPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs3))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CTextPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
//	return CPropertyPage::OnHelpInfo(pHelpInfo);
 
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);
    ASSERT(pApp->m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, pApp->m_pszHelpFilePath, HELP_WM_HELP, (DWORD) (LPSTR) id_pairs3))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CTextPage::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_FIND_TEXT_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_FIND_TEXT_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CTextPage::OnChangeString() 
{
    ctl_text_string_.GetWindowText(pparent_->text_string_);
    FixStrings();
}

void CTextPage::OnSelchangeString() 
{
    ctl_text_string_.GetLBText(ctl_text_string_.GetCurSel(), pparent_->text_string_);
    FixStrings();
}

void CTextPage::OnAllowWildcard() 
{
    ASSERT(GetDlgItem(IDC_FIND_WILDCARD_CHAR) != NULL);
    UpdateData();
    FixWildcard();
    if (pparent_->wildcards_allowed_)
    {
        GetDlgItem(IDC_FIND_WILDCARD_CHAR)->SetFocus();
        ((CEdit *)GetDlgItem(IDC_FIND_WILDCARD_CHAR))->SetSel(0, -1, FALSE);
    }
    FixStrings();
}

void CTextPage::OnChangeDirn() 
{
    UpdateData();
    FixDirn();
}

void CTextPage::OnChangeType() 
{
    UpdateData();
    FixStrings();
}

void CTextPage::OnChangeMatchCase() 
{
    UpdateData();

    // Fix current search string so it has correct case-sensitivity (~ or =)
    CString temp;
    if (pparent_->GetMatchCase())
        temp = CString("=") + pparent_->text_string_;
    else
        temp = CString("~") + pparent_->text_string_;
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
}

void CTextPage::OnChangePrefix() 
{
    UpdateData();
    // Remember that prefix has been entered by user (unless cleared)
    pparent_->prefix_entered_ = !pparent_->bookmark_prefix_.IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////
// CNumberPage property page

IMPLEMENT_DYNCREATE(CNumberPage, CPropertyPage)

CNumberPage::CNumberPage() : CPropertyPage(CNumberPage::IDD)
{
	//{{AFX_DATA_INIT(CNumberPage)
	//}}AFX_DATA_INIT
}

CNumberPage::~CNumberPage()
{
}

void CNumberPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNumberPage)
	DDX_Control(pDX, IDC_FIND_BOOKMARK_PREFIX, ctl_bookmark_prefix_);
	DDX_Control(pDX, IDC_FIND_NUMBER_SIZE, ctl_number_size_);
	DDX_Control(pDX, IDC_FIND_NUM_STRING, ctl_number_string_);
	//}}AFX_DATA_MAP
// We changed the combo to just an edit control
	DDX_Text(pDX, IDC_FIND_NUM_STRING, pparent_->number_string_);
	DDX_Radio(pDX, IDC_FIND_DIRN_UP, *(int *)(&pparent_->direction_));
//	DDX_Radio(pDX, IDC_FIND_SCOPE_SELECTION, *(int *)(&pparent_->scope_));
	DDX_Radio(pDX, IDC_FIND_SCOPE_TOMARK, *(int *)(&pparent_->scope_));
	DDX_CBIndex(pDX, IDC_FIND_NUMBER_FORMAT, pparent_->number_format_);
	DDX_CBIndex(pDX, IDC_FIND_NUMBER_SIZE, pparent_->number_size_);
	DDX_Check(pDX, IDC_FIND_BIG_ENDIAN, pparent_->big_endian_);
	DDX_Radio(pDX, IDC_FIND_ALIGN1, *(int *)(&pparent_->alignment_));
	DDX_Text(pDX, IDC_FIND_BOOKMARK_PREFIX, pparent_->bookmark_prefix_);
}

BEGIN_MESSAGE_MAP(CNumberPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNumberPage)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_FIND_HELP, OnHelp)
	ON_BN_CLICKED(IDC_FIND_DIRN_DOWN, OnChangeDirn)
	ON_CBN_SELCHANGE(IDC_FIND_NUMBER_FORMAT, OnChangeFormat)
	ON_EN_CHANGE(IDC_FIND_NUM_STRING, OnChangeString)
	ON_BN_CLICKED(IDC_FIND_BIG_ENDIAN, OnChangeEndian)
	ON_CBN_SELCHANGE(IDC_FIND_NUMBER_SIZE, OnSelchangeSize)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_FIND_BOOKMARK_ALL, OnBookmarkAll)
	ON_BN_CLICKED(IDC_FIND_DIRN_UP, OnChangeDirn)
	ON_EN_CHANGE(IDC_FIND_BOOKMARK_PREFIX, OnChangePrefix)
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CNumberPage::FixDirn()
{
    ASSERT(GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL);
    GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(pparent_->direction_ == 0 ? "To start of file" : "To end of file");
}

void CNumberPage::FixSizes()
{
    int ii = ctl_number_size_.GetCurSel();
    ctl_number_size_.ResetContent();

    switch (pparent_->number_format_)
    {
    case 0:
    case 1:
        ctl_number_size_.AddString("Byte");
        ctl_number_size_.AddString("Word");
        ctl_number_size_.AddString("Double word");
        ctl_number_size_.AddString("Quad word");
        break;
    case 2:
        ctl_number_size_.AddString("32 bit (float)");
        ctl_number_size_.AddString("64 bit (double)");
        break;
    case 3:
        ctl_number_size_.AddString("32 bit");
        ctl_number_size_.AddString("64 bit");
        break;
    default:
        ASSERT(0);
    }
    if (ctl_number_size_.SetCurSel(ii) == CB_ERR)
        ctl_number_size_.SetCurSel(0);
}

// Update pparent_->hex_string_ and SetSearchString to reflect bytes to search for
// Returns FALSE if number appears to be invalid
BOOL CNumberPage::FixStrings()
{
    size_t bytes = -1;
    unsigned char buf[9];
    memset(buf, sizeof(buf)-1, '\0');
    buf[sizeof(buf)-1] = '\xCD';               // Used to detect buffer overflow
    char *endptr;
    const char *constendptr = NULL;             // We need 2 ptrs because of strtod silliness

    switch (pparent_->number_format_)
    {
    default:
        ASSERT(0);
        // fall through
    case 0:  // unsigned int
    case 1:  // int
        if (pparent_->number_format_ == 0)
        {
            // Convert text to unsigned
            *((unsigned __int64 *)buf) = _atoi64(pparent_->number_string_);
        }
        else
        {
            // Convert text to signed
            *((__int64 *)buf) = _atoi64(pparent_->number_string_);
        }

        // Fix constendptr to point past end of number (incl. leading whitespace and sign)
        constendptr = pparent_->number_string_;
        while (isspace(*constendptr)) ++constendptr;
        if (*constendptr == '-' || *constendptr == '+') ++constendptr;
        while (isdigit(*constendptr)) ++constendptr;

        switch (pparent_->number_size_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            bytes = 1;
            break;
        case 1:
            bytes = 2;
            break;
        case 2:
            bytes = 4;
            break;
        case 3:
            bytes = 8;
            break;
        }
        break;
    case 2:  // IEEE float
        switch (pparent_->number_size_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            bytes = 4;
            *((float *)buf) = (float)strtod(pparent_->number_string_, &endptr);
            constendptr = endptr;
            break;
        case 1:
            bytes = 8;
            *((double *)buf) = strtod(pparent_->number_string_, &endptr);
            constendptr = endptr;
            break;
        }
        break;
    case 3:  // IBM float
        switch (pparent_->number_size_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            bytes = 4;
            ::make_ibm_fp32(buf, strtod(pparent_->number_string_, &endptr), true);
            constendptr = endptr;
            break;
        case 1:
            bytes = 8;
            ::make_ibm_fp64(buf, strtod(pparent_->number_string_, &endptr), true);
            constendptr = endptr;
            break;
        }
        break;
    }
    ASSERT(constendptr != NULL);

    char out[3*8+1];
    char *po = out;

    const char *hex_fmt;
    if (theApp.hex_ucase_)
        hex_fmt = "%02X ";
    else
        hex_fmt = "%02x ";

    // Make hex string allowing for byte order
    if (pparent_->big_endian_)
    {
        for (int ii = bytes; ii > 0; ii--)
        {
            sprintf(po, hex_fmt, buf[ii-1]);
            po += 3;
        }
    }
    else
    {
        for (int ii = 0; ii < bytes; ++ii)
        {
            sprintf(po, hex_fmt, buf[ii]);
            po += 3;
        }
    }
    *po = '\0';

    pparent_->combined_string_ = pparent_->hex_string_ = out;
    pparent_->text_string_.Empty();

    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(out);
    ASSERT(signed char(buf[sizeof(buf)-1]) == '\xCD');

    pparent_->AdjustPrefix(pparent_->number_string_);

    return !pparent_->number_string_.IsEmpty() && *constendptr == '\0';
}

/////////////////////////////////////////////////////////////////////////////
// CNumberPage message handlers
BOOL CNumberPage::OnInitDialog() 
{
    pparent_ = (CFindSheet *)GetParent();
	CPropertyPage::OnInitDialog();

	return TRUE;
}

BOOL CNumberPage::OnSetActive() 
{
    FixDirn();
    FixSizes();
	
	return CPropertyPage::OnSetActive();
}

void CNumberPage::OnFindNext() 
{
    UpdateData();
    if (!FixStrings())
    {
        AfxMessageBox("There was a problem analysing the number.\n\n"
                      "Please check the number and format settings.");
        return;
    }
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_FIND_NEXT);
}

void CNumberPage::OnBookmarkAll() 
{
    UpdateData();
    if (!FixStrings())
    {
        AfxMessageBox("There was a problem analysing the number.\n\n"
                      "Please check the number and format settings.");
        return;
    }
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_BOOKMARK_ALL);
}

static DWORD id_pairs4[] = { 
    IDC_FIND_NUM_STRING, HIDC_FIND_NUM_STRING,
    IDC_FIND_NUMBER_FORMAT, HIDC_FIND_NUMBER_FORMAT,
    IDC_FIND_NUMBER_SIZE, HIDC_FIND_NUMBER_SIZE,
    IDC_FIND_BIG_ENDIAN, HIDC_FIND_BIG_ENDIAN,
    IDC_FIND_NEXT, HIDC_FIND_NEXT,
    IDC_FIND_BOOKMARK_ALL, HIDC_FIND_BOOKMARK_ALL,
    IDC_FIND_BOOKMARK_PREFIX, HIDC_FIND_BOOKMARK_PREFIX,
    IDC_FIND_HELP, HIDC_FIND_HELP,
    IDC_FIND_DIRN_UP, HIDC_FIND_DIRN_UP,
    IDC_FIND_DIRN_DOWN, HIDC_FIND_DIRN_UP,
    IDC_FIND_SCOPE_TOMARK, HIDC_FIND_SCOPE_TOMARK,
    IDC_FIND_SCOPE_TOEND, HIDC_FIND_SCOPE_TOEND,
    IDC_FIND_SCOPE_FILE, HIDC_FIND_SCOPE_FILE,
    IDC_FIND_SCOPE_ALL, HIDC_FIND_SCOPE_ALL,
    IDC_FIND_ALIGN1, HIDC_FIND_ALIGN1,
    IDC_FIND_ALIGN2, HIDC_FIND_ALIGN2,
    IDC_FIND_ALIGN4, HIDC_FIND_ALIGN4,
    IDC_FIND_ALIGN8, HIDC_FIND_ALIGN8,
    0,0 
};

void CNumberPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs4))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CNumberPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
//	return CPropertyPage::OnHelpInfo(pHelpInfo);
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);
    ASSERT(pApp->m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, pApp->m_pszHelpFilePath, HELP_WM_HELP, (DWORD) (LPSTR) id_pairs4))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CNumberPage::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_FIND_NUMBER_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_FIND_NUMBER_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CNumberPage::OnChangeDirn() 
{
    UpdateData();
    FixDirn();
}

void CNumberPage::OnChangeString() 
{
    UpdateData();
    FixStrings();
}

void CNumberPage::OnChangeFormat() 
{
    UpdateData();
    FixSizes();

    UpdateData();
    FixStrings();
}

void CNumberPage::OnChangeEndian() 
{
    UpdateData();
    FixStrings();
}

void CNumberPage::OnSelchangeSize() 
{
    UpdateData();
    FixStrings();
}

void CNumberPage::OnChangePrefix() 
{
    UpdateData();
    // Remember that prefix has been entered by user (unless cleared)
    pparent_->prefix_entered_ = !pparent_->bookmark_prefix_.IsEmpty();
}

/////////////////////////////////////////////////////////////////////////////
// CReplacePage property page

IMPLEMENT_DYNCREATE(CReplacePage, CPropertyPage)

CReplacePage::CReplacePage() : CPropertyPage(CReplacePage::IDD)
{
	//{{AFX_DATA_INIT(CReplacePage)
	//}}AFX_DATA_INIT
}

CReplacePage::~CReplacePage()
{
}

void CReplacePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReplacePage)
	DDX_Control(pDX, IDC_FIND_COMBINED_STRING, ctl_string_);
	DDX_Control(pDX, IDC_FIND_REPLACE_STRING, ctl_replace_string_);
	//}}AFX_DATA_MAP
//	DDX_CBString(pDX, IDC_FIND_COMBINED_STRING, pparent_->combined_string_);
//	DDX_CBString(pDX, IDC_FIND_REPLACE_STRING, pparent_->replace_string_);
	DDX_Radio(pDX, IDC_FIND_DIRN_UP, *(int *)(&pparent_->direction_));
	DDX_Check(pDX, IDC_FIND_WHOLE_WORD, pparent_->whole_word_);
	DDX_Check(pDX, IDC_FIND_MATCH_CASE, pparent_->match_case_);
//	DDX_Radio(pDX, IDC_FIND_SCOPE_SELECTION, *(int *)(&pparent_->scope_));
	DDX_Radio(pDX, IDC_FIND_SCOPE_TOMARK, *(int *)(&pparent_->scope_));
}

BEGIN_MESSAGE_MAP(CReplacePage, CPropertyPage)
	//{{AFX_MSG_MAP(CReplacePage)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_FIND_REPLACE, OnReplace)
	ON_BN_CLICKED(IDC_FIND_REPLACE_ALL, OnReplaceAll)
	ON_BN_CLICKED(IDC_FIND_DIRN_DOWN, OnChangeDirn)
	ON_BN_CLICKED(IDC_FIND_HELP, OnHelp)
	ON_CBN_EDITCHANGE(IDC_FIND_REPLACE_STRING, OnChangeReplaceString)
	ON_CBN_SELCHANGE(IDC_FIND_REPLACE_STRING, OnSelchangeReplaceString)
	ON_BN_CLICKED(IDC_FIND_MATCH_CASE, OnChangeMatchCase)
	ON_CBN_EDITCHANGE(IDC_FIND_COMBINED_STRING, OnChangeString)
	ON_CBN_SELCHANGE(IDC_FIND_COMBINED_STRING, OnSelchangeString)
	ON_BN_CLICKED(IDC_FIND_DIRN_UP, OnChangeDirn)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CReplacePage::FixType(BOOL fix_strings /*=FALSE*/)
{
    ASSERT(GetDlgItem(IDC_FIND_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_FIND_WHOLE_WORD) != NULL);
    ASSERT(GetDlgItem(IDC_FIND_MATCH_CASE) != NULL);

    CString s1 = pparent_->combined_string_;
    s1.TrimLeft();
    CString s2 = pparent_->replace_string_;
    s2.TrimLeft();

    if (s1.IsEmpty() && s2.IsEmpty())
    {
        // Both strings empty - set default description
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Find:");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(FALSE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(FALSE);

        ((CMainFrame *)AfxGetMainWnd())->SetSearchString("");
    }
    else if (pparent_->HexReplace())
    {
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Hex find:");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(FALSE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(FALSE);

        if (fix_strings)
        {
            // Set search strings in other pages
            pparent_->hex_string_ = pparent_->combined_string_;
            pparent_->FixHexString();
            pparent_->TextFromHex(pparent_->combined_string_);  // Extract text from hex bytes using current char set

            ((CMainFrame *)AfxGetMainWnd())->SetSearchString(pparent_->hex_string_);
        }
    }
    else
    {
        GetDlgItem(IDC_FIND_DESC)->SetWindowText("Text find:");
        GetDlgItem(IDC_FIND_WHOLE_WORD)->EnableWindow(TRUE);
        GetDlgItem(IDC_FIND_MATCH_CASE)->EnableWindow(TRUE);

        if (fix_strings)
        {
            // Set search strings in other pages
            pparent_->text_string_ = pparent_->combined_string_;
            pparent_->HexFromText(pparent_->combined_string_);   // Convert text to hex using current char set

            UpdateData(FALSE);                      // Move back to controls before GetMatchCase calls UpdateData()
            CString temp;
            if (pparent_->GetMatchCase())
                temp = CString("=") + pparent_->combined_string_;
            else
                temp = CString("~") + pparent_->combined_string_;
            ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
        }
    }
    ((CMainFrame *)AfxGetMainWnd())->SetReplaceString(pparent_->replace_string_);
}

void CReplacePage::FixDirn()
{
    ASSERT(GetDlgItem(IDC_FIND_SCOPE_TOEND) != NULL);
    GetDlgItem(IDC_FIND_SCOPE_TOEND)->SetWindowText(pparent_->direction_ == 0 ? "To start of file" : "To end of file");
}

/////////////////////////////////////////////////////////////////////////////
// CReplacePage message handlers

BOOL CReplacePage::OnInitDialog() 
{
    pparent_ = (CFindSheet *)GetParent();
	CPropertyPage::OnInitDialog();

	return TRUE;
}

BOOL CReplacePage::OnSetActive() 
{
    ctl_string_.SetWindowText(pparent_->combined_string_);
    ctl_replace_string_.SetWindowText(pparent_->replace_string_);
    FixType();
    FixDirn();
	
	return CPropertyPage::OnSetActive();
}

void CReplacePage::OnFindNext() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_FIND_NEXT);
}

static DWORD id_pairs5[] = { 
    IDC_FIND_DESC, HIDC_FIND_COMBINED_STRING,
    IDC_FIND_COMBINED_STRING, HIDC_FIND_COMBINED_STRING,
    IDC_FIND_REPLACE_STRING, HIDC_FIND_REPLACE_STRING,
    IDC_FIND_NEXT, HIDC_FIND_NEXT,
    IDC_FIND_REPLACE, HIDC_FIND_REPLACE,
    IDC_FIND_REPLACE_ALL, HIDC_FIND_REPLACE_ALL,
    IDC_FIND_HELP, HIDC_FIND_HELP,
    IDC_FIND_DIRN_UP, HIDC_FIND_DIRN_UP,
    IDC_FIND_DIRN_DOWN, HIDC_FIND_DIRN_UP,
    IDC_FIND_SCOPE_TOMARK, HIDC_FIND_SCOPE_TOMARK,
    IDC_FIND_SCOPE_TOEND, HIDC_FIND_SCOPE_TOEND,
    IDC_FIND_SCOPE_FILE, HIDC_FIND_SCOPE_FILE,
    IDC_FIND_SCOPE_ALL, HIDC_FIND_SCOPE_ALL,
    IDC_FIND_MATCH_CASE, HIDC_FIND_MATCH_CASE,
    IDC_FIND_WHOLE_WORD, HIDC_FIND_WHOLE_WORD,
    0,0 
};

void CReplacePage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs5))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

BOOL CReplacePage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
//	return CPropertyPage::OnHelpInfo(pHelpInfo);
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);
    ASSERT(pApp->m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, pApp->m_pszHelpFilePath, HELP_WM_HELP, (DWORD) (LPSTR) id_pairs5))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CReplacePage::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_FIND_REPLACE_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_FIND_REPLACE_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CReplacePage::OnReplace() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_REPLACE);
}

void CReplacePage::OnReplaceAll() 
{
    UpdateData();
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_REPLACE_ALL);
}

void CReplacePage::OnChangeString() 
{
    ctl_string_.GetWindowText(pparent_->combined_string_);
    pparent_->AdjustPrefix(pparent_->combined_string_);
    FixType(TRUE);
}

void CReplacePage::OnSelchangeString() 
{
    ctl_string_.GetLBText(ctl_string_.GetCurSel(), pparent_->combined_string_);
    pparent_->AdjustPrefix(pparent_->combined_string_);
    FixType(TRUE);
}

void CReplacePage::OnChangeReplaceString() 
{
    ctl_replace_string_.GetWindowText(pparent_->replace_string_);
    FixType(TRUE);
}

void CReplacePage::OnSelchangeReplaceString() 
{
    ctl_replace_string_.GetLBText(ctl_replace_string_.GetCurSel(), pparent_->replace_string_);
    FixType(TRUE);
}

void CReplacePage::OnChangeDirn() 
{
    UpdateData();
    FixDirn();
}

void CReplacePage::OnChangeMatchCase() 
{
    UpdateData();

    // Fix current search string so it has correct case-sensitivity (~ or =)
    CString temp;
    if (pparent_->GetMatchCase())
        temp = CString("=") + pparent_->combined_string_;
    else
        temp = CString("~") + pparent_->combined_string_;
    ((CMainFrame *)AfxGetMainWnd())->SetSearchString(temp);
}

// CFindWnd dialog
IMPLEMENT_DYNAMIC(CFindWnd, ModelessDialogBaseClass)

CFindWnd::CFindWnd(CWnd* pParent /*=NULL*/)
#ifndef DIALOG_BAR
	: CDialog(CFindWnd::IDD, pParent)
#endif
{
	m_pSheet = NULL;
}

CFindWnd::~CFindWnd()
{
	if (m_pSheet != NULL)
		delete m_pSheet;
}

void CFindWnd::DoDataExchange(CDataExchange* pDX)
{
	ModelessDialogBaseClass::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFindWnd, ModelessDialogBaseClass)
	ON_WM_CLOSE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CFindWnd::Create(CWnd* pParentWnd /*=NULL*/) 
{
#ifndef DIALOG_BAR
	if (!CDialog::Create(CFindWnd::IDD, pParentWnd))
        return FALSE;
#else
	if (!CHexDialogBar::Create(CFindWnd::IDD, pParentWnd, CBRS_LEFT))
        return FALSE;
#endif

#ifndef DIALOG_BAR
    if (theApp.GetProfileInt("MainFrame", "ShowFindDlg", 0) == 0)
    {
        ShowWindow(SW_HIDE);
        visible_ = FALSE;
    }
	else
    {
        ShowWindow(SW_SHOWNORMAL);
		visible_ = TRUE;
    }

    // Restore window position
    if (theApp.find_y_ != -30000)
    {
        CRect rr;               // Rectangle where we will put the dialog
        GetWindowRect(&rr);

        // Move to where it was when it was last closed
        rr.OffsetRect(theApp.find_x_ - rr.left, theApp.find_y_ - rr.top);

        CRect scr_rect;         // Rectangle that we want to make sure the window is within

        // Get the rectangle that contains the screen work area (excluding system bars etc)
        if (theApp.mult_monitor_)
        {
            HMONITOR hh = MonitorFromRect(&rr, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            if (hh != 0 && GetMonitorInfo(hh, &mi))
                scr_rect = mi.rcWork;  // work area of nearest monitor
            else
            {
                // Shouldn't happen but if it does use the whole virtual screen
                ASSERT(0);
                scr_rect = CRect(::GetSystemMetrics(SM_XVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_XVIRTUALSCREEN) + ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
                    ::GetSystemMetrics(SM_YVIRTUALSCREEN) + ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
            }
        }
        else if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &scr_rect, 0))
        {
            // I don't know if this will ever happen since the Windows documentation
            // is pathetic and does not say when or why SystemParametersInfo might fail.
            scr_rect = CRect(0, 0, ::GetSystemMetrics(SM_CXFULLSCREEN),
                                   ::GetSystemMetrics(SM_CYFULLSCREEN));
        }

        if (rr.left > scr_rect.right - 20)              // off right edge?
            rr.OffsetRect(scr_rect.right - (rr.left+rr.right)/2, 0);
        if (rr.right < scr_rect.left + 20)              // off left edge?
            rr.OffsetRect(scr_rect.left - (rr.left+rr.right)/2, 0);
        if (rr.top > scr_rect.bottom - 20)              // off bottom?
            rr.OffsetRect(0, scr_rect.bottom - (rr.top+rr.bottom)/2);
        // This is not analogous to the prev. 3 since we don't want the window
        // off the top at all, otherwise you can get to the drag bar to move it.
        if (rr.top < scr_rect.top)                      // off top at all?
            rr.OffsetRect(0, scr_rect.top - rr.top);

        MoveWindow(&rr);
    }
#endif

	m_pSheet = new CFindSheet;
	if (!m_pSheet->Create(this, WS_CHILD | WS_VISIBLE))
	{
		TRACE("Could not create find sheet\n");
		delete m_pSheet;
		m_pSheet = NULL;
		return FALSE;
	}

	CRect rct;	
	GetWindowRect(&rct);
	m_pSheet->SetWindowPos(NULL, 0, 0, rct.Width(), rct.Height(),
	                       SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	return TRUE;
}

void CFindWnd::OnClose() 
{
#ifndef DIALOG_BAR
    ShowWindow(SW_HIDE);
    visible_ = FALSE;
#endif

    // Allow prefix auto-generation to start again on next open of find dlg
    m_pSheet->prefix_entered_ = FALSE;

    theApp.SaveToMacro(km_find_dlg, -1);
}

void CFindWnd::OnDestroy() 
{
#ifndef DIALOG_BAR
    CRect rr;
    GetWindowRect(&rr);
    theApp.find_x_ = rr.left;
    theApp.find_y_ = rr.top;
#endif
	ModelessDialogBaseClass::OnDestroy();
}

// Make sure dialog visible and show the specified page
void CFindWnd::ShowPage(int page)
{
#ifndef DIALOG_BAR
    ShowWindow(SW_SHOWNORMAL);
    visible_ = TRUE;
#else
    ((CMainFrame *)AfxGetMainWnd())->ShowControlBar(this, TRUE, FALSE);
	Unroll();
#endif
    if (page < 0)
        m_pSheet->SetActivePage(m_pSheet->p_page_replace_);
    else
        m_pSheet->SetActivePage(page);

    theApp.SaveToMacro(km_find_dlg, page);
    SetFocus();
}

