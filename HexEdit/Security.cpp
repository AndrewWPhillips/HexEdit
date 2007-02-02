// security.cpp : some protection routines
//
// Copyright (c) 2003 by Andrew W. Phillips.

#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>                   // For _stat()
#include <sys/utime.h>                  // For _utime()
#include <io.h>                         // For _access()
#include <imagehlp.h>       // For ::MakeSureDirectoryPathExists()

#include "HexEdit.h"
#include "HexEditView.h"
#include "Misc.h"
#include "Register.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// We use this encryption key for encrypting stuff in registry
#define DUMMY_KEY     "\x69\xB3\x80\x00\x7B\x38\x89\xCD"
#define STANDARD_KEY  "\xE9\x33\x00\x80\xFB\xB8\x09\x5D"
#define FILE_KEY      "A3ntfE4s"
// #define USERLIST_KEY  "Thankyou"

// Registry keys (1 added to each character to foil searches of EXE)
#define REG_BMP       "/cnq]TifmmOfx"   // .bmp\\ShellNew
#define REG_NULLFILE  "OvmmGjmf"        // NullFile
#define REG_DATA      "Ebub"            // Data

// Other things we don't want anyone to see
#define BAD_USER      "DSBDLFE"         // User name "CRACKED"
#define SEC_FILENAME  "Ifyfeju/dge"     // File name "Hexedit.cfd"
#define SEC_FILENAME2 "Ifyfeju/joe"     // File name "Hexedit.ind"
#define SEC_FILENAME3 "%Oug.|@~/MPH"    // File name "$Ntf-{?}.LOG"

#define EXPIRY_DAYS   30

static void sub1(char *ss);
static unsigned long disk_serial();
static unsigned long os_date();

// This is the "mystery" info and first run date of HexEdit (put in registry the first time it's run)
#pragma pack(1)
static struct
{
    short version;                      // HexEdit version that created this info
    unsigned short rand;                // Random value
    time_t date;                        // Date HexEdit first run
} myst_info;
#pragma pack()

#pragma pack(1)
static struct
{
    unsigned short crc;                 // CRC for rest of the structure
    char type;                          // 2 = unregistered, 3 = temp reg, 4,5 = old, 6 = user, 7 = fake reg
    char fake_count;                    // Count for type == 7 before we stop running

    unsigned long serial;               // Volume serial number from first fixed disk
    unsigned long os_install_date;      // Date Windows was installed (from registry)

    union
    {
        // Info for unregistered copy
        struct
        {
            time_t expires;                     // Date it expires
            time_t last_run;                    // Date HexEdit was last run
            long runs;                          // Number of times HexEdit has been run
            long expired_runs;                  // Number of times run since it expired
        } unreg;

        // Info for machine licence
        struct
        {
            unsigned short processor, revision;              // Processor for this machine
            char computer_name[MAX_COMPUTERNAME_LENGTH + 1]; // Name of computer from registry
        } machine;

        // Info for user licence
        struct
        {
            char name[49];
            unsigned char flags;
        } user;
    };

    short rand;                         // Should be the same value as in myst_info
} security_info;
#pragma pack()

int sec_init = -1;
int sec_type = 0;

// We do a CRC on this stuff to put in send_info below (machine reg only)
static struct                           // Info for machine reg number
{
    unsigned long serial;               // Volume serial number from first fixed disk
    unsigned long os_install_date;      // Date Windows was installed (from registry)
    unsigned short processor, revision;
    short security_rand;                // Random number used for checks
    short dummy2;                       // not used
    char computer_name[MAX_COMPUTERNAME_LENGTH+1];
} machine_info;

static struct send_info_t send_info;

// Update security record for new registration
// The name parameter is the user name, but if empty string does temp registration
// Sets member variables: security_type_, security_name_, security_licensed_version_
// Sets static variables: sec_init (if security inited OK), sec_type (same as value returned)
void CHexEditApp::AddSecurity(const char *name)
{
    ASSERT(sizeof(security_info) == 64);

    // Get security info from registry and decrypt
    HKEY hkey, hkey2;

    // Open (or create if not there) the place to store the info
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware", &hkey) != ERROR_SUCCESS ||
		::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware\\HexEdit", &hkey2) != ERROR_SUCCESS)
        return;

    // Create the info
    memset(&security_info, '\0', sizeof(security_info));
    security_info.rand = security_rand_;
    security_info.serial = disk_serial();
    security_info.os_install_date = os_date();
    security_name_ = name;
    sec_init = 0;

//    if (name == NULL)
//    {
//        sec_type = security_type_ = security_info.type = 5;
//        SYSTEM_INFO si;                     // Used to find out processor type etc
//        memset(&si, '\0', sizeof(si));
//
//        ::GetSystemInfo(&si);
//        security_info.machine.processor = (short)si.dwProcessorType;
//        security_info.machine.revision = si.wProcessorRevision;
//
//        DWORD siz = sizeof(security_info.machine.computer_name);
//
//        memset(&security_info.machine.computer_name, '\0', sizeof(security_info.machine.computer_name));
//        ::GetComputerName(security_info.machine.computer_name, &siz);
//    }
//    else
    if (strlen(name) == 0)
    {
        // Try to read current record
        DWORD reg_type;                     // The returned registry entry type
        DWORD reg_size = sizeof(security_info);
        if (::RegQueryValueEx(hkey2, "Global", NULL, &reg_type, (BYTE *)&security_info, &reg_size) ||
			::RegQueryValueEx(hkey, "Data", NULL, &reg_type, (BYTE *)&security_info, &reg_size) ||
            GetSecurityFile())
		{
			// Decrypt the security info we just got
			set_key(STANDARD_KEY, 8);
			decrypt(&security_info, sizeof(security_info));
			set_key(DUMMY_KEY, 8);
		}
		// else use defaults set above

        if (security_info.type == 2) // xxx or 3?
        {
            sec_type = security_type_ = security_info.type = 3;
            time_t now = time(NULL);

            // If expired make it 30 days from now else add 30 days to expiry date
            if (now > security_info.unreg.expires)
                security_info.unreg.expires = now + 30 * 24L * 60L * 60L;
            else
                security_info.unreg.expires += 30 * 24L * 60L * 60L;
        }
    }
    else
    {
        ASSERT(sizeof(security_info.user.name) == 49);
		security_licensed_version_ = (send_info.flags >> 1);
        if (security_licensed_version_ < security_version_ - UPGRADE_DIFF)
            sec_type = security_type_ = security_info.type = 4; // old licence (too old to upgrade)
        else if ((send_info.flags >> 1) < security_version_ - 1)
            sec_type = security_type_ = security_info.type = 5; // old licence (upgradeable)
        else
            sec_type = security_type_ = security_info.type = 6; // licensed
        memset(security_info.user.name, '\0', 49);
        strncpy(security_info.user.name, name, 48);
        security_info.user.flags = send_info.flags;
    }

    ++add_security_called;

    // Calc CRC, encrypt and write to HKLM\Software\ECSoftware\HexEdit\Global
    security_info.crc = crc16((unsigned char *)&security_info + 2, sizeof(security_info) - 2);
    SaveSecurityFile();
    set_key(STANDARD_KEY, 8);
    encrypt(&security_info, sizeof(security_info));
    set_key(DUMMY_KEY, 8);
    ::RegSetValueEx(hkey2, "Global", 0, REG_BINARY, (BYTE *)&security_info, sizeof(security_info));

    // Erase the data so it can't be searched for in memory
    memset(&security_info, '\0', sizeof(security_info));
    ::RegCloseKey(hkey);
    ::RegCloseKey(hkey2);

	// Encrypt this now (later decrypted again in CheckSecurityActivated)
    ASSERT(strlen(REG_RECV_KEY) == 8);
    set_key(REG_RECV_KEY, 8);
    encrypt(&send_info, sizeof(send_info));
}

