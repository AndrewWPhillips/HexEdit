// OpenDiskDlg.cpp : implements dialog for opening disks, volumes etc
//
// Copyright (c) 2005 by Andrew W. Phillips.
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
#include <winioctl.h>
#include <Dbt.h>        // for DBT_DEVICEREMOVECOMPLETE etc
#include "hexedit.h"
#include "OpenDiskDlg.h"
#include "ntapi.h"

#include "resource.hm"    // help IDs
#include "HelpID.hm"            // User defined help IDs
#include ".\opendiskdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenDiskDlg dialog


COpenDiskDlg::COpenDiskDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenDiskDlg::IDD, pParent)
	, m_readonly(FALSE)
{
	//{{AFX_DATA_INIT(COpenDiskDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_selected = -1;
	m_readonly = TRUE;   // default to read-only for safety
	m_first_physical_device = 0; // just in case
	m_do_rebuild = false;
	m_next_time = true;

	GetNTAPIFuncs();
}


void COpenDiskDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COpenDiskDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_OPEN_TREE, m_ctrl_tree);
	DDX_Control(pDX, IDC_OPEN_INFO, m_ctl_info);
	DDX_Control(pDX, IDC_PROP_DESC, m_ctl_desc);
	DDX_Check  (pDX, IDC_OPEN_READONLY, m_readonly);
	DDX_Control(pDX, IDC_OPEN_READONLY, m_ctl_readonly);
	DDX_Control(pDX, IDOK, m_ctl_ok);
}


BEGIN_MESSAGE_MAP(COpenDiskDlg, CDialog)
	//{{AFX_MSG_MAP(COpenDiskDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_OPEN_TREE, OnSelchangedOpenTree)
	ON_NOTIFY(NM_DBLCLK, IDC_OPEN_TREE, OnDblclkOpenTree)
	//}}AFX_MSG_MAP
	ON_NOTIFY(TVN_SELCHANGING, IDC_OPEN_TREE, OnSelchangingOpenTree)
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_DEVICECHANGE()
	ON_WM_CLOSE()
    ON_WM_TIMER()
	ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_ALGORITHM_HELP, OnBnClickedAlgorithmHelp)
END_MESSAGE_MAP()

