#include "stdafx.h"
#include "CFile64.h"
#include "SpecialList.h"
#include "hexedit.h"
#include "misc.h"
#include "ntapi.h"      // Our header for NT native API funcs/structures

#pragma hdrstop

/*
** Author: Samuel R. Blackburn
**
** $Workfile: CFile64.cpp $
** $Revision: 1 $
** $Modtime: 1/17/00 9:02a $
** $Reuse Tracing Code: 1001 $
*/

#if defined( _DEBUG ) && ! defined( WFC_STL )
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif // _DEBUG

#if ! defined( DIMENSION_OF )
#define DIMENSION_OF( argument ) ( sizeof( argument ) / sizeof( *( argument ) ) )
#endif

// Helper functions

static inline BOOL __IsDirSep(TCHAR ch)
{
   return (ch == '\\' || ch == '/');
}

static inline UINT __GetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
   // always capture the complete file name including extension (if present)
   LPTSTR lpszTemp = (LPTSTR)lpszPathName;

   for (LPCTSTR lpsz = lpszPathName; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
   {
      // remember last directory/drive separator
      if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
      {
         lpszTemp = (LPTSTR)_tcsinc(lpsz);
      }
   }

   // lpszTitle can be NULL which just returns the number of bytes
   if (lpszTitle == NULL)
   {
      return lstrlen(lpszTemp)+1;
   }

   // otherwise copy it into the buffer provided
   lstrcpyn(lpszTitle, lpszTemp, nMax);

   return( 0 );
}

static inline void __GetRoot( LPCTSTR lpszPath, CString& strRoot )
{
   ASSERT(lpszPath != NULL);
   // determine the root name of the volume

   TCHAR root_name[ _MAX_PATH ];

   LPTSTR lpszRoot = root_name;
   memset(lpszRoot, 0, _MAX_PATH);
   lstrcpyn(lpszRoot, lpszPath, _MAX_PATH);

   for ( LPTSTR lpsz = lpszRoot; *lpsz != '\0'; lpsz = _tcsinc( lpsz ) )
   {
      // find first double slash and stop
      if ( __IsDirSep( lpsz[ 0 ] ) && __IsDirSep( lpsz[ 1 ] ) )
      {
         break;
      }
   }

   if ( *lpsz != '\0' )
   {
      // it is a UNC name, find second slash past '\\'
      ASSERT( __IsDirSep( lpsz[ 0 ] ) );
      ASSERT( __IsDirSep( lpsz[ 1 ] ) );

      lpsz += 2;

      while ( *lpsz != '\0' && ( ! __IsDirSep( *lpsz ) ) )
      {
         lpsz = _tcsinc(lpsz);
      }

      if ( *lpsz != '\0' )
      {
         lpsz = _tcsinc( lpsz );
      }

      while ( *lpsz != '\0' && ( ! __IsDirSep( *lpsz ) ) )
      {
         lpsz = _tcsinc(lpsz);
      }

      // terminate it just after the UNC root (ie. '\\server\share\')

      if ( *lpsz != '\0' )
      {
         lpsz[ 1 ] = '\0';
      }
   }
   else
   {
      // not a UNC, look for just the first slash
      lpsz = lpszRoot;

      while ( *lpsz != '\0' && ( ! __IsDirSep(*lpsz ) ) )
      {
         lpsz = _tcsinc( lpsz );
      }

      // terminate it just after root (ie. 'x:\')
      if ( *lpsz != '\0' )
      {
         lpsz[ 1 ] = '\0';
      }
   }

   strRoot = root_name;
}

static inline BOOL __FullPath( LPTSTR lpszPathOut, LPCTSTR lpszFileIn )
{
   // first, fully qualify the path name
   LPTSTR lpszFilePart;

   if ( ! GetFullPathName( lpszFileIn, _MAX_PATH, lpszPathOut, &lpszFilePart ) )
   {
      lstrcpyn(lpszPathOut, lpszFileIn, _MAX_PATH); // take it literally
      return( FALSE );
   }

   CString strRoot;

   // determine the root name of the volume
   __GetRoot(lpszPathOut, strRoot);

   // get file system information for the volume

   DWORD dwFlags = 0;
   DWORD dwDummy = 0;

   if ( ! GetVolumeInformation(strRoot, NULL, 0, NULL, &dwDummy, &dwFlags, NULL, 0 ) )
   {
      return( FALSE );   // preserving case may not be correct
   }

   // not all characters have complete uppercase/lowercase

   if ( ! ( dwFlags & FS_CASE_IS_PRESERVED ) )
   {
      CharUpper( lpszPathOut );
   }

   // assume non-UNICODE file systems, use OEM character set

   if ( ! ( dwFlags & FS_UNICODE_STORED_ON_DISK ) )
   {
      WIN32_FIND_DATA data;

      HANDLE h = FindFirstFile( lpszFileIn, &data );

      if ( h != INVALID_HANDLE_VALUE )
      {
         FindClose( h );
         lstrcpy( lpszFilePart, data.cFileName );
      }
   }

   return( TRUE );
}

static inline UINT __GetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
   // use a temporary to avoid bugs in ::GetFileTitle when lpszTitle is NULL
   TCHAR szTemp[_MAX_PATH];
   LPTSTR lpszTemp = lpszTitle;

   if (lpszTemp == NULL)
   {
      lpszTemp = szTemp;
      nMax = DIMENSION_OF(szTemp);
   }

   if ( ::GetFileTitle( lpszPathName, lpszTemp, (WORD) nMax ) != 0 )
   {
      // when ::GetFileTitle fails, use cheap imitation
      return( __GetFileName( lpszPathName, lpszTitle, nMax ) );
   }

   return( ( lpszTitle == NULL ) ? _tcslen( lpszTemp ) + 1 : 0 );
}

CFile64::CFile64()
{
//   WFCLTRACEINIT( TEXT( "CFile64::CFile64()" ) );
   m_hFile = (UINT) hFileNull;
   m_FileHandle           = INVALID_HANDLE_VALUE;
   m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
   m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

   m_Initialize();
}

CFile64::CFile64( int file_handle )
{
//   WFCLTRACEINIT( TEXT( "CFile64::CFile64( int )" ) );

   m_hFile = (UINT) hFileNull;
   m_FileHandle           = INVALID_HANDLE_VALUE;
   m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
   m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

   m_Initialize();

   m_hFile         = file_handle; // Stupid public member that is never used internally
   m_FileHandle    = (HANDLE) file_handle;
   m_CloseOnDelete = FALSE;
}

CFile64::CFile64( LPCTSTR filename, UINT open_flags )
{
//   WFCLTRACEINIT( TEXT( "CFile64::CFile64( LPCTSTR )" ) );

   m_hFile = (UINT) hFileNull;
   m_FileHandle           = INVALID_HANDLE_VALUE;
   m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
   m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;

   m_Initialize();

#if ! defined( WFC_STL )

   CFileException exception;

   if ( Open( filename, open_flags, &exception ) == FALSE )
   {
      AfxThrowFileException( exception.m_cause, exception.m_lOsError, exception.m_strFileName );
   }

#else // WFC_STL

   Open( filename, open_flags );

#endif // WFC_STL
}

CFile64::~CFile64()
{
//   WFCLTRACEINIT( TEXT( "CFile64::~CFile64()" ) );

   if ( m_FileHandle != INVALID_HANDLE_VALUE && m_CloseOnDelete != FALSE )
   {
      Close();
   }

   m_Uninitialize();

   m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
   m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;
}

void CFile64::Abort( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Abort()" ) );

   BOOL return_value = TRUE;

   if ( m_FileHandle != INVALID_HANDLE_VALUE )
   {
      if (!::CloseHandle(m_FileHandle))
//      if ( ::wfc_close_handle( m_FileHandle ) != 0 )
      {
         return_value = FALSE;
      }

      m_FileHandle = INVALID_HANDLE_VALUE;
   }

   m_FileName.Empty();
   m_PathName.Empty();
   m_FileTitle.Empty();
   m_CloseOnDelete = FALSE;

   // Invalidate that stupid public attribute

   m_hFile = (UINT) hFileNull;
}

void CFile64::Close( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Close()" ) );

   BOOL return_value = TRUE;

   if ( m_FileHandle != INVALID_HANDLE_VALUE )
   {
      if (!::CloseHandle(m_FileHandle))
//      if ( ::wfc_close_handle( m_FileHandle ) != 0 )
      {
         return_value = FALSE;
      }

      m_FileHandle = INVALID_HANDLE_VALUE;
   }

   m_FileName.Empty();
   m_PathName.Empty();
   m_FileTitle.Empty();
   m_CloseOnDelete = FALSE;

   // Invalidate that stupid public attribute

   m_hFile = (UINT) hFileNull;

#if ! defined( WFC_STL )

   if ( return_value == FALSE )
   {
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
   }

#endif // WFC_STL
}

