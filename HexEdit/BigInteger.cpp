// class BigInteger v09
// Copyright Cap'n Code (C) 2004 
// "Use this without giving me credit and I'll sue!"

#include "stdafx.h"
#include "BigInteger.h"
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <malloc.h>

BigInteger::BigInteger(void)
:	UnitLen(1), SigBit(0)
{
	Data[0] = 0;
}

BigInteger::BigInteger(signed long l)
{
	if (l > 0)
	{
		unsigned Bit = 0x80000000;
		SigBit = 32;
		while((Bit & l) == 0)
		{
			Bit >>= 1;
			--SigBit;
		}

		UnitLen = SigBit / (sizeof(Unit) * 8);
		UnitLen++;
	}
	else if (l < 0)
	{
		unsigned Bit = 0x80000000;
		SigBit = -33;
		while(Bit & l)
		{
			Bit >>= 1;
			++SigBit;
		}

		UnitLen = -SigBit / (sizeof(Unit) * 8);
		if (-SigBit % (sizeof(Unit) * 8))
			UnitLen++;
	}
	else
	{
		SigBit = 0;
		UnitLen = 1;
	}

	memcpy(Data, &l, UnitLen * sizeof(Unit));
}

BigInteger::BigInteger(const char * Str, unsigned int Radix)
{
	SigBit = 0;
	UnitLen = 1;
	Data[0] = 0;

	if (Radix < 2 || Radix > 36)
		return;
	
	BigInteger R(Radix);

	bool IsNeg = false;

	// First digit
    if (*Str == '-')
	{
		IsNeg = true;
		Str++;
		if (*Str == 0)
			return;	// Bad
	}
	else if (!isalpha(*Str) && !isdigit(*Str))
		return; // Bad

	while(isalpha(*Str) || isdigit(*Str))
	{
		unsigned Digit;
		if (isdigit(*Str))
			Digit = (*Str) - '0';
		else if (isalpha(*Str))
			Digit = (tolower((int)*Str) - (int)'a') + 0xA;
		else
			break;

		if (Digit > Radix)
			break;

		BigInteger::Mul(*this, R, *this);
		BigInteger::Add(*this, BigInteger(Digit), *this);
		++Str;
	}

	if (IsNeg)
		Neg();
}

BigInteger::BigInteger(const wchar_t * Str, unsigned int Radix)
{
	SigBit = 0;
	UnitLen = 1;
	Data[0] = 0;

	if (Radix < 2 || Radix > 36)
		return;
	
	BigInteger R(Radix);

	bool IsNeg = false;

	// First digit
    if (*Str == L'-')
	{
		IsNeg = true;
		Str++;
		if (*Str == 0)
			return;	// Bad
	}
	else if (!iswalpha(*Str) && !iswdigit(*Str))
		return; // Bad

	BigInteger IDigit;
	while(iswalpha(*Str) || iswdigit(*Str))
	{
		unsigned Digit;
		if (iswdigit(*Str))
			Digit = (*Str) - L'0';
		else if (iswalpha(*Str))
			Digit = (towlower((int)*Str) - (int)L'a') + 0xA;
		else
			break;

		if (Digit > Radix)
			break;

		IDigit = Digit;
		BigInteger::Mul(*this, R, *this);
		BigInteger::Add(*this, IDigit, *this);
		++Str;
	}

	if (IsNeg)
		Neg();
}

BigInteger::BigInteger(const Unit * _Data, unsigned Count)
{
	if (Count == 0 || _Data == NULL)
	{
		SigBit = 0;
		UnitLen = 1;
		Data[0] = 0;
		return;
	}

	UnitLen = Count;
	memcpy(Data, _Data, UnitLen * sizeof(Unit));
	_FixSigBit();
}

void BigInteger::Copy(const BigInteger::Unit * _Data, unsigned Count)
{
	::new(this) BigInteger(_Data, Count);
}

BigInteger::BigInteger(const BigInteger & _Copied)
{
	UnitLen = _Copied.UnitLen;
	SigBit = _Copied.SigBit;
	memcpy(Data, _Copied.Data, sizeof(Unit) * _Copied.UnitLen);
}

