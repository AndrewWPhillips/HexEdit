// TParser.cpp - parse C/C++/C# code for data declarations
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

#include "stdafx.h"

#include <vector>
#include <map>
#include <algorithm>
#include <shlwapi.h>    // for PathFindOnPath

#include "hexedit.h"
#include "TParser.h"
#include "DFFDMisc.h"

// This function is the sole reason we derived a new class from TExpr - it allows
// the user to use symbols (with constant values) in a constant expression.
expr_eval::value_t TExpr::find_symbol(const char *sym, value_t parent, size_t index, int *pac,
    __int64 &sym_size, __int64 &sym_address, CString &sym_str)
{
	// We can ignore index, pac, sym_address and just check that parent is TYPE_NONE
	// We only return a valid sym_size if "sym" is in the type list which is
	// just used for getting size of types (eg "sizeof(int)").

	value_t retval;
	retval.int64 = 0;
	if (parent.typ != TYPE_NONE)
		return retval;

	sym_address = 0;                    // Put something here

    // Do macro (#define) substitutions first
	CString subst = sym;
	bool replaced = false;
	while (ptp_->pp_def_.find(subst) != ptp_->pp_def_.end())
	{
		replaced = true;
		subst = ptp_->pp_def_[subst];
		if (subst == sym)        // Subst back to original (avoid inf loop)
			break;
	}
    sym = subst;

	std::map<CString, __int64>::const_iterator pval;

    if (replaced)
    {
        // We end up with a number after #define substitutions
		sym_size = 8;
		retval.typ = TYPE_INT;
        retval.int64 = _atoi64(sym);
    }
    else if ((pval = ptp_->custom_consts_.find(sym)) != ptp_->custom_consts_.end())
	{
		// Found the value in list of constants
		sym_size = 8;
		retval.typ = TYPE_INT;
		retval.int64 = pval->second;
	}
	else if (ptp_->check_values_win_ && (pval = ptp_->win_consts_.find(sym)) != ptp_->win_consts_.end())
	{
		// Found the value in list of Windows constants
		sym_size = 8;
		retval.typ = TYPE_INT;
		retval.int64 = pval->second;
	}
	else if ((sym_size = ptp_->get_size(sym)) != -1)
	{
		// Signal that size is valid (although retval.int64 = 0 since we don't have a value)
		// This is for things like sizeof(type) which is allowed in constant expressions.
		retval.typ = TYPE_INT;
	}
	// else retval.type == TYPE_NONE which indicates that the symbol is not known

	return retval;
}

// Notes on padding (pack_):
// Parsed structures will be padded with dummy elements to conform to the current packing value.
// [Dummy elements added have type "none" and a name of the form "Fill$<n>" where n starts from zero - obsolete]
// The current packing value is obtained from packing_default_, but may be changed with #pragma pack() directives.
// The pack_ vector stores a stack of pack values the top (pack_.back()) value being the current value.
// (The stack is necessary to implement the push/pop options of #pragma pack().)
// Padding is added so that a field is aligned according to the smaller of its requirements & the current pack_ value.
// Padding may also be added at the end (according to elt with largest packing) to make sure all array elts are OK.
// Most DATA types have packing requirements just equal to their size (except "none" and "string").
// - eg for 32 bit int it will be aligned on 4 byte boundary as long as pack_ >= 4 else it is aligned on pack_ boundary.
// STRUCT packing requirements = largest packing of all contained elements (not > pack_ value in effect when parsed).
// - eg if STRUCT's largest elt is 32 bit int then STRUCT packing is 4 OR pack_ (if pack_ < 4).
// We have to save this for every STRUCT that we parse (saved in XML "pack" attr) since we don't know what pack_ value
// was in effect when the STRUCT was parsed - this ensures a STRUCT is aligned properly when used in another STRUCT etc.

TParser::TParser()
{
	ptree_ = NULL;
	pexpr_ = NULL;

	dec_point_ = theApp.dec_point_;
	packing_default_ = 4;

	check_values_win_ = check_values_custom_ = save_values_custom_ = false;
	check_std_ = check_custom_ = check_win_ = check_common_ = save_custom_ = false;
	search_include_ = false;
}

static const char * anon_type_name      = "unnamed$";         // temp name used to store unnamed struct/union/enum
static const char * ptr_type_name       = "pointer$";         // name of integer type to store a pointer
static const char * func_ptr_type_name  = "function_pointer$";// integer type to store a function pointer
static const char * enum_type_name      = "enum$";            // integer type that stores enums (usually same as int)
static const char * bitfield_type_name  = "bit_field$";       // underlying storage unit for bit-fields (usually same as int)

CXmlTree::CFrag TParser::Parse(CXmlTree *ptree, LPCTSTR str)
{
	ptree_ = ptree;
	pexpr_ = new TExpr(this);

    text_.clear();
    next_.clear();
    pp_nesting_.clear();
    filename_.clear();
    line_no_.clear();
	text_.push_back(CString(str));
	next_.push_back(text_.back().GetBuffer(0));
	pp_nesting_.push_back(0);
    filename_.push_back(CString());
    line_no_.push_back(0);

	pp_ = "";

	pack_.clear();
	pack_.push_back(packing_default_);
	//pad_count_ = 0;
	pp_def_.clear();
	win_consts_.clear();
	custom_consts_.clear();

	// Add some predefined macros
	time_t now = time(NULL);
	char * timestr = asctime(localtime(&now));
	CString tmp;
	tmp.Format("%3.3s %2.2s %4.4s", timestr+4, timestr+8, timestr+20);
	pp_def_[CString("__DATE__")] = tmp;                       // current date
	tmp.Format("%8.8s", timestr+11);
	pp_def_[CString("__TIME__")] = tmp;                       // current time
	pp_def_[CString("__TIMESTAMP__")] = CString(timestr, 24);

	pp_def_[CString("__STDC__")] = CString("1");              // indicate that we can handle standard C
	pp_def_[CString("__cplusplus")] = CString("1");           // indicate that we can handle C++
	//pp_def_[CString("_MSC_VER")] = CString("1300");
	//pp_def_[CString("_WIN32")] = CString("1");
	pp_def_[CString("defined:1")] = CString("defined($0$)");  // this avoids substitutions of defined() operator parameter

	// Load constants
	if (check_values_win_)
		LoadValuesFile("_windows_constants.txt", win_consts_, true);

	CString cust_values_filename = "_custom_constants.txt";
	if (check_values_custom_)
		LoadValuesFile(cust_values_filename, custom_consts_, false);

	// Load the type info (.XML) files that we are using
	if (check_std_)
	{
		if (!LoadTypeFile("_standard_types.xml", std_types_, true))
		{
			// Load essential types required for internal use
			CString tmp =	"<binary_file_format>\n"
							"  <data name=\"void\" type=\"none\" len=\"0\"/>\n"
							"  <data name=\"" + CString(ptr_type_name)      + "\" type=\"int\" len=\"4\"/>\n"
							"  <data name=\"" + CString(func_ptr_type_name) + "\" type=\"int\" len=\"4\"/>\n"
							"  <data name=\"" + CString(enum_type_name)     + "\" type=\"int\" len=\"4\"/>\n"
							"  <data name=\"" + CString(bitfield_type_name) + "\" type=\"int\" len=\"4\"/>\n"
							"</binary_file_format>";
			std_types_.LoadString(tmp);
		}
	}

	CString windows_types_filename = "_windows_types.xml";
	if (check_win_)
		LoadTypeFile(windows_types_filename, win_types_, true);

	if (check_common_)
		LoadTypeFile("_common_types.xml", common_types_, true);

	// We always load custom types if present - but file will not be present is user has never saved it (save_custom_)
	CString cust_types_filename = "_custom_types.xml";
    if (check_custom_)
	    LoadTypeFile(cust_types_filename, custom_types_, false);
    else
		custom_types_.LoadString("<binary_file_format></binary_file_format>");

	CXmlTree::CFrag retval;
	try
	{
		long dummy;
		retval = parse_all("", dummy);
	}
	catch (CString ss)
    {
        CString tmp;
        if (filename_.size() > 1)
            tmp.Format("\r\n\r\nat line %ld of \"%s\".", long(line_no_.back()), filename_.back());
        AfxMessageBox(ss + tmp);
    }

	//CString debug_string = custom_types_.DumpXML();
	delete pexpr_;	pexpr_ = NULL;

	if (save_values_custom_ && !SaveValuesFile(cust_values_filename, custom_consts_))
	{
		CString ss;
        ss.Format("Could not save new values to\n\"%s\"", cust_values_filename);
		AfxMessageBox(ss);
	}

	if (save_custom_ && !custom_types_.Save(theApp.xml_dir_ + cust_types_filename))
	{
		CString ss;
        ss.Format("Could not save new types to\n\"%s\"", cust_types_filename);
		AfxMessageBox(ss);
	}

	return retval;
}