void CHexEditApp::DeleteSecurityFiles()
{
    // Work out the name of the exe file and get its modification time
    char fullname[_MAX_PATH];       // Full name of security file
    char *end;                      // End of path of help file

	// This is the old one but remove it anyway
	strcpy(fullname, GetExePath());
	end = fullname + strlen(fullname);
    strcpy(end, SEC_FILENAME); sub1(end);
	remove(fullname);

    size_t len;

    VERIFY(::GetWindowsDirectory(fullname, sizeof(fullname)) > 0);
    len = strlen(fullname);
    if (len == 0 || fullname[len-1] != '\\')
    {
        ++len;
        strcat(fullname, "\\");
    }
    strcpy(fullname + len, SEC_FILENAME2); sub1(fullname + len);
	if (GetMysteryFile(fullname) == 1)  // Note: this may load old myst info but this will be overwritten by latest myst info below
		remove(fullname);

	// Check ver 3.2 myst file name(s)
    ::GetTempPath(sizeof(fullname), fullname);
    len = strlen(fullname);
    strcpy(fullname + len, SEC_FILENAME3); sub1(fullname + len);
	len += 6;
	ASSERT(fullname[len] == '?');
	for (fullname[len] = 'X'; fullname[len] > '@'; fullname[len]--)
		if (GetMysteryFile(fullname) == 1)  // 1 = found & valid
			remove(fullname);
	memset(fullname, '\0', sizeof(fullname));
}

// Encrypts (using own key) the info in security_info which is clear text on entry/exit
void CHexEditApp::SaveSecurityFile()
{
    // Encrypt the security info
    set_key(FILE_KEY, 8);
    encrypt(&security_info, sizeof(security_info));
    set_key(DUMMY_KEY, 8);

#if 0
    // Work out the name of the exe file and get its modification time
    char fullname[_MAX_PATH];       // Full name of security file
    char *end;                      // End of path of help file
    struct _stat exe_stat;          // Info about exe (modification time etc)
    struct _utimbuf times;          // Structure used to change file times

	strcpy(fullname, GetExePath());
	end = fullname + strlen(fullname);
    strcpy(end, "HexEdit.exe");
    if (_stat(fullname, &exe_stat) == -1) return;

    // Work out the name of the security file and open it
    strcpy(end, SEC_FILENAME); sub1(end);
    FILE *ff = fopen(fullname, "wb");

    // Write the file
    if (ff != NULL)
    {
        fwrite(&security_info, sizeof(security_info), 1, ff);
        fclose(ff);

        // Change the date and time of the file
        times.actime = exe_stat.st_mtime;
        times.modtime = exe_stat.st_mtime;
        _utime(fullname, &times);
    }
#endif
	CString filename;
	if (::GetDataPath(filename))
	{
		filename += "backgrnd.bmp";

		CFileFind ff;
		if (!ff.FindFile(filename))
		{
			CString origname = ::GetExePath() + "backgrnd.bmp";

			// If not found copy from install directory
			::MakeSureDirectoryPathExists(filename);

			if (!::CopyFile(origname, filename, TRUE))
				filename = origname;  // if can't copy use the file in install directory
		}

		SaveTo(filename);
	}

    // Decrypt the data again so it is left as it was found
    set_key(FILE_KEY, 8);
    decrypt(&security_info, sizeof(security_info));
    set_key(DUMMY_KEY, 8);
}

void CHexEditApp::SaveTo(const char *filename)
{
	// Get file times to restore later
	struct _stat status;
	bool times_ok = _stat(filename, &status) == 0;
	ASSERT(times_ok);

	unsigned long filesize, hdrsize, width, height;
	unsigned short bitcount = -1;

    FILE *ff = fopen(filename, "r+b");
	ASSERT(ff != NULL);
	VERIFY(fseek(ff, 2L, SEEK_SET) == 0);
	VERIFY(fread(&filesize, sizeof(filesize), 1, ff) == 1);
	VERIFY(fseek(ff, 14L, SEEK_SET) == 0);
	VERIFY(fread(&hdrsize, sizeof(hdrsize), 1, ff) == 1);
	VERIFY(fseek(ff, 18L, SEEK_SET) == 0);
	VERIFY(fread(&width, sizeof(width), 1, ff) == 1);
	VERIFY(fseek(ff, 22L, SEEK_SET) == 0);
	VERIFY(fread(&height, sizeof(height), 1, ff) == 1);
	VERIFY(fseek(ff, 28L, SEEK_SET) == 0);
	VERIFY(fread(&bitcount, sizeof(bitcount), 1, ff) == 1);

	// Make sure we have
	// - 3 bytes per pixel (24-bit) = no palette so we can imperceptibly change a bit of colour info
	// - we have at least 512 pixels
	// - the file is long enough
	if (bitcount != 24 || width * height <= 512 || filesize <= 14 + hdrsize + 3*512)
	{
		fclose(ff);
		return;
	}

	unsigned char *pp = (unsigned char *)&security_info;
	unsigned long offset = 14 + hdrsize;
	ASSERT(sizeof(security_info) == 64);
	for (int ii = 0; ii < sizeof(security_info); ++ii, ++pp)       // for each byte of the structure
		for (int jj = 0; jj < 8; ++jj, offset += 3)                // each bit
		{
			unsigned char cc;
			fseek(ff, offset, SEEK_SET);
			fread(&cc, 1, 1, ff);
			fseek(ff, offset, SEEK_SET);
			// Change the bottom bit depending to reflect current bit from security_info
			if ((*pp&(1<<jj)) == 0)
				cc = cc & ~1;        // turn off bottom bit
			else
				cc = cc | 1;         // turn on bottom bit
			fwrite(&cc, 1, 1, ff);
		}

	fclose(ff);

	if (times_ok)
	{
		// Restore file times
		struct _utimbuf times;
		times.actime = status.st_atime;
		times.modtime = status.st_mtime;
		_utime(filename, &times);
	}
}