#if defined( _DEBUG ) && ! defined( WFC_NO_DUMPING )

void CFile64::Dump( CDumpContext& dump_context ) const
{
   dump_context << TEXT( "a CFile64 at " ) << (VOID *) this << TEXT( "\n{\n" );

   dump_context << TEXT( "   m_PathName             is \"" ) << m_PathName  << TEXT( "\"\n" );
   dump_context << TEXT( "   m_FileName             is \"" ) << m_FileName  << TEXT( "\"\n" );
   dump_context << TEXT( "   m_FileTitle            is \"" ) << m_FileTitle << TEXT( "\"\n" );
   dump_context << TEXT( "   m_CloseOnDelete        is "   );

   if ( m_CloseOnDelete == FALSE )
   {
      dump_context << TEXT( "FALSE" );
   }
   else
   {
      dump_context << TEXT( "TRUE" );
   }

   dump_context << TEXT( "\n" );

   dump_context << TEXT( "   m_SecurityAttributes_p is " );

   if ( m_SecurityAttributes_p != NULL )
   {
      dump_context << (VOID *) m_SecurityDescriptor_p;
      dump_context << TEXT( "\n   {\n" );
      dump_context << TEXT( "      nLength              is " ) << m_SecurityAttributes_p->nLength << TEXT( "\n" );
      dump_context << TEXT( "      lpSecurityDescriptor is " ) << m_SecurityAttributes_p->lpSecurityDescriptor << TEXT( "\n" );
      dump_context << TEXT( "      bInheritHandle       is " );

      if ( m_SecurityAttributes_p->bInheritHandle == FALSE )
      {
         dump_context << TEXT( "FALSE" );
      }
      else
      {
         dump_context << TEXT( "TRUE" );
      }

      dump_context << TEXT( "\n   }\n" );
   }
   else
   {
      dump_context << TEXT( "NULL" );
   }

   dump_context << TEXT( "\n" );

   dump_context << TEXT( "   m_SecurityDescriptor_p is " );

   if ( m_SecurityDescriptor_p != NULL )
   {
      dump_context << (VOID *) m_SecurityDescriptor_p;
   }
   else
   {
      dump_context << TEXT( "NULL" );
   }

   dump_context << TEXT( "\n" );

   dump_context << TEXT( "   m_FileHandle           is " );

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
      dump_context << TEXT( "INVALID_HANDLE_VALUE" );
   }
   else
   {
      dump_context << m_FileHandle;
   }

   dump_context << TEXT( "\n" );

   if ( dump_context.GetDepth() > 0 && m_FileHandle != INVALID_HANDLE_VALUE )
   {
      BY_HANDLE_FILE_INFORMATION information;

      ::GetFileInformationByHandle( m_FileHandle, &information );

      dump_context << TEXT( "\n   extended information from BY_HANDLE_FILE_INFORMATION\n   {\n" );
      dump_context << TEXT( "      dwFileAttributes     is " ) << information.dwFileAttributes << TEXT( " (" );

      CString attributes;

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE )
      {
         attributes = TEXT( "FILE_ATTRIBUTE_ARCHIVE" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_COMPRESSED" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_ENCRYPTED" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_DIRECTORY" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_HIDDEN" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_OFFLINE" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_READONLY" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_REPARSE_POINT" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_SPARSE_FILE" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_SYSTEM" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_TEMPORARY" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED" );
      }

      if ( information.dwFileAttributes & FILE_ATTRIBUTE_NORMAL )
      {
         if ( attributes.GetLength() > 0 )
         {
            attributes += TEXT( " | " );
         }

         attributes += TEXT( "FILE_ATTRIBUTE_NORMAL" );
      }

      dump_context << attributes << TEXT( ")\n" );

      attributes.Empty();

#if 0
      CFileTime file_time;

      CTime normal_time;

      LARGE_INTEGER large_integer;

      dump_context << TEXT( "      ftCreationTime       is " );

      large_integer.LowPart  = information.ftCreationTime.dwLowDateTime;
      large_integer.HighPart = information.ftCreationTime.dwHighDateTime;

      if ( large_integer.QuadPart == 0 )
      {
         dump_context << TEXT( " 0 (file system doesn't support this)" );
      }
      else
      {
         file_time.Copy( &information.ftCreationTime );
         file_time.CopyTo( normal_time );

         attributes = normal_time.Format( WFC_STANDARD_TIME_FORMAT );

         dump_context << attributes;
      }

      dump_context << TEXT( "\n" );

      dump_context << TEXT( "      ftLastAccessTime     is " );

      large_integer.LowPart  = information.ftLastAccessTime.dwLowDateTime;
      large_integer.HighPart = information.ftLastAccessTime.dwHighDateTime;

      if ( large_integer.QuadPart == 0 )
      {
         dump_context << TEXT( " 0 (file system doesn't support this)" );
      }
      else
      {
         file_time.Copy( &information.ftLastAccessTime );
         file_time.CopyTo( normal_time );

         attributes = normal_time.Format( WFC_STANDARD_TIME_FORMAT );

         dump_context << attributes;
      }

      dump_context << TEXT( "\n" );

      dump_context << TEXT( "      ftLastWriteTime      is " );

      large_integer.LowPart  = information.ftLastWriteTime.dwLowDateTime;
      large_integer.HighPart = information.ftLastWriteTime.dwHighDateTime;

      if ( large_integer.QuadPart == 0 )
      {
         dump_context << TEXT( " 0 (file system doesn't support this)" );
      }
      else
      {
         file_time.Copy( &information.ftLastWriteTime );
         file_time.CopyTo( normal_time );

         attributes = normal_time.Format( WFC_STANDARD_TIME_FORMAT );

         dump_context << attributes;
      }

      dump_context << TEXT( "\n" );

      dump_context << TEXT( "      dwVolumeSerialNumber is " ) << information.dwVolumeSerialNumber << TEXT( "(0x" );

      TCHAR temp_string[ 31 ];
      _ultot( information.dwVolumeSerialNumber, temp_string, 16 );

      dump_context << temp_string << TEXT( ")\n" );

      large_integer.LowPart  = information.nFileSizeLow;
      large_integer.HighPart = information.nFileSizeHigh;

      _ui64tot( large_integer.QuadPart, temp_string, 10 );

      dump_context << TEXT( "      nFileSize            is " ) << temp_string;
      dump_context << TEXT( " or MAKEDWORDLONG( " ) << large_integer.LowPart << TEXT( ", " ) << large_integer.HighPart << TEXT( " )\n" );

      dump_context << TEXT( "      nNumberOfLinks       is " ) << information.nNumberOfLinks << TEXT( "\n" );

      large_integer.LowPart  = information.nFileIndexLow;
      large_integer.HighPart = information.nFileIndexHigh;

      _ui64tot( large_integer.QuadPart, temp_string, 10 );

      dump_context << TEXT( "      nFileIndex           is " ) << temp_string;
      dump_context << TEXT( " (0x" );
      _ui64tot( large_integer.QuadPart, temp_string, 16 );
      dump_context << temp_string << TEXT( ")" );

      dump_context << TEXT( " or MAKEDWORDLONG( " ) << large_integer.LowPart;

      _ultot( large_integer.LowPart, temp_string, 16 );

      dump_context << TEXT( " (0x" ) << temp_string << TEXT( "), " ) << large_integer.HighPart << TEXT( " (0x" );
      
      _ultot( large_integer.HighPart, temp_string, 16 );

      dump_context << temp_string << TEXT( ") )\n" );

      dump_context << TEXT( "   }\n" );

      dump_context << TEXT( "\n File type is " );

      large_integer.LowPart = ::GetFileType( m_FileHandle );

      switch( large_integer.LowPart )
      {
         case FILE_TYPE_UNKNOWN:

            dump_context << TEXT( "FILE_TYPE_UNKNOWN" );
            break;

         case FILE_TYPE_DISK:

            dump_context << TEXT( "FILE_TYPE_DISK" );
            break;

         case FILE_TYPE_CHAR:

            dump_context << TEXT( "FILE_TYPE_CHAR" );
            break;

         case FILE_TYPE_PIPE:

            dump_context << TEXT( "FILE_TYPE_PIPE" );
            break;

         case FILE_TYPE_REMOTE:

            dump_context << TEXT( "FILE_TYPE_REMOTE" );
            break;

         default:

            dump_context << TEXT( "Undocumened value of " ) << large_integer.LowPart;
            break;
      }
#endif

      dump_context << TEXT( "\n" );
   }

   dump_context << TEXT( "}\n" );
}

