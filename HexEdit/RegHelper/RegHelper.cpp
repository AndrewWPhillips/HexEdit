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
static void RegAll(const char * exepath, const char * shellkey, const char * shellsubkey, const char * shelldesc);
static void UnregAll(const char * shellkey, const char * shellsubkey);

static void Usage()
{
	_tprintf(_T("Usage:\r\n"));
	_tprintf(_T("   RegHelper EXT <exepath> <progid> <docname> <appid> <ext1,ext2,ext3,...>\r\n"));
	_tprintf(_T("eg:RegHelper EXT \"C:\\Program Files\\HexEditPro\\HexEditPro.exe\" \"HexEditPro.file\" \"HexEdit Pro file\" \"ECSoftware.HexEdit.Pro.40\" \".jpg|.png\"\r\n"));
	_tprintf(_T("   RegHelper REGALL <exepath> <shellkey> <shellsubkey> <shelldesc>\r\n"));
	_tprintf(_T("eg:RegHelper REGALL \"C:\\Program Files\\HexEditPro\\HexEditPro.exe\" \"*\\shell\\HexEditPro\" \"*\\shell\\HexEditPro\\command\" \"Open with HexEdit Pro\"\r\n"));
	_tprintf(_T("   RegHelper UNREGALL <shellkey> <shellsubkey>\r\n"));
	_tprintf(_T("eg:RegHelper UNREGALL \"*\\shell\\HexEditPro\" \"*\\shell\\HexEditPro\\command\"\r\n"));
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
	else if (_stricmp(argv[1], "REGALL") == 0)
	{
		if (argc < 6)
			return Usage(), 1;

		RegAll(argv[2], argv[3], argv[4], argv[5]);
	}
	else if (_stricmp(argv[1], "UNREGALL") == 0)
	{
		if (argc < 4)
			return Usage(), 1;

		UnregAll(argv[2], argv[3]);
	}

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

static void RegAll(const char * exepath, const char * shellkey, const char * shellsubkey, const char * shelldesc)
{
	CString s1 = CString(shelldesc);
	CString s2 = "\"" + CString(exepath) + "\"  \"%1\"";
	::RegSetValue(HKEY_CLASSES_ROOT, shellkey, REG_SZ, s1, s1.GetLength());
	::RegSetValue(HKEY_CLASSES_ROOT, shellsubkey, REG_SZ, s2, s2.GetLength());
}

static void UnregAll(const char * shellkey, const char * shellsubkey)
{
	::RegDeleteKey(HKEY_CLASSES_ROOT, shellsubkey); // Delete subkey first (for NT)
	::RegDeleteKey(HKEY_CLASSES_ROOT, shellkey);    // Delete registry entries
}

