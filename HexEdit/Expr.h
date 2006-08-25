#ifndef EXPR_INCLUDED
#define EXPR_INCLUDED

#include <vector>
#include <map>

#if _MSC_VER >= 1300
#define UNICODE_TYPE_STRING 1     // Use Unicode strings for TYPE_STRING
#else
#undef UNICODE_TYPE_STRING        // Make sure it is not #defined
#endif

#ifdef UNICODE_TYPE_STRING
    typedef CStringW ExprStringType;
#else
    typedef CString ExprStringType;
#endif

class expr_eval
{
public:
    // Types that an expression can have
    enum type_t
    {
        TYPE_NONE,
        TYPE_BOOLEAN, TYPE_INT, TYPE_DATE, TYPE_REAL,
		TYPE_STRING, TYPE_BLOB,
        TYPE_STRUCT, TYPE_ARRAY
    };

    // Tokens that are recognised
    enum tok_t
    {
        TOK_NONE,
        TOK_CONST, TOK_SYMBOL, TOK_DOT,
        TOK_LPAR, TOK_RPAR, TOK_LBRA, TOK_RBRA,
        TOK_ASSIGN, TOK_COMMA, 
        TOK_QUESTION, TOK_COLON,
        // Numeric operations
        TOK_MUL, TOK_DIV, TOK_MOD, TOK_PLUS, TOK_MINUS, 
        TOK_BITNOT, TOK_BITAND, TOK_BITOR, TOK_XOR, TOK_SHL, TOK_SHR, 
        // Boolean operations
        TOK_NOT, TOK_AND, TOK_OR, 
        TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
        //Functions
        TOK_SIZEOF, TOK_ADDRESSOF,
        TOK_ABS, TOK_MIN, TOK_MAX,
        TOK_POW,
		TOK_INT,                        // Convert real or boolean to int
		TOK_ATOI, TOK_ATOF,             // Convert string to number
		TOK_DATE, TOK_TIME,             // Convert string to date,date/time or time
		TOK_YEAR, TOK_MONTH, TOK_DAY,   // Create a date type from individual parts
		TOK_HOUR, TOK_MINUTE, TOK_SECOND,
		TOK_NOW,                        // Current time (date+time)
        TOK_RAND, TOK_STRLEN,
		TOK_LEFT, TOK_RIGHT, TOK_MID,   // Sub string
		TOK_STRCHR, TOK_STRSTR,
		TOK_STRICMP,                    // String compare: 0, -1, +1
        TOK_LTRIM, TOK_RTRIM,           // Trim leading/trailing spaces
		TOK_CHAR,                       // Get single char from string (or -1)
		TOK_SQRT,                       // Floating point square root
		TOK_SIN, TOK_COS, TOK_TAN,      // Transcendental funcs (radians)
		TOK_ASIN, TOK_ACOS, TOK_ATAN,
		TOK_EXP, TOK_LOG,               // e funcs
        TOK_DEFINED,

        TOK_EOL
    };

    struct value_t
    {
        value_t() { error = false; typ = TYPE_NONE; }
        value_t(bool b) { error = false; typ = TYPE_BOOLEAN; boolean = b; }
        value_t(char c) { error = false; typ = TYPE_INT; int64 = c; }
        value_t(int i) { error = false; typ = TYPE_INT; int64 = i; }
        value_t(__int64 i) { error = false; typ = TYPE_INT; int64 = i; }
        value_t(float r) { error = false; typ = TYPE_REAL; real64 = r; }
        value_t(double r) { error = false; typ = TYPE_REAL; real64 = r; }
#ifdef UNICODE_TYPE_STRING
        value_t(const char *s) { error = false; typ = TYPE_STRING; pstr = new ExprStringType(s); }
        value_t(const char *s, int len) { error = false; typ = TYPE_STRING; pstr = new ExprStringType(s, len); }
        value_t(const wchar_t *s) { error = false; typ = TYPE_STRING; pstr = new ExprStringType(s); }
#else
        value_t(const char *s) { error = false; typ = TYPE_STRING; pstr = new ExprStringType(s); }
        value_t(const char *s, int len) { error = false; typ = TYPE_STRING; pstr = new ExprStringType(s, len); }
#endif
		value_t(COleDateTime dt) { error = false; typ = TYPE_DATE; date = (dt.m_status == 0 ? dt.m_dt : -1e30); }