BigInteger & BigInteger::operator = (signed long l)
{
	::new(this) BigInteger(l);
	return *this;
}

BigInteger & BigInteger::operator = (const BigInteger & _Copied)
{
	if (this != &_Copied)
	{
		::new(this) BigInteger(_Copied);
	}
	return *this;
}

void BigInteger::FromString(const wchar_t * Str, unsigned Radix)
{
	::new(this) BigInteger(Str, Radix);
}

void BigInteger::FromString(const char * Str, unsigned Radix)
{
	::new(this) BigInteger(Str, Radix);
}

void BigInteger::MoveTo(BigInteger & Dest)
{
	Dest.UnitLen = UnitLen;
	Dest.SigBit = SigBit;
	memcpy(Dest.Data, Data, sizeof(Unit) * UnitLen);
	::new(this) BigInteger();
}

void BigInteger::_Alloc(unsigned _UnitLen)
{
	UnitLen = _UnitLen;
	SigBit = 0;

	if (_UnitLen > MaxSize)
		throw;

	Data[0] = 0;
}

void BigInteger::_ReAlloc(unsigned _UnitLen)
{
	Unit Ext = IsNegative() ? 0xFFFFFFFF : 0x0;

	if (_UnitLen == 0)
		++_UnitLen;

	if (_UnitLen > MaxSize)
		RaiseException(EXCEPTION_INT_OVERFLOW, EXCEPTION_NONCONTINUABLE, 0, 0);


	unsigned OldUnitLen = UnitLen;
	UnitLen = _UnitLen;

	for (unsigned i = OldUnitLen; i < UnitLen; ++i)
		Data[i] = Ext;
}

bool BigInteger::_IsMax() const
{
	if (SigBit <= 0)
		return false;

	if (Data[UnitLen-1] != 0x7FFFFFFF)
		return false;

	for (unsigned i = UnitLen - 1; i > 0; --i)
		if (Data[i-1] != 0xFFFFFFFF)
			return false;

	return true;
}

bool BigInteger::_IsMin() const
{
	if (SigBit >= 0)
		return false;

	if (Data[UnitLen-1] != 0x80000000)
		return false;

	for (unsigned i = UnitLen - 1; i > 0; --i)
		if (Data[i-1] != 0)
			return false;

	return true;
}

void BigInteger::Not()
{
	for (unsigned i = 0; i < UnitLen; ++i)
		Data[i] ^= 0xFFFFFFFF;

	// Works out nicely in all cases
	SigBit = -SigBit - 1;
}

// Change size of constants, BYTE PTR and register al when switching types
void BigInteger::Neg()
{
	if (IsZero())
		return;	// Less work

	/*
		INT_MIN = -INT_MAX-1
		Check if _ReAllocation is necessary
	*/
	if (_IsMin())
		_ReAlloc(UnitLen + 1);

	Not();

	unsigned Len = UnitLen;
	__asm
	{
		mov ecx, Len
		// Replaced with:
		mov edx, this
		add edx, BigInteger::Data-4
	CarryLoop:
		add edx, 4	// Increment here, not to disturb flags
		mov eax, DWORD PTR [edx]
		inc eax
		mov DWORD PTR [edx], eax
		loopz CarryLoop	// decrement ecx, then test
	}

	_FixSigBit();
}

void BigInteger::ShiftLeft()	// Change sizes
{
	if (IsZero())
		return;

	if (IsNegative())
		--SigBit;
	else
		++SigBit;

	Unit Carry = 0;
	Unit Part;
	for (unsigned i = 0; i < UnitLen; ++i)
	{
		Part = Data[i];
		Part <<= 1;
		Part |= Carry;
		Carry = (Data[i] & 0x80000000) ? 0x1 : 0x0;
		Data[i] = Part;
	}
    
	if (Carry)
	{
		UnitLen++;
		if (UnitLen > MaxSize)
			RaiseException(EXCEPTION_INT_OVERFLOW, EXCEPTION_NONCONTINUABLE, 0, 0);
		Data[UnitLen-1] = (SigBit < 0) ? 0xFFFFFFFF : 0x1;
	}
}