void COpenDiskDlg::Update(int ii)
{
    CString strInfo;                    // Where we build info about the device to display

	CWaitCursor wait;

	m_ctl_readonly.EnableWindow(FALSE);
	m_ctl_ok.EnableWindow(FALSE);

	ASSERT(ii >= 0 && ii < (int)m_device_name.size());

	// First display name of device in header (static text control)
	strInfo.Format("%s  Properties:", ::DeviceName(m_device_name[ii]));
	m_ctl_desc.SetWindowText(strInfo);

	// Now build up info about the device to display in the multi-line text box
	strInfo.Empty();
	if (ii < m_first_physical_device)
    {
        // Volume (ie, partition/device with filesystem)
        bool is_removeable = false;
        bool is_cdrom = false;
		ASSERT(::IsDevice(m_device_name[ii]) && m_device_name[ii].GetLength() == 6);
	    _TCHAR vol[4] = _T("?:\\");         // Vol name used for volume enquiry calls
		vol[0] = m_device_name[ii][4];

		// Find out some more information about the volume to show to the user
		CString ss;
		switch (::GetDriveType(vol))
		{
		default:
			ASSERT(0);
            ss = "";
			break;
		case DRIVE_REMOVABLE:
            is_removeable = true;
			if (vol[0] == 'A' || vol[0] == 'B')
				ss = "Floppy Drive";
			else
				ss = "Removable";
			break;
		case DRIVE_FIXED:
			ss = "Fixed Drive";
			break;
		case DRIVE_REMOTE:
			ss = "Network Drive";
			break;
		case DRIVE_CDROM:
            is_removeable = true;
            is_cdrom = true;
			ss = "Optical Drive";
			break;
		case DRIVE_RAMDISK:
			ss = "RAM Drive";
			break;
		}
		strInfo = "Type: " + ss + "\r\n";;

		// Volume name, serial number and filesystem name
		TCHAR vol_name[128], fs_name[32];
		DWORD vol_serno, max_component, flags;
		if (::GetVolumeInformation(vol,
			                       vol_name, sizeof(vol_name)/sizeof(*vol_name),
			                       &vol_serno,
			                       &max_component,
			                       &flags,
			                       fs_name, sizeof(fs_name)/sizeof(*fs_name) ) )
		{
			if (vol_name[0] != '\0')
			{
				ss.Format(_T("Volume: %s\r\n"), vol_name);
				strInfo += ss;
			}
			ss.Format(_T("File system: %s\r\n"), fs_name);
			strInfo += ss;

		    // Disk space total and free
		    ULARGE_INTEGER bytes_free, bytes_total, dummy;  // Need to pass dummy (not NULL as per doc) otherwise we crash under NT4
		    if (::GetDiskFreeSpaceEx(vol, &dummy, &bytes_total, &bytes_free))
		    {
			    ss.Format(_T("Used space: %d%%\r\n"),
				          int(((bytes_total.QuadPart - bytes_free.QuadPart)*100)/bytes_total.QuadPart));
			    strInfo += ss;
                if (bytes_free.QuadPart > 0)
                {
			        ss.Format(_T("Free space: %sb\r\n"), NumScale((double)(_int64)bytes_free.QuadPart)); 
			        strInfo += ss;
                }
			    ss.Format(_T("Capacity: %sb\r\n"), NumScale((double)(_int64)bytes_total.QuadPart)); 
			    strInfo += ss;
		    }

		    // Get sector size etc
		    DWORD SectorsPerCluster;
		    DWORD BytesPerSector;
		    DWORD NumberOfFreeClusters;
		    DWORD TotalNumberOfClusters;

		    if (::GetDiskFreeSpace(vol,
					    &SectorsPerCluster,
					    &BytesPerSector,
					    &NumberOfFreeClusters,
					    &TotalNumberOfClusters) )
		    {
			    ss.Format(_T("Sector size: %ld bytes\r\n"), (long)BytesPerSector);
			    strInfo += ss;
			    ss.Format(_T("Sectors/cluster: %ld\r\n"), (long)SectorsPerCluster);
			    strInfo += ss;
		    }

			if (theApp.is_nt_)
			{
				// Now use IOCTL to see if disk is writeable
				HANDLE hDevice = ::CreateFile(m_device_name[ii],
								0,              // query only
								FILE_SHARE_READ | FILE_SHARE_WRITE, 
								NULL,           // default security attributes
								OPEN_EXISTING,  // disposition
								0,              // file attributes
								NULL);          // do not copy file attributes

				ASSERT(hDevice != INVALID_HANDLE_VALUE);

				// Check if write-protected
				DWORD got;                      // Length of buffer returned
				if (!is_cdrom &&
					::DeviceIoControl(hDevice,  // device to be queried
									  IOCTL_DISK_IS_WRITABLE,
									  NULL, 0,  // no input buffer
									  NULL, 0,  // no output buffer
									  &got,     // how much we got
									  (LPOVERLAPPED) NULL))
				{
					m_ctl_readonly.EnableWindow();
				}
				else
				{
					// 
					m_ctl_readonly.EnableWindow();
					m_ctl_readonly.SetCheck(1);
					m_ctl_readonly.EnableWindow(FALSE);
					m_readonly = TRUE;
				}
				CloseHandle(hDevice);
			}
			// xxx else
			m_ctl_ok.EnableWindow();
		}
        else if (::GetLastError() == ERROR_NOT_READY && is_removeable)
            strInfo += _T("Status: no disk present\r\n");
        else
		{
            // xxx seems to happen with bad floppy, sometime when no CD in drive, ...?
			CString ss;
			ss.Format("GetVolumeInformation error: %lx", ::GetLastError());
		}
    }
	else
	{
		ASSERT(theApp.is_nt_);
		// Notes:
		// 1. Problems were experienced with NtDeviceIoControlFile return values (esp. with floppy/CD) until we changed it
        //    to use synchronous calls (FILE_SYNCHRONOUS_IO_ALERT) or wait for an event
		// 2. IOCTL_DISK_GET_DRIVE_GEOMETRY seems to work fine for all drive types under XP but for W2K it fails for CD
		//    so use IOCTL_CDROM_GET_DRIVE_GEOMETRY instead
		// 3. IOCTL_DISK_GET_DRIVE_GEOMETRY (NtDeviceIoControlFile) always returns error 259 (ERROR_NO_MORE_ITEMS)??? BUT
		//    DISK_GEOMETRY fields are filled out OK - this is not an error apparently but warning/info message
		// 4. IOCTL_STORAGE_QUERY_PROPERTY fails out for floppies
        // 5. IOCTL_DISK_GET_DRIVE_GEOMETRY fails if there is no media present
		// 6. (*pfQueryInformationFile)(...., FileStandardInformation) does not work at all for devices
		CString ss;

		HANDLE handle = 0;
		OBJECT_ATTRIBUTES oa;
		IO_STATUS_BLOCK iosb;
		UNICODE_STRING us;
		BSTR name = ::GetPhysicalDeviceName(m_device_name[ii]);
		(*pfInitUnicodeString)(&us, name);
		::SysFreeString(name);

		oa.Length = sizeof(oa);
		oa.RootDirectory = NULL;
		oa.ObjectName = &us;
		oa.Attributes = OBJ_CASE_INSENSITIVE;
		oa.SecurityDescriptor = NULL;
		oa.SecurityQualityOfService = NULL;

		if ((*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE, &oa, &iosb,
			              FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT) != STATUS_SUCCESS)
		{
			// Sometimes fails first time but then works
			VERIFY((*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE, &oa, &iosb, 
				FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT) == STATUS_SUCCESS);
		}

		ASSERT(handle != 0);
		FILE_FS_DEVICE_INFORMATION ffdi;
		VERIFY((*pfQueryVolumeInformationFile)(handle, &iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation) == STATUS_SUCCESS);

		bool media_present = false;

		DISK_GEOMETRY dg;             // results from IOCTL_DISK_GET_DRIVE_GEOMETRY
		if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_DISK_GET_DRIVE_GEOMETRY, 0, 0, &dg, sizeof(dg)) == STATUS_SUCCESS)
        {
			ss.Format(_T("Sector size: %ld bytes\r\n"), (long)dg.BytesPerSector);
			strInfo += ss;
			ss.Format(_T("Sectors/track: %ld\r\n"), (long)dg.SectorsPerTrack);
			strInfo += ss;
			ss.Format(_T("Tracks/cylinder: %ld\r\n"), (long)dg.TracksPerCylinder);
			strInfo += ss;
			ss.Format(_T("Cylinders: %g\r\n"), (double)dg.Cylinders.QuadPart);
			strInfo += ss;

			LONGLONG total_size = dg.Cylinders.QuadPart * dg.TracksPerCylinder *
									(LONGLONG)dg.SectorsPerTrack * dg.BytesPerSector;
			ss.Format(_T("Capacity: %sb\r\n"), NumScale((double)total_size)); 
			strInfo += ss;

			m_ctl_ok.EnableWindow();  // Allow it to be opened
			media_present = true;
        }
		else if ((ffdi.Characteristics & FILE_FLOPPY_DISKETTE) == 0)
		{
			// IOCTL_DISK_GET_DRIVE_GEOMETRY out under W2K (only under VMware?) for
			// CD drive even when media is present - so use IOCTL_STORAGE_CHECK_VERIFY.
		    if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_STORAGE_CHECK_VERIFY, 0, 0, 0, 0) == STATUS_SUCCESS)
			{
				m_ctl_ok.EnableWindow();  // Allow it to be opened
                media_present = true;
			}
		}

        STORAGE_PROPERTY_QUERY Query;
		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;
		char buf[2048];
		if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
			                         &Query, sizeof(Query), &buf, sizeof(buf)) == STATUS_SUCCESS)
		{
			STORAGE_DEVICE_DESCRIPTOR *psdd = (STORAGE_DEVICE_DESCRIPTOR *)buf;

			ss = CString(buf + psdd->VendorIdOffset);
			ss.TrimLeft();
			if (psdd->VendorIdOffset > 0 && !ss.IsEmpty())
				strInfo += "Vendor: " + ss + "\r\n";
			ss = CString(buf + psdd->ProductIdOffset);
			ss.TrimLeft();
			if (psdd->ProductIdOffset > 0 && !ss.IsEmpty())
				strInfo += "Product: " + ss + "\r\n";
			ss = CString(buf + psdd->ProductRevisionOffset);
			ss.TrimLeft();
			if (psdd->ProductRevisionOffset > 0 && !ss.IsEmpty())
				strInfo += "Revision: " + ss + "\r\n";
			ss = CString(buf + psdd->SerialNumberOffset);
			ss.TrimLeft();
			if (psdd->SerialNumberOffset > 0 && !ss.IsEmpty())
				strInfo += "Serial: " + ss + "\r\n";
		}

		if (!media_present && (ffdi.Characteristics & FILE_REMOVABLE_MEDIA) != 0) // xxx check status?
        {
	        strInfo += _T("Status: no disk/medium present\r\n");
			m_ctl_readonly.EnableWindow(FALSE);
			m_ctl_ok.EnableWindow(FALSE);
        }
		else if ((ffdi.Characteristics & FILE_DEVICE_IS_MOUNTED) == 0)
			strInfo += CString("Device is not mounted\r\n");

        // Check and disable read-only checkbox if device cannot be written to
	    m_readonly = TRUE;
		if (ffdi.DeviceType != FILE_DEVICE_CD_ROM && (ffdi.Characteristics & FILE_READ_ONLY_DEVICE) == 0)
		{
			// If not a read only device check if the write-protect status is set
		    if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_DISK_IS_WRITABLE, 0, 0, 0, 0) == STATUS_SUCCESS)
                m_readonly = FALSE;             // writeable!
        }

		(*pfClose)(handle);

		if (m_readonly)
		{
			m_ctl_readonly.EnableWindow();
			m_ctl_readonly.SetCheck(1);
			m_ctl_readonly.EnableWindow(FALSE);
		}
		else
		{
			m_ctl_readonly.EnableWindow();
			m_ctl_readonly.SetCheck(0);
		}
	}
    m_ctl_info.SetWindowText(strInfo);
}