#endif // _DEBUG

CFile64 * CFile64::Duplicate( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::Duplicate()" ) );

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
      return( (CFile64 *) NULL );
   }

   CFile64 * return_value = new CFile64;

   HANDLE duplicate_file_handle = INVALID_HANDLE_VALUE;

   if ( ::DuplicateHandle( ::GetCurrentProcess(), m_FileHandle, ::GetCurrentProcess(), &duplicate_file_handle, 0, FALSE, DUPLICATE_SAME_ACCESS ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );

      delete return_value;
      return_value = NULL;

#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }
   else
   {
      return_value->m_hFile         = (UINT) duplicate_file_handle; // Stupid public attribute that will never be used internally
      return_value->m_FileHandle    = duplicate_file_handle;
      return_value->m_CloseOnDelete = m_CloseOnDelete;
      return_value->m_PathName      = m_PathName;
      return_value->m_FileName      = m_FileName;
      return_value->m_FileTitle     = m_FileTitle;
   }

   return( return_value );
}

void CFile64::Flush( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Flush()" ) );

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
      return;
   }

   if ( ::FlushFileBuffers( m_FileHandle ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }
}

CString CFile64::GetFileName( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetFileName()" ) );
    UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
    CString strResult;
    AfxGetFileName(m_FileName, strResult.GetBuffer(_MAX_FNAME), _MAX_FNAME);
    strResult.ReleaseBuffer();
    return strResult;
}

CString CFile64::GetFilePath( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetFilePath()" ) );
   return( m_PathName );
}

CString CFile64::GetFileTitle( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetFileTitle()" ) );
   return( m_FileTitle );
}

HANDLE CFile64::GetHandle( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetHandle()" ) );
   return( m_FileHandle );
}

BOOL CFile64::GetInformation( BY_HANDLE_FILE_INFORMATION& information ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetInformation()" ) );

   if ( ::GetFileInformationByHandle( m_FileHandle, &information ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
      return( FALSE );
   }

   return( TRUE );
}

LONGLONG CFile64::GetLength( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetLength()" ) );

   // For fixed length (physical disk etc) return precalced size
   if (m_Length != -1)
      return m_Length;

   LARGE_INTEGER length;

   length.QuadPart = 0;

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
      return( length.QuadPart );
   }

   length.LowPart = ::GetFileSize( m_FileHandle, (DWORD *) &length.HighPart );

   if ( length.LowPart == 0xffffFFFF && ( ::GetLastError() != NO_ERROR ) )
   {
//      WFCTRACEERROR( ::GetLastError() );
      length.QuadPart = 0;
   }

   return( length.QuadPart );
}

LONGLONG CFile64::GetPosition( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetHandle()" ) );

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
//      WFCTRACE( TEXT( "File is not open." ) );
      return( (LONGLONG) -1 );
   }

   LARGE_INTEGER return_value;

   return_value.QuadPart = 0;

   return_value.LowPart = ::SetFilePointer( m_FileHandle, 0, &return_value.HighPart, FILE_CURRENT );

   if ( return_value.LowPart == 0xffffFFFF && ::GetLastError() != NO_ERROR )
   {
      // An Error Occurred
//      WFCTRACEERROR( ::GetLastError() );
//      WFCTRACE( TEXT( "Can't SetFilePointer() because of above error." ) );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#else // WFC_STL
      return_value.QuadPart = 0I64;
#endif // WFC_STL
   }

   return( return_value.QuadPart );
}

SECURITY_ATTRIBUTES * CFile64::GetSecurityAttributes( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetSecurityAttributes()" ) );
   return( m_SecurityAttributes_p );
}

SECURITY_DESCRIPTOR * CFile64::GetSecurityDescriptor( void ) const
{
//   WFCLTRACEINIT( TEXT( "CFile64::GetSecurityDescriptor()" ) );
   return( m_SecurityDescriptor_p );
}

#if ! defined( WFC_STL )

BOOL CFile64::GetStatus( CFileStatus& status ) const
{
   ZeroMemory( &status, sizeof( status ) );

   BY_HANDLE_FILE_INFORMATION information;

   if ( GetInformation( information ) == FALSE )
   {
      return( FALSE );
   }

   _tcsncpy( status.m_szFullName, m_FileName, DIMENSION_OF( status.m_szFullName ) );

   LARGE_INTEGER full_size;
   full_size.LowPart = information.nFileSizeLow;
   full_size.HighPart = information.nFileSizeHigh;
   status.m_size = full_size.QuadPart;

   if ( m_FileName.IsEmpty() )
   {
      status.m_attribute = 0;
   }
   else
   {
      status.m_attribute = (BYTE) information.dwFileAttributes;

      // convert times as appropriate
      status.m_ctime = CTime( information.ftCreationTime   );
      status.m_atime = CTime( information.ftLastAccessTime );
      status.m_mtime = CTime( information.ftLastWriteTime  );

      if ( status.m_ctime.GetTime() == 0 )
      {
         status.m_ctime = status.m_mtime;
      }

      if ( status.m_atime.GetTime() == 0 )
      {
         status.m_atime = status.m_mtime;
      }
   }

   return( TRUE );
}

BOOL PASCAL CFile64::GetStatus( LPCTSTR filename, CFileStatus& status )
{
   if ( ::__FullPath( status.m_szFullName, filename ) == FALSE )
   {
      status.m_szFullName[ 0 ] = TEXT( '\0' );
      return( FALSE );
   }

   WIN32_FIND_DATA find_data;

   HANDLE find_handle = ::FindFirstFile( (LPTSTR) filename, &find_data );

   if ( find_handle == INVALID_HANDLE_VALUE )
   {
      return( FALSE );
   }

   FindClose( find_handle );

   status.m_attribute = (BYTE) ( find_data.dwFileAttributes & ~FILE_ATTRIBUTE_NORMAL );

   LARGE_INTEGER full_size;
   full_size.LowPart = find_data.nFileSizeLow;
   full_size.HighPart = find_data.nFileSizeHigh;

   status.m_size = full_size.QuadPart;

   // convert times as appropriate
   status.m_ctime = CTime( find_data.ftCreationTime   );
   status.m_atime = CTime( find_data.ftLastAccessTime );
   status.m_mtime = CTime( find_data.ftLastWriteTime  );

   if ( status.m_ctime.GetTime() == 0 )
   {
      status.m_ctime = status.m_mtime;
   }

   if ( status.m_atime.GetTime() == 0 )
   {
      status.m_atime = status.m_mtime;
   }

   return( TRUE );
}

#endif // WFC_STL

__int64 PASCAL CFile64::GetSize( LPCTSTR file_name )
{
	// Get the file size
	CFileStatus fs;                 // Used to get file size
	VERIFY(CFile64::GetStatus(file_name, fs));

    return __int64(fs.m_size);
}

