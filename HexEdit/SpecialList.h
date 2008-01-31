// SpecialList.h : Maintains list of special things that can be opened (eg disks)
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

/////////////////////////////////////////////////////////////////////////////
#ifndef SPECIALLIST_INCLUDED_
#define SPECIALLIST_INCLUDED_  1

#include <afxmt.h>          // For MFC IPC (CMutex)
#include <vector>

// Notes:
// 1. The c'tor (and Refresh) fills in th basic list then starts background thread
//    to fill in the details, since some things (like checking for floppy disk in
//    drive) may take a while.
// 2. Currently 2 types of "things" are supported (as returned by type())
//    0 = volume with a "file" name of something like "\\.\D:"
//    1 = physical device with name something like "\\.\PhysicalDrive0" or "\\.\CdRom1"
// 3. Most of the public members are used to obtain info about one item in the list by
//    passing a single list index.  The number of list entries is given by size().
// 4. A unique id (as returned by id()) is useful when the list changes (eg drives added/removed).
// 5. The "file name" is another unique identifier and is what is stored in the recent file list.

class CSpecialList
{
public:
	CSpecialList();                      // Constructor

	int size() { return m_type.size(); }
	int find(short id);              // find entry given unique id (or -1 if not found)
	int find(LPCTSTR filename);      // find item entry from "file name"
	void refresh(HWND, short id = -1);  // refresh complete list or one entry

	// Information on particular entries
    int type(int idx) {return m_type[idx];} // 0 = volume, 1 = phys device
    short id(int idx)  {return m_id[idx];} // uniquely id: 0=\\.\A:, 100=\\.\Floppy0, 200=\\.\PhysicalDrive0, 300=\\.\CdRom0, etc
	CString filename(int idx) {return m_filename[idx];} // eg \\.\Floppy1
	CString name(int idx) {return m_name[idx];} // eg Floppy Drive 1

	CString nicename(int idx);           // Volume name, Vendor/Model etc (or name() if no other info. avail.)
    DWORD error(int idx);            // if medium present but can't be read this is the error code
	bool medium_present(int);        // false if removeable media device and no medium present
	bool read_only(int);             // true if device is read-only or medium is write-protected
	bool removeable(int);            // device is removeable physical drive (zip, flash disk etc)
	int sector_size(int);            // Size of a sector on the device (512 for disk, may be more for CD)
	__int64 total_size(int);         // Total numbers of bytes for the device (may not be exact)

	// The following only apply for volumes (type() returns 0)
	CString DriveType(int);
	CString FileSystem(int);
    CString VolumeName(int);
	int SectorsPerCluster(int);
    int FreeClusters(int);
    int TotalClusters(int);

	// The following only apply for phys devices (type() returns 1)
    CString Vendor(int);
    CString Product(int);
    CString Revision(int);
    int SectorsPerTrack(int);
    int TracksPerCylinder(int);
    int Cylinders(int);

    BOOL busy();                        // Is the background thread still working?
    // Note: background() is public only because it's called from a non-member function (thread start up function)
    UINT background();                  // called in background thread to fill in the details

private:
    CWinThread *m_pthread;              // Background thread that fills in the details
	HWND m_hwnd;                        // Where message is sent when background thread is finished
    short m_refresh_id;                 // Unique id of entry to update or -1 for all
    int m_sleep;

	// m_mutex protects mosts of the data members from being accessed from the main thread while they are
	// being changed in the background thread.  Note that this does not include m_type, m_id, m_filename
	// and m_name as they are not modified in the background thread.
	CMutex m_mutex;

	void build();
#ifdef _DEBUG
    void DEBUG_CHECK(int idx);
#endif
    void bg_update(int ii);             // Update one entry (part of background thread)

	std::vector<char> m_type;           // 0 = logical volume, 1 = physical device, 
	std::vector<short> m_id;
	std::vector<CString> m_filename;
	std::vector<CString> m_name;

	std::vector<CString> m_nicename;
	std::vector<bool> m_removeable;
	std::vector<bool> m_read_only;
	std::vector<DWORD> m_error;
	std::vector<bool> m_medium_present;
	std::vector<int> m_sector_size;
	std::vector<__int64> m_total_size;

	std::vector<CString> m_info1;       // drive type OR product vendor
	std::vector<CString> m_info2;       // filesystem OR product model
	std::vector<CString> m_info3;       // volume name OR product revision
	std::vector<int> m_size1;           // sectors/cluster OR sector/track
	std::vector<int> m_size2;           // free clusters OR tracks/cyl
	std::vector<int> m_size3;           // total clusters OR total cylinders
};

#endif
