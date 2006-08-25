// DFFDData.cpp : implementation file
//
// Copyright (c) 2004 by Andrew W. Phillips.
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

#include "stdafx.h"
#include "hexedit.h"
#include "HexEditDoc.h"
#include "DataFormatView.h"
#include "DFFDData.h"
#include "DFFDMisc.h"
#include "resource.hm"      // Help IDs
#include "HelpID.hm"            // User defined help IDs

#ifdef USE_HTML_HELP
#include <HtmlHelp.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDFFDData dialog

CDFFDData::CDFFDData(CXmlTree::CElt *pp, signed char parent_type, CWnd* pParent /*=NULL*/)
    : CDialog(CDFFDData::IDD, pParent), saved_(pp->GetOwner())
{
    modified_ = false;
    first_ = false;                     // Default to first (see SetPosition())

    pelt_ = pp;
    parent_type_ = parent_type;

    saved_.SaveKids(pelt_);
    saved_mod_flag_ = pelt_->GetOwner()->IsModified();
    show_prev_next_ = !pParent->IsKindOf(RUNTIME_CLASS(CDataFormatView)) &&
                      (parent_type == CHexEditDoc::DF_FILE ||
                       parent_type == CHexEditDoc::DF_STRUCT ||
                       parent_type == CHexEditDoc::DF_DEFINE_STRUCT ||
                       parent_type == CHexEditDoc::DF_SWITCH ||
                       parent_type == CHexEditDoc::DF_UNION);

    pos_.x = pos_.y = -30000;
    plength_menu_ = NULL;
    pdomain_menu_ = NULL;

    //{{AFX_DATA_INIT(CDFFDData)
    comments_ = _T("");
    domain_ = _T("");
    name_ = _T("");
    type_name_ = _T("");
    units_ = _T("");
    big_endian_ = FALSE;
    display_ = _T("");
    read_only_ = FALSE;
    type_ = -1;
    format_ = -1;
    length_ = _T("");
    //}}AFX_DATA_INIT
    bitfield_ = FALSE;
	bits_ = 1;
	dirn_ = 0;
}

CDFFDData::~CDFFDData()
{
    if (plength_menu_ != NULL)
    {
        // Destroy the menu and free the memory
        plength_menu_->DestroyMenu();
        delete plength_menu_;
    }
    if (pdomain_menu_ != NULL)
    {
        // Destroy the menu and free the memory
        pdomain_menu_->DestroyMenu();
        delete pdomain_menu_;
    }
}

void CDFFDData::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDFFDData)
    DDX_Control(pDX, IDC_DFFD_NAME, ctl_name_);
    DDX_Control(pDX, ID_DFFD_PREV, ctl_prev_);
    DDX_Control(pDX, ID_DFFD_NEXT, ctl_next_);
    DDX_Control(pDX, IDC_DFFD_DISPLAY, ctl_display_);
    DDX_Control(pDX, IDC_DFFD_BIG_ENDIAN, ctl_big_endian_);
    DDX_Control(pDX, IDC_DFFD_UNITS_DESC, ctl_units_desc_);
    DDX_Control(pDX, IDC_DFFD_UNITS, ctl_units_);
    DDX_Control(pDX, IDC_DFFD_TYPE, ctl_type_);
    DDX_Control(pDX, IDC_DFFD_READ_ONLY, ctl_read_inly_);
    DDX_Control(pDX, IDC_DFFD_LENGTH_DESC, ctl_length_desc_);
    DDX_Control(pDX, IDC_DFFD_LENGTH, ctl_length_);
    DDX_Control(pDX, IDC_DFFD_DOMAIN_DESC, ctl_domain_desc_);
    DDX_Control(pDX, IDC_DFFD_FORMAT, ctl_format_);
    DDX_Control(pDX, IDC_DFFD_FORMAT_DESC, ctl_format_desc_);
    DDX_Control(pDX, IDC_DFFD_DOMAIN, ctl_domain_);
    DDX_Control(pDX, IDC_DFFD_DISPLAY_DESC, ctl_display_desc_);
    DDX_Text(pDX, IDC_DFFD_COMMENTS, comments_);
    DDX_Text(pDX, IDC_DFFD_DOMAIN, domain_);
    DDX_Text(pDX, IDC_DFFD_NAME, name_);
    DDX_Text(pDX, IDC_DFFD_TYPE_NAME, type_name_);
    DDX_Text(pDX, IDC_DFFD_UNITS, units_);
    DDX_Check(pDX, IDC_DFFD_BIG_ENDIAN, big_endian_);
    DDX_Text(pDX, IDC_DFFD_DISPLAY, display_);
    DDX_Check(pDX, IDC_DFFD_READ_ONLY, read_only_);
    DDX_CBIndex(pDX, IDC_DFFD_TYPE, type_);
    DDX_CBIndex(pDX, IDC_DFFD_FORMAT, format_);
    DDX_Text(pDX, IDC_DFFD_LENGTH, length_);
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_DFFD_LENGTH_VAR, ctl_length_var_);
    DDX_Control(pDX, IDC_DFFD_DOMAIN_VAR, ctl_domain_var_);
    DDX_Control(pDX, ID_DFFD_CLONE, ctl_clone_);

	// Bit-fields
    DDX_Control(pDX, IDC_DFFD_BITFIELD, ctl_bitfield_);
    DDX_Check(pDX, IDC_DFFD_BITFIELD, bitfield_);
    DDX_Control(pDX, IDC_DFFD_BITFIELD_BITS, ctl_bits_);
	DDX_Text(pDX, IDC_DFFD_BITFIELD_BITS, bits_);
    DDX_Control(pDX, IDC_DFFD_BITFIELD_DIRN, ctl_dirn_);
	DDX_CBIndex(pDX, IDC_DFFD_BITFIELD_DIRN, dirn_);
}


