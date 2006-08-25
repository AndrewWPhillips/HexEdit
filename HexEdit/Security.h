// Security.h : contains encryption keys for registration
//

// Copyright (c) 1999 by Andrew W. Phillips.
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

// Use text keys in case we ever need to put them into a text file on a web site??
#define REG_SEND_KEY "gNd8%j0j"
#define REG_RECV_KEY "Eh7(8Fu$"

// This is encrypted, converted to (13) alphanumeric characters and sent
// This must be 8 bytes long due to restrictions on Blow Fish encryption
// Note: Don't put anything in here that changes as this will cause the code to change
struct send_info_t
{
    unsigned short crc;                 // CRC of machine_info (machine reg), or user name (user reg)
    unsigned char flags;                // bit 0 = temp reg flag, other bits indicate version
    char type;                          // 5 = machine, 6 = user
    long init_date;                     // Date when HexEdit was first run
};
