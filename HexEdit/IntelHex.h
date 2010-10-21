// IntelHex.h - classes to read/write Intel hex files

#ifndef INTELHEX_INCLUDED
#define INTELHEX_INCLUDED  1

// Can be used to create an Intel hex file
class CWriteIntelHex
{
public:
	CWriteIntelHex(const char *filename, unsigned long base_addr = 0L, size_t reclen = 32);
	~CWriteIntelHex();
	void Put(const void *data, size_t len);
	CString Error() const { return error_; }

private:
	void put_rec(int stype, unsigned long addr, void *data, size_t len);
	int put_hex(char *pstart, unsigned long val, int bytes);

	CStdioFile file_;
	unsigned long addr_;                // Address of next record to write
	size_t reclen_;                     // Preferred output record size
	CString error_;                     // Error message for last error (if any)
	int recs_out_;                      // Number of records output so far
};

// This class can be used to read an Intel hex file into memory.  The records must be in address order.
class CReadIntelHex
{
public:
	CReadIntelHex(const char *filename, BOOL allow_discon = FALSE);
	size_t Get(void *data, size_t max, unsigned long &address); // Get a line of data from the file
	CString Error() const { return error_; }

private:
	int get_rec(void *data, size_t max_len, size_t &len, unsigned long &address);
	unsigned long get_hex(char *pstart, int bytes, int &checksum);
	CStdioFile file_;
	unsigned long addr_;                // Address of next record to be read or -1 if at start
	unsigned long line_no_;             // Line number of last line of text read
	CString error_;                     // Error message for last error (if any)
	int recs_in_;                       // Number of records read so far
	BOOL allow_discon_;                 // Allow discontiguous records
};
#endif