        // copy constructor
        value_t(const value_t &from)
        {
            typ = from.typ;
            error = from.error;
            if (typ == TYPE_STRING)
                pstr = new ExprStringType(*(from.pstr));
            else
                int64 = from.int64;
        }

        // copy assignment operator
        value_t &operator=(const value_t &from)
        {
            if (&from != this)
            {
                if (typ == TYPE_STRING)
                    delete pstr;

                typ = from.typ;
                error = from.error;
                if (typ == TYPE_STRING)
                    pstr = new ExprStringType(*(from.pstr));
                else
                    int64 = from.int64;
            }
            return *this;
        }

        ~value_t() { if (typ == TYPE_STRING) delete pstr; }

        unsigned char typ;      // type of data or TYPE_NONE (eg unknown variable name)
        bool error;             // allows us to propagate an error but continue parsing
        union
        {
            bool boolean;
            __int64 int64;
            double real64;
            ExprStringType *pstr;
			DATE date;
        };
    };

    // Constructor
    expr_eval(int max_radix = 10, bool const_sep_allowed = false);
    ~expr_eval() { free(psymbol_); }

    // Operations
    value_t evaluate(const char *expr, int reference, int &ref_ac, int radix = 10);
    const char *get_error_message() { return error_buf_; }
    int get_error_pos() { return error_pos_; }

    virtual value_t find_symbol(const char *sym, value_t parent, size_t index, int *pac,
                                __int64 &sym_size, __int64 &sym_address) =0;

    static tok_t func_token(const char *buf);// Check if a func name and return corresp. token (or TOK_NONE)

	clock_t VarChanged() { return var_changed_; } // Return when last change was made to any variable

protected:
    // This stores any variables assigned to
    std::map<CString, value_t> var_;  // xxx still need to chnage this to store Unicode strings
	clock_t var_changed_;

private:
    // Current lowest precedence operation
    tok_t prec_lowest(value_t &val) { return prec_comma(val); }

    tok_t prec_comma(value_t &val);    // Comma (sequence) operator
    tok_t prec_assign(value_t &val);   // Assignment
    tok_t prec_ternary(value_t &val, CString &vname);  // Conditional operator
    tok_t prec_or(value_t &val, CString &vname);
    tok_t prec_and(value_t &val, CString &vname);
    tok_t prec_comp(value_t &val, CString &vname);     // Comparisons numeric operands, boolean result
    tok_t prec_bitor(value_t &val, CString &vname);
    tok_t prec_xor(value_t &val, CString &vname);
    tok_t prec_bitand(value_t &val, CString &vname);
    tok_t prec_shift(value_t &val, CString &vname);
    tok_t prec_add(value_t &val, CString &vname);
    tok_t prec_mul(value_t &val, CString &vname);
    tok_t prec_prim(value_t &val, CString &vname);

    bool error(expr_eval::tok_t tt, const char *mess);
    tok_t get_next();           // Get next token
    void skip_ws();             // Skip over white space characters

    double make_real(value_t &val);
    __int64 make_int(value_t &val);
	DATE get_date(const char * ss);  // Get a date (and optionally time) from a string
	DATE get_time(const char * ss);  // Get a time from a string (date part = 0)

    const char *p_;             // Pointer to current position in expression
    const char *saved_;         // Pointer to start of previous token

    value_t last_val_;          // Value of last constant read
    char *psymbol_;             // Name of last symbol read

    int ref_;                   // Provides starting point for symbol lookup
    int *pac_;                  // Returns info about the last symbols accessed
    int const_radix_;           // This is the radix to use for int constants in current expression
    int max_radix_;             // This is the largest radix that will be used in all expressions
    bool const_sep_allowed_;    // Can we have separators in int constants (eg 12,345 and AB 1234)
    char dec_sep_char_;         // Seperator character used in decimal numbers (usually comma)
    char dec_point_;            // Character used for decimal point (usually full-stop)

    char error_buf_[256];       // Error message
    int error_pos_;             // Position within the expression where the error was found

	// We need a way to turn off side effects (changes occurring during expression evaluation)
	// so that certain operation behave as in C expressions (for example, the so called
	// "short-circuiting" of && and ||).  For example, if the left operand to && is FALSE
	// then the result is FALSE and the right operand does not need to be evaluated
	// (but the expression still needs to be parsed since this is an interpreter).
	// So the expression "FALSE && (AA=1)" will return FALSE but importantly no change AA.
	bool changes_on_;           // Says whether operands with side effects (just = at the moment) have an effect
};

#endif
