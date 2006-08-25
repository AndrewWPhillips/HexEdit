// Crypto.h : header file for cryptography class
//
// Copyright (c) 2001 by Andrew W. Phillips.
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

#include <vector>
#if _MSC_VER < 1300
#define _WIN32_WINNT  0x0400  // To get stuff in wincrypt.h
#endif
#include <wincrypt.h>
#if _MSC_VER < 1300
#undef _WIN32_WINNT
#endif

class CCrypto
{
    typedef BOOL
        (__stdcall *PFAcquireContext)(
            HCRYPTPROV *phProv,
            LPCSTR pszContainer,
            LPCSTR pszProvider,
            DWORD dwProvType,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFReleaseContext)(
            HCRYPTPROV hProv,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFEnumProviders)(
            DWORD dwIndex,
            DWORD *pdwReserved,
            DWORD dwFlags,
            DWORD *pdwProvType,
            LPSTR pszProvName,
            DWORD *pcbProvName);
    typedef BOOL
        (__stdcall *PFGetProvParam)(
            HCRYPTPROV hProv,
            DWORD dwParam,
            BYTE *pbData,
            DWORD *pdwDataLen,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFContextAddRef)(
            HCRYPTPROV hProv,
            DWORD *pdwReserved,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFGetKeyParam)(
            HCRYPTKEY hKey,
            DWORD dwParam,
            BYTE *pbData,
            DWORD *pdwDataLen,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFDeriveKey)(
            HCRYPTPROV hProv,
            ALG_ID Algid,
            HCRYPTHASH hBaseData,
            DWORD dwFlags,
            HCRYPTKEY *phKey);
    typedef BOOL
        (__stdcall *PFGenKey)(
            HCRYPTPROV hProv,
            ALG_ID Algid,
            DWORD dwFlags,
            HCRYPTKEY *phKey);
    typedef BOOL
        (__stdcall *PFDestroyKey)(
            HCRYPTKEY hKey);
    typedef BOOL
        (__stdcall *PFEncrypt)(
            HCRYPTKEY hKey,
            HCRYPTHASH hHash,
            BOOL Final,
            DWORD dwFlags,
            BYTE *pbData,
            DWORD *pdwDataLen,
            DWORD dwBufLen);
    typedef BOOL
        (__stdcall *PFDecrypt)(
            HCRYPTKEY hKey,
            HCRYPTHASH hHash,
            BOOL Final,
            DWORD dwFlags,
            BYTE *pbData,
            DWORD *pdwDataLen);
    typedef BOOL
        (__stdcall *PFCreateHash)(
            HCRYPTPROV hProv,
            ALG_ID Algid,
            HCRYPTKEY hKey,
            DWORD dwFlags,
            HCRYPTHASH *phHash);
    typedef BOOL
        (__stdcall *PFHashData)(
            HCRYPTHASH hHash,
            CONST BYTE *pbData,
            DWORD dwDataLen,
            DWORD dwFlags);
    typedef BOOL
        (__stdcall *PFDestroyHash)(
            HCRYPTHASH hHash);

public:
    CCrypto();
    ~CCrypto();
    void init();

    size_t GetNum() { if (hdll_ == HINSTANCE(0)) init(); return name_.size(); }  // Number of algorithms available

    const char *GetName(size_t alg);  // Name of algorithm and CSP
    void SetPassword(size_t alg, const char *password = NULL);
    bool NeedsPassword(size_t alg) { if (hdll_ == HINSTANCE(0)) init(); return key_[alg] == HCRYPTKEY(0); }
    int GetBlockLength(size_t alg);     // Length of block (or zero for stream cipher)
    int GetKeyLength(size_t alg);       // Length of key

    size_t needed(size_t alg, size_t len, bool final = true);
    size_t encrypt(size_t alg, BYTE *buf, size_t len, size_t buflen, bool final = true);
    size_t decrypt(size_t alg, BYTE *buf, size_t len, bool final = true);

private:
    HINSTANCE hdll_;                    // Handle to ADVAPI32.DLL (0 if CryptoAPI not implemented)
    PFAcquireContext pAcquireContext_;  // Get handle to CSP
    PFReleaseContext pReleaseContext_;  // Free the CSP handle
    PFContextAddRef pContextAddRef_;    // Add 1 to ref count for CSP handle
    PFEnumProviders pEnumProviders_;    // Find all available CSPs
    PFGetProvParam pGetProvParam_;      // Get CSP info (used to get all algorithms provided by a CSP)
    PFDeriveKey pDeriveKey_;            // Creates an encryption key using a password
    PFGenKey pGenKey_;                  // Generate a random encryption key (only used to get block length)
    PFDestroyKey pDestroyKey_;          // Release key created with CryptDeriveKey (or whatever)
    PFGetKeyParam pGetKeyParam_;        // Get info about the key (stream or block, block length etc)
    PFEncrypt pEncrypt_;                // Encrypt some data
    PFDecrypt pDecrypt_;                // Decrypt some data
    PFCreateHash pCreateHash_;          // Create a hash (hash of password is needed for CryptDeriveKey)
    PFDestroyHash pDestroyHash_;        // Release hash handle
    PFHashData pHashData_;              // Create hash (of password)

    std::vector<CString> name_;         // Name of the algorithm (including CSP)
    std::vector<HCRYPTPROV> csp_;       // This is the CSP providing the algorithm
    std::vector<ALG_ID> algid_;         // Identifies the algorithm
    std::vector<HCRYPTKEY> key_;        // This is the key generated using the password
    // Note key_ is HCRYPTKEY(0) until a password has been supplied
    std::vector<DWORD> block_size_;     // This is the block size (in bits).  It's -1 if not yet worked out
    // For block algorithms you encrypt in multiples of the block size, for stream algs block_size_ == 0
    std::vector<DWORD> key_size_;       // Encryption key length in bits (the more the better encryption)

    void get_info(size_t alg);          // Get block and key lengths
};
