// Parses C/C++ source code to extract variable declarations
// TParser.h : wrapper for MS XML (DOM) objects
//
// Copyright (c) 2005 by Andrew W. Phillips.
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
#ifndef TPARSER_INCLUDED
#define TPARSER_INCLUDED

#pragma once

#include "XmlTree.h"
#include "expr.h"

class TParser;

// Use our expression eval class to eval constant expressions (eg array size).
class TExpr : public expr_eval
{
public:
    TExpr(TParser * ptp) : expr_eval(10, false), ptp_(ptp) {}
    virtual value_t find_symbol(const char *sym, value_t parent, size_t index, int *pac,
        __int64 &sym_size, __int64 &sym_address, CString &sym_str);

private:
	TParser * ptp_;
};

class TParser
{
	friend class TExpr;
public:
	TParser();

	CXmlTree::CFrag Parse(CXmlTree *ptree, LPCTSTR str);

    // These are options set from the dialog
	int packing_default_;       // default packing (1,2,4,8 or 16)

	bool check_std_, check_win_, check_common_, check_custom_; // Type lists to check
	bool save_custom_;          // Save (to cutsom types file) new types (custom_types_) that were parsed?

	bool check_values_win_, check_values_custom_;  // Values list to check
	bool save_values_custom_;   // Save (to custom list file) parsed values?

	bool search_include_;       // Should we search Include env. var. directories for #include files?

private:
	CXmlTree::CFrag parse_all(LPCTSTR outer_name, long &max_align, bool is_union = false); // parse out all type information
	CXmlTree::CElt find_elt(LPCTSTR name, CXmlTree::CElt &root, int &node_num);
	long get_size(LPCTSTR name); // Total size of a type
	long get_pack(LPCTSTR name); // Optimum packing for a type
	CString get_line();         // Get a line of text
	CString get_pp_line();      // Get a line of preprocessed text (handles simple #defines and their substitutions)
	CString get_next();         // get next "token"
	CString peek_next();        // return next token without moving
    CString skip_modifiers(CString ss, bool &, bool &, bool &, bool &, bool);

	CString search_path(LPCTSTR inc, LPCTSTR name);

	static char * ctokens[];
    char dec_point_;            // Character used for decimal point (usually full-stop)

	//const char * next_;         // Start of next line (NULL if none)
	std::vector<CString> text_;      // text_[0] is text passed in, above that is complete text from nested #include files
	std::vector<const char *> next_; // Ptrs into corresponding text_ of start of current parse line
	std::vector<int> pp_nesting_;    // Tracks conditional compilation (#if/#ifdef ... #endif) nesting in each file
    std::vector<CString> filename_;  // Name of #included file (empty string for passed in text at index of zero)
    std::vector<int> line_no_;       // Current line number in include file

	CString str_;               // Current input line after preprocessing
	const char * pp_;           // Ptr into (already preprocessed) line being parsed
	bool typedef_;              // Are we processing a typedef or a real object?
	CString next_token_;		// Saved next token after peek_next() called

	bool is_real_;              // Was the last number a floating pt number?
	__int64 last_int_;          // Value of last integer
	double  last_real_;         // Value of last floating pt number
	CString last_comment_, last_string_;
	std::vector<int> pack_;     // Stack of pack values (used for #pragma pack())
	//int pad_count_;             // Used to generate names for padding bytes

	TExpr *pexpr_;              // Used for evaluating constant int expressions (eg array size)

	CXmlTree *ptree_;

	// Info on all the types a user can use are stored in normal template XML files.
	// Each type is stored as an element in the root (binary_file_format) element with
	// a name that is the type name.  Hence when we have parsed a type name we scan one
	// or more of the following lists for it, get the XML for the element, change the
	// name from the type name to the variable name and use that for the new XML node.
	bool LoadTypeFile(LPCTSTR filename, CXmlTree &types, bool should_exist);
	CXmlTree std_types_;        // Standard C/C++ types
	CXmlTree win_types_;        // Win32 types
	CXmlTree common_types_;     // Other common types
	CXmlTree custom_types_;     // Where new types are added

	// Constants can also be loaded and saved.  These are the constant values in enums and "static const short/int/long" decalarations.
	bool LoadValuesFile(LPCTSTR filename, std::map<CString, __int64> &values, bool should_exist);
	bool SaveValuesFile(LPCTSTR filename, const std::map<CString, __int64> &values) const;
	std::map<CString, __int64> win_consts_;     // Predefined values from Windows
	std::map<CString, __int64> custom_consts_;  // Where parsed values are added (enum and static const)

	std::map<CString, CString> pp_def_;         // All preprocessor #defines seen in this parse

};
#endif // TPARSER_INCLUDED