// Builds tree of devices and selects the current device (if given).
// Returns the index (into m_device_name) of the device selected.
int COpenDiskDlg::BuildTree(LPCTSTR current_device /*=NULL*/)
{
	int retval = 0;
	m_ctrl_tree.DeleteAllItems();
	m_device_name.clear();

	// Add root level nodes
	HTREEITEM hVol  = m_ctrl_tree.InsertItem("Logical Volumes", IMG_DRIVES, IMG_DRIVES);
	m_ctrl_tree.SetItemData(hVol, (LPARAM)-1);
	HTREEITEM hPhys = 0;
	if (theApp.is_nt_)
	{
		// Only allow access to physical devices under NT/2K/XP
		hPhys = m_ctrl_tree.InsertItem("Physical Drives", IMG_PHYS_DRIVES, IMG_PHYS_DRIVES);
		m_ctrl_tree.SetItemData(hPhys, (LPARAM)-1);
	}

	// Add all logical drives to the tree
	DWORD dbits = GetLogicalDrives();   // Bit is on for every drive present
	_TCHAR vol[4] = _T("?:\\");         // Vol name used for volume enquiry calls
	CString idstr;                      // Name of device displayed in the tree
	for (int ii = 0; ii < 26; ++ii)
	{
		if ((dbits & (1<<ii)) != 0)
		{
			// Drive is present so create volume name and display string
			vol[0] = 'A' + (char)ii;
			idstr.Format(" (%c:)", 'A' + (char)ii);

			// Find out what sort of drive it is
			CString ss;
            int img_volume = IMG_DRIVE; // Image list entry to use for volume
			switch (::GetDriveType(vol))
			{
			default:
				ASSERT(0);
				continue;
			case DRIVE_REMOVABLE:
				if (ii == 0 || ii == 1)
				{
					ss = "Floppy Drive";
					img_volume = IMG_FLOPPY;
				}
				else
				{
					ss = "Removable";
					img_volume = IMG_REMOVE;
				}
				break;
			case DRIVE_FIXED:
				ss = "Fixed Drive";
				break;
			case DRIVE_REMOTE:
				ss = "Network Drive";
                img_volume = IMG_NET;
				continue;  // Don't show remote drives for now
			case DRIVE_CDROM:
				ss = "Optical Drive";
                img_volume = IMG_OPT_DRIVE;
				break;
			case DRIVE_RAMDISK:
				ss = "RAM Drive";
				break;
			}
			// Volume name, serial number and filesystem name
			TCHAR vol_name[128], fs_name[32];
			DWORD vol_serno, max_component, flags;
			if (::GetVolumeInformation(vol,
									vol_name, sizeof(vol_name)/sizeof(*vol_name),
									&vol_serno,
									&max_component,
									&flags,
									fs_name, sizeof(fs_name)/sizeof(*fs_name) ) )
			{
				if (vol_name[0] != '\0')
					ss = vol_name;
			}
			idstr = ss + idstr;

			// Add the volume to the tree under "Logical Volumes"
			HTREEITEM hh = m_ctrl_tree.InsertItem(idstr, img_volume, img_volume, hVol);

			// Store vector index as item data so we can relate the tree selection to the device
			m_ctrl_tree.SetItemData(hh, (LPARAM)m_device_name.size());

			// Work out the actual device name passed to ::CreateFile()
			m_device_name.push_back(CString());
			m_device_name.back().Format(_T("\\\\.\\%c:"), 'A' + (char)ii);

			if (current_device != NULL && m_device_name.back() == current_device)
			{
				m_ctrl_tree.SelectItem(hh);
				retval = m_device_name.size() - 1;
			}
		}
	}

	m_first_physical_device = m_device_name.size();

    if (hNTDLL != (HINSTANCE)0)
	{
		ASSERT(hPhys != 0);
		enum { FLOPPY, HARD, CDROM, LAST_TYPE };   // Types of physical devices
		// Note: For \device\HarddiskN entries the sub-entry "Partition0" is not really a partition but
		//       allows access to the whole disk.  (This is only a real device under NT, but is a valid
		//       alias under XP to something like \device\HarddiskN\DRM (where N != M, although N == M
		//       for fixed devices and M > N for removeable (M increases every time media is swapped).
		wchar_t *dev_name[LAST_TYPE] = { L"\\device\\Floppy%d", L"\\device\\Harddisk%d\\Partition0", L"\\device\\Cdrom%d", };
		// These are to create "fake" names although they look like valid device names (the Floppy ones
		// don't work and the CdRom ones only work under XP - PhysicalDriveN names seem OK on NT/2K though).
		char  * save_name[LAST_TYPE] = { "\\\\.\\Floppy%d",     "\\\\.\\PhysicalDrive%d",            "\\\\.\\CdRom%d", };

		long status;
		wchar_t name[128];
		HANDLE handle = 0;
		IO_STATUS_BLOCK iosb;
		OBJECT_ATTRIBUTES oa;
		UNICODE_STRING us;

		for (int dd = FLOPPY; dd < LAST_TYPE; ++dd)
		{
			for (int nn = 0; nn < 100; ++nn)
			{
				swprintf(name, dev_name[dd], nn);
				(*pfInitUnicodeString)(&us, name);

				oa.Length = sizeof(oa);
				oa.RootDirectory = NULL;
				oa.ObjectName = &us;
				oa.Attributes = OBJ_CASE_INSENSITIVE;
				oa.SecurityDescriptor = NULL;
				oa.SecurityQualityOfService = NULL;

				status = (*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE, &oa, &iosb,
					                   FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT);
				if (status != STATUS_SUCCESS)
					break;

				FILE_FS_DEVICE_INFORMATION ffdi;
                status = (*pfQueryVolumeInformationFile)(handle, &iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation);
				ASSERT(status == STATUS_SUCCESS);
				ASSERT(dd != FLOPPY || ffdi.DeviceType == FILE_DEVICE_DISK);
				ASSERT(dd != HARD   || ffdi.DeviceType == FILE_DEVICE_DISK);
				ASSERT(dd != CDROM  || ffdi.DeviceType == FILE_DEVICE_CD_ROM);

				// We may later also need to get FileAlignmentInformation and FileFsSizeInformation?

				int image;
                if (dd == FLOPPY)
                    image = IMG_PHYS_FLOPPY;
                else if (dd == CDROM)
                    image = IMG_PHYS_CD;
				else if ((ffdi.Characteristics & FILE_REMOVABLE_MEDIA) != 0)
					image = IMG_PHYS_REMOVE;
                else
				    image = IMG_PHYS_DRIVE;

                CString nice_name;
                if (dd != FLOPPY)
                {
                    // Get properties of drive (vendor model etc)
                    STORAGE_PROPERTY_QUERY Query;
			        Query.PropertyId = StorageDeviceProperty;
			        Query.QueryType = PropertyStandardQuery;
			        char buf[2048];
					memset(buf, '\0', sizeof(buf));
			        status = (*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
						                              &Query, sizeof(Query), &buf, sizeof(buf));
			        if (status == STATUS_SUCCESS)
					{
						STORAGE_DEVICE_DESCRIPTOR *psdd = (STORAGE_DEVICE_DESCRIPTOR *)buf;

						if (psdd->VendorIdOffset > 0)
							nice_name += CString(buf + psdd->VendorIdOffset);
						if (psdd->ProductIdOffset > 0)
							nice_name += " " + CString(buf + psdd->ProductIdOffset);
						nice_name.TrimLeft();
					}

                    if (nice_name.IsEmpty() && dd == CDROM)
                        nice_name = "Optical Drive %d";
                    else if (nice_name.IsEmpty())
                        nice_name = "Hard Drive %d";
                }
                else
                    nice_name = "Floppy Drive %d";
				idstr.Format(nice_name, nn);

				// Save the device name which is later passed to CFileNC::Open()
				//CString tmp;
				//tmp.Format(save_name[dd], nn)
				//m_device_name.push_back(tmp);
				m_device_name.push_back(CString());
				m_device_name.back().Format(save_name[dd], nn);


				// Add associated mounted volume name (for CD and removeable drives) to help user know which drive it is
				if (image == IMG_PHYS_CD || image == IMG_PHYS_REMOVE)
				{
					int volid = DeviceVolume(m_device_name.back());
					if (volid != -1)
					{
						char vol[8] = " (?:)";
						vol[2] = 'A' + (char)volid;
						idstr += vol;
					}
				}

				HTREEITEM hh = m_ctrl_tree.InsertItem(idstr, image, image, hPhys);

				// Store vector index as item data so we can relate the tree selection to the device
				m_ctrl_tree.SetItemData(hh, (LPARAM)m_device_name.size() - 1);

				(*pfClose)(handle);
			}
		}
	}

	m_ctrl_tree.Expand(hVol, TVE_EXPAND);
	if (hPhys != 0)
		m_ctrl_tree.Expand(hPhys, TVE_EXPAND);
	return retval;
}