bool BigInteger::ShiftRight()	// Change sizes
{
	if (IsZero())
		return false;

	if (IsNegative())
	{
		if (SigBit != -1)	// shifting 0xFF should not change sig bit
			++SigBit;
	}
	else
		--SigBit;

	Unit Carry = Data[UnitLen-1] & 0x80000000;
	Unit Part;
	for (int i = UnitLen - 1; i >=0 ; --i)
	{
		Part = Data[i];
		Part >>= 1;
		Part |= Carry;
		Carry = (Data[i] & 0x1) ? 0x80000000 : 0x0;
		Data[i] = Part;
	}
	return Carry != 0;
}

unsigned BigInteger::GetUnitLength() const
{
	return UnitLen;
}

unsigned BigInteger::GetSigUnitLen() const
{
	if (SigBit == 0)
		return 0;

	if (SigBit > 0)
		return (SigBit) / (sizeof(Unit) * 8) + 1;
	else if (SigBit == -1)	// Special case
		return 1;
	else
		return (-SigBit - 1) / (sizeof(Unit) * 8) + 1;
}

bool BigInteger::GetBit(unsigned BitPos) const
{
	unsigned UIndex = BitPos / (sizeof(Unit) * 8);
	unsigned BIndex = BitPos % (sizeof(Unit) * 8);
	return (Data[UIndex] & (0x1 << BIndex)) != 0;
}

void BigInteger::_DataShiftLeft(unsigned Count)
{
	if (!Count)
		return;

	if (UnitLen < GetSigUnitLen() + Count)
		_ReAlloc(GetSigUnitLen() + Count);

	unsigned i = 0;
	for (i = UnitLen - 1; i >= Count; --i)
		Data[i] = Data[i-Count];

	while(i != 0xFFFFFFFF)	// while (i != -1)
		Data[i--] = 0;

	// Easy correction
	if (SigBit > 0)
		SigBit += (Count * sizeof(Unit) * 8);
	else if (SigBit < 0)
		SigBit -= (Count * sizeof(Unit) * 8);
}

bool BigInteger::IsZero() const
{
	return SigBit == 0;
}

bool BigInteger::IsNegative() const
{
	return SigBit < 0;
}

bool BigInteger::IsPositive() const
{
	return SigBit > 0;
}

bool BigInteger::IsEven() const
{
	return (Data[0] & 0x1) && IsNegative();
}

int BigInteger::Compare(const BigInteger & Lhs, const BigInteger & Rhs)
{
	if (Lhs.SigBit != Rhs.SigBit)
		return Lhs.SigBit - Rhs.SigBit;

	// If SigBit = -1, i = 0
	int i = (Lhs.SigBit < 0) ? (- Lhs.SigBit - 2) : (Lhs.SigBit - 1);
	i /= (sizeof(Unit) * 8);

	for (; i >= 0; --i)
	{
		if (Lhs.Data[i] != Rhs.Data[i])
		{
			// Make sure we don't send the wrong sign
			return (Lhs.Data[i] < Rhs.Data[i]) ? -1 : 1;
		}
	}

	return 0;	// Both cases are equal
}

bool BigInteger::operator ==(const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) == 0;
}

bool BigInteger::operator !=(const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) != 0;
}

bool BigInteger::operator < (const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) < 0;
}

bool BigInteger::operator >(const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) > 0;
}

bool BigInteger::operator >=(const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) >= 0;
}

bool BigInteger::operator <=(const BigInteger & _Comp) const
{
	return Compare(*this, _Comp) <= 0;
}

void BigInteger::ToString(char * Dest, unsigned Radix) const
{
	if (IsZero())
	{
		Dest[0] = '0';
		Dest[1] = 0;
		return;
	}
	if (Radix < 2 || Radix > 36)
	{
		Dest[0] = 0;
		return;
	}

	BigInteger Out;
	Out = *this;
	if (IsNegative())
	{
		Out.Neg();
		*Dest = '-';
		++Dest;
	}

	unsigned Len = 0;

	BigInteger Rem;
	BigInteger Rad(Radix);
    while(!Out.IsZero())
	{
		// Uses fast division. R is a single digit
		BigInteger::Div(Out, Rad, Out, Rem);

		if (Rem.Data[0] > 9)
			Dest[Len] = ((char)Rem.Data[0] - 10) + 'A';
		else
			Dest[Len] = (char)Rem.Data[0] + '0';
		++Len;
	}

	Dest[Len] = 0;

	// Reverse digits
	for (unsigned i = 0; i < (Len >> 1); ++i)
	{
		char c = Dest[i];
		Dest[i] = Dest[Len-1-i];
		Dest[Len-1-i] = c;
	}
}