bool TParser::LoadTypeFile(LPCTSTR filename, CXmlTree &types, bool should_exist)
{
	bool retval = true;

	CString pathname = theApp.xml_dir_ + filename;
	WIN32_FIND_DATA wfd;
	if (::FindFirstFile(pathname, &wfd) == INVALID_HANDLE_VALUE)
	{
		if (should_exist)
		{
			CString ss;
			ss.Format("Types file not found:\n\"%s\"", pathname);
			AfxMessageBox(ss);
		}
		retval = false;
	}
	else if (!types.LoadFile(pathname))
	{
		CString ss;
        ss.Format("Error reading types file\n\"%s\"\n\n"
				  "XML parse error at line %ld:%s\n%s", 
                  pathname, long(ptree_->ErrorLine()),
                  ptree_->ErrorLineText(), ptree_->ErrorMessage());
        AfxMessageBox(ss);
		retval = false;
	}

	// Load minimal required XML if file load failed
	if (!retval && !types.LoadString("<binary_file_format></binary_file_format>"))
	{
		AfxMessageBox("BinaryFileFormat.dtd not found");
		types.LoadString("<binary_file_format></binary_file_format>");
	}

	return retval;
}

bool TParser::LoadValuesFile(LPCTSTR filename, std::map<CString, __int64> &values, bool should_exist)
{
	bool retval = true;

	CString pathname = theApp.xml_dir_ + filename;
	WIN32_FIND_DATA wfd;
	retval = ::FindFirstFile(pathname, &wfd) != INVALID_HANDLE_VALUE;
	if (retval)
	{
		// Read the values from the file
		try 
		{
			CStdioFile fin(pathname, CFile::modeRead|CFile::shareDenyWrite|CFile::typeText);

			CString ss, name, value;
			while (fin.ReadString(ss))
			{
				if (ss.IsEmpty() || ss[0] == ';')    // ignore empty/comment lines
					continue;

				AfxExtractSubString(name, ss, 0, '|');
				AfxExtractSubString(value, ss, 1, '|');
				if (!name.IsEmpty())
					values[name] = _atoi64(value);
			}
		}
		catch (CFileException *pfe)
		{
			AfxMessageBox(::FileErrorMessage(pfe, CFile::modeRead));
			pfe->Delete();
			retval = false;
		}
	}
	else if (should_exist)
	{
		CString ss;
		ss.Format("Constants file not found:\n\"%s\"", pathname);
		AfxMessageBox(ss);
	}

	return retval;
}

bool TParser::SaveValuesFile(LPCTSTR filename, const std::map<CString, __int64> &values) const
{
	bool retval = true;

	CString pathname = theApp.xml_dir_ + filename;
	// Read the values from the file
	try 
	{
		CStdioFile fout(pathname, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText);
		fout.WriteString("; version 1\n");

		std::map<CString, __int64>::const_iterator pv;
		for (pv = values.begin(); pv != values.end(); ++pv)
		{
			char buf[1024];
#ifdef _DEBUG
			buf[sizeof(buf)-1] = '\xCD';
#endif
			sprintf(buf, "%.1000s|%I64d\n", pv->first, pv->second);  // max length 1000+1+20+2 (max I64 is 20 digits) = 1023
			ASSERT(buf[sizeof(buf)-1] == '\xCD');
			fout.WriteString(buf);
		}
	}
	catch (CFileException *pfe)
	{
		AfxMessageBox(::FileErrorMessage(pfe, CFile::modeWrite));
		pfe->Delete();
		retval = false;
	}

	return retval;
}

// Get next line allowing for continued lines using backslash (\) or multiline comment.
// Removes comments but stores the last one seen in last_comment_.
CString TParser::get_line()
{
	CString retval;
	bool in_str = false;                // Are we in a string?
	bool in_chr = false;                // Are we in a character literal?
	bool in_comment = false;            // Are we inside a /* C style comment */
	bool cr_seen = false;               // Remember if we saw a CR/LF in C style comment

	const char * start = next_.back(); // start of next line
	const char * pb = start;           // current point in the text string

	while (*pb != '\0')
	{
        if (*pb == '\n')
            line_no_.back()++;

		if (in_comment)
		{
			ASSERT(!in_str && !in_chr);
			if (*pb == '\r' || *pb == '\n')
			{
				cr_seen = true;
			}
			else if (*pb == '*' && *(pb+1) == '/')
			{
				in_comment = false;
				last_comment_ = CString(start, int(pb-start));
				start = pb + 2;
				++pb;
				if (cr_seen)
				{
					++pb;
					break;              // Don't allow /* CR/LF */ to join lines (even though latest VC++ does)
				}
			}
		}
		else if ((*pb == '\r' || *pb == '\n') && (pb == start || *(pb-1) != '\\'))
		{
			// End of the line
			retval += CString(start, int(pb-start));
			if (in_str)
				retval += '"';  // terminate string
			else if (in_chr)
				retval += '\'';
			start = pb;
			break;
		}
		else if (in_str)
		{
			ASSERT(!in_comment);
			ASSERT(pb > start);         // we must have already seen at least a quote (")
			if (*pb == '"' && *(pb-1) != '\\')
				in_str = false;
		}
		else if (in_chr)
		{
			// Note: we have to handle this because a character literal can in theory be something like '//' or '/*'
			ASSERT(!in_comment);
			ASSERT(pb > start);         // we must have already seen at least an apostrophe (')
			if (*pb == '\'' && *(pb-1) != '\\')
				in_chr = false;
		}
		else if (*pb == '/' && *(pb+1) == '*')
		{
			// C style comment
			retval += CString(start, int(pb-start)) + CString(" ");  // A comment is replaced by a single space
			in_comment = true;
			start = pb + 2;
			++pb;
		}
		else if (*pb == '/' && *(pb+1) == '/')
		{
			// C++ style comment
			retval += CString(start, int(pb-start));
			start = pb + 2;
			pb += 2;
			while (*pb != '\0' && *pb != '\r' && *pb != '\n')
				++pb;
			last_comment_ = CString(start, int(pb-start));
			start = pb;
            if (*pb == '\n')
                line_no_.back()++;
			break;
		}
		else if (*pb == '"')
		{
			in_str = true;
		}
		else if (*pb == '\'')
		{
			in_chr = true;
		}
		++pb;
	}

	retval += CString(start, int(pb-start));
	while (*pb == '\r' || *pb == '\n')
    {
        if (*pb == '\n')
            line_no_.back()++;
		++pb;
    }
	next_.back() = pb;
	return retval;
}