void CFile64::m_Initialize( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::m_Initialize()" ) );

   m_CloseOnDelete = FALSE;

   try
   {
      m_SecurityAttributes_p = new SECURITY_ATTRIBUTES;
   }
   catch( ... )
   {
      m_SecurityAttributes_p = NULL;
   }

   if ( m_SecurityAttributes_p == NULL )
   {
      return;
   }

   try
   {
      m_SecurityDescriptor_p = new SECURITY_DESCRIPTOR;
   }
   catch( ... )
   {
      m_SecurityDescriptor_p = NULL;
   }

   if ( m_SecurityDescriptor_p == NULL )
   {
      delete m_SecurityAttributes_p;
      m_SecurityAttributes_p = NULL;

      return;
   }

   if ( ::InitializeSecurityDescriptor( m_SecurityDescriptor_p, SECURITY_DESCRIPTOR_REVISION ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
//      WFCTRACE( TEXT( "InitializeSecurityDescriptor() failed with the above error." ) );

      delete m_SecurityAttributes_p;
      m_SecurityAttributes_p = NULL;

      delete m_SecurityDescriptor_p;
      m_SecurityDescriptor_p = NULL;

      return;
   }

   if ( ::SetSecurityDescriptorDacl( m_SecurityDescriptor_p, TRUE, NULL, FALSE ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
//      WFCTRACE( TEXT( "SetSecurityDescriptorDacl() failed with the above error." ) );

      delete m_SecurityAttributes_p;
      m_SecurityAttributes_p = NULL;

      delete m_SecurityDescriptor_p;
      m_SecurityDescriptor_p = NULL;

      return;
   }

   m_SecurityAttributes_p->nLength              = sizeof( SECURITY_ATTRIBUTES );
   m_SecurityAttributes_p->lpSecurityDescriptor = m_SecurityDescriptor_p;
   m_SecurityAttributes_p->bInheritHandle       = TRUE;
}

void CFile64::m_Uninitialize( void )
{
//   WFCLTRACEINIT( TEXT( "CMemoryFile::m_Uninitialize()" ) );

   if ( m_SecurityAttributes_p != NULL )
   {
      delete m_SecurityAttributes_p;
      m_SecurityAttributes_p = NULL;
   }

   if ( m_SecurityDescriptor_p != NULL )
   {
      delete m_SecurityDescriptor_p;
      m_SecurityDescriptor_p = NULL;
   }
}

void CFile64::LockRange( LONGLONG position, LONGLONG number_of_bytes_to_lock )
{
//   WFCLTRACEINIT( TEXT( "CFile64::LockRange()" ) );

   LARGE_INTEGER parameter_1;
   LARGE_INTEGER parameter_2;

   parameter_1.QuadPart = position;
   parameter_2.QuadPart = number_of_bytes_to_lock;

   if ( ::LockFile( m_FileHandle, parameter_1.LowPart, parameter_1.HighPart, parameter_2.LowPart, parameter_2.HighPart ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }
}

#if ! defined( WFC_STL )
BOOL CFile64::Open( LPCTSTR filename, UINT open_flags, CFileException * exception_p )
#else // WFC_STL
BOOL CFile64::Open( LPCTSTR filename, UINT open_flags )
#endif // WFC_STL
{
//   WFCLTRACEINIT( TEXT( "CFile64::Open()" ) );

   Close();

   try
   {
      m_FileName = filename;

      TCHAR full_path[ _MAX_PATH ];

      ::__FullPath( full_path, filename );

      m_PathName = full_path;

      TCHAR file_title[ _MAX_FNAME ];

      if ( ::__GetFileTitle( full_path, file_title, _MAX_FNAME ) == 0 )
      {
         m_FileTitle = file_title;
      }
      else
      {
         m_FileTitle.Empty();
      }

      open_flags &= ~ (UINT) typeBinary;

      DWORD access = 0;

      switch ( open_flags & 3 )
      {
         case modeRead:

            access = GENERIC_READ;
            break;

         case modeWrite:

            access = GENERIC_WRITE;
            break;

         case modeReadWrite:

            access = GENERIC_READ | GENERIC_WRITE;
            break;

         default:

            ASSERT( FALSE );  // invalid share mode
      }

      DWORD share_mode = 0;

      switch ( open_flags & 0x70 )    // map compatibility mode to exclusive
      {
         case shareCompat:
         case shareExclusive:

            share_mode = 0;
            break;

         case shareDenyWrite:

            share_mode = FILE_SHARE_READ;
            break;

         case shareDenyRead:

            share_mode = FILE_SHARE_WRITE;
            break;

         case shareDenyNone:

            share_mode = FILE_SHARE_WRITE|FILE_SHARE_READ;
            break;

         default:

            ASSERT( FALSE );  // invalid share mode?
      }

      if ( m_SecurityAttributes_p != NULL )
      {
         m_SecurityAttributes_p->bInheritHandle = ( ( open_flags & modeNoInherit ) == 0 ) ? TRUE : FALSE;
      }

      DWORD creation_flags = 0;

      if ( open_flags & modeCreate )
      {
         if ( open_flags & modeNoTruncate )
         {
            creation_flags = OPEN_ALWAYS;
         }
         else
         {
            creation_flags = CREATE_ALWAYS;
         }
      }
      else
      {
         creation_flags = OPEN_EXISTING;
      }

	  DWORD flags = FILE_ATTRIBUTE_NORMAL;

	  if ( open_flags & CFile64::osNoBuffer )
		  flags |= FILE_FLAG_NO_BUFFERING;

      m_FileHandle = ::CreateFile( filename,
                                   access,
                                   share_mode,
                                   GetSecurityAttributes(),
                                   creation_flags,
                                   flags,
                                   NULL );

      if ( m_FileHandle == INVALID_HANDLE_VALUE )
      {
//         WFCTRACEERROR( ::GetLastError() );

#if ! defined( WFC_STL )

         if ( exception_p != NULL )
         {
            exception_p->m_lOsError    = ::GetLastError();
            exception_p->m_cause       = CFileException::OsErrorToException( exception_p->m_lOsError );
            exception_p->m_strFileName = filename;
         }

#endif // WFC_STL

         Close();
         return( FALSE );
      }

      m_hFile = (UINT) m_FileHandle; // Set the stupid public member that is never used internally

      m_CloseOnDelete = TRUE;
   }
   catch( ... )
   {
      return( FALSE );
   }

	// Set length to -1 (to force recalc. whenever asked for).  Derived class (CFileNC)
    // will calculate this value later for devices (which never change in length).
	m_Length = -1;

	// Note: If a device then derived class CFileNC has to be used in which case CFileNC::Open
	//       calculates the sector size.
	// This means m_SectorSize is calced twice if CFileNC is used to open an ordinary file!!
	if (!::IsDevice(filename))
	{
		// Work out the sector size
		_TCHAR fullname[1024];
		_TCHAR *FilePart;
		VERIFY(::GetFullPathName(filename, sizeof(fullname), fullname, &FilePart));

		ASSERT(fullname[1] == ':' && fullname[2] == '\\');
		fullname[3] = '\0';

		// Get bytes per sector (parameter 2 to GetDiskFreeSpace)
		DWORD SectorsPerCluster;
		DWORD NumberOfFreeClusters;
		DWORD TotalNumberOfClusters;

		VERIFY(::GetDiskFreeSpace(fullname,
								&SectorsPerCluster,
								&m_SectorSize,
								&NumberOfFreeClusters,
								&TotalNumberOfClusters));
	}

	return TRUE;
}

DWORD CFile64::Read( CByteArray& buffer, DWORD number_of_bytes_to_read )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Read( CByteArray )" ) );

   buffer.SetSize( number_of_bytes_to_read, 8192 );

   return( Read( (void *) buffer.GetData(), buffer.GetSize() ) );
}

DWORD CFile64::Read( void * buffer, DWORD number_of_bytes_to_read )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Read( void * )" ) );
//   WFCTRACEVAL( TEXT( "Reading " ), number_of_bytes_to_read );

#if defined( _DEBUG )
   if ( number_of_bytes_to_read == 1 )
   {
//      WFC_COVERAGE( 29 );
   }
#endif // _DEBUG

   if ( number_of_bytes_to_read == 0 )
   {
      return( 0 );
   }

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
      return( 0 );
   }

   DWORD number_of_bytes_read = 0;

   if ( ::ReadFile( m_FileHandle, buffer, number_of_bytes_to_read, &number_of_bytes_read, NULL ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
//      WFCTRACE( TEXT( "Can't read from file because of above error." ) );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }

   return( number_of_bytes_read );
}

DWORD CFile64::ReadHuge( void * buffer, DWORD number_of_bytes_to_read )
{
   return( Read( buffer, number_of_bytes_to_read ) );
}

void PASCAL CFile64::Rename( LPCTSTR old_name, LPCTSTR new_name )
{
   if ( ::MoveFile( (LPTSTR) old_name, (LPTSTR) new_name ) == FALSE )
   {
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }
}

void PASCAL CFile64::Remove( LPCTSTR filename )
{
   if ( ::DeleteFile( (LPTSTR) filename ) == FALSE )
   {
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG)::GetLastError() );
#endif // WFC_STL
   }
}

LONGLONG CFile64::Seek( LONGLONG offset, UINT from )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Seek()" ) );

   if ( m_FileHandle == INVALID_HANDLE_VALUE )
   {
//      WFCTRACE( TEXT( "File is not open." ) );
      return( (LONGLONG) -1 );
   }

   DWORD move_method = 0;

   switch( from )
   {
     case CFile::begin:

//        WFCTRACEVAL( TEXT( "From beginning to " ), offset );
        move_method = FILE_BEGIN;
        break;

     case CFile::current:

//        WFCTRACEVAL( TEXT( "From current to " ), offset );
        move_method = FILE_CURRENT;
        break;

     case CFile::end:

//        WFCTRACEVAL( TEXT( "From end to " ), offset );
        move_method = FILE_END;
        break;

     default:

//        WFCTRACEVAL( TEXT( "Unknown from position (it wasn't CFile64::begin, CFile64::current or CFile64::end " ), from );
        return( (LONGLONG) -1 );
   }

   LARGE_INTEGER return_value;

   return_value.QuadPart = offset;

   return_value.LowPart = ::SetFilePointer( m_FileHandle, return_value.LowPart, &return_value.HighPart, move_method );

   if ( return_value.LowPart == 0xffffFFFF && ::GetLastError() != NO_ERROR )
   {
//      WFCTRACEERROR( ::GetLastError() );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }

   return( return_value.QuadPart );
}