BEGIN_MESSAGE_MAP(CDFFDData, CDialog)
    //{{AFX_MSG_MAP(CDFFDData)
    ON_BN_CLICKED(ID_DFFD_NEXT, OnNext)
    ON_BN_CLICKED(ID_DFFD_PREV, OnPrev)
    ON_EN_CHANGE(IDC_DFFD_NAME, OnChange)
    ON_EN_CHANGE(IDC_DFFD_DOMAIN, OnChange)
    ON_EN_CHANGE(IDC_DFFD_COMMENTS, OnChange)
    ON_EN_CHANGE(IDC_DFFD_TYPE_NAME, OnChange)
    ON_EN_CHANGE(IDC_DFFD_UNITS, OnChange)
    ON_EN_CHANGE(IDC_DFFD_DISPLAY, OnChange)
    ON_BN_CLICKED(IDC_DFFD_READ_ONLY, OnChange)
    ON_BN_CLICKED(IDC_DFFD_BIG_ENDIAN, OnChange)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
    ON_CBN_SELCHANGE(IDC_DFFD_TYPE, OnSelchangeType)
    ON_CBN_SELCHANGE(IDC_DFFD_FORMAT, OnSelchangeFormat)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(ID_DFFD_HELP, OnHelp)
    ON_BN_CLICKED(IDC_DFFD_LENGTH_VAR, OnGetLengthVar)
    ON_BN_CLICKED(IDC_DFFD_DOMAIN_VAR, OnGetDomainVar)
    ON_BN_CLICKED(ID_DFFD_CLONE, OnClone)
    ON_EN_CHANGE(IDC_DFFD_LENGTH, OnChangeLength)
    ON_BN_CLICKED(IDC_DFFD_BITFIELD, OnBitfield)
    ON_EN_CHANGE(IDC_DFFD_BITFIELD_BITS, OnChange)
    ON_CBN_SELCHANGE(IDC_DFFD_TYPE, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFFDData message handlers

BOOL CDFFDData::OnInitDialog() 
{
    CDialog::OnInitDialog();
    SetIcon(AfxGetApp()->LoadIcon(IDI_DATA), TRUE);

    // If window position specified
    if (pos_.y != -30000)
    {
        CRect rr;               // Rectangle where we will put the dialog
        GetWindowRect(&rr);

        // Move to where it was when it was last closed
        rr.OffsetRect(pos_.x - rr.left, pos_.y - rr.top);
		(void)::NeedsFix(rr);  // Make sure its visible

        MoveWindow(&rr);
    }

    if (!show_prev_next_)
    {
        // Hide Clone>>, << and >> (previous/next) buttons
        ctl_clone_.ShowWindow(SW_HIDE);
        ctl_prev_.ShowWindow(SW_HIDE);
        ctl_next_.ShowWindow(SW_HIDE);
    }
    else
    {
        // Check if there are older/younger siblings
        CXmlTree::CElt ee;
		if (parent_type_ == CHexEditDoc::DF_SWITCH)
			ee = pelt_->GetParent();
		else
			ee = *pelt_;
        ee--;
        ctl_prev_.EnableWindow(!ee.IsEmpty());

		if (parent_type_ == CHexEditDoc::DF_SWITCH)
			ee = pelt_->GetParent();
		else
			ee = *pelt_;
        ++ee;
        ctl_next_.EnableWindow(!ee.IsEmpty());
    }

    load_data();

    VERIFY(button_menu_.LoadMenu(IDR_DFFD));

    // Set up menu that allows the user to choose any valid variable name
    plength_menu_ = make_var_menu_tree(*pelt_);

    if (type_ == 0 || type_ == 5)
        ctl_length_var_.m_hMenu = plength_menu_->GetSafeHmenu();
    else if (type_ == 2)
        ctl_length_var_.m_hMenu = button_menu_.GetSubMenu(5)->GetSafeHmenu();
    else if (type_ == 3)
        ctl_length_var_.m_hMenu = button_menu_.GetSubMenu(6)->GetSafeHmenu();
    else
    {
        // Disable the "length" popup menu since char and date types don't need a length
        ASSERT(type_ == 1 || type_ == 4);
        ctl_length_var_.EnableWindow(FALSE);
    }

    ASSERT(GetDlgItem(IDC_DFFD_BITFIELD_BITS_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_DFFD_BITFIELD_DIRN_DESC) != NULL);
    ASSERT(GetDlgItem(IDC_SPIN_BITFIELD_BITS) != NULL);
	ctl_bitfield_.EnableWindow(type_ == 2);
	ctl_bits_.EnableWindow(type_ == 2 && bitfield_);
    GetDlgItem(IDC_DFFD_BITFIELD_BITS_DESC)->EnableWindow(type_ == 2 && bitfield_);
	ctl_dirn_.EnableWindow(type_ == 2 && bitfield_);
    GetDlgItem(IDC_DFFD_BITFIELD_DIRN_DESC)->EnableWindow(type_ == 2 && bitfield_);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_BITFIELD_BITS))->SetRange(1, atoi(length_)*8-1);

    ctl_length_var_.m_bOSMenu = TRUE;
    ctl_length_var_.m_bStayPressed = TRUE;
    ctl_length_var_.m_bRightArrow = TRUE;
    ctl_length_.SetSel(0, -1);           // Select all so a var selection (see OnGetLengthVar) replaces current contents

    pdomain_menu_ = make_var_menu_tree(*pelt_, true);
    ctl_domain_var_.m_hMenu = pdomain_menu_->GetSafeHmenu();
    ctl_domain_var_.m_bOSMenu = TRUE;
    ctl_domain_var_.m_bStayPressed = TRUE;
    ctl_domain_var_.m_bRightArrow = TRUE;
    ctl_domain_.SetSel(0, -1);            // Select all so a var selection (see OnGetDomainVar) replaces current contents

    return TRUE;
}


