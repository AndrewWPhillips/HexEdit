// class Integer v09
// Copyright Cap'n Code (C) 2004 
// "Use this without giving me credit and I'll sue!"
#pragma once
#include <iostream>

class BigInteger
{
public:
	typedef unsigned	__int32 Unit;
	static const unsigned MaxSize = 256;

protected:
	// Number of units that store data
	unsigned	UnitLen;

	// If > 0, (SigBit - 1) is the offset of the highest set bit
	// If < 0, (-SigBit - 2) is the offset of the highest set bit, or is -1 for Integer(-1)
	// Otherwise, Integer(0)
	signed	SigBit;

	// Actual data
	Unit Data[MaxSize];

public:
	BigInteger(void);
	BigInteger(const BigInteger &);
	BigInteger(const Unit *, unsigned Count);
	BigInteger(const char *, unsigned Radix);
	BigInteger(const wchar_t *, unsigned Radix);
	explicit BigInteger(signed long);

	void Not();	// 1's completement
	void Neg(); // 2's completement
	void ShiftLeft();
	bool ShiftRight();

	long GetLong() const;
	void Copy(const Unit *, unsigned Count);
	void FromString(const char *, unsigned Radix);
	void FromString(const wchar_t *, unsigned Radix);
	void MoveTo(BigInteger &);	// Moves data and clears this
	void ToString(char *, unsigned Radix) const;
	void ToString(wchar_t *, unsigned Radix) const;

	unsigned GetUnitLength() const;
	unsigned GetSigUnitLen() const;
	Unit GetUnit(unsigned) const;
	bool GetBit(unsigned) const;

	bool IsZero() const;
	bool IsNegative() const;
	bool IsPositive() const;
	bool IsEven() const;

	BigInteger & operator = (signed long);
	BigInteger & operator = (const BigInteger &);

	bool operator == (const BigInteger &) const;
	bool operator != (const BigInteger &) const;
	bool operator <  (const BigInteger &) const;
	bool operator >  (const BigInteger &) const;
	bool operator <= (const BigInteger &) const;
	bool operator >= (const BigInteger &) const;

	static int Compare(const BigInteger &, const BigInteger &);
	static void Add(const BigInteger &, const BigInteger &, BigInteger &);
	static void Sub(const BigInteger &, const BigInteger &, BigInteger &);
	static void Mul(const BigInteger &, const BigInteger &, BigInteger &); 
	static void Div(const BigInteger &, const BigInteger &, BigInteger & Quo, BigInteger & Rem);
	static void Mod(const BigInteger &, const BigInteger &, BigInteger &);
	static void Exp(const BigInteger &, const BigInteger &, BigInteger &);
	static void ExpMod(const BigInteger &, const BigInteger &, const BigInteger &, BigInteger &);	// Computes A ^ B mod C

protected:
	void _Alloc(unsigned);
	void _ReAlloc(unsigned);
	void _DataShiftLeft(unsigned);
	bool _IsMax() const;
	bool _IsMin() const;
	void _FixSigBit();

	friend std::ostream & operator << (std::ostream & Str, const BigInteger & Int);
};

__inline std::ostream & operator << (std::ostream & Str, const BigInteger & Int)
{
	char Buffer[1024];
	Int.ToString(Buffer, 10);
	Str << Buffer;
	return Str;
}

__inline long BigInteger::GetLong() const
{
	if (IsNegative())
	{
		if ((Data[0] & 0x80000000) == 0)
			return LONG_MIN;
	}
	else if (Data[0] & 0x80000000)
		return LONG_MAX;

	return *(signed long*)Data;
}
