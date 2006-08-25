// NewFile.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "MainFrm.h"   // to write to status bar
#include "NewFile.h"

#ifdef USE_HTML_HELP
#include <HtmlHelp.h>
#endif

#include "resource.hm"
#include "HelpID.hm"            // For dlg help ID

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFile dialog

CNewFile::CNewFile(CWnd* pParent /*=NULL*/, bool ibmode /*=false*/)
	: CDialog(CNewFile::IDD, pParent), insert_block_mode_(ibmode), next_cb_hwnd_(0)
{
    first_time_ = true;

	//{{AFX_DATA_INIT(CNewFile)
	fill_random_range_ = _T("");
	fill_hex_value_ = _T("");
	fill_number_value_ = _T("");
	fill_string_value_ = _T("");
	//}}AFX_DATA_INIT

    ASSERT(sizeof(fill_) == 8);
	// This stores lots of options: size, size_source, type, charset, number_type, big_endian etc
	fill_state_ = _atoi64(theApp.GetProfileString("Fill", "State", "0"));

    fill_hex_value_      = theApp.GetProfileString("Fill", "Hex", "00");
    fill_string_value_   = theApp.GetProfileString("Fill", "String");
    fill_random_range_   = theApp.GetProfileString("Fill", "Random", "0-255");
    fill_number_value_   = theApp.GetProfileString("Fill", "Number");

	// Use the 40 bits from fill_ as the size (size_src == 0) or repeat count (size_src == 1).
	// Default the repeat count to 1 (size_src == 0) but default new file size to be repeat
	// count (size_src == 1).  Also make them both the same when size_src == 2.
    fill_size_ = fill_repeat_ = (__int64(fill_.size_high)<<32) + fill_.size_low;
	if (fill_.size_src == 0)
		fill_repeat_ = 1;

    in_update_ = TRUE;
}

void CNewFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

    int  fill_size_src;
	int  fill_type;
	BOOL fill_create_on_disk;
	int  fill_string_charset;
	int  fill_number_type;
	BOOL fill_big_endian;

	if (!pDX->m_bSaveAndValidate)
	{
		// Move from bitfields to DDXable variables (thence put into controls)
        fill_size_src       = fill_.size_src == 0 ? 2 : fill_.size_src-1;
		fill_type           = fill_.type;
		fill_create_on_disk = fill_.create_on_disk;
		fill_string_charset = fill_.string_charset;
		fill_number_type    = fill_.number_type;
		fill_big_endian     = fill_.big_endian;
	}

	//{{AFX_DATA_MAP(CNewFile)
	DDX_Control(pDX, IDC_BIG_ENDIAN, ctl_big_endian_);
	DDX_Control(pDX, IDC_FILL_RANDOM_RANGE, ctl_fill_random_range_);
	DDX_Control(pDX, IDC_FILL_STRING_VALUE, ctl_fill_string_value_);
	DDX_Control(pDX, IDC_FILL_STRING_CHARSET, ctl_fill_string_charset_);
	DDX_Control(pDX, IDC_FILL_NUMBER_VALUE, ctl_fill_number_value_);
	DDX_Control(pDX, IDC_FILL_NUMBER_TYPE, ctl_fill_number_type_);
	DDX_Control(pDX, IDC_DESC_FILL_STRING_CHARSET, ctl_desc_fill_string_charset_);
	DDX_Control(pDX, IDC_DESC_FILL_NUMBER_TYPE, ctl_desc_fill_number_type_);
	DDX_Text(pDX, IDC_FILL_RANDOM_RANGE, fill_random_range_);
	DDX_Text(pDX, IDC_FILL_HEX_VALUE, fill_hex_value_);
	DDX_Text(pDX, IDC_FILL_NUMBER_VALUE, fill_number_value_);
	DDX_Text(pDX, IDC_FILL_STRING_VALUE, fill_string_value_);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_SIZE_FACTOR, ctl_size_factor_);
	DDX_Control(pDX, IDC_SIZE_LENGTH, ctl_size_length_);
	
	DDX_Control(pDX, IDC_DECIMAL_SIZE, ctl_decimal_size_);
	DDX_Control(pDX, IDC_HEX_SIZE, ctl_hex_size_);
	DDX_Control(pDX, IDC_FILL_HEX_VALUE, ctl_hex_value_);
	DDX_Control(pDX, IDC_FILL_CLIPBOARD_SAMPLE, ctl_fill_clipboard_sample_);
	DDX_Control(pDX, IDC_FILL_CLIPBOARD_LENGTH, ctl_fill_clipboard_length_);
	DDX_Control(pDX, IDC_DESC_FILL_CLIPBOARD_LENGTH, ctl_desc_fill_clipboard_length_);