void CDFFDData::OnCancel() 
{
    pelt_->DeleteAllChildren();
    saved_.InsertKids(pelt_);
    pelt_->GetOwner()->SetModified(saved_mod_flag_);

    CDialog::OnCancel();
}

void CDFFDData::OnOK()
{
    if (modified_)
    {
        if (!check_data())
            return;

        save_data();
    }

    CDialog::OnOK();
}

void CDFFDData::OnClone() 
{
    // Does the same as OnOK but returns ID_DFFD_CLONE to indicate
    // that a sibling should be created that is a clone of this one
    if (modified_)
    {
        if (!check_data())
            return;

        save_data();
    }

    // Save position so sibling node's dialog can be put in the same place
    pos_ = update_posn(this);

    CDialog::EndDialog(ID_DFFD_CLONE);
}

void CDFFDData::OnNext() 
{
    // Does the same as OnOK but returns ID_DFFD_NEXT to indicate
    // that the next sibling should now be edited
    if (modified_)
    {
        if (!check_data())
            return;

        save_data();
    }

    // Save position so sibling node's dialog can be put in the same place
    pos_ = update_posn(this);

    CDialog::EndDialog(ID_DFFD_NEXT);
}

void CDFFDData::OnPrev() 
{
    // Does the same as OnOK but returns ID_DFFD_PREV to indicate
    // that the previous sibling should now be edited
    if (modified_)
    {
        if (!check_data())
            return;

        save_data();
    }

    // Save position so sibling node's dialog can be put in the same place
    pos_ = update_posn(this);

    CDialog::EndDialog(ID_DFFD_PREV);
}

LRESULT CDFFDData::OnKickIdle(WPARAM, LPARAM lCount)
{
    return FALSE;
}

void CDFFDData::OnSelchangeType()
{
    if (type_ != ctl_type_.GetCurSel())
    {
        type_ = ctl_type_.GetCurSel();
		format_ = -1;
        fix_controls();

        if (type_ == 0 || type_ == 5)
            ctl_length_var_.m_hMenu = plength_menu_->GetSafeHmenu();
        else if (type_ == 2)
            ctl_length_var_.m_hMenu = button_menu_.GetSubMenu(5)->GetSafeHmenu();
        else if (type_ == 3)
            ctl_length_var_.m_hMenu = button_menu_.GetSubMenu(6)->GetSafeHmenu();

    }
    modified_ = true;
}

void CDFFDData::OnSelchangeFormat() 
{
    if (format_ != ctl_format_.GetCurSel())
	{
		format_ = ctl_format_.GetCurSel();
		fix_big_endian();
	}
    modified_ = true;
}

