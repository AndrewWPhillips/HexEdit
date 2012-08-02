// CSpecialList.cpp : CSpecialList class that maintains list of
//                   special things that can be opened (eg disks)
//
// Copyright (c) 2005-2012 by Andrew W. Phillips.
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
#include <vector>

#include "hexedit.h"
#include "misc.h"
#include "ntapi.h"
#include "SpecialList.h"

static UINT bg_func(LPVOID pParam)
{
	return ((CSpecialList *)pParam)->background();
}

CSpecialList::CSpecialList(int sleep /* = 5 */) : m_hwnd(0), m_pthread(NULL)
{
	GetNTAPIFuncs();   // Make sure we have function addresses from ntdll.dll (if NT/2K/XP)

	m_refresh_id = -1;  // Force complete rebuild ...
	m_sleep = sleep;    // wait on starting background scan thread
	build();
}

BOOL CSpecialList::busy()
{
	BOOL ok = m_mutex.Lock(0);
	if (ok)
		m_mutex.Unlock();
	return !ok;
}

#ifdef _DEBUG
void CSpecialList::DEBUG_CHECK(int idx)
{
	ASSERT(m_id.size() == m_type.size());
	ASSERT(m_filename.size() == m_type.size());
	ASSERT(m_name.size() == m_type.size());
	ASSERT(m_nicename.size() == m_type.size());
	ASSERT(m_error.size() == m_type.size());
	ASSERT(m_medium_present.size() == m_type.size());
	ASSERT(m_read_only.size() == m_type.size());
	ASSERT(m_removeable.size() == m_type.size());
	ASSERT(m_sector_size.size() == m_type.size());
	ASSERT(m_total_size.size() == m_type.size());
	ASSERT(m_info1.size() == m_type.size());
	ASSERT(m_info2.size() == m_type.size());
	ASSERT(m_info3.size() == m_type.size());
	ASSERT(m_size1.size() == m_type.size());
	ASSERT(m_size2.size() == m_type.size());
	ASSERT(m_size3.size() == m_type.size());
	ASSERT(idx < m_type.size());
}
#else
#define DEBUG_CHECK(x) (void)0
#endif

int CSpecialList::find(LPCTSTR filename)
{
	DEBUG_CHECK(0);
	for (int ii = 0; ii < (int)m_filename.size(); ++ii)
		if (m_filename[ii] == filename)
			return ii;          // found
	return -1;                  // not found
}

int CSpecialList::find(short id)
{
	DEBUG_CHECK(0);
	for (int ii = 0; ii < (int)m_id.size(); ++ii)
		if (m_id[ii] == id)
			return ii;          // found
	return -1;                  // not found
}

CString CSpecialList::nicename(int idx)
{
	DEBUG_CHECK(idx);

	CSingleLock sl(&m_mutex);
	// Note the || operands must be in this order so that we do not try to access m_nicename
	// while m_mutex is locked (since CString is not thread safe, nor vector completely).
	if (sl.IsLocked() || m_nicename[idx].IsEmpty())
		return m_name[idx];   // return ordinary name if we don't know the nice name yet
	else
		return m_nicename[idx];
}

DWORD CSpecialList::error(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_error[idx];
}

bool CSpecialList::medium_present(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_medium_present[idx];
}

bool CSpecialList::read_only(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_read_only[idx];
}

bool CSpecialList::removeable(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_removeable[idx];
}

int CSpecialList::sector_size(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_sector_size[idx];
}

__int64 CSpecialList::total_size(int idx)
{
	DEBUG_CHECK(idx);
	CSingleLock sl(&m_mutex, TRUE);
	return m_total_size[idx];
}

CString CSpecialList::DriveType(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
//	CSingleLock sl(&m_mutex, TRUE);
	return m_info1[idx];
}

CString CSpecialList::FileSystem(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
	CSingleLock sl(&m_mutex, TRUE);
	return m_info2[idx];
}

CString CSpecialList::VolumeName(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
	CSingleLock sl(&m_mutex, TRUE);
	return m_info3[idx];
}

