// Crypto.cpp : implements the cryptography class
//
// Copyright (c) 2003-2010 by Andrew W. Phillips.
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
#include "crypto.h"

CCrypto::CCrypto()
{
	hdll_ = HINSTANCE(0);
}

void CCrypto::init()
{
	ASSERT(hdll_ == HINSTANCE(0));

	// Get the DLL
	if ((hdll_ = ::LoadLibrary("ADVAPI32.DLL")) == HINSTANCE(0))
		return;

	// Get the CryptoAPI routines we need
	if ((pAcquireContext_ = (PFAcquireContext)GetProcAddress(hdll_, "CryptAcquireContextA")) == 0 ||
		(pReleaseContext_ = (PFReleaseContext)GetProcAddress(hdll_, "CryptReleaseContext")) == 0 ||
		(pEnumProviders_ = (PFEnumProviders)GetProcAddress(hdll_, "CryptEnumProvidersA")) == 0 ||
		(pGetProvParam_ = (PFGetProvParam)GetProcAddress(hdll_, "CryptGetProvParam")) == 0 ||
		(pContextAddRef_ = (PFContextAddRef)GetProcAddress(hdll_, "CryptContextAddRef")) == 0 ||
		(pDeriveKey_ = (PFDeriveKey)GetProcAddress(hdll_, "CryptDeriveKey")) == 0 ||
		(pGenKey_ = (PFGenKey)GetProcAddress(hdll_, "CryptGenKey")) == 0 ||
		(pDestroyKey_ = (PFDestroyKey)GetProcAddress(hdll_, "CryptDestroyKey")) == 0 ||
		(pGetKeyParam_ = (PFGetKeyParam)GetProcAddress(hdll_, "CryptGetKeyParam")) == 0 ||
		(pEncrypt_ = (PFEncrypt)GetProcAddress(hdll_, "CryptEncrypt")) == 0 ||
		(pDecrypt_ = (PFDecrypt)GetProcAddress(hdll_, "CryptDecrypt")) == 0 ||
		(pCreateHash_ = (PFCreateHash)GetProcAddress(hdll_, "CryptCreateHash")) == 0 ||
		(pDestroyHash_ = (PFDestroyHash)GetProcAddress(hdll_, "CryptDestroyHash")) == 0 ||
		(pHashData_ = (PFHashData)GetProcAddress(hdll_, "CryptHashData")) == 0 )
	{
		ASSERT(0);
		::FreeLibrary(hdll_);
		hdll_ = HINSTANCE(0);
		return;
	}

	// Find all the CSPs
	for (int ii = 0;   ; ++ii)
	{
		char provider_name[256];
		DWORD provider_type;
		DWORD name_len = sizeof(provider_name);
		HCRYPTPROV hcsp;

		// CryptEnumProviders
		if (!pEnumProviders_(ii, NULL, 0, &provider_type, provider_name, &name_len))
		{
			if (::GetLastError() == ERROR_NO_MORE_ITEMS)
				break;          // No more CSPs
			else
			{
				ASSERT(0);
				continue;       // Can't access this CSP for some reason
			}
		}
		// CryptAcquireContext
		else if (!pAcquireContext_(&hcsp, NULL, provider_name, provider_type, CRYPT_VERIFYCONTEXT))
		{
			ASSERT(0);
			continue;
		}

		// Now get all the encryption algorithms of this service with CryptGetProvParam
		struct
		{
			ALG_ID algid;
			DWORD bits;
			DWORD len;
			CHAR name[256];
		} alginfo;
		DWORD len;
		DWORD flags;

		for (flags = CRYPT_FIRST;  ; flags = 0)
		{
			len = sizeof(alginfo);

			// Call CryptGetProvParam to get info on all the algorithms
			if (!pGetProvParam_(hcsp, PP_ENUMALGS, (BYTE *)&alginfo, &len, flags))
			{
				if (::GetLastError() == ERROR_NO_MORE_ITEMS)
					break;          // No more CSPs
				else
				{
					ASSERT(0);
					continue;       // Can't access this alg. for some reason
				}
			}

			// Only get encryption algorithms (not hash etc algorithms)
			if (GET_ALG_CLASS(alginfo.algid) == ALG_CLASS_DATA_ENCRYPT)
			{
				// Store the info about this CSP and algorithm in the vectors
				name_.push_back(CString(alginfo.name) + ":" + CString(provider_name));
				// since each entry gets a copy of hcsp we need to add to ref count using CryptContextAddRef
				pContextAddRef_(hcsp, NULL, 0);
				csp_.push_back(hcsp);
				algid_.push_back(alginfo.algid);
				key_.push_back(HCRYPTKEY(0));
				block_size_.push_back(-1);
				key_size_.push_back(-1);
			}
		}

		// Since we added ref counts for all stored copies of hcsp we can delete this one with CryptReleaseContext
		VERIFY(pReleaseContext_(hcsp, 0));
	}
}

