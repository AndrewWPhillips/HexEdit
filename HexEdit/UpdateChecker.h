// UpdateChecker.h : header file for UpdateChecker class
//
// Copyright (c) 2010 by Andrew W. Phillips. 
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
