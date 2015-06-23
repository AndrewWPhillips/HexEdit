// UpdateChecker.h : header file for UpdateChecker class
//
// Copyright (c) 2015 by Andrew W. Phillips
//
// This file is distributed under the MIT license, which basically says
// you can do what you want with it and I take no responsibility for bugs.
// See http://www.opensource.org/licenses/mit-license.php for full details.
//

#pragma once

class UpdateChecker
{
public:
	UpdateChecker(LPCTSTR url) : strUrl(url) { }

	bool Online();              // Can we check for an update?
	bool UpdateAvailable(int);  // Is a newer version available?

private:
	CString strUrl;
};