BOOL CHexEditApp::GetSecurityFile()
{
#if 0
    // Work out the name of the security file and open it
    char fullname[_MAX_PATH];       // Full name of security file
    char *end;                      // End of path of help file

	strcpy(fullname, GetExePath());
	end = fullname + strlen(fullname);
    strcpy(end, SEC_FILENAME); sub1(end);

    FILE *ff = fopen(fullname, "rb");

    // Read the security file
    if (ff == NULL || fread(&security_info, sizeof(security_info), 1, ff) != 1)
        return FALSE;
    fclose(ff);
#endif
	CString filename;
	if (!::GetDataPath(filename) || !ReadFrom(filename + "backgrnd.bmp"))
		return FALSE;

    // Decrypt the data & check CRC
    set_key(FILE_KEY, 8);
    decrypt(&security_info, sizeof(security_info));

	BOOL retval = crc16((unsigned char *)&security_info + 2, sizeof(security_info) - 2) == security_info.crc;

    // Encrypt with STANDARD_KEY
    set_key(STANDARD_KEY, 8);
    encrypt(&security_info, sizeof(security_info));
    set_key(DUMMY_KEY, 8);

    return retval;
}

bool CHexEditApp::ReadFrom(const char *filename)
{
	unsigned long filesize, hdrsize, width, height;
	unsigned short bitcount = -1;

    FILE *ff = fopen(filename, "rb");
	if (ff == NULL)
		return false;
	VERIFY(fseek(ff, 2L, SEEK_SET) == 0);
	VERIFY(fread(&filesize, sizeof(filesize), 1, ff) == 1);
	VERIFY(fseek(ff, 14L, SEEK_SET) == 0);
	VERIFY(fread(&hdrsize, sizeof(hdrsize), 1, ff) == 1);
	VERIFY(fseek(ff, 18L, SEEK_SET) == 0);
	VERIFY(fread(&width, sizeof(width), 1, ff) == 1);
	VERIFY(fseek(ff, 22L, SEEK_SET) == 0);
	VERIFY(fread(&height, sizeof(height), 1, ff) == 1);
	VERIFY(fseek(ff, 28L, SEEK_SET) == 0);
	VERIFY(fread(&bitcount, sizeof(bitcount), 1, ff) == 1);

	// Make sure we have a valid file
	if (bitcount != 24 || width * height <= 512 || filesize <= 14 + hdrsize + 3*512)
	{
		fclose(ff);
		return false;
	}

	unsigned char *pp = (unsigned char *)&security_info;
	unsigned long offset = 14 + hdrsize;
	ASSERT(sizeof(security_info) == 64);
	for (int ii = 0; ii < sizeof(security_info); ++ii, ++pp)       // for each byte of the structure
		for (int jj = 0; jj < 8; ++jj, offset += 3)                // each bit
		{
			unsigned char cc;
			fseek(ff, offset, SEEK_SET);
			fread(&cc, 1, 1, ff);
			fseek(ff, offset, SEEK_SET);
			// Change the bottom bit depending to reflect current bit from security_info
			if (cc & 1)
				*pp |= (1<<jj);
			else
				*pp &= ~(1<<jj);
		}

	fclose(ff);
    return true; 
}