// Returns the next line but filters out preprocessing directives.
// Also performs substitutions based on any #defines already encountered.
CString TParser::get_pp_line()
{
	CString str;
	int skipping = 0;                   // 0 = not skipping, 1 = skip to #else/#elif/#endif, 2 = skip to #endif
	int skip_nesting;                   // pp_nesting_ value when we started skipping

	for (;;)
	{
		// Check if we are at the end of the file/input string
		while (*(next_.back()) == '\0')
		{
			if (pp_nesting_.back() > 0)
			{
				if (next_.size() > 1)
					AfxMessageBox("Warning: Missing #endif in #include file");
				else
					AfxMessageBox("Warning: Missing #endif");
			}
			if (next_.size() == 1)
			    return "";              // That's it - end of string in input string (original text being parsed)
			text_.pop_back();
			next_.pop_back();
			pp_nesting_.pop_back();
			filename_.pop_back();
			line_no_.pop_back();
		}

		str = get_line();               // get next line (comments already removed)
		str.TrimLeft();
		if (str.IsEmpty())
			continue;

		// We don't do substitutions in #define, #ifdef etc (but do in #include)
		bool no_subst = false;
		if (str[0] == '#')
		{
			CString tmp = str.Mid(1);
			tmp.TrimLeft();
            if (strncmp(tmp, "define", 6) == 0 ||
                strncmp(tmp, "undef",  5) == 0 ||
//                strncmp(tmp, "if",     2) == 0 ||  // we did this so #if defined(MACRO) worked but it causes other problems
//                strncmp(tmp, "elif",   4) == 0 ||
                strncmp(tmp, "ifdef",  5) == 0 ||
                strncmp(tmp, "ifndef", 6) == 0 )
            {
				no_subst = true;
            }
		}
		else if (skipping)
			continue;                           // This line is conditionally excluded

		CString retval;                         // String after substitutions

		if (no_subst)
		{
			retval = str;                       // Just copy as is
		}
		else
		{
			// Do preprocessor substitutions
			const char * pb = str.GetBuffer(0);
			const char * start = pb;
			bool in_str = false;                // Are we in a string?
			bool in_chr = false;                // Are we in a character literal?

			while (*pb != '\0')
			{
				// Don't do subtitutions in string and character literals
				if (in_str)
				{
					ASSERT(pb > start);         // we must have already seen at least a quote (")
					if (*pb == '"' && *(pb-1) != '\\')
						in_str = false;
				}
				else if (in_chr)
				{
					ASSERT(pb > start);         // we must have already seen at least an apostrophe (')
					if (*pb == '\'' && *(pb-1) != '\\')
						in_chr = false;
				}
				else if (*pb == '"')
				{
					in_str = true;
				}
				else if (*pb == '\'')
				{
					in_chr = true;
				}
				else if (::isalpha(*pb) || *pb == '_')
				{
					// Save what we have to here
					retval += CString(start, int(pb-start));

					// Get identifier
					const char * pb2;
					for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
						; // nothing here
					ASSERT(pb2 > pb);           // There must be at least one letter

					// Get the name and find any substitutions
					CString id(pb, int(pb2 - pb));
					CString subst = id;
					bool replaced = false;
					if (subst == "__FILE__")
					{
						replaced = true;
						if (filename_.size() < 2)
							subst = "\"EDIT_BOX\"";
						else
							subst = "\"" + filename_.back() + "\"";
					}
					else if (subst == "__LINE__")
					{
						replaced = true;
						subst.Format("%d", line_no_.back()); 
					}
					else
						while (pp_def_.find(subst) != pp_def_.end())
						{
							replaced = true;
							subst = pp_def_[subst];
							if (subst == id)        // Subst back to original (avoid inf loop)
								break;
						}

					const char *saved = pb2;   // Save where we are up to in case it is not a macro
					while (::isspace(*pb2))    // Skip to '('
						++pb2;

					// If macro not found see if it is a "function-like" macro
					if (!replaced && *pb2 == '(')
					{
						std::vector<CString> pvalue;
						int nesting = 0;
						bool in_str = false, in_chr = false;
						CString tmp;
						for (;;)
						{
							++pb2;
							if (*pb2 == '\0')
							{
								// We don't support multi-line macro invocations
								pvalue.clear();
								break;
							}
							if (in_str)
							{
								if (*pb2 == '"')
									in_str = false;
							}
							else if (in_chr)
							{
								if (*pb2 == '\'')
									in_chr = false;
							}
							else if (*pb2 == '"')
							{
								in_str = true;
							}
							else if (*pb2 == '\'')
							{
								in_chr = true;
							}
							else if ((*pb2 == ',' || *pb2 == ')') && nesting == 0)
							{
								pvalue.push_back(tmp);
								tmp.Empty();
								if (*pb2 == ')')
									break;
								continue;            // don't add comma to a substitution
							}
							else if (*pb2 == '(')
								++nesting;
							else if (*pb2 == ')')
								nesting--;

							tmp += *pb2;
						}
						ASSERT(*pb2 == '\0' || *pb2 == ')');
						if (*pb2 == ')')
							++pb2;

						// Generate a name including parameter count
						CString macro_name;
						macro_name.Format("%s:%ld", subst, long(pvalue.size()));
						if (pp_def_.find(macro_name) != pp_def_.end())
						{
							// Found it - so get replacement text
							subst = pp_def_[macro_name];

							// Perform substitutions
							for (std::vector<CString>::const_iterator pv = pvalue.begin(); pv != pvalue.end(); ++pv)
							{
								CString tmp;
								tmp.Format("$%ld$", long(pv - pvalue.begin()));
								subst.Replace(tmp, *pv);
							}
							replaced = true;
						}
						else
							pb2 = saved;        // Not found so go back to '('
					}
					else
						pb2 = saved;

					retval += subst;
					start = pb2;
					pb = pb2 - 1;               // Allow for increment below
				}

				++pb;
			}

			retval += CString(start, int(pb-start));
			retval.Replace("##", "");           // Remove ## for pasting tokens together
		}

		if (retval[0] != '#')
			return retval;                  // We have it (a non-preprocessor line)

		// Handle preprocessor command
		const char * pb = retval.GetBuffer(0);
		++pb;
		while (::isspace(*pb))
			++pb;

		// Get preprocessor command
		const char * pb2;
		for (pb2 = pb; ::isalpha(*pb2) || *pb2 == '_'; ++pb2)
			; // nothing here
		CString tmp(pb, int(pb2-pb));
		pb = pb2;

		if (tmp == "ifdef" || tmp == "ifndef")
		{
			++pp_nesting_.back();
			if (!skipping)
			{
				while (::isspace(*pb))
					++pb;
				if (::isalpha(*pb) || *pb == '_')
				{
					// Get macro identifier
					for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
						; // nothing here
					ASSERT(pb2 > pb);           // There must be at least one letter

					CString macro_name = CString(pb, int(pb2-pb));
					std::map<CString, CString>::const_iterator pp = pp_def_.find(macro_name);

					if (tmp == "ifdef"  && pp == pp_def_.end() || 
						tmp == "ifndef" && pp != pp_def_.end())
					{
						skipping = 1;   // skip to #endif/#else/#elif
						skip_nesting = pp_nesting_.back();
					}
				}
				else
					throw CString("Expected identifier after #ifdef");
			}
		}
		else if (tmp == "if")
		{
			++pp_nesting_.back();
			if (!skipping)
			{
				int ac = -1;
				ASSERT(pexpr_ != NULL);
				expr_eval::value_t val = pexpr_->evaluate(pb, 0, ac);

				if (val.typ == expr_eval::TYPE_INT)
					skipping = val.int64 != 0 ? 0 : 1;
				else if (val.typ == expr_eval::TYPE_BOOLEAN)
					skipping = val.boolean ? 0 : 1;
				else
					throw CString("Invalid expression in #if");
				skip_nesting = pp_nesting_.back();
			}
		}
		else if (tmp == "elif")
		{
			if (skipping == 0)
			{
				// We are apparently processing #if/#ifdef part (or #elif part) when we got to here -
				// so the following #else/#elif part must be skipped.
				skipping = 2;           // skip to next #endif
				skip_nesting = pp_nesting_.back();
			}
			else if (skipping == 1 && pp_nesting_.back() == skip_nesting)
			{
				ASSERT(skip_nesting <= pp_nesting_.back());
				int ac = -1;
				ASSERT(pexpr_ != NULL);
				expr_eval::value_t val = pexpr_->evaluate(pb, 0, ac);

				if (val.typ == expr_eval::TYPE_INT)
					skipping = val.int64 != 0 ? 0 : 1;
				else if (val.typ == expr_eval::TYPE_BOOLEAN)
					skipping = val.boolean ? 0 : 1;
				else
					throw CString("Invalid expression in #elif");
				skip_nesting = pp_nesting_.back();
			}
		}
		else if (tmp == "else")
		{
			if (skipping == 0)
			{
				// We are apparently processing #if/#ifdef part (or #elif part) when we got to here -
				// so the following #else/#elif part must be skipped.
				skipping = 2;           // skip to #endif (bypassing #else and #elif)
				skip_nesting = pp_nesting_.back();
			}
			else if (skipping == 1 && pp_nesting_.back() == skip_nesting)
			{
				ASSERT(skip_nesting <= pp_nesting_.back());
				skipping = 0;           // Found #else at same level so turn off skipping
			}
		}
		else if (tmp == "endif")
		{
			if (skipping > 0 && pp_nesting_.back() == skip_nesting)
			{
				ASSERT(skip_nesting <= pp_nesting_.back());
				skipping = 0;           // Found #endif at same level so turn off skipping
			}

			if (pp_nesting_.back() == 0)
				AfxMessageBox("Warning: Unexpected extra #endif");
			else
				pp_nesting_.back()--;
		}
		else if (skipping)
		{
			ASSERT(skip_nesting <= pp_nesting_.back());
			// Don't do other preprocessing if conditionally excluded
			continue;
		}
		else if (tmp == "include")
		{
			// Get name of file to include
			while (::isspace(*pb))
				++pb;
			char stop_ch;
            if (*pb == '<')
				stop_ch = '>';
			else if (*pb == '"')
				stop_ch = '"';
			else
				throw CString("Expected file name after #include");
			++pb;
			for (pb2 = pb; *pb2 != '\0' && *pb2 != stop_ch; ++pb2)
				; // nothing here

			CString inc_name(pb, int(pb2-pb));

			// If include is of the form #include <name> then only search in "standard" include directories
			// If include is of the form #include "name" then search current directory first, then include directories
	        WIN32_FIND_DATA wfd;
	        if (stop_ch == '>' || ::FindFirstFile(inc_name, &wfd) == INVALID_HANDLE_VALUE)
	        {
				// Search in our list of include directories
				CString ss = search_path(theApp.GetProfileString("DataFormat", "IncludeFolders"), inc_name);
				// If not found also check %INCLUDE% environment variable
				if (ss.IsEmpty() && search_include_)
					ss = search_path(::getenv("INCLUDE"), inc_name);

				if (!ss.IsEmpty())
					inc_name = ss;
				else
					throw "Could not find include file \"" + inc_name + "\".";
            }

			try
			{
				// Open the file and read all its text
				CFile ff(inc_name, CFile::modeRead|CFile::shareDenyNone);
				size_t flen = size_t(ff.GetLength());
				char *pp = new char[flen+1];
				if (pp == NULL)
					throw "Not enough memory to load include file \"" + inc_name + "\".";
				ff.Read(pp, flen);
				pp[flen] = '\0';

				text_.push_back(CString(pp));
				next_.push_back(text_.back().GetBuffer(0));
				pp_nesting_.push_back(0);
                filename_.push_back(inc_name);
                line_no_.push_back(0);
				delete[] pp;
			}
			catch (CFileException *pfe)
			{
				CString err = ::FileErrorMessage(pfe, CFile::modeRead);
		        pfe->Delete();
				throw err;
			}
			catch (std::bad_alloc)
			{
				throw "Not enough memory to load include file \"" + inc_name + "\".";
			}
		}
		else if (tmp == "define")
		{
			// We store macros and their replacement text in the std::map pp_def_.
			// For macros without parameters this just stores the name and the text. Eg:
			// #define ULONG  unsigned long      => name="ULONG", text="unsigned long"
			// For macros with parameters we also store the number of parameters in the macro
			// name, and where they are substituted in the replacement text using $<param_num>$
			// eg #define ADD(a,b) ((a)+(b))     => name="ADD:2" text="(( $1$ )+( $2$ ))"
			while (::isspace(*pb))
				++pb;

			if (::isalpha(*pb) || *pb == '_')
			{
				// Get macro identifier
				for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
					; // nothing here
				ASSERT(pb2 > pb);           // There must be at least one letter

				CString macro_name = CString(pb, int(pb2-pb));
				pb = pb2;

				//while (::isspace(*pb))    // Don't skip spaces here since '(' must be right after macro name for function-like macro
				//	++pb;
				std::vector<CString> param;
                if (*pb == '(')
				{
					do
					{
						++pb;
						while (::isspace(*pb))
							++pb;
						// Get parameter name
						for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
							; // nothing here
						if (pb2 == pb)
							throw "Error in macro parameters for " + macro_name;

						param.push_back(CString(pb, int(pb2-pb)));
						pb = pb2;
						while (::isspace(*pb))
							++pb;
					} while (*pb == ',');
					if (*pb != ')')
						throw "Error in macro parameters for " + macro_name;
					++pb;
				}
				if (param.size() > 0)
				{
					// Add the number of parameters to the name preceded by a colon
					CString tmp;
					tmp.Format(":%ld", long(param.size()));
					macro_name += tmp;
				}

				while (::isspace(*pb))
					++pb;
				// Get macro text looking for parameters
				CString txt;
				for (pb2 = pb; *pb2 != '\0'; ++pb2)
				{
					if (isalpha(*pb2) || *pb2 == '_')
					{
						const char * pb3;
						// Get what could be the name of a parameter
						for (pb3 = pb2; ::isalnum(*pb3) || *pb3 == '_'; ++pb3)
							; // nothing here
						tmp = CString(pb2, int(pb3-pb2));

						// See if this id is a parameter and replace it with $<param_num>$ if it is
						std::vector<CString>::const_iterator pparam = std::find(param.begin(), param.end(), tmp);
						if (pparam != param.end())
							tmp.Format(" $%ld$ ", long(pparam - param.begin()));  // Put spaces on either side so we get separate tokens (unless ## used - see below)
						txt += tmp;

						pb2 = pb3 - 1;
					}
					else
						txt += *pb2;
				}
				// Allow for pasting using ##
				txt.Replace("$ ##", "$##");
				txt.Replace("## $", "##$");

				txt.TrimLeft();                     // C std says leading/trailing space is removed
                txt.TrimRight();

				// Save the macro
				pp_def_[macro_name] = txt;
			}
			else
				throw CString("Expected macro name after #define");
		}
		else if (tmp == "undef")
		{
			while (::isspace(*pb))
				++pb;

			if (::isalpha(*pb) || *pb == '_')
			{
				// Get macro identifier
				for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
					; // nothing here
				ASSERT(pb2 > pb);           // There must be at least one letter

				CString macro_name = CString(pb, int(pb2-pb));
                pp_def_.erase(macro_name);
			}
			else
				throw CString("Expected macro name after #undef");
		}
		else if (tmp == "pragma")
		{
			while (::isspace(*pb))
				++pb;

			if (::isalpha(*pb) || *pb == '_')
			{
				// Get pragma name
				for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
					; // nothing here
				ASSERT(pb2 > pb);           // There must be at least one letter
				tmp = CString(pb, int(pb2-pb));
				pb = pb2;

				if (tmp == "pack")
				{
					while (::isspace(*pb))
						++pb;
					if (*pb != '(')
                        throw CString("Expected #pragma pack parameters");

					++pb;
					while (::isspace(*pb))
						++pb;
					if (*pb == ')')
					{
						// Restore default packing
						pack_.back() = packing_default_;
					}
					for (pb2 = pb; ::isalnum(*pb2) || *pb2 == '_'; ++pb2)
						; // nothing here
					tmp = CString(pb, int(pb2-pb));
					pb = pb2;
					if (tmp == "pop")
					{
						if (pack_.size() > 0)
							pack_.pop_back();
						continue;
					}
					else if (tmp == "push")
					{
						ASSERT(pack_.size() > 0);
						pack_.push_back(pack_.back());

						while (::isspace(*pb))
							++pb;
						if (*pb == ')')
							continue;
						else if (*pb != ',')
				            throw CString("Error in #pragma pack parameters");
						++pb;
						while (::isspace(*pb))
							++pb;
						for (pb2 = pb; ::isdigit(*pb2); ++pb2)
							; // nothing here
						tmp = CString(pb, int(pb2-pb));
						pb = pb2;
					}
					int val = atoi(tmp.GetBuffer(0));
					if (val == 1 || val == 2 || val == 4 || val == 8 || val == 16)
					{
						pack_.back() = val;
					}
					else
				        throw CString("Invalid pack value in #pragma pack");
					
				}
				// else - ignore unhandled pragmas
			}
			else
				throw CString("#pragma name not found");
		}
		else if (tmp == "line")
			; // Just ignore #line directives
		else if (tmp == "error")
			throw "Preprocessor #error - " + CString(pb);
		else
			throw CString("Unknown preprocessor command - #") + tmp;
	}

	ASSERT(0);
	return "";
}

