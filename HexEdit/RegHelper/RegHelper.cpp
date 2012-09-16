// RegHelper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

static void Usage();
static BOOL CreateProgId(const char * exepath, const char * progid, const char * docname, const char * appid);
static void CreateOpenWith(const char * progid, const char * ext);

static void Usage()
{
	_tprintf(_T("Usage:\r\n"));
	_tprintf(_T("   RegHelper EXT <exepath> <progid> <docname> <appid> <ext1,ext2,ext3,...>\r\n"));
	// RegHelper EXT  "C:\Program Files\HexEdit Pro\HexEditPro.exe"  HexEditPro.file  "HexEdit Pro file"  ECSoftware.HexEdit.Pro.40  ".jpg|.png"
	// ARG:      1     2                                            3                 4                  5                           6
	_tprintf(_T("eg:RegHelper EXT \"C:\\Program Files\\HexEditPro\\HexEditPro.exe\" \"HexEditPro.file\" \"HexEdit Pro file\" \"ECSoftware.HexEdit.Pro.40\" \".jpg|.png\"\r\n"));
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// Initialise MFC
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (hModule == NULL || !AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: GetModuleHandle or AfxWinInit failed\n"));
		return 2;
	}

	if (argc < 2)
		return Usage(), 1;

	if (_stricmp(argv[1], "EXT") == 0)
	{
		if (argc < 7)
			return Usage(), 1;

		if (!PathFileExists(argv[2]))
		{
			_tprintf(_T("Fatal Error: Exe file not found\n"));
			return 3;
		}

		if (!CreateProgId(argv[2], argv[3], argv[4], argv[5]))
		{
			_tprintf(_T("Fatal Error: Failed to create progid\n"));
			return 4;
		}

		for (int ii = 0; ; ++ii)
		{
			CString ext;
			AfxExtractSubString(ext, argv[6], ii, '|');
			if (ext.IsEmpty())
				break;
			if (ext[0] != '.')
				ext = "." + ext;

			CreateOpenWith(argv[3], ext);
		}
	}
	// else add other commands here

	return 0;
}

static BOOL CreateProgId(const char * exepath, const char * progid, const char * docname, const char * appid)
{
	HKEY hkey, hkey_icon, hkey_cmd;
	if (::RegCreateKey(HKEY_CLASSES_ROOT, progid, &hkey) != ERROR_SUCCESS)
		return FALSE;

	// Tell Windows to use the first icon out of the .EXE file
	CString strIcon(exepath);
	strIcon += ",0";
	if (::RegCreateKey(hkey, "DefaultIcon", &hkey_icon) != ERROR_SUCCESS)
		return FALSE;
	::RegSetValueEx(hkey_icon, NULL, 0, REG_SZ, (BYTE *)(const char *)strIcon, strIcon.GetLength()+1);
	::RegCloseKey(hkey_icon);

	// Tell Windows how to run the command - just pass the file name as a command line parameter
	CString strCmd(exepath);
	strCmd += " %1";
	if (::RegCreateKey(hkey, "shell\\open\\command", &hkey_cmd) != ERROR_SUCCESS)
		return FALSE;
	::RegSetValueEx(hkey_cmd, NULL, 0, REG_SZ, (BYTE *)(const char *)strCmd, strCmd.GetLength()+1);
	::RegCloseKey(hkey_cmd);

	::RegSetValueEx(hkey, "FriendlyTypeName", 0, REG_SZ, (BYTE *)docname, strlen(docname)+1);
	::RegSetValueEx(hkey, "AppUserModelID",   0, REG_SZ, (BYTE *)appid,   strlen(appid)+1);

	::RegCloseKey(hkey);
	return TRUE;
}

static void CreateOpenWith(const char * progid, const char * ext)
{
	HKEY hkey_ext, hkey_owp;
	if (::RegCreateKey(HKEY_CLASSES_ROOT, ext, &hkey_ext) != ERROR_SUCCESS ||
		::RegCreateKey(hkey_ext, "OpenWithProgids", &hkey_owp) != ERROR_SUCCESS)
	{
		return;
	}
	::RegSetValueEx(hkey_owp, progid, 0, REG_BINARY, NULL, 0);

	::RegCloseKey(hkey_owp);
	::RegCloseKey(hkey_ext);
}