void BigInteger::ToString(wchar_t * Dest, unsigned Radix) const
{
	if (IsZero())
	{
		Dest[0] = L'0';
		Dest[1] = 0;
		return;
	}
	if (Radix < 2 || Radix > 36)
	{
		Dest[0] = 0;
		return;
	}

	BigInteger Out;
	Out = *this;
	if (IsNegative())
	{
		Out.Neg();
		*Dest = '-';
		++Dest;
	}

	unsigned Len = 0;

	BigInteger Rem;
	BigInteger Rad(Radix);
    while(!Out.IsZero())
	{
		// Uses fast division. R is a single digit
		BigInteger::Div(Out, Rad, Out, Rem);

		if (Rem.Data[0] > 9)
			Dest[Len] = ((wchar_t)Rem.Data[0] - 10) + L'A';
		else
			Dest[Len] = (wchar_t)Rem.Data[0] + L'0';
		++Len;
	}

	Dest[Len] = 0;

	// Reverse digits
	for (unsigned i = 0; i < (Len >> 1); ++i)
	{
		wchar_t c = Dest[i];
		Dest[i] = Dest[Len-1-i];
		Dest[Len-1-i] = c;
	}
}

BigInteger::Unit BigInteger::GetUnit(unsigned Index)	const // Size dependent
{
	if (Index < UnitLen)
		return Data[Index];

	// Return out-of-bounds as extension
	return SigBit >= 0 ? 0 : 0xFFFFFFFF;
}

void BigInteger::Add(const BigInteger & Lhs, const BigInteger & Rhs, BigInteger & Sum)
{
	unsigned LLen = Lhs.GetSigUnitLen();
	unsigned RLen = Rhs.GetSigUnitLen();
	unsigned NewSize = max(LLen, RLen) + 1;

	if (&Lhs == &Sum || &Rhs == &Sum)
		Sum._ReAlloc(NewSize);
	else
		Sum._Alloc(NewSize);

	unsigned __int64 Carry = 0;
	unsigned UIndex = 0;
	// Keep on adding until both have reached their end
	while(true)
	{
		if (UIndex >= LLen && UIndex >= RLen)
			break;

		// Already has carry 1 in high dword
		Carry += Lhs.GetUnit(UIndex);
		Carry += Rhs.GetUnit(UIndex);

		Sum.Data[UIndex] = *(Unit*)&Carry;
		Carry >>= 32;
		++UIndex;
	}

	if ((Carry == 0 && UIndex == 1 && Sum.Data[0] == 0 /*Zero*/) || (Lhs.IsNegative() || Rhs.IsNegative()))
	{
		--NewSize;
		Sum._ReAlloc(NewSize);
	}
	else
		Sum.Data[UIndex] = *(Unit*)&Carry;

	Sum._FixSigBit();
}

void BigInteger::_FixSigBit()
{
	bool IsNeg = (Data[UnitLen-1] & 0x80000000) != 0;

	unsigned UIndex;
	if (IsNeg)
	{
		UIndex = UnitLen - 1;
		while(true)
		{
			if (~Data[UIndex] & 0xFFFFFFFF)
				break;

			if (UIndex == 0)
			{
				SigBit = -1;
				return;
			}
			--UIndex;
		}

		SigBit = -33 - (int)UIndex * sizeof(Unit) * 8;
		Unit Bit = 0x80000000;
		while(Bit & Data[UIndex])
		{
			++SigBit;
			Bit >>= 1;
		}
	}
	else
	{
		UIndex = UnitLen - 1;
		while(true)
		{
			if (Data[UIndex] & 0xFFFFFFFF)
				break;

			if (UIndex == 0)
			{
				SigBit = 0;
				return;
			}
			--UIndex;
		}

		SigBit = (UIndex+1) * sizeof(Unit) * 8;//32 + UIndex * sizeof(Unit) * 8;
		Unit Bit = 0x80000000;
		while((Bit & Data[UIndex]) == 0)
		{
			--SigBit;
			Bit >>= 1;
		}
	}
}