// Search for include file in list of directories
//     - returns the full path of the file if found or an empty string if not
// inc = list of directories separated by semicol (;) - eg "D:\\include;d:\\include\\sys"
//     - may be NULL in which case an empoty string is returned
// name = the name of the include file to look for
CString TParser::search_path(LPCTSTR inc, LPCTSTR name)
{
    CString str_include(inc);
    std::vector<CString> tmp;                   // array of the dir names
    std::vector<LPCTSTR> dirs;                  // array of pointers into tmp
    tmp.push_back(CString());
    for (int ii = 0; AfxExtractSubString(tmp.back(), str_include, ii, ';'); ++ii)
    {
        dirs.push_back((LPCTSTR)tmp.back());
        tmp.push_back(CString());
    }
    dirs.push_back((LPCTSTR)0);                 // terminate 

    char inc_path[_MAX_PATH];
    strcpy(inc_path, name);
    if (::PathFindOnPath(inc_path, &dirs[0]))
		return CString(inc_path);
	else
		return CString();
}

// All tokens that consist of more than one punctuation character.
char *TParser::ctokens[] =
{
	"::*",
	"::",  // Must be after "::*" in this list
	".*",
	"...",
	"<=",
	"<<=",
	"<<",  // Must be after "<<=" 
	">=",
	">>=",
	">>",  // Must be after ">>=" 
	// "##",  // this is handled during preprocessing
	"-=",
	"--",
	"->*",
	"->",  // Must be after "->*"
	"+=",
	"++",
	"*=",
	"/=",
	"/*",
	"//",
	"%=",
	"^=",
	"&=",
	"&&",
	"|=",
	"||",
	"==",
	"!=",
	NULL
};

// Get next token without advancing past it
CString TParser::peek_next()
{
	// First check if we have already peeked ahead for it
	if (next_token_.IsEmpty())
		next_token_ = get_next();
	return next_token_;
}