void CDFFDData::OnChangeLength() 
{
	UpdateData();         // get length from control
    fix_controls();       // update spin control range
    modified_ = true;
}

void CDFFDData::OnBitfield()
{
	UpdateData();
    fix_controls();       // enable other controls depending on whether bit-fields now on or off
    modified_ = true;
}

void CDFFDData::OnChange() 
{
    modified_ = true;
}

void CDFFDData::fix_controls()
{
    // Set up the valid formats for the type
    ctl_format_.EnableWindow(TRUE);

    // Note these numbers match the order of items in the type dropdown list (IDC_DFFD_TYPE)
    ctl_format_.ResetContent();
    switch (type_)
    {
    case 0:     // none
        /* do nothing here */           // does not have any formats
        break;
    case 1:     // character
    case 5:     // string
        ctl_format_.AddString("Default");
        ctl_format_.AddString("ASCII");
        ctl_format_.AddString("ANSI");
        ctl_format_.AddString("IBM/OEM");
        ctl_format_.AddString("EBCDIC");
        ctl_format_.AddString("Unicode");
        break;
    case 2:     // integer
        ctl_format_.AddString("signed");
        ctl_format_.AddString("unsigned");
        ctl_format_.AddString("sign + magnitude");
        break;
    case 3:     // real
        ctl_format_.AddString("IEEE");
        ctl_format_.AddString("IBM");
        break;
    case 4:     // date
        ctl_format_.AddString("normal time_t");
        ctl_format_.AddString("time_t (MSC 5.1)");
        ctl_format_.AddString("time_t (MSC 7)");
        ctl_format_.AddString("time_t (minutes)");
        ctl_format_.AddString("OLE DATE/TIME");
        ctl_format_.AddString("SYSTEMTIME");
        ctl_format_.AddString("FILETIME");
        ctl_format_.AddString("MSDOS date/time");
        break;
    }

    ctl_format_.EnableWindow(type_ != 0);
    ctl_format_desc_.EnableWindow(type_ != 0);
    ctl_display_.EnableWindow(type_ != 0);
    ctl_display_desc_.EnableWindow(type_ != 0);
    ctl_domain_.EnableWindow(type_ != 0);
    ctl_domain_desc_.EnableWindow(type_ != 0);
    ctl_domain_var_.EnableWindow(type_ != 0);

    ctl_length_.EnableWindow(type_ != 1 && type_ != 4);
    ctl_length_desc_.EnableWindow(type_ != 1 && type_ != 4);
    ctl_length_var_.EnableWindow(type_ != 1 && type_ != 4);
	fix_big_endian();
    ctl_units_.EnableWindow(type_ != 0 && type_ != 4);
    ctl_units_desc_.EnableWindow(type_ != 0 && type_ != 4);
    ctl_units_desc_.SetWindowText(type_ == 5 ? "Terminator:" : "Units:");

	// Enable bitfield option if type is int
	ctl_bitfield_.EnableWindow(type_ == 2);
	ctl_bits_.EnableWindow(type_ == 2 && bitfield_);
    GetDlgItem(IDC_DFFD_BITFIELD_BITS_DESC)->EnableWindow(type_ == 2 && bitfield_);
	ctl_dirn_.EnableWindow(type_ == 2 && bitfield_);
    GetDlgItem(IDC_DFFD_BITFIELD_DIRN_DESC)->EnableWindow(type_ == 2 && bitfield_);
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_BITFIELD_BITS))->SetRange(1, atoi(length_)*8-1);
} // fix_controls

void CDFFDData::fix_big_endian()
{
    ctl_big_endian_.EnableWindow(type_ ==2 || type_ == 3 ||       // numeric 
		                         type_ == 4 ||                    // date
								 (type_ == 1 && format_ == 5) ||  // unicode char
								 (type_ ==5 && format_ == 5));    // unicode string
}

void CDFFDData::OnGetLengthVar()
{
    if (ctl_length_var_.m_nMenuResult != 0)
    {
        if (ctl_length_var_.m_nMenuResult == ID_DFFD_BYTE)
        {
            ctl_length_.SetWindowText("1");
            ctl_length_.SetFocus();
            modified_ = true;
            return;
        }
        else if (ctl_length_var_.m_nMenuResult == ID_DFFD_WORD)
        {
            ctl_length_.SetWindowText("2");
            ctl_length_.SetFocus();
            modified_ = true;
            return;
        }
        else if (ctl_length_var_.m_nMenuResult == ID_DFFD_DWORD)
        {
            ctl_length_.SetWindowText("4");
            ctl_length_.SetFocus();
            modified_ = true;
            return;
        }
        else if (ctl_length_var_.m_nMenuResult == ID_DFFD_QWORD)
        {
            ctl_length_.SetWindowText("8");
            ctl_length_.SetFocus();
            modified_ = true;
            return;
        }

        CString strVar = get_menu_text(plength_menu_, ctl_length_var_.m_nMenuResult);
        ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

        // Put the var name into the "length" control
        for (int ii = 0; ii < strVar.GetLength (); ii++)
            ctl_length_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
        ctl_length_.SetFocus();
        modified_ = true;
    }
}