void BigInteger::Sub(const BigInteger & Minuend, const BigInteger & Subtrahend, BigInteger & Remainder)
{
	BigInteger SubtrahendNeg(Subtrahend);
	SubtrahendNeg.Neg();
	BigInteger::Add(Minuend, SubtrahendNeg, Remainder);
}



void BigInteger::Mul(const BigInteger & Multiplicand, const BigInteger & Multiplier, BigInteger & Product)
{
	if (Multiplicand.IsZero() || Multiplier.IsZero())
	{
		Product = 0;
		return;
	}

	if (Multiplicand.IsNegative())
	{
		BigInteger newVal1 = Multiplicand;
		newVal1.Neg();
		if (Multiplier.IsNegative())
		{
			BigInteger newVal2 = Multiplier;
			newVal2.Neg();
			BigInteger::Mul(newVal1, newVal2, Product);
			return;	// Double negative
		}
		else
		{
			Mul(newVal1, Multiplier, Product);
			Product.Neg();
			return;
		}
	}
	else if (Multiplier.IsNegative())
	{
		BigInteger newVal2 = Multiplier;
		newVal2.Neg();
		BigInteger::Mul(Multiplicand, newVal2, Product);
		Product.Neg();
		return;
	}

	if (&Multiplicand == &Product || &Multiplier == &Product)
	{
		BigInteger NewVal;
		BigInteger::Mul(Multiplicand, Multiplier, NewVal);
		NewVal.MoveTo(Product);
		return;
	}
	else
	{
		Product = 0;
	}

	// Quick, because the CPU can help
	if (Multiplier.SigBit <= 32 || Multiplicand.SigBit <= 32)
	{
		const BigInteger & Small = (Multiplicand.GetSigUnitLen() == 1) ? Multiplicand : Multiplier;
		const BigInteger & Big   = (&Multiplicand == &Small) ? Multiplier : Multiplicand;

		unsigned BLen = Big.GetSigUnitLen();
		Product._ReAlloc(BLen + 1);

		unsigned __int64 Carry = 0;
		unsigned i;
		for (i = 0; i < BLen; ++i)
		{
			unsigned __int64 BigP = Big.Data[i];
			unsigned __int64 SmallP = Small.Data[0];
			Carry += BigP * SmallP;
			Product.Data[i] = *(Unit*)&Carry;
			Carry >>= 32;
		}

		if (Carry)
			Product.Data[i] = *(Unit*)&Carry;
		if (Carry & 0x80000000)
		{
			// Make sure it's positive
			Product._ReAlloc(BLen+2);
			Product.Data[i+1] = 0;
		}

		Product._FixSigBit();
		return;
	}
	else
	{
		/*	New Hotness method with recursion:
			A * B = ((W << 2) + X) * ((Y << 2) + Z) 
				= (X*Y << 4) + ((W*Z + X*Y) << 2) + X * Z

			P = W * Y
			Q = X * Z
			R = (W + X) * (Y + Z)

			A * B = (P << 4) + ((R - (P + Q)) << 2) + Q
		*/

		const BigInteger & Big		= (Multiplicand.GetSigUnitLen() > Multiplier.GetSigUnitLen()) ? Multiplicand : Multiplier;
		const BigInteger & Small	= (&Multiplicand == &Big) ? Multiplier : Multiplicand;
		Unit Saved;

		unsigned BigSize = Big.GetSigUnitLen();
		unsigned SmallSize = Small.GetSigUnitLen();
		unsigned LoSize = BigSize / 2;
		unsigned HiSize = BigSize - LoSize;

		BigInteger W(Big.Data + LoSize, HiSize);
		// Make sure X is negative because of leading bit
		Saved = Big.Data[LoSize];
		*(Unit*)&Big.Data[LoSize] = 0;
		BigInteger X(Big.Data, LoSize + 1);	// +1 makes sure it is not negative
		*(Unit*)&Big.Data[LoSize] = Saved;

		BigInteger Z;
		BigInteger Y;
		if (SmallSize < LoSize)
		{
			// Y defaults to zero
			// No need to save leading Unit
			Z.Copy(Small.Data, SmallSize);
		}
		else
		{
            if (SmallSize < LoSize + HiSize)
				Y.Copy(Small.Data + LoSize, SmallSize - LoSize);
			else
				Y.Copy(Small.Data + LoSize, HiSize);
		
			Saved = Small.Data[LoSize];
			*(Unit*)&Small.Data[LoSize] = 0;
			Z.Copy(Small.Data, LoSize + 1);	// +1 makes sure it is not negative
			*(Unit*)&Small.Data[LoSize] = Saved;
		}

		BigInteger P, Q, R;
		// Do R first
		BigInteger::Add(W, X, P);
		BigInteger::Add(Y, Z, Q);
		BigInteger::Mul(P, Q, R);
		// Do P and Q
		BigInteger::Mul(W, Y, P);
		BigInteger::Mul(X, Z, Q);
		// R = (R - P - Q) << 2
		BigInteger::Sub(R, P, R);
		BigInteger::Sub(R, Q, R);
		R._DataShiftLeft(LoSize);
		// P = P << 4
		P._DataShiftLeft(LoSize << 1);
		// A * B = P* + *R + Q
		BigInteger::Add(P, R, Product);
		BigInteger::Add(Product, Q, Product);
	}
}