CString TParser::get_next()
{
	CString retval;

	// First check if we have already peeked ahead for it
	if (!next_token_.IsEmpty())
	{
		retval = next_token_;
		next_token_ = "";
		return retval;
	}

	// Skip any whitespace
	while (::isspace(*pp_))
		++pp_;

	// Check if we have reached the end of this line of text
	if (*pp_ == '\0')
	{
		str_ = get_pp_line();           // Get next line with preprocessing done
		if (str_.IsEmpty())
		{
			pp_ = "";
			return "";                  // That's all folks
		}

		pp_ = str_.GetBuffer(0);         // Set current input ptr to start of the new line
	}

	if (::isalpha(*pp_) || *pp_ == '_')
	{
		// Get keyword/identifier
		const char * pb;
		for (pb = pp_; ::isalnum(*pb) || *pb == '_'; ++pb)
			; // nothing here

		retval = CString(pp_, int(pb - pp_));
		pp_ = pb;
	}
	else if (*pp_ == '"')
	{
		// Get the string
		const char * pb = pp_ + 1;
		while (*pb != '\0' && *pb != '"')
		{
			if (*pb == '\\')
				++pb;
			++pb;
		}
		last_string_ = CString(pp_+1, int(pb - (pp_+1)));
		pp_ = pb;
		if (*pp_ == '"')
			++pp_;

		// Return quote (") to flag that it was a string
		retval = '"';
	}
	else if (*pp_ == '\'')
	{
		// Get the "character"
		const char * pb = pp_ + 1;
		is_real_ = false;
		last_int_ = 0;
		while (*pb != '\0' && *pb != '\'')
		{
			if (*pb == '\\')
			{
				if (*++pb == '\0')
					break;
			}
			last_int_ = (last_int_ << 8) + (unsigned char)*pb;
			++pb;
		}
		pp_ = pb;
		if (*pp_ == '\'')
			++pp_;

		// Return character value as a number
		retval.Format("%d", int(last_int_));
	}
	else if (::isdigit(*pp_) || *pp_ == dec_point_ && ::isdigit(*(pp_+1)))
	{
		// Get the number (integer or float)
		if (*pp_ == dec_point_)
			retval = "0";         // Add leading zero (so first char is always a digit)

		bool is_hex = false;
		is_real_ = false;

		const char * pb = pp_;

		// Get number
		if (_strnicmp(pb, "0x", 2) == 0)
		{
			is_hex = true;
			pb += 2;
		}
		if (is_hex)
			while (::isxdigit(*pb))
				++pb;
		else
			while (::isdigit(*pb))
				++pb;

		if (!is_hex && *pb == dec_point_)
		{
			is_real_ = true;
			// Skip fractional part
			++pb;
			while (::isdigit(*pb))
				++pb;
		}
		if (!is_hex && ::toupper(*pb) == 'E')
		{
			is_real_ = true;
			// Skip exponent
			++pb;
			if (*pb == '+' || *pb == '-') // Exponent sign
				++pb;
			while (::isdigit(*pb))
				++pb;
		}

		if (is_real_)
		{
			// Get value
			char *end;
			last_real_ = ::strtod(pp_, &end);
			ASSERT(end == pb);

			// Skip suffix
			if (::toupper(*pb) == 'F' || ::toupper(*pb) == 'L')
				++pb;
		}
		else
		{
			// Get value
#if _MSC_VER >= 1300
			char *end;
			last_int_ = ::_strtoi64(pp_, &end, 0 /*is_hex ? 16 : (*pp_ == '0') ? 8 : 10*/);
			ASSERT(end == pb);
#else
            last_int_ = _atoi64(pp_);    // does not handle "0x"
#endif

			// Skip suffix
			while (::toupper(*pb) == 'U' || ::toupper(*pb) == 'L')
				++pb;
		}

		// Return number as string
		retval += CString(pp_, int(pb - pp_));
		pp_ = pb;
	}
	else
	{
		// Just punctuation, so check for multi-char tokens
		size_t len = 0;
		for (char ** ptok = ctokens; *ptok != NULL; ++ptok)
		{
			len = strlen(*ptok);
			if (strncmp(pp_, *ptok, len) == 0)
				break;
		}
		if (*ptok == NULL)
		{
			// Just use next punct char as token
			retval = CString(*pp_);
			++pp_;
		}
		else
		{
			// Return the multi-char token found
			ASSERT(len > 1);
			retval = CString(pp_, int(len));
			pp_ += len;
		}
	}

	return retval;
}

