#if ! defined( FILE_64_CLASS_HEADER )

#define FILE_64_CLASS_HEADER

#include <winioctl.h>
#include <map>

#if ! defined( FILE_ATTRIBUTE_ENCRYPTED )
#define FILE_ATTRIBUTE_ENCRYPTED (0x00000040)
#endif

#if ! defined( FILE_ATTRIBUTE_REPARSE_POINT )
#define FILE_ATTRIBUTE_REPARSE_POINT (0x00000400)
#endif

#if ! defined( FILE_ATTRIBUTE_SPARSE_FILE )
#define FILE_ATTRIBUTE_SPARSE_FILE (0x00000200)
#endif

#if ! defined( FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED (0x00002000)
#endif

class CFile64
{
   private:

      CFile64( const CFile64& ) {};
      CFile64& operator=( const CFile64& ) { return( *this ); };

   protected:

      SECURITY_ATTRIBUTES * m_SecurityAttributes_p;

      SECURITY_DESCRIPTOR * m_SecurityDescriptor_p;

      HANDLE m_FileHandle;

      CString m_PathName;
      CString m_FileName;
      CString m_FileTitle;

      BOOL m_CloseOnDelete;

      DWORD m_SectorSize;
      LONGLONG m_Length;          // Length of volume/physical drive or -1 if normal file

      void m_Initialize( void );
      void m_Uninitialize( void );

   public:

      // Flag values
      enum OpenFlags
      {
         modeRead =          0x0000,
         modeWrite =         0x0001,
         modeReadWrite =     0x0002,
         shareCompat =       0x0000,
         shareExclusive =    0x0010,
         shareDenyWrite =    0x0020,
         shareDenyRead =     0x0030,
         shareDenyNone =     0x0040,
         modeNoInherit =     0x0080,
         modeCreate =        0x1000,
         modeNoTruncate =    0x2000,
         typeText =          0x4000, // typeText and typeBinary are used in
         typeBinary =   (int)0x8000, // derived classes only
         osNoBuffer =  (int) 0x10000, // unbuffered
		 osWriteThrough=(int)0x20000, // write straight to disk (bypass even hardware buffers)
      };

      enum Attribute
      {
         normal =    0x00,
         readOnly =  0x01,
         hidden =    0x02,
         system =    0x04,
         volume =    0x08,
         directory = 0x10,
         archive =   0x20
      };

      enum SeekPosition
      {
         begin   = 0x0,
         current = 0x1,
         end     = 0x2
      };

      enum
      {
         hFileNull = -1
      };

      CFile64();
      CFile64( int file_handle );
      CFile64( LPCTSTR filename, UINT open_flags );
     ~CFile64();

      UINT m_hFile; // AGAINST CODING STANDARDS!!!

      virtual operator HFILE() const;

      virtual void                  Abort( void );
      virtual void                  Close( void );
      virtual CFile64 *             Duplicate( void ) const;
      virtual void                  Flush( void );
      virtual CString               GetFileName( void ) const;
      virtual CString               GetFilePath( void ) const;
      virtual CString               GetFileTitle( void ) const;
      virtual HANDLE                GetHandle( void ) const;
      virtual BOOL                  GetInformation( BY_HANDLE_FILE_INFORMATION& information ) const;
      virtual LONGLONG              GetLength( void ) const;
      virtual DWORD                 SectorSize() const { return m_SectorSize; }
      virtual LONGLONG              GetPosition( void ) const;
      virtual SECURITY_ATTRIBUTES * GetSecurityAttributes( void ) const;
      virtual SECURITY_DESCRIPTOR * GetSecurityDescriptor( void ) const;
#if ! defined( WFC_STL )
      virtual BOOL                  GetStatus( CFileStatus& status ) const;
#endif // WFC_STL
      virtual void                  LockRange( LONGLONG position, LONGLONG number_of_bytes_to_lock );
#if ! defined( WFC_STL )
      virtual BOOL                  Open( LPCTSTR filename, UINT open_flags, CFileException * exception_p = NULL );
#else // WFC_STL
      virtual BOOL                  Open( LPCTSTR filename, UINT open_flags );
#endif // WFC_STL
      virtual DWORD                 Read( void * buffer, DWORD number_of_bytes_to_read );
      virtual DWORD                 Read( CByteArray& buffer, DWORD number_of_bytes_to_read );
      virtual DWORD                 ReadHuge( void * buffer, DWORD number_of_bytes_to_read );
      virtual LONGLONG              Seek( LONGLONG offset, UINT from );
      virtual void                  SeekToBegin( void );
      virtual LONGLONG              SeekToEnd( void );
      virtual BOOL                  SetEndOfFile( LONGLONG length ); // when CBigFile becomes not read-only
      virtual void                  SetFilePath( LPCTSTR new_name );
      virtual void                  SetLength( LONGLONG length );
      virtual void                  UnlockRange( LONGLONG position, LONGLONG number_of_bytes_to_unlock );
      virtual void                  Write( const void * buffer, DWORD number_of_bytes_to_write );
      virtual void                  WriteHuge( const void * buffer, DWORD number_of_bytes_to_write );

#if defined( _DEBUG ) && ! defined( WFC_NO_DUMPING )