//    DDX_Text(pDX, IDC_FILL_CLIPBOARD_SAMPLE, clipboard_sample_);
//    DDX_Text(pDX, IDC_FILL_CLIPBOARD_LENGTH, clipboard_len_);
	DDX_Radio(pDX, IDC_FILL_SIZE_DATA, fill_size_src);
	DDX_Radio(pDX, IDC_FILL_HEX, fill_type);
	DDX_Check(pDX, IDC_FILL_DISK, fill_create_on_disk);
	DDX_CBIndex(pDX, IDC_FILL_STRING_CHARSET, fill_string_charset);
	DDX_CBIndex(pDX, IDC_FILL_NUMBER_TYPE, fill_number_type);
	DDX_Check(pDX, IDC_BIG_ENDIAN, fill_big_endian);

	if (pDX->m_bSaveAndValidate)
	{
		// Move from (controls thence) DDXable variable to bitfields
        fill_.size_src       = fill_size_src == 2 ? 0 : fill_size_src+1;
		fill_.type           = fill_type;
		fill_.create_on_disk = fill_create_on_disk;
		fill_.string_charset = fill_string_charset;
		fill_.number_type    = fill_number_type;
		fill_.big_endian     = fill_big_endian;
	}
}

BEGIN_MESSAGE_MAP(CNewFile, CDialog)
	//{{AFX_MSG_MAP(CNewFile)
	ON_BN_CLICKED(IDC_FILL_HEX, OnChangeFill)
	ON_EN_CHANGE(IDC_DECIMAL_SIZE, OnChangeDecimalSize)
	ON_EN_CHANGE(IDC_HEX_SIZE, OnChangeHexSize)
	ON_EN_CHANGE(IDC_FILL_HEX_VALUE, OnChangeHex)
	ON_EN_CHANGE(IDC_FILL_NUMBER_VALUE, OnChangeFillNumber)
	ON_BN_CLICKED(IDC_FILL_STRING, OnChangeFill)
	ON_BN_CLICKED(IDC_FILL_RANDOM, OnChangeFill)
	ON_BN_CLICKED(IDC_FILL_NUMBER, OnChangeFill)
	ON_CBN_SELCHANGE(IDC_FILL_NUMBER_TYPE, OnSelchangeFillNumberType)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_FILL_STRING_VALUE, OnChangeString)
	ON_CBN_SELCHANGE(IDC_FILL_STRING_CHARSET, OnSelchangeCharset)
	ON_EN_CHANGE(IDC_SIZE_FACTOR, OnChangeSizeFactor)
	ON_BN_CLICKED(IDC_FILL_CLIPBOARD, OnChangeFill)
	ON_BN_CLICKED(IDC_FILL_SIZE_DATA, OnChangeFill)
	ON_BN_CLICKED(IDC_FILL_SIZE_CALC, OnChangeFill)
	ON_BN_CLICKED(IDC_FILL_SIZE_SPECIFIED, OnChangeFill)
	ON_WM_DESTROY()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_CHANGECBCHAIN()
	ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_FILL_HELP, OnHelp)
END_MESSAGE_MAP()