void CFile64::SeekToBegin( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::SeekToBegin()" ) );
   Seek( (LONGLONG) 0, CFile64::begin );
}

LONGLONG CFile64::SeekToEnd( void )
{
//   WFCLTRACEINIT( TEXT( "CFile64::SeekToEnd()" ) );
   return( Seek( (LONGLONG) 0, CFile64::end ) );
}

BOOL CFile64::SetEndOfFile( LONGLONG length )
{
//   WFCLTRACEINIT( TEXT( "CFile64::SetEndOfFile()" ) );

   if ( Seek( length, CFile::begin ) == FALSE )
   {
//      WFCTRACE( TEXT( "Can't seek." ) );
      return( FALSE );
   }

   if ( ::SetEndOfFile( m_FileHandle ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
//      WFCTRACE( TEXT( "Can't set end of file because of above error." ) );
      return( FALSE );
   }

   return( TRUE );
}

void CFile64::SetFilePath( LPCTSTR new_name )
{
   m_FileName = new_name;
}

void CFile64::SetLength( LONGLONG new_length )
{
   if ( SetEndOfFile( new_length ) == FALSE )
   {
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG)::GetLastError() );
#endif // WFC_STL
   }
}

#if ! defined( WFC_STL )

void PASCAL CFile64::SetStatus( LPCTSTR filename, const CFileStatus& status )
{
   CFile::SetStatus( filename, status );
}

#endif // WFC_STL

void CFile64::UnlockRange( LONGLONG position, LONGLONG number_of_bytes_to_unlock )
{
//   WFCLTRACEINIT( TEXT( "CFile64::UnlockRange()" ) );

   LARGE_INTEGER parameter_1;
   LARGE_INTEGER parameter_2;

   parameter_1.QuadPart = position;
   parameter_2.QuadPart = number_of_bytes_to_unlock;

   if ( ::UnlockFile( m_FileHandle, parameter_1.LowPart, parameter_1.HighPart, parameter_2.LowPart, parameter_2.HighPart ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError() );
#endif // WFC_STL
   }
}

void CFile64::Write( const void * buffer, DWORD number_of_bytes_to_write )
{
//   WFCLTRACEINIT( TEXT( "CFile64::Write()" ) );

   if ( number_of_bytes_to_write == 0 )
   {
      return;
   }

   DWORD number_of_bytes_written = 0;

   if ( ::WriteFile( m_FileHandle, buffer, number_of_bytes_to_write, &number_of_bytes_written, NULL ) == FALSE )
   {
//      WFCTRACEERROR( ::GetLastError() );
#if ! defined( WFC_STL )
      CFileException::ThrowOsError( (LONG) ::GetLastError(), m_FileName );
#endif // WFC_STL
   }

#if ! defined( WFC_STL )
   if ( number_of_bytes_written != number_of_bytes_to_write )
   {
      ::AfxThrowFileException( CFileException::diskFull, -1, m_FileName );
   }
#endif // WFC_STL
}

void CFile64::WriteHuge( const void * buffer, DWORD number_of_bytes_to_write )
{
   Write( buffer, number_of_bytes_to_write );
}

// Operators

CFile64::operator HFILE ( void ) const
{
   return( (HFILE) m_FileHandle );
}

///////////////////////////////////////////////////////////////////////////
// CFileNC: used to access file in non-cached (FILE_FLAG_NO_BUFFERING) mode.

// Note: Even though the code for this constructor is exactly the same as for
// the base class (CFile64) constructor we have to override so that we call
// the derived version of Open().
CFileNC::CFileNC( LPCTSTR filename, UINT open_flags )
{
	m_Buffer = NULL;
	m_hFile = (UINT) hFileNull;
	m_FileHandle           = INVALID_HANDLE_VALUE;
	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;
	m_dirty = false;

	m_Initialize();

#if ! defined( WFC_STL )

	CFileException exception;

	if ( Open( filename, open_flags, &exception ) == FALSE )
	{
		AfxThrowFileException( exception.m_cause, exception.m_lOsError, exception.m_strFileName );
	}

#else // WFC_STL

	Open( filename, open_flags );

#endif // WFC_STL
}

// CFileNC::Open handles several different types of filenames:
// 0. NTAPI device name (\device\*) - these are no longer passed in as the filename
// 1. Fake device name - NT/2K/XP only - converted to NTAPI device name: \\.\FloppyN => \device\FloppyN,
//    \\.\CdRomN => \device\CdromN, \\.\PhysicalDriveN => \device\HarddiskN\Partition0 where N=0,1,2,...
// 2. Volume name (\\.\D:) under Win 9X - accessed through vwin32.vxd (\\.\vwin32)
// 3. Volume name under NT/2K/XP - opened as normal file using volume name
// 4. Ordinary file which we want opened non-cached

// Notes on working out "file" length (m_Length)
// 1. For device names (NT etc only)
//    A. Calculating from IOCTL_DISK_GET_DRIVE_GEOMETRY gives too small a value (does not account for reserved sectors?)
//       - IOCTL_DISK_GET_DRIVE_GEOMETRY does not work for CD/DVD except under XP but IOCTL_CDROM_GET_DRIVE_GEOMETRY works fine
//    B. IOCTL_DISK_GET_LENGTH_INFO is supposed to return correct length but is only available under XP
//       - IOCTL_DISK_GET_LENGTH_INFO works perfectly for hard disks (\device\HarddiskN\Partition0)
//       - IOCTL_DISK_GET_LENGTH_INFO does not work at all for floppies (\device\FloppyN)
//       - IOCTL_DISK_GET_LENGTH_INFO generally gives too large a value for CD/DVD, but may be OS/driver specific problem?
//    C. GetDiskFreeSpaceEx seems to always work perfectly for CD/DVD
//       - but to use we need to work out "DOS" volume name from device name (can be done with QueryDosDevice)
//       - doc. says value may be less if quotas are in use
//    D. We can't use NtQueryInformationFile(...., FileStandardInformation) as it does not appear to work for devices
//    Strategy:
//       XP:
//          floppy: use IOCTL_DISK_GET_DRIVE_GEOMETRY
//          optical: IOCTL_DISK_GET_LENGTH_INFO then read last sector
//             if error reading last sector use GetDiskFreeSpaceEx then read next sector
//                if can read next sector scan for actual end
//          hard disk: IOCTL_DISK_GET_LENGTH_INFO
//       W2K/NT: use IOCTL_DISK_GET_DRIVE_GEOMETRY
//          if not floppy scan for actual end
// 2. For volumes (9X)
//    use GetDiskFreeSpaceEx
// 3. For volumes (NT etc) GetDiskFreeSpaceEx reports correct size for fixed and optical drives but
//    gives a smaller size for floppy and flash drives.  Possibilities:
//    - scan for end (currently done and seems to work)
//    - use physical device size as above (1) biut how to get it and what happens for mult. partn drives?
//    xxx check removeable drive with multiple partitions xxx

