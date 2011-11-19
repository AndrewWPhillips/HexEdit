// GeneralCRC.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "GeneralCRC.h"

// CGeneralCRC dialog

const char * CGeneralCRC::reg_locn = "Software\\ECSoftware\\HexEdit\\CRCSettings";

IMPLEMENT_DYNAMIC(CGeneralCRC, CDialog)

CGeneralCRC::CGeneralCRC(CWnd* pParent /*=NULL*/)
	: CDialog(CGeneralCRC::IDD, pParent)
{
	memset(&par_, 0, sizeof(par_));
	bits_idx_ = 4;
	par_.bits = 16;
	max_ = 0xFFFF;
	note_ = "Notes:\n"
			"1. The more bits provided the better for detecting errors.\n"
			"2. Always use values from recognised CRC standards;\n"
			"   random parameters will probably not give good results.\n"
			"3. Use optimized CRC commands for better performance.\n"
			"4. The CRC value is stored in the Calculator.\n"
			 ;
}

BOOL CGeneralCRC::OnInitDialog()
{
	CDialog::OnInitDialog();

	set_.push_back("Dummy");   // Reserve index 0 since we use the index as the menu ID and an ID of zero is not allowed

	// Create the popup menu to attach to the "Select" button
	CMenu mm;
	mm.CreatePopupMenu();

	// Add predefined CRC settings - add names to menu and store corresponding parameters strings in set_
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 8");
	set_.push_back("8|07|0|0|0|0|F4");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 10");
	set_.push_back("10|233|0|0|0|0|199");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 12");
	set_.push_back("12|80F|0|0|0|1|DAF");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 16");
	set_.push_back("16|8005|0|0|1|1|BB3D");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 16 USB");
	set_.push_back("16|8005|FFFF|FFFF|1|1|B4C8");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 16 CCITT");
	set_.push_back("16|1021|0|0|1|1|2189");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC CCITT F");
	set_.push_back("16|1021|FFFF|0|0|0|29B1");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC CCITT AUG");
	set_.push_back("16|1021|1D0F|0|0|0|E5CC");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC-A");
	set_.push_back("16|1021|C6C6|0|1|1|BF05");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC X-25");
	set_.push_back("16|1021|FFFF|FFFF|1|1|906E");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC XModem");
	set_.push_back("16|1021|0|0|0|0|31C3");
	//mm.AppendMenu(MF_STRING, set_.size(), "CRC XModem");
	//set_.push_back("16|8408|0|0|1|1");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC 32");
	set_.push_back("32|04C11DB7|FFFFFFFF|FFFFFFFF|1|1|CBF43926");
	mm.AppendMenu(MF_STRING, set_.size(), "CRC MPEG-2");
	set_.push_back("32|04C11DB7|FFFFFFFF|0|0|0|0376E6E7");

	mm.AppendMenu(MF_SEPARATOR);
	last_selected_ = -1;
	last_predefined_ = set_.size();

	// Get any saved settings from the registry
	HKEY hkey;
	if (::RegCreateKey(HKEY_CURRENT_USER, reg_locn, &hkey) == ERROR_SUCCESS)
	{
		LONG err;                        // error return from registry enum function
		char name[80];                   // buffer to hold the name of any registry setting found
		BYTE value[80];                  // buffer to hold the value of any registry setting found
		DWORD name_len = sizeof(name), value_len = sizeof(value);  // corresponding buffer lengths
		DWORD type;                      // the type of registry entry found (we only want strings = REG_SZ)

		for (int ii = 0;
			(err = ::RegEnumValue(hkey, ii, name, &name_len, NULL, &type, value, &value_len)) != ERROR_NO_MORE_ITEMS;
			++ii)
		{
			// If we got a valid string entry
			if (err == ERROR_SUCCESS && type == REG_SZ)
			{
				mm.AppendMenu(MF_STRING, set_.size(), name);
				set_.push_back((char*)value);
			}

			// Reset in/out vars that have been modified
			name_len = sizeof(name);
			value_len = sizeof(value);
		}

		::RegCloseKey(hkey);
	}

	// Attach the menu to the button
	select_menu_.m_hMenu = mm.GetSafeHmenu();
	mm.Detach();

	OnChangeName();

	return TRUE;
}