void CNewFile::fix_controls() 
{
	// Enable controls that dep. on which size option is selected
	ctl_size_factor_.EnableWindow(fill_.size_src == 1);
	ctl_size_length_.EnableWindow(fill_.size_src == 1);
	ASSERT(GetDlgItem(IDC_SPIN_SIZE_FACTOR) != NULL);
	GetDlgItem(IDC_SPIN_SIZE_FACTOR)->EnableWindow(fill_.size_src == 1);
	ASSERT(GetDlgItem(IDC_STATIC1) != NULL);
	GetDlgItem(IDC_STATIC1)->EnableWindow(fill_.size_src == 1);
	ASSERT(GetDlgItem(IDC_STATIC1) != NULL);
	GetDlgItem(IDC_STATIC1)->EnableWindow(fill_.size_src == 1);
	ASSERT(GetDlgItem(IDC_STATIC2) != NULL);
	GetDlgItem(IDC_STATIC2)->EnableWindow(fill_.size_src == 1);
	ASSERT(GetDlgItem(IDC_STATIC3) != NULL);
	GetDlgItem(IDC_STATIC3)->EnableWindow(fill_.size_src == 1);

	ctl_decimal_size_.EnableWindow(fill_.size_src == 0);
	ctl_hex_size_.EnableWindow(fill_.size_src == 0);
	ASSERT(GetDlgItem(IDC_SPIN_DECIMAL_SIZE) != NULL);
	GetDlgItem(IDC_SPIN_DECIMAL_SIZE)->EnableWindow(fill_.size_src == 0);
	ASSERT(GetDlgItem(IDC_DESC_DECIMAL_SIZE) != NULL);
	GetDlgItem(IDC_DESC_DECIMAL_SIZE)->EnableWindow(fill_.size_src == 0);
	ASSERT(GetDlgItem(IDC_DESC_HEX_SIZE) != NULL);
	GetDlgItem(IDC_DESC_HEX_SIZE)->EnableWindow(fill_.size_src == 0);
	ASSERT(GetDlgItem(IDC_STATIC4) != NULL);
	GetDlgItem(IDC_STATIC4)->EnableWindow(fill_.size_src == 0);

	// Disable clipboard option if clipboard is empty or there is nothing there we can use
	ASSERT(GetDlgItem(IDC_FILL_CLIPBOARD) != NULL);
	GetDlgItem(IDC_FILL_CLIPBOARD)->EnableWindow(clipboard_format_ > 0);

	// Hide "Prompt for file name" control if we are only in insert block mode
	ASSERT(GetDlgItem(IDC_FILL_DISK) != NULL);
	GetDlgItem(IDC_FILL_DISK)->ShowWindow(insert_block_mode_ ? SW_HIDE : SW_SHOW);

	// Enable controls depending on type of fill selected
	ctl_hex_value_.EnableWindow(fill_.type == FILL_HEX);

	ctl_fill_string_value_.EnableWindow(fill_.type == FILL_STRING);
	ctl_desc_fill_string_charset_.EnableWindow(fill_.type == FILL_STRING);
	ctl_fill_string_charset_.EnableWindow(fill_.type == FILL_STRING);

    ctl_fill_clipboard_sample_.EnableWindow(fill_.type == FILL_CLIPBOARD);
    ctl_fill_clipboard_length_.EnableWindow(fill_.type == FILL_CLIPBOARD);
    ctl_desc_fill_clipboard_length_.EnableWindow(fill_.type == FILL_CLIPBOARD);

	ctl_fill_random_range_.EnableWindow(fill_.type == FILL_RANDOM);

	ctl_fill_number_value_.EnableWindow(fill_.type == FILL_NUMBER);
	ctl_desc_fill_number_type_.EnableWindow(fill_.type == FILL_NUMBER);
	ctl_fill_number_type_.EnableWindow(fill_.type == FILL_NUMBER);
//	ctl_desc_fill_number_format_.EnableWindow(fill_.type == FILL_NUMBER);
//	ctl_fill_number_format_.EnableWindow(fill_.type == FILL_NUMBER);
    ctl_big_endian_.EnableWindow(fill_.type == FILL_NUMBER && (fill_.number_is_float || fill_.number_type > 0));

	// Fix number type drop down list depending on type of number entered
    bool is_float = fill_number_value_.Find('.') != -1 ||
                    fill_number_value_.Find('e') != -1 ||
                    fill_number_value_.Find('E') != -1;

    if (first_time_ || is_float != (bool)fill_.number_is_float)
    {
        if (is_float)
        {
            ctl_fill_number_type_.ResetContent();
            ctl_fill_number_type_.AddString("32 bit IEEE");
            ctl_fill_number_type_.AddString("64 bit IEEE");
            if (fill_.number_type > 1) fill_.number_type = 1;
            ctl_fill_number_type_.SetCurSel(fill_.number_type);
        }
        else
        {
            ctl_fill_number_type_.ResetContent();
            ctl_fill_number_type_.AddString("8 bit int");
            ctl_fill_number_type_.AddString("16 bit int");
            ctl_fill_number_type_.AddString("32 bit int");
            ctl_fill_number_type_.AddString("64 bit int");
            ctl_fill_number_type_.SetCurSel(fill_.number_type);
        }

        fill_.number_is_float = is_float;
    }
    first_time_ = false;

	calc_length();
}