void BigInteger::Div(const BigInteger & Dividend, const BigInteger & Divisor, BigInteger & Quo, BigInteger & Rem)
{
	if (Divisor.IsZero())
	{
		// Raise a continuable exception
		RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO, 0, 0, 0);
		Quo = 0;
        Rem = 0;
		return;
	}

	if (Dividend.IsZero())
	{
		Quo = 0;
		Rem = 0;
		return;
	}

	// Check duplicate args

	if (&Quo == &Rem)
	{
		// This call should throw away the remainder
		BigInteger newRem;
		BigInteger::Div(Dividend, Divisor, Quo, newRem);
		return;
	}

	if (&Dividend == &Quo || &Divisor == &Quo)
	{
		BigInteger newVal;
		BigInteger::Div(Dividend, Divisor, newVal, Rem);
		newVal.MoveTo(Quo);
		return;
	}

	if (&Dividend == &Rem || &Divisor == &Rem)
	{
		BigInteger newVal;
		BigInteger::Div(Dividend, Divisor, Quo, newVal);
		newVal.MoveTo(Rem);
		return;
	}

	// Check negativity
	if (Dividend.IsNegative())
	{
		BigInteger newVal1 = Dividend;
		newVal1.Neg();
		if (Divisor.IsNegative())
		{
			BigInteger newVal2 = Divisor;
			newVal2.Neg();
			BigInteger::Div(newVal1, newVal2, Quo, Rem);
			Rem.Neg();
			return;	// Double negative, don't touch Quo
		}
		else
		{
			BigInteger::Div(newVal1, Divisor, Quo, Rem);
			BigInteger::Sub(Divisor, Rem, Rem);
			Quo.Neg();
			return;
		}
	}
	else if (Divisor.IsNegative())
	{
		BigInteger newVal2 = Divisor;
		newVal2.Neg();
		BigInteger::Div(Dividend, newVal2, Quo, Rem);
		if (!Rem.IsZero())
			BigInteger::Add(Rem, Divisor, Rem);
		Quo.Neg();
		return;
	}

	if (Divisor > Dividend)
	{
		Quo = 0;
		Rem = Dividend;
		return;
	}

	if (Divisor.SigBit <= 32)	// Small method?
	{
		Quo._Alloc(Dividend.UnitLen); // Rem stays = 0, UnitLen = 1
		unsigned Len = Dividend.UnitLen;
		Unit Divs = Divisor.Data[0];

		__asm
		{
			mov ecx, Len
			// Get Dividend.Data[Len] and Quo.Data[Len]
			mov edx, Len
			shl edx, 2		// 4 bytes
			mov edi, Dividend
			add edi, BigInteger::Data
			add edi, edx
			mov esi, Quo
			add esi, BigInteger::Data
			add esi, edx
			mov eax, 0
			mov edx, 0
			mov ebx, Divs
		DivLoop:
			sub esi, 4	// 4 bytes
			sub edi, 4  // 4 bytes
			mov eax, DWORD PTR[edi]
			div ebx
			mov DWORD PTR [esi], eax
			loop DivLoop
			mov Divs, edx	// Remainder
		}

		Rem.Data[0] = Divs;
		if (Divs & 0x80000000)
		{
			Rem.Data[1] = 0;	// Leading zero, to make the number positive
			Rem.UnitLen = 2;
		}
		else
		{
			Rem.UnitLen = 1;
		}

		Rem._FixSigBit();
		Quo._FixSigBit();
	}
	else
	{
		// Slow method using individual bits
		Quo = 0;
		Quo._ReAlloc(Dividend.GetSigUnitLen());
		Rem = 0;
		Rem._ReAlloc(Divisor.GetSigUnitLen());
		unsigned Bit = Dividend.SigBit;
		--Bit;	// Actual index

		while(Rem < Divisor)
		{
			Rem.ShiftLeft();
			if (Dividend.GetBit(Bit))
			{
				Rem.Data[0] |= 0x1;
				// Quick fix
				if (Rem.SigBit == 0)
					Rem.SigBit = 1;
			}
			--Bit;
		}
		++Bit;

		while(true)
		{
			if (Rem < Divisor)
			{
				if (Bit == 0)
					break;
				--Bit;
				Rem.ShiftLeft();
				if (Dividend.GetBit(Bit))
				{
					Rem.Data[0] |= 0x1;
					// Quick fix
					if (Rem.SigBit == 0)
						Rem.SigBit = 1;
				}
			}
			else
			{
				BigInteger::Sub(Rem, Divisor, Rem);
				// Set bit in quotient
				unsigned UIndex = Bit / (sizeof(Unit) * 8);
				unsigned BIndex = Bit % (sizeof(Unit) * 8);
				Quo.Data[UIndex] |= (0x1 << BIndex);
			}
		}
		// No need to fix Rem
		Quo._FixSigBit();
	}
}