void CDFFDData::OnGetDomainVar()
{
    if (ctl_domain_var_.m_nMenuResult != 0)
    {
        CString strVar = get_menu_text(pdomain_menu_, ctl_domain_var_.m_nMenuResult);
        ASSERT(!strVar.IsEmpty());      // The selected menu item should be found in the menu tree

        // Put the var name into the "domain" control
        for (int ii = 0; ii < strVar.GetLength (); ii++)
            ctl_domain_.SendMessage(WM_CHAR, (TCHAR)strVar[ii]);
        ctl_domain_.SetFocus();
        modified_ = true;
    }
}

void CDFFDData::load_data()
{
#if 0  // Stored in item list of control (see dialog editor)
    // Set up the type list
    ctl_format_.ResetContent();
    ctl_type_.AddString("none");
    ctl_type_.AddString("character");
    ctl_type_.AddString("integer");
    ctl_type_.AddString("real");
    ctl_type_.AddString("date");
    ctl_type_.AddString("string");
#endif

    // Get values from XML data node
    ASSERT(pelt_ != NULL && pelt_->GetName() == "data");
    name_ = pelt_->GetAttr("name");

    // Work out the type so that we can set up the other controls
    CString type_str = pelt_->GetAttr("type");

    if (type_str.CompareNoCase("none") == 0)
        type_ = 0;
    else if (type_str.CompareNoCase("char") == 0)
        type_ = 1;
    else if (type_str.CompareNoCase("int") == 0)
        type_ = 2;
    else if (type_str.CompareNoCase("real") == 0)
        type_ = 3;
    else if (type_str.CompareNoCase("date") == 0)
        type_ = 4;
    else if (type_str.Left(6).CompareNoCase("string") == 0)
        type_ = 5;
    else
    {
        ASSERT(0);
        type_ = 0;
    }

    CString format_str = pelt_->GetAttr("format");

    switch (type_)
    {
    case 0:     // none
        format_ = 0;     // No formats supported for type "none"
        break;
    case 1:     // character
        if (format_str.CompareNoCase("ascii") == 0)
            format_ = 1;
        else if (format_str.CompareNoCase("ansi") == 0)
            format_ = 2;
        else if (format_str.CompareNoCase("oem") == 0)
            format_ = 3;
        else if (format_str.CompareNoCase("ebcdic") == 0)
            format_ = 4;
        else if (format_str.CompareNoCase("unicode") == 0)
            format_ = 5;
        else
            format_ = 0;
        break;
    case 2:     // integer
        if (format_str.GetLength() < 1)
            format_ = 0;
        else if (toupper(format_str[0]) == 'S')  // signed
            format_ = 0;
        else if (toupper(format_str[0]) == 'U')  // unsigned
            format_ = 1;
        else if (toupper(format_str[0]) == 'M')  // sign + magnitude
            format_ = 2;
        else
        {
            ASSERT(0);
            format_ = 0;
        }
        break;
    case 3:     // real
        if (format_str.CompareNoCase("ieee") == 0)
            format_ = 0;
        else if (format_str.CompareNoCase("ibm") == 0)
            format_ = 1;
        else
            format_ = 0;
        break;
    case 4:     // date
        if (format_str.CompareNoCase("c") == 0)
            format_ = 0;
        else if (format_str.CompareNoCase("c51") == 0)
            format_ = 1;
        else if (format_str.CompareNoCase("c7") == 0)
            format_ = 2;
        else if (format_str.CompareNoCase("cmin") == 0)
            format_ = 3;
        else if (format_str.CompareNoCase("ole") == 0)
            format_ = 4;
        else if (format_str.CompareNoCase("systemtime") == 0)
            format_ = 5;
        else if (format_str.CompareNoCase("filetime") == 0)
            format_ = 6;
        else if (format_str.CompareNoCase("msdos") == 0)
            format_ = 7;
        else
            format_ = 0;
        break;
    case 5:     // string
        if (format_str.CompareNoCase("ascii") == 0)
            format_ = 1;
        else if (format_str.CompareNoCase("ansi") == 0)
            format_ = 2;
        else if (format_str.CompareNoCase("oem") == 0)
            format_ = 3;
        else if (format_str.CompareNoCase("ebcdic") == 0)
            format_ = 4;
        else if (format_str.CompareNoCase("unicode") == 0)
            format_ = 5;
        else
            format_ = 0;
        break;
    }

	// Enable and update controls based on current settings
    fix_controls();

    CString byte_order_str = pelt_->GetAttr("byte_order");
    if (byte_order_str.CompareNoCase("default") == 0)
        big_endian_ = 2;
    else if (byte_order_str.CompareNoCase("big") == 0)
        big_endian_ = 1;
    else if (byte_order_str.CompareNoCase("little") == 0)
        big_endian_ = 0;
    else
    {
        ASSERT(0);
        big_endian_ = 2;
    }

    CString read_only_str = pelt_->GetAttr("read_only");
    if (read_only_str.CompareNoCase("default") == 0)
        read_only_ = 2;
    else if (read_only_str.CompareNoCase("true") == 0)
        read_only_ = 1;
    else if (read_only_str.CompareNoCase("false") == 0)
        read_only_ = 0;
    else
    {
        ASSERT(0);
        read_only_ = 2;
    }

    length_ = pelt_->GetAttr("len");
    domain_ = pelt_->GetAttr("domain");
    display_ = pelt_->GetAttr("display");
    if (type_ == 5)
        units_ = type_str.Mid(6);
    else
        units_ = pelt_->GetAttr("units");
    type_name_ = pelt_->GetAttr("type_name");
    comments_ = pelt_->GetAttr("comment");

	// Bit-field fields
	bits_ = atoi(pelt_->GetAttr("bits"));
	bitfield_ = bits_ > 0;
	dirn_ = pelt_->GetAttr("direction").CompareNoCase("down") == 0; // 0 = up, 1 = down

    UpdateData(FALSE);
}