// Parse a list of declarations
// outer_name is enclosing class/struct name - used for check for c'tor + saving class const names (class_name::const_name)
// max_align is returned alignment requirements for this class - depends on packing and size of largest member
// is_union is just used to indicate that all elements are at same location (union not struct)
CXmlTree::CFrag TParser::parse_all(LPCTSTR outer_name, long &max_align, bool is_union /*false*/)
{
	long curr_offset = 0;               // Current end of structure
	long max_size = 0;                  // Size of largest encountered (used for unions)
	max_align = 1;						// Biggest pack value found so far
	bool is_virtual = false;            // Used to work out if we need to add a vtable ptr
	bool is_class = false;              // True if in class/struct, false if just parsing var declarations
	int bits_used = 0;                  // Number of bits in immediately preceding bit-fields
	int bitfield_size;                  // Size of preceding bit-field or zero if not a bit-field
	int last_size = 0;					// Size of storage unit of preceding bit-field

	CXmlTree::CElt root;                // These are dummy params to find_elt that are not used
	int node_num;

	ASSERT(ptree_ != NULL);
	CXmlTree::CFrag retval(ptree_);
	CString ss = get_next();

	if (ss == "{" || ss == ":")
	{
		if (ss == ":")
		{
			ASSERT(!is_union);
			// get base classes
			ss = get_next();
			for (;;)
			{
				// Skip modifiers
				while (ss == "virtual" || ss == "public" || ss == "protected" || ss == "private" || ss == "internal")
				{
					if (ss == "virtual")
						is_virtual = true;
					ss = get_next();
				}

				// Get scoped class name
				while (peek_next() == "::" || peek_next() == ".")  // C# uses "." not "::"
				{
					ss += get_next();   // Add ::
					ss += get_next();   // Add next ID
				}

				CXmlTree::CElt base = find_elt(ss, root, node_num);
				if (base.IsEmpty())
				    throw CString("Unknown base class type - ") + ss;
				base = CXmlTree::CElt(base.m_pelt, ptree_);    // Same node but with diff assoc. parent

				// We need to handle padding for multiple inheritance
				long curr_size = get_size(ss);
				ASSERT(curr_size > 0);
				long curr_pack = get_pack(ss);
				ASSERT(curr_pack == 1 || curr_pack == 2 || curr_pack == 4 || curr_pack == 8);

				ASSERT(pack_.size() > 0);
				long align = min(curr_pack, pack_.back());
				if (align > max_align)
					max_align = align;
				long pad = align - curr_offset%align;
				if (pad < align)
				{
					CString tmp;
					// Add padding
					CXmlTree::CElt pad_ee("data", ptree_);
                    // Leave name attribute empty so field is not visible in view mode
					//tmp.Format("Fill$%d", int(pad_count_++));
					//pad_ee.SetAttr("name", tmp);
					pad_ee.SetAttr("type", "none");
					tmp.Format("%ld", long(pad));
					pad_ee.SetAttr("len", tmp);
					tmp.Format("Padding to align to %ld-byte boundary", long(align));
					pad_ee.SetAttr("comment", tmp);
					retval.AppendClone(pad_ee);

					curr_offset += pad;
				}
				//CXmlTree::CElt new_ee(base.m_pelt, ptree_);
				//CXmlTree::CElt ee = retval.AppendClone(new_ee);
				CXmlTree::CElt ee = retval.AppendClone(base);
				//ee.SetAttr("name", "__super");         // VC++ keyword for base class ptr
				ee.SetAttr("name", "base_class_" + ee.GetAttr("name"));   // indicate its a base class element

				ss = get_next();
				if (ss != ",")
					break;
				ss = get_next();
			}
		}
		is_class = true;

		ss = get_next();
	}

	while (!ss.IsEmpty() && ss != "}")
	{
		// Get next type declaration/definition
		bool is_typedef = false;        // If typedef we just store the type (in custom_types_)
		bool is_ignored = false;        // We don't store some things (eg static declaration in a class)
		bool is_const   = false;		// If const it might be just declaring a value
		bool is_unknown = false;        // If the data type name is not found (but could still be ptr to such)
		long curr_size = -1;            // The number of bytes for current base type
		long curr_pack = -1;            // Padding requirements for current base type

        // Check and skip modifiers
        ss = skip_modifiers(ss, is_typedef, is_ignored, is_virtual, is_const, is_class);

		if (ss == "struct" || ss == "class" || ss == "enum" || ss == "union")
		{
			bool is_enum  = ss == "enum";
			bool is_union = ss == "union";

			if (peek_next() == "{")     // eg struct { ... } var;
				ss = anon_type_name;    // since no name given store using a temp name
			else
				ss = get_next();        // get type (class/struct) name

			// Include scope as part of name (eg "std::string")
			while (peek_next() == "::" || peek_next() == ".")  // C# uses "." not "::"
			{
				ss += get_next();   // Add scope operator
				ss += get_next();   // Add next ID
			}

			if (peek_next() == "{" || peek_next() == ":")
			{
				if (is_enum)
				{
					CString enum_name = ss;     // save the name for this "type"

					ss = get_next();
					ASSERT(ss == "{");

					CString enum_str = ss;
					ss = get_next();
					__int64 value = 0;          // Value of current enum constant (first defaults to zero)
					while (!ss.IsEmpty() && ss != "}")
					{
						CString const_name = ss;
						enum_str += ss;
						ss = get_next();
						if (ss == "=")
						{
							CString expr;
							while ((ss = get_next()) != "," && ss != "}" && !ss.IsEmpty())
								expr += ss;

							int ac = -1;
							ASSERT(pexpr_ != NULL);
							expr_eval::value_t val = pexpr_->evaluate(expr, 0, ac);

							if (val.typ != expr_eval::TYPE_INT)
				                throw CString("Invalid enum constant - ") + expr;
							value = (long)val.int64;

							CString value_str;
							value_str.Format("%ld", long(value));
							enum_str += "=" + value_str;
						}
						custom_consts_[const_name] = value;  // Add to our list of custom constants
						if (outer_name[0] != '\0')
							custom_consts_[CString(outer_name)+"::"+const_name] = value;   // Also add value with scope of containing class

						if (ss == ",")
							ss = get_next();
						enum_str += ",";

						++value;
					}
					enum_str += "}";

					// Add enum to custom types
					CXmlTree::CElt new_ee = find_elt(enum_type_name, root, node_num); // Get default enum (mainly for "len" attrib)
					if (new_ee.IsEmpty())
				        throw CString("Enum type missing from standard type list - \"_standard_types.xml\"");

					// Move node from std types to custom types and insert clone of it
					new_ee = custom_types_.GetRoot().InsertClone(CXmlTree::CElt(new_ee.m_pelt, &custom_types_));
					new_ee.SetAttr("name", enum_name);
					if (enum_name != anon_type_name)
						new_ee.SetAttr("type_name", "enum " + enum_name);
					new_ee.SetAttr("domain", enum_str);

					//CString debug_string = custom_types_.DumpXML();
					ss = enum_name;     // Set ss back to type name since it is used below to find the type of any decls
				}
				else
				{
					// Get the class/struct/union defn
					long pack = -1;             // Packing requirements for the new STRUCT
					CXmlTree::CFrag cdef = parse_all(ss, pack, is_union);

					if (!cdef.IsEmpty())
					{
						// Create "STRUCT" element and add it to the custom list
						CXmlTree::CElt new_ee = custom_types_.GetRoot().InsertNewChild("struct");
						new_ee.SetAttr("name", ss);
						new_ee.SetAttr("type_name", ss);
						CString pack_str;
						pack_str.Format("%ld", long(pack));
						new_ee.SetAttr("pack", pack_str);
						cdef.InsertKids(&new_ee);
					}
					else
						is_ignored = true;       // signal not to use this type (we can't use an empty struct)
				}
			}
		}
		else
		{
			// Include scope as part of name (eg "std::string")
			while (peek_next() == "::" || peek_next() == ".")  // C# uses "." not "::"
			{
				ss += get_next();   // Add scope operator
				ss += get_next();   // Add next ID
			}
		}


		// Check for multi-word types (signed char, long double etc)
		if (ss == "long" || ss == "signed" || ss == "unsigned")
		{
			CString next = peek_next();
			if (next == "char"   || next == "short"   || next == "int"     || next == "long"    || 
				next == "__int8" || next == "__int16" || next == "__int32" || next == "__int64" ||
				next == "double")
			{
				ss += "$" + next;
				(void)get_next();
				if (peek_next() == "int")    // ignored "int" part of 3 word types: signed/unsigned short/long int
					(void)get_next();
			}
		}

		if (peek_next() == "<")
		{
			// Include <template_spec> in the type name
			CString tmp = get_next();
			int nesting = 0;
			while (!tmp.IsEmpty())
			{
				ss += tmp;              // Add token to type name
				if (tmp == "<")
					++nesting;
				else if (tmp == ">" && --nesting == 0)
					break;
				tmp = get_next();
			}
		}

        ss = skip_modifiers(ss, is_typedef, is_ignored, is_virtual, is_const, is_class);

		bool is_func = false;       // Is it a function?
        if (ss == outer_name && peek_next() == "(" || ss == "~" && (ss=get_next()) == outer_name)
            is_func = true;         // c'tor or d'tor

		// At this point ss contains the base type name (eg "int", std::vector<int>, class/struct/union/enum name etc)
        CString type_name = ss;

		CXmlTree::CElt base = find_elt(ss, root, node_num);
		if (base.IsEmpty())
			is_unknown = true;								// Keep going in case it's a pointer (to unknown type)
		else
			base = CXmlTree::CElt(base.m_pelt, ptree_);     // Same node but with diff assoc. parent
		curr_size = get_size(ss);
		curr_pack = get_pack(ss);

		ss = get_next();
		while (!ss.IsEmpty() && ss != ";" && ss != "}")
		{
			// We need to distinguish size of base type from size of variables in declarators. Consider:
			//    char a, *b, c[3];
			// The base type has size (curr_size) 1, as does 'a', since it has the base type.
			// However, b has size 4 (pointer) and c has size 3 (array).
			long decl_curr_size = curr_size; // size of var in decl (!= curr_size if ptr, etc)
			long decl_curr_pack = curr_pack;

			CXmlTree::CElt actual;      // XML element representing the actual variable as we currently know it
			if (!is_unknown)
				actual = base;          // Leave empty if unknown type
			int nest_level = 0;         // Current level of brackets ()
			int ptr_level = -1;         // Level at which * found or -1 if not found
			CString var_name;           // Actual variable name
            if (is_func)
                var_name = type_name;   // c'tor or d'tor so force parse of params

			// Get each declarator (incl var name) for this type (eg f(int), a[4], (*f())() etc)
			// Stop at the end of the declarator:
			//    int a,                     // end of declarator (presumably another one next)
			//    int a;                     // end of declaration
			//    { struct type {...} }      // missing semi-colon before }
			//    const int a =              // initialiser after declarator
			//    int f() const              // const member function
			//    int a :                    // bit-field
			while (!ss.IsEmpty() && ss != "," && ss != ";" && ss != "{" && ss != "}" && ss != "=" && ss != ":")
			{
				if (ss == "*" || ss == "&" || (::isalpha(ss[0]) || ss[0] == '_') && peek_next() == "::*")
				{
					// Type to be stored is a ptr (but may be array of ptrs, or func ptr - handled later)
					actual = find_elt(ptr_type_name, root, node_num);
					if (actual.IsEmpty())
				        throw CString("Pointer type missing from standard type list - \"_standard_types.xml\"");
					actual = CXmlTree::CElt(actual.m_pelt, ptree_);    // Same node but with diff assoc. parent
					long pointer_len = atol(actual.GetAttr("len"));
					if (pointer_len != 2 && pointer_len != 4 && pointer_len != 8)
						throw CString("Invalid pointer length in \"_standard_types.xml\" - ") + ptr_type_name;
					decl_curr_size = decl_curr_pack = pointer_len;

					if (ptr_level == -1)
						ptr_level = nest_level;

					if (::isalpha(ss[0]) || ss[0] == '_')
						ss = get_next();                  // Skip to "::*"

					is_unknown = false; // even if base type is unknown we know this is a pointer
				}
				else if (::isalpha(ss[0]) || ss[0] == '_')
				{
					// Here it is - the actual variable name
					var_name = ss;
                    while (peek_next() == "::" || peek_next() == ".")
                    {
                        var_name += get_next();
                        var_name += get_next();
                    }
                    if (var_name == "operator")
                        var_name += get_next();
				}
				else if (!var_name.IsEmpty() && (ss == "(" || ss == "<"))
                {
					// Function or function ptr (or array of func ptrs)
					if (ptr_level <= nest_level)
					{
						// Function declaration/definition
						is_func = true;  // signal that we have a function
					}
					else if (!is_func)
					{
						// At this level it must be a ptr to function
						if (actual.GetName() == "data")
						{
							// Convert ptr to function ptr
							ASSERT(actual.GetAttr("type") == "int" && actual.GetAttr("name") == ptr_type_name);
							actual = find_elt(func_ptr_type_name, root, node_num);
							if (actual.IsEmpty())
								throw CString("Function pointer type missing from standard type list - \"_standard_types.xml\"");
							actual = CXmlTree::CElt(actual.m_pelt, ptree_);     // Same node but with diff assoc. parent
							long pointer_len = atol(actual.GetAttr("len"));
							if (pointer_len != 2 && pointer_len != 4 && pointer_len != 8)
								throw CString("Invalid function pointer length in \"_standard_types.xml\" - ") + func_ptr_type_name;
							decl_curr_size = decl_curr_pack = pointer_len;
						}
					}
					// Else: We already know its a function (is_func == true) so we can just ignore subsequent
					// parameter lists - this can occur for function returning function ptr - eg void (*f())();

					// Skip any template specifier
					if (ss == "<")
					{
						int nesting = 0;
						while (!ss.IsEmpty())
						{
							if (ss == "<")
								++nesting;
							else if (ss == ">" && --nesting == 0)
								break;
							ss = get_next();
						}
					}

					ASSERT(ss == "(");
					// Now skip the (function or function ptr) parameter list
					int nesting = 0;
					while (!ss.IsEmpty())
					{
						if (ss == "(")
							++nesting;
						else if (ss == ")" && --nesting == 0)
							break;
						ss = get_next();
					}
				}
				else if (!var_name.IsEmpty() && ss == "[")
				{
					// Array or ptr to array
					CString expr;
					while ((ss = get_next()) != "]" && !ss.IsEmpty())
						expr += ss;
					if (ptr_level <= nest_level)
					{
						CString val_str = "0";  // default to zero elements for empty dimension []
						if (!expr.IsEmpty())
						{
							// Get index (constant int expr) and enclose current element in array
							int ac = -1;
							ASSERT(pexpr_ != NULL);
							expr_eval::value_t val = pexpr_->evaluate(expr, 0, ac);

							if (val.typ != expr_eval::TYPE_INT)
								throw "Could not evaluate expression - " + expr;
							decl_curr_size *= long(val.int64);
							val_str.Format("%ld", long(val.int64));
						}

						// xxx this gives reversed index order (left to right not right to left)
						CXmlTree::CElt new_ee("for", ptree_);
						new_ee.SetAttr("count", val_str);
						CXmlTree::CElt child = new_ee.InsertClone(actual);
						child.RemoveAttr("name");                 // The element in a for does not need a name (accessed using [])
						child.RemoveAttr("comment");
						actual = new_ee;
					}
					// Else: Do nothing since it's still a pointer (ptr to array of whatever it pointed to before)
				}
				else if (ss == "(")
				{
					++nest_level;
				}
				else if (ss == ")")
				{
					nest_level--;
				}
				else if (ss == "const")
				{
					// "const" can be placed after decl spec
					is_const = true;
				}
				else
				{
					ASSERT(0);
					break;
				}

				ss = get_next();
			}
			if (nest_level != 0)
				throw CString("Matching right bracket ')' not found");

			bitfield_size = 0;
			if (ss == ":")
			{
				if (is_func)
				{
					// Must be ctor initialiser list - skip to start of function statement
					while (ss != "{")
						ss = get_next();
				}
				else
				{
					// Must be bitfield
					if (actual.GetAttr("type") != "int")
						throw CString("Bitfields can only use integer types");

					// Get const expression for size of bit-field
					CString expr;
					while ((ss = get_next()) != "," && ss != ";" && ss != "{" && ss != "}" && !ss.IsEmpty())
						expr += ss;

					// Get index (constant int expr) and enclose current element in array
					int ac = -1;
					ASSERT(pexpr_ != NULL);
					expr_eval::value_t val = pexpr_->evaluate(expr, 0, ac);

					if (val.typ != expr_eval::TYPE_INT)
						throw CString("Bitfield size must be an integer (or constant integer expression)");
					bitfield_size = int(val.int64);
					CString tmp;
					tmp.Format("%ld", long(bitfield_size));

					actual = find_elt(bitfield_type_name, root, node_num);
					if (actual.IsEmpty())
						throw CString("Bit-field unit (bit-field$) missing from standard type list - \"_standard_types.xml\"");
					actual = CXmlTree::CElt(actual.m_pelt, ptree_);    // This is still the node in standard types but with diff assoc. parent

					// Note this actually modifies the node in "_standard_types" but since we set the number of bits
					// each time we use it, and we don't save back to file this doesn't matter.
					actual.SetAttr("bits", tmp);

					int bitfield_unit_size = atol(actual.GetAttr("len"));
					if (bitfield_unit_size != 1 && bitfield_unit_size != 2 && bitfield_unit_size != 4 && bitfield_unit_size != 8)
						throw CString("Invalid bitfield storage unit length in \"_standard_types.xml\" - ") + bitfield_type_name;
					decl_curr_size = decl_curr_pack = bitfield_unit_size;
					ASSERT(last_size == 0 || last_size == bitfield_unit_size);
				}
			}

			if (is_func)
			{
				//if (ss == "const")    // const has now been filtered out (above)
				//	ss = get_next();

				// Nothing is stored for a function but we should skip body if present
				if (ss == "{")
				{
					int nesting = 0;
					while (!ss.IsEmpty())
					{
						if (ss == "{")
							++nesting;
						else if (ss == "}" && --nesting == 0)
						{
							ss = get_next();    // skip closing "}" too
							break;
						}
						ss = get_next();
					}
				}
                break;   // There is only one decl if its a function
			}
			else if (is_unknown)
			{
                // CString ss(next_.back());
                throw "Unknown type \"" + type_name + "\"";
			}
			else if (!var_name.IsEmpty())
			{
			    ASSERT(decl_curr_size > 0);
			    ASSERT(decl_curr_pack == 1 || decl_curr_pack == 2 || decl_curr_pack == 4 || decl_curr_pack == 8);

				if (is_typedef)
				{
					// Just store the type we extracted in our custom types (XML) container
					CXmlTree::CElt ee(actual.m_pelt, &custom_types_);
					ee = custom_types_.GetRoot().InsertClone(ee);
					CString type_str = ee.GetName();
					CString name_str = ee.GetAttr("name");
                    ee.SetAttr("name", var_name);
                    ee.SetAttr("type_name", var_name);
					if (type_str == "data" && name_str != ptr_type_name && name_str != func_ptr_type_name && name_str != enum_type_name)
						ee.SetAttr("comment", "typedef " + type_name);
				}
				else if (!is_ignored)
				{
					// Check if we need to terminate the previous bit-field
					if (bits_used > 0 && (bitfield_size == 0 || bits_used + bitfield_size > last_size*8))
					{
						curr_offset += last_size;
						bits_used = 0;
					}

					// If we are not in the middle of a bit-field
					if (bits_used == 0 || bitfield_size == 0)
					{
						// See if we need to add any padding
						ASSERT(pack_.size() > 0);
						long align = min(decl_curr_pack, pack_.back());
						if (align > max_align)
							max_align = align;
						long pad = align - curr_offset%align;
						if (!is_union && pad < align)
						{
							CString tmp;
							// Add padding
							CXmlTree::CElt pad_ee("data", ptree_);
                            // Leave name attribute empty so field is not visible in view mode
							//tmp.Format("Fill$%d", int(pad_count_++));
							//pad_ee.SetAttr("name", tmp);
							pad_ee.SetAttr("type", "none");
							tmp.Format("%ld", long(pad));
							pad_ee.SetAttr("len", tmp);
							tmp.Format("Padding to align to %ld-byte boundary", long(align));
							pad_ee.SetAttr("comment", tmp);
							retval.AppendClone(pad_ee);

							curr_offset += pad;
						}
					}

					if (is_union)
					{
						// Insert into "JUMP" element.  The new position is 0 relative to current position so the
						// file position is not changed but when the JUMP "returns" the file position reverts back.
						// This effectively means that all elements of the union use the same bit of the file.  The
						// bytes for the union (= size of largest element) are reserved below using data type "none".
						CXmlTree::CElt new_ee("jump", ptree_);
						new_ee.SetAttr("offset", "0");
						new_ee.SetAttr("origin", "current");
						CXmlTree::CElt child = new_ee.InsertClone(actual);
						child.SetAttr("name", var_name);        // name the "data" elt (not "jump" elt which does not have name attr)
						child.RemoveAttr("comment");
						retval.AppendClone(new_ee);

						// Keep track of largest union member seen
						if (decl_curr_size > max_size)
							max_size = decl_curr_size;
					}
					else
					{
						// Store an instance of the type and set its name
						//CXmlTree::CElt src_elt(actual.m_pelt, ptree_);
						CXmlTree::CElt ee = retval.AppendClone(actual);
						ee.SetAttr("name", var_name);          // name the elt (data, struct or for)
						ee.RemoveAttr("comment");
					}

					bits_used += bitfield_size;
					if (bitfield_size != 0)
						last_size = atoi(actual.GetAttr("len"));
					else
					{
						last_size = 0;                        // Store zero for consistency checks
						curr_offset += decl_curr_size;
					}
				}
			}

			if (ss == "=")
			{
				// Scan for end of initialiser
				CString expr;
				while ((ss = get_next()) != ";" && ss != "," && ss != "}" && !ss.IsEmpty())  // not sure that it can occur?
					expr += ss;
				if (is_const)
				{
					int ac = -1;
					ASSERT(pexpr_ != NULL);
					expr_eval::value_t val = pexpr_->evaluate(expr, 0, ac);

					if (val.typ == expr_eval::TYPE_INT)
					{
						custom_consts_[var_name] = val.int64;      // Add to our list of custom constants
						if (outer_name[0] != '\0')
							custom_consts_[CString(outer_name)+"::"+var_name] = val.int64;   // Also add value with scope of containing type
					}
					else
						AfxMessageBox("Definition of constant \"" + var_name + "\" was ignored.\r\n"
						              "(Only integer constants are allowed.)");
				}
			}
			if (ss == ',')
				ss = get_next();
		}

        // If unnamed type remove it again from custom_types_
        if (type_name == anon_type_name)
        {
		    CXmlTree::CElt ee = find_elt(anon_type_name, root, node_num);
            root.DeleteChild(ee);
        }

		if (ss == ";")
			ss = get_next();
	}

	// Terminate properly if last field was a bit-field
	if (bitfield_size > 0)
		curr_offset += last_size;

	// Add vptr (vtable pointer) at the start if class is virtual
	if (is_virtual)
	{
		// Get an element for the pointer
		ASSERT(is_class);
		CXmlTree::CElt vtable_ee = find_elt(ptr_type_name, root, node_num);
		if (vtable_ee.IsEmpty())
			throw CString("Pointer type missing from \"_standard_types.xml\" - ") + ptr_type_name;
		vtable_ee = CXmlTree::CElt(vtable_ee.m_pelt, ptree_);       // Same node but with diff assoc. parent
		long pointer_len = atol(vtable_ee.GetAttr("len"));
		if (pointer_len != 2 && pointer_len != 4 && pointer_len != 8)
			throw CString("Invalid pointer length in \"_standard_types.xml\" - ") + ptr_type_name;
		else if (pointer_len < pack_.back())
		{
			CString ss;
			ss.Format("Sorry, size of pointer for vtable (%ld) must not be less than current packing (%ld)",
				      long(pointer_len), long(pack_.back()));
			throw ss;
		}

		// Update padding info
		curr_offset += pointer_len;
		long align = min(pointer_len, pack_.back());
		if (align > max_align)
			max_align = align;

		// Insert as the first element in the fragment
		CXmlTree::CElt ee = retval.InsertClone(vtable_ee, &retval.GetFirstChild());
		ee.SetAttr("name", "vptr");
		ee.SetAttr("comment", "pointer to vtable");
	}

	// Pad end of structure if necessary
	long pad = max_align - curr_offset%max_align;
	if (is_union && max_size > 0)
	{
		// Add padding equal to size of largest element
		CString tmp;
		CXmlTree::CElt pad_ee("data", ptree_);
        // Leave name attribute empty so field is not visible in view mode
		//tmp.Format("Fill$%d", int(pad_count_++));
		//pad_ee.SetAttr("name", tmp);
		pad_ee.SetAttr("type", "none");
		tmp.Format("%ld", long(max_size));
		pad_ee.SetAttr("len", tmp);
		pad_ee.SetAttr("comment", "padding for size of largest union member - " + tmp);
		retval.AppendClone(pad_ee);
	}
	else if (is_class && pad < max_align)
	{
		// A class/struct needs to be padded at end (eg, in case it is used in an array)
		CString tmp;
		// Add padding
		CXmlTree::CElt pad_ee("data", ptree_);
        // Leave name attribute empty so field is not visible in view mode
		//tmp.Format("Fill$%d", int(pad_count_++));
		//pad_ee.SetAttr("name", tmp);
		pad_ee.SetAttr("type", "none");
		tmp.Format("%ld", long(pad));
		pad_ee.SetAttr("len", tmp);
		tmp.Format("End padding to align to %ld-byte boundary", long(max_align));
		pad_ee.SetAttr("comment", tmp);
		retval.AppendClone(pad_ee);

		curr_offset += pad;
	}
	return retval;
}