void BigInteger::Mod(const BigInteger & Dividend, const BigInteger & Divisor, BigInteger & Modulus)
{
	if (Divisor.IsZero())
	{
		// Raise a continuable exception
		RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO, 0, 0, 0);
		Modulus = 0;
		return;
	}

	if (Dividend.IsZero())
	{
		Modulus = 0;
		return;
	}

	// Check duplicate args

	if (&Dividend == &Modulus || &Divisor == &Modulus)
	{
		BigInteger newVal;
		BigInteger::Mod(Dividend, Divisor, newVal);
		newVal.MoveTo(Modulus);
		return;
	}

	// Check negativity
	if (Dividend.IsNegative())
	{
		BigInteger newVal1 = Dividend;
		newVal1.Neg();
		if (Divisor.IsNegative())
		{
			BigInteger newVal2 = Divisor;
			newVal2.Neg();
			BigInteger::Mod(newVal1, newVal2, Modulus);
			Modulus.Neg();
			return;	// Double negative, don't touch Quo
		}
		else
		{
			BigInteger::Mod(newVal1, Divisor, Modulus);
			BigInteger::Sub(Divisor, Modulus, Modulus);
			return;
		}
	}
	else if (Divisor.IsNegative())
	{
		BigInteger newVal2 = Divisor;
		newVal2.Neg();
		BigInteger::Mod(Dividend, newVal2, Modulus);
		if (!Modulus.IsZero())
			BigInteger::Add(Modulus, Divisor, Modulus);
		return;
	}

	if (Divisor > Dividend)
	{
		Modulus = Dividend;
		return;
	}

	if (Divisor.SigBit <= 32)	// Small method?
	{
		unsigned Len = Dividend.UnitLen;
		Unit Divs = Divisor.Data[0];

		__asm
		{
			mov ecx, Len
			// Get Dividend.Data[Len]
			mov edx, Len
			shl edx, 2		// 4 bytes
			mov edi, Dividend
			add edi, BigInteger::Data
			add edi, edx
			mov eax, 0
			mov edx, 0
			mov ebx, Divs
		DivLoop:
			sub edi, 4  // 4 bytes
			mov eax, DWORD PTR[edi]
			div ebx
			loop DivLoop
			mov Divs, edx	// Remainder
		}

		Modulus.Data[0] = Divs;
		if (Divs & 0x80000000)
		{
			// Make sure the number stays positive
			Modulus.Data[1] = 0;
			Modulus.UnitLen = 2;
		}
		else
			Modulus.UnitLen = 1;

		Modulus._FixSigBit();
	}
	else
	{
		// Slow method using individual bits
		Modulus = 0;
		Modulus._ReAlloc(Divisor.UnitLen);
		unsigned Bit = Dividend.SigBit;
		--Bit;	// Actual index

		while(Modulus < Divisor)
		{
			Modulus.ShiftLeft();
			if (Dividend.GetBit(Bit))
			{
				Modulus.Data[0] |= 0x1;
				// Quick fix
				if (Modulus.SigBit == 0)
					Modulus.SigBit = 1;
			}
			--Bit;
		}
		++Bit;

		while(true)
		{
			if (Modulus < Divisor)
			{
				if (Bit == 0)
					break;
				--Bit;
				Modulus.ShiftLeft();
				if (Dividend.GetBit(Bit))
				{
					Modulus.Data[0] |= 0x1;
					// Quick fix
					if (Modulus.SigBit == 0)
						Modulus.SigBit = 1;
				}
			}
			else
			{
				BigInteger::Sub(Modulus, Divisor, Modulus);
			}
		}
		// No need to fix Rem
	}
}