bool CDFFDData::check_data()
{
    // Get values from the controls and validate them
    UpdateData();

//        && AfxMessageBox("You cannot refer to this data element\n"
//                      "in expressions unless you specify a name.\n\n"
//                      "Do you want to continue anyway?",
//                      MB_YESNO | MB_ICONINFORMATION) != IDYES)

    // Make sure name is given unless the container is a FOR (whence sub-elements
    // are accessed via array index) OR the user wants a hdden field.
    if (name_.IsEmpty() &&
        parent_type_ != CHexEditDoc::DF_FORV &&
        parent_type_ != CHexEditDoc::DF_FORF &&
        AfxMessageBox("The name for this DATA element is empty.\r\n"
                      "This means it will not be available for\r\n"
                      "use in expressions or visible in view mode.\r\n\n"
                      "Are you sure you want an unnamed element?",
                      MB_YESNO) != IDYES)
    {
        ctl_name_.SetFocus();
        return false;
    }

    // If there is a name do some checks that it is valid
    if (!name_.IsEmpty())
    {
        if (!valid_id(name_))
        {
            AfxMessageBox("Invalid name.  Please use alphanumeric characters (or\r\n"
                          "underscore) and start with an alphabetic character.");
            ctl_name_.SetFocus();
            return false;
        }

        if (name_.CompareNoCase("true") == 0 ||
		    name_.CompareNoCase("false") == 0 ||
		    name_.CompareNoCase("end") == 0 ||
		    name_.CompareNoCase("index") == 0 ||
		    name_.CompareNoCase("member") == 0 ||
		    expr_eval::func_token(name_) != expr_eval::TOK_NONE)
        {
            AfxMessageBox("This name is reserved");
            ctl_name_.SetFocus();
            return false;
        }

        // Check that the name is not the same as any siblings
        CXmlTree::CElt ee;

        // First find the actual element whose siblings we want to check (an IF takes the name of its child)
        for (ee = *pelt_; ee.GetParent().GetName() == "if" || ee.GetParent().GetName() == "jump"; ee = ee.GetParent())
            ; // empty loop body

        // Check that older siblings don't have the same name
        for (ee--; !ee.IsEmpty(); ee--)
        {
            CXmlTree::CElt ee2;
            for (ee2 = ee; !ee2.IsEmpty() && (ee2.GetName() == "if" || ee2.GetName() == "jump"); ee2 = ee2.GetFirstChild())
                ; // nothing here
            if (!ee2.IsEmpty() && ee2.GetAttr("name") == name_)
            {
                AfxMessageBox("This element has a sibling with the same name");
                ctl_name_.SetFocus();
                return false;
            }
        }

        for (ee = *pelt_; ee.GetParent().GetName() == "if" || ee.GetParent().GetName() == "jump"; ee = ee.GetParent())
            ; // empty loop body

        // Check that younger siblings don't have the same name
        for (++ee; !ee.IsEmpty(); ++ee)
        {
            CXmlTree::CElt ee2;
            for (ee2 = ee; !ee2.IsEmpty() && (ee2.GetName() == "if" || ee2.GetName() == "jump"); ee2 = ee2.GetFirstChild())
                ; // nothing here
            if (!ee2.IsEmpty() && ee2.GetAttr("name") == name_)
            {
                AfxMessageBox("This element has a sibling with the same name");
                ctl_name_.SetFocus();
                return false;
            }
        }
    }

    if (type_ > 0 && format_ == -1)
    {
//        AfxMessageBox("Please select a format from the list");
//        ctl_format_.SetFocus();
//        return false;
        format_ = 0;                    // The first format is always best as the default
    }

    // xxx Make sure domain is empty or a valid boolean expression

    // Make sure length is valid
    // Note for types 1 (char) and 4 (date) type is implicit and is ignored
    length_.TrimLeft();
    if (type_ == 2 && length_.IsEmpty())
    {
        length_ = "4";
    }
    else if (type_ == 3 && length_.IsEmpty())
    {
        length_ = "8";
    }
#if 0  // Allow length to be expressions - xxx we need to check at "run-time" that the lengths are OK
    else if (type_ == 2)
    {
        // Length must be 1,2,4,8 for integers
        int len = atoi(length_);

        if (len != 1 && len != 2 && len != 4 && len != 8)
        {
            AfxMessageBox("Length must be 1, 2, 4, or 8 for integer data types");
            ctl_length_.SetFocus();
            return false;
        }
    }
    else if (type_ == 3)
    {
        // Length must be 4 or 8 for reals
        int len = atoi(length_);

        if (len != 4 && len != 8)
        {
            AfxMessageBox("Length must be 4, or 8 for real data types");
            ctl_length_.SetFocus();
            return false;
        }
    }
#endif
    else if (type_ == 0 || type_ == 5)
    {
        // Length must be empty or a valid integer expression
        // xxx TBD
    }

    // xxx Make sure display is valid: check for % or valid strings???

    return true;
}