/////////////////////////////////////////////////////////////////////////////
// COpenDiskDlg message handlers

BOOL COpenDiskDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWindowText("Open Disk");

    // Make sure when dialog is resized that the controls move to sensible places
	resizer_.Create(this);
	resizer_.SetMinimumTrackingSize();
	resizer_.SetGripEnabled(TRUE);
    resizer_.Add(IDC_OPEN_TREE,     0,   0, 100, 100);
    resizer_.Add(IDC_OPEN_READONLY,100, 100,   0,   0);
    resizer_.Add(IDC_PROP_DESC,   100,   0,   0,   0);
    resizer_.Add(IDC_OPEN_INFO,   100,   0,   0, 100);
    resizer_.Add(IDOK,            100, 100,   0,   0);
    resizer_.Add(IDCANCEL,        100, 100,   0,   0);

	m_ctrl_tree.ModifyStyle(0, TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);

    m_imagelist.Create(MAKEINTRESOURCE(IDB_DEVIMAGES), 16, 1, RGB(0xFF, 0xFF, 0xFF));
    m_ctrl_tree.SetImageList(&m_imagelist, TVSIL_NORMAL);

	Update(BuildTree());
    SetTimer(1, 10000, NULL);

	return TRUE;
}

void COpenDiskDlg::OnClose()
{
	CDialog::OnClose();
}