CCrypto::~CCrypto()
{
	if (hdll_ == HINSTANCE(0))
		return;

	// For each alg free the CSP and key (if set)
	ASSERT(csp_.size() == key_.size());
	for (size_t ii = 0; ii < GetNum(); ++ii)
	{
		if (key_[ii] != HCRYPTKEY(0))
		{
			// CryptDestroyKey
//            VERIFY(pDestroyKey_(key_[ii]));
			pDestroyKey_(key_[ii]);    // Don't VERIFY as some CSPs return garbage
			key_[ii] = HCRYPTKEY(0);
		}

		// CryptReleaseContext
		VERIFY(pReleaseContext_(csp_[ii], 0));
	}

	::FreeLibrary(hdll_);
	hdll_ = HINSTANCE(0);
}

// Get algorith name (includes CSP name)
const char *CCrypto::GetName(size_t alg)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < name_.size());
	return name_[alg];
}

// Set encryption password (pass NULL to clear password, ie. to free the encryption key)
void CCrypto::SetPassword(size_t alg, const char *password /*=NULL*/)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < csp_.size());
	ASSERT(algid_.size() == csp_.size());
	ASSERT(key_.size() == csp_.size());
	ASSERT(block_size_.size() == csp_.size());
	ASSERT(key_size_.size() == csp_.size());

	// Delete existing key if any
	if (key_[alg] != HCRYPTKEY(0))
	{
		// CryptDestroyKey
//        VERIFY(pDestroyKey_(key_[alg])); // Don't VERIFY as some CSPs return garbage
		pDestroyKey_(key_[alg]);
		key_[alg] = HCRYPTKEY(0);
	}

	if (password != NULL)
	{
		HCRYPTHASH hh;

		// CryptCreateHash (try CALG_SHA or CALG_MD5 hask algorithms only)
		if (!pCreateHash_(csp_[alg], CALG_SHA, 0, 0, &hh) &&
			!pCreateHash_(csp_[alg], CALG_MD5, 0, 0, &hh) )
		{
			ASSERT(0);
			AfxMessageBox("Could not create hash using\n" + name_[alg]);
			return;
		}

		// Hash the password work and use the has to create the key
		// CryptHashData, CryptDeriveKey, CryptDestroyHash
		if (!pHashData_(hh, (const unsigned char *)password, strlen(password), 0) ||
			!pDeriveKey_(csp_[alg], algid_[alg], hh, 0, &key_[alg]) ||
			!pDestroyHash_(hh))
		{
			ASSERT(0);
			AfxMessageBox("Could not create key from password using\n" + name_[alg]);
			key_[alg] = HCRYPTKEY(0);
			return;
		}

		// Check if we have the block size (and key size)
		if (block_size_[alg] == -1)
		{
			// Get block size with CryptGetKeyParam
			DWORD len = sizeof(block_size_[alg]);
			VERIFY(pGetKeyParam_(key_[alg], KP_BLOCKLEN, (BYTE *)&block_size_[alg], &len, 0));
			ASSERT(len == sizeof(block_size_[alg]));
			// Get key size with CryptGetKeyParam
			len = sizeof(key_size_[alg]);
			key_size_[alg] = -1;                    // Use -1 for unknown (some CSPs return error)
			pGetKeyParam_(key_[alg], KP_KEYLEN, (BYTE *)&key_size_[alg], &len, 0);
			ASSERT(len == sizeof(key_size_[alg]));
		}
	}
}

// Gets the cipher block length (returns zero for stream ciphers)
int CCrypto::GetBlockLength(size_t alg)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < block_size_.size());

	get_info(alg);

	return block_size_[alg];
}