void CNewFile::calc_length() 
{
	// Fix data length calculation
    VERIFY(::OpenClipboard(m_hWnd));
    const char *pcb = NULL;             // Ptr to clipboard data
    const char *data_str = NULL;
    switch (fill_.type)
    {
    case CNewFile::FILL_HEX:   // count hex digits
        data_str = fill_hex_value_;
        break;
    case CNewFile::FILL_STRING:   // string
        data_str = fill_string_value_;
        break;
    //case CNewFile::FILL_RANDOM:   // random - default to one byte
    //    data_str = fill_random_range_;
    //    break;
    //case CNewFile::FILL_NUMBER:   // number - get size from type
    //    data_str = fill_number_value_;
    //    break;
    }
    size_t data_len = fill_.Length(data_str, pcb);          // Length of the data we are using
	VERIFY(::CloseClipboard());
    char buf[24];
    sprintf(buf, "%I64u", __int64(data_len * fill_repeat_));
	CString tmp(buf);
	::AddCommas(tmp);
    ctl_size_length_.SetWindowText(tmp);
}

void CNewFile::check_clipboard() 
{
	clipboard_format_ = -1;             // default to nothing found on clipboard
	clipboard_length_ = -1;

    if (!OpenClipboard())
		return;

    UINT ff = 0;                        // Clipboard format number
    HANDLE hh;                          // Handle to clipboard memory
    unsigned char *pp;                          // Pointer to actual data

	// First search for our binary data format
	while ((ff = ::EnumClipboardFormats(ff)) != 0)
    {
		clipboard_format_ = 0;        // There is at least something there

		// Get name of this format
        char name[16];
        size_t nlen = ::GetClipboardFormatName(ff, name, 15);
        name[nlen] = '\0';
        if (strcmp(name, "BinaryData") == 0)
        {
            // BINARY DATA
            if ((hh = ::GetClipboardData(ff)) != NULL &&
                (pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
            {
				clipboard_length_ = *((long *)pp);  // Length of bin data is in first 4 bytes
                pp += sizeof(long);
				char buf[64];
				char *pout;

				// Get a sample of the data
				long ii;
				for (ii = 0, pout = buf; ii < clipboard_length_ && ii < 11L; ++ii, pout += 3)
					sprintf(pout, theApp.hex_ucase_ ? "%02.2X " : "%02.2x ", pp[ii]);

				if (ii < clipboard_length_)
                    strcat(buf, "...");

                ASSERT(GetDlgItem(IDC_FILL_CLIPBOARD_SAMPLE) != NULL);
                GetDlgItem(IDC_FILL_CLIPBOARD_SAMPLE)->SetWindowText(buf);

                CString strLen;
                strLen.Format("%d", clipboard_length_);
                ::AddCommas(strLen);
                ctl_fill_clipboard_length_.SetWindowText(strLen);

				clipboard_format_ = 1;
			}
			break;
		}
	}

    if (clipboard_format_ < 1)
    {
	    // Check if there is any text data on the clipboard
        if ((hh = ::GetClipboardData(CF_TEXT)) != NULL &&
            (pp = reinterpret_cast<unsigned char *>(::GlobalLock(hh))) != NULL)
        {
		    clipboard_length_ = strlen(((char *)pp));

            ASSERT(GetDlgItem(IDC_FILL_CLIPBOARD_SAMPLE) != NULL);
		    // Get a sample of the data
		    if (clipboard_length_ <= 40)
                GetDlgItem(IDC_FILL_CLIPBOARD_SAMPLE)->SetWindowText((char *)pp);
		    else
                GetDlgItem(IDC_FILL_CLIPBOARD_SAMPLE)->SetWindowText(CString((char *)pp, 40) + "...");

            CString strLen;
            strLen.Format("%d", clipboard_length_);
            ::AddCommas(strLen);
            ctl_fill_clipboard_length_.SetWindowText(strLen);

		    clipboard_format_ = 2;
	    }
    }
	if (clipboard_format_ < 1 && fill_.type == FILL_CLIPBOARD)
    {
        ctl_fill_clipboard_sample_.SetWindowText("");
        ctl_fill_clipboard_length_.SetWindowText("");
        UpdateData();
		fill_.type = FILL_HEX;			// Unrec. cb format so clipboard option will be disabled
        UpdateData(FALSE);
    }
	ASSERT(GetDlgItem(IDC_FILL_CLIPBOARD) != NULL);
	GetDlgItem(IDC_FILL_CLIPBOARD)->EnableWindow(clipboard_format_ > 0);

	::CloseClipboard();
	return;
}

/////////////////////////////////////////////////////////////////////////////
// CNewFile message handlers

BOOL CNewFile::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Add ouselves to the clipboard chain so we find out when cb changes (via WM_DRAWCLIPBOARD)
	next_cb_hwnd_ = SetClipboardViewer();

	// We have hijacked the "New File" dialog for the purpose of inserting a 
	// block filled in the same ways as the new file dialog.  The only diff.
	// to the dialog is the title (and IDC_FILL_DISK check box is hidden).
	if (insert_block_mode_)
		SetWindowText(_T("Insert Block"));

	// Check if text or binary data available from clipboard and get first bit as a sample
	check_clipboard();

    ASSERT(GetDlgItem(IDC_SPIN_SIZE_FACTOR) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_SIZE_FACTOR))->SetRange32(1, INT_MAX);

    ASSERT(GetDlgItem(IDC_SPIN_DECIMAL_SIZE) != NULL);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_DECIMAL_SIZE))->SetRange32(0, INT_MAX);
    ctl_hex_size_.set_right_align();
    ctl_hex_value_.set_group_by(2);

    char buf[24];

    // Update the decimal number
    //sprintf(buf, "%I64u", (__int64(fill_.size_high)<<32) + fill_.size_low);
    sprintf(buf, "%I64u", __int64(fill_size_));
    ctl_decimal_size_.SetWindowText(buf);
    ctl_decimal_size_.add_commas();

    // Update the hex number
    //sprintf(buf, "%I64x", (__int64(fill_.size_high)<<32) + fill_.size_low);
    sprintf(buf, "%I64x", __int64(fill_size_));
    ctl_hex_size_.SetWindowText(buf);
    ctl_hex_size_.add_spaces();

	// Update the repeat count
    sprintf(buf, "%I64u", __int64(fill_repeat_));
    ctl_size_factor_.SetWindowText(buf);
    ctl_size_factor_.add_commas();

    in_update_ = FALSE;

    fix_controls();

	return TRUE;
}