// Skip any modifiers, noting anything special
CString TParser::skip_modifiers(CString ss, 
                                bool &is_typedef, bool &is_ignored, bool &is_virtual, bool &is_const,
                                bool is_class)
{
	while (ss == "typedef" || ss == "static" || ss == "extern" || ss == "register" || ss == "auto" || 
			ss == "__cdecl" || ss == "__stdcall" || ss == "__fastcall" ||
			ss == "__inline" || ss == "__forceinline" || ss == "inline" || 
			ss == "const" || ss == "volatile" || ss == "mutable" || ss == "readonly" || 
			ss == "friend" || ss == "explicit" || ss == "virtual" || ss == "typename" || 
			ss == "public" || ss == "private" || ss == "protected" || ss == "internal" || 
			// C# only keywords
			ss == "abstract" || ss == "override" || ss == "sealed" || ss == "unsafe" || ss == "event" || 
			// C++ .Net stuff
			ss == "__gc" || ss == "__nogc" || ss == "__abstract" || ss == "__sealed" || ss == "__interface" || 
			ss == "__value" || ss == "__pin" || ss == "__delegate" || ss == "__property" || ss == "__event" || 
			// DOS/Windows keywords and #defines
			ss == "near" || ss == "far" || ss == "NEAR" || ss == "FAR" || ss == "pascal" || ss == "PASCAL" || 
			ss == "cdecl" || ss == "CDECL" || ss == "WINAPI" || ss == "APIENTRY" || ss == "CALLBACK")
	{
		if (ss == "__declspec" || ss == "__based")
		{
			// Skip the "parameters" for these
			if ((ss = get_next()) == "(")
			{
				// Now skip the (function or function ptr) parameter list
				int nesting = 0;
				while (!ss.IsEmpty())
				{
					if (ss == "(")
						++nesting;
					else if (ss == ")" && --nesting == 0)
						break;
					ss = get_next();
				}
				ss = get_next();
			}
		}
		else if (ss == "public" || ss == "private" || ss == "protected" || ss == "internal")
		{
			ss = get_next();
			if (ss == ":")         // C/C++ but not C#
				ss = get_next();
		}
		else
		{
			if (ss == "static" || ss == "extern" || ss == "friend")
				is_ignored = true;
			else if (ss == "virtual")
				is_virtual = true;
			else if (ss == "typedef")
				is_typedef = true;
			else if (ss == "const")
			{
				is_const = true;
				if (!is_class)
					is_ignored = true;      // const declaration outside a class does not generate anything
			}

			ss = get_next();
		}
	}
    return ss;
}