void COpenDiskDlg::OnOK()
{
	HTREEITEM hh = m_ctrl_tree.GetSelectedItem();
	if (hh == 0)
		return;                         // Don't exit dlg if nothing selected

    // Get tree item selected
	m_selected = m_ctrl_tree.GetItemData(hh);
    if (m_selected == -1)
        return;                         // Don't exit dlg if leaf not selected

	CDialog::OnOK();
}

static DWORD id_pairs[] = { 
    IDC_OPEN_TREE, HIDC_OPEN_TREE,
    IDC_OPEN_READONLY, HIDC_OPEN_READONLY,
    IDC_PROP_DESC, HIDC_OPEN_INFO,
    IDC_OPEN_INFO, HIDC_OPEN_INFO,
    0,0
};

BOOL COpenDiskDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
                   HELP_WM_HELP, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    return TRUE;
}

void COpenDiskDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ASSERT(theApp.m_pszHelpFilePath != NULL);

    CWaitCursor wait;

    if (!::WinHelp((HWND)pWnd->GetSafeHwnd(), theApp.m_pszHelpFilePath, 
                   HELP_CONTEXTMENU, (DWORD) (LPSTR) id_pairs))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}

LRESULT COpenDiskDlg::OnKickIdle(WPARAM, LPARAM lCount)
{
    return FALSE;
}