void BigInteger::Exp(const BigInteger & Factor, const BigInteger & Exponent, BigInteger & Product)
{
	if (Exponent.IsZero())
	{
		Product = 1;
		return;
	}

	if (Exponent.IsNegative() || Factor.IsZero())
	{
		Product = 0;
		return;
	}

	if (Factor.IsNegative())
	{
		bool NoSwitch = Factor.IsEven();
		BigInteger newVal(Factor);
		newVal.Neg();
		BigInteger::Exp(newVal, Exponent, Product);
		if (!NoSwitch)
			Product.Neg();
		return;
	}

	if (&Factor == &Product || &Exponent == & Product)
	{
		BigInteger newVal;
		BigInteger::Exp(newVal, Exponent, newVal);
		newVal.MoveTo(Product);
		return;
	}

	BigInteger Power(Factor);
	Product = 1;
	int Bit = 0;

	while(Bit < Exponent.SigBit)
	{
		if (Exponent.GetBit(Bit))
			BigInteger::Mul(Product, Power, Product);
		BigInteger::Mul(Power, Power, Power);
		Bit++;
	}
}

void BigInteger::ExpMod(const BigInteger & Factor, const BigInteger & Exponent, const BigInteger & Mod, BigInteger & Product)
{
	if (Exponent.IsZero())
	{
		Product = 1;
		return;
	}

	if (Exponent.IsNegative() || Factor.IsZero())
	{
		Product = 0;
		return;
	}

	if (Factor.IsNegative())
	{
		bool NoSwitch = Factor.IsEven();
		BigInteger newVal(Factor);
		newVal.Neg();
		BigInteger::Exp(newVal, Exponent, Product);
		if (!NoSwitch)
			Product.Neg();
		return;
	}

	if (&Factor == &Product || &Exponent == &Product || &Mod == &Product)
	{
		BigInteger newVal;
		BigInteger::Exp(newVal, Exponent, newVal);
		newVal.MoveTo(Product);
		return;
	}

	BigInteger Power, Quo;	// Quo is thrown away
	BigInteger::Div(Factor, Mod, Quo, Power);
	Product = 1;
	int Bit = 0;

	while(Bit < Exponent.SigBit)
	{
		if (Exponent.GetBit(Bit))
		{
			BigInteger::Mul(Product, Power, Product);
			BigInteger::Div(Product, Mod, Quo, Product);
		}
		BigInteger::Mul(Power, Power, Power);
		BigInteger::Div(Power, Mod, Quo, Power);
		Bit++;
	}
}