void CNewFile::OnOK() 
{
    UpdateData();

    switch (fill_.type)
    {
    case FILL_HEX:
        // Make sure at least one hex digit has been entered
        if (strcspn(fill_hex_value_, "0123456789abcdefABCDEF") == fill_hex_value_.GetLength())
        {
            AfxMessageBox("Please enter at least one hex digit");
            ctl_hex_value_.SetFocus();
            return;
        }
        break;
    case FILL_STRING:
        // Make sure the string is not empty
        if (fill_string_value_.IsEmpty())
        {
            AfxMessageBox("Please enter one or more characters");
            ctl_fill_string_value_.SetFocus();
            return;
        }
        break;
    case FILL_RANDOM:
        {
            range_set<int> rand_rs;    // The range(s) of values for random fills
            std::istringstream str((const char *)(fill_random_range_));
            str >> rand_rs;

            if (rand_rs.empty() || *(rand_rs.begin()) > 255)
            {
                AfxMessageBox("Please enter at least one byte\r\n"
                              "value in the range 0 to 255.");
                ctl_fill_random_range_.SetFocus();
                return;
            }
        }
        // Make sure there is at least one byte between 0 and 255 in the range
        break;
    case FILL_NUMBER:
        // Make sure there is a number
        if (strcspn(fill_number_value_, "0123456789") == fill_number_value_.GetLength())
        {
            AfxMessageBox("Please enter a number");
            ctl_fill_number_value_.SetFocus();
            return;
        }
        break;
    }

	// Save 40 bit number from fill_size_ (if size_src == 0) or fill_repeat_ (if size_src == 1)
	if (fill_.size_src == 1)
	{
		fill_.size_low = (unsigned long)fill_repeat_;
		fill_.size_high = (unsigned char)(fill_repeat_>>32);
	}
	else
	{
		fill_.size_low = (unsigned long)fill_size_;
		fill_.size_high = (unsigned char)(fill_size_>>32);
	}

    // Save used values in registry for restore next time
	char buf[32];
	sprintf(buf, "%I64d", fill_state_);
    theApp.WriteProfileString("Fill", "State", buf);

    theApp.WriteProfileString("Fill", "Hex",       fill_hex_value_);
    theApp.WriteProfileString("Fill", "String",    fill_string_value_);
    theApp.WriteProfileString("Fill", "Random",    fill_random_range_);
    theApp.WriteProfileString("Fill", "Number",    fill_number_value_);

	CDialog::OnOK();
}