void CDFFDData::save_data()
{
    // Save data to XML element node
    pelt_->SetAttr("name", name_);

    switch (type_)
    {
    default:
        ASSERT(0);
        // fall through
    case 0:     // none
        pelt_->SetAttr("type", "none");
        pelt_->RemoveAttr("format");
        break;
    case 1:     // character
        pelt_->SetAttr("type", "char");
        switch (format_)
        {
        default:
			ASSERT(0);
			// fall through
		case 0:
            pelt_->SetAttr("format", "default");
            break;
        case 1:
            pelt_->SetAttr("format", "ascii");
            break;
        case 2:
            pelt_->SetAttr("format", "ansi");
            break;
        case 3:
            pelt_->SetAttr("format", "oem");
            break;
        case 4:
            pelt_->SetAttr("format", "ebcdic");
            break;
        case 5:
            pelt_->SetAttr("format", "unicode");
            break;
        }
        break;
    case 2:     // integer
        pelt_->SetAttr("type", "int");
        switch (format_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            pelt_->SetAttr("format", "signed");
            break;
        case 1:
            pelt_->SetAttr("format", "unsigned");
            break;
        case 2:
            pelt_->SetAttr("format", "magnitude");
            break;
        }
        break;
    case 3:     // real
        pelt_->SetAttr("type", "real");
        switch (format_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            pelt_->SetAttr("format", "ieee");
            break;
        case 1:
            pelt_->SetAttr("format", "ibm");
            break;
        }
        break;
    case 4:     // date
        pelt_->SetAttr("type", "date");
        switch (format_)
        {
        default:
            ASSERT(0);
            // fall through
        case 0:
            pelt_->SetAttr("format", "c");
            break;
        case 1:
            pelt_->SetAttr("format", "c51");
            break;
        case 2:
            pelt_->SetAttr("format", "c7");
            break;
        case 3:
            pelt_->SetAttr("format", "cmin");
            break;
        case 4:
            pelt_->SetAttr("format", "ole");
            break;
        case 5:
            pelt_->SetAttr("format", "systemtime");
            break;
        case 6:
            pelt_->SetAttr("format", "filetime");
            break;
        case 7:
            pelt_->SetAttr("format", "msdos");
            break;
        }
        break;
    case 5:     // string
        int term = atoi(units_);
        if (term == 0)
            pelt_->SetAttr("type", "string");
        else
        {
            CString ss;
            ss.Format("%d", term);
            pelt_->SetAttr("type", CString("string")+ss);
        }
        switch (format_)
        {
        default:
			ASSERT(0);
			// fall through
		case 0:
            pelt_->SetAttr("format", "default");
            break;
        case 1:
            pelt_->SetAttr("format", "ascii");
            break;
        case 2:
            pelt_->SetAttr("format", "ansi");
            break;
        case 3:
            pelt_->SetAttr("format", "oem");
            break;
        case 4:
            pelt_->SetAttr("format", "ebcdic");
            break;
        case 5:
            pelt_->SetAttr("format", "unicode");
            break;
        }
        break;
    }

    switch (big_endian_)
    {
    case 0:     // false
        pelt_->SetAttr("byte_order", "little");
        break;
    case 1:     // true
        pelt_->SetAttr("byte_order", "big");
        break;

    default:    // default for file
        pelt_->RemoveAttr("byte_order");  // if not present defaults to "default"
        break;
    }

    switch (read_only_)
    {
    case 0:     // false
        pelt_->SetAttr("read_only", "false");
        break;
    case 1:     // true
        pelt_->SetAttr("read_only", "true");
        break;

    default:    // default for file
        pelt_->RemoveAttr("read_only");  // if not present defaults to "default"
        break;
    }

    if (type_ == 1 || type_ == 4 || length_.IsEmpty())
        pelt_->RemoveAttr("len");
    else
        pelt_->SetAttr("len", length_);
    if (type_ == 0 || domain_.IsEmpty())
        pelt_->RemoveAttr("domain");
    else
        pelt_->SetAttr("domain", domain_);
    if (type_ == 0)
        pelt_->RemoveAttr("display");
    else
        pelt_->SetAttr("display", display_);
    if (type_ == 5 || units_.IsEmpty())
        pelt_->RemoveAttr("units");
    else
        pelt_->SetAttr("units", units_);
    if (type_name_.IsEmpty())
        pelt_->RemoveAttr("type_name");
    else
        pelt_->SetAttr("type_name", type_name_);
    if (comments_.IsEmpty())
        pelt_->RemoveAttr("comment");
    else
        pelt_->SetAttr("comment", comments_);

	if (type_ == 2 && bitfield_)
	{
		CString ss;
		ss.Format("%d", bits_ < 1 || signed(bits_) >= atoi(length_)*8 ? 1 : bits_);
        pelt_->SetAttr("bits", ss);
		pelt_->SetAttr("direction", dirn_ ? "down" : "up");
	}
	else
	{
        pelt_->RemoveAttr("bits");
        pelt_->RemoveAttr("direction");
	}
}