int CSpecialList::SectorsPerCluster(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size1[idx];
}

int CSpecialList::FreeClusters(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size2[idx];
}

int CSpecialList::TotalClusters(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 0);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size3[idx];
}

CString CSpecialList::Vendor(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_info1[idx];
}

CString CSpecialList::Product(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_info2[idx];
}

CString CSpecialList::Revision(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_info3[idx];
}

int CSpecialList::SectorsPerTrack(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size1[idx];
}

int CSpecialList::TracksPerCylinder(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size2[idx];
}

int CSpecialList::Cylinders(int idx)
{
	DEBUG_CHECK(idx);
	ASSERT(m_type[idx] == 1);
	CSingleLock sl(&m_mutex, TRUE);
	return m_size3[idx];
}

void CSpecialList::refresh(HWND hwnd, short id /*= -1*/)
{
	m_hwnd = hwnd;
	m_refresh_id = id;
	m_sleep = 0;

	if (id == -1)
	{
		m_type.clear();
		m_id.clear();
		m_filename.clear();
		m_name.clear();

		m_nicename.clear();
		m_error.clear();
		m_medium_present.clear();
		m_read_only.clear();
		m_removeable.clear();
		m_sector_size.clear();
		m_total_size.clear();

		m_info1.clear();
		m_info2.clear();
		m_info3.clear();
		m_size1.clear();
		m_size2.clear();
		m_size3.clear();

		build();
	}
	else
	{
		// Start background process to update the entry
		ASSERT(m_pthread == NULL);
		m_pthread = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_NORMAL);
		ASSERT(m_pthread != NULL);
	}
}

void CSpecialList::build()
{
	// First add volumes to the list
	DWORD dbits = GetLogicalDrives();   // Bit is on for every drive present
	char vol[4] = "?:\\";               // Vol name used for volume enquiry calls
	for (int ii = 0; ii < 26; ++ii)
	{
		if ((dbits & (1<<ii)) != 0)
		{
			// Drive is present so create volume name and display string
			vol[0] = 'A' + (char)ii;

			// Find out what sort of drive it is
			CString ss;
			switch (::GetDriveType(vol))
			{
			default:
				ASSERT(0);
				continue;
			case DRIVE_REMOVABLE:
				if (ii == 0 || ii == 1)
					ss = "Floppy";
				else
					ss = "Removable";
				break;
			case DRIVE_FIXED:
				ss = "Fixed";
				break;
			case DRIVE_REMOTE:
				ss = "Network";
				continue;               // Don't add network drives
			case DRIVE_CDROM:
				ss = "Optical";
				break;
			case DRIVE_RAMDISK:
				ss = "RAM";
				break;
			}

			m_type.push_back(0);
			m_id.push_back(ii);
			m_filename.push_back("");
			m_filename.back().Format("\\\\.\\%c:", 'A' + (char)ii);
			m_name.push_back("");
			m_name.back().Format("%s Drive (%c:)", (const char *)ss, 'A' + (char)ii);

			// Add entries for other stuff that will be added by the background thread
			m_nicename.push_back("");
			m_removeable.push_back(false);   // default to fairly arbitrary value
			m_read_only.push_back(true);
			m_error.push_back(0);
			m_medium_present.push_back(false);
			m_sector_size.push_back(0);
			m_total_size.push_back(0);

			m_info1.push_back(ss);
			m_info2.push_back("");
			m_info3.push_back("");
			m_size1.push_back(0);
			m_size2.push_back(0);
			m_size3.push_back(0);
		}
	}

	// Add physical device names to the list (if NT)
	if (hNTDLL != (HINSTANCE)0)
	{
		enum { FLOPPY, HARD, CDROM, LAST_TYPE };   // Types of physical devices
		// These are to create "fake" names although they look like valid device names (the Floppy ones
		// don't work and the CdRom ones only work under XP - PhysicalDriveN names seem OK on NT/2K though).
		char  * save_name[LAST_TYPE] = { "\\\\.\\Floppy%d",     "\\\\.\\PhysicalDrive%d",            "\\\\.\\CdRom%d", };

		long status;
		HANDLE handle = 0;
		IO_STATUS_BLOCK iosb;
		OBJECT_ATTRIBUTES oa;
		UNICODE_STRING us;

		for (int dd = FLOPPY; dd < LAST_TYPE; ++dd)
		{
			for (int nn = 0; nn < 100; ++nn)
			{
				CString filename;
				filename.Format(save_name[dd], nn);
				BSTR name = GetPhysicalDeviceName(filename);
				(*pfInitUnicodeString)(&us, name);
				::SysFreeString(name);

				oa.Length = sizeof(oa);
				oa.RootDirectory = NULL;
				oa.ObjectName = &us;
				oa.Attributes = OBJ_CASE_INSENSITIVE;
				oa.SecurityDescriptor = NULL;
				oa.SecurityQualityOfService = NULL;

				// Try to open just to see if it is there
				status = (*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE, &oa, &iosb,
									   FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT);
				if (status != STATUS_SUCCESS)
					break;

				(*pfClose)(handle);

				m_type.push_back(1);
				m_id.push_back(((dd+1)<<8) + nn);
				m_filename.push_back(filename);
				if (filename.Left(17) == "\\\\.\\PhysicalDrive")
					m_name.push_back("Physical Drive " + filename.Mid(17));
				else if (filename.Left(9) == "\\\\.\\CdRom")
					m_name.push_back("Optical Drive " + filename.Mid(9));
				else if (filename.Left(10) == "\\\\.\\Floppy")
					m_name.push_back("Floppy Drive " + filename.Mid(10));
				else
					m_name.push_back("");

				m_nicename.push_back("");
				m_removeable.push_back(false);   // default to fairly arbitrary value
				m_read_only.push_back(true);
				m_error.push_back(0);
				m_medium_present.push_back(false);
				m_sector_size.push_back(0);
				m_total_size.push_back(0);

				m_info1.push_back("");
				m_info2.push_back("");
				m_info3.push_back("");
				m_size1.push_back(0);
				m_size2.push_back(0);
				m_size3.push_back(0);
			}
		}
	}

	// Start background process to fill in details
	ASSERT(m_pthread == NULL);
	m_pthread = AfxBeginThread(&bg_func, this, THREAD_PRIORITY_NORMAL);
	ASSERT(m_pthread != NULL);
}