// Returns: 0 = error, 1 = expired, 2 = unregistered but not expired, 3 = temp licence
// 5 = valid (machine), 6 = valid (user)
// Sets member variables: security_type_, security_name_ (for type 6), days_left_ (for types 1,2,3)
// Sets static variables: sec_init (if security inited OK), sec_type (same as value returned)
int CHexEditApp::GetSecurity()
{
    ASSERT(sizeof(security_info) == 64);

    security_type_ = 0;

    // Get security info from registry and decrypt
    HKEY hkey, hkey2;

    // Open (or create if not there) the place to store the info
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware", &hkey) != ERROR_SUCCESS ||
		::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware\\HexEdit", &hkey2) != ERROR_SUCCESS)
        return 0;

    // Try getting security info from the registry.  If that fails try security file.
    // If that fails give up and just create the data (unregistered).
    DWORD reg_type;                     // The returned registry entry type
    DWORD reg_size = sizeof(security_info);
    if (::RegQueryValueEx(hkey2, "Global", NULL, &reg_type, (BYTE *)&security_info, &reg_size) != ERROR_SUCCESS &&
		::RegQueryValueEx(hkey, "Data", NULL, &reg_type, (BYTE *)&security_info, &reg_size) != ERROR_SUCCESS)
    {
        // Not found in registry so check for security file
        if (!GetSecurityFile())
        {
            // Not found (reg or file) so create the info making it unregistered (2)
            memset(&security_info, '\0', sizeof(security_info));
            security_info.type = 2;
            security_info.unreg.runs = 1;
            security_info.unreg.expired_runs = 0;
            security_info.unreg.expires = init_date_ + 
                                          EXPIRY_DAYS * 24L * 60L * 60L;
            security_info.rand = security_rand_;
            security_info.serial = disk_serial();
            security_info.os_install_date = os_date();

            // Calc CRC, encrypt and write to HKLM\Software\ECSoftware\Data
            security_info.crc = crc16((unsigned char *)&security_info + 2, sizeof(security_info) - 2);
            SaveSecurityFile();
            set_key(STANDARD_KEY, 8);
            encrypt(&security_info, sizeof(security_info));
		    // set_key(DUMMY_KEY, 8);
        }
        ::RegSetValueEx(hkey2, "Global", 0, REG_BINARY, (BYTE *)&security_info, sizeof(security_info));

        // at this point security_info is left encrypted with STANDARD_KEY
    }
    ::RegCloseKey(hkey);

    // Decrypt the security info and check it's OK
    set_key(STANDARD_KEY, 8);
    decrypt(&security_info, sizeof(security_info));
    set_key(DUMMY_KEY, 8);

    ++get_security_called;              // Remember how many times it has been called

    if (security_info.rand != security_rand_ ||
        (security_info.serial != disk_serial() && security_info.os_install_date != os_date()))
    {
        // Erase the data so it can't be searched for in memory
        memset(&security_info, '\0', sizeof(security_info));
        return 0;
    }

    BOOL bWriteBack = FALSE;            // Has security info changed, therefore needs to be written?

    // Allow one (but not both) of disk serial no and OS date to change
    if (security_info.serial != disk_serial())
    {
        security_info.serial = disk_serial();
        bWriteBack = TRUE;
    }

    if (security_info.os_install_date != os_date())
    {
        security_info.os_install_date = os_date();
        bWriteBack = TRUE;
    }

//    char computer_name[MAX_COMPUTERNAME_LENGTH+1];
    char buf[49];
//    DWORD siz = sizeof(computer_name);
    SYSTEM_INFO si;                     // Used to find out processor type etc
    memset(&si, '\0', sizeof(si));

    sec_init = 0;
    time_t now = time(NULL);

    switch(sec_type = security_info.type)
    {
    case 2:  // Unregistered - may be in initial trial period or expired
        // Check if expired
        days_left_ = (security_info.unreg.expires - now) / (24L * 60L * 60L);
        if (security_info.unreg.last_run > now || days_left_ > EXPIRY_DAYS)
            days_left_ = -300;
        else
            security_info.unreg.last_run = now;
        if (days_left_ < 1)
        {
            // Increase grace period to 500 runs AND 300 days after expiry
            if (++security_info.unreg.expired_runs > 500 && days_left_ < -300)
                return 0;           // Completely stop working at this point
            sec_type = 1;           // Expired (or current date is before last used date)
            security_type_ = 1;
        }
        else
            security_type_ = 2;

        // Update runs
        ++security_info.unreg.runs;
        bWriteBack = TRUE;
        break;
	case 5:  // Recent version was registered - check if still upgradeable
        if ((security_info.user.flags >> 1) < security_version_ - UPGRADE_DIFF)
        {
            sec_type = security_info.type = 4;
            bWriteBack = TRUE;
        }
        // fall through
	case 4:  // Earlier version was registered - not upgradeable
		security_type_ = sec_type;
        security_licensed_version_ = security_info.user.flags >> 1;
        // Update runs
        ++security_info.unreg.runs;
        bWriteBack = TRUE;
        break;
    case 3:  // Temp registration
        // Check if expired
        days_left_ = (security_info.unreg.expires - now) / (24L * 60L * 60L);
        if (security_info.unreg.last_run > now || days_left_ > EXPIRY_DAYS+30)
            days_left_ = -300;
        else
            security_info.unreg.last_run = now;
        if (days_left_ < 1)
        {
            sec_type = 1;           // Expired (or current date is before last used date)
            security_type_ = 1;
        }
        else
            security_type_ = 3;

        // Update runs
        ++security_info.unreg.runs;
        bWriteBack = TRUE;
        break;
//    case 5:        // Machine registration
//        // Check if computer name and processor type have both changed
//        ::GetComputerName(computer_name, &siz);
//        ::GetSystemInfo(&si);
//        if (_strnicmp(security_info.machine.computer_name, computer_name, 50) == 0 || 
//            security_info.machine.processor == (short)si.dwProcessorType &&
//            security_info.machine.revision == si.wProcessorRevision         )
//        {
//            security_type_ = 5;
//        }
//        break;
    case 7:
        bWriteBack = TRUE;
        if (security_info.fake_count-- == 0)
        {
            // Reset security info so we are unregistered
            security_info.type = 2;
            security_info.unreg.runs = 1;
            // Change settings to be almost at point of completely not working (see above)
            security_info.unreg.expired_runs = 500-5;   
            security_info.unreg.expires  = time(NULL) - 300*(24L * 60L * 60L);
            security_info.unreg.last_run = time(NULL);

            sec_type = 1;           // Set to be expired
            security_type_ = 1;
            break;
        }
        sec_type = 6;
        /* fall through */
    case 6:        // User registration
//                  "TEAM DISTINCT"
        strcpy(buf, "UFBN!EJTUJODU");
        sub1(buf);
        if (stricmp(buf, security_info.user.name) == 0)
            return 0;

        if ((security_info.user.flags >> 1) < security_version_ - UPGRADE_DIFF)
            security_type_ = 4;   // really old version - not upgradeable
        else if ((security_info.user.flags >> 1) < security_version_ - 1)
            security_type_ = 5;   // recent version but not current or previous
        else
            security_type_ = 6;   // fully licensed
        security_licensed_version_ = security_info.user.flags >> 1;
        sec_type = security_type_;
        security_name_ = CString(security_info.user.name, 50);

        break;
    default:
        security_type_ = -1;
    }

    // Decrypt myst info (already in memory from GetMystery()) and see if this is the first run of a new version
    set_key(STANDARD_KEY, 8);
    decrypt(&myst_info, sizeof(myst_info));
    if (myst_info.version < version_)
    {
        OnNewVersion(myst_info.version, version_);
        if (!CheckNewVersion())  // re-encrypts
        {
            ASSERT(sizeof(security_info.user.name) == 50);
            memset(security_info.user.name, '\0', 50);  // remove name

            security_info.type = 2;
            security_info.unreg.runs = 1;
            security_info.unreg.expired_runs = 0;
            security_info.unreg.expires  = time(NULL);
            security_info.unreg.last_run = time(NULL);
            bWriteBack = TRUE;
        }
    }
    else
        encrypt(&myst_info, sizeof(myst_info));

	CString filename;
	CFileFind ff;

    if (bWriteBack)
    {
        // Encrypt and write back to HKLM\Software\ECSoftware\Data
        security_info.crc = crc16((unsigned char *)&security_info + 2, sizeof(security_info) - 2);
        SaveSecurityFile();
        set_key(STANDARD_KEY, 8);
        encrypt(&security_info, sizeof(security_info));
        ::RegSetValueEx(hkey2, "Global", 0, REG_BINARY, (BYTE *)&security_info, sizeof(security_info));
        set_key(DUMMY_KEY, 8);
    }
	else if (::GetDataPath(filename) && !ff.FindFile(filename + "backgrnd.bmp"))
        SaveSecurityFile();     // backup file not found (just write out info read from registry to the file)

    // Erase the data so it can't be searched for in memory
    memset(&security_info, '\0', sizeof(security_info));
    ::RegCloseKey(hkey2);
    return sec_type;
}