void CNewFile::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_NEW_FILE_HELP))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_NEW_FILE_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

static DWORD id_pairs[] = { 
    IDC_FILL_SIZE_DATA, HIDC_FILL_SIZE_DATA,
	IDC_SIZE_FACTOR, HIDC_SIZE_FACTOR,
	IDC_SIZE_LENGTH, HIDC_SIZE_LENGTH,
    IDC_FILL_SIZE_CALC, HIDC_FILL_SIZE_CALC,
    IDC_FILL_SIZE_SPECIFIED, HIDC_FILL_SIZE_SPECIFIED,
    IDC_DECIMAL_SIZE, HIDC_DECIMAL_SIZE,
    IDC_SPIN_DECIMAL_SIZE, HIDC_DECIMAL_SIZE,
    IDC_HEX_SIZE, HIDC_HEX_SIZE,
    IDC_FILL_DISK, HIDC_FILL_DISK,
    IDC_FILL_HEX, HIDC_FILL_HEX,
    IDC_FILL_STRING, HIDC_FILL_STRING,
    IDC_FILL_CLIPBOARD, HIDC_FILL_CLIPBOARD,
    IDC_FILL_RANDOM, HIDC_FILL_RANDOM,
    IDC_FILL_NUMBER, HIDC_FILL_NUMBER,
    IDC_FILL_HEX_VALUE, HIDC_FILL_HEX_VALUE,
    IDC_FILL_STRING_VALUE, HIDC_FILL_STRING_VALUE,
    IDC_FILL_STRING_CHARSET, HIDC_FILL_STRING_CHARSET,
    IDC_DESC_FILL_STRING_CHARSET, HIDC_FILL_STRING_CHARSET,
    IDC_FILL_CLIPBOARD_SAMPLE, HIDC_FILL_CLIPBOARD_SAMPLE,
    IDC_DESC_FILL_CLIPBOARD_LENGTH, HIDC_FILL_CLIPBOARD_LENGTH,
    IDC_FILL_CLIPBOARD_LENGTH, HIDC_FILL_CLIPBOARD_LENGTH,
    IDC_FILL_RANDOM_RANGE, HIDC_FILL_RANDOM_RANGE,
    IDC_FILL_NUMBER_VALUE, HIDC_FILL_NUMBER_VALUE,
    IDC_FILL_NUMBER_TYPE, HIDC_FILL_NUMBER_TYPE,
    IDC_DESC_FILL_NUMBER_TYPE, HIDC_FILL_NUMBER_TYPE,
    IDC_BIG_ENDIAN, HIDC_BIG_ENDIAN,
    0,0 
}; 