void CGeneralCRC::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CRC_BITS, ctl_bits_);
	DDX_Control(pDX, IDC_CRC_SELECT, select_menu_);
	DDX_Control(pDX, IDC_CRC_POLY, ctl_poly_);
	DDX_Control(pDX, IDC_CRC_INITREM, ctl_init_rem_);
	DDX_Control(pDX, IDC_CRC_FINALXOR, ctl_final_xor_);
	DDX_Text(pDX, IDC_CRC_NOTE, note_);
	DDX_CBIndex(pDX, IDC_CRC_BITS, bits_idx_);
	DDX_Check(pDX, IDC_CRC_REFLECTREM, par_.reflect_rem);
	DDX_Check(pDX, IDC_CRC_REFLECTIN, par_.reflect_in);

	// We need to handle DDX for the hex values ourselves as there are now hex DDV routines
	if (pDX->m_bSaveAndValidate)
	{
		CString ss;

		ctl_poly_.GetWindowText(ss);
		par_.poly = ::strtoi64(ss, 16);
		if (par_.poly > max_)
		{
			TaskMessageBox("Truncated Polynomial Too Big",
				"The value must fit within the number of bits specified.");
			pDX->Fail();
		}
		ctl_init_rem_.GetWindowText(ss);
		par_.init_rem = ::strtoi64(ss, 16);
		if (par_.init_rem > max_)
		{
			TaskMessageBox("Initial Remainder Too Big",
				"The value must fit within the number of bits specified.");
			pDX->Fail();
		}
		ctl_final_xor_.GetWindowText(ss);
		par_.final_xor = ::strtoi64(ss, 16);
		if (par_.final_xor > max_)
		{
			TaskMessageBox("Final XOR Value Too Big",
				"The value must fit within the number of bits specified.");
			pDX->Fail();
		}
	}
	else
	{
		CString ss;

		ss.Format(theApp.hex_ucase_ ? "%I64X" : "%I64x", __int64(par_.poly));
		ctl_poly_.SetWindowText(ss);
		ctl_poly_.add_spaces();

		ss.Format(theApp.hex_ucase_ ? "%I64X" : "%I64x", __int64(par_.init_rem));
		ctl_init_rem_.SetWindowText(ss);
		ctl_init_rem_.add_spaces();

		ss.Format(theApp.hex_ucase_ ? "%I64X" : "%I64x", __int64(par_.final_xor));
		ctl_final_xor_.SetWindowText(ss);
		ctl_final_xor_.add_spaces();
	}
}

CString CGeneralCRC::SaveParams()
{
	CString ss;
	ss.Format("%d|%I64x|%I64x|%I64x|%d|%d|%I64x",
		par_.bits,
		par_.poly,
		par_.init_rem,
		par_.final_xor,
		par_.reflect_in,
		par_.reflect_rem,
		par_.check );
	return ss;
}

void CGeneralCRC::LoadParams(LPCTSTR params)
{
	::load_crc_params(&par_, params);

	// Work out currently selected item in the Bits list
	// Note this assumes that the drop list has entries of: 4,8,10,12,16,32
	switch (par_.bits)
	{
	case 4:
		bits_idx_ = 0;
		break;

	case 8:
		bits_idx_ = 1;
		break;

	case 10:
		bits_idx_ = 2;
		break;

	case 12:
		bits_idx_ = 3;
		break;

	default:
		ASSERT(0);
		par_.bits = 16;
		// fall through
	case 16:
		bits_idx_ = 4;
		break;

	case 32:
		bits_idx_ = 5;
		break;
	}

	// Work out the maximum values for poly etc
	max_ = (1Ui64 << par_.bits) - 1;

	// Ensure poly etc fit within the specified bits
	par_.poly &= max_;
	par_.final_xor &= max_;
	par_.init_rem &= max_;
}

// Search the "Select" menu to see if a CRC setting name is in use.
// Returns the ID (index into set_) if found or -1 if not
int CGeneralCRC::find_name(LPCTSTR name)
{
	int retval = -1;

	CMenu menu;
	menu.Attach(select_menu_.m_hMenu);
	int lst = menu.GetMenuItemCount();
	for (int ii = 0; ii < lst; ++ii)
	{
		CString ss;
		menu.GetMenuString(ii, ss, MF_BYPOSITION);
		if (ss.CompareNoCase(name) == 0)
		{
			retval = menu.GetMenuItemID(ii);
			break;
		}
	}

	menu.Detach();
	return retval;
}

BEGIN_MESSAGE_MAP(CGeneralCRC, CDialog)
	ON_BN_CLICKED(IDC_CRC_SELECT, OnSelect)
	ON_BN_CLICKED(IDC_CRC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_CRC_DELETE, OnDelete)
	ON_CBN_SELCHANGE(IDC_CRC_BITS, &CGeneralCRC::OnCbnSelchangeCrcBits)
	ON_EN_CHANGE(IDC_CRC_NAME, OnChangeName)
	ON_EN_CHANGE(IDC_CRC_POLY, OnChange)
	ON_EN_CHANGE(IDC_CRC_INITREM, OnChange)
	ON_EN_CHANGE(IDC_CRC_FINALXOR, OnChange)
	ON_BN_CLICKED(IDC_CRC_REFLECTIN, OnChange)
	ON_BN_CLICKED(IDC_CRC_REFLECTREM, OnChange)
END_MESSAGE_MAP()