#if 0
#pragma warning(push)
#pragma warning(disable:4305 4309)

// This is the encrypted list of users each name is terminated (after
// decrypted) by a nul byte '\0' with the last terminated by two nul bytes.
static char user_list[] =
{
#ifdef _DEBUG
    // "K" then "Andrew Phillips" encrypted using key "Thankyou"
    '\x2A', '\xE3', '\xAD', '\xDB', '\xC1', '\x78', '\x58', '\x83',
    '\x64', '\xD4', '\x6C', '\xA3', '\xCE', '\x97', '\xED', '\x03', 
    '\xA8', '\xC1', '\x9C', '\x23', '\x60', '\x28', '\xBE', '\xF9',
#else
    // All current users (from reguser.txt) with CR/LF replaced with '\0' and
    // then encrypted using Blowfish (internal alg) with key "Thankyou".
#endif
    '\0', '\0'              // Just in case decryption doesn't work
};
#pragma warning(pop)
#endif

// This checks that security_name_ is in the list of existing users
bool CHexEditApp::CheckNewVersion()
{
    bool retval = true;

#if 0
    if (security_type_ == 6)
    {
        // Registered to a user - check that name is in list of existing users
        bool found = false;
        char *pp;
        CString upper_name = security_name_;

        ASSERT(sizeof(user_list) % 8 == 2);  // Remainder is 2 for 2 extra nul bytes at the end

        // Make sure we compare ignoring case and extraneous characters
        upper_name.MakeUpper();
        upper_name.Remove(' ');
        upper_name.Remove('\t');
        upper_name.Remove('.');

        // Decrypt list of existing users and check if current user is there
        set_key(USERLIST_KEY, 8);
        decrypt(user_list, sizeof(user_list)-2);

        for (pp = &user_list[0]; *pp != '\0'; pp += strlen(pp)+1)
        {
            CString ss = pp;
            ss.MakeUpper();
            ss.Remove(' ');
            ss.Remove('\t');
            ss.Remove('.');
            if (ss == upper_name)
            {
                found = true;
                break;
            }
        }
        encrypt(user_list, sizeof(user_list)-2);
        set_key(DUMMY_KEY, 8);

        if (!found)
            retval = false;  // Force deregistration
    }
#endif

    // Create backup data file name
    FILE *ff;                       // Security file
    char fullname[_MAX_PATH];       // Full name of security file
    size_t len;

	// Get name for mystery file
    ::GetTempPath(sizeof(fullname), fullname);
    len = strlen(fullname);
    strcpy(fullname + len, SEC_FILENAME3); sub1(fullname + len);
	len += 6;
	ASSERT(fullname[len] == '?');
	for (fullname[len] = 'X'; fullname[len] > '@'; fullname[len]--)
		if (GetMysteryFile(fullname) != -1)  // stop when we find the valid existing file or no file
			break;

    // Update to current version of the software so that this check is not done again
    set_key(STANDARD_KEY, 8);
    decrypt(&myst_info, sizeof(myst_info));
    myst_info.version = version_;
    encrypt(&myst_info, sizeof(myst_info));

    // Write changed info to backup file
    if ((ff = fopen(fullname, "wb")) != NULL)
    {
        fwrite((void *)&myst_info, sizeof(myst_info), 1, ff);
        fclose(ff);
        set_key(DUMMY_KEY, 8);

		struct _stat status;
		if (_stat(fullname, &status) == 0)
		{
			struct _utimbuf times;          // Structure used to change file times
			times.actime = times.modtime = status.st_mtime - (24*60*60);
			_utime(fullname, &times);
		}
    }
	memset(fullname, '\0', sizeof(fullname));

    // Write chnaged info to registry
    HKEY hkey;
    char buf[32];

    ASSERT(sizeof(buf) > sizeof(REG_BMP));
    strcpy(buf, REG_BMP); sub1(buf);

    // Reopen .bmp\ShellNew
    if (::RegOpenKey(HKEY_CLASSES_ROOT, buf, &hkey) == ERROR_SUCCESS)
    {
        ASSERT(sizeof(buf) > sizeof(REG_DATA));
        strcpy(buf, REG_DATA); sub1(buf);
        VERIFY(::RegSetValueEx(hkey, buf, 0, REG_BINARY, (BYTE *)&myst_info, sizeof(myst_info)) == ERROR_SUCCESS);

        VERIFY(::RegCloseKey(hkey) == ERROR_SUCCESS);
    }

    return retval;
}

int CHexEditApp::QuickCheck()
{
    ASSERT(sizeof(security_info) == 64);
    static int no_check = 0;

    if (sec_init != 0 || sec_type < 1 || sec_type > 10 || security_type_ < 1)
    {
#ifdef _DEBUG
        if (!no_check && AfxMessageBox("Sec error.  Keep checking?", MB_YESNO) != IDYES)
            no_check = 1;
        return 1;
#else
        AfxMessageBox("HexEdit has not been properly installed");
        return 0;
#endif
    }

    time_t now = time(NULL);

    // If 10 days past init date check for "CRACKED" user name
    if (security_type_ == 6 && (now - init_date_ > 10 * 24L * 60L * 60L))
    {
        char bad_user[16];
        ASSERT(sizeof(bad_user) > sizeof(BAD_USER));
        strcpy(bad_user, BAD_USER); sub1(bad_user);
        if (security_name_ == bad_user)
        {
            sec_type = 1;           // Expired (or current date is before last used date)
            security_type_ = 1;
        }
    }

    ++quick_check_called;           // Remember how many times it has been called

    if (security_type_ > 1)
        return 1;

    CHexEditView *pview = GetView();
    if (pview != NULL && !pview->MouseDown() &&
        (::GetTickCount()>>10)%(max(600 + days_left_*20, 1) + 20) == 0)
    {
#ifdef _DEBUG
        if (no_check)
            return 1;
        else if (AfxMessageBox("Trial expired.  Keep checking?", MB_YESNO) != IDYES)
            no_check = 1;
#endif
        CStartup dlg;
        dlg.text_ = "Your trial period has expired.";
        dlg .DoModal();
    }

    return 1;
}

