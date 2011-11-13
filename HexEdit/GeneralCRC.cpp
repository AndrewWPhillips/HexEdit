// GeneralCRC.cpp : implementation file
//

#include "stdafx.h"
#include "hexedit.h"
#include "GeneralCRC.h"

// CGeneralCRC dialog

IMPLEMENT_DYNAMIC(CGeneralCRC, CDialog)

CGeneralCRC::CGeneralCRC(CWnd* pParent /*=NULL*/)
	: CDialog(CGeneralCRC::IDD, pParent)
{
	memset(&par_, 0, sizeof(par_));
	bits_idx_ = 2;
	par_.bits = 16;
	max_ = 0xFFFF;
}

BOOL CGeneralCRC::OnInitDialog()
{
	CDialog::OnInitDialog();

	predefined_.push_back("Dummy");   // Just in case a menu id of zero is not allowed

	// Create the popup menu to attach to the "Select" button
	CMenu mm;
	mm.CreatePopupMenu();

	// Build menu entries (CRC names) and corresponding parameters strings
	mm.AppendMenu(MF_STRING, predefined_.size(), "CRC 8");
	predefined_.push_back("8|07|0|0|0|0");
	mm.AppendMenu(MF_STRING, predefined_.size(), "CRC 16");
	predefined_.push_back("16|8005|0|0|1|1");
	mm.AppendMenu(MF_STRING, predefined_.size(), "CRC 16 CCITT");
	predefined_.push_back("16|1021|FFFF|0|0|0");
	//mm.AppendMenu(MF_STRING, predefined_.size(), "CRC XModem");
	//predefined_.push_back("16|8408|0|0|1|1");
	mm.AppendMenu(MF_STRING, predefined_.size(), "CRC 32");
	predefined_.push_back("32|04C11DB7|FFFFFFFF|FFFFFFFF|1|1");

	// Attach the menu to the button
	select_menu_.m_hMenu = mm.GetSafeHmenu();
	mm.Detach();

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

	DDX_CBIndex(pDX, IDC_CRC_BITS, bits_idx_);
	DDX_Check(pDX, IDC_CRC_REFLECTREM, par_.reflect_rem);
	DDX_Check(pDX, IDC_CRC_REFLECTIN, par_.reflect_in);

//	DDX_Text(pDX, IDC_CRC_POLY, par_.poly);
//	DDV_MinMaxULongLong(pDX, par_.poly, 0, max_);
//	DDX_Text(pDX, IDC_CRC_INITREM, par_.init_rem);
//	DDV_MinMaxULongLong(pDX, par_.init_rem, 0, max_);
//	DDX_Text(pDX, IDC_CRC_FINALXOR, par_.final_xor);
//	DDV_MinMaxULongLong(pDX, par_.final_xor, 0, max_);
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
	ss.Format("%d|%I64x|%I64x|%I64x|%d|%d",
		par_.bits,
		par_.poly,
		par_.init_rem,
		par_.final_xor,
		par_.reflect_in,
		par_.reflect_rem );
	return ss;
}

void CGeneralCRC::LoadParams(LPCTSTR params)
{
	::load_crc_params(&par_, params);

	// Work out currently selected item in the Bits list
	// Note this assumes that the drop list has entries of: 4,8,16,32
	switch (par_.bits)
	{
	case 4:
		bits_idx_ = 0;
		break;

	case 8:
		bits_idx_ = 1;
		break;

	default:
		par_.bits = 16;
		// fall through
	case 16:
		bits_idx_ = 2;
		break;

	case 32:
		bits_idx_ = 3;
		break;
	}

	// Work out the maximum values for poly etc
	max_ = (1Ui64 << par_.bits) - 1;

	// Ensure poly etc fit within the specified bits
	par_.poly &= max_;
	par_.final_xor &= max_;
	par_.init_rem &= max_;
}

BEGIN_MESSAGE_MAP(CGeneralCRC, CDialog)
	ON_CBN_SELCHANGE(IDC_CRC_BITS, &CGeneralCRC::OnCbnSelchangeCrcBits)
	ON_BN_CLICKED(IDC_CRC_SELECT, OnSelect)
END_MESSAGE_MAP()

// CGeneralCRC message handlers
void CGeneralCRC::OnCbnSelchangeCrcBits()
{
	UpdateData();

	CString ss;
	ctl_bits_.GetLBText(bits_idx_, ss);
	par_.bits = atoi(ss);
	max_ = (1Ui64 << par_.bits) - 1;
}

void CGeneralCRC::OnSelect()
{
	if (select_menu_.m_nMenuResult > 0 && select_menu_.m_nMenuResult < predefined_.size())
	{
		// Get the param string associated with the select menu item and load it into member vars
		LoadParams(predefined_[select_menu_.m_nMenuResult]);

		// Update dlg controls from members variables
		UpdateData(FALSE);
	}
}
