#ifndef OPTYPES_INCLUDED_
#define OPTYPES_INCLUDED_ 1

// These values are used to distinguish between different operators used in the calculator.
// They are also used with keystroke macros when calculator operations are recorded (see km_binop
// and km_unaryop) and also for commands in the Operations menu (see km_op_binary & km_op_unary).

enum binop_type
{
    binop_none = 0,

    binop_and,
    binop_or,
    binop_xor,

    // Arithmetic operators
    binop_add,
    binop_subtract,
    binop_multiply,

    binop_gtr = 90,                 // Leave the greater value
    binop_less,
    binop_gtr_unsigned,             // For macros only (in calc GTR/LESS are unsigned except in decimal mode)
    binop_less_unsigned,

    binop_mask = 100,               // Above this operands are masked to displayed bits

    binop_divide,
    binop_mod,
    binop_pow,                      // m^n

    // Moved gtr/less before binop_mask so that overflow test not done,
    // but left dummies here for macros recorded in version 1.1
    binop_gtr_old,                  
    binop_less_old,

    // Bit manipulation
    binop_rol,                      // Rotate 1st operand by number of bits in 2nd
    binop_ror,
    binop_lsl,                      // Shifts
    binop_lsr,
    binop_asr,

    // These are just for km_op_binary file operations (not calculator) where the operands are exchanged
    binop_subtract_x,
    binop_divide_x,
    binop_mod_x,
};

enum unary_type
{
    unary_none = 0,

    // Arithmetic operators
    unary_inc,                      // Increment
    unary_dec,
    unary_sign,                     // Change sign

    // Bit manipulation
    unary_not,                      // Invert all bits
    unary_lsl,                      // Shifts

    unary_mask = 100,               // Above this the operand is masked to displayed bits

    unary_lsr,
    unary_asr,
    unary_rol,                      // Rotate one bit position
    unary_ror,
    unary_rev,                      // Reverse bit order

    unary_square,
    unary_squareroot,
    unary_cube,
    unary_factorial,

    // Others
    unary_flip,                     // Flip (reverse) byte order
    unary_at,                       // Get the value at the address
                                    // For Operations/Assign it means the opposite, ie, put from calc to file (see km_op_unary)

    unary_rand,                     // Only curently used for keystroke macro 
    unary_rand_fast,                // for Operations/Random Number command (see km_op_unary)
};

#endif