// Returns the key length in bits for this algorithm (the more the better)
int CCrypto::GetKeyLength(size_t alg)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < key_size_.size());

	get_info(alg);

	return key_size_[alg];
}

void CCrypto::get_info(size_t alg)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < csp_.size());
	ASSERT(algid_.size() == csp_.size());
	ASSERT(key_.size() == csp_.size());
	ASSERT(block_size_.size() == csp_.size());
	ASSERT(key_size_.size() == csp_.size());

	if (block_size_[alg] == -1)
	{
		// We have not yet worked out the cipher block size
		HCRYPTKEY hkey;                  // Temp key just used to get block length

		// Create a temporary key (with CryptGenKey) and get block length (with CryptGetKeyParam)
		DWORD len = sizeof(block_size_[alg]);
		DWORD len2 = sizeof(key_size_[alg]);
		if (!(pGenKey_(csp_[alg], algid_[alg], 0, &hkey)) ||
			!(pGetKeyParam_(hkey, KP_BLOCKLEN, (BYTE *)&block_size_[alg], &len, 0)) )
		{
			ASSERT(0);
			pDestroyKey_(hkey);
			block_size_[alg] = key_size_[alg] = -1;
			return;
		}
		key_size_[alg] = -1;                         // Use -1 for unknown (some CSPs return error)
		pGetKeyParam_(hkey, KP_KEYLEN, (BYTE *)&key_size_[alg], &len2, 0);
		pDestroyKey_(hkey);
		ASSERT(len == sizeof(block_size_[alg]));
		ASSERT(len2 == sizeof(key_size_[alg]));
	}
#ifdef _DEBUG
	else
	{
		HCRYPTKEY hkey;                  // Temp key just used to get block length
		DWORD block_size, key_size = -1;
		DWORD len = sizeof(block_size);
		DWORD len2 = sizeof(key_size);

		// Create a temporary key so we can get the block size using CryptGenKey
		VERIFY(pGenKey_(csp_[alg], algid_[alg], 0, &hkey));
		VERIFY(pGetKeyParam_(hkey, KP_BLOCKLEN, (BYTE *)&block_size, &len, 0));
		pGetKeyParam_(hkey, KP_KEYLEN, (BYTE *)&key_size, &len2, 0);  // Don't VERIFY as some CSPs return error
		pDestroyKey_(hkey);              // Destroy temp key (Don't VERIFY as some CSPs return garbage)
		ASSERT(len == sizeof(block_size));
		ASSERT(block_size == block_size_[alg]);
		ASSERT(len2 == sizeof(key_size));
		ASSERT(key_size == key_size_[alg]);
	}
#endif
}

// Space needed to encrypt len bytes (may be > len)
size_t CCrypto::needed(size_t alg, size_t len, bool final /*=true*/)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < key_.size());
	ASSERT(key_[alg] != HCRYPTKEY(0));

	DWORD reqd = len;               // Len to encrypted and length required

	if (!pEncrypt_(key_[alg], HCRYPTHASH(0), BOOL(final), 0, NULL, &reqd, len))
	{
		ASSERT(0);
		AfxMessageBox("Could not get encryption length needed using\n" + name_[alg]);
		return -1;
	}

	return reqd;
}

// Returns size of encrypted bytes (>= len)
size_t CCrypto::encrypt(size_t alg, BYTE *buf, size_t len, size_t buflen, bool final /*=true*/)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < key_.size());
	ASSERT(key_[alg] != HCRYPTKEY(0));

	DWORD reqd = len;               // Len to encrypted and length required

	if (!pEncrypt_(key_[alg], HCRYPTHASH(0), BOOL(final), 0, buf, &reqd, buflen))
	{
		ASSERT(0);
		return -1;
	}

	ASSERT(reqd <= buflen);
	return reqd;
}

// Returns size of encrypted bytes (<= len)
size_t CCrypto::decrypt(size_t alg, BYTE *buf, size_t len, bool final /*=true*/)
{
	if (hdll_ == HINSTANCE(0)) init();

	ASSERT(alg < key_.size());
	ASSERT(key_[alg] != HCRYPTKEY(0));

	DWORD reqd = len;               // Len to be decrypted and length returned

	if (!pDecrypt_(key_[alg], HCRYPTHASH(0), BOOL(final), 0, buf, &reqd))
	{
		return -1;
	}

	return reqd;
}