#ifndef FILE_READ_ONLY_VOLUME
#define FILE_READ_ONLY_VOLUME           0x00080000
#endif

void CSpecialList::bg_update(int ii)
{
	m_nicename[ii] = m_name[ii];  // default this if case nothing better can be used

	if (m_type[ii] == 0)
	{
		if (m_info1[ii] == "Removable" || m_info1[ii] == "Floppy" || m_info1[ii] == "Optical")
			m_removeable[ii] = true;

		char vol[4] = "?:\\";
		vol[0] = *(((const char *)m_filename[ii]) + 4);

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
			m_medium_present[ii] = true;
			m_read_only[ii] = false;    // Assume writeable unless we can show otherwise
			if (m_info1[ii] == "Optical")
				m_read_only[ii] = true;

			// xxx TBD TODO check this works for write-prot floppy
			if (theApp.is_xp_ && (flags & FILE_READ_ONLY_VOLUME) != 0)
				m_read_only[ii] = true;
			// We should do more when not under XP - eg. floppy/zip may be write-protected

			m_info2[ii] = fs_name;
			m_info3[ii] = vol_name;

			if (!m_info3[ii].IsEmpty())
				m_nicename[ii] = m_info3[ii] +
								 " (" + 
								 CString(((const char *)m_filename[ii]) + 4) +
								 ")";


			DWORD SectorsPerCluster;
			DWORD BytesPerSector;
			DWORD NumberOfFreeClusters;
			DWORD TotalNumberOfClusters;

			VERIFY(::GetDiskFreeSpace(vol,
									  &SectorsPerCluster,
									  &BytesPerSector,
									  &NumberOfFreeClusters,
									  &TotalNumberOfClusters) );
			m_sector_size[ii] = BytesPerSector;
			m_total_size[ii] = (__int64)BytesPerSector * SectorsPerCluster * (__int64)TotalNumberOfClusters;
			m_size1[ii] = SectorsPerCluster;
			m_size2[ii] = NumberOfFreeClusters;
			m_size3[ii] = TotalNumberOfClusters;

			ULARGE_INTEGER bytes_free, bytes_total, dummy;
			// NOTE: NT 4 crashes if we pass NULL instead of &dummy
			if (::GetDiskFreeSpaceEx(vol, &dummy, &bytes_total, &bytes_free))
			{
				if (bytes_total.QuadPart > (ULONGLONG)m_total_size[ii])
					m_total_size[ii] = bytes_total.QuadPart;
			}
		}
		else if (::GetLastError() == ERROR_NOT_READY)
		{
			m_medium_present[ii] = false;
		}
		else
		{
			m_error[ii] = ::GetLastError();
		}
	}
	else if (m_type[ii] == 1)
	{
		ASSERT(theApp.is_nt_);

		// Physical device
		long status;
		HANDLE handle = 0;
		OBJECT_ATTRIBUTES oa;
		IO_STATUS_BLOCK iosb;
		UNICODE_STRING us;
		BSTR name = GetPhysicalDeviceName(m_filename[ii]);
		(*pfInitUnicodeString)(&us, name);
		::SysFreeString(name);

		oa.Length = sizeof(oa);
		oa.RootDirectory = NULL;
		oa.ObjectName = &us;
		oa.Attributes = OBJ_CASE_INSENSITIVE;
		oa.SecurityDescriptor = NULL;
		oa.SecurityQualityOfService = NULL;

		status = (*pfOpenFile)(&handle, FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE, &oa, &iosb,
							   FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT);
		if (status != STATUS_SUCCESS)
		{
			m_error[ii] = status;
		}
		else if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_STORAGE_CHECK_VERIFY, 0, 0, 0, 0) == STATUS_SUCCESS)
		{
			m_medium_present[ii] = true;

			FILE_FS_DEVICE_INFORMATION ffdi;
			VERIFY((*pfQueryVolumeInformationFile)(handle, &iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation) == STATUS_SUCCESS);

			if ((ffdi.Characteristics & FILE_REMOVABLE_MEDIA) != 0 ||
				(ffdi.Characteristics & FILE_FLOPPY_DISKETTE) != 0 ||
				ffdi.DeviceType == FILE_DEVICE_CD_ROM)
			{
				m_removeable[ii] = true;
			}

			// If not a read only device check if the write-protect status is set
			m_read_only[ii] = true;
			if (ffdi.DeviceType != FILE_DEVICE_CD_ROM &&
				(ffdi.Characteristics & FILE_READ_ONLY_DEVICE) == 0 &&
				(*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_DISK_IS_WRITABLE, 0, 0, 0, 0) == STATUS_SUCCESS)
			{
				m_read_only[ii] = false;
			}

			DISK_GEOMETRY dg;             // results from IOCTL_DISK_GET_DRIVE_GEOMETRY
			dg.BytesPerSector = 0;
			// Note: IOCTL_DISK_GET_DRIVE_GEOMETRY does not always work for CD use IOCTL_CDROM_GET_DRIVE_GEOMETRY
			ULONG cmd = (ffdi.DeviceType != FILE_DEVICE_CD_ROM) ? IOCTL_DISK_GET_DRIVE_GEOMETRY : IOCTL_CDROM_GET_DRIVE_GEOMETRY;
			if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, cmd, 0, 0, &dg, sizeof(dg)) == STATUS_SUCCESS)
			{
				m_sector_size[ii] = dg.BytesPerSector;
				m_total_size[ii] = dg.Cylinders.QuadPart *
								   (__int64)dg.TracksPerCylinder *
								   (__int64)dg.SectorsPerTrack *
								   (__int64)dg.BytesPerSector;

				m_size1[ii] = dg.SectorsPerTrack;
				m_size2[ii] = dg.TracksPerCylinder;
				m_size3[ii] = (int)dg.Cylinders.QuadPart;
			}
			else
			{
				// Floppy raw device (eg, \\.\Floppy0) may pass IOCTL_STORAGE_CHECK_VERIFY with no disk in drive but then IOCTL_DISK_GET_DRIVE_GEOMETRY fails
				m_medium_present[ii] = false;
				m_removeable[ii] = true;
			}
		}
		else
		{
			m_medium_present[ii] = false;
			m_removeable[ii] = true;        // must be removeable if no medium present
		}

		STORAGE_PROPERTY_QUERY Query;
		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;
		char buf[2048];
		if ((*pfDeviceIoControlFile)(handle, 0, 0, 0, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
									 &Query, sizeof(Query), &buf, sizeof(buf)) == STATUS_SUCCESS)
		{
			STORAGE_DEVICE_DESCRIPTOR *psdd = (STORAGE_DEVICE_DESCRIPTOR *)buf;
			CString ss;

			if (psdd->VendorIdOffset > 0)
				m_info1[ii] = CString(buf + psdd->VendorIdOffset);
			m_info1[ii].TrimLeft();

			if (psdd->ProductIdOffset > 0)
				m_info2[ii] = CString(buf + psdd->ProductIdOffset);
			m_info2[ii].TrimLeft();

			if (psdd->ProductRevisionOffset > 0)
				m_info3[ii] = CString(buf + psdd->ProductRevisionOffset);
			m_info3[ii].TrimLeft();

			if (psdd->SerialNumberOffset > 0)
				 CString(buf + psdd->SerialNumberOffset);
			// TBD: TODO save serial no?

			if (!m_info1[ii].IsEmpty() || !m_info2[ii].IsEmpty())
				m_nicename[ii] = m_info1[ii] + " " + m_info2[ii];
		}

		if (m_removeable[ii] && _tcsncmp(m_filename[ii], _T("\\\\.\\Floppy"), 10) != 0)  // removeable drives (except floppies)
		{
			// Removable drives usually have just one volume - add drive letter to name to assist user
			int volid = DeviceVolume(m_filename[ii]);
			if (volid != -1)
			{
				char vol[8] = " (?:)";
				vol[2] = 'A' + (char)volid;
				m_nicename[ii] += vol;
			}
		}

		(*pfClose)(handle);
	}
	else
		ASSERT(0);
}