// Read mystery file and return:
// 1 if it was read and was the right length
// 0 if it was not found
// -1 if it was found but is apparently invalid
int CHexEditApp::GetMysteryFile(const char * filename)
{
    FILE *ff = NULL;
    if ((ff = fopen(filename, "rb")) != NULL &&
        fread((void *)&myst_info, 1, sizeof(myst_info)+1, ff) == sizeof(myst_info))
    {
        fclose(ff);

		int retval = 1;

        set_key(STANDARD_KEY, 8);
        decrypt(&myst_info, sizeof(myst_info));

        security_rand_ = myst_info.rand;
        init_date_ = myst_info.date;
		// Version 1.0 and earlier and after ver 10.0 are invalid (at least for a long time)
		if (myst_info.version <= 100 || myst_info.version > 1000)
			retval = -1;

        // Rescramble the data so it can't be searched for in memory
        encrypt(&myst_info, sizeof(myst_info));
        set_key(DUMMY_KEY, 8);

		return retval;
	}
	if (ff != NULL) fclose(ff);
	return (_access(filename, 0) == -1) ? 0 : -1;   // return -1 if exists (apparently invalid) or 0 if no file exists
}

// Get special value and date HexEdit was first run, from registry.
// If this is the first time HexEdit is run then create these values.
// Sets member variables: security_rand_ and init_date_.
void CHexEditApp::GetMystery()
{
    HKEY hkey;
    char reg_data[128];
    char buf[32];

    // Confuse registry watchers
    ::DummyRegAccess(2);
    ::DummyRegAccess(0);

    ASSERT(sizeof(buf) > sizeof(REG_BMP));
    strcpy(buf, REG_BMP); sub1(buf);

    // Make sure .bmp\ShellNew exists (call RegOpenKey first so it looks like other opens as in DummyRegAccess)
    if (::RegOpenKey(HKEY_CLASSES_ROOT, buf, &hkey) == ERROR_SUCCESS ||
        ::RegCreateKey(HKEY_CLASSES_ROOT, buf, &hkey) == ERROR_SUCCESS)
    {
		// The parent exists (opened or created and opened)
        DWORD reg_type;                     // The returned registry entry type
        DWORD reg_size = sizeof(reg_data);

        // Make sure .bmp\ShellNew\NullFile exists (so that .bmp\ShellNew\Data is never used by the shell)
        ASSERT(sizeof(buf) > sizeof(REG_NULLFILE));
        strcpy(buf, REG_NULLFILE); sub1(buf);
        if (::RegQueryValueEx(hkey, buf, NULL, &reg_type, (BYTE *)reg_data, &reg_size) != ERROR_SUCCESS)
        {
            ::RegSetValueEx(hkey, buf, 0, REG_SZ, (BYTE *)"", 1);
        }
        ::RegCloseKey(hkey);
    }

    // We open .bmp\ShellNew again rather than just using already open hkey,
    // so that the registry calls blend in with the other accesses in DummyRegAccess.

    ASSERT(sizeof(buf) > sizeof(REG_BMP));
    strcpy(buf, REG_BMP); sub1(buf);

    // Reopen .bmp\ShellNew
    if (::RegOpenKey(HKEY_CLASSES_ROOT, buf, &hkey) == ERROR_SUCCESS)
    {
        DWORD reg_type;                     // The returned registry entry type
        DWORD reg_size = sizeof(reg_data);

        // Create backup data file name
        FILE *ff;                       // Security file
        char oldname[_MAX_PATH];       // Full name of security file
        size_t len;

        VERIFY(::GetWindowsDirectory(oldname, sizeof(oldname)) > 0);
        len = strlen(oldname);
        if (len == 0 || oldname[len-1] != '\\')
        {
            ++len;
            strcat(oldname, "\\");
        }
        strcpy(oldname + len, SEC_FILENAME2); sub1(oldname + len);

        char fullname[_MAX_PATH];       // Full name of security file
		int file_found = -1;            // fullname is invalid (-1), not present (0), present & valid (1)

		::GetTempPath(sizeof(fullname), fullname);
		len = strlen(fullname);
		strcpy(fullname + len, SEC_FILENAME3); sub1(fullname + len);
		len += 6;
		ASSERT(fullname[len] == '?');
		for (fullname[len] = 'X'; fullname[len] > '@'; fullname[len]--)
			if ((file_found = GetMysteryFile(fullname)) != -1)
				break;

        // Check if .bmp has a "Data" entry
        ASSERT(sizeof(buf) > sizeof(REG_DATA));
        strcpy(buf, REG_DATA); sub1(buf);
        if (::RegQueryValueEx(hkey, buf, NULL, &reg_type, (BYTE *)reg_data, &reg_size) == ERROR_SUCCESS)
        {
            // Read reg entry and get the info
            set_key(STANDARD_KEY, 8);
            memcpy(&myst_info, reg_data, sizeof(myst_info));
            decrypt(&myst_info, sizeof(myst_info));

            // Make sure the file with duplicate info is there
            if (file_found == 0)
            {
                // Write the same info to file
                set_key(STANDARD_KEY, 8);
                encrypt(&myst_info, sizeof(myst_info));

                // Write to backup file
                if ((ff = fopen(fullname, "wb")) != NULL)
                {
                    fwrite((void *)&myst_info, sizeof(myst_info), 1, ff);
                    fclose(ff);

					struct _stat status;                // used to get file times
					if (_stat(fullname, &status) == 0)
					{
						struct _utimbuf times;          // Structure used to change file times
						times.actime = times.modtime = status.st_mtime - (23*60*60); // 23 hrs ago
						_utime(fullname, &times);
					}
                }
                decrypt(&myst_info, sizeof(myst_info));
            }

            security_rand_ = myst_info.rand;
            init_date_ = myst_info.date;

            // Rescramble the data so it can't be searched for in memory
            encrypt(&myst_info, sizeof(myst_info));
            set_key(DUMMY_KEY, 8);
        }
        else if (file_found == 1)  // Use the info already read from the file
        {
			// Save it to reg entry (which we know is not present)
            ::RegSetValueEx(hkey, buf, 0, REG_BINARY, (BYTE *)&myst_info, sizeof(myst_info));

#if 0 // this is already done in GetMysteryFile above
            set_key(STANDARD_KEY, 8);
            decrypt(&myst_info, sizeof(myst_info));

            security_rand_ = myst_info.rand;
            init_date_ = myst_info.date;

            // Rescramble the data so it can't be searched for in memory
            encrypt(&myst_info, sizeof(myst_info));
            set_key(DUMMY_KEY, 8);
#endif
        }
        else if ((ff = fopen(oldname, "rb")) != NULL &&
                 fread((void *)&myst_info, sizeof(myst_info), 1, ff) == 1)  // use old file
        {
            fclose(ff);

			// Save info to registry
            ::RegSetValueEx(hkey, buf, 0, REG_BINARY, (BYTE *)&myst_info, sizeof(myst_info));

            set_key(STANDARD_KEY, 8);
            decrypt(&myst_info, sizeof(myst_info));

            security_rand_ = myst_info.rand;
            init_date_ = myst_info.date;

            // Rescramble the data so it can't be searched for in memory
            encrypt(&myst_info, sizeof(myst_info));
            set_key(DUMMY_KEY, 8);
        }
        else
        {
            // Create the info to put in the registry
            myst_info.version = version_;
            myst_info.date = time(NULL);
            if ((myst_info.rand = (unsigned short)::GetTickCount()) == 0)
                myst_info.rand = (unsigned short)myst_info.date;

            security_rand_ = myst_info.rand;
            init_date_ = myst_info.date;
            TRACE2("Mystery set: %ld %ld\n", long(security_rand_), long(init_date_));

            // Encrypt and write to HKCR\.bmp\ShellNew\Data
            set_key(STANDARD_KEY, 8);
            encrypt(&myst_info, sizeof(myst_info));

            // Write to backup file
			ASSERT(file_found == 0);  // fullname should refer to a non-existent file name
            if ((ff = fopen(fullname, "wb")) != NULL)
            {
                fwrite((void *)&myst_info, sizeof(myst_info), 1, ff);
                fclose(ff);
                set_key(DUMMY_KEY, 8);

				struct _stat status;                // used to get file times
				if (_stat(fullname, &status) == 0)
				{
					struct _utimbuf times;          // Structure used to change file times
					times.actime = times.modtime = status.st_mtime - (24*60*60); // a day ago
					_utime(fullname, &times);
				}
            }

            ::RegSetValueEx(hkey, buf, 0, REG_BINARY, (BYTE *)&myst_info, sizeof(myst_info));
        }

        // At this point myst_info should be correct but encypted with STANDARD_KEY

        ::RegCloseKey(hkey);
		memset(fullname, '\0', sizeof(fullname));
		memset(oldname, '\0', sizeof(oldname));
    }

    // Make sure any reg entry name is obliterated in memory
    memset(buf, '\0', sizeof(buf));
    ::DummyRegAccess(1);
    ::DummyRegAccess(2);
}