static DWORD id_pairs[] = { 
    IDC_DFFD_NAME, HIDC_DFFD_NAME,
    IDC_DFFD_TYPE, HIDC_DFFD_TYPE,
    IDC_DFFD_FORMAT, HIDC_DFFD_FORMAT,
    IDC_DFFD_LENGTH, HIDC_DFFD_LENGTH,
    IDC_DFFD_LENGTH_VAR, HIDC_DFFD_LENGTH_VAR,
    IDC_DFFD_DOMAIN, HIDC_DFFD_DOMAIN,
    IDC_DFFD_DOMAIN_VAR, HIDC_DFFD_DOMAIN_VAR,
    IDC_DFFD_DISPLAY, HIDC_DFFD_DISPLAY,
    IDC_DFFD_UNITS, HIDC_DFFD_UNITS,
    IDC_DFFD_READ_ONLY, HIDC_DFFD_READ_ONLY,
    IDC_DFFD_BIG_ENDIAN, HIDC_DFFD_BIG_ENDIAN,
    IDC_DFFD_TYPE_NAME, HIDC_DFFD_TYPE_NAME,
    IDC_DFFD_COMMENTS, HIDC_DFFD_COMMENTS,
    ID_DFFD_CLONE, HID_DFFD_CLONE,
    ID_DFFD_PREV, HID_DFFD_PREV,
    ID_DFFD_NEXT, HID_DFFD_NEXT,
    IDC_DFFD_BITFIELD, HIDC_DFFD_BITFIELD,
    IDC_DFFD_BITFIELD_BITS, HIDC_DFFD_BITFIELD_BITS,
    IDC_DFFD_BITFIELD_BITS_DESC, HIDC_DFFD_BITFIELD_BITS,
    IDC_SPIN_BITFIELD_BITS, HIDC_DFFD_BITFIELD_BITS,
    IDC_DFFD_BITFIELD_DIRN, HIDC_DFFD_BITFIELD_DIRN,
    IDC_DFFD_BITFIELD_DIRN_DESC, HIDC_DFFD_BITFIELD_DIRN,
    0,0 
};

BOOL CDFFDData::OnHelpInfo(HELPINFO* pHelpInfo) 
{

    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
                   HELP_WM_HELP, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void CDFFDData::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

void CDFFDData::OnHelp() 
{
    // Display help for this page
#ifdef USE_HTML_HELP
    if (!theApp.htmlhelp_file_.IsEmpty())
    {
        if (::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_DFFD_DATA))
            return;
    }
#endif
    if (!::WinHelp(AfxGetMainWnd()->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
                   HELP_CONTEXT, HIDD_DFFD_DATA))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