#if ! defined( WFC_STL )
BOOL CFileNC::Open( LPCTSTR filename, UINT open_flags, CFileException * exception_p )
{
#else // WFC_STL
BOOL CFileNC::Open( LPCTSTR filename, UINT open_flags )
{
#endif
	m_hFile = (UINT) hFileNull;
	m_FileHandle           = INVALID_HANDLE_VALUE;
	m_SecurityAttributes_p = (SECURITY_ATTRIBUTES *) NULL;
	m_SecurityDescriptor_p = (SECURITY_DESCRIPTOR *) NULL;
    bool scan_for_end = false;                        // Do we need to scan for the real end of file/device?

	GetNTAPIFuncs();
	m_retries = -1;
	if (theApp.is_nt_ && _tcslen(filename) > 6 && ::IsDevice(filename))
		m_retries = 1;                                // +ve value is also used to indicate that this is a physical device

	// Get a suitably aligned buffer
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_BufferSize = si.dwPageSize * 2;

	m_Buffer = VirtualAlloc(NULL, m_BufferSize, MEM_COMMIT, PAGE_READWRITE);

	// Check if this is a physical device (under NT)
	if (m_retries >= 0)
	{
		// 1. PHYSICAL DEVICE (NT)
		bool try_vol_size = false;      // Try using vol size (GetDiskFreeSpaceEx) - only use where device=volume (probably just CD/DVD)

		ASSERT(hNTDLL != (HINSTANCE)0);
		OBJECT_ATTRIBUTES oa;
		IO_STATUS_BLOCK iosb;
		UNICODE_STRING us;
		BSTR name = GetPhysicalDeviceName(filename);
		(*pfInitUnicodeString)(&us, name);
		::SysFreeString(name);

		oa.Length = sizeof(oa);
		oa.RootDirectory = NULL;
		oa.ObjectName = &us;
		oa.Attributes = OBJ_CASE_INSENSITIVE;
		oa.SecurityDescriptor = NULL;
		oa.SecurityQualityOfService = NULL;

		// Handle open flags
		ACCESS_MASK DesiredAccess = FILE_READ_DATA;
		if ((open_flags & modeWrite) != 0 || (open_flags & modeReadWrite) != 0)
			DesiredAccess = FILE_READ_DATA|FILE_WRITE_DATA;
		ULONG ShareAccess = 0;
		switch (open_flags & 0xF0)
		{
		case shareExclusive:
			ShareAccess = 0;
			break;
		case shareDenyWrite:
			ShareAccess = FILE_SHARE_READ;
			break;
		case shareDenyRead:
			ShareAccess = FILE_SHARE_WRITE;
			break;
		case shareDenyNone:
			ShareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
			break;
		}

		if ((*pfOpenFile)(&m_FileHandle, DesiredAccess|SYNCHRONIZE, &oa, &iosb, ShareAccess,
			              FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_ALERT) != STATUS_SUCCESS)
		{
			m_FileHandle = INVALID_HANDLE_VALUE;
			return FALSE;
		}

		// Open does not fail if no disk present so check that too
		if ((*pfDeviceIoControlFile)(m_FileHandle, 0, 0, 0, &iosb, IOCTL_STORAGE_CHECK_VERIFY, 0, 0, 0, 0) != STATUS_SUCCESS)
		{
			(*pfClose)(m_FileHandle);
			m_FileHandle = INVALID_HANDLE_VALUE;
			return FALSE;
		}

        m_FileName = filename;
		m_PathName = filename;
        m_hFile = (UINT) m_FileHandle; // Set the stupid public member that is never used internally
        m_CloseOnDelete = TRUE;

		// Get disk geometry for m_SectorSize (and in some cases it is also used for m_Length)
		DISK_GEOMETRY dg;             // results from IOCTL_DISK_GET_DRIVE_GEOMETRY
		dg.BytesPerSector = 0;
        // Note: IOCTL_DISK_GET_DRIVE_GEOMETRY does not work for CD (except XP) use IOCTL_CDROM_GET_DRIVE_GEOMETRY
        if (_tcsncmp(filename, _T("\\\\.\\CdRom"), 9) != 0)
            VERIFY((*pfDeviceIoControlFile)(m_FileHandle, 0, 0, 0, &iosb, IOCTL_DISK_GET_DRIVE_GEOMETRY, 0, 0, &dg, sizeof(dg)) == STATUS_SUCCESS);
        else
    		VERIFY((*pfDeviceIoControlFile)(m_FileHandle, 0, 0, 0, &iosb, IOCTL_CDROM_GET_DRIVE_GEOMETRY, 0, 0, &dg, sizeof(dg)) == STATUS_SUCCESS);

		// Use this as an estimate until/unless we find something better
		m_Length = dg.Cylinders.QuadPart * dg.TracksPerCylinder * (LONGLONG)dg.SectorsPerTrack * dg.BytesPerSector;
		m_SectorSize = dg.BytesPerSector;

		// Work out m_Length
		if (_tcsncmp(filename, _T("\\\\.\\Floppy"), 10) == 0)
		{
			// nothing here (IOCTL_DISK_GET_DRIVE_GEOMETRY works)
		}
		else if (theApp.is_xp_)
		{
			GET_LENGTH_INFORMATION gli;
		    VERIFY((*pfDeviceIoControlFile)(m_FileHandle, 0, 0, 0, &iosb, IOCTL_DISK_GET_LENGTH_INFO, 0, 0, &gli, sizeof(gli)) == STATUS_SUCCESS);
			m_Length = gli.Length.QuadPart;

			// Now make sure we can read the last sector on CD
			if (_tcsncmp(filename, _T("\\\\.\\CdRom"), 9) == 0)
			{
				// Some optical drives return a value slightly too big but using the volume size works OK
				LARGE_INTEGER pos;
				pos.QuadPart = m_Length - m_SectorSize;   // try reading last sector
				NTSTATUS ns = (*pfReadFile)(m_FileHandle, 0, 0, 0, &iosb, (char *)m_Buffer, m_SectorSize, &pos, 0);
				if (ns == STATUS_END_OF_FILE || ns == STATUS_IO_DEVICE_ERROR || ns == STATUS_NONEXISTENT_SECTOR)
					try_vol_size = true;
			}
		}
		else if (_tcsncmp(filename, _T("\\\\.\\CdRom"), 9) == 0)
		{
			try_vol_size = true;
		}
		else
		{
			ASSERT(_tcsncmp(filename, _T("\\\\.\\PhysicalDrive"), 17) == 0);
			scan_for_end = true;
		}

		// See if using the volume size gives a better value
		if (try_vol_size)
 		{
			// Default to something that is absolute minimum possible size
			m_Length = dg.Cylinders.QuadPart * dg.TracksPerCylinder * (LONGLONG)dg.SectorsPerTrack * dg.BytesPerSector;

			// Get mounted volume associated with this device
			int volid = ::DeviceVolume(filename);
			if (volid != -1)
			{
				// Get total bytes of the volume
				char vol[4] = "?:\\";
				vol[0] = 'A' + (char)volid;
				ULARGE_INTEGER total_bytes;
				total_bytes.QuadPart = 0;
				if (::GetDiskFreeSpaceEx(vol, NULL, &total_bytes, NULL) &&      // may fail - err 5 (access denied)
				    total_bytes.QuadPart > (ULONGLONG)m_Length)                 // m_Length is our minimum
				{
					m_Length = total_bytes.QuadPart;
				}
			}

			// Check if we can read the sector after the end in which case we need to scan past this position
			LARGE_INTEGER pos;
			pos.QuadPart = m_Length;         // try reading next sector
			NTSTATUS ns = (*pfReadFile)(m_FileHandle, 0, 0, 0, &iosb, (char *)m_Buffer, m_SectorSize, &pos, 0);
			if (ns != STATUS_END_OF_FILE && ns != STATUS_IO_DEVICE_ERROR && ns != STATUS_NONEXISTENT_SECTOR)
				scan_for_end = true;         // we could read it so scan forward until we find one we can't read
		}

		ASSERT(m_retries >= 0);   // make sure we indicate that it is a physical device
	}
	else if (!theApp.is_nt_ && ::IsDevice(filename))
    {
		// 2. VOLUME (9X) - eg "\\.\D:" - open using Win95 "vwin32.vxd" to access the volume
		ASSERT(_tcslen(filename) == 6);
        Close();

		// xxx Handle open flags

        // Open the VXD which gives access to the disk sectors under Windows 9X
		m_FileHandle = ::CreateFile("\\\\.\\vwin32", 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (m_FileHandle == INVALID_HANDLE_VALUE)
			return FALSE;

        m_FileName = filename;
        m_hFile = (UINT) m_FileHandle; // Set the stupid public member that is never used internally
        m_CloseOnDelete = TRUE;

		// We call GetDiskFreeSpace(Ex) to work out the sector size and file length.
		DWORD SectorsPerCluster;
		DWORD NumberOfFreeClusters;
		DWORD TotalNumberOfClusters;
		VERIFY(::GetDiskFreeSpace(filename,				// This fails if called after device opened under XP
								&SectorsPerCluster,
								&m_SectorSize,
								&NumberOfFreeClusters,
								&TotalNumberOfClusters));
		ULARGE_INTEGER bytes_total;
		if (::GetDiskFreeSpaceEx(filename, NULL, &bytes_total, NULL))
			m_Length = bytes_total.QuadPart;
		else
			m_Length = (LONGLONG)m_SectorSize * SectorsPerCluster * (LONGLONG)TotalNumberOfClusters;

		m_PathName = filename;

		// Do we need to scan_for_end for volumes? xxx
    }
	else if (theApp.is_nt_ && ::IsDevice(filename))
	{
		// 3. VOLUME (NT) - eg "\\.\D:" - open using this special name to access raw bytes
		ASSERT(_tcslen(filename) == 6 && filename[5] == ':');
		_TCHAR vol[4] = _T("?:\\");     // Path of root dir to pass to GetDiskFreeSpace
		vol[0] = filename[4];

		// We call GetDiskFreeSpace(Ex) to work out the file length.
		// Note: The value from GetDiskFreeSpaceEx is preferred as it can handle larger file sizes but
		//       we fall back to GetDiskFreeSpace if it does not work (probably only under Win 95 < OSR2).
		DWORD SectorsPerCluster;
		DWORD NumberOfFreeClusters;
		DWORD TotalNumberOfClusters;
		if (!::GetDiskFreeSpace(vol,				// This fails if called after device opened under XP
							   &SectorsPerCluster,
							   &m_SectorSize,
							   &NumberOfFreeClusters,
							   &TotalNumberOfClusters))
		{
			// Must be corrupt volume
#if ! defined( WFC_STL )
			if (exception_p != NULL)
			{
				exception_p->m_lOsError    = ::GetLastError();
				if (exception_p->m_lOsError != ERROR_UNRECOGNIZED_VOLUME)
					exception_p->m_cause = CFileException::OsErrorToException( exception_p->m_lOsError );
				else
					exception_p->m_cause = 0xDEAD; // CFileException enum (currently) only goes up to 14 so this special value should be safe
				exception_p->m_strFileName = filename;
			}
#endif
			return FALSE;
		}

		ULARGE_INTEGER bytes_total;
		if (!::GetDiskFreeSpaceEx(vol, NULL, &bytes_total, NULL))
			bytes_total.QuadPart = 0;               // Signal that it failed

		// Volume accessed through NT using volume name
#if ! defined( WFC_STL )
        ASSERT((open_flags & modeCreate) == 0);
        if (!CFile64::Open( filename, open_flags|CFile64::osNoBuffer, exception_p ))
		    return FALSE;
#else // WFC_STL
	    if (!CFile64::Open( filename, open_flags|CFile64::osNoBuffer ))
		    return FALSE;
#endif // WFC_STL

		// Set file length after calling base class Open (which sets it to -1)
		if (bytes_total.QuadPart == 0)
			m_Length = (LONGLONG)m_SectorSize * SectorsPerCluster * (LONGLONG)TotalNumberOfClusters;
		else
			m_Length = bytes_total.QuadPart; // xxx wrong for \\.\A: and flash disk under XP, OK for CD

		scan_for_end = true;
	}
	else
	{
		// 4. Normal file (opened in non-cached mode)
#if ! defined( WFC_STL )
        ASSERT((open_flags & modeCreate) == 0);
        if (!CFile64::Open( filename, open_flags|CFile64::osNoBuffer, exception_p ))
		    return FALSE;
#else // WFC_STL
	    if (!CFile64::Open( filename, open_flags|CFile64::osNoBuffer ))
		    return FALSE;
#endif // WFC_STL
	}

	ASSERT(m_BufferSize % (m_SectorSize*2) == 0);   // buffer must be multiple of double sector size

#ifdef _DEBUG
	LONGLONG saved_length = m_Length;
	// Always scan in debug (but don't change scan_for_end so the assertion below works)
	if (true)
#else
    if (scan_for_end)
#endif
    {
		// Now we need to scan for the real end since the size does not include reserved
		// sectors stored at the start of the device (boot sector/MBR + empty sectors).
		// The algorithm doubles the amount it scans out until it finds a sector it can' read,
		// then it does a binary search between the last sectors it could and couldn't read.
		LONGLONG last_good = m_Length - m_SectorSize; // address of first byte of the last sector that we know we can read
		LONGLONG increment = m_SectorSize;
		bool bad_found = false;                       // have we encountered a sector we couldn't read yet?
		for (;;)
		{
			LARGE_INTEGER pos;
			pos.QuadPart = last_good + increment;       // next sector to try and read
			if (m_retries < 0)
				pos.LowPart = ::SetFilePointer(m_FileHandle, pos.LowPart, &pos.HighPart, FILE_BEGIN);
			ASSERT(pos.QuadPart == last_good + increment);

		    DWORD num_read;
			if (m_retries < 0)  // Check if normal file handle or Native
			{
				if (::ReadFile(m_FileHandle, (char *)m_Buffer, m_SectorSize, &num_read, NULL) && num_read == m_SectorSize)
					last_good += increment;    // Store that pos was readable
				else
					bad_found = true;
			}
			else
			{
				IO_STATUS_BLOCK iosb;
				NTSTATUS ns = (*pfReadFile)(m_FileHandle, 0, 0, 0, &iosb, m_Buffer, m_SectorSize, &pos, 0);
				if (ns == STATUS_END_OF_FILE || ns == STATUS_IO_DEVICE_ERROR || ns == STATUS_NONEXISTENT_SECTOR)
					bad_found = true;
				else
					last_good += increment;    // Remember that sector at pos was read OK
			}

			ASSERT(increment >= m_SectorSize);
			if (last_good >= 0x1000000000000000)  // How can it be this big???
				goto scan_failed;          // Read past end did not fail! - just stick with the m_Length we got
			else if (!bad_found)
				increment *= 2;            // Go out twice as far next time
			else if (increment > m_SectorSize)
				increment /= 2;            // Split the range we know it is in
			else
				break;                     // when increment == sector size we must be there
		}
		m_Length = last_good + m_SectorSize; // First byte of the 1st (lowest address) sector we could not read
	}
scan_failed:

	// Check that if not scanning for end (in release build) then m_Length is correct
	ASSERT(m_Length == saved_length || scan_for_end);

	m_FilePos = 0;                 // Begin at start of file
	m_start = m_end = 0;           // Indicate that buffer contains nothing currently
	m_dirty = false;
	return TRUE;
}

// Normally file name inc. ext. - for devices an extension is fabricated depending on filesystem in use (or RAW for none)
CString CFileNC::GetFileName( void ) const
{
	// If a device file create a special file name
	if (!::IsDevice(m_FileName))
		return CFile64::GetFileName();

	CString retval;
#if defined(BG_DEVICE_SEARCH) || defined(_DEBUG)
	CSpecialList *psl = theApp.GetSpecialList();
	int idx = psl->find(m_FileName);
	if (idx == -1)
		retval = m_FileName;
	else
	{
		retval = psl->name(idx);
		if (psl->type(idx) == 0)
		{
			if (!psl->FileSystem(idx).IsEmpty())
				retval += "." + psl->FileSystem(idx);
		}
		else
			retval += ".RAW";
	}
#else
	retval = ::DeviceName(m_FileName);
	// If logical volume add the filesystem name as the "file extension"
	if (_tcslen(m_FileName) == 6)
	{
		// xxx test this under NT and Windows 95
		// Disk volume - file name of form <\\.\D:>
		_TCHAR vol[4] = _T("?:\\");     // Path of root dir to pass to GetDiskFreeSpace
		vol[0] = m_FileName[4];
		TCHAR vol_name[128], fs_name[32];
		DWORD vol_serno, max_component, flags;
		if (::GetVolumeInformation(vol,
									vol_name, sizeof(vol_name)/sizeof(*vol_name),
									&vol_serno,
									&max_component,
									&flags,
									fs_name, sizeof(fs_name)/sizeof(*fs_name) ) )
		{
			retval += _T(".") + CString(fs_name);
		}
	}
	else
	{
		// Use .RAW as the "file extension"
		retval += _T(".RAW");
	}
#endif
	return retval;
}

// Normally file name without extension - for devices it's a more readable name like "Floppy Drive 0"
CString CFileNC::GetFileTitle( void ) const
{
	if (!::IsDevice(m_FileName))
		return CFile64::GetFileTitle();

#if defined(BG_DEVICE_SEARCH) || defined(_DEBUG)
	CSpecialList *psl = theApp.GetSpecialList();
	int idx = psl->find(m_FileName);
	if (idx == -1)
		return m_FileName;
	else
		return psl->name(idx);
#else
    return ::DeviceName(m_FileName);
#endif
}

DWORD CFileNC::Read( void * buffer, DWORD len )
{
	LONGLONG file_length = GetLength();                     // length of the file
	ASSERT(file_length%m_SectorSize == 0);
	LONGLONG end_address = m_FilePos + (LONGLONG)len;       // one past last byte to read
	if (end_address > file_length) end_address = file_length;
	DWORD done = 0;                                         // Bytes copied to output buffer so far

	while (m_FilePos < end_address)
	{
		get_current();

		DWORD tocopy = (DWORD)((end_address > m_end ? m_end : end_address) - m_FilePos);
		memcpy((char *)buffer + done, (char *)m_Buffer + m_FilePos - m_start, tocopy);
		done += tocopy;
		m_FilePos += tocopy;
	}
	ASSERT(m_FilePos == end_address);
	return done;
}

void CFileNC::Write( const void * buffer, DWORD len )
{
	LONGLONG file_length = GetLength();                     // length of the file
	ASSERT(file_length%m_SectorSize == 0);
	LONGLONG end_address = m_FilePos + (LONGLONG)len;       // one past last byte to read
	if (end_address > file_length) end_address = file_length;
	DWORD done = 0;                                         // Bytes copied to output buffer so far

	while (m_FilePos < end_address)
	{
		get_current();

		DWORD tocopy = (DWORD)((end_address > m_end ? m_end : end_address) - m_FilePos);
		memcpy((char *)m_Buffer + m_FilePos - m_start, (char *)buffer + done, tocopy);
		m_dirty = true;                 // The buffer is now "dirty" (needs to be written out)

		done += tocopy;
		m_FilePos += tocopy;
	}
	ASSERT(m_FilePos == end_address && done == len);
}

#pragma pack(push, 1)
struct
{
	DWORD start_sector;
	WORD  sectors;
	DWORD buffer;
} diskio;

struct
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} dioc_reg;
#pragma pack(pop)

// Fill buffer from current position in the file, keeping tracking of any sectors that could not be read
void CFileNC::get_current()
{
	// Only read from the file if what we want is not in the buffer
	if (m_FilePos < m_start || m_FilePos >= m_end)
	{
		if (m_dirty) make_clean();  // Write out current buffer if necessary.

		TRACE("xxx READ non-cached %ld\r\n", long(m_FilePos));

		// Work out which bit of the file we need to read
		m_start = (m_FilePos/m_SectorSize) * m_SectorSize - m_BufferSize/2;
		if (m_start < 0) m_start = 0;
		m_end = m_start + m_BufferSize;  // always read in a full buffer full
		if (m_end > GetLength()) m_end = GetLength();

        if (IsDevice(m_FileName) && !theApp.is_nt_)
        {
			// Read the data
			ASSERT(m_end - m_start < ULONG_MAX);
			ASSERT(m_FileHandle != INVALID_HANDLE_VALUE && m_Buffer != NULL);
			for (DWORD done = 0; done < m_end - m_start; done += m_SectorSize)
			{
				__int64 sec = (m_start + done)/m_SectorSize;
				diskio.start_sector = DWORD(sec);
				diskio.sectors = 1;
				diskio.buffer = DWORD((char *)m_Buffer + done);

				dioc_reg.reg_ESI = 0;       // read
				dioc_reg.reg_ECX = -1;      // always -1
				dioc_reg.reg_EBX = DWORD(&diskio);
				ASSERT(isalpha(m_FileName[4]));
				dioc_reg.reg_EDX = toupper(m_FileName[4]) - 'A' + 1;
				dioc_reg.reg_EAX = 0x7305;  // Ext_ABSDiskReadWrite

				DWORD junk;
				VERIFY(DeviceIoControl(m_FileHandle,
									   6,            // VWIN32_DIOC_DOS_DRIVEINFO
									   &dioc_reg, sizeof(dioc_reg),
									   &dioc_reg, sizeof(dioc_reg),
									   &junk,                 // # bytes returned
									   (LPOVERLAPPED) NULL));
				ASSERT(sizeof(dioc_reg) == junk);

				if ((dioc_reg.reg_Flags & 0x0001) != 0)
				{
					m_bad[sec] = dioc_reg.reg_EAX & 0xFFFF;
				}
			}
        }
		else
		{
			// Move to the file position to read
			LARGE_INTEGER pos;
			pos.QuadPart = m_start;
			if (m_retries < 0)
				pos.LowPart = ::SetFilePointer(m_FileHandle, pos.LowPart, &pos.HighPart, FILE_BEGIN);
			ASSERT(pos.QuadPart == m_start);

			IO_STATUS_BLOCK iosb;

			// Read the data
			ASSERT(m_end - m_start < ULONG_MAX);
			ASSERT(m_FileHandle != INVALID_HANDLE_VALUE && m_Buffer != NULL);
			DWORD num_read;
			for (DWORD done = 0; done < m_end - m_start; done += m_SectorSize)
			{
#ifdef _DEBUG
				memset((char *)m_Buffer + done, '\xCD', m_SectorSize);  // Just in case we want to see if the buffer was changed
#endif
				// Read one sector at a time since if reading multiple at a time and there
				// is an error in one then number read is always returned as zero.
				bool ok;
				DWORD status = NO_ERROR;
				if (m_retries < 0)
				{
					ok = ::ReadFile(m_FileHandle, (char *)m_Buffer + done, m_SectorSize, &num_read, NULL) == TRUE;
					if (!ok)
						status = ::GetLastError();
				}
				else
				{
					for (int retry = 0; ; ++retry)
					{
						status = (*pfReadFile)(m_FileHandle, 0, 0, 0, &iosb, (char *)m_Buffer + done, m_SectorSize, &pos, 0);
						ok = NT_SUCCESS(status);   // xxx do we need to track errors/warnings separately?
						num_read = ok ? m_SectorSize : 0;
						if (ok || retry >= m_retries)
							break;

						// If we have errors make sure we don't leave data in the buffer from a previous read
						memset((char *)m_Buffer + done, '\0', m_SectorSize);
					}
				}
				pos.QuadPart += m_SectorSize;       // Keep track of current file position
				if (!ok)
				{
					ASSERT(status != NO_ERROR && num_read == 0);

					// Flag this sector as bad and move on to the next one
					__int64 sec = (m_start + done)/m_SectorSize;
					m_bad[sec] = status;  // xxx does this need translating to Windows error?

					if (m_retries < 0)
						pos.LowPart = ::SetFilePointer(m_FileHandle, pos.LowPart, &pos.HighPart, FILE_CURRENT);
					ASSERT(pos.QuadPart == m_start + done + m_SectorSize);
				}
			}
		}
	}
}

// Write out the "dirty" buffer
void CFileNC::make_clean()
{
	TRACE("xxx WRITE non-cached\r\n");
	ASSERT(m_dirty && m_start < m_end && (m_end-m_start)%m_SectorSize == 0);
	DWORD num_written;

	LARGE_INTEGER pos;          // Position to write at
	pos.QuadPart = m_start;
    if (m_retries < 0)
    {
		pos.LowPart = ::SetFilePointer(m_FileHandle, pos.LowPart, &pos.HighPart, FILE_BEGIN);
		ASSERT(pos.QuadPart == m_start);

	    VERIFY(::WriteFile(m_FileHandle, m_Buffer, (DWORD)(m_end - m_start), &num_written, NULL));
	    ASSERT(num_written == (DWORD)(m_end - m_start));
    }
	else
    {
		IO_STATUS_BLOCK iosb;
        VERIFY((*pfWriteFile)(m_FileHandle, 0, 0, 0, &iosb, (char *)m_Buffer, ULONG(m_end - m_start), &pos, 0) == STATUS_SUCCESS);
    }
	m_dirty = false;                       // mark it clean
}

LONGLONG CFileNC::Seek( LONGLONG offset, UINT from )
{
	switch (from)
	{
	default:
		ASSERT(0);
		// fall through
	case CFile::begin:
		m_FilePos = offset;
		break;
	case CFile::current:
		m_FilePos += offset;
		break;
	case CFile::end:
		m_FilePos = GetLength() + offset;
		break;
	}
	return m_FilePos;
}

void CFileNC::Close()
{
	if (m_dirty)
		make_clean();  // Write out current buffer if necessary.
	if (m_retries >= 0)
	{
		if ( m_FileHandle != INVALID_HANDLE_VALUE )
		{
			(*pfClose)(m_FileHandle);
			m_FileHandle = INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		// Close other special files just like normal files.
		// Note for volume under Windows 9X (where we opened VXD \\.\vwin32) we let base class
		// close the handle since it just uses ::CloseHandle() as we would.
		CFile64::Close();
	}
}

// xxx checks/fixes:
// Flush allowed on FILE_FLAG_NO_BUFFERING files?
// Make sure GetFileTitle, GetFileName, GetInformation, 
// GetStatus, GetFileSize, SetEndOfFile, SetStatus not called for device file?
// Test works on normal file