// This member function is called in the background thread
UINT CSpecialList::background()
{
	//TRACE("--- START THREAD\r\n");
	if (m_sleep > 0) ::Sleep(m_sleep*1000);
	CSingleLock sl(&m_mutex, TRUE);

	VERIFY(sl.IsLocked());

	if (m_refresh_id == -1)
	{
		// Update all entries
		for (int ii = 0; ii < (int)m_type.size(); ++ii)
			bg_update(ii);

		if (::IsWindow(m_hwnd))
			::PostMessage(m_hwnd, WM_USER + 1, 0, 0);
	}
	else
	{
		int ii;
		for (ii = 0; ii < (int)m_id.size(); ++ii)
			if (m_id[ii] == m_refresh_id)
			{
				// Default values that may be different from last time
				m_medium_present[ii] = false;  // not really nec. as this will always be set in bg_update
				m_read_only[ii] = true;        // this may change - eg floppy write-protect tab changed
				m_sector_size[ii] = 0;
				m_total_size[ii] = 0;

				// Don't wipe out m_info1 (DiskType/Vendor)
				if (m_type[ii] == 0)
				{
					m_info2[ii] = "";  // don't wipe out model
					m_info3[ii] = "";  // don't wipe out revision
				}
				m_size1[ii] = 0;
				m_size2[ii] = 0;
				m_size3[ii] = 0;
				bg_update(ii);
				break;
			}

		ASSERT(ii < (int)m_type.size());     // we should have found it somewhere in the list
		if (ii < (int)m_type.size() && ::IsWindow(m_hwnd))
			::PostMessage(m_hwnd, WM_USER + 2, 0, 0);
	}

	//TRACE("--- END THREAD %p\r\n", m_pthread);
	m_pthread = NULL;
	sl.Unlock();

	return 0;
}

