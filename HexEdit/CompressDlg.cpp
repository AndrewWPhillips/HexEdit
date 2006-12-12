// CompressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "HexEdit.h"
#include "CompressDlg.h"
#include "zlib/zlib.h"    // For zlib version number


// CCompressDlg dialog

IMPLEMENT_DYNAMIC(CCompressDlg, CDialog)
CCompressDlg::CCompressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCompressDlg::IDD, pParent)
{
}

CCompressDlg::~CCompressDlg()
{
}

void CCompressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_COMPRESSION_LEVEL_DEFAULT, m_defaultLevel);
	DDX_Control(pDX, IDC_COMPRESSION_LEVEL, m_ctlLevel);
	DDX_Control(pDX, IDC_COMPRESSION_LEVEL_SPIN, m_ctlLevelSpin);
	DDX_Text(pDX, IDC_COMPRESSION_LEVEL, m_level);
	DDV_MinMaxUInt(pDX, m_level, 1, 9);
	DDX_Check(pDX, IDC_COMPRESSION_WINDOW_SIZE_DEFAULT, m_defaultWindow);
	DDX_Control(pDX, IDC_COMPRESSION_WINDOW_SIZE, m_ctlWindow);
	DDX_Control(pDX, IDC_COMPRESSION_WINDOW_SIZE_SPIN, m_ctlWindowSpin);
	DDX_Text(pDX, IDC_COMPRESSION_WINDOW_SIZE, m_window);
	DDV_MinMaxUInt(pDX, m_window, 8, 15);
	DDX_Check(pDX, IDC_COMPRESSION_MEMORY_USAGE_DEFAULT, m_defaultMemory);
	DDX_Control(pDX, IDC_COMPRESSION_MEMORY_USAGE, m_ctlMemory);
	DDX_Control(pDX, IDC_COMPRESSION_MEMORY_USAGE_SPIN, m_ctlMemorySpin);
	DDX_Text(pDX, IDC_COMPRESSION_MEMORY_USAGE, m_memory);
	DDV_MinMaxUInt(pDX, m_memory, 1, 9);
	DDX_Text(pDX, IDC_COMPRESSION_SYNC, m_sync);
	DDV_MinMaxUInt(pDX, m_memory, 0, 9999999);
	DDX_Radio(pDX, IDC_COMPRESSION_HEADER_RAW, m_headerType);
	DDX_Radio(pDX, IDC_COMPRESSION_STRATEGY_DEFAULT, m_strategy);
}


BEGIN_MESSAGE_MAP(CCompressDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_COMPRESSION_LEVEL_DEFAULT, OnBnClickedCompressionLevelDefault)
	ON_BN_CLICKED(IDC_COMPRESSION_WINDOW_SIZE_DEFAULT, OnBnClickedCompressionWindowSizeDefault)
	ON_BN_CLICKED(IDC_COMPRESSION_MEMORY_USAGE_DEFAULT, OnBnClickedCompressionMemoryUsageDefault)
END_MESSAGE_MAP()

BOOL CCompressDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

	SetWindowText(CString("ZLIB (version ") + ZLIB_VERSION + CString(") Settings"));

	FixControls();

    return TRUE;
}

void CCompressDlg::FixControls()
{
    m_ctlLevel.EnableWindow(!m_defaultLevel);
    m_ctlLevelSpin.EnableWindow(!m_defaultLevel);
    m_ctlLevelSpin.SetRange(1, 9);
    m_ctlWindow.EnableWindow(!m_defaultWindow);
    m_ctlWindowSpin.EnableWindow(!m_defaultWindow);
    m_ctlWindowSpin.SetRange(8, 15);
    m_ctlMemory.EnableWindow(!m_defaultMemory);
    m_ctlMemorySpin.EnableWindow(!m_defaultMemory);
    m_ctlMemorySpin.SetRange(1, 9);
}

// CCompressDlg message handlers

void CCompressDlg::OnBnClickedOk()
{
	OnOK();
}

void CCompressDlg::OnBnClickedCompressionLevelDefault()
{
	UpdateData();
	FixControls();
}

void CCompressDlg::OnBnClickedCompressionWindowSizeDefault()
{
	UpdateData();
	FixControls();
}

void CCompressDlg::OnBnClickedCompressionMemoryUsageDefault()
{
	UpdateData();
	FixControls();
}