// Find a type by searching all the (enabled) type files.
// Takes the name of the type ("name" attr) and returns a ptr to the node.
// Also returns the root node for the file and the index of the found node.
CXmlTree::CElt TParser::find_elt(LPCTSTR name, CXmlTree::CElt &root, int &node_num)
{
	// Search standard types then custom types then any optional list
	CXmlTree::CElt ee;
	if (check_std_)
	{
		root = std_types_.GetRoot();
		for (node_num = 0, ee = root.GetFirstChild(); !ee.IsEmpty(); ++node_num, ++ee)
		{
			if (ee.GetAttr("name") == name)
				return ee;
		}
	}

    // Always chekc custom since that is where newly parsed types are placed
	root = custom_types_.GetRoot();
	for (node_num = 0, ee = root.GetFirstChild(); !ee.IsEmpty(); ++node_num, ++ee)
	{
		if (ee.GetAttr("name") == name)
			return ee;
	}

	if (check_win_)
	{
		root = win_types_.GetRoot();
		for (node_num = 0, ee = root.GetFirstChild(); !ee.IsEmpty(); ++node_num, ++ee)
		{
			if (ee.GetAttr("name") == name)
				return ee;
		}
	}
	if (check_common_)
	{
		root = common_types_.GetRoot();
		for (node_num = 0, ee = root.GetFirstChild(); !ee.IsEmpty(); ++node_num, ++ee)
		{
			if (ee.GetAttr("name") == name)
				return ee;
		}
	}

	root = CXmlTree::CElt();            // Make root empty
	return root;                        // Return empty element to signal not found
}

// Return the size of a type (including nested types like STRUCT).
// If the type is not found or the size is not fixed -1 is returned.
long TParser::get_size(LPCTSTR name)
{
	CXmlTree::CElt root, ee;
	int ii;

	ee = find_elt(name, root, ii);
	if (!ee.IsEmpty())
		return ::get_size(root, ii, ii+1);
	else
		return -1;                      // not found
}

// Return the packing requirements for a type (1,2,4,8 or 16) or -1 if not found.
// For data types this is usually just the size of the object except for
// "none" types which have packing requirements of 1.
// For STRUCT types it returns the packing required for the whole structure,
// which is the smaller of the largest element packing value and pack_ value
// that was in force when the STRUCT was parsed.
// For "FOR" and "IF" types it is just the packing requirement of the contained elt.
// Note: Currently the largest data type is 8 bytes so 16 should never be returned.
long TParser::get_pack(LPCTSTR name)
{
	CXmlTree::CElt parent, ee;
	int child_num;

	ee = find_elt(name, parent, child_num);
	if (!ee.IsEmpty())
	{
        CString elt_type = ee.GetName();
		// Find the contained element of any IF or FOR elements
		while (elt_type == "if" || elt_type == "for")
		{
			parent = ee;
			ee = parent.GetFirstChild();
			child_num = 0;
			elt_type = ee.GetName();
		}

        if (elt_type == "data")
		{
			CString data_type = ee.GetAttr("type");
			if (data_type == "none" || data_type == "string")
				return 1;
			else if (data_type == "date" && ee.GetAttr("format") == "systemtime")
				return 2;                       // SYSTEMTIME structure with largest element of size 2 (WORD)
			else
				return ::get_size(parent, child_num, child_num+1);
		}
		else if (elt_type == "struct")
		{
			long pack = atol(ee.GetAttr("pack"));
			if (pack > 0)
				return pack;
            else
                return 1;           // use no padding by default for structs
		}
	}
	return -1;                      // not found
}
