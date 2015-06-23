// SystemSound.cpp : Implements CSystemSound class
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#include "stdafx.h"
#include <Mmsystem.h>           // For PlaySound()
#include "SystemSound.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Creates a new sound event for the application, and assigns it a sound.
// If value is NULL then the sound is created but no sound is assigned (the
// user can then add a sound to the sound event in the Control Panel).
// If the sound event already exists no changes are made.
void CSystemSound::Add(LPCTSTR name, LPCTSTR value /*=NULL*/, LPCTSTR app /*=NULL*/, LPCTSTR key_name /*=NULL*/)
{
	ASSERT(name != NULL);
	HKEY key;                           // Key returned from registry open/create calls
	CString sound_name(name);

	if (app == NULL)
	{
		app = AfxGetAppName();
		sound_name = CString(app) + "_" + sound_name;
	}
	if (key_name == NULL) key_name = _T(".current");

	// Create the label key for this event
	VERIFY(::RegCreateKey(HKEY_CURRENT_USER, make_label_key(sound_name), &key) == ERROR_SUCCESS);
	VERIFY(::RegSetValue(key, NULL, REG_SZ, name, strlen(name)) == ERROR_SUCCESS);
	::RegCloseKey(key);

	// Open or create the Sound Events subkey for this app
	CString app_key_name;
	app_key_name.Format(_T("AppEvents\\Schemes\\Apps\\%s"), app);
	VERIFY(::RegCreateKey(HKEY_CURRENT_USER, app_key_name, &key) == ERROR_SUCCESS);
	VERIFY(::RegSetValue(key, NULL, REG_SZ, app, strlen(app)) == ERROR_SUCCESS);
	::RegCloseKey(key);

	// Test if the sound event is already assigned by trying to open it
	if (::RegOpenKey(HKEY_CURRENT_USER, make_key(sound_name, app, key_name), &key) != ERROR_SUCCESS)
	{
		// Not present so create the registry key for this sound event
		VERIFY(::RegCreateKey(HKEY_CURRENT_USER, make_key(sound_name, app, key_name), &key) == ERROR_SUCCESS);

		// Set the sound for this sound event if one is given
		if (value != NULL)
			VERIFY(::RegSetValue(key, NULL, REG_SZ, value, strlen(value)) == ERROR_SUCCESS);
	}
	::RegCloseKey(key);
}

// Gets a sound file name given the sound event name.
// Returns an empty string if there is no sound attached to the event.
CString CSystemSound::Get(LPCTSTR name, LPCTSTR app /*=NULL*/, LPCTSTR key_name /*=NULL*/)
{
	ASSERT(name != NULL);
	HKEY key;                           // Key returned from registry open/create calls
	CString sound_name(name);

	if (app == NULL)
	{
		app = AfxGetAppName();
		sound_name = CString(app) + "_" + sound_name;
	}
	if (key_name == NULL) key_name = _T(".current");

	CString retval;
	TCHAR buf[256] = "";                // Buffer that gets the name of the sound file to play
	LONG buf_len = sizeof(buf);         // Length of buffer

	// Open the key for the sound event and read its value
	if (::RegOpenKey(HKEY_CURRENT_USER, make_key(sound_name, app, key_name), &key) == ERROR_SUCCESS &&
		::RegQueryValue(key, NULL, buf, &buf_len) == ERROR_SUCCESS &&
		buf_len > 1)
	{
		// Just play the sound
		retval = buf;
		::RegCloseKey(key);
	}
	// else we just return an empty string if there is no assoc. sound

	return retval;
}

// Sets a sound file to be associated with a sound event.
// If value is NULL then any sound attached to the event is removed.
void CSystemSound::Set(LPCTSTR name, LPCTSTR value /*=NULL*/, LPCTSTR app /*=NULL*/, LPCTSTR key_name /*=NULL*/)
{
	ASSERT(name != NULL);
	HKEY key;                           // Key returned from registry open/create calls
	CString sound_name(name);

	if (app == NULL)
	{
		app = AfxGetAppName();
		sound_name = CString(app) + "_" + sound_name;
	}
	if (key_name == NULL) key_name = _T(".current");

	// Open the key for the sound event and read its value
	if (::RegOpenKey(HKEY_CURRENT_USER, make_key(sound_name, app, key_name), &key) == ERROR_SUCCESS)
	{
		if (value == NULL)
			VERIFY(::RegDeleteValue(key, NULL) == ERROR_SUCCESS);
		else
			VERIFY(::RegSetValue(key, NULL, REG_SZ, value, strlen(value)) == ERROR_SUCCESS);
	}
	else
		ASSERT(0);
}

// Plays a sound (if there is one) given by sound event name
BOOL CSystemSound::Play(LPCTSTR name, LPCTSTR app /*=NULL*/, LPCTSTR key_name /*=NULL*/)
{
	ASSERT(name != NULL);
	CString fname = Get(name, app, key_name);
	if (fname.IsEmpty())
		return TRUE;     // return success as no sound is wanted
	else
		return ::PlaySound(fname, NULL, SND_FILENAME);
}

// Make a key name from a an app name, a sound name and a key name (usually ".current")
CString CSystemSound::make_key(LPCTSTR name, LPCTSTR app, LPCTSTR key_name)
{
	ASSERT(name != NULL && app != NULL && key_name != NULL);
	CString ss = name;
	ss.Remove(' ');

	CString retval;
	retval.Format(_T("AppEvents\\Schemes\\Apps\\%s\\%s\\%s"), app, ss, key_name);
	return retval;
}

// Make a label key name from a sound name
CString CSystemSound::make_label_key(LPCTSTR name)
{
	ASSERT(name != NULL);
	CString ss = name;
	ss.Remove(' ');

	CString retval;
	retval.Format(_T("AppEvents\\EventLabels\\%s"), ss);
	return retval;
}