// CGeneralCRC message handlers
void CGeneralCRC::OnChangeName()
{
	CString strName;
	GetDlgItemText(IDC_CRC_NAME, strName);
	GetDlgItem(IDC_CRC_SAVE)->EnableWindow(!strName.IsEmpty() && find_name(strName) == -1);

	// Disable deletion since name changed (unless name change was caused by OnSelect)
	if (last_selected_ != find_name(strName))
		last_selected_ = -1;
	GetDlgItem(IDC_CRC_DELETE)->EnableWindow(last_selected_ >= last_predefined_);
}

void CGeneralCRC::OnCbnSelchangeCrcBits()
{
	UpdateData();

	// Work out max value for poly etc from the number of bits
	CString ss;
	ctl_bits_.GetLBText(bits_idx_, ss);
	par_.bits = atoi(ss);
	max_ = (1Ui64 << par_.bits) - 1;

	// Also clear name since one of the params has changed
	OnChange();
}

void CGeneralCRC::OnChange()
{
	if (last_selected_ > -1)
	{
		// Clear the name since the parameters no longer match any selected name
		SetDlgItemText(IDC_CRC_NAME, "");

		// We can only delete CRC settings after selected and no changes made
		GetDlgItem(IDC_CRC_DELETE)->EnableWindow(FALSE);

		last_selected_ = -1;
	}
}

// Selection was made from the Select menu button of a predefined CRC settings
void CGeneralCRC::OnSelect()
{
	int id = select_menu_.m_nMenuResult;  // slected menu ID (= index into set_)
	if (id > 0 && id < set_.size())
	{
		// Get the param string associated with the select menu item and load it into member vars
		LoadParams(set_[id]);

		// Update dlg controls from members variables
		UpdateData(FALSE);

		// Get the menu item text and load it into the name field
		CMenu menu;
		menu.Attach(select_menu_.m_hMenu);
		CString ss = get_menu_text(&menu, id);
		menu.Detach();
		SetDlgItemText(IDC_CRC_NAME, ss);

		// Enable the Delete button if this is a user defined setting
		GetDlgItem(IDC_CRC_DELETE)->EnableWindow(id >= last_predefined_);
		last_selected_ = id;
	}
}

void CGeneralCRC::OnSave()
{
	if (!UpdateData())
		return;

	// Get the name to save the settings under and check its valid
	CString strName;
	GetDlgItemText(IDC_CRC_NAME, strName);
	if (strName.IsEmpty())
	{
		TaskMessageBox("Invalid Name", "The CRC name cannot be empty");
		return;
	}
	else if (find_name(strName) != -1)
	{
		CString ss;
		ss.Format("The name \"%s\" is already being used.  "
			        "Please choose another name.", strName);
		TaskMessageBox("Name in Use", ss);
		return;
	}

	// Get the current settings to be associated with the name
	CString params = SaveParams();

	// Add the name to the end of the Select menu
	CMenu menu;
	menu.Attach(select_menu_.m_hMenu);
	menu.AppendMenu(MF_STRING, set_.size(), strName);
	menu.Detach();

	// Add corresponding parameters to the end of set_
	set_.push_back(params);

	// Add to the registry (for next time the dialog is opened)
	HKEY hkey;
	if (::RegCreateKey(HKEY_CURRENT_USER, reg_locn, &hkey) == ERROR_SUCCESS)
	{
		::RegSetValueEx(hkey, strName, 0, REG_SZ, (BYTE *)params.GetBuffer(), params.GetLength());
		::RegCloseKey(hkey);
	}

	// Disable Save button as we can't save again with the same name
	GetDlgItem(IDC_CRC_SAVE)->EnableWindow(FALSE);
}

void CGeneralCRC::OnDelete()
{
	// First check that we selected from the list (using Select menu)
	// else we don't know what to delete
	if (last_selected_ < last_predefined_ || last_selected_ >= set_.size())
	{
		assert(0);   // Code should prevent this from happening
		return;
	}

	// Delete from the Select menu
	CMenu menu;
	menu.Attach(select_menu_.m_hMenu);
	CString strName = get_menu_text(&menu, last_selected_);  // save mneu item name - used to delete reg value by name

	CString mess;
	mess.Format("Are you sure you want to delete the settings for %s", strName);
	if (TaskMessageBox("Delete CRC Settings", mess, MB_YESNO) != IDYES)
	{
		menu.Detach();
		return;
	}
	menu.DeleteMenu(last_selected_, MF_BYCOMMAND);  // Delete menu item with selected ID
	menu.Detach();

	// Delete from set_ (where ID == last_selected_ is the index into the array)
	set_[last_selected_] = CString("");  // clear but don't delete unused entries (else menu item ID's won't mathc array indices)

	// Remove the setting from the registry
	HKEY hkey;
	if (::RegCreateKey(HKEY_CURRENT_USER, reg_locn, &hkey) == ERROR_SUCCESS)
	{
		::RegDeleteValue(hkey, strName);
		::RegCloseKey(hkey);
	}

	// Disable Delete button now as we just deleted it
	GetDlgItem(IDC_CRC_DELETE)->EnableWindow(FALSE);
}