void COpenDiskDlg::OnTimer(UINT nIDEvent)
{
    ASSERT(nIDEvent == 1);
	if (m_do_rebuild && m_next_time)
	{
		// This avoids doing rebuilds on 2 consecutive timer events - this happens often when USB device
		// is removed as we seem to get several DEVNODES_CHANGED events over several seconds.
		m_next_time = false;
	}
	else if (m_do_rebuild)
	{
		int ii;
		HTREEITEM hh = m_ctrl_tree.GetSelectedItem();
		if (hh != NULL && (ii = (int)m_ctrl_tree.GetItemData(hh)) != -1)
			Update(BuildTree(CString(m_device_name[ii])));            // Rescan and reselect current device (if still there)
		else
			Update(BuildTree());
		m_do_rebuild = false;
		m_next_time = true;
	}
	else
    {
	    int ii;
	    HTREEITEM hh = m_ctrl_tree.GetSelectedItem();
	    if (hh != NULL &&
            (ii = (int)m_ctrl_tree.GetItemData(hh)) != -1)
        {
            if (ii < m_first_physical_device)
            {
				ASSERT(::IsDevice(m_device_name[ii]) && m_device_name[ii].GetLength() == 6);
	            _TCHAR vol[4] = _T("?:\\");         // Vol name used for volume enquiry calls
		        vol[0] = m_device_name[ii][4];

		        if (::GetDriveType(vol) == DRIVE_REMOVABLE)
			        Update(ii);
            }
            else
            {
		        if (m_device_name[ii].Left(10) == _T("\\\\.\\Floppy"))
			        Update(ii);
#if 0
		        long status;
		        HANDLE handle = 0;
		        IO_STATUS_BLOCK iosb;
		        OBJECT_ATTRIBUTES oa;
		        UNICODE_STRING us;
			    BSTR name = m_device_name[ii].AllocSysString();
			    (*pfInitUnicodeString)(&us, name);
			    ::SysFreeString(name);

				oa.Length = sizeof(oa);
				oa.RootDirectory = NULL;
				oa.ObjectName = &us;
				oa.Attributes = OBJ_CASE_INSENSITIVE;
				oa.SecurityDescriptor = NULL;
				oa.SecurityQualityOfService = NULL;

				status = (*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES, &oa, &iosb,
					                   FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
				if (status != STATUS_SUCCESS)
					return;

				FILE_FS_DEVICE_INFORMATION ffdi;
                status = (*pfQueryVolumeInformationFile)(handle, &iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation);
				if ((ffdi.Characteristics & FILE_FLOPPY_DISKETTE) != 0 /*|| (ffdi.Characteristics & FILE_REMOVABLE_MEDIA) != 0*/)
                {
			        Update(ii);
                }
			    (*pfClose)(handle);
#endif
            }
        }
    }
}

BOOL COpenDiskDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
    if (nEventType == DBT_DEVICEARRIVAL || nEventType == DBT_DEVICEREMOVECOMPLETE)
	{
		// Media has been removed/inserted so update info in case it was the device we are looking at
		int ii;
		HTREEITEM hh = m_ctrl_tree.GetSelectedItem();
		if (hh != NULL && (ii = (int)m_ctrl_tree.GetItemData(hh)) != -1)
			Update(ii);
	}
    else if (nEventType == DBT_DEVNODES_CHANGED || nEventType == DBT_CONFIGCHANGED)
	{
		// A volume has been added or removed so we need to completely rebuild the list of devices
		// Note: We can get several of these messages related to the one "event" so we just make a note
		// of this and then rebuild the whole tree later (in OnTimer).
		m_do_rebuild = true;
	}

    return TRUE;
}

void COpenDiskDlg::OnSelchangedOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	int ii = (int)m_ctrl_tree.GetItemData(pNMTreeView->itemNew.hItem);
	if (ii == -1)
	{
		m_ctl_info.SetWindowText(_T(""));
		m_ctl_desc.SetWindowText(_T(""));
	}
	else
        Update(ii);

	*pResult = 0;
}

void COpenDiskDlg::OnSelchangingOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	m_ctl_info.SetWindowText(_T(""));
	m_ctl_desc.SetWindowText(_T(""));

	*pResult = 0;
}

void COpenDiskDlg::OnDblclkOpenTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Check if there is a valid selection and close the dialog
	OnOK();

	*pResult = 0;
}

void COpenDiskDlg::OnBnClickedAlgorithmHelp()
{
    if (!::HtmlHelp(AfxGetMainWnd()->m_hWnd, theApp.htmlhelp_file_, HH_HELP_CONTEXT, HIDD_OPEN_SPECIAL_HELP))
        ::HMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
}