void CNewFile::OnDestroy() 
{
	// Remove ouselves from clipboard chain (see SetClipboardViewer() above)
	ChangeClipboardChain(next_cb_hwnd_);

    CDialog::OnDestroy();
}

// Called when the contents of the clipboard changes
void CNewFile::OnDrawClipboard()
{
	if (next_cb_hwnd_ != 0)
		::SendMessage(next_cb_hwnd_, WM_DRAWCLIPBOARD, (WPARAM)0, (LPARAM)0);  // signal next HWND in chain

	check_clipboard();
    fix_controls();
}

// Called when another window is being removed from clipboard chain
void CNewFile::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
	if (hWndRemove == next_cb_hwnd_)                                          // our successor is going - bye-bye
		next_cb_hwnd_ = hWndAfter;
	else if (next_cb_hwnd_ != 0)
		::SendMessage(next_cb_hwnd_, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter);  // pass the message on
}

BOOL CNewFile::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
                   HELP_WM_HELP, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CNewFile::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CNewFile::OnChangeDecimalSize() 
{
    if (in_update_) return;
    in_update_ = TRUE;

    // Get the number
    const char *pp;
    char buf[20];
    char *dd = buf;
    CString ss;
    ctl_decimal_size_.GetWindowText(ss);
    for (pp = ss.GetBuffer(0); dd < buf+sizeof(buf)-1 && *pp != '\0'; ++pp)
    {
        if (isdigit(*pp))
            *dd++ = *pp;
    }
    *dd = '\0';

    fill_size_ = ::strtoi64(buf, 10);
	//fill_.size_low = (unsigned long)fill_size;
	//fill_.size_high = (unsigned char)(fill_size>>32);

    // Update the hex number
    sprintf(buf, "%I64x", __int64(fill_size_));
    ctl_hex_size_.SetWindowText(buf);
    ctl_hex_size_.add_spaces();

    in_update_ = FALSE;
}

void CNewFile::OnChangeHexSize() 
{
    if (in_update_) return;
    in_update_ = TRUE;

    // Get the number
    const char *pp;
    char buf[16];
    char *dd = buf;
    CString ss;
    ctl_hex_size_.GetWindowText(ss);
    for (pp = ss.GetBuffer(0); dd < buf+sizeof(buf)-1 && *pp != '\0'; ++pp)
    {
        if (isxdigit(*pp))
            *dd++ = *pp;
    }
    *dd = '\0';

    fill_size_ = ::strtoi64(buf, 16);
	//fill_.size_low = (unsigned long)fill_size;
	//fill_.size_high = (unsigned char)(fill_size>>32);

    // Update the decimal number
    sprintf(buf, "%I64u", __int64(fill_size_));
    ctl_decimal_size_.SetWindowText(buf);
    ctl_decimal_size_.add_commas();

    in_update_ = FALSE;
}

void CNewFile::OnChangeSizeFactor()
{
    if (in_update_) return;
    in_update_ = TRUE;

    // Get the number
    const char *pp;
    char buf[20];
    char *dd = buf;
    CString ss;
    ctl_size_factor_.GetWindowText(ss);
    for (pp = ss.GetBuffer(0); dd < buf+sizeof(buf)-1 && *pp != '\0'; ++pp)
    {
        if (isdigit(*pp))
            *dd++ = *pp;
    }
    *dd = '\0';

    fill_repeat_ = ::strtoi64(buf, 10);

    in_update_ = FALSE;

	fix_controls();
}