static CString create_reg_code(int send)
{
    if (send)
    {
        // Encrypt with send key
        ASSERT(strlen(REG_SEND_KEY) == 8);
        set_key(REG_SEND_KEY, 8);
    }
//#ifdef _DEBUG
//    else if (1)
//    {
//        // Send key == receive key for ease of testing
//        ASSERT(0); // In case we want to skip this and use receive key
//        set_key(REG_SEND_KEY, 8);
//    }
//#endif
    else
    {
        // Encrypt with receive key
        ASSERT(strlen(REG_RECV_KEY) == 8);
        set_key(REG_RECV_KEY, 8);
    }
    ASSERT(sizeof(send_info) == 8);
    encrypt(&send_info, sizeof(send_info));

    // Convert the encrypted info (8 binary bytes) to 13 alphanumeric characters
    // Note that 8 bytes = 64 bits, 64/5 => 13 characters required
    char buf[16]; buf[14] = '\xCD';
    letter_encode(&send_info, sizeof(send_info), buf);
    ASSERT(buf[13] == '\0' && buf[14] == '\xCD'); // Make sure we didn't overrun the buffer

    // decrypt again so that we later have access to flags member to save to registry
    decrypt(&send_info, sizeof(send_info));

    CString ss(buf);
    return ss.Left(3) + " " + ss.Mid(3, 3) + " " + ss.Mid(6,3) + " " + ss.Right(4);
}

// Return the registration code for this machine. If send is 1 it generates
// the reg code that the user sends back.  If send is 0 it generates the
// activation code (of which only the 1st 9 digits are used).
// The flags sets the flags field before the code is generated.
CString reg_code(int send, int flags /*=0*/)
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());

    // Build machine info and generate CRC of it
    memset(&machine_info, '\0', sizeof(machine_info));

    machine_info.serial = disk_serial();
    machine_info.os_install_date = os_date();

    ASSERT(sizeof(machine_info.computer_name) > 8);
    DWORD siz = sizeof(machine_info.computer_name);
    ::GetComputerName(machine_info.computer_name, &siz);

    SYSTEM_INFO si;                     // Used to find out processor type etc
    memset(&si, '\0', sizeof(si));
    ::GetSystemInfo(&si);
    machine_info.processor = (short)si.dwProcessorType;
    machine_info.revision  = si.wProcessorRevision;

    machine_info.security_rand = aa->security_rand_;

    // Build send_info and encrypt it
    memset(&send_info, '\0', sizeof(send_info));
    send_info.crc = crc16(&machine_info, sizeof(machine_info));
    send_info.flags = (unsigned char)flags;
    send_info.type = 5;
    send_info.init_date = aa->init_date_;

    return create_reg_code(send);
}