      virtual void                  Dump( CDumpContext& dump_context ) const;

#endif // _DEBUG

      static void PASCAL            Rename( LPCTSTR old_name, LPCTSTR new_name );
      static void PASCAL            Remove( LPCTSTR filename );
#if ! defined( WFC_STL )
      static BOOL PASCAL            GetStatus( LPCTSTR filename, CFileStatus& status );
      static void PASCAL            SetStatus( LPCTSTR filename, const CFileStatus& status );
#endif // WFC_STL
      static __int64 PASCAL         GetSize(LPCTSTR filename);
};

///////////////////////////////////////////////////////////////////////////
// CFileNC: used to access file in non-cached (FILE_FLAG_NO_BUFFERING) mode.
// This hides restrictions on offsets and block sizes for seek, read, write etc.
// Volumes and physical drives must (in general) be opened non-cached.

class CFileNC : public CFile64
{
public:
	CFileNC() : CFile64() { m_Buffer = NULL; m_dirty = false; }

	virtual CString GetFileName( void ) const;
	virtual CString GetFileTitle( void ) const;

	CFileNC( LPCTSTR filename, UINT open_flags );
#if ! defined( WFC_STL )
	virtual BOOL Open( LPCTSTR filename, UINT open_flags, CFileException * exception_p );
#else // WFC_STL
	virtual BOOL Open( LPCTSTR filename, UINT open_flags );
#endif // WFC_STL

	virtual void Close( void );
	virtual DWORD Read( void * buffer, DWORD len );
	virtual void Write( const void * buffer, DWORD len );
	virtual LONGLONG Seek( LONGLONG offset, UINT from );

	virtual LONGLONG GetPosition( void ) const
	{
		return m_FilePos;
	}

	virtual void SeekToBegin( void )
	{
		m_FilePos = 0;
	}

	virtual LONGLONG SeekToEnd( void )
	{
		return m_FilePos = GetLength();
	}

	bool HasError() { return !m_bad.empty(); }         // returns true if at least one sector had a read error
	DWORD Error(__int64 sec) { if (m_bad.find(sec) == m_bad.end()) return 0; else return m_bad[sec]; }  // return sector error or 0 if none

private:
	void make_clean();          // Write buffer to disk (must be dirty)
	void get_current();         // Read buffer from disk

	LONGLONG m_FilePos;         // current file posn requested
	DWORD m_BufferSize;         // how big is the buffer we have allocated
	LPVOID m_Buffer;			// Pointer to buffer

	LONGLONG m_start, m_end;	// Address range of bytes in m_Buffer
	bool m_dirty;               // Does the current buffer have changes that need writing back?
	std::map<__int64, DWORD> m_bad; // Bad sectors and the error they had

	// Note m_retries > -1 indicates that this is a windows native API physical device
	int m_retries;				// number of read retries on physical devices
};

#endif // FILE_64_CLASS_HEADER