void CNewFile::OnChangeFill() 
{
    UpdateData();

    fix_controls();
}

void CNewFile::OnChangeHex() 
{
    UpdateData();

	calc_length();   // Number of hex digits may have changed so update length
}

void CNewFile::OnChangeString() 
{
    UpdateData();

	calc_length();   // Number of chars may have changed so update length
}

void CNewFile::OnSelchangeCharset() 
{
    UpdateData();

	calc_length();   // Switching between Unicode and other char set changes length
}

void CNewFile::OnChangeFillNumber() 
{
    UpdateData();

    fix_controls();
}

void CNewFile::OnSelchangeFillNumberType() 
{
    UpdateData();

    fix_controls();
}

// Returns length of data (depends on type) or 0 on error
// For FILL_HEX and FILL_STRING data_str is the data string (hex digits for FILL_HEX)
// For FILL_CLIPBOARD pcb is where the pointer to the clipboard data is placed
// Note: If FILL_CLIPBOARD then clipboard must be open on entry (see ::OpenClipboard())
//       and must be closed again, of course, afterwards (with ::CloseClipboard()).
size_t CNewFile::fill_bits::Length(const char *data_str, const char * &pcb)
{
	size_t data_len = 0;

    switch (type)
    {
    default:
        ASSERT(0);
        // fall through
    case CNewFile::FILL_HEX:   // count hex digits
        {
            const char *pp = data_str;
            int ii, count;
            for (count = 0 ; ii = strcspn(pp, "0123456789abcdefABCDEF"), pp[ii] != '\0'; pp += ii + 1)
                ++count;
            data_len = (count+1)/2;    // 2 hex digits per byte
        }
        break;
    case CNewFile::FILL_STRING:   // string
        data_len = strlen(data_str);
        if (string_charset == 2)
            data_len *= 2;        // Unicode characters are twice as wide
        break;
    case CNewFile::FILL_CLIPBOARD:   // clipboard
		{
            UINT ff = 0;                        // Clipboard format number
            HANDLE hh;                          // Handle to clipboard memory
            const char *pp;                     // Pointer to actual data

            // First search for our binary data format
            while ((ff = ::EnumClipboardFormats(ff)) != 0)
            {
                // Get name of this format
                char name[16];
                size_t nlen = ::GetClipboardFormatName(ff, name, 15);
                name[nlen] = '\0';
                if (strcmp(name, "BinaryData") == 0)
                {
                    // BINARY DATA
                    if ((hh = ::GetClipboardData(ff)) != NULL &&
                        (pp = reinterpret_cast<char *>(::GlobalLock(hh))) != NULL)
                    {
                        data_len = size_t(*((long *)pp));
                        pcb = pp + sizeof(long);
                    }
                    break;   // No need to keep scanning formats
                }
            }

            // Check if there is any text data on the clipboard
            if (pcb == NULL &&
                (hh = ::GetClipboardData(CF_TEXT)) != NULL &&
                (pp = reinterpret_cast<char *>(::GlobalLock(hh))) != NULL)
            {
                data_len = strlen((char *)pp);
                pcb = pp;
            }
        }
        if (pcb == NULL)
        {
            // No relevant clipboard types found
            ((CMainFrame *)AfxGetMainWnd())->StatusBarText("No recognized formats found on clipboard");
            theApp.mac_error_ = 1;
            return 0;
        }
        break;
    case CNewFile::FILL_RANDOM:   // random - default to one byte
        data_len = 1;
        break;
    case CNewFile::FILL_NUMBER:   // number - get size from type
        if (number_is_float)
        {
            data_len = number_type == 0 ? 4 : 8;    // float or double
        }
        else
        {
            data_len = number_type == 0 ? 1 :
                       number_type == 1 ? 2 :
                       number_type == 2 ? 4 : 8;
        }
        break;
    }

	return data_len;
}
