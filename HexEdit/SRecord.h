// srecord.h - classes to read/write Motorola S-record files

#ifndef SRECORD_INCLUDED
#define SRECORD_INCLUDED


// Writes Motorola S-records to a file from data in memory
class CWriteSRecord
{
public:
    CWriteSRecord(const char *filename, unsigned long base_addr = 0L, int stype = 3, size_t reclen = 32);
    ~CWriteSRecord();
    void Put(const void *data, size_t len);
    CString Error() const { return error_; }

private:
    void put_rec(int stype, unsigned long addr, void *data, size_t len);
    int put_hex(char *pstart, unsigned long val, int bytes);

    CStdioFile file_;
    int stype_;                         // preferred record type: 1 = S1, 2 = S2, 3 = S3
    unsigned long addr_;                // Address of next S record to write
    size_t reclen_;                     // Preferred output record size
    CString error_;                     // Error message for last error (if any)
    int recs_out_;                      // Number of S1/S2/S3 records output so far
};

// This class can be used to read a file containing Motorola S-records into memory.
// It handles any combination of S1,S2,S3 records but the records must be in address order.
class CReadSRecord
{
public:
    CReadSRecord(const char *filename, BOOL allow_discon = FALSE);
    size_t Get(void *data, size_t max, unsigned long &address); // Get a line of data (S1/S2/S3) from the file
    CString Error() const { return error_; }

private:
    int get_rec(void *data, size_t max_len, size_t &len, unsigned long &address);
    unsigned long get_hex(char *pstart, int bytes, int &checksum);
    CStdioFile file_;
    unsigned long addr_;                // Address of next record to be read or -1 if at start
    unsigned long line_no_;             // Line number of last line of text read
    CString error_;                     // Error message for last error (if any)
    int recs_in_;                       // Number of S1/S2/S3 records read so far
    BOOL allow_discon_;                 // Allow discontiguous records
};
#endif