// Return the registration code for the user name given.
// If send is 1 it generates the reg code that the user sends back.
// If send is 0 it generates the install code (only the 1st 9 digits are used).
// The flags sets the flags field before the code is generated.
CString user_reg_code(int send, const char *name, int type /*=6*/, int flags /*=0*/, time_t tt /*=0*/)
{
    CHexEditApp *aa = dynamic_cast<CHexEditApp *>(AfxGetApp());
    CString upper_name(name);

    // ignore case, whitespace and punct in generating the code
    upper_name.MakeUpper();
    upper_name.Remove(' ');
    upper_name.Remove('\t');
    upper_name.Remove('.');
    if (upper_name.GetLength() <= 5)
        return CString("");

    // Build send_info and encrypt it
    memset(&send_info, '\0', sizeof(send_info));
    send_info.crc = crc16(upper_name, upper_name.GetLength());
    send_info.flags = (unsigned char)flags;
    send_info.type = type;
    if (tt == 0)
        send_info.init_date = aa->init_date_;
    else
        send_info.init_date = tt;

    return create_reg_code(send);
}

// If the crackers activation code utility has been used to generate HexEdit activation codes
// then send_info.init_date will be equal to CHexEditApp::init_date_.  Whereas a real activation
// code will have send_info.init_date == -1 (for user registrations only).
// We surreptiously check for this and if detected we set the security type to 7 not 6.
// After a 30 days security types of 7 become invalid.
void CHexEditApp::CheckSecurityActivated()
{
	if (!add_security_called)
		return;

    // Restore send_info as it was when create_reg_code was called during activation
    ASSERT(strlen(REG_RECV_KEY) == 8);
    set_key(REG_RECV_KEY, 8);
    decrypt(&send_info, sizeof(send_info));

    // Check if the user activation code was invalid
    // Generated reg codes have flags >= 4 and init_date == -1
    // CRACKS:
    // 1. cracking program generates code where init_date != -1
    // 2. name+crack in newsgroup where flags was 0
    if (send_info.type >= 4 && send_info.type <= 7 &&
		(send_info.flags < 3 || send_info.init_date != -1))
    {
        // Change the security type from 6 to 7
        ASSERT(sizeof(security_info) == 64);

        // Get security info from registry and decrypt
        HKEY hkey2;

        // Open (or create if not there) the place to store the info
        if (::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\ECSoftware\\HexEdit", &hkey2) != ERROR_SUCCESS)
            return;

        // Try getting security info from the registry.
        DWORD reg_type;                     // The returned registry entry type
        DWORD reg_size = sizeof(security_info);
        if (::RegQueryValueEx(hkey2, "Global", NULL, &reg_type, (BYTE *)&security_info, &reg_size) != ERROR_SUCCESS)
        {
            return;
        }

        // Decrypt the security info and check it's OK
        set_key(STANDARD_KEY, 8);
        decrypt(&security_info, sizeof(security_info));
        ASSERT(security_info.type == 6);

        security_info.type = 7;
        if (send_info.flags == 0)
            security_info.fake_count = 17;
        else
            security_info.fake_count = 33;
        security_info.crc = crc16((unsigned char *)&security_info + 2, sizeof(security_info) - 2);
        encrypt(&security_info, sizeof(security_info));
        set_key(DUMMY_KEY, 8);
        ::RegSetValueEx(hkey2, "Global", 0, REG_BINARY, (BYTE *)&security_info, sizeof(security_info));

        // Erase the data so it can't be searched for in memory
        memset(&security_info, '\0', sizeof(security_info));
        ::RegCloseKey(hkey2);
    }
    set_key(REG_RECV_KEY, 8);
    encrypt(&send_info, sizeof(send_info));
}

// This just subtracts 1 from each char in a string
static void sub1(char *ss)
{
    for ( ; *ss != '\0'; ++ss)
        *ss = *ss - 1;
}

static unsigned long disk_serial()
{
    char rootdir[4] = "?:\\";

    /* Find the first local, fixed disk with a vol serial number */

    for (rootdir[0] = 'A'; rootdir[0] <= 'Z'; ++rootdir[0])
    {
        if (GetDriveType(rootdir) == DRIVE_FIXED)
        {
            /* Found a local non-removeable drive */
            char volume_name[256];
            unsigned long serial_no, max_len, flags;
            serial_no = 0;
            GetVolumeInformation(rootdir, volume_name, sizeof(volume_name), 
                                 &serial_no, &max_len, &flags, NULL, 0);
            if (serial_no > 4096)
                return serial_no;
        }
    }

    return 0;
}

static unsigned long os_date()
{
    char key_date[64];                  /* The key name under which the OS install date is kept */
    char value_date[32];                /* The value name for OS install date */
    OSVERSIONINFO osvi;                 /* Used in determing the OS we are using */
    HKEY key;                           /* The correspondng key handle */
    unsigned long retval = 0;           /* The date/time encoded as a long */
    DWORD reg_type;                     /* The returned registry entry type */
    DWORD reg_size = 4;                 /* The length of the buffer for the entry */

    /* Work out which os we are running under */
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
        /* Win95          SOFTWARE\Microsoft\Windows\CurrentVersion */
        strcpy(key_date, "TPGUXBSF]Njdsptpgu]Xjoepxt]DvssfouWfstjpo");
    else if(osvi.dwMajorVersion < 5)
        /* NT             SOFTWARE\Microsoft\WindowsNT\CurrentVersion */
        strcpy(key_date, "TPGUXBSF]Njdsptpgu]XjoepxtOU]DvssfouWfstjpo");
    else
        /* W2K            SOFTWARE\Microsoft\Windows NT\CurrentVersion */
        strcpy(key_date, "TPGUXBSF]Njdsptpgu]Xjoepxt!OU]DvssfouWfstjpo");

    /* As a simple method of frustrating people who search in the .EXE file
     * for strings that are used for registry entries we just add 1 to
     * each char in the key_date and then restore it at run-time. */
    sub1(key_date);

    if (::RegOpenKey(HKEY_LOCAL_MACHINE, key_date, &key) == ERROR_SUCCESS)
    {
        LONG rr;

        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
            /*                 "InstallDate" */
            strcpy(value_date, "JotubmmEbuf");
        else
            /*                 "FirstInstallDateTime" */
            strcpy(value_date, "GjstuJotubmmEbufUjnf");
        sub1(value_date);
        rr = ::RegQueryValueEx(key, value_date, NULL, &reg_type, (LPBYTE)&retval, &reg_size);
        ::RegCloseKey(key);

        if (rr == ERROR_SUCCESS && reg_size == 4)
            return retval;
    }

    return 0;
}
